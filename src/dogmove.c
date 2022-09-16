/* NetHack 3.7	dogmove.c	$NHDT-Date: 1646688063 2022/03/07 21:21:03 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.112 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include "mfndpos.h"

#define DOG_HUNGRY      300
#define DOG_WEAK        500
#define DOG_STARVE      750

static void dog_starve(struct monst *);
static boolean dog_hunger(struct monst *, struct edog *);
static int dog_invent(struct monst *, struct edog *, int);
static int dog_goal(struct monst *, struct edog *, int, int, int);
static struct monst *find_targ(struct monst *, int, int, int);
static int find_friends(struct monst *, struct monst *, int);
static struct monst *best_target(struct monst *);
static long score_targ(struct monst *, struct monst *);
static boolean can_reach_location(struct monst *, coordxy, coordxy, coordxy,
                                  coordxy);
static boolean could_reach_item(struct monst *, coordxy, coordxy);
static void quickmimic(struct monst *);

/* pick a carried item for pet to drop */
struct obj *
droppables(struct monst *mon)
{
    /*
     * 'key|pickaxe|&c = &dummy' is used to make various creatures
     * that can't use a key/pick-axe/&c behave as if they are already
     * holding one so that any other such item in their inventory will
     * be considered a duplicate and get treated as a normal candidate
     * for dropping.
     *
     * This could be 'auto', but then 'gcc -O2' warns that this function
     * might return the address of a local variable.  It's mistaken,
     * &dummy is never returned.  'static' is simplest way to shut it up.
     */
    static struct obj dummy;
    struct obj *obj, *wep, *pickaxe, *unihorn, *key;

    dummy = cg.zeroobj;
    dummy.otyp = GOLD_PIECE; /* not STRANGE_OBJECT or tools of interest */
    dummy.oartifact = 1; /* so real artifact won't override "don't keep it" */
    pickaxe = unihorn = key = (struct obj *) 0;
    wep = MON_WEP(mon);

    if (is_animal(mon->data) || mindless(mon->data)) {
        /* won't hang on to any objects of these types */
        pickaxe = unihorn = key = &dummy; /* act as if already have them */
    } else {
        /* don't hang on to pick-axe if can't use one or don't need one */
        if (!tunnels(mon->data) || !needspick(mon->data))
            pickaxe = &dummy;
        /* don't hang on to key if can't open doors */
        if (nohands(mon->data) || verysmall(mon->data))
            key = &dummy;
    }
    if (wep) {
        if (is_pick(wep))
            pickaxe = wep;
        if (wep->otyp == UNICORN_HORN)
            unihorn = wep;
        /* don't need any wielded check for keys... */
    }

    for (obj = mon->minvent; obj; obj = obj->nobj) {
        switch (obj->otyp) {
        case DWARVISH_MATTOCK:
            /* reject mattock if couldn't wield it */
            if (which_armor(mon, W_ARMS))
                break;
            /* keep mattock in preference to pick unless pick is already
               wielded or is an artifact and mattock isn't */
            if (pickaxe && pickaxe->otyp == PICK_AXE && pickaxe != wep
                && (!pickaxe->oartifact || obj->oartifact))
                return pickaxe; /* drop the one we earlier decided to keep */
        /*FALLTHRU*/
        case PICK_AXE:
            if (!pickaxe || (obj->oartifact && !pickaxe->oartifact)) {
                if (pickaxe)
                    return pickaxe;
                pickaxe = obj; /* keep this digging tool */
                continue;
            }
            break;

        case UNICORN_HORN:
            /* reject cursed unicorn horns */
            if (obj->cursed)
                break;
            /* keep artifact unihorn in preference to ordinary one */
            if (!unihorn || (obj->oartifact && !unihorn->oartifact)) {
                if (unihorn)
                    return unihorn;
                unihorn = obj; /* keep this unicorn horn */
                continue;
            }
            break;

        case SKELETON_KEY:
            /* keep key in preference to lock-pick */
            if (key && key->otyp == LOCK_PICK
                && (!key->oartifact || obj->oartifact))
                return key; /* drop the one we earlier decided to keep */
        /*FALLTHRU*/
        case LOCK_PICK:
            /* keep lock-pick in preference to credit card */
            if (key && key->otyp == CREDIT_CARD
                && (!key->oartifact || obj->oartifact))
                return key;
        /*FALLTHRU*/
        case CREDIT_CARD:
            if (!key || (obj->oartifact && !key->oartifact)) {
                if (key)
                    return key;
                key = obj; /* keep this unlocking tool */
                continue;
            }
            break;

        default:
            break;
        }

        if (!obj->owornmask && obj != wep)
            return obj;
    }

    return (struct obj *) 0; /* don't drop anything */
}

static NEARDATA const char nofetch[] = { BALL_CLASS, CHAIN_CLASS, ROCK_CLASS,
                                         0 };


static void wantdoor(coordxy, coordxy, genericptr_t);

boolean
cursed_object_at(coordxy x, coordxy y)
{
    struct obj *otmp;

    for (otmp = g.level.objects[x][y]; otmp; otmp = otmp->nexthere)
        if (otmp->cursed)
            return TRUE;
    return FALSE;
}

int
dog_nutrition(struct monst *mtmp, struct obj *obj)
{
    int nutrit;

    /*
     * It is arbitrary that the pet takes the same length of time to eat
     * as a human, but gets more nutritional value.
     */
    if (obj->oclass == FOOD_CLASS) {
        if (obj->otyp == CORPSE) {
            mtmp->meating = 3 + (mons[obj->corpsenm].cwt >> 6);
            nutrit = mons[obj->corpsenm].cnutrit;
        } else {
            mtmp->meating = objects[obj->otyp].oc_delay;
            nutrit = objects[obj->otyp].oc_nutrition;
        }
        switch (mtmp->data->msize) {
        case MZ_TINY:
            nutrit *= 8;
            break;
        case MZ_SMALL:
            nutrit *= 6;
            break;
        default:
        case MZ_MEDIUM:
            nutrit *= 5;
            break;
        case MZ_LARGE:
            nutrit *= 4;
            break;
        case MZ_HUGE:
            nutrit *= 3;
            break;
        case MZ_GIGANTIC:
            nutrit *= 2;
            break;
        }
        if (obj->oeaten) {
            mtmp->meating = eaten_stat(mtmp->meating, obj);
            nutrit = eaten_stat(nutrit, obj);
        }
    } else if (obj->oclass == COIN_CLASS) {
        mtmp->meating = (int) (obj->quan / 2000) + 1;
        if (mtmp->meating < 0)
            mtmp->meating = 1;
        nutrit = (int) (obj->quan / 20);
        if (nutrit < 0)
            nutrit = 0;
    } else {
        /* Unusual pet such as gelatinous cube eating odd stuff.
         * meating made consistent with wild monsters in mon.c.
         * nutrit made consistent with polymorphed player nutrit in
         * eat.c.  (This also applies to pets eating gold.)
         */
        mtmp->meating = obj->owt / 20 + 1;
        nutrit = 5 * objects[obj->otyp].oc_nutrition;
    }
    return nutrit;
}

/* returns 2 if pet dies, otherwise 1 */
int
dog_eat(struct monst *mtmp,
        struct obj *obj, /* if unpaid, then thrown or kicked by hero */
        coordxy x,       /* dog's starting location, */
        coordxy y,       /* might be different from current */
        boolean devour)
{
    register struct edog *edog = EDOG(mtmp);
    boolean poly, grow, heal, eyes, slimer, deadmimic;
    int nutrit, res, corpsenm;
    long oprice;
    char objnambuf[BUFSZ], *obj_name;

    objnambuf[0] = '\0';
    if (edog->hungrytime < g.moves)
        edog->hungrytime = g.moves;
    nutrit = dog_nutrition(mtmp, obj);

    deadmimic = (obj->otyp == CORPSE && (obj->corpsenm == PM_SMALL_MIMIC
                                         || obj->corpsenm == PM_LARGE_MIMIC
                                         || obj->corpsenm == PM_GIANT_MIMIC));
    slimer = (obj->otyp == GLOB_OF_GREEN_SLIME);
    poly = polyfodder(obj);
    grow = mlevelgain(obj);
    heal = mhealup(obj);
    eyes = (obj->otyp == CARROT);
    corpsenm = (obj->otyp == CORPSE ? obj->corpsenm : NON_PM);

    if (devour) {
        if (mtmp->meating > 1)
            mtmp->meating /= 2;
        if (nutrit > 1)
            nutrit = (nutrit * 3) / 4;
    }
    edog->hungrytime += nutrit;
    mtmp->mconf = 0;
    if (edog->mhpmax_penalty) {
        /* no longer starving */
        mtmp->mhpmax += edog->mhpmax_penalty;
        edog->mhpmax_penalty = 0;
    }
    if (mtmp->mflee && mtmp->mfleetim > 1)
        mtmp->mfleetim /= 2;
    if (mtmp->mtame < 20)
        mtmp->mtame++;
    if (x != mtmp->mx || y != mtmp->my) { /* moved & ate on same turn */
        newsym(x, y);
        newsym(mtmp->mx, mtmp->my);
    }
    if (mtmp->data == &mons[PM_KILLER_BEE]
        && obj->otyp == LUMP_OF_ROYAL_JELLY
        && (res = bee_eat_jelly(mtmp, obj)) >= 0)
        /* bypass most of dog_eat(), including apport update */
        return (res + 1); /* 1 -> 2, 0 -> 1; -1, keep going */

    /* food items are eaten one at a time; entire stack for other stuff */
    if (obj->quan > 1L && obj->oclass == FOOD_CLASS)
        obj = splitobj(obj, 1L);
    if (obj->unpaid)
        iflags.suppress_price++;
    if (is_pool(x, y) && !Underwater) {
        /* Don't print obj */
        /* TODO: Reveal presence of sea monster (especially sharks) */
    } else {
        /* food is at monster's current location, <mx,my>;
           <x,y> was monster's location at start of this turn;
           they might be the same but will be different when
           the monster is moving+eating on same turn */
        boolean seeobj = cansee(mtmp->mx, mtmp->my),
                sawpet = cansee(x, y) && mon_visible(mtmp);

        /* Observe the action if either the food location or the pet
           itself is in view.  When pet which was in view moves to an
           unseen spot to eat the food there, avoid referring to that
           pet as "it".  However, we want "it" if invisible/unsensed
           pet eats visible food. */
        if (sawpet || (seeobj && canspotmon(mtmp))) {
            /* call distant_name() for possible side-effects even if the
               result won't be printed */
            obj_name = distant_name(obj, doname);
            if (tunnels(mtmp->data))
                pline("%s digs in.", noit_Monnam(mtmp));
            else
                pline("%s %s %s.", noit_Monnam(mtmp),
                      devour ? "devours" : "eats", obj_name);
        } else if (seeobj) {
            obj_name = distant_name(obj, doname);
            pline("It %s %s.", devour ? "devours" : "eats", obj_name);
        }
    }
    if (obj->unpaid) {
        Strcpy(objnambuf, xname(obj));
        iflags.suppress_price--;
    }
    /* some monsters that eat items could eat a container with contents */
    if (Has_contents(obj))
        meatbox(mtmp, obj);
    /* It's a reward if it's DOGFOOD and the player dropped/threw it.
       We know the player had it if invlet is set. -dlc */
    if (dogfood(mtmp, obj) == DOGFOOD && obj->invlet)
#ifdef LINT
        edog->apport = 0;
#else
        edog->apport += (int) (200L / ((long) edog->dropdist + g.moves
                                       - edog->droptime));
#endif
    if (mtmp->data == &mons[PM_RUST_MONSTER] && obj->oerodeproof) {
        /* The object's rustproofing is gone now */
        if (obj->unpaid)
            costly_alteration(obj, COST_DEGRD);
        obj->oerodeproof = 0;
        mtmp->mstun = 1;
        if (canseemon(mtmp)) {
            obj_name = distant_name(obj, doname); /* (see above) */
            if (Verbose(0, dog_eat))
                pline("%s spits %s out in disgust!",
                      Monnam(mtmp), obj_name);
        }
    } else if (obj == uball) {
        unpunish();
        delobj(obj); /* we assume this can't be unpaid */
    } else if (obj == uchain) {
        unpunish();
    } else {
        if (obj->unpaid) {
            /* edible item owned by shop has been thrown or kicked
               by hero and caught by tame or food-tameable monst */
            oprice = unpaid_cost(obj, TRUE);
            pline("That %s will cost you %ld %s.", objnambuf, oprice,
                  currency(oprice));
            /* delobj->obfree will handle actual shop billing update */
        }
        delobj(obj);
    }

#if 0 /* pet is eating, so slime recovery is not feasible... */
    /* turning into slime might be cureable */
    if (slimer && munslime(mtmp, FALSE)) {
        /* but the cure (fire directed at self) might be fatal */
        if (DEADMONSTER(mtmp))
            return 2;
        slimer = FALSE; /* sliming is avoided, skip polymorph */
    }
#endif

    if (poly || slimer) {
        struct permonst *ptr = slimer ? &mons[PM_GREEN_SLIME] : 0;

        (void) newcham(mtmp, ptr,
                       cansee(mtmp->mx, mtmp->my) ? NC_SHOW_MSG : 0);
    }

    /* limit "instant" growth to prevent potential abuse */
    if (grow && (int) mtmp->m_lev < (int) mtmp->data->mlevel + 15) {
        if (!grow_up(mtmp, (struct monst *) 0))
            return 2;
    }
    if (heal)
        mtmp->mhp = mtmp->mhpmax;
    if ((eyes || heal) && !mtmp->mcansee)
        mcureblindness(mtmp, canseemon(mtmp));
    if (deadmimic)
        quickmimic(mtmp);
    if (corpsenm != NON_PM)
        mon_givit(mtmp, &mons[corpsenm]);
    return 1;
}

static void
dog_starve(struct monst *mtmp)
{
    if (mtmp->mleashed && mtmp != u.usteed)
        Your("leash goes slack.");
    else if (cansee(mtmp->mx, mtmp->my))
        pline("%s starves.", Monnam(mtmp));
    else
        You_feel("%s for a moment.",
                    Hallucination ? "bummed" : "sad");
    mondied(mtmp);
}

/* hunger effects -- returns TRUE on starvation */
static boolean
dog_hunger(struct monst *mtmp, struct edog *edog)
{
    if (g.moves > edog->hungrytime + DOG_WEAK) {
        if (!carnivorous(mtmp->data) && !herbivorous(mtmp->data)) {
            edog->hungrytime = g.moves + DOG_WEAK;
            /* but not too high; it might polymorph */
        } else if (!edog->mhpmax_penalty) {
            /* starving pets are limited in healing */
            int newmhpmax = mtmp->mhpmax / 3;
            mtmp->mconf = 1;
            edog->mhpmax_penalty = mtmp->mhpmax - newmhpmax;
            mtmp->mhpmax = newmhpmax;
            if (mtmp->mhp > mtmp->mhpmax)
                mtmp->mhp = mtmp->mhpmax;
            if (DEADMONSTER(mtmp)) {
                dog_starve(mtmp);
                return TRUE;
            }
            if (cansee(mtmp->mx, mtmp->my))
                pline("%s is confused from hunger.", Monnam(mtmp));
            else if (couldsee(mtmp->mx, mtmp->my))
                beg(mtmp);
            else
                You_feel("worried about %s.", y_monnam(mtmp));
            stop_occupation();
        } else if (g.moves > edog->hungrytime + DOG_STARVE
                   || DEADMONSTER(mtmp)) {
            dog_starve(mtmp);
            return TRUE;
        }
    }
    return FALSE;
}

/* do something with object (drop, pick up, eat) at current position
 * returns 1 if object eaten (since that counts as dog's move), 2 if died
 */
static int
dog_invent(struct monst *mtmp, struct edog *edog, int udist)
{
    int omx, omy, carryamt = 0;
    struct obj *obj, *otmp;

    if (helpless(mtmp))
        return 0;

    omx = mtmp->mx;
    omy = mtmp->my;

    /* If we are carrying something then we drop it (perhaps near @).
     * Note: if apport == 1 then our behavior is independent of udist.
     * Use udist+1 so steed won't cause divide by zero.
     */
    if (droppables(mtmp)) {
        if (!rn2(udist + 1) || !rn2(edog->apport))
            if (rn2(10) < edog->apport) {
                relobj(mtmp, (int) mtmp->minvis, TRUE);
                if (edog->apport > 1)
                    edog->apport--;
                edog->dropdist = udist; /* hpscdi!jon */
                edog->droptime = g.moves;
            }
    } else {
        if ((obj = g.level.objects[omx][omy]) != 0
            && !index(nofetch, obj->oclass)
#ifdef MAIL_STRUCTURES
            && obj->otyp != SCR_MAIL
#endif
            /* avoid special items; once hero picks them up, they'll cease
               being special and become eligible for normal monst activity */
            && !(is_mines_prize(obj) || is_soko_prize(obj))) {
            int edible = dogfood(mtmp, obj);

            if ((edible <= CADAVER
                 /* starving pet is more aggressive about eating */
                 || (edog->mhpmax_penalty && edible == ACCFOOD))
                && could_reach_item(mtmp, obj->ox, obj->oy))
                return dog_eat(mtmp, obj, omx, omy, FALSE);

            carryamt = can_carry(mtmp, obj);
            if (carryamt > 0 && !obj->cursed
                && could_reach_item(mtmp, obj->ox, obj->oy)) {
                if (rn2(20) < edog->apport + 3) {
                    if (rn2(udist) || !rn2(edog->apport)) {
                        otmp = obj;
                        if (carryamt != obj->quan)
                            otmp = splitobj(obj, carryamt);
                        if (cansee(omx, omy)) {
                            /* call distant_name() for possible side-effects
                               even if the result won't be printed; should be
                               done before extract+pickup for distant_name()
                               -> doname() -> xname() -> find_artifact()
                               while otmp is still on floor */
                            char *otmpname = distant_name(otmp, doname);

                            if (Verbose(0, dog_invent))
                                pline("%s picks up %s.",
                                      Monnam(mtmp), otmpname);
                        }
                        obj_extract_self(otmp);
                        newsym(omx, omy);
                        (void) mpickobj(mtmp, otmp);
                        if (attacktype(mtmp->data, AT_WEAP)
                            && mtmp->weapon_check == NEED_WEAPON) {
                            mtmp->weapon_check = NEED_HTH_WEAPON;
                            (void) mon_wield_item(mtmp);
                        }
                        check_gear_next_turn(mtmp);
                    }
                }
            }
        }
    }
    return 0;
}

/* set dog's goal -- gtyp, gx, gy;
   returns -1/0/1 (dog's desire to approach player) or -2 (abort move) */
static int
dog_goal(register struct monst *mtmp, struct edog *edog,
         int after, int udist, int whappr)
{
    register coordxy omx, omy;
    boolean in_masters_sight, dog_has_minvent;
    register struct obj *obj;
    xint16 otyp;
    int appr;

    /* Steeds don't move on their own will */
    if (mtmp == u.usteed)
        return -2;

    omx = mtmp->mx;
    omy = mtmp->my;

    in_masters_sight = couldsee(omx, omy);
    dog_has_minvent = (droppables(mtmp) != 0);

    if (!edog || mtmp->mleashed) { /* he's not going anywhere... */
        g.gtyp = APPORT;
        g.gx = u.ux;
        g.gy = u.uy;
    } else {
#define DDIST(x, y) (dist2(x, y, omx, omy))
#define SQSRCHRADIUS 5
        int min_x, max_x, min_y, max_y;
        coordxy nx, ny;

        g.gtyp = UNDEF; /* no goal as yet */
        g.gx = g.gy = 0;  /* suppress 'used before set' message */

        if ((min_x = omx - SQSRCHRADIUS) < 1)
            min_x = 1;
        if ((max_x = omx + SQSRCHRADIUS) >= COLNO)
            max_x = COLNO - 1;
        if ((min_y = omy - SQSRCHRADIUS) < 0)
            min_y = 0;
        if ((max_y = omy + SQSRCHRADIUS) >= ROWNO)
            max_y = ROWNO - 1;

        /* nearby food is the first choice, then other objects */
        for (obj = fobj; obj; obj = obj->nobj) {
            nx = obj->ox;
            ny = obj->oy;
            if (nx >= min_x && nx <= max_x && ny >= min_y && ny <= max_y) {
                otyp = dogfood(mtmp, obj);
                /* skip inferior goals */
                if (otyp > g.gtyp || otyp == UNDEF)
                    continue;
                /* avoid cursed items unless starving */
                if (cursed_object_at(nx, ny)
                    && !(edog->mhpmax_penalty && otyp < MANFOOD))
                    continue;
                /* skip completely unreachable goals */
                if (!could_reach_item(mtmp, nx, ny)
                    || !can_reach_location(mtmp, mtmp->mx, mtmp->my, nx, ny))
                    continue;
                if (otyp < MANFOOD) {
                    if (otyp < g.gtyp || DDIST(nx, ny) < DDIST(g.gx, g.gy)) {
                        g.gx = nx;
                        g.gy = ny;
                        g.gtyp = otyp;
                    }
                } else if (g.gtyp == UNDEF && in_masters_sight
                           && !dog_has_minvent
                           && (!levl[omx][omy].lit || levl[u.ux][u.uy].lit)
                           && (otyp == MANFOOD || m_cansee(mtmp, nx, ny))
                           && edog->apport > rn2(8)
                           && can_carry(mtmp, obj) > 0) {
                    g.gx = nx;
                    g.gy = ny;
                    g.gtyp = APPORT;
                }
            }
        }
    }

    /* follow player if appropriate */
    if (g.gtyp == UNDEF || (g.gtyp != DOGFOOD && g.gtyp != APPORT
                          && g.moves < edog->hungrytime)) {
        g.gx = u.ux;
        g.gy = u.uy;
        if (after && udist <= 4 && u_at(g.gx, g.gy))
            return -2;
        appr = (udist >= 9) ? 1 : (mtmp->mflee) ? -1 : 0;
        if (udist > 1) {
            if (!IS_ROOM(levl[u.ux][u.uy].typ) || !rn2(4) || whappr
                || (dog_has_minvent && rn2(edog->apport)))
                appr = 1;
        }
        /* if you have dog food it'll follow you more closely; if you are
           on stairs (or ladder) or on or next to a magic portal, it will
           behave as if you have dog food */
        if (appr == 0) {
            if (On_stairs(u.ux, u.uy)) {
                appr = 1;
            } else {
                for (obj = g.invent; obj; obj = obj->nobj)
                    if (dogfood(mtmp, obj) == DOGFOOD) {
                        appr = 1;
                        break;
                    }
                if (appr == 0) {
                    struct trap *t;

                    /* assume at most one magic portal per level;
                       [should this be limited to known portals?] */
                    for (t = g.ftrap; t; t = t->ntrap)
                        if (t->ttyp == MAGIC_PORTAL) {
                            if (/*t->tseen &&*/ distu(t->tx, t->ty) <= 2)
                                appr = 1;
                            break;
                        }
                }
            }
        }
    } else
        appr = 1; /* gtyp != UNDEF */
    if (mtmp->mconf)
        appr = 0;

#define FARAWAY (COLNO + 2) /* position outside screen */
    if (u_at(g.gx, g.gy) && !in_masters_sight) {
        register coord *cp;

        cp = gettrack(omx, omy);
        if (cp) {
            g.gx = cp->x;
            g.gy = cp->y;
            if (edog)
                edog->ogoal.x = 0;
        } else {
            /* assume master hasn't moved far, and reuse previous goal */
            if (edog && edog->ogoal.x
                && (edog->ogoal.x != omx || edog->ogoal.y != omy)) {
                g.gx = edog->ogoal.x;
                g.gy = edog->ogoal.y;
                edog->ogoal.x = 0;
            } else {
                int fardist = FARAWAY * FARAWAY;
                g.gx = g.gy = FARAWAY; /* random */
                do_clear_area(omx, omy, 9, wantdoor, (genericptr_t) &fardist);

                /* here gx == FARAWAY e.g. when dog is in a vault */
                if (g.gx == FARAWAY || (g.gx == omx && g.gy == omy)) {
                    g.gx = u.ux;
                    g.gy = u.uy;
                } else if (edog) {
                    edog->ogoal.x = g.gx;
                    edog->ogoal.y = g.gy;
                }
            }
        }
    } else if (edog) {
        edog->ogoal.x = 0;
    }
    return appr;
}

static struct monst *
find_targ(register struct monst *mtmp, int dx, int dy, int maxdist)
{
    struct monst *targ = 0;
    int curx = mtmp->mx, cury = mtmp->my;
    int dist = 0;

    /* Walk outwards */
    for ( ; dist < maxdist; ++dist) {
        curx += dx;
        cury += dy;
        if (!isok(curx, cury))
            break;

        /* FIXME: Check if we hit a wall/door/boulder to
         *        short-circuit unnecessary subsequent checks
         */

        /* If we can't see up to here, forget it - will this
         * mean pets in corridors don't breathe at monsters
         * in rooms? If so, is that necessarily bad?
         */
        if (!m_cansee(mtmp, curx, cury))
            break;

        if (curx == mtmp->mux && cury == mtmp->muy)
            return &g.youmonst;

        if ((targ = m_at(curx, cury)) != 0) {
            /* Is the monster visible to the pet? */
            if ((!targ->minvis || perceives(mtmp->data))
                && !targ->mundetected)
                break;
            /* If the pet can't see it, it assumes it aint there */
            targ = 0;
        }
    }
    return targ;
}

static int
find_friends(struct monst *mtmp, struct monst *mtarg, int maxdist)
{
    struct monst *pal;
    int dx = sgn(mtarg->mx - mtmp->mx),
        dy = sgn(mtarg->my - mtmp->my);
    int curx = mtarg->mx, cury = mtarg->my;
    int dist = distmin(mtarg->mx, mtarg->my, mtmp->mx, mtmp->my);

    for ( ; dist <= maxdist; ++dist) {
        curx += dx;
        cury += dy;

        if (!isok(curx, cury))
            return 0;

        /* If the pet can't see beyond this point, don't
         * check any farther
         */
        if (!m_cansee(mtmp, curx, cury))
            return 0;

        /* Does pet think you're here? */
        if (mtmp->mux == curx && mtmp->muy == cury)
            return 1;

        pal = m_at(curx, cury);

        if (pal) {
            if (pal->mtame) {
                /* Pet won't notice invisible pets */
                if (!pal->minvis || perceives(mtmp->data))
                    return 1;
            } else {
                /* Quest leaders and guardians are always seen */
                if (pal->data->msound == MS_LEADER
                    || pal->data->msound == MS_GUARDIAN)
                    return 1;
            }
        }
    }
    return 0;
}

static long
score_targ(struct monst *mtmp, struct monst *mtarg)
{
    long score = 0L;

    /* If the monster is confused, normal scoring is disrupted -
     * anything may happen
     */

    /* Give 1 in 3 chance of safe breathing even if pet is confused or
     * if you're on the quest start level */
    if (!mtmp->mconf || !rn2(3) || Is_qstart(&u.uz)) {
        int mtmp_lev;
        aligntyp align1 = A_NONE, align2 = A_NONE; /* For priests, minions */
        boolean faith1 = TRUE,  faith2 = TRUE;

        if (mtmp->isminion)
            align1 = EMIN(mtmp)->min_align;
        else if (mtmp->ispriest)
            align1 = EPRI(mtmp)->shralign;
        else
            faith1 = FALSE;
        if (mtarg->isminion)
            align2 = EMIN(mtarg)->min_align; /* MAR */
        else if (mtarg->ispriest)
            align2 = EPRI(mtarg)->shralign; /* MAR */
        else
            faith2 = FALSE;

        /* Never target quest friendlies */
        if (mtarg->data->msound == MS_LEADER
            || mtarg->data->msound == MS_GUARDIAN)
            return -5000L;
        /* D: Fixed angelic beings using gaze attacks on coaligned priests */
        if (faith1 && faith2 && align1 == align2 && mtarg->mpeaceful) {
            score -= 5000L;
            return score;
        }
        /* Is monster adjacent? */
        if (distmin(mtmp->mx, mtmp->my, mtarg->mx, mtarg->my) <= 1) {
            score -= 3000L;
            return score;
        }
        /* Is the monster peaceful or tame? */
        if (/*mtarg->mpeaceful ||*/ mtarg->mtame || mtarg == &g.youmonst) {
            /* Pets will never be targeted */
            score -= 3000L;
            return score;
        }
        /* Is master/pet behind monster? Check up to 15 squares beyond pet. */
        if (find_friends(mtmp, mtarg, 15)) {
            score -= 3000L;
            return score;
        }
        /* Target hostile monsters in preference to peaceful ones */
        if (!mtarg->mpeaceful)
            score += 10;
        /* Is the monster passive? Don't waste energy on it, if so */
        if (mtarg->data->mattk[0].aatyp == AT_NONE)
            score -= 1000;
        /* Even weak pets with breath attacks shouldn't take on very
           low-level monsters. Wasting breath on lichens is ridiculous. */
        if ((mtarg->m_lev < 2 && mtmp->m_lev > 5)
            || (mtmp->m_lev > 12 && mtarg->m_lev < mtmp->m_lev - 9
                && u.ulevel > 8 && mtarg->m_lev < u.ulevel - 7))
            score -= 25;
        /* for strength purposes, a vampshifter in weak form (vampire bat,
           fog cloud, maybe wolf) will attack as if in vampire form;
           otherwise if won't do much and usually wouldn't suffer enough
           damage (from counterattacks) to switch back to vampire form;
           make it be more aggressive by behaving as if stronger */
        mtmp_lev = mtmp->m_lev;
        if (is_vampshifter(mtmp) && mtmp->data->mlet != S_VAMPIRE) {
            /* is_vampshifter() implies (mtmp->cham >= LOW_PM) */
            mtmp_lev = mons[mtmp->cham].mlevel;
            /* actual vampire level would range from 1.0*mlvl to 1.5*mlvl */
            mtmp_lev += rn2(mtmp_lev / 2 + 1);
            /* we don't expect actual level in weak form to exceed
               base level of strong form, but handle that if it happens */
            if (mtmp->m_lev > mtmp_lev)
                mtmp_lev = mtmp->m_lev;
        }
        /* And pets will hesitate to attack vastly stronger foes.
           This penalty will be discarded if master's in trouble. */
        if (mtarg->m_lev > mtmp_lev + 4L)
            score -= (mtarg->m_lev - mtmp_lev) * 20L;
        /* All things being the same, go for the beefiest monster. This
           bonus should not be large enough to override the pet's aversion
           to attacking much stronger monsters. */
        score += mtarg->m_lev * 2 + mtarg->mhp / 3;
    }
    /* Fuzz factor to make things less predictable when very
       similar targets are abundant. */
    score += rnd(5);
    /* Pet may decide not to use ranged attack when confused */
    if (mtmp->mconf && !rn2(3))
        score -= 1000;
    return score;
}

static struct monst *
best_target(struct monst *mtmp)   /* Pet */
{
    int dx, dy;
    long bestscore = -40000L, currscore;
    struct monst *best_targ = 0, *temp_targ = 0;

    /* Help! */
    if (!mtmp)
        return 0;

    /* If the pet is blind, it's not going to see any target */
    if (!mtmp->mcansee)
        return 0;

    /* Search for any monsters lined up with the pet, within an arbitrary
     * distance from the pet (7 squares, even along diagonals). Monsters
     * are assigned scores and the best score is chosen.
     */
    for (dy = -1; dy < 2; ++dy) {
        for (dx = -1; dx < 2; ++dx) {
            if (!dx && !dy)
                continue;
            /* Traverse the line to find the first monster within 7
             * squares. Invisible monsters are skipped (if the
             * pet doesn't have see invisible).
             */
            temp_targ = find_targ(mtmp, dx, dy, 7);

            /* Nothing in this line? */
            if (!temp_targ)
                continue;

            /* Decide how attractive the target is */
            currscore = score_targ(mtmp, temp_targ);

            if (currscore > bestscore) {
                bestscore = currscore;
                best_targ = temp_targ;
            }
        }
    }

    /* Filter out targets the pet doesn't like */
    if (bestscore < 0L)
        best_targ = 0;

    return best_targ;
}

/* Return values (same as m_move):
 * 0: did not move, but can still attack and do other stuff.
 * 1: moved, possibly can attack.
 * 2: monster died.
 * 3: did not move, and can't do anything else either.
 *    (may have attacked something)
 */
int
dog_move(register struct monst *mtmp,
         int after) /* this is extra fast monster movement */
{
    int omx, omy; /* original mtmp position */
    int appr, whappr, udist;
    int i, j, k;
    register struct edog *edog = EDOG(mtmp);
    struct obj *obj = (struct obj *) 0;
    xint16 otyp;
    boolean has_edog, cursemsg[9], do_eat = FALSE;
    boolean better_with_displacing = FALSE;
    coordxy nix, niy;      /* position mtmp is (considering) moving to */
    coordxy nx, ny; /* temporary coordinates */
    xint16 cnt, uncursedcnt, chcnt;
    int chi = -1, nidist, ndist;
    coord poss[9];
    long info[9], allowflags;
#define GDIST(x, y) (dist2(x, y, g.gx, g.gy))

    /*
     * Tame Angels have isminion set and an ispriest structure instead of
     * an edog structure.  Fortunately, guardian Angels need not worry
     * about mundane things like eating and fetching objects, and can
     * spend all their energy defending the player.  (They are the only
     * monsters with other structures that can be tame.)
     */
    has_edog = !mtmp->isminion;

    omx = mtmp->mx;
    omy = mtmp->my;
    if (has_edog && dog_hunger(mtmp, edog))
        return MMOVE_DIED; /* starved */

    udist = distu(omx, omy);
    /* Let steeds eat and maybe throw rider during Conflict */
    if (mtmp == u.usteed) {
        if (Conflict && !resist_conflict(mtmp)) {
            dismount_steed(DISMOUNT_THROWN);
            return MMOVE_MOVED;
        }
        udist = 1;
    } else if (!udist)
        /* maybe we tamed him while being swallowed --jgm */
        return MMOVE_NOTHING;

    nix = omx; /* set before newdogpos */
    niy = omy;
    cursemsg[0] = FALSE; /* lint suppression */
    info[0] = 0;         /* ditto */

    if (has_edog) {
        j = dog_invent(mtmp, edog, udist);
        if (j == 2)
            return MMOVE_DIED; /* died */
        else if (j == 1)
            goto newdogpos; /* eating something */

        whappr = (g.moves - edog->whistletime < 5);
    } else
        whappr = 0;

    appr = dog_goal(mtmp, has_edog ? edog : (struct edog *) 0, after, udist,
                    whappr);
    if (appr == -2)
        return MMOVE_NOTHING;

    if (Conflict && !resist_conflict(mtmp)) {
        if (!has_edog) {
            /* Guardian angel refuses to be conflicted; rather,
             * it disappears, angrily, and sends in some nasties
             */
            lose_guardian_angel(mtmp);
            return MMOVE_DIED; /* current monster is gone */
        }
    }
#if 0 /* [this is now handled in dochug()] */
    if (!Conflict && !mtmp->mconf
        && mtmp == u.ustuck && !sticks(g.youmonst.data)) {
        unstuck(mtmp); /* swallowed case handled above */
        You("get released!");
    }
#endif
    allowflags = mon_allowflags(mtmp);
    cnt = mfndpos(mtmp, poss, info, allowflags);

    /* Normally dogs don't step on cursed items, but if they have no
     * other choice they will.  This requires checking ahead of time
     * to see how many uncursed item squares are around.
     */
    uncursedcnt = 0;
    for (i = 0; i < cnt; i++) {
        nx = poss[i].x;
        ny = poss[i].y;
        if (MON_AT(nx, ny) && !((info[i] & ALLOW_M) || info[i] & ALLOW_MDISP))
            continue;
        if (cursed_object_at(nx, ny))
            continue;
        uncursedcnt++;
    }

    better_with_displacing = should_displace(mtmp, poss, info, cnt,
                                             g.gx, g.gy);

    chcnt = 0;
    chi = -1;
    nidist = GDIST(nix, niy);

    for (i = 0; i < cnt; i++) {
        nx = poss[i].x;
        ny = poss[i].y;
        cursemsg[i] = FALSE;

        /* if leashed, we drag him along. */
        if (mtmp->mleashed && distu(nx, ny) > 4)
            continue;

        /* if a guardian, try to stay close by choice */
        if (!has_edog && (j = distu(nx, ny)) > 16 && j >= udist)
            continue;

        if ((info[i] & ALLOW_M) && MON_AT(nx, ny)) {
            int mstatus;
            register struct monst *mtmp2 = m_at(nx, ny);
            /* weight the audacity of the pet to attack a differently-leveled
             * foe based on its fraction of max HP:
             *       100%:  up to level + 2
             * 80% and up:  up to level + 1
             * 60% to 80%:  up to level
             * 40% to 60%:  up to level - 1
             * 25% to 40%:  up to level - 2
             *  below 25%:  won't attack peacefuls of any level (different case)
             *  below 20%:  up to level - 3
             *
             * note that balk's maximum value is +3, as it is the lowest level
             * the pet will balk at attacking rather than the highest level they
             * are willing to attack; note the >= used when comparing it.
             */
            int balk = mtmp->m_lev + ((5 * mtmp->mhp) / mtmp->mhpmax) - 2;

            if ((int) mtmp2->m_lev >= balk
                || (mtmp2->data == &mons[PM_FLOATING_EYE] && rn2(10)
                    && mtmp->mcansee && haseyes(mtmp->data) && mtmp2->mcansee
                    && (perceives(mtmp->data) || !mtmp2->minvis))
                || (mtmp2->data == &mons[PM_GELATINOUS_CUBE] && rn2(10))
                || (max_passive_dmg(mtmp2, mtmp) >= mtmp->mhp)
                || ((mtmp->mhp * 4 < mtmp->mhpmax
                     || mtmp2->data->msound == MS_GUARDIAN
                     || mtmp2->data->msound == MS_LEADER) && mtmp2->mpeaceful
                    && !Conflict)
                || (touch_petrifies(mtmp2->data) && !resists_ston(mtmp)))
                continue;

            if (after)
                return MMOVE_NOTHING; /* hit only once each move */

            g.notonhead = 0;
            mstatus = mattackm(mtmp, mtmp2);

            /* aggressor (pet) died */
            if (mstatus & MM_AGR_DIED)
                return MMOVE_DIED;

            if ((mstatus & MM_HIT) && !(mstatus & MM_DEF_DIED) && rn2(4)
                && mtmp2->mlstmv != g.moves
                && !onscary(mtmp->mx, mtmp->my, mtmp2)
                /* monnear check needed: long worms hit on tail */
                && monnear(mtmp2, mtmp->mx, mtmp->my)) {
                mstatus = mattackm(mtmp2, mtmp); /* return attack */
                if (mstatus & MM_DEF_DIED)
                    return MMOVE_DIED;
            }
            return MMOVE_DONE;
        }
        if ((info[i] & ALLOW_MDISP) && MON_AT(nx, ny)
            && better_with_displacing && !undesirable_disp(mtmp, nx, ny)) {
            int mstatus;
            register struct monst *mtmp2 = m_at(nx, ny);

            mstatus = mdisplacem(mtmp, mtmp2, FALSE); /* displace monster */
            if (mstatus & MM_DEF_DIED)
                return MMOVE_DIED;
            return MMOVE_NOTHING;
        }

        {
            /* Dog avoids harmful traps, but perhaps it has to pass one
             * in order to follow player.  (Non-harmful traps do not
             * have ALLOW_TRAPS in info[].)  The dog only avoids the
             * trap if you've seen it, unlike enemies who avoid traps
             * if they've seen some trap of that type sometime in the
             * past.  (Neither behavior is really realistic.)
             */
            struct trap *trap;

            if ((info[i] & ALLOW_TRAPS) && (trap = t_at(nx, ny))) {
                if (mtmp->mleashed) {
                    if (!Deaf)
                        whimper(mtmp);
                } else {
                    /* 1/40 chance of stepping on it anyway, in case
                     * it has to pass one to follow the player...
                     */
                    if (trap->tseen && rn2(40))
                        continue;
                }
            }
        }

        /* dog eschews cursed objects, but likes dog food */
        /* (minion isn't interested; `cursemsg' stays FALSE) */
        if (has_edog) {
            boolean can_reach_food = could_reach_item(mtmp, nx, ny);
            for (obj = g.level.objects[nx][ny]; obj; obj = obj->nexthere) {
                if (obj->cursed) {
                    cursemsg[i] = TRUE;
                } else if (can_reach_food
                           && (otyp = dogfood(mtmp, obj)) < MANFOOD
                           && (otyp < ACCFOOD
                               || edog->hungrytime <= g.moves)) {
                    /* Note: our dog likes the food so much that he
                     * might eat it even when it conceals a cursed object */
                    nix = nx;
                    niy = ny;
                    chi = i;
                    do_eat = TRUE;
                    cursemsg[i] = FALSE; /* not reluctant */
                    goto newdogpos;
                }
            }
        }
        /* didn't find something to eat; if we saw a cursed item and
           aren't being forced to walk on it, usually keep looking */
        if (cursemsg[i] && !mtmp->mleashed && uncursedcnt > 0
            && rn2(13 * uncursedcnt))
            continue;

        /*
         * Lessen the chance of backtracking to previous position(s).
         * This causes unintended issues for pets trying to follow the
         * hero.  Thus, only run it if not leashed and >5 tiles away.
         */
        if (!mtmp->mleashed && distmin(mtmp->mx, mtmp->my, u.ux, u.uy) > 5) {
            k = has_edog ? uncursedcnt : cnt;
            for (j = 0; j < MTSZ && j < k - 1; j++)
                if (nx == mtmp->mtrack[j].x && ny == mtmp->mtrack[j].y)
                    if (rn2(MTSZ * (k - j)))
                        goto nxti;
        }

        j = ((ndist = GDIST(nx, ny)) - nidist) * appr;
        if ((j == 0 && !rn2(++chcnt)) || j < 0
            || (j > 0 && !whappr
                && ((omx == nix && omy == niy && !rn2(3)) || !rn2(12)))) {
            nix = nx;
            niy = ny;
            nidist = ndist;
            if (j < 0)
                chcnt = 0;
            chi = i;
        }
 nxti:
        ;
    }

    /* Pet hasn't attacked anything but is considering moving -
     * now's the time for ranged attacks. Note that the pet can move
     * after it performs its ranged attack. Should this be changed?
     */
    {
        struct monst *mtarg;
        int hungry = 0;

        /* How hungry is the pet? */
        if (!mtmp->isminion) {
            struct edog *dog = EDOG(mtmp);

            hungry = (g.moves > (dog->hungrytime + DOG_HUNGRY));
        }

        /* Identify the best target in a straight line from the pet;
         * if there is such a target, we'll let the pet attempt an attack.
         */
        mtarg = best_target(mtmp);

        /* Hungry pets are unlikely to use breath/spit attacks */
        if (mtarg && (!hungry || !rn2(5))) {
            int mstatus = MM_MISS;

            if (mtarg == &g.youmonst) {
                if (mattacku(mtmp))
                    return MMOVE_DIED;
                /* Treat this as the pet having initiated an attack even if it
                 * didn't, so it will lose its move.  This isn't entirely fair,
                 * but mattacku doesn't distinguish between "did not attack"
                 * and "attacked but didn't die" cases, and this is preferable
                 * to letting the pet attack the player and continuing to move.
                 */
                mstatus = MM_HIT;
            } else {
                mstatus = mattackm(mtmp, mtarg);

                /* Shouldn't happen, really */
                if (mstatus & MM_AGR_DIED)
                    return MMOVE_DIED;

                /* Allow the targeted nasty to strike back - if
                 * the targeted beast doesn't have a ranged attack,
                 * nothing will happen.
                 */
                if ((mstatus & MM_HIT) && !(mstatus & MM_DEF_DIED)
                    && rn2(4) && mtarg != &g.youmonst) {

                    /* Can monster see?  If it can, it can retaliate
                     * even if the pet is invisible, since it'll see
                     * the direction from which the ranged attack came;
                     * if it's blind or unseeing, it can't retaliate
                     */
                    if (mtarg->mcansee && haseyes(mtarg->data)) {
                        mstatus = mattackm(mtarg, mtmp);
                        if (mstatus & MM_DEF_DIED)
                            return MMOVE_DIED;
                    }
                }
            }
            /* Only return 3 if the pet actually made a ranged attack, and
             * thus should lose the rest of its move.
             * There's a chain of assumptions here:
             * 1. score_targ and best_target will never select a monster
             *    that can be attacked in melee, so the mattackm call can
             *    only ever try ranged options
             * 2. if the only attacks available to mattackm are ranged
             *    options, and the monster cannot make a ranged attack, it
             *    will return MM_MISS.
             */
            if (mstatus != MM_MISS)
                return MMOVE_DONE;
        }
    }

 newdogpos:
    if (nix != omx || niy != omy) {
        boolean wasseen;

        if (info[chi] & ALLOW_U) {
            if (mtmp->mleashed) { /* play it safe */
                pline("%s breaks loose of %s leash!", Monnam(mtmp),
                      mhis(mtmp));
                m_unleash(mtmp, FALSE);
            }
            (void) mattacku(mtmp);
            return MMOVE_DONE;
        }
        if (!m_in_out_region(mtmp, nix, niy))
            return MMOVE_MOVED;
        if (m_digweapon_check(mtmp, nix,niy))
            return MMOVE_NOTHING;

        /* insert a worm_move() if worms ever begin to eat things */
        wasseen = canseemon(mtmp);
        remove_monster(omx, omy);
        place_monster(mtmp, nix, niy);
        if (cursemsg[chi] && (wasseen || canseemon(mtmp))) {
            /* describe top item of pile, not necessarily cursed item itself;
               don't use glyph_at() here--it would return the pet but we want
               to know whether an object is remembered at this map location */
            struct obj *o = (!Hallucination && g.level.flags.hero_memory
                             && glyph_is_object(levl[nix][niy].glyph))
                               ? vobj_at(nix, niy) : 0;
            const char *what = o ? distant_name(o, doname) : something;

            pline("%s %s reluctantly over %s.", noit_Monnam(mtmp),
                  vtense((char *) 0, locomotion(mtmp->data, "step")), what);
        }
        mon_track_add(mtmp, omx, omy);
        /* We have to know if the pet's going to do a combined eat and
         * move before moving it, but it can't eat until after being
         * moved.  Thus the do_eat flag.
         */
        if (do_eat) {
            if (dog_eat(mtmp, obj, omx, omy, FALSE) == 2)
                return MMOVE_DIED;
        }
    } else if (mtmp->mleashed && distu(omx, omy) > 4) {
        /* an incredible kludge, but the only way to keep pooch near
         * after it spends time eating or in a trap, etc.
         */
        coord cc;

        nx = sgn(omx - u.ux);
        ny = sgn(omy - u.uy);
        cc.x = u.ux + nx;
        cc.y = u.uy + ny;
        if (goodpos(cc.x, cc.y, mtmp, 0))
            goto dognext;

        i = xytod(nx, ny);
        for (j = DIR_LEFT(i); j < DIR_RIGHT(i); j++) {
            dtoxy(&cc, j);
            if (goodpos(cc.x, cc.y, mtmp, 0))
                goto dognext;
        }
        for (j = DIR_LEFT2(i); j < DIR_RIGHT2(i); j++) {
            dtoxy(&cc, j);
            if (goodpos(cc.x, cc.y, mtmp, 0))
                goto dognext;
        }
        cc.x = mtmp->mx;
        cc.y = mtmp->my;
 dognext:
        if (!m_in_out_region(mtmp, nix, niy))
            return MMOVE_MOVED;
        remove_monster(mtmp->mx, mtmp->my);
        place_monster(mtmp, cc.x, cc.y);
        newsym(cc.x, cc.y);
        set_apparxy(mtmp);
    }
    return MMOVE_MOVED;
}

/* check if a monster could pick up objects from a location */
static boolean
could_reach_item(struct monst *mon, coordxy nx, coordxy ny)
{
    if ((!is_pool(nx, ny) || is_swimmer(mon->data))
        && (!is_lava(nx, ny) || likes_lava(mon->data))
        && (!sobj_at(BOULDER, nx, ny) || throws_rocks(mon->data)))
        return TRUE;
    return FALSE;
}

/* Hack to prevent a dog from being endlessly stuck near an object that
 * it can't reach, such as caught in a teleport scroll niche.  It recursively
 * checks to see if the squares in between are good.  The checking could be
 * a little smarter; a full check would probably be useful in m_move() too.
 * Since the maximum food distance is 5, this should never be more than 5
 * calls deep.
 */
static boolean
can_reach_location(struct monst *mon, coordxy mx, coordxy my, coordxy fx, coordxy fy)
{
    int i, j;
    int dist;

    if (mx == fx && my == fy)
        return TRUE;
    if (!isok(mx, my))
        return FALSE; /* should not happen */

    dist = dist2(mx, my, fx, fy);
    for (i = mx - 1; i <= mx + 1; i++) {
        for (j = my - 1; j <= my + 1; j++) {
            if (!isok(i, j))
                continue;
            if (dist2(i, j, fx, fy) >= dist)
                continue;
            if (IS_ROCK(levl[i][j].typ) && !passes_walls(mon->data)
                && (!may_dig(i, j) || !tunnels(mon->data)
                    /* tunnelling monsters can't do that on the rogue level */
                    || Is_rogue_level(&u.uz)))
                continue;
            if (IS_DOOR(levl[i][j].typ)
                && (levl[i][j].doormask & (D_CLOSED | D_LOCKED)))
                continue;
            if (!could_reach_item(mon, i, j))
                continue;
            if (can_reach_location(mon, i, j, fx, fy))
                return TRUE;
        }
    }
    return FALSE;
}

/* do_clear_area client */
static void
wantdoor(coordxy x, coordxy y, genericptr_t distance)
{
    int ndist, *dist_ptr = (int *) distance;

    if (*dist_ptr > (ndist = distu(x, y))) {
        g.gx = x;
        g.gy = y;
        *dist_ptr = ndist;
    }
}

static const struct qmchoices {
    int mndx;             /* type of pet, 0 means any  */
    char mlet;            /* symbol of pet, 0 means any */
    unsigned mappearance; /* mimic this */
    uchar m_ap_type;      /* what is the thing it is mimicing? */
} qm[] = {
    /* Things that some pets might be thinking about at the time */
    { PM_LITTLE_DOG, 0, PM_KITTEN, M_AP_MONSTER },
    { PM_DOG, 0, PM_HOUSECAT, M_AP_MONSTER },
    { PM_LARGE_DOG, 0, PM_LARGE_CAT, M_AP_MONSTER },
    { PM_KITTEN, 0, PM_LITTLE_DOG, M_AP_MONSTER },
    { PM_HOUSECAT, 0, PM_DOG, M_AP_MONSTER },
    { PM_LARGE_CAT, 0, PM_LARGE_DOG, M_AP_MONSTER },
    { PM_HOUSECAT, 0, PM_GIANT_RAT, M_AP_MONSTER },
    { 0, S_DOG, S_sink, M_AP_FURNITURE }, /* sorry, no fire hydrants */
    { 0, 0, TRIPE_RATION, M_AP_OBJECT }, /* leave this at end */
};

void
finish_meating(struct monst *mtmp)
{
    mtmp->meating = 0;
    if (M_AP_TYPE(mtmp) && mtmp->mappearance && mtmp->data->mlet != S_MIMIC) {
        /* was eating a mimic and now appearance needs resetting */
        mtmp->m_ap_type = M_AP_NOTHING;
        mtmp->mappearance = 0;
        newsym(mtmp->mx, mtmp->my);
    }
}

static void
quickmimic(struct monst *mtmp)
{
    int idx = 0, trycnt = 5, spotted, seeloc;
    char buf[BUFSZ];

    if (Protection_from_shape_changers || !mtmp->meating)
        return;

    /* with polymorph, the steed's equipment would be re-checked and its
       saddle would come off, triggering DISMOUNT_FELL, but mimicking
       doesn't impact monster's equipment; normally DISMOUNT_POLY is for
       rider taking on an unsuitable shape, but its message works fine
       for this and also avoids inflicting damage during forced dismount;
       do this before changing so that dismount refers to original shape */
    if (mtmp == u.usteed)
        dismount_steed(DISMOUNT_POLY);

    do {
        idx = rn2(SIZE(qm));
        if (qm[idx].mndx != 0 && monsndx(mtmp->data) == qm[idx].mndx)
            break;
        if (qm[idx].mlet != 0 && mtmp->data->mlet == qm[idx].mlet)
            break;
        if (qm[idx].mndx == 0 && qm[idx].mlet == 0)
            break;
    } while (--trycnt > 0);
    if (trycnt == 0)
        idx = SIZE(qm) - 1;

    Strcpy(buf, mon_nam(mtmp));
    spotted = canspotmon(mtmp);
    seeloc = cansee(mtmp->mx, mtmp->my);

    mtmp->m_ap_type = qm[idx].m_ap_type;
    mtmp->mappearance = qm[idx].mappearance;

    if (spotted || seeloc || canspotmon(mtmp)) {
        int prev_glyph = glyph_at(mtmp->mx, mtmp->my);
        const char *what = (M_AP_TYPE(mtmp) == M_AP_FURNITURE)
                           ? defsyms[mtmp->mappearance].explanation
                           : (M_AP_TYPE(mtmp) == M_AP_OBJECT
                              && OBJ_DESCR(objects[mtmp->mappearance]))
                             ? OBJ_DESCR(objects[mtmp->mappearance])
                             : (M_AP_TYPE(mtmp) == M_AP_OBJECT
                                && OBJ_NAME(objects[mtmp->mappearance]))
                               ? OBJ_NAME(objects[mtmp->mappearance])
                               : (M_AP_TYPE(mtmp) == M_AP_MONSTER)
                                 ? pmname(&mons[mtmp->mappearance],
                                          Mgender(mtmp))
                                 : something;

        newsym(mtmp->mx, mtmp->my);
        if (glyph_at(mtmp->mx, mtmp->my) != prev_glyph)
            You("%s %s %s where %s was!",
                seeloc ? "see" : "sense that",
                (what != something) ? an(what) : what,
                seeloc ? "appear" : "has appeared", buf);
        else
            You("sense that %s feels rather %s-ish.", buf, what);

        display_nhwindow(WIN_MAP, TRUE);
    }
}

/*dogmove.c*/

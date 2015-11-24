/* NetHack 3.6	detect.c	$NHDT-Date: 1446369464 2015/11/01 09:17:44 $  $NHDT-Branch: master $:$NHDT-Revision: 1.61 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Detection routines, including crystal ball, magic mapping, and search
 * command.
 */

#include "hack.h"
#include "artifact.h"

extern boolean known; /* from read.c */

STATIC_DCL void FDECL(do_dknown_of, (struct obj *));
STATIC_DCL boolean FDECL(check_map_spot, (int, int, CHAR_P, unsigned));
STATIC_DCL boolean FDECL(clear_stale_map, (CHAR_P, unsigned));
STATIC_DCL void FDECL(sense_trap, (struct trap *, XCHAR_P, XCHAR_P, int));
STATIC_DCL int FDECL(detect_obj_traps, (struct obj *, BOOLEAN_P, int));
STATIC_DCL void FDECL(show_map_spot, (int, int));
STATIC_PTR void FDECL(findone, (int, int, genericptr_t));
STATIC_PTR void FDECL(openone, (int, int, genericptr_t));

/* Recursively search obj for an object in class oclass and return 1st found
 */
struct obj *
o_in(obj, oclass)
struct obj *obj;
char oclass;
{
    register struct obj *otmp;
    struct obj *temp;

    if (obj->oclass == oclass)
        return obj;

    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (otmp->oclass == oclass)
                return otmp;
            else if (Has_contents(otmp) && (temp = o_in(otmp, oclass)))
                return temp;
    }
    return (struct obj *) 0;
}

/* Recursively search obj for an object made of specified material.
 * Return first found.
 */
struct obj *
o_material(obj, material)
struct obj *obj;
unsigned material;
{
    register struct obj *otmp;
    struct obj *temp;

    if (objects[obj->otyp].oc_material == material)
        return obj;

    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (objects[otmp->otyp].oc_material == material)
                return otmp;
            else if (Has_contents(otmp)
                     && (temp = o_material(otmp, material)))
                return temp;
    }
    return (struct obj *) 0;
}

STATIC_OVL void
do_dknown_of(obj)
struct obj *obj;
{
    struct obj *otmp;

    obj->dknown = 1;
    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            do_dknown_of(otmp);
    }
}

/* Check whether the location has an outdated object displayed on it. */
STATIC_OVL boolean
check_map_spot(x, y, oclass, material)
int x, y;
char oclass;
unsigned material;
{
    int glyph;
    register struct obj *otmp;
    register struct monst *mtmp;

    glyph = glyph_at(x, y);
    if (glyph_is_object(glyph)) {
        /* there's some object shown here */
        if (oclass == ALL_CLASSES) {
            return (boolean) !(level.objects[x][y] /* stale if nothing here */
                               || ((mtmp = m_at(x, y)) != 0 && mtmp->minvent));
        } else {
            if (material
                && objects[glyph_to_obj(glyph)].oc_material == material) {
                /* object shown here is of interest because material matches */
                for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
                    if (o_material(otmp, GOLD))
                        return FALSE;
                /* didn't find it; perhaps a monster is carrying it */
                if ((mtmp = m_at(x, y)) != 0) {
                    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                        if (o_material(otmp, GOLD))
                            return FALSE;
                }
                /* detection indicates removal of this object from the map */
                return TRUE;
            }
            if (oclass && objects[glyph_to_obj(glyph)].oc_class == oclass) {
                /* obj shown here is of interest because its class matches */
                for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
                    if (o_in(otmp, oclass))
                        return FALSE;
                /* didn't find it; perhaps a monster is carrying it */
                if ((mtmp = m_at(x, y)) != 0) {
                    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                        if (o_in(otmp, oclass))
                            return FALSE;
                }
                /* detection indicates removal of this object from the map */
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
 * When doing detection, remove stale data from the map display (corpses
 * rotted away, objects carried away by monsters, etc) so that it won't
 * reappear after the detection has completed.  Return true if noticeable
 * change occurs.
 */
STATIC_OVL boolean
clear_stale_map(oclass, material)
char oclass;
unsigned material;
{
    register int zx, zy;
    boolean change_made = FALSE;

    for (zx = 1; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            if (check_map_spot(zx, zy, oclass, material)) {
                unmap_object(zx, zy);
                change_made = TRUE;
            }

    return change_made;
}

/* look for gold, on the floor or in monsters' possession */
int
gold_detect(sobj)
register struct obj *sobj;
{
    register struct obj *obj;
    register struct monst *mtmp;
    struct obj *temp;
    boolean stale;

    known = stale =
        clear_stale_map(COIN_CLASS, (unsigned) (sobj->blessed ? GOLD : 0));

    /* look for gold carried by monsters (might be in a container) */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue; /* probably not needed in this case but... */
        if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
            known = TRUE;
            goto outgoldmap; /* skip further searching */
        } else
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (sobj->blessed && o_material(obj, GOLD)) {
                    known = TRUE;
                    goto outgoldmap;
                } else if (o_in(obj, COIN_CLASS)) {
                    known = TRUE;
                    goto outgoldmap; /* skip further searching */
                }
    }

    /* look for gold objects */
    for (obj = fobj; obj; obj = obj->nobj) {
        if (sobj->blessed && o_material(obj, GOLD)) {
            known = TRUE;
            if (obj->ox != u.ux || obj->oy != u.uy)
                goto outgoldmap;
        } else if (o_in(obj, COIN_CLASS)) {
            known = TRUE;
            if (obj->ox != u.ux || obj->oy != u.uy)
                goto outgoldmap;
        }
    }

    if (!known) {
        /* no gold found on floor or monster's inventory.
           adjust message if you have gold in your inventory */
        if (sobj) {
            char buf[BUFSZ];
            if (youmonst.data == &mons[PM_GOLD_GOLEM]) {
                Sprintf(buf, "You feel like a million %s!", currency(2L));
            } else if (hidden_gold() || money_cnt(invent))
                Strcpy(buf,
                   "You feel worried about your future financial situation.");
            else
                Strcpy(buf, "You feel materially poor.");
            strange_feeling(sobj, buf);
        }
        return 1;
    }
    /* only under me - no separate display required */
    if (stale)
        docrt();
    You("notice some gold between your %s.", makeplural(body_part(FOOT)));
    return 0;

outgoldmap:
    cls();

    iflags.save_uinwater = u.uinwater, iflags.save_uburied = u.uburied;
    u.uinwater = u.uburied = 0;
    /* Discover gold locations. */
    for (obj = fobj; obj; obj = obj->nobj) {
        if (sobj->blessed && (temp = o_material(obj, GOLD))) {
            if (temp != obj) {
                temp->ox = obj->ox;
                temp->oy = obj->oy;
            }
            map_object(temp, 1);
        } else if ((temp = o_in(obj, COIN_CLASS))) {
            if (temp != obj) {
                temp->ox = obj->ox;
                temp->oy = obj->oy;
            }
            map_object(temp, 1);
        }
    }
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue; /* probably overkill here */
        if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
            struct obj gold;
            gold = zeroobj; /* ensure oextra is cleared too */
            gold.otyp = GOLD_PIECE;
            gold.ox = mtmp->mx;
            gold.oy = mtmp->my;
            map_object(&gold, 1);
        } else
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (sobj->blessed && (temp = o_material(obj, GOLD))) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break;
                } else if ((temp = o_in(obj, COIN_CLASS))) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break;
                }
    }
    newsym(u.ux, u.uy);
    u.uinwater = iflags.save_uinwater, u.uburied = iflags.save_uburied;
    You_feel("very greedy, and sense gold!");
    exercise(A_WIS, TRUE);
    display_nhwindow(WIN_MAP, TRUE);
    docrt();
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
    return 0;
}

/* returns 1 if nothing was detected   */
/* returns 0 if something was detected */
int
food_detect(sobj)
register struct obj *sobj;
{
    register struct obj *obj;
    register struct monst *mtmp;
    register int ct = 0, ctu = 0;
    boolean confused = (Confusion || (sobj && sobj->cursed)), stale;
    char oclass = confused ? POTION_CLASS : FOOD_CLASS;
    const char *what = confused ? something : "food";

    stale = clear_stale_map(oclass, 0);

    for (obj = fobj; obj; obj = obj->nobj)
        if (o_in(obj, oclass)) {
            if (obj->ox == u.ux && obj->oy == u.uy)
                ctu++;
            else
                ct++;
        }
    for (mtmp = fmon; mtmp && !ct; mtmp = mtmp->nmon) {
        /* no DEADMONSTER(mtmp) check needed since dmons never have inventory
         */
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if (o_in(obj, oclass)) {
                ct++;
                break;
            }
    }

    if (!ct && !ctu) {
        known = stale && !confused;
        if (stale) {
            docrt();
            You("sense a lack of %s nearby.", what);
            if (sobj && sobj->blessed) {
                if (!u.uedibility)
                    Your("%s starts to tingle.", body_part(NOSE));
                u.uedibility = 1;
            }
        } else if (sobj) {
            char buf[BUFSZ];
            Sprintf(buf, "Your %s twitches%s.", body_part(NOSE),
                    (sobj->blessed && !u.uedibility)
                        ? " then starts to tingle"
                        : "");
            if (sobj->blessed && !u.uedibility) {
                boolean savebeginner = flags.beginner;

                flags.beginner = FALSE; /* prevent non-delivery of message */
                strange_feeling(sobj, buf);
                flags.beginner = savebeginner;
                u.uedibility = 1;
            } else
                strange_feeling(sobj, buf);
        }
        return !stale;
    } else if (!ct) {
        known = TRUE;
        You("%s %s nearby.", sobj ? "smell" : "sense", what);
        if (sobj && sobj->blessed) {
            if (!u.uedibility)
                pline("Your %s starts to tingle.", body_part(NOSE));
            u.uedibility = 1;
        }
    } else {
        struct obj *temp;
        known = TRUE;
        cls();
        iflags.save_uinwater = u.uinwater, iflags.save_uburied = u.uburied;
        u.uinwater = u.uburied = 0;
        for (obj = fobj; obj; obj = obj->nobj)
            if ((temp = o_in(obj, oclass)) != 0) {
                if (temp != obj) {
                    temp->ox = obj->ox;
                    temp->oy = obj->oy;
                }
                map_object(temp, 1);
            }
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            /* no DEADMONSTER(mtmp) check needed since dmons never have
             * inventory */
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if ((temp = o_in(obj, oclass)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break; /* skip rest of this monster's inventory */
                }
        newsym(u.ux, u.uy);
        u.uinwater = iflags.save_uinwater, u.uburied = iflags.save_uburied;
        if (sobj) {
            if (sobj->blessed) {
                Your("%s %s to tingle and you smell %s.", body_part(NOSE),
                     u.uedibility ? "continues" : "starts", what);
                u.uedibility = 1;
            } else
                Your("%s tingles and you smell %s.", body_part(NOSE), what);
        } else
            You("sense %s.", what);
        display_nhwindow(WIN_MAP, TRUE);
        exercise(A_WIS, TRUE);
        docrt();
        if (Underwater)
            under_water(2);
        if (u.uburied)
            under_ground(2);
    }
    return 0;
}

/*
 * Used for scrolls, potions, spells, and crystal balls.  Returns:
 *
 *      1 - nothing was detected
 *      0 - something was detected
 */
int
object_detect(detector, class)
struct obj *detector; /* object doing the detecting */
int class;            /* an object class, 0 for all */
{
    register int x, y;
    char stuff[BUFSZ];
    int is_cursed = (detector && detector->cursed);
    int do_dknown = (detector && (detector->oclass == POTION_CLASS
                                  || detector->oclass == SPBOOK_CLASS)
                     && detector->blessed);
    int ct = 0, ctu = 0;
    register struct obj *obj, *otmp = (struct obj *) 0;
    register struct monst *mtmp;
    int sym, boulder = 0;

    if (class < 0 || class >= MAXOCLASSES) {
        impossible("object_detect:  illegal class %d", class);
        class = 0;
    }

    /* Special boulder symbol check - does the class symbol happen
     * to match iflags.bouldersym which is a user-defined?
     * If so, that means we aren't sure what they really wanted to
     * detect. Rather than trump anything, show both possibilities.
     * We can exclude checking the buried obj chain for boulders below.
     */
    sym = class ? def_oc_syms[class].sym : 0;
    if (sym && iflags.bouldersym && sym == iflags.bouldersym)
        boulder = ROCK_CLASS;

    if (Hallucination || (Confusion && class == SCROLL_CLASS))
        Strcpy(stuff, something);
    else
        Strcpy(stuff, class ? def_oc_syms[class].name : "objects");
    if (boulder && class != ROCK_CLASS)
        Strcat(stuff, " and/or large stones");

    if (do_dknown)
        for (obj = invent; obj; obj = obj->nobj)
            do_dknown_of(obj);

    for (obj = fobj; obj; obj = obj->nobj) {
        if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) {
            if (obj->ox == u.ux && obj->oy == u.uy)
                ctu++;
            else
                ct++;
        }
        if (do_dknown)
            do_dknown_of(obj);
    }

    for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
        if (!class || o_in(obj, class)) {
            if (obj->ox == u.ux && obj->oy == u.uy)
                ctu++;
            else
                ct++;
        }
        if (do_dknown)
            do_dknown_of(obj);
    }

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj) {
            if ((!class && !boulder) || o_in(obj, class)
                || o_in(obj, boulder))
                ct++;
            if (do_dknown)
                do_dknown_of(obj);
        }
        if ((is_cursed && mtmp->m_ap_type == M_AP_OBJECT
             && (!class || class == objects[mtmp->mappearance].oc_class))
            || (findgold(mtmp->minvent) && (!class || class == COIN_CLASS))) {
            ct++;
            break;
        }
    }

    if (!clear_stale_map(!class ? ALL_CLASSES : class, 0) && !ct) {
        if (!ctu) {
            if (detector)
                strange_feeling(detector, "You feel a lack of something.");
            return 1;
        }

        You("sense %s nearby.", stuff);
        return 0;
    }

    cls();

    iflags.save_uinwater = u.uinwater, iflags.save_uburied = u.uburied;
    u.uinwater = u.uburied = 0;
    /*
     *  Map all buried objects first.
     */
    for (obj = level.buriedobjlist; obj; obj = obj->nobj)
        if (!class || (otmp = o_in(obj, class))) {
            if (class) {
                if (otmp != obj) {
                    otmp->ox = obj->ox;
                    otmp->oy = obj->oy;
                }
                map_object(otmp, 1);
            } else
                map_object(obj, 1);
        }
    /*
     * If we are mapping all objects, map only the top object of a pile or
     * the first object in a monster's inventory.  Otherwise, go looking
     * for a matching object class and display the first one encountered
     * at each location.
     *
     * Objects on the floor override buried objects.
     */
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
                if ((!class && !boulder) || (otmp = o_in(obj, class))
                    || (otmp = o_in(obj, boulder))) {
                    if (class || boulder) {
                        if (otmp != obj) {
                            otmp->ox = obj->ox;
                            otmp->oy = obj->oy;
                        }
                        map_object(otmp, 1);
                    } else
                        map_object(obj, 1);
                    break;
                }

    /* Objects in the monster's inventory override floor objects. */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if ((!class && !boulder) || (otmp = o_in(obj, class))
                || (otmp = o_in(obj, boulder))) {
                if (!class && !boulder)
                    otmp = obj;
                otmp->ox = mtmp->mx; /* at monster location */
                otmp->oy = mtmp->my;
                map_object(otmp, 1);
                break;
            }
        /* Allow a mimic to override the detected objects it is carrying. */
        if (is_cursed && mtmp->m_ap_type == M_AP_OBJECT
            && (!class || class == objects[mtmp->mappearance].oc_class)) {
            struct obj temp;

            temp.oextra = (struct oextra *) 0;
            temp.otyp = mtmp->mappearance; /* needed for obj_to_glyph() */
            temp.ox = mtmp->mx;
            temp.oy = mtmp->my;
            temp.corpsenm = PM_TENGU; /* if mimicing a corpse */
            map_object(&temp, 1);
        } else if (findgold(mtmp->minvent)
                   && (!class || class == COIN_CLASS)) {
            struct obj gold;
            gold = zeroobj; /* ensure oextra is cleared too */
            gold.otyp = GOLD_PIECE;
            gold.ox = mtmp->mx;
            gold.oy = mtmp->my;
            map_object(&gold, 1);
        }
    }

    newsym(u.ux, u.uy);
    u.uinwater = iflags.save_uinwater, u.uburied = iflags.save_uburied;
    You("detect the %s of %s.", ct ? "presence" : "absence", stuff);
    display_nhwindow(WIN_MAP, TRUE);
    /*
     * What are we going to do when the hero does an object detect while blind
     * and the detected object covers a known pool?
     */
    docrt(); /* this will correctly reset vision */

    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
    return 0;
}

/*
 * Used by: crystal balls, potions, fountains
 *
 * Returns 1 if nothing was detected.
 * Returns 0 if something was detected.
 */
int
monster_detect(otmp, mclass)
register struct obj *otmp; /* detecting object (if any) */
int mclass;                /* monster class, 0 for all */
{
    register struct monst *mtmp;
    int mcnt = 0;

    /* Note: This used to just check fmon for a non-zero value
     * but in versions since 3.3.0 fmon can test TRUE due to the
     * presence of dmons, so we have to find at least one
     * with positive hit-points to know for sure.
     */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp)) {
            mcnt++;
            break;
        }

    if (!mcnt) {
        if (otmp)
            strange_feeling(otmp, Hallucination
                                      ? "You get the heebie jeebies."
                                      : "You feel threatened.");
        return 1;
    } else {
        boolean woken = FALSE;

        cls();
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (!mclass || mtmp->data->mlet == mclass
                || (mtmp->data == &mons[PM_LONG_WORM]
                    && mclass == S_WORM_TAIL))
                if (mtmp->mx > 0) {
                    if (mclass && def_monsyms[mclass].sym == ' ')
                        show_glyph(mtmp->mx, mtmp->my,
                                   detected_mon_to_glyph(mtmp));
                    else
                        show_glyph(mtmp->mx, mtmp->my,
                                   mtmp->mtame ? pet_to_glyph(mtmp) : mon_to_glyph(mtmp));
                    /* don't be stingy - display entire worm */
                    if (mtmp->data == &mons[PM_LONG_WORM])
                        detect_wsegs(mtmp, 0);
                }
            if (otmp && otmp->cursed
                && (mtmp->msleeping || !mtmp->mcanmove)) {
                mtmp->msleeping = mtmp->mfrozen = 0;
                mtmp->mcanmove = 1;
                woken = TRUE;
            }
        }
        display_self();
        You("sense the presence of monsters.");
        if (woken)
            pline("Monsters sense the presence of you.");
        display_nhwindow(WIN_MAP, TRUE);
        docrt();
        if (Underwater)
            under_water(2);
        if (u.uburied)
            under_ground(2);
    }
    return 0;
}

STATIC_OVL void
sense_trap(trap, x, y, src_cursed)
struct trap *trap;
xchar x, y;
int src_cursed;
{
    if (Hallucination || src_cursed) {
        struct obj obj; /* fake object */

        obj.oextra = (struct oextra *) 0;
        if (trap) {
            obj.ox = trap->tx;
            obj.oy = trap->ty;
        } else {
            obj.ox = x;
            obj.oy = y;
        }
        obj.otyp = (src_cursed) ? GOLD_PIECE : random_object();
        obj.corpsenm = random_monster(); /* if otyp == CORPSE */
        map_object(&obj, 1);
    } else if (trap) {
        map_trap(trap, 1);
        trap->tseen = 1;
    } else {
        struct trap temp_trap; /* fake trap */
        temp_trap.tx = x;
        temp_trap.ty = y;
        temp_trap.ttyp = BEAR_TRAP; /* some kind of trap */
        map_trap(&temp_trap, 1);
    }
}

#define OTRAP_NONE 0  /* nothing found */
#define OTRAP_HERE 1  /* found at hero's location */
#define OTRAP_THERE 2 /* found at any other location */

/* check a list of objects for chest traps; return 1 if found at <ux,uy>,
   2 if found at some other spot, 3 if both, 0 otherwise; optionally
   update the map to show where such traps were found */
STATIC_OVL int
detect_obj_traps(objlist, show_them, how)
struct obj *objlist;
boolean show_them;
int how; /* 1 for misleading map feedback */
{
    struct obj *otmp;
    xchar x, y;
    int result = OTRAP_NONE;

    for (otmp = objlist; otmp; otmp = otmp->nobj) {
        if (Is_box(otmp) && otmp->otrapped
            && get_obj_location(otmp, &x, &y, BURIED_TOO | CONTAINED_TOO)) {
            result |= (x == u.ux && y == u.uy) ? OTRAP_HERE : OTRAP_THERE;
            if (show_them)
                sense_trap((struct trap *) 0, x, y, how);
        }
        if (Has_contents(otmp))
            result |= detect_obj_traps(otmp->cobj, show_them, how);
    }
    return result;
}

/* the detections are pulled out so they can
 * also be used in the crystal ball routine
 * returns 1 if nothing was detected
 * returns 0 if something was detected
 */
int
trap_detect(sobj)
register struct obj *sobj;
/* sobj is null if crystal ball, *scroll if gold detection scroll */
{
    register struct trap *ttmp;
    struct monst *mon;
    int door, glyph, tr;
    int cursed_src = sobj && sobj->cursed;
    boolean found = FALSE;
    coord cc;

    /* floor/ceiling traps */
    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
        if (ttmp->tx != u.ux || ttmp->ty != u.uy)
            goto outtrapmap;
        else
            found = TRUE;
    }
    /* chest traps (might be buried or carried) */
    if ((tr = detect_obj_traps(fobj, FALSE, 0)) != OTRAP_NONE) {
        if (tr & OTRAP_THERE)
            goto outtrapmap;
        else
            found = TRUE;
    }
    if ((tr = detect_obj_traps(level.buriedobjlist, FALSE, 0))
        != OTRAP_NONE) {
        if (tr & OTRAP_THERE)
            goto outtrapmap;
        else
            found = TRUE;
    }
    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        if ((tr = detect_obj_traps(mon->minvent, FALSE, 0)) != OTRAP_NONE) {
            if (tr & OTRAP_THERE)
                goto outtrapmap;
            else
                found = TRUE;
        }
    }
    if (detect_obj_traps(invent, FALSE, 0) != OTRAP_NONE)
        found = TRUE;
    /* door traps */
    for (door = 0; door < doorindex; door++) {
        cc = doors[door];
        if (levl[cc.x][cc.y].doormask & D_TRAPPED) {
            if (cc.x != u.ux || cc.y != u.uy)
                goto outtrapmap;
            else
                found = TRUE;
        }
    }
    if (!found) {
        char buf[BUFSZ];

        Sprintf(buf, "Your %s stop itching.", makeplural(body_part(TOE)));
        strange_feeling(sobj, buf);
        return 1;
    }
    /* traps exist, but only under me - no separate display required */
    Your("%s itch.", makeplural(body_part(TOE)));
    return 0;
outtrapmap:
    cls();

    iflags.save_uinwater = u.uinwater, iflags.save_uburied = u.uburied;
    u.uinwater = u.uburied = 0;

    /* show chest traps first, so that subsequent floor trap display
       will override if both types are present at the same location */
    (void) detect_obj_traps(fobj, TRUE, cursed_src);
    (void) detect_obj_traps(level.buriedobjlist, TRUE, cursed_src);
    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        (void) detect_obj_traps(mon->minvent, TRUE, cursed_src);
    }
    (void) detect_obj_traps(invent, TRUE, cursed_src);

    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
        sense_trap(ttmp, 0, 0, cursed_src);

    for (door = 0; door < doorindex; door++) {
        cc = doors[door];
        if (levl[cc.x][cc.y].doormask & D_TRAPPED)
            sense_trap((struct trap *) 0, cc.x, cc.y, cursed_src);
    }

    /* redisplay hero unless sense_trap() revealed something at <ux,uy> */
    glyph = glyph_at(u.ux, u.uy);
    if (!(glyph_is_trap(glyph) || glyph_is_object(glyph)))
        newsym(u.ux, u.uy);
    u.uinwater = iflags.save_uinwater, u.uburied = iflags.save_uburied;

    You_feel("%s.", cursed_src ? "very greedy" : "entrapped");
    /* wait for user to respond, then reset map display to normal */
    display_nhwindow(WIN_MAP, TRUE);
    docrt();
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
    return 0;
}

const char *
level_distance(where)
d_level *where;
{
    register schar ll = depth(&u.uz) - depth(where);
    register boolean indun = (u.uz.dnum == where->dnum);

    if (ll < 0) {
        if (ll < (-8 - rn2(3)))
            if (!indun)
                return "far away";
            else
                return "far below";
        else if (ll < -1)
            if (!indun)
                return "away below you";
            else
                return "below you";
        else if (!indun)
            return "in the distance";
        else
            return "just below";
    } else if (ll > 0) {
        if (ll > (8 + rn2(3)))
            if (!indun)
                return "far away";
            else
                return "far above";
        else if (ll > 1)
            if (!indun)
                return "away above you";
            else
                return "above you";
        else if (!indun)
            return "in the distance";
        else
            return "just above";
    } else if (!indun)
        return "in the distance";
    else
        return "near you";
}

static const struct {
    const char *what;
    d_level *where;
} level_detects[] = {
    { "Delphi", &oracle_level },
    { "Medusa's lair", &medusa_level },
    { "a castle", &stronghold_level },
    { "the Wizard of Yendor's tower", &wiz1_level },
};

void
use_crystal_ball(optr)
struct obj **optr;
{
    char ch;
    int oops;
    struct obj *obj = *optr;

    if (Blind) {
        pline("Too bad you can't see %s.", the(xname(obj)));
        return;
    }
    oops = (rnd(20) > ACURR(A_INT) || obj->cursed);
    if (oops && (obj->spe > 0)) {
        switch (rnd(obj->oartifact ? 4 : 5)) {
        case 1:
            pline("%s too much to comprehend!", Tobjnam(obj, "are"));
            break;
        case 2:
            pline("%s you!", Tobjnam(obj, "confuse"));
            make_confused((HConfusion & TIMEOUT) + (long) rnd(100), FALSE);
            break;
        case 3:
            if (!resists_blnd(&youmonst)) {
                pline("%s your vision!", Tobjnam(obj, "damage"));
                make_blinded((Blinded & TIMEOUT) + (long) rnd(100), FALSE);
                if (!Blind)
                    Your1(vision_clears);
            } else {
                pline("%s your vision.", Tobjnam(obj, "assault"));
                You("are unaffected!");
            }
            break;
        case 4:
            pline("%s your mind!", Tobjnam(obj, "zap"));
            (void) make_hallucinated(
                (HHallucination & TIMEOUT) + (long) rnd(100), FALSE, 0L);
            break;
        case 5:
            pline("%s!", Tobjnam(obj, "explode"));
            useup(obj);
            *optr = obj = 0; /* it's gone */
            /* physical damage cause by the shards and force */
            losehp(Maybe_Half_Phys(rnd(30)), "exploding crystal ball",
                   KILLED_BY_AN);
            break;
        }
        if (obj)
            consume_obj_charge(obj, TRUE);
        return;
    }

    if (Hallucination) {
        if (!obj->spe) {
            pline("All you see is funky %s haze.", hcolor((char *) 0));
        } else {
            switch (rnd(6)) {
            case 1:
                You("grok some groovy globs of incandescent lava.");
                break;
            case 2:
                pline("Whoa!  Psychedelic colors, %s!",
                      poly_gender() == 1 ? "babe" : "dude");
                break;
            case 3:
                pline_The("crystal pulses with sinister %s light!",
                          hcolor((char *) 0));
                break;
            case 4:
                You_see("goldfish swimming above fluorescent rocks.");
                break;
            case 5:
                You_see(
                    "tiny snowflakes spinning around a miniature farmhouse.");
                break;
            default:
                pline("Oh wow... like a kaleidoscope!");
                break;
            }
            consume_obj_charge(obj, TRUE);
        }
        return;
    }

    /* read a single character */
    if (flags.verbose)
        You("may look for an object or monster symbol.");
    ch = yn_function("What do you look for?", (char *) 0, '\0');
    /* Don't filter out ' ' here; it has a use */
    if ((ch != def_monsyms[S_GHOST].sym) && index(quitchars, ch)) {
        if (flags.verbose)
            pline1(Never_mind);
        return;
    }
    You("peer into %s...", the(xname(obj)));
    nomul(-rnd(10));
    multi_reason = "gazing into a crystal ball";
    nomovemsg = "";
    if (obj->spe <= 0)
        pline_The("vision is unclear.");
    else {
        int class;
        int ret = 0;

        makeknown(CRYSTAL_BALL);
        consume_obj_charge(obj, TRUE);

        /* special case: accept ']' as synonym for mimic
         * we have to do this before the def_char_to_objclass check
         */
        if (ch == DEF_MIMIC_DEF)
            ch = DEF_MIMIC;

        if ((class = def_char_to_objclass(ch)) != MAXOCLASSES)
            ret = object_detect((struct obj *) 0, class);
        else if ((class = def_char_to_monclass(ch)) != MAXMCLASSES)
            ret = monster_detect((struct obj *) 0, class);
        else if (iflags.bouldersym && (ch == iflags.bouldersym))
            ret = object_detect((struct obj *) 0, ROCK_CLASS);
        else
            switch (ch) {
            case '^':
                ret = trap_detect((struct obj *) 0);
                break;
            default: {
                int i = rn2(SIZE(level_detects));
                You_see("%s, %s.", level_detects[i].what,
                        level_distance(level_detects[i].where));
            }
                ret = 0;
                break;
            }

        if (ret) {
            if (!rn2(100)) /* make them nervous */
                You_see("the Wizard of Yendor gazing out at you.");
            else
                pline_The("vision is unclear.");
        }
    }
    return;
}

STATIC_OVL void
show_map_spot(x, y)
register int x, y;
{
    struct rm *lev;
    struct trap *t;
    int oldglyph;

    if (Confusion && rn2(7))
        return;
    lev = &levl[x][y];

    lev->seenv = SVALL;

    /* Secret corridors are found, but not secret doors. */
    if (lev->typ == SCORR) {
        lev->typ = CORR;
        unblock_point(x, y);
    }

    /*
     * Force the real background, then if it's not furniture and there's
     * a known trap there, display the trap, else if there was an object
     * shown there, redisplay the object.  So during mapping, furniture
     * takes precedence over traps, which take precedence over objects,
     * opposite to how normal vision behaves.
     */
    oldglyph = glyph_at(x, y);
    if (level.flags.hero_memory) {
        magic_map_background(x, y, 0);
        newsym(x, y); /* show it, if not blocked */
    } else {
        magic_map_background(x, y, 1); /* display it */
    }
    if (!IS_FURNITURE(lev->typ)) {
        if ((t = t_at(x, y)) != 0 && t->tseen) {
            map_trap(t, 1);
        } else if (glyph_is_trap(oldglyph) || glyph_is_object(oldglyph)) {
            show_glyph(x, y, oldglyph);
            if (level.flags.hero_memory)
                lev->glyph = oldglyph;
        }
    }
}

void
do_mapping()
{
    register int zx, zy;

    iflags.save_uinwater = u.uinwater, iflags.save_uburied = u.uburied;
    u.uinwater = u.uburied = 0;
    for (zx = 1; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            show_map_spot(zx, zy);
    u.uinwater = iflags.save_uinwater, u.uburied = iflags.save_uburied;
    if (!level.flags.hero_memory || Underwater) {
        flush_screen(1);                 /* flush temp screen */
        display_nhwindow(WIN_MAP, TRUE); /* wait */
        docrt();
    }
    exercise(A_WIS, TRUE);
}

void
do_vicinity_map()
{
    register int zx, zy;
    int lo_y = (u.uy - 5 < 0 ? 0 : u.uy - 5),
        hi_y = (u.uy + 6 > ROWNO ? ROWNO : u.uy + 6),
        lo_x = (u.ux - 9 < 1 ? 1 : u.ux - 9), /* avoid column 0 */
        hi_x = (u.ux + 10 > COLNO ? COLNO : u.ux + 10);

    for (zx = lo_x; zx < hi_x; zx++)
        for (zy = lo_y; zy < hi_y; zy++)
            show_map_spot(zx, zy);

    if (!level.flags.hero_memory || Underwater) {
        flush_screen(1);                 /* flush temp screen */
        display_nhwindow(WIN_MAP, TRUE); /* wait */
        docrt();
    }
}

/* convert a secret door into a normal door */
void
cvt_sdoor_to_door(lev)
struct rm *lev;
{
    int newmask = lev->doormask & ~WM_MASK;

    if (Is_rogue_level(&u.uz))
        /* rogue didn't have doors, only doorways */
        newmask = D_NODOOR;
    else
        /* newly exposed door is closed */
        if (!(newmask & D_LOCKED))
        newmask |= D_CLOSED;

    lev->typ = DOOR;
    lev->doormask = newmask;
}

STATIC_PTR void
findone(zx, zy, num)
int zx, zy;
genericptr_t num;
{
    register struct trap *ttmp;
    register struct monst *mtmp;

    if (levl[zx][zy].typ == SDOOR) {
        cvt_sdoor_to_door(&levl[zx][zy]); /* .typ = DOOR */
        magic_map_background(zx, zy, 0);
        newsym(zx, zy);
        (*(int *) num)++;
    } else if (levl[zx][zy].typ == SCORR) {
        levl[zx][zy].typ = CORR;
        unblock_point(zx, zy);
        magic_map_background(zx, zy, 0);
        newsym(zx, zy);
        (*(int *) num)++;
    } else if ((ttmp = t_at(zx, zy)) != 0) {
        if (!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
            ttmp->tseen = 1;
            newsym(zx, zy);
            (*(int *) num)++;
        }
    } else if ((mtmp = m_at(zx, zy)) != 0) {
        if (mtmp->m_ap_type) {
            seemimic(mtmp);
            (*(int *) num)++;
        }
        if (mtmp->mundetected
            && (is_hider(mtmp->data) || mtmp->data->mlet == S_EEL)) {
            mtmp->mundetected = 0;
            newsym(zx, zy);
            (*(int *) num)++;
        }
        if (!canspotmon(mtmp) && !glyph_is_invisible(levl[zx][zy].glyph))
            map_invisible(zx, zy);
    } else if (glyph_is_invisible(levl[zx][zy].glyph)) {
        unmap_object(zx, zy);
        newsym(zx, zy);
        (*(int *) num)++;
    }
}

STATIC_PTR void
openone(zx, zy, num)
int zx, zy;
genericptr_t num;
{
    register struct trap *ttmp;
    register struct obj *otmp;
    int *num_p = (int *) num;

    if (OBJ_AT(zx, zy)) {
        for (otmp = level.objects[zx][zy]; otmp; otmp = otmp->nexthere) {
            if (Is_box(otmp) && otmp->olocked) {
                otmp->olocked = 0;
                (*num_p)++;
            }
        }
        /* let it fall to the next cases. could be on trap. */
    }
    if (levl[zx][zy].typ == SDOOR
        || (levl[zx][zy].typ == DOOR
            && (levl[zx][zy].doormask & (D_CLOSED | D_LOCKED)))) {
        if (levl[zx][zy].typ == SDOOR)
            cvt_sdoor_to_door(&levl[zx][zy]); /* .typ = DOOR */
        if (levl[zx][zy].doormask & D_TRAPPED) {
            if (distu(zx, zy) < 3)
                b_trapped("door", 0);
            else
                Norep("You %s an explosion!",
                      cansee(zx, zy) ? "see" : (!Deaf ? "hear"
                                                      : "feel the shock of"));
            wake_nearto(zx, zy, 11 * 11);
            levl[zx][zy].doormask = D_NODOOR;
        } else
            levl[zx][zy].doormask = D_ISOPEN;
        unblock_point(zx, zy);
        newsym(zx, zy);
        (*num_p)++;
    } else if (levl[zx][zy].typ == SCORR) {
        levl[zx][zy].typ = CORR;
        unblock_point(zx, zy);
        newsym(zx, zy);
        (*num_p)++;
    } else if ((ttmp = t_at(zx, zy)) != 0) {
        struct monst *mon;
        boolean dummy; /* unneeded "you notice it arg" */

        if (!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
            ttmp->tseen = 1;
            newsym(zx, zy);
            (*num_p)++;
        }
        mon = (zx == u.ux && zy == u.uy) ? &youmonst : m_at(zx, zy);
        if (openholdingtrap(mon, &dummy)
            || openfallingtrap(mon, TRUE, &dummy))
            (*num_p)++;
    } else if (find_drawbridge(&zx, &zy)) {
        /* make sure it isn't an open drawbridge */
        open_drawbridge(zx, zy);
        (*num_p)++;
    }
}

/* returns number of things found */
int
findit()
{
    int num = 0;

    if (u.uswallow)
        return 0;
    do_clear_area(u.ux, u.uy, BOLT_LIM, findone, (genericptr_t) &num);
    return num;
}

/* returns number of things found and opened */
int
openit()
{
    int num = 0;

    if (u.uswallow) {
        if (is_animal(u.ustuck->data)) {
            if (Blind)
                pline("Its mouth opens!");
            else
                pline("%s opens its mouth!", Monnam(u.ustuck));
        }
        expels(u.ustuck, u.ustuck->data, TRUE);
        return -1;
    }

    do_clear_area(u.ux, u.uy, BOLT_LIM, openone, (genericptr_t) &num);
    return num;
}

/* callback hack for overriding vision in do_clear_area() */
boolean
detecting(func)
void FDECL((*func), (int, int, genericptr_t));
{
    return (func == findone || func == openone);
}

void
find_trap(trap)
struct trap *trap;
{
    int tt = what_trap(trap->ttyp);
    boolean cleared = FALSE;

    trap->tseen = 1;
    exercise(A_WIS, TRUE);
    feel_newsym(trap->tx, trap->ty);

    if (levl[trap->tx][trap->ty].glyph != trap_to_glyph(trap)) {
        /* There's too much clutter to see your find otherwise */
        cls();
        map_trap(trap, 1);
        display_self();
        cleared = TRUE;
    }

    You("find %s.", an(defsyms[trap_to_defsym(tt)].explanation));

    if (cleared) {
        display_nhwindow(WIN_MAP, TRUE); /* wait */
        docrt();
    }
}

int
dosearch0(aflag)
register int aflag; /* intrinsic autosearch vs explicit searching */
{
#ifdef GCC_BUG
    /* some versions of gcc seriously muck up nested loops. if you get strange
       crashes while searching in a version compiled with gcc, try putting
       #define GCC_BUG in *conf.h (or adding -DGCC_BUG to CFLAGS in the
       makefile).
     */
    volatile xchar x, y;
#else
    register xchar x, y;
#endif
    register struct trap *trap;
    register struct monst *mtmp;

    if (u.uswallow) {
        if (!aflag)
            pline("What are you looking for?  The exit?");
    } else {
        int fund = (uwep && uwep->oartifact
                    && spec_ability(uwep, SPFX_SEARCH)) ? uwep->spe : 0;

        if (ublindf && ublindf->otyp == LENSES && !Blind)
            fund += 2; /* JDS: lenses help searching */
        if (fund > 5)
            fund = 5;
        for (x = u.ux - 1; x < u.ux + 2; x++)
            for (y = u.uy - 1; y < u.uy + 2; y++) {
                if (!isok(x, y))
                    continue;
                if (x == u.ux && y == u.uy)
                    continue;

                if (Blind && !aflag)
                    feel_location(x, y);
                if (levl[x][y].typ == SDOOR) {
                    if (rnl(7 - fund))
                        continue;
                    cvt_sdoor_to_door(&levl[x][y]); /* .typ = DOOR */
                    exercise(A_WIS, TRUE);
                    nomul(0);
                    feel_location(x, y); /* make sure it shows up */
                    You("find a hidden door.");
                } else if (levl[x][y].typ == SCORR) {
                    if (rnl(7 - fund))
                        continue;
                    levl[x][y].typ = CORR;
                    unblock_point(x, y); /* vision */
                    exercise(A_WIS, TRUE);
                    nomul(0);
                    feel_location(x, y); /* make sure it shows up */
                    You("find a hidden passage.");
                } else {
                    /* Be careful not to find anything in an SCORR or SDOOR */
                    if ((mtmp = m_at(x, y)) != 0 && !aflag) {
                        if (mtmp->m_ap_type) {
                            seemimic(mtmp);
                        find:
                            exercise(A_WIS, TRUE);
                            if (!canspotmon(mtmp)) {
                                if (glyph_is_invisible(levl[x][y].glyph)) {
                                    /* found invisible monster in a square
                                     * which already has an 'I' in it.
                                     * Logically, this should still take
                                     * time and lead to a return(1), but
                                     * if we did that the player would keep
                                     * finding the same monster every turn.
                                     */
                                    continue;
                                } else {
                                    You_feel("an unseen monster!");
                                    map_invisible(x, y);
                                }
                            } else if (!sensemon(mtmp))
                                You("find %s.", mtmp->mtame
                                                   ? y_monnam(mtmp)
                                                   : a_monnam(mtmp));
                            return 1;
                        }
                        if (!canspotmon(mtmp)) {
                            if (mtmp->mundetected
                                && (is_hider(mtmp->data)
                                    || mtmp->data->mlet == S_EEL))
                                mtmp->mundetected = 0;
                            newsym(x, y);
                            goto find;
                        }
                    }

                    /* see if an invisible monster has moved--if Blind,
                     * feel_location() already did it
                     */
                    if (!aflag && !mtmp && !Blind
                        && glyph_is_invisible(levl[x][y].glyph)) {
                        unmap_object(x, y);
                        newsym(x, y);
                    }

                    if ((trap = t_at(x, y)) && !trap->tseen && !rnl(8)) {
                        nomul(0);
                        if (trap->ttyp == STATUE_TRAP) {
                            if (activate_statue_trap(trap, x, y, FALSE))
                                exercise(A_WIS, TRUE);
                            return 1;
                        } else {
                            find_trap(trap);
                        }
                    }
                }
            }
    }
    return 1;
}

/* the 's' command -- explicit searching */
int
dosearch()
{
    return dosearch0(0);
}

/* Pre-map the sokoban levels */
void
sokoban_detect()
{
    register int x, y;
    register struct trap *ttmp;
    register struct obj *obj;

    /* Map the background and boulders */
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            levl[x][y].seenv = SVALL;
            levl[x][y].waslit = TRUE;
            map_background(x, y, 1);
            if ((obj = sobj_at(BOULDER, x, y)) != 0)
                map_object(obj, 1);
        }

    /* Map the traps */
    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
        ttmp->tseen = 1;
        map_trap(ttmp, 1);
        /* set sokoban_rules when there is at least one pit or hole */
        if (ttmp->ttyp == PIT || ttmp->ttyp == HOLE)
            Sokoban = 1;
    }
}

/* idea from crawl; show known portion of map without any monsters,
   objects, or traps occluding the view of the underlying terrain */
void
reveal_terrain(full, which_subset)
int full; /* wizard|explore modes allow player to request full map */
int which_subset; /* when not full, whether to suppress objs and/or traps */
{
    if ((Hallucination || Stunned || Confusion) && !full) {
        You("are too disoriented for this.");
    } else {
        int x, y, glyph, levl_glyph, default_glyph;
        uchar seenv;
        unsigned save_swallowed;
        struct monst *mtmp;
        struct trap *t;
        char buf[BUFSZ];
        boolean keep_traps = (which_subset & 1) !=0,
                keep_objs = (which_subset & 2) != 0,
                keep_mons = (which_subset & 4) != 0; /* actually always 0 */

        save_swallowed = u.uswallow;
        iflags.save_uinwater = u.uinwater, iflags.save_uburied = u.uburied;
        u.uinwater = u.uburied = 0;
        u.uswallow = 0;
        default_glyph = cmap_to_glyph(level.flags.arboreal ? S_tree : S_stone);
        /* for 'full', show the actual terrain for the entire level,
           otherwise what the hero remembers for seen locations with
           monsters, objects, and/or traps removed as caller dictates */
        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++) {
                seenv = (full || level.flags.hero_memory)
                           ? levl[x][y].seenv : cansee(x, y) ? SVALL : 0;
                if (full) {
                    levl[x][y].seenv = SVALL;
                    glyph = back_to_glyph(x, y);
                    levl[x][y].seenv = seenv;
                } else {
                    levl_glyph = level.flags.hero_memory
                                    ? levl[x][y].glyph
                                    : seenv
                                       ? back_to_glyph(x, y)
                                       : default_glyph;
                    /* glyph_at() returns the displayed glyph, which might
                       be a monster.  levl[][].glyph contains the remembered
                       glyph, which will never be a monster (unless it is
                       the invisible monster glyph, which is handled like
                       an object, replacing any object or trap at its spot) */
                    glyph = !save_swallowed ? glyph_at(x, y) : levl_glyph;
                    if (keep_mons && x == u.ux && y == u.uy && save_swallowed)
                        glyph = mon_to_glyph(u.ustuck);
                    else if (((glyph_is_monster(glyph)
                               || glyph_is_warning(glyph)) && !keep_mons)
                             || glyph_is_swallow(glyph))
                        glyph = levl_glyph;
                    if (((glyph_is_object(glyph) && !keep_objs)
                         || glyph_is_invisible(glyph))
                        && keep_traps && !covers_traps(x, y)) {
                        if ((t = t_at(x, y)) != 0 && t->tseen)
                            glyph = trap_to_glyph(t);
                    }
                    if ((glyph_is_object(glyph) && !keep_objs)
                        || (glyph_is_trap(glyph) && !keep_traps)
                        || glyph_is_invisible(glyph)) {
                        if (!seenv) {
                            glyph = default_glyph;
                        } else if (lastseentyp[x][y] == levl[x][y].typ) {
                            glyph = back_to_glyph(x, y);
                        } else {
                            /* look for a mimic here posing as furniture;
                               if we don't find one, we'll have to fake it */
                            if ((mtmp = m_at(x, y)) != 0
                                && mtmp->m_ap_type == M_AP_FURNITURE) {
                                glyph = cmap_to_glyph(mtmp->mappearance);
                            } else {
                                /* we have a topology type but we want a
                                   screen symbol in order to derive a glyph;
                                   some screen symbols need the flags field
                                   of levl[][] in addition to the type
                                   (to disambiguate STAIRS to S_upstair or
                                   S_dnstair, for example; current flags
                                   might not be intended for remembered
                                   type, but we've got no other choice) */
                                schar save_typ = levl[x][y].typ;

                                levl[x][y].typ = lastseentyp[x][y];
                                glyph = back_to_glyph(x, y);
                                levl[x][y].typ = save_typ;
                            }
                        }
                    }
                }
                show_glyph(x, y, glyph);
            }

        /* [TODO: highlight hero's location somehow] */
        u.uinwater = iflags.save_uinwater, u.uburied = iflags.save_uburied;
        if (save_swallowed)
            u.uswallow = 1;
        flush_screen(1);
        if (full) {
            Strcpy(buf, "underlying terrain");
        } else {
            Strcpy(buf, "known terrain");
            if (keep_traps)
                Sprintf(eos(buf), "%s traps",
                        (keep_objs || keep_mons) ? "," : " and");
            if (keep_objs)
                Sprintf(eos(buf), "%s%s objects",
                        (keep_traps || keep_mons) ? "," : "",
                        keep_mons ? "" : " and");
            if (keep_mons)
                Sprintf(eos(buf), "%s and monsters",
                        (keep_traps || keep_objs) ? "," : "");
        }
        pline("Showing %s only...", buf);
        display_nhwindow(WIN_MAP, TRUE); /* give "--More--" prompt */
        docrt(); /* redraw the screen, restoring regular map */
        if (Underwater)
            under_water(2);
        if (u.uburied)
            under_ground(2);
    }
    return;
}

/*detect.c*/

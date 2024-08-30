/* NetHack 3.7	detect.c	$NHDT-Date: 1721684299 2024/07/22 21:38:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.180 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Detection routines, including crystal ball, magic mapping, and search
 * command.
 */

#include "hack.h"
#include "artifact.h"

#ifndef FOUND_FLASH_COUNT
/* for screen alert shown to player when secret door detection or ^E
   finds stuff; to use tmp_at() instead of flash_glyph_at(), define as 0;
   extra code for tmp_at() will be included and the flash_glyph_at()
   calls will execute but won't do anything */
#define FOUND_FLASH_COUNT 6
#endif

struct found_things;

staticfn boolean unconstrain_map(void);
staticfn void reconstrain_map(void);
staticfn void map_redisplay(void);
staticfn void browse_map(unsigned, const char *);
staticfn void map_monst(struct monst *, boolean);
staticfn void do_dknown_of(struct obj *);
staticfn boolean check_map_spot(coordxy, coordxy, char, unsigned);
staticfn boolean clear_stale_map(char, unsigned);
staticfn void sense_trap(struct trap *, coordxy, coordxy, int);
staticfn int detect_obj_traps(struct obj *, boolean, int,
                              struct found_things *) NO_NNARGS;
staticfn void display_trap_map(int);
staticfn int furniture_detect(void);
staticfn void foundone(coordxy, coordxy, int);
staticfn void findone(coordxy, coordxy, genericptr_t);
staticfn void openone(coordxy, coordxy, genericptr_t);
staticfn int mfind0(struct monst *, boolean);
staticfn int reveal_terrain_getglyph(coordxy, coordxy, unsigned, int,
                                     unsigned);

/* dummytrap: used when detecting traps finds a door or chest trap; the
   couple of fields that matter are always re-initialized during use so
   this does not need to be part of 'struct instance_globals g'; fields
   that aren't used are compile-/link-/load-time initialized to 0 */
static struct trap dummytrap;

/* data for enhanced feedback from findone() */
struct found_things {
    coord ft_cc; /* for passing extra info to detect_obj_traps() */
    uchar num_sdoors;
    uchar num_scorrs;
    uchar num_traps;
    uchar num_mons;
    uchar num_invis;
    uchar num_cleared_invis;
    uchar num_kept_invis;
};

/* wildcard class for clear_stale_map - this used to be used as a getobj()
   input but it's no longer used for that function */
#define ALL_CLASSES (MAXOCLASSES + 1)

/* bring hero out from underwater or underground or being engulfed;
   return True iff any change occurred */
staticfn boolean
unconstrain_map(void)
{
    boolean res = u.uinwater || u.uburied || u.uswallow;

    /* bring Underwater, buried, or swallowed hero to normal map;
       bypass set_uinwater() */
    iflags.save_uinwater = u.uinwater, u.uinwater = 0;
    iflags.save_uburied  = u.uburied,  u.uburied  = 0;
    iflags.save_uswallow = u.uswallow, u.uswallow = 0;

    return res;
}

/* put hero back underwater or underground or engulfed */
staticfn void
reconstrain_map(void)
{
    /* if was in water and taken out, put back; bypass set_uinwater() */
    u.uinwater = iflags.save_uinwater, iflags.save_uinwater = 0;
    u.uburied  = iflags.save_uburied,  iflags.save_uburied  = 0;
    u.uswallow = iflags.save_uswallow, iflags.save_uswallow = 0;
}

staticfn void
map_redisplay(void)
{
    reconstrain_map();
    docrt(); /* redraw the screen to remove unseen traps from the map */
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
}

/* use getpos()'s 'autodescribe' to view whatever is currently shown on map */
staticfn void
browse_map(unsigned ter_typ, const char *ter_explain)
{
    coord dummy_pos; /* don't care whether player actually picks a spot */
    boolean save_autodescribe;

    dummy_pos.x = u.ux, dummy_pos.y = u.uy; /* starting spot for getpos() */
    save_autodescribe = iflags.autodescribe;
    iflags.autodescribe = TRUE;
    iflags.terrainmode = ter_typ;
    (void) getpos(&dummy_pos, FALSE, ter_explain);
    iflags.terrainmode = 0;
    iflags.autodescribe = save_autodescribe;
}

/* extracted from monster_detection() so can be shared by do_vicinity_map() */
staticfn void
map_monst(struct monst *mtmp, boolean showtail)
{
    int glyph = (monsym(mtmp->data) == ' ')
                ? detected_mon_to_glyph(mtmp, newsym_rn2)
                : mtmp->mtame
                  ? pet_to_glyph(mtmp, newsym_rn2)
                  : mon_to_glyph(mtmp, newsym_rn2);

    show_glyph(mtmp->mx, mtmp->my, glyph);

    if (showtail && mtmp->data == &mons[PM_LONG_WORM])
        detect_wsegs(mtmp, 0);
}

/* this is checking whether a trap symbol represents a trapped chest,
   not whether a trapped chest is actually present */
boolean
trapped_chest_at(int ttyp, coordxy x, coordxy y)
{
    struct monst *mtmp;
    struct obj *otmp;

    if (!glyph_is_trap(glyph_at(x, y)))
        return FALSE;
    if (ttyp != TRAPPED_CHEST || (Hallucination && rn2(20)))
        return FALSE;

    /*
     * TODO?  We should check containers recursively like the trap
     * detecting routine does.  Chests and large boxes do not nest in
     * themselves or each other, but could be contained inside statues.
     *
     * For farlook, we should also check for buried containers, but
     * for '^' command to examine adjacent trap glyph, we shouldn't.
     */

    /* on map, presence of any trappable container will do */
    if (sobj_at(CHEST, x, y) || sobj_at(LARGE_BOX, x, y))
        return TRUE;
    /* in inventory, we need to find one which is actually trapped */
    if (u_at(x, y)) {
        for (otmp = gi.invent; otmp; otmp = otmp->nobj)
            if (Is_box(otmp) && otmp->otrapped)
                return TRUE;
        if (u.usteed) { /* steed isn't on map so won't be found by m_at() */
            for (otmp = u.usteed->minvent; otmp; otmp = otmp->nobj)
                if (Is_box(otmp) && otmp->otrapped)
                    return TRUE;
        }
    }
    if ((mtmp = m_at(x, y)) != 0)
        for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
            if (Is_box(otmp) && otmp->otrapped)
                return TRUE;
    return FALSE;
}

/* this is checking whether a trap symbol represents a trapped door,
   not whether the door here is actually trapped */
boolean
trapped_door_at(int ttyp, coordxy x, coordxy y)
{
    struct rm *lev;

    if (!glyph_is_trap(glyph_at(x, y)))
        return FALSE;
    if (ttyp != TRAPPED_DOOR || (Hallucination && rn2(20)))
        return FALSE;
    lev = &levl[x][y];
    if (!IS_DOOR(lev->typ))
        return FALSE;
    if ((lev->doormask & (D_NODOOR | D_BROKEN | D_ISOPEN)) != 0
         && trapped_chest_at(ttyp, x, y))
        return FALSE;
    return TRUE;
}

/* recursively search obj for an object in class oclass, return 1st found */
struct obj *
o_in(struct obj *obj, char oclass)
{
    struct obj *otmp;
    struct obj *temp;

    if (obj->oclass == oclass)
        return obj;
    /*
     * Note:  we exclude SchroedingersBox because the corpse it contains
     * isn't necessarily a corpse yet.  Resolving the status would lead
     * to complications if it turns out to be a live cat.  We know that
     * that Box can't contain anything else because putting something in
     * would resolve the cat/corpse situation and convert to ordinary box.
     */
    if (Has_contents(obj) && !SchroedingersBox(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (otmp->oclass == oclass)
                return otmp;
            else if (Has_contents(otmp) && (temp = o_in(otmp, oclass)) != 0)
                return temp;
    }
    return (struct obj *) 0;
}

/* Recursively search obj for an object made of specified material.
 * Return first found.
 */
struct obj *
o_material(struct obj *obj, unsigned material)
{
    struct obj *otmp;
    struct obj *temp;

    if (objects[obj->otyp].oc_material == material)
        return obj;

    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (objects[otmp->otyp].oc_material == material)
                return otmp;
            else if (Has_contents(otmp)
                     && (temp = o_material(otmp, material)) != 0)
                return temp;
    }
    return (struct obj *) 0;
}

staticfn void
do_dknown_of(struct obj *obj)
{
    struct obj *otmp;

    obj->dknown = 1;
    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            do_dknown_of(otmp);
    }
}

/* Check whether the location has an outdated object displayed on it. */
staticfn boolean
check_map_spot(coordxy x, coordxy y, char oclass, unsigned material)
{
    int glyph;
    struct obj *otmp;
    struct monst *mtmp;

    glyph = glyph_at(x, y);
    if (glyph_is_object(glyph)) {
        /* there's some object shown here */
        if (oclass == ALL_CLASSES) {
            return !(svl.level.objects[x][y] /* stale if nothing here */
                     || ((mtmp = m_at(x, y)) != 0 && mtmp->minvent));
        } else {
            if (material
                && objects[glyph_to_obj(glyph)].oc_material == material) {
                /* object shown here is of interest because material matches */
                for (otmp = svl.level.objects[x][y]; otmp;
                     otmp = otmp->nexthere)
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
                for (otmp = svl.level.objects[x][y]; otmp;
                     otmp = otmp->nexthere)
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
staticfn boolean
clear_stale_map(char oclass, unsigned material)
{
    coordxy zx, zy;
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
gold_detect(struct obj *sobj)
{
    struct obj *obj;
    struct monst *mtmp;
    struct obj gold, *temp = 0;
    boolean stale, ugold = FALSE, steedgold = FALSE;
    int ter_typ = TER_DETECT | TER_OBJ;

    gk.known = stale = clear_stale_map(COIN_CLASS,
                                       (unsigned) (sobj->blessed ? GOLD : 0));

    /* look for gold carried by monsters (might be in a container) */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
            continue;
        if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
            if (mtmp == u.usteed) {
                steedgold = TRUE;
            } else {
                gk.known = TRUE;
                goto outgoldmap; /* skip further searching */
            }
        } else {
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if ((sobj->blessed && o_material(obj, GOLD))
                    || o_in(obj, COIN_CLASS)) {
                    if (mtmp == u.usteed) {
                        steedgold = TRUE;
                    } else {
                        gk.known = TRUE;
                        goto outgoldmap; /* skip further searching */
                    }
                }
        }
    }

    /* look for gold objects */
    for (obj = fobj; obj; obj = obj->nobj) {
        if (sobj->blessed && o_material(obj, GOLD)) {
            gk.known = TRUE;
            if (obj->ox != u.ux || obj->oy != u.uy)
                goto outgoldmap;
        } else if (o_in(obj, COIN_CLASS)) {
            gk.known = TRUE;
            if (obj->ox != u.ux || obj->oy != u.uy)
                goto outgoldmap;
        }
    }

    if (!gk.known) {
        /* no gold found on floor or monster's inventory.
           adjust message if you have gold in your inventory */
        char buf[BUFSZ];

        if (gy.youmonst.data == &mons[PM_GOLD_GOLEM])
            Sprintf(buf, "You feel like a million %s!", currency(2L));
        else if (money_cnt(gi.invent) || hidden_gold(TRUE))
            Strcpy(buf,
               "You feel worried about your future financial situation.");
        else if (steedgold)
            Sprintf(buf, "You feel interested in %s financial situation.",
                    s_suffix(x_monnam(u.usteed,
                                      u.usteed->mtame ? ARTICLE_YOUR
                                                      : ARTICLE_THE,
                                      (char *) 0,
                                      SUPPRESS_SADDLE, FALSE)));
        else
            Strcpy(buf, "You feel materially poor.");

        strange_feeling(sobj, buf);
        return 1;
    }
    /* only under me - no separate display required */
    if (stale)
        docrt();
    You("notice some gold between your %s.", makeplural(body_part(FOOT)));
    return 0;

 outgoldmap:
    cls();

    (void) unconstrain_map();
    /* Discover gold locations. */
    for (obj = fobj; obj; obj = obj->nobj) {
        if (sobj->blessed && (temp = o_material(obj, GOLD)) != 0) {
            if (temp != obj) {
                temp->ox = obj->ox;
                temp->oy = obj->oy;
            }
            map_object(temp, 1);
        } else if ((temp = o_in(obj, COIN_CLASS)) != 0) {
            if (temp != obj) {
                temp->ox = obj->ox;
                temp->oy = obj->oy;
            }
            map_object(temp, 1);
        }
        if (temp && u_at(temp->ox, temp->oy))
            ugold = TRUE;
    }
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
            continue;
        temp = 0;
        if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
            gold = cg.zeroobj; /* ensure oextra is cleared too */
            gold.otyp = GOLD_PIECE;
            gold.quan = (long) rnd(10); /* usually more than 1 */
            gold.ox = mtmp->mx;
            gold.oy = mtmp->my;
            map_object(&gold, 1);
            temp = &gold;
        } else {
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (sobj->blessed && (temp = o_material(obj, GOLD)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break;
                } else if ((temp = o_in(obj, COIN_CLASS)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break;
                }
        }
        if (temp && u_at(temp->ox, temp->oy))
            ugold = TRUE;
    }
    if (!ugold) {
        newsym(u.ux, u.uy);
        ter_typ |= TER_MON; /* so autodescribe will recognize hero */
    }
    You_feel("very greedy, and sense gold!");
    exercise(A_WIS, TRUE);

    browse_map(ter_typ, "gold");

    map_redisplay();
    return 0;
}

/* returns 1 if nothing was detected, 0 if something was detected */
int
food_detect(struct obj *sobj)
{
    struct obj *obj;
    struct monst *mtmp;
    int ct = 0, ctu = 0;
    boolean confused = (Confusion || (sobj && sobj->cursed)), stale;
    char oclass = confused ? POTION_CLASS : FOOD_CLASS;
    const char *what = confused ? something : "food";

    stale = clear_stale_map(oclass, 0);
    if (u.usteed) /* some situations leave steed with stale coordinates */
        u.usteed->mx = u.ux, u.usteed->my = u.uy;

    for (obj = fobj; obj; obj = obj->nobj)
        if (o_in(obj, oclass)) {
            if (u_at(obj->ox, obj->oy))
                ctu++;
            else
                ct++;
        }
    for (mtmp = fmon; mtmp && (!ct || !ctu); mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if (o_in(obj, oclass)) {
                if (u_at(mtmp->mx, mtmp->my))
                    ctu++; /* steed or an engulfer with inventory */
                else
                    ct++;
                break;
            }
    }

    if (!ct && !ctu) {
        gk.known = stale && !confused;
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
        gk.known = TRUE;
        You("%s %s nearby.", sobj ? "smell" : "sense", what);
        if (sobj && sobj->blessed) {
            if (!u.uedibility)
                Your("%s starts to tingle.", body_part(NOSE));
            u.uedibility = 1;
        }
    } else {
        struct obj *temp;
        int ter_typ = TER_DETECT | TER_OBJ;

        gk.known = TRUE;
        cls();
        (void) unconstrain_map();
        for (obj = fobj; obj; obj = obj->nobj)
            if ((temp = o_in(obj, oclass)) != 0) {
                if (temp != obj) {
                    temp->ox = obj->ox;
                    temp->oy = obj->oy;
                }
                map_object(temp, 1);
            }
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
                continue;
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if ((temp = o_in(obj, oclass)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break; /* skip rest of this monster's inventory */
                }
        }
        if (!ctu) {
            newsym(u.ux, u.uy);
            ter_typ |= TER_MON; /* for autodescribe of self */
        }
        if (sobj) {
            if (sobj->blessed) {
                Your("%s %s to tingle and you smell %s.", body_part(NOSE),
                     u.uedibility ? "continues" : "starts", what);
                u.uedibility = 1;
            } else
                Your("%s tingles and you smell %s.", body_part(NOSE), what);
        } else
            You("sense %s.", what);
        exercise(A_WIS, TRUE);

        browse_map(ter_typ, "food");

        map_redisplay();
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
object_detect(struct obj *detector, /* object doing the detecting */
              int class)            /* an object class, 0 for all */
{
    coordxy x, y;
    char stuff[BUFSZ];
    int is_cursed = (detector && detector->cursed);
    int do_dknown = (detector && (detector->oclass == POTION_CLASS
                                  || detector->oclass == SPBOOK_CLASS)
                     && detector->blessed);
    int ct = 0, ctu = 0;
    struct obj *obj, *otmp = (struct obj *) 0;
    struct monst *mtmp;
    int sym, boulder = 0, ter_typ = TER_DETECT | TER_OBJ;

    if (class < 0 || class >= MAXOCLASSES) {
        impossible("object_detect:  illegal class %d", class);
        class = 0;
    }

    /* Special boulder symbol check - does the class symbol happen
     * to match showsyms[SYM_BOULDER + SYM_OFF_X] which is user-defined.
     * If so, that means we aren't sure what they really wanted to
     * detect. Rather than trump anything, show both possibilities.
     * We can exclude checking the buried obj chain for boulders below.
     */
    sym = class ? def_oc_syms[class].sym : 0;
    if (sym && sym == gs.showsyms[SYM_BOULDER + SYM_OFF_X])
        boulder = ROCK_CLASS;

    if (Hallucination || (Confusion && class == SCROLL_CLASS))
        Strcpy(stuff, something);
    else
        Strcpy(stuff, class ? def_oc_syms[class].name : "objects");
    if (boulder && class != ROCK_CLASS)
        Strcat(stuff, " and/or large stones");

    if (do_dknown)
        for (obj = gi.invent; obj; obj = obj->nobj)
            do_dknown_of(obj);

    for (obj = fobj; obj; obj = obj->nobj) {
        if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) {
            if (u_at(obj->ox, obj->oy))
                ctu++;
            else
                ct++;
        }
        if (do_dknown)
            do_dknown_of(obj);
    }

    for (obj = svl.level.buriedobjlist; obj; obj = obj->nobj) {
        if (!class || o_in(obj, class)) {
            if (u_at(obj->ox, obj->oy))
                ctu++;
            else
                ct++;
        }
        if (do_dknown)
            do_dknown_of(obj);
    }

    if (u.usteed)
        u.usteed->mx = u.ux, u.usteed->my = u.uy;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj) {
            if ((!class && !boulder) || o_in(obj, class)
                || o_in(obj, boulder))
                ct++;
            if (do_dknown)
                do_dknown_of(obj);
        }
        if ((is_cursed && M_AP_TYPE(mtmp) == M_AP_OBJECT
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

    (void) unconstrain_map();
    /*
     *  Map all buried objects first.
     */
    for (obj = svl.level.buriedobjlist; obj; obj = obj->nobj)
        if (!class || (otmp = o_in(obj, class)) != 0) {
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
            for (obj = svl.level.objects[x][y]; obj; obj = obj->nexthere)
                if ((!class && !boulder) || (otmp = o_in(obj, class)) != 0
                    || (otmp = o_in(obj, boulder)) != 0) {
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
        if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if ((!class && !boulder) || (otmp = o_in(obj, class)) != 0
                || (otmp = o_in(obj, boulder)) != 0) {
                if (!class && !boulder)
                    otmp = obj;
                otmp->ox = mtmp->mx; /* at monster location */
                otmp->oy = mtmp->my;
                map_object(otmp, 1);
                break;
            }
        /* Allow a mimic to override the detected objects it is carrying. */
        if (is_cursed && M_AP_TYPE(mtmp) == M_AP_OBJECT
            && (!class || class == objects[mtmp->mappearance].oc_class)) {
            struct obj temp;

            temp = cg.zeroobj;
            temp.otyp = mtmp->mappearance; /* needed for obj_to_glyph() */
            temp.quan = 1L;
            temp.ox = mtmp->mx;
            temp.oy = mtmp->my;
            /* used for mimicking a corpse or statue */
            temp.corpsenm = has_mcorpsenm(mtmp) ? MCORPSENM(mtmp) : PM_TENGU;
            map_object(&temp, 1);
        } else if (findgold(mtmp->minvent)
                   && (!class || class == COIN_CLASS)) {
            struct obj gold;

            gold = cg.zeroobj; /* ensure oextra is cleared too */
            gold.otyp = GOLD_PIECE;
            gold.quan = (long) rnd(10); /* usually more than 1 */
            gold.ox = mtmp->mx;
            gold.oy = mtmp->my;
            map_object(&gold, 1);
        }
    }
    if (!glyph_is_object(glyph_at(u.ux, u.uy))) {
        newsym(u.ux, u.uy);
        ter_typ |= TER_MON;
    }
    You("detect the %s of %s.", ct ? "presence" : "absence", stuff);

    if (!ct)
        display_nhwindow(WIN_MAP, TRUE);
    else
        browse_map(ter_typ, "object");

    map_redisplay();
    return 0;
}

/*
 * Used by: crystal balls, potions, fountains
 *
 * Returns 1 if nothing was detected.
 * Returns 0 if something was detected.
 */
int
monster_detect(struct obj *otmp, /* detecting object (if any) */
               int mclass)       /* monster class, 0 for all */
{
    struct monst *mtmp;
    int mcnt = 0;

    /* Note: This used to just check fmon for a non-zero value
     * but in versions since 3.3.0 fmon can test TRUE due to the
     * presence of dmons, so we have to find at least one
     * with positive hit-points to know for sure.
     */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
            continue;
        ++mcnt;
        break; /* no need for full count, just 1 or more vs 0 */
    }

    if (!mcnt) {
        if (otmp)
            strange_feeling(otmp, Hallucination
                                      ? "You get the heebie jeebies."
                                      : "You feel threatened.");
        return 1;
    } else {
        boolean unconstrained, woken = FALSE;
        unsigned swallowed = u.uswallow; /* before unconstrain_map() */

        cls();
        unconstrained = unconstrain_map();
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx))
                continue;
            if (!mclass || mtmp->data->mlet == mclass
                || (mtmp->data == &mons[PM_LONG_WORM]
                    && mclass == S_WORM_TAIL))
                map_monst(mtmp, TRUE);

            if (otmp && otmp->cursed && helpless(mtmp)) {
                mtmp->msleeping = mtmp->mfrozen = 0;
                mtmp->mcanmove = 1;
                woken = TRUE;
            }
        }
        if (!swallowed)
            display_self();
        You("sense the presence of monsters.");
        if (woken)
            pline("Monsters sense the presence of you.");

        if ((otmp && otmp->blessed) && !unconstrained) {
            /* persistent detection--just show updated map */
            display_nhwindow(WIN_MAP, TRUE);
        } else {
            /* one-shot detection--allow player to move cursor around and
               get autodescribe feedback */
            EDetect_monsters |= I_SPECIAL;
            browse_map(TER_DETECT | TER_MON, "monster of interest");
            EDetect_monsters &= ~I_SPECIAL;
        }

        map_redisplay();
    }
    return 0;
}

staticfn void
sense_trap(struct trap *trap, coordxy x, coordxy y, int src_cursed)
{
    if (Hallucination || src_cursed) {
        struct obj obj; /* fake object */

        obj = cg.zeroobj;
        if (trap) {
            obj.ox = trap->tx;
            obj.oy = trap->ty;
        } else {
            obj.ox = x;
            obj.oy = y;
        }
        obj.otyp = !Hallucination ? GOLD_PIECE : random_object(rn2);
        obj.quan = (long) ((obj.otyp == GOLD_PIECE) ? rnd(10)
                           : objects[obj.otyp].oc_merge ? rnd(2) : 1);
        obj.corpsenm = random_monster(rn2); /* if otyp == CORPSE */
        map_object(&obj, 1);
    } else if (trap) {
        map_trap(trap, 1);
        trap->tseen = 1;
    } else {
        /*
         * OBSOLETE; this was for trapped door or trapped chest
         * but those are handled by 'if (trap) {map_trap()}' now
         * and this block of code shouldn't be reachable anymore.
         */
        dummytrap.tx = x;
        dummytrap.ty = y;
        dummytrap.ttyp = BEAR_TRAP; /* some kind of trap */
        map_trap(&dummytrap, 1);
    }
}

#define OTRAP_NONE 0  /* nothing found */
#define OTRAP_HERE 1  /* found at hero's location */
#define OTRAP_THERE 2 /* found at any other location */

/* check a list of objects for chest traps; return 1 if found at <ux,uy>,
   2 if found at some other spot, 3 if both, 0 otherwise; optionally
   update the map to show where such traps were found */
staticfn int
detect_obj_traps(
    struct obj *objlist,
    boolean show_them,
    int how, /* 1 for misleading map feedback */
    struct found_things *ft) /* being called by findone() when non-Null */
{
    struct obj *otmp;
    coordxy x, y;
    int trapglyph, result = OTRAP_NONE;

    /*
     * TODO?  Display locations of unarmed land mine and beartrap objects.
     * If so, should they be displayed as objects or as traps?
     */

    dummytrap.ttyp = TRAPPED_CHEST;
    trapglyph = ft ? trap_to_glyph(&dummytrap) : GLYPH_NOTHING;
    for (otmp = objlist; otmp; otmp = otmp->nobj) {
        x = y = 0; /* lint suppression */
        if ((Is_box(otmp) && otmp->otrapped) || Has_contents(otmp)) {
            /* !get_obj_location and !isok should both be impossible here */
            if (!get_obj_location(otmp, &x, &y, BURIED_TOO | CONTAINED_TOO)
                || !isok(x, y)
                || (ft && (x != ft->ft_cc.x || y != ft->ft_cc.y)))
                continue;
        }
        if (Is_box(otmp) && otmp->otrapped) {
            result |= u_at(x, y) ? OTRAP_HERE : OTRAP_THERE;
            if (ft) {
                flash_glyph_at(x, y, trapglyph, FOUND_FLASH_COUNT);
            }
            if (show_them) {
                dummytrap.tx = x, dummytrap.ty = y;
                sense_trap(&dummytrap, x, y, how);
            }
            if (ft) {
                foundone(x, y, trapglyph);
                ft->num_traps++;
            }
        }
        if (Has_contents(otmp))
            result |= detect_obj_traps(otmp->cobj, show_them, how, ft);
    }
    return result;
}

staticfn void
display_trap_map(int cursed_src)
{
    struct monst *mon;
    struct trap *ttmp;
    int door, glyph, ter_typ = TER_DETECT | (cursed_src ? TER_OBJ : TER_TRP);
    coord cc;

    cls();

    (void) unconstrain_map();
    /* show chest traps first, first buried chests then floor chests, so
       that subsequent floor trap display will override if both types are
       present at the same location */
    (void) detect_obj_traps(svl.level.buriedobjlist, TRUE, cursed_src, NULL);
    (void) detect_obj_traps(fobj, TRUE, cursed_src, NULL);
    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon) || (mon->isgd && !mon->mx))
            continue;
        (void) detect_obj_traps(mon->minvent, TRUE, cursed_src, NULL);
    }
    (void) detect_obj_traps(gi.invent, TRUE, cursed_src, NULL);

    for (ttmp = gf.ftrap; ttmp; ttmp = ttmp->ntrap)
        sense_trap(ttmp, 0, 0, cursed_src);

    dummytrap.ttyp = TRAPPED_DOOR;
    for (door = 0; door < gd.doorindex; door++) {
        cc = svd.doors[door];
        if (levl[cc.x][cc.y].typ == SDOOR) /* can't be trapped; see above */
            continue;
        if (levl[cc.x][cc.y].doormask & D_TRAPPED) {
            dummytrap.tx = cc.x, dummytrap.ty = cc.y;
            sense_trap(&dummytrap, cc.x, cc.y, cursed_src);
        }
    }

    /* redisplay hero unless sense_trap() revealed something at <ux,uy> */
    glyph = glyph_at(u.ux, u.uy);
    if (!(glyph_is_trap(glyph) || glyph_is_object(glyph))) {
        newsym(u.ux, u.uy);
        ter_typ |= TER_MON; /* for autodescribe at <u.ux,u.uy> */
    }
    You_feel("%s.", cursed_src ? "very greedy" : "entrapped");

    browse_map(ter_typ, cursed_src ? "gold" : "trap of interest");

    map_redisplay();
}

/* the detections are pulled out so they can
 * also be used in the crystal ball routine
 * returns 1 if nothing was detected
 * returns 0 if something was detected
 */
int
trap_detect(
    struct obj *sobj) /* Null if crystal ball, scroll if gold detection */
{
    struct trap *ttmp;
    struct monst *mon;
    int door, tr;
    int cursed_src = sobj && sobj->cursed;
    boolean found = FALSE;
    coord cc;

    if (u.usteed)
        u.usteed->mx = u.ux, u.usteed->my = u.uy;

    /* floor/ceiling traps */
    for (ttmp = gf.ftrap; ttmp; ttmp = ttmp->ntrap) {
        if (ttmp->tx != u.ux || ttmp->ty != u.uy) {
            display_trap_map(cursed_src);
            return 0;
        }
        found = TRUE;
    }
    /* chest traps (might be buried or carried) */
    if ((tr = detect_obj_traps(fobj, FALSE, 0, NULL)) != OTRAP_NONE) {
        if (tr & OTRAP_THERE) {
            display_trap_map(cursed_src);
            return 0;
        }
        found = TRUE;
    }
    if ((tr = detect_obj_traps(svl.level.buriedobjlist, FALSE, 0, NULL))
        != OTRAP_NONE) {
        if (tr & OTRAP_THERE) {
            display_trap_map(cursed_src);
            return 0;
        }
        found = TRUE;
    }
    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon) || (mon->isgd && !mon->mx))
            continue;
        if ((tr = detect_obj_traps(mon->minvent, FALSE, 0, NULL))
            != OTRAP_NONE) {
            if (tr & OTRAP_THERE) {
                display_trap_map(cursed_src);
                return 0;
            }
            found = TRUE;
        }
    }
    if (detect_obj_traps(gi.invent, FALSE, 0, NULL) != OTRAP_NONE)
        found = TRUE;
    /* door traps */
    for (door = 0; door < gd.doorindex; door++) {
        cc = svd.doors[door];
        /* levl[][].doormask and .wall_info both overlay levl[][].flags;
           the bit in doormask for D_TRAPPED is also a bit in wall_info;
           secret doors use wall_info so can't be marked as trapped */
        if (levl[cc.x][cc.y].typ == SDOOR)
            continue;
        if (levl[cc.x][cc.y].doormask & D_TRAPPED) {
            if (cc.x != u.ux || cc.y != u.uy) {
                display_trap_map(cursed_src);
                return 0;
            }
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
}

staticfn int
furniture_detect(void)
{
    struct monst *mon;
    coordxy x, y;
    int glyph, sym, found = 0, revealed = 0;

    (void) unconstrain_map();

    for (y = 0; y < ROWNO; ++y)
        for (x = 1; x < COLNO; ++x) {
            glyph = glyph_at(x, y);
            sym = glyph_to_cmap(glyph);
            if (IS_FURNITURE(levl[x][y].typ)) {
                ++found;
                magic_map_background(x, y, 1);
            } else if (is_cmap_furniture(sym)) {
                ++found;
                if ((mon = m_at(x, y)) != 0
                    && M_AP_TYPE(mon) == M_AP_FURNITURE)
                    seemimic(mon);
                if (!mon || !canspotmon(mon))
                    map_invisible(x, y);
            }
            if (glyph_at(x, y) != glyph)
                ++revealed;
        }

    if (!found)
        There("seems to be nothing of interest on this level.");
    else if (!revealed)
        /* [what about clipped map with points of interest outside of the
            currently shown area?] */
        Your("map already shows all relevant locations.");

    if (!revealed)
        display_nhwindow(WIN_MAP, TRUE);
    else /* we need to browse all types because we haven't redrawn the map
          * with only points of interest */
        browse_map(TER_DETECT | TER_MAP | TER_TRP | TER_OBJ | TER_MON,
                   "location");

    map_redisplay();
    return 0;
}

/* way back in 3.0plN and/or 2.x, you could use a crystal ball to find out
   where the wizard was relative to your current location; that was when the
   Wizard guarded the Amulet and was located on a random maze level, and you
   were expected to level teleport deep into Hell and hunt for him while
   working your way up; this isn't of much use anymore */
const char *
level_distance(d_level *where)
{
    schar ll = depth(&u.uz) - depth(where);
    boolean indun = (u.uz.dnum == where->dnum);
    const char *res = ""; /* always replaced by some other non-Null value */

    if (ll < 0) {
        if (ll < (-8 - rn2(3)))
            if (!indun)
                res = "far away";
            else
                res = "far below";
        else if (ll < -1)
            if (!indun)
                res = "away below you";
            else
                res = "below you";
        else if (!indun)
            res = "in the distance";
        else
            res = "just below";
    } else if (ll > 0) {
        if (ll > (8 + rn2(3)))
            if (!indun)
                res = "far away";
            else
                res = "far above";
        else if (ll > 1)
            if (!indun)
                res = "away above you";
            else
                res = "above you";
        else if (!indun)
            res = "in the distance";
        else
            res = "just above";
    } else { /* l1 == 0 */
        if (!indun)
            res = "in the distance";
        else
            res = "near you";
    }
    return res;
}

    /*
     * This could be made a lot more useful.  Especially now that
     * amnesia no longer causes levels to be forgotten.  Perhaps a
     * menu, and it ought to include the entrance to Vlad's Tower,
     * one of the few things that requires active searching/mapping
     * to find.  And once the Wizard is in play, he is easy for the
     * game to locate but not necessarily for the player.
     */
static const struct crystalballlevels {
    const char *what;
    d_level *where;
} level_detects[] = {
    { "Delphi", &oracle_level },
    { "Medusa's lair", &medusa_level },
    { "a castle", &stronghold_level },
    { "the Wizard of Yendor's tower", &wiz1_level },
};

void
use_crystal_ball(struct obj **optr)
{
    char ch;
    int oops;
    struct obj *obj = *optr;
    boolean charged = (obj->spe > 0);

    if (Blind) {
        pline("Too bad you can't see %s.", the(xname(obj)));
        return;
    }
    oops = is_quest_artifact(obj) ? 8 : obj->blessed ? 16 : 20;
    if (charged && (obj->cursed || rnd(oops) > ACURR(A_INT))) {
        long impair = (long) rnd(100 - 3 * ACURR(A_INT));

        switch (rnd((obj->oartifact || obj->blessed) ? 4 : 5)) {
        case 1:
            pline("%s too much to comprehend!", Tobjnam(obj, "are"));
            break;
        case 2:
            pline("%s you!", Tobjnam(obj, "confuse"));
            make_confused((HConfusion & TIMEOUT) + impair, FALSE);
            break;
        case 3:
            if (!resists_blnd(&gy.youmonst)) {
                pline("%s your vision!", Tobjnam(obj, "damage"));
                make_blinded(BlindedTimeout + impair, FALSE);
                if (!Blind)
                    Your1(vision_clears);
            } else {
                pline("%s your vision.", Tobjnam(obj, "assault"));
                You("are unaffected!");
            }
            break;
        case 4:
            pline("%s your mind!", Tobjnam(obj, "zap"));
            (void) make_hallucinated((HHallucination & TIMEOUT) + impair,
                                     FALSE, 0L);
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
        nomul(-rnd(charged ? 4 : 2));
        gm.multi_reason = "gazing into a Magic 8-Ball (tm)";
        gn.nomovemsg = "";

        if (!charged) {
            pline("All you see is funky %s haze.", hcolor((char *) 0));
            if (obj->spe < 0)
                goto implode; /* destroy it when it has been cancelled */
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
        You("may look for an object, monster, or special map symbol.");
    ch = yn_function("What do you look for?", (char *) 0, '\0', TRUE);
    /* Don't filter out ' ' here; it has a use */
    if ((ch != def_monsyms[S_GHOST].sym) && strchr(quitchars, ch)) {
        if (flags.verbose)
            pline1(Never_mind);
        return;
    }
    /* Possible extension:
     *  If ch=='?', ask whether player wants to find scrolls or is asking
     *  for help in using the crystal ball.
     */

    You("peer into %s...", the(xname(obj)));
    nomul(-rnd(charged ? 10 : 2));
    gm.multi_reason = "gazing into a crystal ball";
    gn.nomovemsg = "";

    if (!charged) {
        pline_The("vision is unclear.");

        if (obj->spe < 0) { /* destroy ball if used after being cancelled */
 implode:   /* no damage to hero but 'multi' has a small negative value */
            pline("%s!", Tobjnam(obj, "implode"));
            useup(obj);
            *optr = obj = (struct obj *) 0; /* it's gone */
            return;
        }
    } else {
        int class, i;
        int ret = 0;

        makeknown(CRYSTAL_BALL);
        consume_obj_charge(obj, TRUE);

        /* special case: accept ']' as synonym for mimic
         * we have to do this before the def_char_to_objclass check
         */
        if (ch == DEF_MIMIC_DEF)
            ch = DEF_MIMIC;

        /* checking furniture before objects allows '_' to find altars
           (along with other furniture) instead of finding iron chains */
        if (def_char_is_furniture(ch) >= 0) {
            ret = furniture_detect();
        } else if ((class = def_char_to_objclass(ch)) != MAXOCLASSES) {
            ret = object_detect((struct obj *) 0, class);
        } else if ((class = def_char_to_monclass(ch)) != MAXMCLASSES) {
            ret = monster_detect((struct obj *) 0, class);
        } else if (gs.showsyms[SYM_BOULDER + SYM_OFF_X]
                 && (ch == gs.showsyms[SYM_BOULDER + SYM_OFF_X])) {
            ret = object_detect((struct obj *) 0, ROCK_CLASS);
        } else if (ch == '^') {
            ret = trap_detect((struct obj *) 0);
        } else {
            i = rn2(SIZE(level_detects));
            You_see("%s, %s.", level_detects[i].what,
                    level_distance(level_detects[i].where));
            ret = 0;
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

/* used by magic mapping, clairvoyance, and wand of probing */
void
show_map_spot(coordxy x, coordxy y, boolean cnf)
{
    struct rm *lev;
    struct trap *t;
    struct engr *ep;
    int oldglyph;

    if (cnf && rn2(7))
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
    if (svl.level.flags.hero_memory) {
        magic_map_background(x, y, 0);
        newsym(x, y); /* show it, if not blocked */
    } else {
        magic_map_background(x, y, 1); /* display it */
    }
    if (!IS_FURNITURE(lev->typ)) {
        if ((t = t_at(x, y)) != 0 && t->tseen) {
            map_trap(t, 1);
        } else if ((ep = engr_at(x, y)) != 0) {
            map_engraving(ep, 1);
        } else if (glyph_is_trap(oldglyph) || glyph_is_object(oldglyph)) {
            show_glyph(x, y, oldglyph);
            if (svl.level.flags.hero_memory)
                lev->glyph = oldglyph;
        }
    }
    /* possibly update #overview */
    if (!cnf && lev->roomno >= ROOMOFFSET)
        room_discovered(lev->roomno - ROOMOFFSET);
}

void
do_mapping(void)
{
    int zx, zy;
    boolean unconstrained;

    unconstrained = unconstrain_map();
    for (zx = 1; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            show_map_spot(zx, zy, Confusion);

    if (!svl.level.flags.hero_memory || unconstrained) {
        flush_screen(1);                 /* flush temp screen */
        /* browse_map() instead of display_nhwindow(WIN_MAP, TRUE) */
        browse_map(TER_DETECT | TER_MAP | TER_TRP | TER_OBJ,
                   "anything of interest");
        map_redisplay(); /* calls reconstrain_map() and docrt() */
    } else {
        /* we only get here when unconstrained is False, so reconstrain_map
           will be a no-op; call it anyway */
        reconstrain_map();
    }
    exercise(A_WIS, TRUE);
}

/* clairvoyance */
void
do_vicinity_map(
    struct obj *sobj) /* scroll--actually fake spellbook--object */
{
    int zx, zy;
    struct monst *mtmp;
    struct obj *otmp;
    long save_EDetect_mons;
    char save_viz_uyux;
    boolean unconstrained, refresh = FALSE,
            mdetected = FALSE, odetected = FALSE,
            /* fake spellbook 'sobj' implies hero has cast the spell;
               when book is blessed, casting is skilled or expert level;
               if already clairvoyant, non-skilled spell acts like skilled */
            extended = (sobj && (sobj->blessed || Clairvoyant)),
            random_farsight = !sobj;
    int newglyph, oldglyph,
        lo_y = ((u.uy - 5 < 0) ? 0 : u.uy - 5),
        hi_y = ((u.uy + 6 >= ROWNO) ? ROWNO - 1 : u.uy + 6),
        lo_x = ((u.ux - 9 < 1) ? 1 : u.ux - 9), /* avoid column 0 */
        hi_x = ((u.ux + 10 >= COLNO) ? COLNO - 1 : u.ux + 10),
        ter_typ = TER_DETECT | TER_MAP | TER_TRP | TER_OBJ;

    /*
     * 3.6.0 attempted to emphasize terrain over transient map
     * properties (monsters and objects) but that led to problems.
     * Notably, known trap would be displayed instead of a monster
     * on or in it and then the display remained that way after the
     * clairvoyant snapshot finished.  That could have been fixed by
     * issuing --More-- and then regular vision update, but we want
     * to avoid that when having a clairvoyant episode every N turns
     * (from donating to a temple priest or by carrying the Amulet).
     * Unlike when casting the spell, it is much too intrusive when
     * in the midst of walking around or combatting monsters.
     *
     * As of 3.6.2, show terrain, then object, then monster like regular
     * map updating, except in this case the map locations get marked
     * as seen from every direction rather than just from direction of
     * hero.  Skilled spell marks revealed objects as 'seen up close'
     * (but for piles, only the top item) and shows monsters as if
     * detected.  Non-skilled and timed clairvoyance reveals non-visible
     * monsters as 'remembered, unseen'.
     */

    /* if hero is engulfed, show engulfer at <u.ux,u.uy> */
    save_viz_uyux = gv.viz_array[u.uy][u.ux];
    if (u.uswallow)
        gv.viz_array[u.uy][u.ux] |= IN_SIGHT; /* <x,y> are reversed, [y][x] */
    save_EDetect_mons = EDetect_monsters;
    /* for skilled spell, getpos() scanning of the map will display all
       monsters within range; otherwise, "unseen creature" will be shown */
    EDetect_monsters |= I_SPECIAL;
    unconstrained = unconstrain_map();
    for (zx = lo_x; zx <= hi_x; zx++)
        for (zy = lo_y; zy <= hi_y; zy++) {
            oldglyph = glyph_at(zx, zy);
            /* this will remove 'remembered, unseen mon' (and objects) */
            show_map_spot(zx, zy, Confusion);
            /* if there are any objects here, see the top one */
            if (OBJ_AT(zx, zy)) {
                /* not vobj_at(); this is not vision-based access;
                   unlike object detection, we don't notice buried items */
                otmp = svl.level.objects[zx][zy];
                if (extended)
                    otmp->dknown = 1;
                map_object(otmp, TRUE);
                newglyph = glyph_at(zx, zy);
                /* if otmp is underwater, we'll need to redisplay the water */
                if (newglyph != oldglyph && covers_objects(zx, zy))
                    odetected = TRUE;
            }
            /* if there is a monster here, see or detect it,
               possibly as "remembered, unseen monster" */
            if ((mtmp = m_at(zx, zy)) != 0
                && mtmp->mx == zx && mtmp->my == zy) { /* skip worm tails */
                /* if we're going to offer browse_map()/getpos() scanning of
                   the map and we're not doing extended/blessed clairvoyance
                   (hence must be swallowed or underwater), show "unseen
                   creature" unless map already displayed a monster here */
                if ((unconstrained || !svl.level.flags.hero_memory)
                    && !extended && (zx != u.ux || zy != u.uy)
                    && !glyph_is_monster(oldglyph))
                    map_invisible(zx, zy);
                else
                    map_monst(mtmp, FALSE);
                newglyph = glyph_at(zx, zy);
                if (extended && newglyph != oldglyph
                    && !glyph_is_invisible(newglyph))
                    mdetected = TRUE;
            }
        }

    /* when this instance of clairvoyance is random (see allmain()) and
       the only reason to browse the map is that previously undetected
       monster(s) or object(s) have been revealed, player can prevent
       the you-sense-your-surroundings message and browse operation from
       happening by setting 'quick_farsight' option; for clairvoyance
       spell, that option is ignored because the message and the pause
       for map browsing isn't as intrusive in that circumstance */
    if (random_farsight && flags.quick_farsight)
        mdetected = odetected = FALSE;

    if (!svl.level.flags.hero_memory || unconstrained
        || mdetected || odetected) {
        flush_screen(1);                 /* flush temp screen */
        /* the getpos() prompt from browse_map() is only shown when
           flags.verbose is set, but make this unconditional so that
           not-verbose users become aware of the prompting situation */
        You("sense your surroundings.");
        if (extended || glyph_is_monster(glyph_at(u.ux, u.uy)))
            ter_typ |= TER_MON;
        browse_map(ter_typ, "anything of interest");
        refresh = TRUE;
    }
    reconstrain_map();
    EDetect_monsters = save_EDetect_mons;
    gv.viz_array[u.uy][u.ux] = save_viz_uyux;

    /* replace monsters with remembered,unseen monster, then run
       see_monsters() to update visible ones and warned-of ones */
    for (zx = lo_x; zx <= hi_x; zx++)
        for (zy = lo_y; zy <= hi_y; zy++) {
            if (u_at(zx, zy))
                continue;
            newglyph = glyph_at(zx, zy);
            if (glyph_is_monster(newglyph)
                && glyph_to_mon(newglyph) != PM_LONG_WORM_TAIL) {
                /* map_invisible() was unconditional here but that made
                   remembered objects be forgotten for the case where a
                   monster is immediately redrawn by see_monsters() */
                if ((mtmp = m_at(zx, zy)) == 0 || !canspotmon(mtmp))
                    map_invisible(zx, zy);
            }
        }
    see_monsters();

    if (refresh)
        docrt();
}

/* convert a secret door into a normal door */
void
cvt_sdoor_to_door(struct rm *lev)
{
    int newmask = lev->doormask & ~WM_MASK;

    if (Is_rogue_level(&u.uz)) {
        /* rogue didn't have doors, only doorways */
        newmask = D_NODOOR;
    } else {
        /* newly exposed door is closed */
        if (!(newmask & D_LOCKED))
            newmask |= D_CLOSED;
    }
    lev->typ = DOOR;
    lev->doormask = newmask;
}

/* update the map for something which has just been found by wand of secret
   door detection or wizard mode ^E; will be called multiple times during a
   single operation if multiple things of interest are discovered */
staticfn void
foundone(coordxy zx, coordxy zy, int glyph)
{
    if (glyph_is_cmap(glyph) || glyph_is_unexplored(glyph))
        levl[zx][zy].seenv = SVALL;

    if (!Blind) {
        seenV save_viz = gv.viz_array[zy][zx];

        gv.viz_array[zy][zx] = COULD_SEE | IN_SIGHT;
        newsym(zx, zy);
        gv.viz_array[zy][zx] = save_viz;
    }

#if FOUND_FLASH_COUNT == 0
    /*
     * This works [for non-monsters at present] but flash_glyph_at()
     * seems preferrable because the tmp_at() variation requires that
     * the player respond to --More-- at the end, the flash_glyph
     * variation doesn't.
     */
    tmp_at(DISP_CHANGE, glyph);
    tmp_at(zx, zy);
#endif
}

/* find something at one location; this should find all somethings there
   since it is used for magical detection rather than physical searching */
staticfn void
findone(coordxy zx, coordxy zy, genericptr_t whatfound)
{
    struct rm *lev = &levl[zx][zy];
    struct trap *ttmp = t_at(zx, zy);
    struct monst *mtmp = m_at(zx, zy);
    struct found_things *found_p = (struct found_things *) whatfound;

    if (mtmp && (DEADMONSTER(mtmp) || (mtmp->isgd && !mtmp->mx)))
        mtmp = (struct monst *) NULL;
    found_p->ft_cc.x = zx; /* needed by detect_obj_traps() */
    found_p->ft_cc.y = zy;

    if (lev->typ == SDOOR) {
        nhsym sym = lev->horizontal ? S_hcdoor : S_vcdoor;

        flash_glyph_at(zx, zy, cmap_to_glyph(sym), FOUND_FLASH_COUNT);
        cvt_sdoor_to_door(lev); /* set lev->typ = DOOR */
        magic_map_background(zx, zy, 0);
        foundone(zx, zy, back_to_glyph(zx, zy));
        found_p->num_sdoors++;
    } else if (lev->typ == SCORR) {
        flash_glyph_at(zx, zy, cmap_to_glyph(S_corr), FOUND_FLASH_COUNT);
        lev->typ = CORR;
        unblock_point(zx, zy);
        magic_map_background(zx, zy, 0);
        foundone(zx, zy, cmap_to_glyph(S_corr));
        found_p->num_scorrs++;
    }

    if (ttmp && !ttmp->tseen
        /* [shouldn't successful 'find' reveal and activate statue traps?] */
        && ttmp->ttyp != STATUE_TRAP) {
        flash_glyph_at(zx, zy, trap_to_glyph(ttmp), FOUND_FLASH_COUNT);
        ttmp->tseen = 1;
        foundone(zx, zy, trap_to_glyph(ttmp));
        found_p->num_traps++;
    }
    if (closed_door(zx, zy) && (lev->doormask & D_TRAPPED) != 0) {
        dummytrap.ttyp = TRAPPED_DOOR;
        dummytrap.tx = zx, dummytrap.ty = zy;
        flash_glyph_at(zx, zy, trap_to_glyph(&dummytrap), FOUND_FLASH_COUNT);
        dummytrap.tseen = 1;
        map_trap(&dummytrap, 1);
        sense_trap(&dummytrap, zx, zy, 0); /* handles Hallucination */
        foundone(zx, zy, trap_to_glyph(&dummytrap));
        found_p->num_traps++;
    }
    /* trapped chests */
    (void) detect_obj_traps(svl.level.buriedobjlist, TRUE, 0, found_p);
    (void) detect_obj_traps(fobj, TRUE, 0, found_p);
    if (mtmp)
        (void) detect_obj_traps(mtmp->minvent, TRUE, 0, found_p);
    if (u_at(zx, zy))
        (void) detect_obj_traps(gi.invent, TRUE, 0, found_p);

    if (mtmp && (!canspotmon(mtmp) || mtmp->mundetected || M_AP_TYPE(mtmp))) {
        if (M_AP_TYPE(mtmp)) {
            flash_glyph_at(zx, zy, mon_to_glyph(mtmp, rn2_on_display_rng),
                           FOUND_FLASH_COUNT);
            seemimic(mtmp);
            /*foundone(zx, zy, mon_to_glyph(mtmp, rn2_on_display_rng);*/
            found_p->num_mons++;
        } else if (mtmp->mundetected && (is_hider(mtmp->data)
                                         || hides_under(mtmp->data)
                                         || mtmp->data->mlet == S_EEL)) {
            flash_glyph_at(zx, zy, mon_to_glyph(mtmp, rn2_on_display_rng),
                           FOUND_FLASH_COUNT);
            mtmp->mundetected = 0;
            /*foundone(zx, zy, mon_to_glyph(mtmp, rn2_on_display_rng);*/
            newsym(zx, zy);
            found_p->num_mons++;
        }
        if (!glyph_is_invisible(lev->glyph)) {
            if (!canspotmon(mtmp)) {
                flash_glyph_at(zx, zy, GLYPH_INVISIBLE, FOUND_FLASH_COUNT);
                map_invisible(zx, zy);
                found_p->num_invis++;
            }
        } else {
            found_p->num_kept_invis++;
        }
    } else if (unmap_invisible(zx, zy)) {
        /* flash the invisible monster glyph because it is already gone */
        flash_glyph_at(zx, zy, GLYPH_INVISIBLE, FOUND_FLASH_COUNT);
        found_p->num_cleared_invis++;
    }
}

staticfn void
openone(coordxy zx, coordxy zy, genericptr_t num)
{
    struct trap *ttmp;
    struct obj *otmp;
    int *num_p = (int *) num;

    if (OBJ_AT(zx, zy)) {
        for (otmp = svl.level.objects[zx][zy]; otmp; otmp = otmp->nexthere) {
            if (Is_box(otmp) && otmp->olocked) {
                otmp->olocked = 0;
                (*num_p)++;
            }
        }
        /* let it fall to the next cases. could be on trap. */
    }
    /* note: secret doors can't be trapped; they use levl[][].wall_info;
       see rm.h for the troublesome overlay of doormask and wall_info */
    if (levl[zx][zy].typ == SDOOR
        || (levl[zx][zy].typ == DOOR
            && (levl[zx][zy].doormask & (D_CLOSED | D_LOCKED)))) {
        if (levl[zx][zy].typ == SDOOR)
            cvt_sdoor_to_door(&levl[zx][zy]); /* .typ = DOOR */
        if (levl[zx][zy].doormask & D_TRAPPED) {
            if (distu(zx, zy) < 3)
                b_trapped("door", NO_PART);
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
        mon = u_at(zx, zy) ? &gy.youmonst : m_at(zx, zy);
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
findit(void)
{
    int num = 0, k;
    char buf[BUFSZ];
    struct found_things found;

    /*
     *  findit() -> do_clear_area(findone) -> findone() -> foundone()
     *  is used to notify player where various things have been found.
     *  Changing FOUND_FLASH_COUNT to 0 will switch to tmp_at() to
     *  highlight all discoveries for the current operation, but requires
     *  player to respond to --More-- when done.  Neither allows browsing
     *  the map via getpos() autodescribe (until after it has reverted to
     *  normal display, where found traps might be covered by objects).
     */

    if (u.uswallow)
        return 0;

#if FOUND_FLASH_COUNT == 0 /* _COUNT > 0 doesn't need to init tmp_at() */
    tmp_at(DISP_ALL, GLYPH_NOTHING);
#endif
    (void) memset((genericptr_t) &found, 0, sizeof found);
    do_clear_area(u.ux, u.uy, BOLT_LIM, findone, (genericptr_t) &found);
    /* count that controls "reveal" punctuation; 0..4 */
    k = !!found.num_sdoors + !!found.num_scorrs + !!found.num_traps
        + !!found.num_mons;

    buf[0] = '\0';
    if (found.num_sdoors) {
        if (found.num_sdoors > 1)
            Sprintf(eos(buf), "%d secret doors", found.num_sdoors);
        else
            Strcat(buf, "a secret door");
        num += found.num_sdoors;
    }
    /* note: non-\0 *buf implies that at least one previous type is present */
    if (found.num_scorrs) {
        if (*buf) /* "doors and corrs" or "doors, corrs ..." */
            Strcat(buf, (k == 2) ? " and " : ", ");
        if (found.num_scorrs > 1)
            Sprintf(eos(buf), "%d secret corridors", found.num_scorrs);
        else
            Strcat(buf, "a secret corridor");
        num += found.num_scorrs;
    }
    if (found.num_traps) {
        if (*buf) /* "doors, corrs, and traps" or "{doors|corrs} and traps"
                   * or "..., traps ..." */
            Strcat(buf, (k == 3 && !found.num_mons) ? ", and "
                        : (k == 2) ? " and " : ", ");
        if (found.num_traps > 1)
            Sprintf(eos(buf), "%d traps", found.num_traps);
        else
            Strcat(buf, "a trap");
        num += found.num_traps;
    }

#if FOUND_FLASH_COUNT == 0
    int tmp_num;
    tmp_num = num; /* sdoors, scorrs, and traps call tmp_at() */
#endif

    if (found.num_mons) {
        if (*buf)
            Strcat(buf, (k > 2) ? ", and " : " and ");
        if (found.num_mons > 1)
            Sprintf(eos(buf), "%d hidden monsters", found.num_mons);
        else
            Strcat(buf, "a hidden monster");
        num += found.num_mons;
    }
    if (*buf)
        You("reveal %s!", buf);

    if (found.num_invis) {
        if (found.num_invis > 1)
            Sprintf(buf, "%d%s unseen monsters", found.num_invis,
                    found.num_kept_invis ? " other" : "");
        else
            Sprintf(buf, "%s unseen monster",
                    found.num_kept_invis ? "another" : "an");
        You("detect %s!", buf);
        num += found.num_invis;
    }

    if (found.num_cleared_invis) {
        /* at least 1 "remembered, unseen monster" marker has been removed */
        if (!num)
            You_feel("%sless paranoid.",
                     found.num_kept_invis ? "somewhat " : "");
        num += found.num_cleared_invis;
    }
    /* note: num_kept_invis is not included in the final result */

    if (!num)
        You("don't find anything.");
#if FOUND_FLASH_COUNT == 0
    else if (tmp_num) {
        flush_screen(1);
        display_nhwindow(WIN_MAP, TRUE);
    }
    tmp_at(DISP_END, GLYPH_NOTHING); /* note: outside of 'if (tmp_num) { }' */
#endif

    return num;
}

/* returns number of things found and opened */
int
openit(void)
{
    int num = 0;

    if (u.uswallow) {
        if (digests(u.ustuck->data)) {
            /* purple worm */
            if (Blind)
                pline("Its mouth opens!");
            else
                pline("%s opens its mouth!", Monnam(u.ustuck));
#if 0   /* expels() will take care of this */
        } else if (enfolds(u.ustuck->data)) {
            /* trapper or lurker above */
            pline("%s unfolds!", Monnam(u.ustuck));
#endif
        }
        expels(u.ustuck, u.ustuck->data, TRUE);
        return -1;
    }

    do_clear_area(u.ux, u.uy, BOLT_LIM, openone, (genericptr_t) &num);
    return num;
}

/* callback hack for overriding vision in do_clear_area() */
boolean
detecting(void (*func)(coordxy, coordxy, genericptr_t))
{
    return (func == findone || func == openone);
}

void
find_trap(struct trap *trap)
{
    boolean cleared = FALSE;

    trap->tseen = 1;
    exercise(A_WIS, TRUE);
    feel_newsym(trap->tx, trap->ty);

    /* The "Hallucination ||" is to preserve 3.6.1 behavior, but this
       behavior might need a rework in the hallucination case
       (e.g. to not prompt if any trap glyph appears on the square). */
    if (Hallucination ||
        levl[trap->tx][trap->ty].glyph != trap_to_glyph(trap)) {
        /* There's too much clutter to see your find otherwise */
        cls();
        map_trap(trap, 1);
        display_self();
        cleared = TRUE;
    }

    set_msg_xy(trap->tx, trap->ty);
    You("find %s.", an(trapname(trap->ttyp, FALSE)));

    if (cleared) {
        display_nhwindow(WIN_MAP, TRUE); /* wait */
        docrt();
    }
}

staticfn int
mfind0(struct monst *mtmp, boolean via_warning)
{
    coordxy x = mtmp->mx, y = mtmp->my;
    boolean found_something = FALSE;

    if (via_warning && !warning_of(mtmp))
        return -1;

    if (M_AP_TYPE(mtmp)) {
        seemimic(mtmp);
        found_something = TRUE;
    } else {
        /* this used to only be executed if a !canspotmon() test passed
           but that failed to bring sensed monsters out of hiding */
        found_something = !canspotmon(mtmp);
        if (mtmp->mundetected && (is_hider(mtmp->data)
                                  || hides_under(mtmp->data)
                                  || mtmp->data->mlet == S_EEL)) {
            if (via_warning && found_something) {
                set_msg_xy(x, y);
                Your("danger sense causes you to take a second %s.",
                     Blind ? "to check nearby" : "look close by");
                display_nhwindow(WIN_MESSAGE, FALSE); /* flush messages */
            }
            mtmp->mundetected = 0;
            found_something = TRUE;
        }
        newsym(x, y);
    }

    if (found_something) {
        if (!canspotmon(mtmp) && glyph_is_invisible(levl[x][y].glyph))
            return -1; /* Found invisible monster in square which already has
                        * 'I' in it.  Logically, this should still take time
                        * and lead to `return 1', but if we did that the hero
                        * would keep finding the same monster every turn. */
        exercise(A_WIS, TRUE);
        if (!canspotmon(mtmp)) {
            map_invisible(x, y);
            set_msg_xy(x, y);
            You_feel("an unseen monster!");
        } else if (!sensemon(mtmp)) {
            set_msg_xy(x, y);
            You("find %s.", mtmp->mtame ? y_monnam(mtmp) : a_monnam(mtmp));
        }
        return 1;
    }
    return 0;
}

int
dosearch0(int aflag) /* intrinsic autosearch vs explicit searching */
{
    coordxy x, y;
    struct trap *trap;
    struct monst *mtmp;

    if (u.uswallow) {
        if (!aflag)
            Norep("What are you looking for?  The exit?");
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
                if (u_at(x, y))
                    continue;

                if (!aflag && (Blind || visible_region_at(x, y)))
                    feel_location(x, y);
                if (levl[x][y].typ == SDOOR) {
                    if (rnl(7 - fund))
                        continue;
                    cvt_sdoor_to_door(&levl[x][y]); /* .typ = DOOR */
                    exercise(A_WIS, TRUE);
                    nomul(0);
                    feel_location(x, y); /* make sure it shows up */
                    set_msg_xy(x, y);
                    You("find a hidden door.");
                } else if (levl[x][y].typ == SCORR) {
                    if (rnl(7 - fund))
                        continue;
                    levl[x][y].typ = CORR;
                    unblock_point(x, y); /* vision */
                    exercise(A_WIS, TRUE);
                    nomul(0);
                    feel_newsym(x, y); /* make sure it shows up */
                    set_msg_xy(x, y);
                    You("find a hidden passage.");
                } else {
                    /* Be careful not to find anything in an SCORR or SDOOR */
                    if ((mtmp = m_at(x, y)) != 0 && !aflag) {
                        int mfres = mfind0(mtmp, 0);

                        if (mfres == -1)
                            continue;
                        else if (mfres > 0)
                            return mfres;
                    }

                    /* see if an invisible monster has moved--if Blind,
                     * feel_location() already did it
                     */
                    if (!aflag && !mtmp && !Blind)
                        (void) unmap_invisible(x, y);

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

/* the #search command -- explicit searching */
int
dosearch(void)
{
    if (cmd_safety_prevention("Searching", "another search",
                          "You already found a monster.",
                          &ga.already_found_flag))
        return ECMD_OK;
    return dosearch0(0) ? ECMD_TIME : ECMD_OK;
}

void
warnreveal(void)
{
    coordxy x, y;
    struct monst *mtmp;

    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++) {
            if (!isok(x, y) || u_at(x, y))
                continue;
            if ((mtmp = m_at(x, y)) != 0
                && warning_of(mtmp) && mtmp->mundetected)
                (void) mfind0(mtmp, 1); /* via_warning */
        }
}

/* Pre-map (the sokoban) levels */
void
premap_detect(void)
{
    coordxy x, y;
    struct trap *ttmp;
    struct obj *obj;

    /* Map the background and boulders */
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            levl[x][y].seenv = SVALL;
            levl[x][y].waslit = TRUE;
            if (levl[x][y].typ == SDOOR)
                levl[x][y].wall_info = 0; /* see rm.h for explanation */
            map_background(x, y, 1);
            if ((obj = sobj_at(BOULDER, x, y)) != 0)
                map_object(obj, 1);
        }

    /* Map the traps */
    for (ttmp = gf.ftrap; ttmp; ttmp = ttmp->ntrap) {
        ttmp->tseen = 1;
        map_trap(ttmp, 1);
    }
}

/* used to see under visible gas/cloud regions; caller must declare cmaptmp */
#define glyph_is_gascloud(glyph) \
    (glyph_is_cmap(glyph) && ((cmaptmp = glyph_to_cmap(glyph)) == S_cloud \
                              || cmaptmp == S_poisoncloud))

staticfn int
reveal_terrain_getglyph(
    coordxy x, coordxy y,
    unsigned swallowed,
    int default_glyph,
    unsigned which_subset)
{
    struct trap *t;
    struct monst *mtmp;
    int glyph, levl_glyph;
    uchar seenv;
    boolean keep_traps = (which_subset & TER_TRP) != 0,
            keep_objs = (which_subset & TER_OBJ) != 0,
            keep_mons = (which_subset & TER_MON) != 0,
            full = (which_subset & TER_FULL) != 0;

    /* for 'full', show the actual terrain for the entire level,
       otherwise what the hero remembers for seen locations with
       monsters, objects, and/or traps removed as caller dictates */
    seenv = (full || svl.level.flags.hero_memory)
              ? levl[x][y].seenv : cansee(x, y) ? SVALL : 0;
    if (full) {
        levl[x][y].seenv = SVALL;
        glyph = back_to_glyph(x, y);
        levl[x][y].seenv = seenv;
    } else {
        int cmaptmp = 0; /* used by glyph_is_gascloud() macro */
        NhRegion *reg = visible_region_at(x, y);
        boolean was_mon = FALSE;

        levl_glyph = svl.level.flags.hero_memory ? levl[x][y].glyph
                     : seenv ? back_to_glyph(x, y)
                       : default_glyph;
        /* glyph_at() returns the displayed glyph, which might
           be a monster.  levl[][].glyph contains the remembered
           glyph, which will never be a monster (unless it is
           the invisible monster glyph, which is handled like
           an object, replacing any object or trap at its spot) */
        glyph = !swallowed ? glyph_at(x, y) : levl_glyph;
        if (keep_mons && u_at(x, y) && swallowed) {
            glyph = mon_to_glyph(u.ustuck, rn2_on_display_rng);
        } else if ((!keep_mons && (glyph_is_monster(glyph)
                                 || glyph_is_warning(glyph)))
                   || glyph_is_swallow(glyph)) {
            glyph = levl_glyph;
            was_mon = TRUE;
        }
        if (((!keep_objs && glyph_is_object(glyph))
             || glyph_is_invisible(glyph))
            && keep_traps && !covers_traps(x, y)) {
            if ((t = t_at(x, y)) != 0 && t->tseen)
                glyph = trap_to_glyph(t);
        }
        if ((!keep_objs && glyph_is_object(glyph))
            /* we either show both traps and visible regions (trap if both
               are present at the same spot) or neither traps nor regions */
            || (!keep_traps && (glyph_is_trap(glyph)
                                || (reg && glyph_is_gascloud(glyph))))
            || (reg && was_mon)
            || glyph_is_invisible(glyph)) {
            if (!seenv) {
                /* it's possible to have a visible region shown at an
                   otherwise unexplored location (cast stinking cloud
                   through unexplored corridor into lit room, then approach
                   far enough to be adjacent to the cloud without having
                   seen the corridor underneath it) */
                glyph = !reg ? default_glyph : GLYPH_UNEXPLORED;
            } else if (keep_traps && reg
                       && (glyph_is_gascloud(glyph) || was_mon)) {
                t = t_at(x, y);
                /* we need reg->glyph here when there's a monster shown
                   at a region spot; the region glyph isn't the remembered
                   background glyph or the current glyph */
                glyph = (t && t->tseen) ? trap_to_glyph(t) : reg->glyph;
                /* FIXME? what about objects temporarily hidden by regions?
                   when objects are being shown, shouldn't showing them take
                   precedence over showing the region, just like traps? */
            } else if (svl.lastseentyp[x][y] == levl[x][y].typ) {
                glyph = back_to_glyph(x, y);
            } else {
                /* look for a mimic here posing as furniture;
                   if we don't find one, we'll have to fake it */
                if ((mtmp = m_at(x, y)) != 0
                    && M_AP_TYPE(mtmp) == M_AP_FURNITURE) {
                    glyph = cmap_to_glyph(mtmp->mappearance);
                } else {
                    struct rm save_spot;

                    /*
                     * We have a topology type but we want a screen symbol
                     * in order to derive a glyph.  Some screen symbols need
                     * the flags field of levl[][] in addition to the type
                     * (to disambiguate STAIRS to S_upstair or S_dnstair,
                     * for example).  Current flags might not be intended
                     * for remembered type, but we've got no other choice.
                     * An exception is wall_info which can be recalculated and
                     * needs to be.  Otherwise back_to_glyph() -> wall_angle()
                     * might issue an impossible() for it if it is currently
                     * doormask==D_OPEN for an open door remembered as a wall.
                     */
                    save_spot = levl[x][y];
                    levl[x][y].typ = svl.lastseentyp[x][y];
                    if (IS_WALL(levl[x][y].typ) || levl[x][y].typ == SDOOR)
                        xy_set_wall_state(x, y); /* levl[x][y].wall_info */
                    glyph = back_to_glyph(x, y);
                    levl[x][y] = save_spot;
                }
            }
        }
    }
    /* FIXME: dirty hack */
    if (glyph == cmap_to_glyph(S_darkroom))
        glyph = cmap_to_glyph(S_room);
    else if (glyph == cmap_to_glyph(S_litcorr))
        glyph = cmap_to_glyph(S_corr);
    return glyph;
}

#undef glyph_is_gascloud

#ifdef DUMPLOG
void
dump_map(void)
{
    char buf[COLBUFSZ];
    coordxy x, y;
    int glyph, skippedrows, lastnonblank;
    boolean blankrow, toprow;
    unsigned subset = TER_MAP | TER_TRP | TER_OBJ | TER_MON;
    /* cmap_to_glyph() evaluates its argument multiple times, so pull the
       tree vs stone conditional out of it */
    nhsym default_sym = svl.level.flags.arboreal ? S_tree : S_stone;
    int default_glyph = cmap_to_glyph(default_sym);

    /*
     * Squeeze out excess vertical space when dumping the map.
     * If there are any blank map rows at the top, suppress them
     * (our caller has already printed a separator).  If there is
     * more than one blank map row at the bottom, keep just one.
     * Any blank rows within the middle of the map are kept.
     * Note: putstr() with winid==0 is for dumplog.
     */
    skippedrows = 0;
    toprow = TRUE;
    for (y = 0; y < ROWNO; y++) {
        blankrow = TRUE; /* assume blank until we discover otherwise */
        lastnonblank = -1; /* buf[] index rather than map's x */
        for (x = 1; x < COLNO; x++) {
            int ch;
            glyph_info glyphinfo;

            glyph = reveal_terrain_getglyph(x, y, u.uswallow,
                                            default_glyph, subset);
            map_glyphinfo(x, y, glyph, 0, &glyphinfo);
            ch = glyphinfo.ttychar;
            buf[x - 1] = ch;
            if (ch != ' ') {
                blankrow = FALSE;
                lastnonblank = x - 1;
            }
        }
        if (!blankrow) {
            buf[lastnonblank + 1] = '\0';
            if (toprow) {
                skippedrows = 0;
                toprow = FALSE;
            }
            for (x = 0; x < skippedrows; x++)
                putstr(0, 0, "");
            putstr(0, 0, buf); /* map row #y */
            skippedrows = 0;
        } else {
            ++skippedrows;
        }
    }
    if (skippedrows)
        putstr(0, 0, "");
}
#endif /* DUMPLOG */

/* idea from crawl; show known portion of map without any monsters,
   objects, or traps occluding the view of the underlying terrain;
   in explore or wizard modes, can also display unexplored portion */
void
reveal_terrain(
    unsigned which_subset) /* TER_TRP | TER_OBJ | TER_MON | TER_FULL */
{
    /* 'full' overrides impairment and implies no-traps, no-objs, no-mons */
    boolean full = (which_subset & TER_FULL) != 0; /* show whole map */

    if ((Hallucination || Stunned || Confusion) && !full) {
        You("are too disoriented for this.");
    } else {
        coordxy x, y;
        int glyph, default_glyph;
        char buf[BUFSZ];
        /* there is a TER_MAP bit too; we always show map regardless of it */
        boolean keep_traps = (which_subset & TER_TRP) != 0,
                keep_objs = (which_subset & TER_OBJ) != 0,
                keep_mons = (which_subset & TER_MON) != 0; /* not used */
        unsigned swallowed = u.uswallow; /* before unconstrain_map() */

        if (unconstrain_map())
            docrt();
        default_glyph = cmap_to_glyph(svl.level.flags.arboreal ? S_tree
                                                             : S_stone);

        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++) {
                glyph = reveal_terrain_getglyph(x, y, swallowed,
                                                default_glyph, which_subset);
                show_glyph(x, y, glyph);
            }

        /* hero's location is not highlighted, but getpos() starts with
           cursor there, and after moving it anywhere '@' moves it back */
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

        /* allow player to move cursor around and get autodescribe feedback
           based on what is visible now rather than what is on 'real' map */
        which_subset |= TER_MAP; /* guarantee non-zero */
        browse_map(which_subset, "anything of interest");

        map_redisplay();
    }
    return;
}

#undef FOUND_FLASH_COUNT

/*detect.c*/

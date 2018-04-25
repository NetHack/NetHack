/* NetHack 3.6	invent.c	$NHDT-Date: 1519672703 2018/02/26 19:18:23 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.225 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define NOINVSYM '#'
#define CONTAINED_SYM '>' /* designator for inside a container */
#define HANDS_SYM '-'

STATIC_DCL int FDECL(CFDECLSPEC sortloot_cmp, (const genericptr,
                                               const genericptr));
STATIC_DCL void NDECL(reorder_invent);
STATIC_DCL void FDECL(noarmor, (BOOLEAN_P));
STATIC_DCL void FDECL(invdisp_nothing, (const char *, const char *));
STATIC_DCL boolean FDECL(worn_wield_only, (struct obj *));
STATIC_DCL boolean FDECL(only_here, (struct obj *));
STATIC_DCL void FDECL(compactify, (char *));
STATIC_DCL boolean FDECL(taking_off, (const char *));
STATIC_DCL boolean FDECL(putting_on, (const char *));
STATIC_PTR int FDECL(ckunpaid, (struct obj *));
STATIC_PTR int FDECL(ckvalidcat, (struct obj *));
STATIC_PTR char *FDECL(safeq_xprname, (struct obj *));
STATIC_PTR char *FDECL(safeq_shortxprname, (struct obj *));
STATIC_DCL char FDECL(display_pickinv, (const char *, const char *,
                                        const char *, BOOLEAN_P, long *));
STATIC_DCL char FDECL(display_used_invlets, (CHAR_P));
STATIC_DCL boolean FDECL(this_type_only, (struct obj *));
STATIC_DCL void NDECL(dounpaid);
STATIC_DCL struct obj *FDECL(find_unpaid, (struct obj *, struct obj **));
STATIC_DCL void FDECL(menu_identify, (int));
STATIC_DCL boolean FDECL(tool_in_use, (struct obj *));
STATIC_DCL char FDECL(obj_to_let, (struct obj *));

static int lastinvnr = 51; /* 0 ... 51 (never saved&restored) */

/* wizards can wish for venom, which will become an invisible inventory
 * item without this.  putting it in inv_order would mean venom would
 * suddenly become a choice for all the inventory-class commands, which
 * would probably cause mass confusion.  the test for inventory venom
 * is only WIZARD and not wizard because the wizard can leave venom lying
 * around on a bones level for normal players to find.  [Note to the
 * confused:  'WIZARD' used to be a compile-time conditional so this was
 * guarded by #ifdef WIZARD/.../#endif.]
 */
static char venom_inv[] = { VENOM_CLASS, 0 }; /* (constant) */

struct sortloot_item {
    struct obj *obj;
    int indx;
};
unsigned sortlootmode = 0;

/* qsort comparison routine for sortloot() */
STATIC_OVL int CFDECLSPEC
sortloot_cmp(vptr1, vptr2)
const genericptr vptr1;
const genericptr vptr2;
{
    struct sortloot_item *sli1 = (struct sortloot_item *) vptr1,
                         *sli2 = (struct sortloot_item *) vptr2;
    struct obj *obj1 = sli1->obj,
               *obj2 = sli2->obj,
               sav1, sav2;
    char *cls1, *cls2, nam1[BUFSZ], nam2[BUFSZ];
    int val1, val2, c, namcmp;

    /* order by object class like inventory display */
    if ((sortlootmode & SORTLOOT_PACK) != 0) {
        cls1 = index(flags.inv_order, obj1->oclass);
        cls2 = index(flags.inv_order, obj2->oclass);
        if (cls1 != cls2)
            return (int) (cls1 - cls2);

        if ((sortlootmode & SORTLOOT_INVLET) != 0) {
            ; /* skip sub-classes when sorting by packorder+invlet */

        /* for armor, group by sub-category */
        } else if (obj1->oclass == ARMOR_CLASS) {
            static int armcat[7 + 1];

            if (!armcat[7]) {
                /* one-time init; we want to control the order */
                armcat[ARM_HELM]   = 1; /* [2] */
                armcat[ARM_GLOVES] = 2; /* [3] */
                armcat[ARM_BOOTS]  = 3; /* [4] */
                armcat[ARM_SHIELD] = 4; /* [1] */
                armcat[ARM_CLOAK]  = 5; /* [5] */
                armcat[ARM_SHIRT]  = 6; /* [6] */
                armcat[ARM_SUIT]   = 7; /* [0] */
                armcat[7]          = 8;
            }
            val1 = armcat[objects[obj1->otyp].oc_armcat];
            val2 = armcat[objects[obj2->otyp].oc_armcat];
            if (val1 != val2)
                return val1 - val2;

        /* for weapons, group by ammo (arrows, bolts), launcher (bows),
           missile (dart, boomerang), stackable (daggers, knives, spears),
           'other' (swords, axes, &c), polearm */
        } else if (obj1->oclass == WEAPON_CLASS) {
            val1 = objects[obj1->otyp].oc_skill;
            val1 = (val1 < 0)
                    ? (val1 >= -P_CROSSBOW && val1 <= -P_BOW) ? 1 : 3
                    : (val1 >= P_BOW && val1 <= P_CROSSBOW) ? 2
                       : (val1 == P_SPEAR || val1 == P_DAGGER
                          || val1 == P_KNIFE) ? 4 : !is_pole(obj1) ? 5 : 6;
            val2 = objects[obj2->otyp].oc_skill;
            val2 = (val2 < 0)
                    ? (val2 >= -P_CROSSBOW && val2 <= -P_BOW) ? 1 : 3
                    : (val2 >= P_BOW && val2 <= P_CROSSBOW) ? 2
                       : (val2 == P_SPEAR || val2 == P_DAGGER
                          || val2 == P_KNIFE) ? 4 : !is_pole(obj2) ? 5 : 6;
            if (val1 != val2)
                return val1 - val2;
        }
    }

    /* order by assigned inventory letter */
    if ((sortlootmode & SORTLOOT_INVLET) != 0) {
        c = obj1->invlet;
        val1 = ('a' <= c && c <= 'z') ? (c - 'a' + 2)
               : ('A' <= c && c <= 'Z') ? (c - 'A' + 2 + 26)
                 : (c == '$') ? 1
                   : (c == '#') ? 1 + 52 + 1
                     : 1 + 52 + 1 + 1; /* none of the above */
        c = obj2->invlet;
        val2 = ('a' <= c && c <= 'z') ? (c - 'a' + 2)
               : ('A' <= c && c <= 'Z') ? (c - 'A' + 2 + 26)
                 : (c == '$') ? 1
                   : (c == '#') ? 1 + 52 + 1
                     : 1 + 52 + 1 + 1; /* none of the above */
        if (val1 != val2)
            return val1 - val2;
    }

    if ((sortlootmode & SORTLOOT_LOOT) == 0)
        goto tiebreak;

    /*
     * Sort object names in lexicographical order, ignoring quantity.
     */
    /* Force diluted potions to come out after undiluted of same type;
       obj->odiluted overloads obj->oeroded. */
    sav1.odiluted = obj1->odiluted;
    sav2.odiluted = obj2->odiluted;
    if (obj1->oclass == POTION_CLASS)
        obj1->odiluted = 0;
    if (obj1->oclass == POTION_CLASS)
        obj2->odiluted = 0;
    /* Force holy and unholy water to sort adjacent to water rather
       than among 'h's and 'u's.  BUCX order will keep them distinct. */
    Strcpy(nam1, cxname_singular(obj1));
    if (obj1->otyp == POT_WATER && obj1->bknown
        && (obj1->blessed || obj1->cursed))
        (void) strsubst(nam1, obj1->blessed ? "holy " : "unholy ", "");
    Strcpy(nam2, cxname_singular(obj2));
    if (obj2->otyp == POT_WATER && obj2->bknown
        && (obj2->blessed || obj2->cursed))
        (void) strsubst(nam2, obj2->blessed ? "holy " : "unholy ", "");
    obj1->odiluted = sav1.odiluted;
    obj2->odiluted = sav2.odiluted;

    if ((namcmp = strcmpi(nam1, nam2)) != 0)
        return namcmp;

    /* Sort by BUCX. */
    val1 = obj1->bknown ? (obj1->blessed ? 3 : !obj1->cursed ? 2 : 1) : 0;
    val2 = obj2->bknown ? (obj2->blessed ? 3 : !obj2->cursed ? 2 : 1) : 0;
    if (val1 != val2)
        return val2 - val1; /* bigger is better */

    /* Sort by greasing.  This will put the objects in degreasing order. */
    val1 = obj1->greased;
    val2 = obj2->greased;
    if (val1 != val2)
        return val2 - val1; /* bigger is better */

    /* Sort by erosion.  The effective amount is what matters. */
    val1 = greatest_erosion(obj1);
    val2 = greatest_erosion(obj2);
    if (val1 != val2)
        return val1 - val2; /* bigger is WORSE */

    /* Sort by erodeproofing.  Map known-invulnerable to 1, and both
       known-vulnerable and unknown-vulnerability to 0, because that's
       how they're displayed. */
    val1 = obj1->rknown && obj1->oerodeproof;
    val2 = obj2->rknown && obj2->oerodeproof;
    if (val1 != val2)
        return val2 - val1; /* bigger is better */

    /* Sort by enchantment.  Map unknown to -1000, which is comfortably
       below the range of obj->spe.  oc_uses_known means that obj->known
       matters, which usually indirectly means that obj->spe is relevant.
       Lots of objects use obj->spe for some other purpose (see obj.h). */
    if (objects[obj1->otyp].oc_uses_known
        /* exclude eggs (laid by you) and tins (homemade, pureed, &c) */
        && obj1->oclass != FOOD_CLASS) {
        val1 = obj1->known ? obj1->spe : -1000;
        val2 = obj2->known ? obj2->spe : -1000;
        if (val1 != val2)
            return val2 - val1; /* bigger is better */
    }

tiebreak:
    /* They're identical, as far as we're concerned.  We want
       to force a deterministic order, and do so by producing a
       stable sort: maintain the original order of equal items. */
    return (sli1->indx - sli2->indx);
}

void
sortloot(olist, mode, by_nexthere)
struct obj **olist;
unsigned mode; /* flags for sortloot_cmp() */
boolean by_nexthere; /* T: traverse via obj->nexthere, F: via obj->nobj */
{
    struct sortloot_item *sliarray, osli, nsli;
    struct obj *o, **nxt_p;
    unsigned n, i;
    boolean already_sorted = TRUE;

    sortlootmode = mode; /* extra input for sortloot_cmp() */
    for (n = osli.indx = 0, osli.obj = *olist; (o = osli.obj) != 0;
         osli = nsli) {
        nsli.obj = by_nexthere ? o->nexthere : o->nobj;
        nsli.indx = (int) ++n;
        if (nsli.obj && already_sorted
            && sortloot_cmp((genericptr_t) &osli, (genericptr_t) &nsli) > 0)
            already_sorted = FALSE;
    }
    if (n > 1 && !already_sorted) {
        sliarray = (struct sortloot_item *) alloc(n * sizeof *sliarray);
        for (i = 0, o = *olist; o;
             ++i, o = by_nexthere ? o->nexthere : o->nobj)
            sliarray[i].obj = o, sliarray[i].indx = (int) i;

        qsort((genericptr_t) sliarray, n, sizeof *sliarray, sortloot_cmp);
        for (i = 0; i < n; ++i) {
            o = sliarray[i].obj;
            nxt_p = by_nexthere ? &(o->nexthere) : &(o->nobj);
            *nxt_p = (i < n - 1) ? sliarray[i + 1].obj : (struct obj *) 0;
        }
        *olist = sliarray[0].obj;
        free((genericptr_t) sliarray);
    }
    sortlootmode = 0;
}

void
assigninvlet(otmp)
register struct obj *otmp;
{
    boolean inuse[52];
    register int i;
    register struct obj *obj;

    /* there should be at most one of these in inventory... */
    if (otmp->oclass == COIN_CLASS) {
        otmp->invlet = GOLD_SYM;
        return;
    }

    for (i = 0; i < 52; i++)
        inuse[i] = FALSE;
    for (obj = invent; obj; obj = obj->nobj)
        if (obj != otmp) {
            i = obj->invlet;
            if ('a' <= i && i <= 'z')
                inuse[i - 'a'] = TRUE;
            else if ('A' <= i && i <= 'Z')
                inuse[i - 'A' + 26] = TRUE;
            if (i == otmp->invlet)
                otmp->invlet = 0;
        }
    if ((i = otmp->invlet)
        && (('a' <= i && i <= 'z') || ('A' <= i && i <= 'Z')))
        return;
    for (i = lastinvnr + 1; i != lastinvnr; i++) {
        if (i == 52) {
            i = -1;
            continue;
        }
        if (!inuse[i])
            break;
    }
    otmp->invlet =
        (inuse[i] ? NOINVSYM : (i < 26) ? ('a' + i) : ('A' + i - 26));
    lastinvnr = i;
}

/* note: assumes ASCII; toggling a bit puts lowercase in front of uppercase */
#define inv_rank(o) ((o)->invlet ^ 040)

/* sort the inventory; used by addinv() and doorganize() */
STATIC_OVL void
reorder_invent()
{
    struct obj *otmp, *prev, *next;
    boolean need_more_sorting;

    do {
        /*
         * We expect at most one item to be out of order, so this
         * isn't nearly as inefficient as it may first appear.
         */
        need_more_sorting = FALSE;
        for (otmp = invent, prev = 0; otmp;) {
            next = otmp->nobj;
            if (next && inv_rank(next) < inv_rank(otmp)) {
                need_more_sorting = TRUE;
                if (prev)
                    prev->nobj = next;
                else
                    invent = next;
                otmp->nobj = next->nobj;
                next->nobj = otmp;
                prev = next;
            } else {
                prev = otmp;
                otmp = next;
            }
        }
    } while (need_more_sorting);
}

#undef inv_rank

/* scan a list of objects to see whether another object will merge with
   one of them; used in pickup.c when all 52 inventory slots are in use,
   to figure out whether another object could still be picked up */
struct obj *
merge_choice(objlist, obj)
struct obj *objlist, *obj;
{
    struct monst *shkp;
    int save_nocharge;

    if (obj->otyp == SCR_SCARE_MONSTER) /* punt on these */
        return (struct obj *) 0;
    /* if this is an item on the shop floor, the attributes it will
       have when carried are different from what they are now; prevent
       that from eliciting an incorrect result from mergable() */
    save_nocharge = obj->no_charge;
    if (objlist == invent && obj->where == OBJ_FLOOR
        && (shkp = shop_keeper(inside_shop(obj->ox, obj->oy))) != 0) {
        if (obj->no_charge)
            obj->no_charge = 0;
        /* A billable object won't have its `unpaid' bit set, so would
           erroneously seem to be a candidate to merge with a similar
           ordinary object.  That's no good, because once it's really
           picked up, it won't merge after all.  It might merge with
           another unpaid object, but we can't check that here (depends
           too much upon shk's bill) and if it doesn't merge it would
           end up in the '#' overflow inventory slot, so reject it now. */
        else if (inhishop(shkp))
            return (struct obj *) 0;
    }
    while (objlist) {
        if (mergable(objlist, obj))
            break;
        objlist = objlist->nobj;
    }
    obj->no_charge = save_nocharge;
    return objlist;
}

/* merge obj with otmp and delete obj if types agree */
int
merged(potmp, pobj)
struct obj **potmp, **pobj;
{
    register struct obj *otmp = *potmp, *obj = *pobj;

    if (mergable(otmp, obj)) {
        /* Approximate age: we do it this way because if we were to
         * do it "accurately" (merge only when ages are identical)
         * we'd wind up never merging any corpses.
         * otmp->age = otmp->age*(1-proportion) + obj->age*proportion;
         *
         * Don't do the age manipulation if lit.  We would need
         * to stop the burn on both items, then merge the age,
         * then restart the burn.  Glob ages are averaged in the
         * absorb routine, which uses weight rather than quantity
         * to adjust for proportion (glob quantity is always 1).
         */
        if (!obj->lamplit && !obj->globby)
            otmp->age = ((otmp->age * otmp->quan) + (obj->age * obj->quan))
                        / (otmp->quan + obj->quan);

        otmp->quan += obj->quan;
        /* temporary special case for gold objects!!!! */
        if (otmp->oclass == COIN_CLASS)
            otmp->owt = weight(otmp), otmp->bknown = 0;
        /* and puddings!!!1!!one! */
        else if (!Is_pudding(otmp))
            otmp->owt += obj->owt;
        if (!has_oname(otmp) && has_oname(obj))
            otmp = *potmp = oname(otmp, ONAME(obj));
        obj_extract_self(obj);

        /* really should merge the timeouts */
        if (obj->lamplit)
            obj_merge_light_sources(obj, otmp);
        if (obj->timed)
            obj_stop_timers(obj); /* follows lights */

        /* fixup for `#adjust' merging wielded darts, daggers, &c */
        if (obj->owornmask && carried(otmp)) {
            long wmask = otmp->owornmask | obj->owornmask;

            /* Both the items might be worn in competing slots;
               merger preference (regardless of which is which):
             primary weapon + alternate weapon -> primary weapon;
             primary weapon + quiver -> primary weapon;
             alternate weapon + quiver -> alternate weapon.
               (Prior to 3.3.0, it was not possible for the two
               stacks to be worn in different slots and `obj'
               didn't need to be unworn when merging.) */
            if (wmask & W_WEP)
                wmask = W_WEP;
            else if (wmask & W_SWAPWEP)
                wmask = W_SWAPWEP;
            else if (wmask & W_QUIVER)
                wmask = W_QUIVER;
            else {
                impossible("merging strangely worn items (%lx)", wmask);
                wmask = otmp->owornmask;
            }
            if ((otmp->owornmask & ~wmask) != 0L)
                setnotworn(otmp);
            setworn(otmp, wmask);
            setnotworn(obj);
#if 0
        /* (this should not be necessary, since items
            already in a monster's inventory don't ever get
            merged into other objects [only vice versa]) */
        } else if (obj->owornmask && mcarried(otmp)) {
            if (obj == MON_WEP(otmp->ocarry)) {
                MON_WEP(otmp->ocarry) = otmp;
                otmp->owornmask = W_WEP;
            }
#endif /*0*/
        }

        /* handle puddings a bit differently; absorption will free the
           other object automatically so we can just return out from here */
        if (obj->globby) {
            pudding_merge_message(otmp, obj);
            obj_absorb(potmp, pobj);
            return 1;
        }

        obfree(obj, otmp); /* free(obj), bill->otmp */
        return 1;
    }
    return 0;
}

/*
 * Adjust hero intrinsics as if this object was being added to the hero's
 * inventory.  Called _before_ the object has been added to the hero's
 * inventory.
 *
 * This is called when adding objects to the hero's inventory normally (via
 * addinv) or when an object in the hero's inventory has been polymorphed
 * in-place.
 *
 * It may be valid to merge this code with with addinv_core2().
 */
void
addinv_core1(obj)
struct obj *obj;
{
    if (obj->oclass == COIN_CLASS) {
        context.botl = 1;
    } else if (obj->otyp == AMULET_OF_YENDOR) {
        if (u.uhave.amulet)
            impossible("already have amulet?");
        u.uhave.amulet = 1;
        u.uachieve.amulet = 1;
    } else if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
        if (u.uhave.menorah)
            impossible("already have candelabrum?");
        u.uhave.menorah = 1;
        u.uachieve.menorah = 1;
    } else if (obj->otyp == BELL_OF_OPENING) {
        if (u.uhave.bell)
            impossible("already have silver bell?");
        u.uhave.bell = 1;
        u.uachieve.bell = 1;
    } else if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        if (u.uhave.book)
            impossible("already have the book?");
        u.uhave.book = 1;
        u.uachieve.book = 1;
    } else if (obj->oartifact) {
        if (is_quest_artifact(obj)) {
            if (u.uhave.questart)
                impossible("already have quest artifact?");
            u.uhave.questart = 1;
            artitouch(obj);
        }
        set_artifact_intrinsic(obj, 1, W_ART);
    }

    /* "special achievements" aren't discoverable during play, they
       end up being recorded in XLOGFILE at end of game, nowhere else;
       record_achieve_special overloads corpsenm which is ordinarily
       initialized to NON_PM (-1) rather than to 0; any special prize
       must never be a corpse, egg, tin, figurine, or statue because
       their use of obj->corpsenm for monster type would conflict,
       nor be a leash (corpsenm overloaded for m_id of leashed
       monster) or a novel (corpsenm overloaded for novel index) */
    if (is_mines_prize(obj)) {
        u.uachieve.mines_luckstone = 1;
        obj->record_achieve_special = NON_PM;
    } else if (is_soko_prize(obj)) {
        u.uachieve.finish_sokoban = 1;
        obj->record_achieve_special = NON_PM;
    }
}

/*
 * Adjust hero intrinsics as if this object was being added to the hero's
 * inventory.  Called _after_ the object has been added to the hero's
 * inventory.
 *
 * This is called when adding objects to the hero's inventory normally (via
 * addinv) or when an object in the hero's inventory has been polymorphed
 * in-place.
 */
void
addinv_core2(obj)
struct obj *obj;
{
    if (confers_luck(obj)) {
        /* new luckstone must be in inventory by this point
         * for correct calculation */
        set_moreluck();
    }
}

/*
 * Add obj to the hero's inventory.  Make sure the object is "free".
 * Adjust hero attributes as necessary.
 */
struct obj *
addinv(obj)
struct obj *obj;
{
    struct obj *otmp, *prev;
    int saved_otyp = (int) obj->otyp; /* for panic */
    boolean obj_was_thrown;

    if (obj->where != OBJ_FREE)
        panic("addinv: obj not free");
    /* normally addtobill() clears no_charge when items in a shop are
       picked up, but won't do so if the shop has become untended */
    obj->no_charge = 0; /* should not be set in hero's invent */
    if (Has_contents(obj))
        picked_container(obj); /* clear no_charge */
    obj_was_thrown = obj->was_thrown;
    obj->was_thrown = 0;       /* not meaningful for invent */

    addinv_core1(obj);

    /* merge with quiver in preference to any other inventory slot
       in case quiver and wielded weapon are both eligible; adding
       extra to quivered stack is more useful than to wielded one */
    if (uquiver && merged(&uquiver, &obj)) {
        obj = uquiver;
        if (!obj)
            panic("addinv: null obj after quiver merge otyp=%d", saved_otyp);
        goto added;
    }
    /* merge if possible; find end of chain in the process */
    for (prev = 0, otmp = invent; otmp; prev = otmp, otmp = otmp->nobj)
        if (merged(&otmp, &obj)) {
            obj = otmp;
            if (!obj)
                panic("addinv: null obj after merge otyp=%d", saved_otyp);
            goto added;
        }
    /* didn't merge, so insert into chain */
    assigninvlet(obj);
    if (flags.invlet_constant || !prev) {
        obj->nobj = invent; /* insert at beginning */
        invent = obj;
        if (flags.invlet_constant)
            reorder_invent();
    } else {
        prev->nobj = obj; /* insert at end */
        obj->nobj = 0;
    }
    obj->where = OBJ_INVENT;

    /* fill empty quiver if obj was thrown */
    if (flags.pickup_thrown && !uquiver && obj_was_thrown
        /* if Mjollnir is thrown and fails to return, we want to
           auto-pick it when we move to its spot, but not into quiver;
           aklyses behave like Mjollnir when thrown while wielded, but
           we lack sufficient information here make them exceptions */
        && obj->oartifact != ART_MJOLLNIR
        && (throwing_weapon(obj) || is_ammo(obj)))
        setuqwep(obj);
added:
    addinv_core2(obj);
    carry_obj_effects(obj); /* carrying affects the obj */
    update_inventory();
    return obj;
}

/*
 * Some objects are affected by being carried.
 * Make those adjustments here. Called _after_ the object
 * has been added to the hero's or monster's inventory,
 * and after hero's intrinsics have been updated.
 */
void
carry_obj_effects(obj)
struct obj *obj;
{
    /* Cursed figurines can spontaneously transform when carried. */
    if (obj->otyp == FIGURINE) {
        if (obj->cursed && obj->corpsenm != NON_PM
            && !dead_species(obj->corpsenm, TRUE)) {
            attach_fig_transform_timeout(obj);
        }
    }
}

/* Add an item to the inventory unless we're fumbling or it refuses to be
 * held (via touch_artifact), and give a message.
 * If there aren't any free inventory slots, we'll drop it instead.
 * If both success and failure messages are NULL, then we're just doing the
 * fumbling/slot-limit checking for a silent grab.  In any case,
 * touch_artifact will print its own messages if they are warranted.
 */
struct obj *
hold_another_object(obj, drop_fmt, drop_arg, hold_msg)
struct obj *obj;
const char *drop_fmt, *drop_arg, *hold_msg;
{
    char buf[BUFSZ];

    if (!Blind)
        obj->dknown = 1; /* maximize mergibility */
    if (obj->oartifact) {
        /* place_object may change these */
        boolean crysknife = (obj->otyp == CRYSKNIFE);
        int oerode = obj->oerodeproof;
        boolean wasUpolyd = Upolyd;

        /* in case touching this object turns out to be fatal */
        place_object(obj, u.ux, u.uy);

        if (!touch_artifact(obj, &youmonst)) {
            obj_extract_self(obj); /* remove it from the floor */
            dropy(obj);            /* now put it back again :-) */
            return obj;
        } else if (wasUpolyd && !Upolyd) {
            /* loose your grip if you revert your form */
            if (drop_fmt)
                pline(drop_fmt, drop_arg);
            obj_extract_self(obj);
            dropy(obj);
            return obj;
        }
        obj_extract_self(obj);
        if (crysknife) {
            obj->otyp = CRYSKNIFE;
            obj->oerodeproof = oerode;
        }
    }
    if (Fumbling) {
        if (drop_fmt)
            pline(drop_fmt, drop_arg);
        dropy(obj);
    } else {
        long oquan = obj->quan;
        int prev_encumbr = near_capacity(); /* before addinv() */

        /* encumbrance only matters if it would now become worse
           than max( current_value, stressed ) */
        if (prev_encumbr < MOD_ENCUMBER)
            prev_encumbr = MOD_ENCUMBER;
        /* addinv() may redraw the entire inventory, overwriting
           drop_arg when it comes from something like doname() */
        if (drop_arg)
            drop_arg = strcpy(buf, drop_arg);

        obj = addinv(obj);
        if (inv_cnt(FALSE) > 52 || ((obj->otyp != LOADSTONE || !obj->cursed)
                                    && near_capacity() > prev_encumbr)) {
            if (drop_fmt)
                pline(drop_fmt, drop_arg);
            /* undo any merge which took place */
            if (obj->quan > oquan)
                obj = splitobj(obj, oquan);
            dropx(obj);
        } else {
            if (flags.autoquiver && !uquiver && !obj->owornmask
                && (is_missile(obj) || ammo_and_launcher(obj, uwep)
                    || ammo_and_launcher(obj, uswapwep)))
                setuqwep(obj);
            if (hold_msg || drop_fmt)
                prinv(hold_msg, obj, oquan);
        }
    }
    return obj;
}

/* useup() all of an item regardless of its quantity */
void
useupall(obj)
struct obj *obj;
{
    setnotworn(obj);
    freeinv(obj);
    obfree(obj, (struct obj *) 0); /* deletes contents also */
}

void
useup(obj)
register struct obj *obj;
{
    /* Note:  This works correctly for containers because they (containers)
       don't merge. */
    if (obj->quan > 1L) {
        obj->in_use = FALSE; /* no longer in use */
        obj->quan--;
        obj->owt = weight(obj);
        update_inventory();
    } else {
        useupall(obj);
    }
}

/* use one charge from an item and possibly incur shop debt for it */
void
consume_obj_charge(obj, maybe_unpaid)
struct obj *obj;
boolean maybe_unpaid; /* false if caller handles shop billing */
{
    if (maybe_unpaid)
        check_unpaid(obj);
    obj->spe -= 1;
    if (obj->known)
        update_inventory();
}

/*
 * Adjust hero's attributes as if this object was being removed from the
 * hero's inventory.  This should only be called from freeinv() and
 * where we are polymorphing an object already in the hero's inventory.
 *
 * Should think of a better name...
 */
void
freeinv_core(obj)
struct obj *obj;
{
    if (obj->oclass == COIN_CLASS) {
        context.botl = 1;
        return;
    } else if (obj->otyp == AMULET_OF_YENDOR) {
        if (!u.uhave.amulet)
            impossible("don't have amulet?");
        u.uhave.amulet = 0;
    } else if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
        if (!u.uhave.menorah)
            impossible("don't have candelabrum?");
        u.uhave.menorah = 0;
    } else if (obj->otyp == BELL_OF_OPENING) {
        if (!u.uhave.bell)
            impossible("don't have silver bell?");
        u.uhave.bell = 0;
    } else if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        if (!u.uhave.book)
            impossible("don't have the book?");
        u.uhave.book = 0;
    } else if (obj->oartifact) {
        if (is_quest_artifact(obj)) {
            if (!u.uhave.questart)
                impossible("don't have quest artifact?");
            u.uhave.questart = 0;
        }
        set_artifact_intrinsic(obj, 0, W_ART);
    }

    if (obj->otyp == LOADSTONE) {
        curse(obj);
    } else if (confers_luck(obj)) {
        set_moreluck();
        context.botl = 1;
    } else if (obj->otyp == FIGURINE && obj->timed) {
        (void) stop_timer(FIG_TRANSFORM, obj_to_any(obj));
    }
}

/* remove an object from the hero's inventory */
void
freeinv(obj)
register struct obj *obj;
{
    extract_nobj(obj, &invent);
    freeinv_core(obj);
    update_inventory();
}

void
delallobj(x, y)
int x, y;
{
    struct obj *otmp, *otmp2;

    for (otmp = level.objects[x][y]; otmp; otmp = otmp2) {
        if (otmp == uball)
            unpunish();
        /* after unpunish(), or might get deallocated chain */
        otmp2 = otmp->nexthere;
        if (otmp == uchain)
            continue;
        delobj(otmp);
    }
}

/* destroy object in fobj chain (if unpaid, it remains on the bill) */
void
delobj(obj)
register struct obj *obj;
{
    boolean update_map;

    if (obj->otyp == AMULET_OF_YENDOR
        || obj->otyp == CANDELABRUM_OF_INVOCATION
        || obj->otyp == BELL_OF_OPENING
        || obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        /* player might be doing something stupid, but we
         * can't guarantee that.  assume special artifacts
         * are indestructible via drawbridges, and exploding
         * chests, and golem creation, and ...
         */
        return;
    }
    update_map = (obj->where == OBJ_FLOOR);
    obj_extract_self(obj);
    if (update_map)
        newsym(obj->ox, obj->oy);
    obfree(obj, (struct obj *) 0); /* frees contents also */
}

/* try to find a particular type of object at designated map location */
struct obj *
sobj_at(otyp, x, y)
int otyp;
int x, y;
{
    register struct obj *otmp;

    for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
        if (otmp->otyp == otyp)
            break;

    return otmp;
}

/* sobj_at(&c) traversal -- find next object of specified type */
struct obj *
nxtobj(obj, type, by_nexthere)
struct obj *obj;
int type;
boolean by_nexthere;
{
    register struct obj *otmp;

    otmp = obj; /* start with the object after this one */
    do {
        otmp = !by_nexthere ? otmp->nobj : otmp->nexthere;
        if (!otmp)
            break;
    } while (otmp->otyp != type);

    return otmp;
}

struct obj *
carrying(type)
register int type;
{
    register struct obj *otmp;

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == type)
            return  otmp;
    return (struct obj *) 0;
}

/* Fictional and not-so-fictional currencies.
 * http://concord.wikia.com/wiki/List_of_Fictional_Currencies
 */
static const char *const currencies[] = {
    "Altarian Dollar",       /* The Hitchhiker's Guide to the Galaxy */
    "Ankh-Morpork Dollar",   /* Discworld */
    "auric",                 /* The Domination of Draka */
    "buckazoid",             /* Space Quest */
    "cirbozoid",             /* Starslip */
    "credit chit",           /* Deus Ex */
    "cubit",                 /* Battlestar Galactica */
    "Flanian Pobble Bead",   /* The Hitchhiker's Guide to the Galaxy */
    "fretzer",               /* Jules Verne */
    "imperial credit",       /* Star Wars */
    "Hong Kong Luna Dollar", /* The Moon is a Harsh Mistress */
    "kongbuck",              /* Snow Crash */
    "nanite",                /* System Shock 2 */
    "quatloo",               /* Star Trek, Sim City */
    "simoleon",              /* Sim City */
    "solari",                /* Spaceballs */
    "spacebuck",             /* Spaceballs */
    "sporebuck",             /* Spore */
    "Triganic Pu",           /* The Hitchhiker's Guide to the Galaxy */
    "woolong",               /* Cowboy Bebop */
    "zorkmid",               /* Zork, NetHack */
};

const char *
currency(amount)
long amount;
{
    const char *res;

    res = Hallucination ? currencies[rn2(SIZE(currencies))] : "zorkmid";
    if (amount != 1L)
        res = makeplural(res);
    return res;
}

boolean
have_lizard()
{
    register struct obj *otmp;

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == CORPSE && otmp->corpsenm == PM_LIZARD)
            return  TRUE;
    return FALSE;
}

/* 3.6 tribute */
struct obj *
u_have_novel()
{
    register struct obj *otmp;

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == SPE_NOVEL)
            return otmp;
    return (struct obj *) 0;
}

struct obj *
o_on(id, objchn)
unsigned int id;
register struct obj *objchn;
{
    struct obj *temp;

    while (objchn) {
        if (objchn->o_id == id)
            return objchn;
        if (Has_contents(objchn) && (temp = o_on(id, objchn->cobj)))
            return temp;
        objchn = objchn->nobj;
    }
    return (struct obj *) 0;
}

boolean
obj_here(obj, x, y)
register struct obj *obj;
int x, y;
{
    register struct obj *otmp;

    for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
        if (obj == otmp)
            return TRUE;
    return FALSE;
}

struct obj *
g_at(x, y)
register int x, y;
{
    register struct obj *obj = level.objects[x][y];

    while (obj) {
        if (obj->oclass == COIN_CLASS)
            return obj;
        obj = obj->nexthere;
    }
    return (struct obj *) 0;
}

/* compact a string of inventory letters by dashing runs of letters */
STATIC_OVL void
compactify(buf)
register char *buf;
{
    register int i1 = 1, i2 = 1;
    register char ilet, ilet1, ilet2;

    ilet2 = buf[0];
    ilet1 = buf[1];
    buf[++i2] = buf[++i1];
    ilet = buf[i1];
    while (ilet) {
        if (ilet == ilet1 + 1) {
            if (ilet1 == ilet2 + 1)
                buf[i2 - 1] = ilet1 = '-';
            else if (ilet2 == '-') {
                buf[i2 - 1] = ++ilet1;
                buf[i2] = buf[++i1];
                ilet = buf[i1];
                continue;
            }
        } else if (ilet == NOINVSYM) {
            /* compact three or more consecutive '#'
               characters into "#-#" */
            if (i2 >= 2 && buf[i2 - 2] == NOINVSYM && buf[i2 - 1] == NOINVSYM)
                buf[i2 - 1] = '-';
            else if (i2 >= 3 && buf[i2 - 3] == NOINVSYM && buf[i2 - 2] == '-'
                     && buf[i2 - 1] == NOINVSYM)
                --i2;
        }
        ilet2 = ilet1;
        ilet1 = ilet;
        buf[++i2] = buf[++i1];
        ilet = buf[i1];
    }
}

/* some objects shouldn't be split when count given to getobj or askchain */
boolean
splittable(obj)
struct obj *obj;
{
    return !((obj->otyp == LOADSTONE && obj->cursed)
             || (obj == uwep && welded(uwep)));
}

/* match the prompt for either 'T' or 'R' command */
STATIC_OVL boolean
taking_off(action)
const char *action;
{
    return !strcmp(action, "take off") || !strcmp(action, "remove");
}

/* match the prompt for either 'W' or 'P' command */
STATIC_OVL boolean
putting_on(action)
const char *action;
{
    return !strcmp(action, "wear") || !strcmp(action, "put on");
}

/*
 * getobj returns:
 *      struct obj *xxx:        object to do something with.
 *      (struct obj *) 0        error return: no object.
 *      &zeroobj                explicitly no object (as in w-).
!!!! test if gold can be used in unusual ways (eaten etc.)
!!!! may be able to remove "usegold"
 */
struct obj *
getobj(let, word)
register const char *let, *word;
{
    register struct obj *otmp;
    register char ilet;
    char buf[BUFSZ], qbuf[QBUFSZ];
    char lets[BUFSZ], altlets[BUFSZ], *ap;
    register int foo = 0;
    register char *bp = buf;
    xchar allowcnt = 0; /* 0, 1 or 2 */
    boolean usegold = FALSE; /* can't use gold because its illegal */
    boolean allowall = FALSE;
    boolean allownone = FALSE;
    boolean useboulder = FALSE;
    xchar foox = 0;
    long cnt;
    boolean cntgiven = FALSE;
    boolean msggiven = FALSE;
    boolean oneloop = FALSE;
    long dummymask;

    if (*let == ALLOW_COUNT)
        let++, allowcnt = 1;
    if (*let == COIN_CLASS)
        let++, usegold = TRUE;

    /* Equivalent of an "ugly check" for gold */
    if (usegold && !strcmp(word, "eat")
        && (!metallivorous(youmonst.data)
            || youmonst.data == &mons[PM_RUST_MONSTER]))
        usegold = FALSE;

    if (*let == ALL_CLASSES)
        let++, allowall = TRUE;
    if (*let == ALLOW_NONE)
        let++, allownone = TRUE;
    /* "ugly check" for reading fortune cookies, part 1.
     * The normal 'ugly check' keeps the object on the inventory list.
     * We don't want to do that for shirts/cookies, so the check for
     * them is handled a bit differently (and also requires that we set
     * allowall in the caller).
     */
    if (allowall && !strcmp(word, "read"))
        allowall = FALSE;

    /* another ugly check: show boulders (not statues) */
    if (*let == WEAPON_CLASS && !strcmp(word, "throw")
        && throws_rocks(youmonst.data))
        useboulder = TRUE;

    if (allownone)
        *bp++ = HANDS_SYM, *bp++ = ' '; /* '-' */
    ap = altlets;

    if (!flags.invlet_constant)
        reassign();
    else
        /* in case invent is in packorder, force it to be in invlet
           order before collecing candidate inventory letters;
           if player responds with '?' or '*' it will be changed
           back by display_pickinv(), but by then we'll have 'lets'
           and so won't have to re-sort in the for(;;) loop below */
        sortloot(&invent, SORTLOOT_INVLET, FALSE);

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (&bp[foo] == &buf[sizeof buf - 1]
            || ap == &altlets[sizeof altlets - 1]) {
            /* we must have a huge number of NOINVSYM items somehow */
            impossible("getobj: inventory overflow");
            break;
        }

        if (!*let || index(let, otmp->oclass)
            || (usegold && otmp->invlet == GOLD_SYM)
            || (useboulder && otmp->otyp == BOULDER)) {
            register int otyp = otmp->otyp;

            bp[foo++] = otmp->invlet;
/* clang-format off */
/* *INDENT-OFF* */
            /* ugly check: remove inappropriate things */
            if (
                (taking_off(word) /* exclude if not worn */
                 && !(otmp->owornmask & (W_ARMOR | W_ACCESSORY)))
             || (putting_on(word) /* exclude if already worn */
                 && (otmp->owornmask & (W_ARMOR | W_ACCESSORY)))
#if 0 /* 3.4.1 -- include currently wielded weapon among 'wield' choices */
             || (!strcmp(word, "wield")
                 && (otmp->owornmask & W_WEP))
#endif
             || (!strcmp(word, "ready")    /* exclude when wielded... */
                 && ((otmp == uwep || (otmp == uswapwep && u.twoweap))
                     && otmp->quan == 1L)) /* ...unless more than one */
             || ((!strcmp(word, "dip") || !strcmp(word, "grease"))
                 && inaccessible_equipment(otmp, (const char *) 0, FALSE))
             ) {
                foo--;
                foox++;
            }
            /* Second ugly check; unlike the first it won't trigger an
             * "else" in "you don't have anything else to ___".
             */
            else if (
                (putting_on(word)
                 && ((otmp->oclass == FOOD_CLASS && otmp->otyp != MEAT_RING)
                     || (otmp->oclass == TOOL_CLASS && otyp != BLINDFOLD
                         && otyp != TOWEL && otyp != LENSES)))
             || (!strcmp(word, "wield")
                 && (otmp->oclass == TOOL_CLASS && !is_weptool(otmp)))
             || (!strcmp(word, "eat") && !is_edible(otmp))
             || (!strcmp(word, "sacrifice")
                 && (otyp != CORPSE && otyp != AMULET_OF_YENDOR
                     && otyp != FAKE_AMULET_OF_YENDOR))
             || (!strcmp(word, "write with")
                 && (otmp->oclass == TOOL_CLASS
                     && otyp != MAGIC_MARKER && otyp != TOWEL))
             || (!strcmp(word, "tin")
                 && (otyp != CORPSE || !tinnable(otmp)))
             || (!strcmp(word, "rub")
                 && ((otmp->oclass == TOOL_CLASS && otyp != OIL_LAMP
                      && otyp != MAGIC_LAMP && otyp != BRASS_LANTERN)
                     || (otmp->oclass == GEM_CLASS && !is_graystone(otmp))))
             || (!strcmp(word, "use or apply")
                 /* Picks, axes, pole-weapons, bullwhips */
                 && ((otmp->oclass == WEAPON_CLASS
                      && !is_pick(otmp) && !is_axe(otmp)
                      && !is_pole(otmp) && otyp != BULLWHIP)
                     || (otmp->oclass == POTION_CLASS
                         /* only applicable potion is oil, and it will only
                            be offered as a choice when already discovered */
                         && (otyp != POT_OIL || !otmp->dknown
                             || !objects[POT_OIL].oc_name_known))
                     || (otmp->oclass == FOOD_CLASS
                         && otyp != CREAM_PIE && otyp != EUCALYPTUS_LEAF)
                     || (otmp->oclass == GEM_CLASS && !is_graystone(otmp))))
             || (!strcmp(word, "invoke")
                 && !otmp->oartifact
                 && !objects[otyp].oc_unique
                 && (otyp != FAKE_AMULET_OF_YENDOR || otmp->known)
                 && otyp != CRYSTAL_BALL /* synonym for apply */
                 /* note: presenting the possibility of invoking non-artifact
                    mirrors and/or lamps is simply a cruel deception... */
                 && otyp != MIRROR
                 && otyp != MAGIC_LAMP
                 && (otyp != OIL_LAMP /* don't list known oil lamp */
                     || (otmp->dknown && objects[OIL_LAMP].oc_name_known)))
             || (!strcmp(word, "untrap with")
                 && ((otmp->oclass == TOOL_CLASS && otyp != CAN_OF_GREASE)
                     || (otmp->oclass == POTION_CLASS
                         /* only applicable potion is oil, and it will only
                            be offered as a choice when already discovered */
                         && (otyp != POT_OIL || !otmp->dknown
                             || !objects[POT_OIL].oc_name_known))))
             || (!strcmp(word, "tip") && !Is_container(otmp)
                 /* include horn of plenty if sufficiently discovered */
                 && (otmp->otyp != HORN_OF_PLENTY || !otmp->dknown
                     || !objects[HORN_OF_PLENTY].oc_name_known))
             || (!strcmp(word, "charge") && !is_chargeable(otmp))
             || (!strcmp(word, "open") && otyp != TIN)
             || (!strcmp(word, "call") && !objtyp_is_callable(otyp))
             || (!strcmp(word, "adjust") && otmp->oclass == COIN_CLASS
                 && !usegold)
             ) {
                foo--;
            }
            /* Third ugly check:  acceptable but not listed as likely
             * candidates in the prompt or in the inventory subset if
             * player responds with '?'.
             */
            else if (
             /* ugly check for unworn armor that can't be worn */
                (putting_on(word) && *let == ARMOR_CLASS
                 && !canwearobj(otmp, &dummymask, FALSE))
             /* or armor with 'P' or 'R' or accessory with 'W' or 'T' */
             || ((putting_on(word) || taking_off(word))
                 && ((*let == ARMOR_CLASS) ^ (otmp->oclass == ARMOR_CLASS)))
             /* or unsuitable items rubbed on known touchstone */
             || (!strncmp(word, "rub on the stone", 16)
                 && *let == GEM_CLASS && otmp->dknown
                 && objects[otyp].oc_name_known)
             /* suppress corpses on astral, amulets elsewhere */
             || (!strcmp(word, "sacrifice")
                 /* (!astral && amulet) || (astral && !amulet) */
                 && (!Is_astralevel(&u.uz) ^ (otmp->oclass != AMULET_CLASS)))
             /* suppress container being stashed into */
             || (!strcmp(word, "stash") && !ck_bag(otmp))
             /* worn armor (shirt, suit) covered by worn armor (suit, cloak)
                or accessory (ring) covered by cursed worn armor (gloves) */
             || (taking_off(word)
                 && inaccessible_equipment(otmp, (const char *) 0,
                                      (boolean) (otmp->oclass == RING_CLASS)))
             || (!strcmp(word, "write on")
                 && (!(otyp == SCR_BLANK_PAPER || otyp == SPE_BLANK_PAPER)
                     || !otmp->dknown || !objects[otyp].oc_name_known))
             ) {
                /* acceptable but not listed as likely candidate */
                foo--;
                allowall = TRUE;
                *ap++ = otmp->invlet;
            }
/* *INDENT-ON* */
/* clang-format on */
        } else {
            /* "ugly check" for reading fortune cookies, part 2 */
            if ((!strcmp(word, "read") && is_readable(otmp)))
                allowall = usegold = TRUE;
        }
    }

    bp[foo] = 0;
    if (foo == 0 && bp > buf && bp[-1] == ' ')
        *--bp = 0;
    Strcpy(lets, bp); /* necessary since we destroy buf */
    if (foo > 5)      /* compactify string */
        compactify(bp);
    *ap = '\0';

    if (!foo && !allowall && !allownone) {
        You("don't have anything %sto %s.", foox ? "else " : "", word);
        return (struct obj *) 0;
    } else if (!strcmp(word, "write on")) { /* ugly check for magic marker */
        /* we wanted all scrolls and books in altlets[], but that came with
           'allowall' which we don't want since it prevents "silly thing"
           result if anything other than scroll or spellbook is chosen */
        allowall = FALSE;
    }
    for (;;) {
        cnt = 0;
        cntgiven = FALSE;
        Sprintf(qbuf, "What do you want to %s?", word);
        if (in_doagain)
            ilet = readchar();
        else if (iflags.force_invmenu) {
            /* don't overwrite a possible quitchars */
            if (!oneloop)
                ilet = *let ? '?' : '*';
            if (!msggiven)
                putmsghistory(qbuf, FALSE);
            msggiven = TRUE;
            oneloop = TRUE;
        } else {
            if (!buf[0])
                Strcat(qbuf, " [*]");
            else
                Sprintf(eos(qbuf), " [%s or ?*]", buf);
            ilet = yn_function(qbuf, (char *) 0, '\0');
        }
        if (digit(ilet)) {
            long tmpcnt = 0;

            if (!allowcnt) {
                pline("No count allowed with this command.");
                continue;
            }
            ilet = get_count(NULL, ilet, LARGEST_INT, &tmpcnt, TRUE);
            if (tmpcnt) {
                cnt = tmpcnt;
                cntgiven = TRUE;
            }
        }
        if (index(quitchars, ilet)) {
            if (flags.verbose)
                pline1(Never_mind);
            return (struct obj *) 0;
        }
        if (ilet == HANDS_SYM) { /* '-' */
            if (!allownone) {
                char *suf = (char *) 0;

                strcpy(buf, word);
                if ((bp = strstr(buf, " on the ")) != 0) {
                    /* rub on the stone[s] */
                    *bp = '\0';
                    suf = (bp + 1);
                }
                if ((bp = strstr(buf, " or ")) != 0) {
                    *bp = '\0';
                    bp = (rn2(2) ? buf : (bp + 4));
                } else
                    bp = buf;
                You("mime %s something%s%s.", ing_suffix(bp), suf ? " " : "",
                    suf ? suf : "");
            }
            return (allownone ? &zeroobj : (struct obj *) 0);
        }
redo_menu:
        /* since gold is now kept in inventory, we need to do processing for
           select-from-invent before checking whether gold has been picked */
        if (ilet == '?' || ilet == '*') {
            char *allowed_choices = (ilet == '?') ? lets : (char *) 0;
            long ctmp = 0;
            char menuquery[QBUFSZ];

            menuquery[0] = qbuf[0] = '\0';
            if (iflags.force_invmenu)
                Sprintf(menuquery, "What do you want to %s?", word);
            if (!strcmp(word, "grease"))
                Sprintf(qbuf, "your %s", makeplural(body_part(FINGER)));
            else if (!strcmp(word, "write with"))
                Sprintf(qbuf, "your %s", body_part(FINGERTIP));
            else if (!strcmp(word, "wield"))
                Sprintf(qbuf, "your %s %s%s", uarmg ? "gloved" : "bare",
                        makeplural(body_part(HAND)),
                        !uwep ? " (wielded)" : "");
            else if (!strcmp(word, "ready"))
                Sprintf(qbuf, "empty quiver%s",
                        !uquiver ? " (nothing readied)" : "");

            if (ilet == '?' && !*lets && *altlets)
                allowed_choices = altlets;
            ilet = display_pickinv(allowed_choices, *qbuf ? qbuf : (char *) 0,
                                   menuquery,
                                   TRUE, allowcnt ? &ctmp : (long *) 0);
            if (!ilet)
                continue;
            if (ilet == HANDS_SYM)
                return &zeroobj;
            if (ilet == '\033') {
                if (flags.verbose)
                    pline1(Never_mind);
                return (struct obj *) 0;
            }
            if (ilet == '*')
                goto redo_menu;
            if (allowcnt && ctmp >= 0) {
                cnt = ctmp;
                cntgiven = TRUE;
            }
            /* they typed a letter (not a space) at the prompt */
        }
        /* find the item which was picked */
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (otmp->invlet == ilet)
                break;
        /* some items have restrictions */
        if (ilet == def_oc_syms[COIN_CLASS].sym
            /* guard against the [hypothetical] chace of having more
               than one invent slot of gold and picking the non-'$' one */
            || (otmp && otmp->oclass == COIN_CLASS)) {
            if (!usegold) {
                You("cannot %s gold.", word);
                return (struct obj *) 0;
            }
            /* Historic note: early Nethack had a bug which was
             * first reported for Larn, where trying to drop 2^32-n
             * gold pieces was allowed, and did interesting things
             * to your money supply.  The LRS is the tax bureau
             * from Larn.
             */
            if (cntgiven && cnt <= 0) {
                if (cnt < 0)
                    pline_The(
                  "LRS would be very interested to know you have that much.");
                return (struct obj *) 0;
            }
        }
        if (cntgiven && !strcmp(word, "throw")) {
            /* permit counts for throwing gold, but don't accept
             * counts for other things since the throw code will
             * split off a single item anyway */
            if (cnt == 0)
                return (struct obj *) 0;
            if (cnt > 1 && (ilet != def_oc_syms[COIN_CLASS].sym
                && !(otmp && otmp->oclass == COIN_CLASS))) {
                You("can only throw one item at a time.");
                continue;
            }
        }
        context.botl = 1; /* May have changed the amount of money */
        savech(ilet);
        /* [we used to set otmp (by finding ilet in invent) here, but
           that's been moved above so that otmp can be checked earlier] */
        /* verify the chosen object */
        if (!otmp) {
            You("don't have that object.");
            if (in_doagain)
                return (struct obj *) 0;
            continue;
        } else if (cnt < 0 || otmp->quan < cnt) {
            You("don't have that many!  You have only %ld.", otmp->quan);
            if (in_doagain)
                return (struct obj *) 0;
            continue;
        }
        break;
    }
    if (!allowall && let && !index(let, otmp->oclass)
        && !(usegold && otmp->oclass == COIN_CLASS)) {
        silly_thing(word, otmp);
        return (struct obj *) 0;
    }
    if (cntgiven) {
        if (cnt == 0)
            return (struct obj *) 0;
        if (cnt != otmp->quan) {
            /* don't split a stack of cursed loadstones */
            if (splittable(otmp))
                otmp = splitobj(otmp, cnt);
            else if (otmp->otyp == LOADSTONE && otmp->cursed)
                /* kludge for canletgo()'s can't-drop-this message */
                otmp->corpsenm = (int) cnt;
        }
    }
    return otmp;
}

void
silly_thing(word, otmp)
const char *word;
struct obj *otmp;
{
#if 1 /* 'P','R' vs 'W','T' handling is obsolete */
    nhUse(otmp);
#else
    const char *s1, *s2, *s3;
    int ocls = otmp->oclass, otyp = otmp->otyp;

    s1 = s2 = s3 = 0;
    /* check for attempted use of accessory commands ('P','R') on armor
       and for corresponding armor commands ('W','T') on accessories */
    if (ocls == ARMOR_CLASS) {
        if (!strcmp(word, "put on"))
            s1 = "W", s2 = "wear", s3 = "";
        else if (!strcmp(word, "remove"))
            s1 = "T", s2 = "take", s3 = " off";
    } else if ((ocls == RING_CLASS || otyp == MEAT_RING)
               || ocls == AMULET_CLASS
               || (otyp == BLINDFOLD || otyp == TOWEL || otyp == LENSES)) {
        if (!strcmp(word, "wear"))
            s1 = "P", s2 = "put", s3 = " on";
        else if (!strcmp(word, "take off"))
            s1 = "R", s2 = "remove", s3 = "";
    }
    if (s1)
        pline("Use the '%s' command to %s %s%s.", s1, s2,
              !(is_plural(otmp) || pair_of(otmp)) ? "that" : "those", s3);
    else
#endif
        pline(silly_thing_to, word);
}

STATIC_PTR int
ckvalidcat(otmp)
struct obj *otmp;
{
    /* use allow_category() from pickup.c */
    return (int) allow_category(otmp);
}

STATIC_PTR int
ckunpaid(otmp)
struct obj *otmp;
{
    return (otmp->unpaid || (Has_contents(otmp) && count_unpaid(otmp->cobj)));
}

boolean
wearing_armor()
{
    return (boolean) (uarm || uarmc || uarmf || uarmg
                      || uarmh || uarms || uarmu);
}

boolean
is_worn(otmp)
struct obj *otmp;
{
    return (otmp->owornmask & (W_ARMOR | W_ACCESSORY | W_SADDLE | W_WEAPON))
            ? TRUE
            : FALSE;
}

/* extra xprname() input that askchain() can't pass through safe_qbuf() */
STATIC_VAR struct xprnctx {
    char let;
    boolean dot;
} safeq_xprn_ctx;

/* safe_qbuf() -> short_oname() callback */
STATIC_PTR char *
safeq_xprname(obj)
struct obj *obj;
{
    return xprname(obj, (char *) 0, safeq_xprn_ctx.let, safeq_xprn_ctx.dot,
                   0L, 0L);
}

/* alternate safe_qbuf() -> short_oname() callback */
STATIC_PTR char *
safeq_shortxprname(obj)
struct obj *obj;
{
    return xprname(obj, ansimpleoname(obj), safeq_xprn_ctx.let,
                   safeq_xprn_ctx.dot, 0L, 0L);
}

static NEARDATA const char removeables[] = { ARMOR_CLASS, WEAPON_CLASS,
                                             RING_CLASS,  AMULET_CLASS,
                                             TOOL_CLASS,  0 };

/* Interactive version of getobj - used for Drop, Identify, and Takeoff (A).
   Return the number of times fn was called successfully.
   If combo is TRUE, we just use this to get a category list. */
int
ggetobj(word, fn, mx, combo, resultflags)
const char *word;
int FDECL((*fn), (OBJ_P)), mx;
boolean combo; /* combination menu flag */
unsigned *resultflags;
{
    int FDECL((*ckfn), (OBJ_P)) = (int FDECL((*), (OBJ_P))) 0;
    boolean FDECL((*ofilter), (OBJ_P)) = (boolean FDECL((*), (OBJ_P))) 0;
    boolean takeoff, ident, allflag, m_seen;
    int itemcount;
    int oletct, iletct, unpaid, oc_of_sym;
    char sym, *ip, olets[MAXOCLASSES + 5], ilets[MAXOCLASSES + 10];
    char extra_removeables[3 + 1]; /* uwep,uswapwep,uquiver */
    char buf[BUFSZ] = DUMMY, qbuf[QBUFSZ];

    if (!invent) {
        You("have nothing to %s.", word);
        if (resultflags)
            *resultflags = ALL_FINISHED;
        return 0;
    }
    if (resultflags)
        *resultflags = 0;
    takeoff = ident = allflag = m_seen = FALSE;
    add_valid_menu_class(0); /* reset */
    if (taking_off(word)) {
        takeoff = TRUE;
        ofilter = is_worn;
    } else if (!strcmp(word, "identify")) {
        ident = TRUE;
        ofilter = not_fully_identified;
    }

    iletct = collect_obj_classes(ilets, invent, FALSE, ofilter, &itemcount);
    unpaid = count_unpaid(invent);

    if (ident && !iletct) {
        return -1; /* no further identifications */
    } else if (invent) {
        ilets[iletct++] = ' ';
        if (unpaid)
            ilets[iletct++] = 'u';
        if (count_buc(invent, BUC_BLESSED, ofilter))
            ilets[iletct++] = 'B';
        if (count_buc(invent, BUC_UNCURSED, ofilter))
            ilets[iletct++] = 'U';
        if (count_buc(invent, BUC_CURSED, ofilter))
            ilets[iletct++] = 'C';
        if (count_buc(invent, BUC_UNKNOWN, ofilter))
            ilets[iletct++] = 'X';
        ilets[iletct++] = 'a';
    }
    ilets[iletct++] = 'i';
    if (!combo)
        ilets[iletct++] = 'm'; /* allow menu presentation on request */
    ilets[iletct] = '\0';

    for (;;) {
        Sprintf(qbuf, "What kinds of thing do you want to %s? [%s]",
                word, ilets);
        getlin(qbuf, buf);
        if (buf[0] == '\033')
            return 0;
        if (index(buf, 'i')) {
            char ailets[1+26+26+1+5+1]; /* $ + a-z + A-Z + # + slop + \0 */
            struct obj *otmp;

            /* applicable inventory letters; if empty, show entire invent */
            ailets[0] = '\0';
            if (ofilter)
                for (otmp = invent; otmp; otmp = otmp->nobj)
                    /* index() check: limit overflow items to one '#' */
                    if ((*ofilter)(otmp) && !index(ailets, otmp->invlet))
                        (void) strkitten(ailets, otmp->invlet);
            if (display_inventory(ailets, TRUE) == '\033')
                return 0;
        } else
            break;
    }

    extra_removeables[0] = '\0';
    if (takeoff) {
        /* arbitrary types of items can be placed in the weapon slots
           [any duplicate entries in extra_removeables[] won't matter] */
        if (uwep)
            (void) strkitten(extra_removeables, uwep->oclass);
        if (uswapwep)
            (void) strkitten(extra_removeables, uswapwep->oclass);
        if (uquiver)
            (void) strkitten(extra_removeables, uquiver->oclass);
    }

    ip = buf;
    olets[oletct = 0] = '\0';
    while ((sym = *ip++) != '\0') {
        if (sym == ' ')
            continue;
        oc_of_sym = def_char_to_objclass(sym);
        if (takeoff && oc_of_sym != MAXOCLASSES) {
            if (index(extra_removeables, oc_of_sym)) {
                ; /* skip rest of takeoff checks */
            } else if (!index(removeables, oc_of_sym)) {
                pline("Not applicable.");
                return 0;
            } else if (oc_of_sym == ARMOR_CLASS && !wearing_armor()) {
                noarmor(FALSE);
                return 0;
            } else if (oc_of_sym == WEAPON_CLASS && !uwep && !uswapwep
                       && !uquiver) {
                You("are not wielding anything.");
                return 0;
            } else if (oc_of_sym == RING_CLASS && !uright && !uleft) {
                You("are not wearing rings.");
                return 0;
            } else if (oc_of_sym == AMULET_CLASS && !uamul) {
                You("are not wearing an amulet.");
                return 0;
            } else if (oc_of_sym == TOOL_CLASS && !ublindf) {
                You("are not wearing a blindfold.");
                return 0;
            }
        }

        if (oc_of_sym == COIN_CLASS && !combo) {
            context.botl = 1;
        } else if (sym == 'a') {
            allflag = TRUE;
        } else if (sym == 'A') {
            ; /* same as the default */
        } else if (sym == 'u') {
            add_valid_menu_class('u');
            ckfn = ckunpaid;
        } else if (index("BUCX", sym)) {
            add_valid_menu_class(sym); /* 'B','U','C',or 'X' */
            ckfn = ckvalidcat;
        } else if (sym == 'm') {
            m_seen = TRUE;
        } else if (oc_of_sym == MAXOCLASSES) {
            You("don't have any %c's.", sym);
        } else if (oc_of_sym != VENOM_CLASS) { /* suppress venom */
            if (!index(olets, oc_of_sym)) {
                add_valid_menu_class(oc_of_sym);
                olets[oletct++] = oc_of_sym;
                olets[oletct] = 0;
            }
        }
    }

    if (m_seen) {
        return (allflag
                || (!oletct && ckfn != ckunpaid && ckfn != ckvalidcat))
               ? -2 : -3;
    } else if (flags.menu_style != MENU_TRADITIONAL && combo && !allflag) {
        return 0;
#if 0
    /* !!!! test gold dropping */
    } else if (allowgold == 2 && !oletct) {
        return 1; /* you dropped gold (or at least tried to)  */
#endif
    } else {
        int cnt = askchain(&invent, olets, allflag, fn, ckfn, mx, word);
        /*
         * askchain() has already finished the job in this case
         * so set a special flag to convey that back to the caller
         * so that it won't continue processing.
         * Fix for bug C331-1 reported by Irina Rempt-Drijfhout.
         */
        if (combo && allflag && resultflags)
            *resultflags |= ALL_FINISHED;
        return cnt;
    }
}

/*
 * Walk through the chain starting at objchn and ask for all objects
 * with olet in olets (if nonNULL) and satisfying ckfn (if nonnull)
 * whether the action in question (i.e., fn) has to be performed.
 * If allflag then no questions are asked.  Mx gives the max number
 * of objects to be treated.  Return the number of objects treated.
 */
int
askchain(objchn, olets, allflag, fn, ckfn, mx, word)
struct obj **objchn;
int allflag, mx;
const char *olets, *word; /* olets is an Obj Class char array */
int FDECL((*fn), (OBJ_P)), FDECL((*ckfn), (OBJ_P));
{
    struct obj *otmp, *otmpo;
    register char sym, ilet;
    register int cnt = 0, dud = 0, tmp;
    boolean takeoff, nodot, ident, take_out, put_in, first, ininv, bycat;
    char qbuf[QBUFSZ], qpfx[QBUFSZ];

    takeoff = taking_off(word);
    ident = !strcmp(word, "identify");
    take_out = !strcmp(word, "take out");
    put_in = !strcmp(word, "put in");
    nodot = (!strcmp(word, "nodot") || !strcmp(word, "drop") || ident
             || takeoff || take_out || put_in);
    ininv = (*objchn == invent);
    bycat = (menu_class_present('u')
             || menu_class_present('B') || menu_class_present('U')
             || menu_class_present('C') || menu_class_present('X'));

    /* someday maybe we'll sort by 'olets' too (temporarily replace
       flags.packorder and pass SORTLOOT_PACK), but not yet... */
    sortloot(objchn, SORTLOOT_INVLET, FALSE);

    first = TRUE;
    /*
     * Interrogate in the object class order specified.
     * For example, if a person specifies =/ then first all rings
     * will be asked about followed by all wands.  -dgk
     */
nextclass:
    ilet = 'a' - 1;
    if (*objchn && (*objchn)->oclass == COIN_CLASS)
        ilet--;                     /* extra iteration */
    /*
     * Multiple Drop can change the invent chain while it operates
     * (dropping a burning potion of oil while levitating creates
     * an explosion which can destroy inventory items), so simple
     * list traversal
     *  for (otmp = *objchn; otmp; otmp = otmp2) {
     *      otmp2 = otmp->nobj;
     *      ...
     *  }
     * is inadequate here.  Use each object's bypass bit to keep
     * track of which list elements have already been processed.
     */
    bypass_objlist(*objchn, FALSE); /* clear chain's bypass bits */
    while ((otmp = nxt_unbypassed_obj(*objchn)) != 0) {
        if (ilet == 'z')
            ilet = 'A';
        else if (ilet == 'Z')
            ilet = NOINVSYM; /* '#' */
        else
            ilet++;
        if (olets && *olets && otmp->oclass != *olets)
            continue;
        if (takeoff && !is_worn(otmp))
            continue;
        if (ident && !not_fully_identified(otmp))
            continue;
        if (ckfn && !(*ckfn)(otmp))
            continue;
        if (bycat && !ckvalidcat(otmp))
            continue;
        if (!allflag) {
            safeq_xprn_ctx.let = ilet;
            safeq_xprn_ctx.dot = !nodot;
            *qpfx = '\0';
            if (first) {
                /* traditional_loot() skips prompting when only one
                   class of objects is involved, so prefix the first
                   object being queried here with an explanation why */
                if (take_out || put_in)
                    Sprintf(qpfx, "%s: ", word), *qpfx = highc(*qpfx);
                first = FALSE;
            }
            (void) safe_qbuf(qbuf, qpfx, "?", otmp,
                             ininv ? safeq_xprname : doname,
                             ininv ? safeq_shortxprname : ansimpleoname,
                             "item");
            sym = (takeoff || ident || otmp->quan < 2L) ? nyaq(qbuf)
                                                        : nyNaq(qbuf);
        } else
            sym = 'y';

        otmpo = otmp;
        if (sym == '#') {
            /* Number was entered; split the object unless it corresponds
               to 'none' or 'all'.  2 special cases: cursed loadstones and
               welded weapons (eg, multiple daggers) will remain as merged
               unit; done to avoid splitting an object that won't be
               droppable (even if we're picking up rather than dropping). */
            if (!yn_number) {
                sym = 'n';
            } else {
                sym = 'y';
                if (yn_number < otmp->quan && splittable(otmp))
                    otmp = splitobj(otmp, yn_number);
            }
        }
        switch (sym) {
        case 'a':
            allflag = 1;
            /*FALLTHRU*/
        case 'y':
            tmp = (*fn)(otmp);
            if (tmp < 0) {
                if (container_gone(fn)) {
                    /* otmp caused magic bag to explode;
                       both are now gone */
                    otmp = 0; /* and return */
                } else if (otmp && otmp != otmpo) {
                    /* split occurred, merge again */
                    (void) merged(&otmpo, &otmp);
                }
                goto ret;
            }
            cnt += tmp;
            if (--mx == 0)
                goto ret;
            /*FALLTHRU*/
        case 'n':
            if (nodot)
                dud++;
        default:
            break;
        case 'q':
            /* special case for seffects() */
            if (ident)
                cnt = -1;
            goto ret;
        }
    }
    if (olets && *olets && *++olets)
        goto nextclass;
    if (!takeoff && (dud || cnt))
        pline("That was all.");
    else if (!dud && !cnt)
        pline("No applicable objects.");
ret:
    bypass_objlist(*objchn, FALSE);
    return cnt;
}

/*
 *      Object identification routines:
 */

/* make an object actually be identified; no display updating */
void
fully_identify_obj(otmp)
struct obj *otmp;
{
    makeknown(otmp->otyp);
    if (otmp->oartifact)
        discover_artifact((xchar) otmp->oartifact);
    otmp->known = otmp->dknown = otmp->bknown = otmp->rknown = 1;
    if (Is_container(otmp) || otmp->otyp == STATUE)
        otmp->cknown = otmp->lknown = 1;
    if (otmp->otyp == EGG && otmp->corpsenm != NON_PM)
        learn_egg_type(otmp->corpsenm);
}

/* ggetobj callback routine; identify an object and give immediate feedback */
int
identify(otmp)
struct obj *otmp;
{
    fully_identify_obj(otmp);
    prinv((char *) 0, otmp, 0L);
    return 1;
}

/* menu of unidentified objects; select and identify up to id_limit of them */
STATIC_OVL void
menu_identify(id_limit)
int id_limit;
{
    menu_item *pick_list;
    int n, i, first = 1, tryct = 5;
    char buf[BUFSZ];
    /* assumptions:  id_limit > 0 and at least one unID'd item is present */

    while (id_limit) {
        Sprintf(buf, "What would you like to identify %s?",
                first ? "first" : "next");
        n = query_objlist(buf, &invent, (SIGNAL_NOMENU | SIGNAL_ESCAPE
                                         | USE_INVLET | INVORDER_SORT),
                          &pick_list, PICK_ANY, not_fully_identified);

        if (n > 0) {
            if (n > id_limit)
                n = id_limit;
            for (i = 0; i < n; i++, id_limit--)
                (void) identify(pick_list[i].item.a_obj);
            free((genericptr_t) pick_list);
            mark_synch(); /* Before we loop to pop open another menu */
            first = 0;
        } else if (n == -2) { /* player used ESC to quit menu */
            break;
        } else if (n == -1) { /* no eligible items found */
            pline("That was all.");
            break;
        } else if (!--tryct) { /* stop re-prompting */
            pline1(thats_enough_tries);
            break;
        } else { /* try again */
            pline("Choose an item; use ESC to decline.");
        }
    }
}

/* dialog with user to identify a given number of items; 0 means all */
void
identify_pack(id_limit, learning_id)
int id_limit;
boolean learning_id; /* true if we just read unknown identify scroll */
{
    struct obj *obj, *the_obj;
    int n, unid_cnt;

    unid_cnt = 0;
    the_obj = 0; /* if unid_cnt ends up 1, this will be it */
    for (obj = invent; obj; obj = obj->nobj)
        if (not_fully_identified(obj))
            ++unid_cnt, the_obj = obj;

    if (!unid_cnt) {
        You("have already identified all %sof your possessions.",
            learning_id ? "the rest " : "");
    } else if (!id_limit || id_limit >= unid_cnt) {
        /* identify everything */
        if (unid_cnt == 1) {
            (void) identify(the_obj);
        } else {
            /* TODO:  use fully_identify_obj and cornline/menu/whatever here
             */
            for (obj = invent; obj; obj = obj->nobj)
                if (not_fully_identified(obj))
                    (void) identify(obj);
        }
    } else {
        /* identify up to `id_limit' items */
        n = 0;
        if (flags.menu_style == MENU_TRADITIONAL)
            do {
                n = ggetobj("identify", identify, id_limit, FALSE,
                            (unsigned *) 0);
                if (n < 0)
                    break; /* quit or no eligible items */
            } while ((id_limit -= n) > 0);
        if (n == 0 || n < -1)
            menu_identify(id_limit);
    }
    update_inventory();
}

/* called when regaining sight; mark inventory objects which were picked
   up while blind as now having been seen */
void
learn_unseen_invent()
{
    struct obj *otmp;

    if (Blind)
        return; /* sanity check */

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (otmp->dknown)
            continue; /* already seen */
        /* set dknown, perhaps bknown (for priest[ess]) */
        (void) xname(otmp);
        /*
         * If object->eknown gets implemented (see learnwand(zap.c)),
         * handle deferred discovery here.
         */
    }
    update_inventory();
}

/* should of course only be called for things in invent */
STATIC_OVL char
obj_to_let(obj)
struct obj *obj;
{
    if (!flags.invlet_constant) {
        obj->invlet = NOINVSYM;
        reassign();
    }
    return obj->invlet;
}

/*
 * Print the indicated quantity of the given object.  If quan == 0L then use
 * the current quantity.
 */
void
prinv(prefix, obj, quan)
const char *prefix;
struct obj *obj;
long quan;
{
    if (!prefix)
        prefix = "";
    pline("%s%s%s", prefix, *prefix ? " " : "",
          xprname(obj, (char *) 0, obj_to_let(obj), TRUE, 0L, quan));
}

char *
xprname(obj, txt, let, dot, cost, quan)
struct obj *obj;
const char *txt; /* text to print instead of obj */
char let;        /* inventory letter */
boolean dot;     /* append period; (dot && cost => Iu) */
long cost;       /* cost (for inventory of unpaid or expended items) */
long quan;       /* if non-0, print this quantity, not obj->quan */
{
#ifdef LINT /* handle static char li[BUFSZ]; */
    char li[BUFSZ];
#else
    static char li[BUFSZ];
#endif
    boolean use_invlet = (flags.invlet_constant
                          && let != CONTAINED_SYM && let != HANDS_SYM);
    long savequan = 0;

    if (quan && obj) {
        savequan = obj->quan;
        obj->quan = quan;
    }
    /*
     * If let is:
     *  -  Then obj == null and 'txt' refers to hands or fingers.
     *  *  Then obj == null and we are printing a total amount.
     *  >  Then the object is contained and doesn't have an inventory letter.
     */
    if (cost != 0 || let == '*') {
        /* if dot is true, we're doing Iu, otherwise Ix */
        Sprintf(li,
                iflags.menu_tab_sep ? "%c - %s\t%6ld %s"
                                    : "%c - %-45s %6ld %s",
                (dot && use_invlet ? obj->invlet : let),
                (txt ? txt : doname(obj)), cost, currency(cost));
    } else {
        /* ordinary inventory display or pickup message */
        Sprintf(li, "%c - %s%s", (use_invlet ? obj->invlet : let),
                (txt ? txt : doname(obj)), (dot ? "." : ""));
    }
    if (savequan)
        obj->quan = savequan;

    return li;
}

/* the 'i' command */
int
ddoinv()
{
    (void) display_inventory((char *) 0, FALSE);
    return 0;
}

/*
 * find_unpaid()
 *
 * Scan the given list of objects.  If last_found is NULL, return the first
 * unpaid object found.  If last_found is not NULL, then skip over unpaid
 * objects until last_found is reached, then set last_found to NULL so the
 * next unpaid object is returned.  This routine recursively follows
 * containers.
 */
STATIC_OVL struct obj *
find_unpaid(list, last_found)
struct obj *list, **last_found;
{
    struct obj *obj;

    while (list) {
        if (list->unpaid) {
            if (*last_found) {
                /* still looking for previous unpaid object */
                if (list == *last_found)
                    *last_found = (struct obj *) 0;
            } else
                return ((*last_found = list));
        }
        if (Has_contents(list)) {
            if ((obj = find_unpaid(list->cobj, last_found)) != 0)
                return obj;
        }
        list = list->nobj;
    }
    return (struct obj *) 0;
}

/* for perm_invent when operating on a partial inventory display, so that
   the persistent one doesn't get shrunk during filtering for item selection
   then regrown to full inventory, possibly being resized in the process */
static winid cached_pickinv_win = WIN_ERR;

void
free_pickinv_cache()
{
    if (cached_pickinv_win != WIN_ERR) {
        destroy_nhwindow(cached_pickinv_win);
        cached_pickinv_win = WIN_ERR;
    }
}

/*
 * Internal function used by display_inventory and getobj that can display
 * inventory and return a count as well as a letter. If out_cnt is not null,
 * any count returned from the menu selection is placed here.
 */
STATIC_OVL char
display_pickinv(lets, xtra_choice, query, want_reply, out_cnt)
register const char *lets;
const char *xtra_choice; /* "fingers", pick hands rather than an object */
const char *query;
boolean want_reply;
long *out_cnt;
{
    static const char not_carrying_anything[] = "Not carrying anything";
    struct obj *otmp;
    char ilet, ret;
    char *invlet = flags.inv_order;
    int n, classcount;
    winid win;                        /* windows being used */
    anything any;
    menu_item *selected;

    if (lets && !*lets)
        lets = 0; /* simplify tests: (lets) instead of (lets && *lets) */

    if (flags.perm_invent && (lets || xtra_choice)) {
        /* partial inventory in perm_invent setting; don't operate on
           full inventory window, use an alternate one instead; create
           the first time needed and keep it for re-use as needed later */
        if (cached_pickinv_win == WIN_ERR)
            cached_pickinv_win = create_nhwindow(NHW_MENU);
        win = cached_pickinv_win;
    } else
        win = WIN_INVEN;

    /*
     * Exit early if no inventory -- but keep going if we are doing
     * a permanent inventory update.  We need to keep going so the
     * permanent inventory window updates itself to remove the last
     * item(s) dropped.  One down side:  the addition of the exception
     * for permanent inventory window updates _can_ pop the window
     * up when it's not displayed -- even if it's empty -- because we
     * don't know at this level if its up or not.  This may not be
     * an issue if empty checks are done before hand and the call
     * to here is short circuited away.
     *
     * 2: our count here is only to distinguish between 0 and 1 and
     * more than 1; for the last one, we don't need a precise number.
     * For perm_invent update we force 'more than 1'.
     */
    n = (flags.perm_invent && !lets && !want_reply) ? 2
        : lets ? (int) strlen(lets)
               : !invent ? 0 : !invent->nobj ? 1 : 2;
    /* for xtra_choice, there's another 'item' not included in initial 'n';
       for !lets (full invent) and for override_ID (wizard mode identify),
       skip message_menu handling of single item even if item count was 1 */
    if (xtra_choice || (n == 1 && (!lets || iflags.override_ID)))
        ++n;

    if (n == 0) {
        pline("%s.", not_carrying_anything);
        return 0;
    }

    /* oxymoron? temporarily assign permanent inventory letters */
    if (!flags.invlet_constant)
        reassign();

    if (n == 1 && !iflags.force_invmenu) {
        /* when only one item of interest, use pline instead of menus;
           we actually use a fake message-line menu in order to allow
           the user to perform selection at the --More-- prompt for tty */
        ret = '\0';
        if (xtra_choice) {
            /* xtra_choice is "bare hands" (wield), "fingertip" (Engrave),
               "nothing" (ready Quiver), or "fingers" (apply grease) */
            ret = message_menu(HANDS_SYM, PICK_ONE,
                               xprname((struct obj *) 0, xtra_choice,
                                       HANDS_SYM, TRUE, 0L, 0L)); /* '-' */
        } else {
            for (otmp = invent; otmp; otmp = otmp->nobj)
                if (!lets || otmp->invlet == lets[0])
                    break;
            if (otmp)
                ret = message_menu(otmp->invlet,
                                   want_reply ? PICK_ONE : PICK_NONE,
                                   xprname(otmp, (char *) 0, lets[0],
                                           TRUE, 0L, 0L));
        }
        if (out_cnt)
            *out_cnt = -1L; /* select all */
        return ret;
    }

    sortloot(&invent,
             (((flags.sortloot == 'f') ? SORTLOOT_LOOT : SORTLOOT_INVLET)
              | (flags.sortpack ? SORTLOOT_PACK : 0)),
             FALSE);

    start_menu(win);
    any = zeroany;
    if (wizard && iflags.override_ID) {
        char prompt[QBUFSZ];

        any.a_char = -1;
        /* wiz_identify stuffed the wiz_identify command character (^I)
           into iflags.override_ID for our use as an accelerator */
        Sprintf(prompt, "Debug Identify (%s to permanently identify)",
                visctrl(iflags.override_ID));
        add_menu(win, NO_GLYPH, &any, '_', iflags.override_ID, ATR_NONE,
                 prompt, MENU_UNSELECTED);
    } else if (xtra_choice) {
        /* wizard override ID and xtra_choice are mutually exclusive */
        if (flags.sortpack)
            add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
                     "Miscellaneous", MENU_UNSELECTED);
        any.a_char = HANDS_SYM; /* '-' */
        add_menu(win, NO_GLYPH, &any, HANDS_SYM, 0, ATR_NONE,
                 xtra_choice, MENU_UNSELECTED);
    }
nextclass:
    classcount = 0;
    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (lets && !index(lets, otmp->invlet))
            continue;
        if (!flags.sortpack || otmp->oclass == *invlet) {
            any = zeroany; /* all bits zero */
            ilet = otmp->invlet;
            if (flags.sortpack && !classcount) {
                add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
                         let_to_name(*invlet, FALSE,
                                     (want_reply && iflags.menu_head_objsym)),
                         MENU_UNSELECTED);
                classcount++;
            }
            any.a_char = ilet;
            add_menu(win, obj_to_glyph(otmp), &any, ilet, 0, ATR_NONE,
                     doname(otmp), MENU_UNSELECTED);
        }
    }
    if (flags.sortpack) {
        if (*++invlet)
            goto nextclass;
        if (--invlet != venom_inv) {
            invlet = venom_inv;
            goto nextclass;
        }
    }
    if (iflags.force_invmenu && lets && want_reply) {
        any = zeroany;
        add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
                 "Special", MENU_UNSELECTED);
        any.a_char = '*';
        add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE,
                 "(list everything)", MENU_UNSELECTED);
    }
    /* for permanent inventory where we intend to show everything but
       nothing has been listed (because there isn't anyhing to list;
       recognized via any.a_char still being zero; the n==0 case above
       gets skipped for perm_invent), put something into the menu */
    if (flags.perm_invent && !lets && !any.a_char) {
        any = zeroany;
        add_menu(win, NO_GLYPH, &any, 0, 0, 0,
                 not_carrying_anything, MENU_UNSELECTED);
        want_reply = FALSE;
    }
    end_menu(win, query && *query ? query : (char *) 0);

    n = select_menu(win, want_reply ? PICK_ONE : PICK_NONE, &selected);
    if (n > 0) {
        ret = selected[0].item.a_char;
        if (out_cnt)
            *out_cnt = selected[0].count;
        free((genericptr_t) selected);
    } else
        ret = !n ? '\0' : '\033'; /* cancelled */

    return ret;
}

/*
 * If lets == NULL or "", list all objects in the inventory.  Otherwise,
 * list all objects with object classes that match the order in lets.
 *
 * Returns the letter identifier of a selected item, or 0 if nothing
 * was selected.
 */
char
display_inventory(lets, want_reply)
const char *lets;
boolean want_reply;
{
    return display_pickinv(lets, (char *) 0, (char *) 0,
                           want_reply, (long *) 0);
}

/*
 * Show what is current using inventory letters.
 *
 */
STATIC_OVL char
display_used_invlets(avoidlet)
char avoidlet;
{
    struct obj *otmp;
    char ilet, ret = 0;
    char *invlet = flags.inv_order;
    int n, classcount, invdone = 0;
    winid win;
    anything any;
    menu_item *selected;

    if (invent) {
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        while (!invdone) {
            any = zeroany; /* set all bits to zero */
            classcount = 0;
            for (otmp = invent; otmp; otmp = otmp->nobj) {
                ilet = otmp->invlet;
                if (ilet == avoidlet)
                    continue;
                if (!flags.sortpack || otmp->oclass == *invlet) {
                    if (flags.sortpack && !classcount) {
                        any = zeroany; /* zero */
                        add_menu(win, NO_GLYPH, &any, 0, 0,
                                 iflags.menu_headings,
                                 let_to_name(*invlet, FALSE, FALSE),
                                 MENU_UNSELECTED);
                        classcount++;
                    }
                    any.a_char = ilet;
                    add_menu(win, obj_to_glyph(otmp), &any, ilet, 0, ATR_NONE,
                             doname(otmp), MENU_UNSELECTED);
                }
            }
            if (flags.sortpack && *++invlet)
                continue;
            invdone = 1;
        }
        end_menu(win, "Inventory letters used:");

        n = select_menu(win, PICK_ONE, &selected);
        if (n > 0) {
            ret = selected[0].item.a_char;
            free((genericptr_t) selected);
        } else
            ret = !n ? '\0' : '\033'; /* cancelled */
        destroy_nhwindow(win);
    }
    return ret;
}

/*
 * Returns the number of unpaid items within the given list.  This includes
 * contained objects.
 */
int
count_unpaid(list)
struct obj *list;
{
    int count = 0;

    while (list) {
        if (list->unpaid)
            count++;
        if (Has_contents(list))
            count += count_unpaid(list->cobj);
        list = list->nobj;
    }
    return count;
}

/*
 * Returns the number of items with b/u/c/unknown within the given list.
 * This does NOT include contained objects.
 *
 * Assumes that the hero sees or touches or otherwise senses the objects
 * at some point:  bknown is forced for priest[ess], like in xname().
 */
int
count_buc(list, type, filterfunc)
struct obj *list;
int type;
boolean FDECL((*filterfunc), (OBJ_P));
{
    int count = 0;

    for (; list; list = list->nobj) {
        /* priests always know bless/curse state */
        if (Role_if(PM_PRIEST))
            list->bknown = (list->oclass != COIN_CLASS);
        /* some actions exclude some or most items */
        if (filterfunc && !(*filterfunc)(list))
            continue;

        /* coins are either uncursed or unknown based upon option setting */
        if (list->oclass == COIN_CLASS) {
            if (type == (iflags.goldX ? BUC_UNKNOWN : BUC_UNCURSED))
                ++count;
            continue;
        }
        /* check whether this object matches the requested type */
        if (!list->bknown
                ? (type == BUC_UNKNOWN)
                : list->blessed ? (type == BUC_BLESSED)
                                : list->cursed ? (type == BUC_CURSED)
                                               : (type == BUC_UNCURSED))
            ++count;
    }
    return count;
}

/* similar to count_buc(), but tallies all states at once
   rather than looking for a specific type */
void
tally_BUCX(list, by_nexthere, bcp, ucp, ccp, xcp, ocp)
struct obj *list;
boolean by_nexthere;
int *bcp, *ucp, *ccp, *xcp, *ocp;
{
    /* Future extensions:
     *  Skip current_container when list is invent, uchain when
     *  first object of list is located on the floor.  'ocp' will then
     *  have a function again (it was a counter for having skipped gold,
     *  but that's not skipped anymore).
     */
    *bcp = *ucp = *ccp = *xcp = *ocp = 0;
    for ( ; list; list = (by_nexthere ? list->nexthere : list->nobj)) {
        /* priests always know bless/curse state */
        if (Role_if(PM_PRIEST))
            list->bknown = (list->oclass != COIN_CLASS);
        /* coins are either uncursed or unknown based upon option setting */
        if (list->oclass == COIN_CLASS) {
            if (iflags.goldX)
                ++(*xcp);
            else
                ++(*ucp);
            continue;
        }
        /* ordinary items */
        if (!list->bknown)
            ++(*xcp);
        else if (list->blessed)
            ++(*bcp);
        else if (list->cursed)
            ++(*ccp);
        else /* neither blessed nor cursed => uncursed */
            ++(*ucp);
    }
}

long
count_contents(container, nested, quantity, everything)
struct obj *container;
boolean nested, /* include contents of any nested containers */
    quantity,   /* count all vs count separate stacks */
    everything; /* all objects vs only unpaid objects */
{
    struct obj *otmp;
    long count = 0L;

    for (otmp = container->cobj; otmp; otmp = otmp->nobj) {
        if (nested && Has_contents(otmp))
            count += count_contents(otmp, nested, quantity, everything);
        if (everything || otmp->unpaid)
            count += quantity ? otmp->quan : 1L;
    }
    return count;
}

STATIC_OVL void
dounpaid()
{
    winid win;
    struct obj *otmp, *marker, *contnr;
    register char ilet;
    char *invlet = flags.inv_order;
    int classcount, count, num_so_far;
    long cost, totcost;

    count = count_unpaid(invent);
    otmp = marker = contnr = (struct obj *) 0;

    if (count == 1) {
        otmp = find_unpaid(invent, &marker);
        contnr = unknwn_contnr_contents(otmp);
    }
    if  (otmp && !contnr) {
        /* 1 item; use pline instead of popup menu */
        cost = unpaid_cost(otmp, FALSE);
        iflags.suppress_price++; /* suppress "(unpaid)" suffix */
        pline1(xprname(otmp, distant_name(otmp, doname),
                       carried(otmp) ? otmp->invlet : CONTAINED_SYM,
                       TRUE, cost, 0L));
        iflags.suppress_price--;
        return;
    }

    win = create_nhwindow(NHW_MENU);
    cost = totcost = 0;
    num_so_far = 0; /* count of # printed so far */
    if (!flags.invlet_constant)
        reassign();

    do {
        classcount = 0;
        for (otmp = invent; otmp; otmp = otmp->nobj) {
            ilet = otmp->invlet;
            if (otmp->unpaid) {
                if (!flags.sortpack || otmp->oclass == *invlet) {
                    if (flags.sortpack && !classcount) {
                        putstr(win, 0, let_to_name(*invlet, TRUE, FALSE));
                        classcount++;
                    }

                    totcost += cost = unpaid_cost(otmp, FALSE);
                    iflags.suppress_price++; /* suppress "(unpaid)" suffix */
                    putstr(win, 0, xprname(otmp, distant_name(otmp, doname),
                                           ilet, TRUE, cost, 0L));
                    iflags.suppress_price--;
                    num_so_far++;
                }
            }
        }
    } while (flags.sortpack && (*++invlet));

    if (count > num_so_far) {
        /* something unpaid is contained */
        if (flags.sortpack)
            putstr(win, 0, let_to_name(CONTAINED_SYM, TRUE, FALSE));
        /*
         * Search through the container objects in the inventory for
         * unpaid items.  The top level inventory items have already
         * been listed.
         */
        for (otmp = invent; otmp; otmp = otmp->nobj) {
            if (Has_contents(otmp)) {
                long contcost = 0L;

                marker = (struct obj *) 0; /* haven't found any */
                while (find_unpaid(otmp->cobj, &marker)) {
                    totcost += cost = unpaid_cost(marker, FALSE);
                    contcost += cost;
                    if (otmp->cknown) {
                        iflags.suppress_price++; /* suppress "(unpaid)" sfx */
                        putstr(win, 0,
                               xprname(marker, distant_name(marker, doname),
                                       CONTAINED_SYM, TRUE, cost, 0L));
                        iflags.suppress_price--;
                    }
                }
                if (!otmp->cknown) {
                    char contbuf[BUFSZ];

                    /* Shopkeeper knows what to charge for contents */
                    Sprintf(contbuf, "%s contents", s_suffix(xname(otmp)));
                    putstr(win, 0,
                           xprname((struct obj *) 0, contbuf, CONTAINED_SYM,
                                   TRUE, contcost, 0L));
                }
            }
        }
    }

    putstr(win, 0, "");
    putstr(win, 0,
           xprname((struct obj *) 0, "Total:", '*', FALSE, totcost, 0L));
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
}

/* query objlist callback: return TRUE if obj type matches "this_type" */
static int this_type;

STATIC_OVL boolean
this_type_only(obj)
struct obj *obj;
{
    boolean res = (obj->oclass == this_type);

    if (obj->oclass == COIN_CLASS) {
        /* if filtering by bless/curse state, gold is classified as
           either unknown or uncursed based on user option setting */
        if (this_type && index("BUCX", this_type))
            res = (this_type == (iflags.goldX ? 'X' : 'U'));
    } else {
        switch (this_type) {
        case 'B':
            res = (obj->bknown && obj->blessed);
            break;
        case 'U':
            res = (obj->bknown && !(obj->blessed || obj->cursed));
            break;
        case 'C':
            res = (obj->bknown && obj->cursed);
            break;
        case 'X':
            res = !obj->bknown;
            break;
        default:
            break; /* use 'res' as-is */
        }
    }
    return res;
}

/* the 'I' command */
int
dotypeinv()
{
    char c = '\0';
    int n, i = 0;
    char *extra_types, types[BUFSZ];
    int class_count, oclass, unpaid_count, itemcount;
    int bcnt, ccnt, ucnt, xcnt, ocnt;
    boolean billx = *u.ushops && doinvbill(0);
    menu_item *pick_list;
    boolean traditional = TRUE;
    const char *prompt = "What type of object do you want an inventory of?";

    if (!invent && !billx) {
        You("aren't carrying anything.");
        return 0;
    }
    unpaid_count = count_unpaid(invent);
    tally_BUCX(invent, FALSE, &bcnt, &ucnt, &ccnt, &xcnt, &ocnt);

    if (flags.menu_style != MENU_TRADITIONAL) {
        if (flags.menu_style == MENU_FULL
            || flags.menu_style == MENU_PARTIAL) {
            traditional = FALSE;
            i = UNPAID_TYPES;
            if (billx)
                i |= BILLED_TYPES;
            if (bcnt)
                i |= BUC_BLESSED;
            if (ucnt)
                i |= BUC_UNCURSED;
            if (ccnt)
                i |= BUC_CURSED;
            if (xcnt)
                i |= BUC_UNKNOWN;
            n = query_category(prompt, invent, i, &pick_list, PICK_ONE);
            if (!n)
                return 0;
            this_type = c = pick_list[0].item.a_int;
            free((genericptr_t) pick_list);
        }
    }
    if (traditional) {
        /* collect a list of classes of objects carried, for use as a prompt
         */
        types[0] = 0;
        class_count = collect_obj_classes(types, invent, FALSE,
                                          (boolean FDECL((*), (OBJ_P))) 0,
                                          &itemcount);
        if (unpaid_count || billx || (bcnt + ccnt + ucnt + xcnt) != 0)
            types[class_count++] = ' ';
        if (unpaid_count)
            types[class_count++] = 'u';
        if (billx)
            types[class_count++] = 'x';
        if (bcnt)
            types[class_count++] = 'B';
        if (ucnt)
            types[class_count++] = 'U';
        if (ccnt)
            types[class_count++] = 'C';
        if (xcnt)
            types[class_count++] = 'X';
        types[class_count] = '\0';
        /* add everything not already included; user won't see these */
        extra_types = eos(types);
        *extra_types++ = '\033';
        if (!unpaid_count)
            *extra_types++ = 'u';
        if (!billx)
            *extra_types++ = 'x';
        if (!bcnt)
            *extra_types++ = 'B';
        if (!ucnt)
            *extra_types++ = 'U';
        if (!ccnt)
            *extra_types++ = 'C';
        if (!xcnt)
            *extra_types++ = 'X';
        *extra_types = '\0'; /* for index() */
        for (i = 0; i < MAXOCLASSES; i++)
            if (!index(types, def_oc_syms[i].sym)) {
                *extra_types++ = def_oc_syms[i].sym;
                *extra_types = '\0';
            }

        if (class_count > 1) {
            c = yn_function(prompt, types, '\0');
            savech(c);
            if (c == '\0') {
                clear_nhwindow(WIN_MESSAGE);
                return 0;
            }
        } else {
            /* only one thing to itemize */
            if (unpaid_count)
                c = 'u';
            else if (billx)
                c = 'x';
            else
                c = types[0];
        }
    }
    if (c == 'x' || (c == 'X' && billx && !xcnt)) {
        if (billx)
            (void) doinvbill(1);
        else
            pline("No used-up objects%s.",
                  unpaid_count ? " on your shopping bill" : "");
        return 0;
    }
    if (c == 'u' || (c == 'U' && unpaid_count && !ucnt)) {
        if (unpaid_count)
            dounpaid();
        else
            You("are not carrying any unpaid objects.");
        return 0;
    }
    if (traditional) {
        if (index("BUCX", c))
            oclass = c; /* not a class but understood by this_type_only() */
        else
            oclass = def_char_to_objclass(c); /* change to object class */

        if (oclass == COIN_CLASS)
            return doprgold();
        if (index(types, c) > index(types, '\033')) {
            /* '> ESC' => hidden choice, something known not to be carried */
            const char *before = "", *after = "";

            switch (c) {
            case 'B':
                before = "known to be blessed ";
                break;
            case 'U':
                before = "known to be uncursed ";
                break;
            case 'C':
                before = "known to be cursed ";
                break;
            case 'X':
                after = " whose blessed/uncursed/cursed status is unknown";
                break; /* better phrasing is desirable */
            default:
                /* 'c' is an object class, because we've already handled
                   all the non-class letters which were put into 'types[]';
                   could/should move object class names[] array from below
                   to somewhere above so that we can access it here (via
                   lcase(strcpy(classnamebuf, names[(int) c]))), but the
                   game-play value of doing so is low... */
                before = "such ";
                break;
            }
            You("have no %sobjects%s.", before, after);
            return 0;
        }
        this_type = oclass;
    }
    if (query_objlist((char *) 0, &invent,
                      ((flags.invlet_constant ? USE_INVLET : 0)
                       | INVORDER_SORT),
                      &pick_list, PICK_NONE, this_type_only) > 0)
        free((genericptr_t) pick_list);
    return 0;
}

/* return a string describing the dungeon feature at <x,y> if there
   is one worth mentioning at that location; otherwise null */
const char *
dfeature_at(x, y, buf)
int x, y;
char *buf;
{
    struct rm *lev = &levl[x][y];
    int ltyp = lev->typ, cmap = -1;
    const char *dfeature = 0;
    static char altbuf[BUFSZ];

    if (IS_DOOR(ltyp)) {
        switch (lev->doormask) {
        case D_NODOOR:
            cmap = S_ndoor;
            break; /* "doorway" */
        case D_ISOPEN:
            cmap = S_vodoor;
            break; /* "open door" */
        case D_BROKEN:
            dfeature = "broken door";
            break;
        default:
            cmap = S_vcdoor;
            break; /* "closed door" */
        }
        /* override door description for open drawbridge */
        if (is_drawbridge_wall(x, y) >= 0)
            dfeature = "open drawbridge portcullis", cmap = -1;
    } else if (IS_FOUNTAIN(ltyp))
        cmap = S_fountain; /* "fountain" */
    else if (IS_THRONE(ltyp))
        cmap = S_throne; /* "opulent throne" */
    else if (is_lava(x, y))
        cmap = S_lava; /* "molten lava" */
    else if (is_ice(x, y))
        cmap = S_ice; /* "ice" */
    else if (is_pool(x, y))
        dfeature = "pool of water";
    else if (IS_SINK(ltyp))
        cmap = S_sink; /* "sink" */
    else if (IS_ALTAR(ltyp)) {
        Sprintf(altbuf, "%saltar to %s (%s)",
                ((lev->altarmask & AM_SHRINE)
                 && (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)))
                    ? "high "
                    : "",
                a_gname(),
                align_str(Amask2align(lev->altarmask & ~AM_SHRINE)));
        dfeature = altbuf;
    } else if ((x == xupstair && y == yupstair)
               || (x == sstairs.sx && y == sstairs.sy && sstairs.up))
        cmap = S_upstair; /* "staircase up" */
    else if ((x == xdnstair && y == ydnstair)
             || (x == sstairs.sx && y == sstairs.sy && !sstairs.up))
        cmap = S_dnstair; /* "staircase down" */
    else if (x == xupladder && y == yupladder)
        cmap = S_upladder; /* "ladder up" */
    else if (x == xdnladder && y == ydnladder)
        cmap = S_dnladder; /* "ladder down" */
    else if (ltyp == DRAWBRIDGE_DOWN)
        cmap = S_vodbridge; /* "lowered drawbridge" */
    else if (ltyp == DBWALL)
        cmap = S_vcdbridge; /* "raised drawbridge" */
    else if (IS_GRAVE(ltyp))
        cmap = S_grave; /* "grave" */
    else if (ltyp == TREE)
        cmap = S_tree; /* "tree" */
    else if (ltyp == IRONBARS)
        dfeature = "set of iron bars";

    if (cmap >= 0)
        dfeature = defsyms[cmap].explanation;
    if (dfeature)
        Strcpy(buf, dfeature);
    return dfeature;
}

/* look at what is here; if there are many objects (pile_limit or more),
   don't show them unless obj_cnt is 0 */
int
look_here(obj_cnt, picked_some)
int obj_cnt; /* obj_cnt > 0 implies that autopickup is in progress */
boolean picked_some;
{
    struct obj *otmp;
    struct trap *trap;
    const char *verb = Blind ? "feel" : "see";
    const char *dfeature = (char *) 0;
    char fbuf[BUFSZ], fbuf2[BUFSZ];
    winid tmpwin;
    boolean skip_objects, felt_cockatrice = FALSE;

    /* default pile_limit is 5; a value of 0 means "never skip"
       (and 1 effectively forces "always skip") */
    skip_objects = (flags.pile_limit > 0 && obj_cnt >= flags.pile_limit);
    if (u.uswallow && u.ustuck) {
        struct monst *mtmp = u.ustuck;

        Sprintf(fbuf, "Contents of %s %s", s_suffix(mon_nam(mtmp)),
                mbodypart(mtmp, STOMACH));
        /* Skip "Contents of " by using fbuf index 12 */
        You("%s to %s what is lying in %s.", Blind ? "try" : "look around",
            verb, &fbuf[12]);
        otmp = mtmp->minvent;
        if (otmp) {
            for (; otmp; otmp = otmp->nobj) {
                /* If swallower is an animal, it should have become stone
                 * but... */
                if (otmp->otyp == CORPSE)
                    feel_cockatrice(otmp, FALSE);
            }
            if (Blind)
                Strcpy(fbuf, "You feel");
            Strcat(fbuf, ":");
            (void) display_minventory(mtmp, MINV_ALL | PICK_NONE, fbuf);
        } else {
            You("%s no objects here.", verb);
        }
        return !!Blind;
    }
    if (!skip_objects && (trap = t_at(u.ux, u.uy)) && trap->tseen)
        There("is %s here.",
              an(defsyms[trap_to_defsym(trap->ttyp)].explanation));

    otmp = level.objects[u.ux][u.uy];
    dfeature = dfeature_at(u.ux, u.uy, fbuf2);
    if (dfeature && !strcmp(dfeature, "pool of water") && Underwater)
        dfeature = 0;

    if (Blind) {
        boolean drift = Is_airlevel(&u.uz) || Is_waterlevel(&u.uz);

        if (dfeature && !strncmp(dfeature, "altar ", 6)) {
            /* don't say "altar" twice, dfeature has more info */
            You("try to feel what is here.");
        } else {
            const char *where = (Blind && !can_reach_floor(TRUE))
                                    ? "lying beneath you"
                                    : "lying here on the ",
                       *onwhat = (Blind && !can_reach_floor(TRUE))
                                     ? ""
                                     : surface(u.ux, u.uy);

            You("try to feel what is %s%s.", drift ? "floating here" : where,
                drift ? "" : onwhat);
        }
        if (dfeature && !drift && !strcmp(dfeature, surface(u.ux, u.uy)))
            dfeature = 0; /* ice already identified */
        if (!can_reach_floor(TRUE)) {
            pline("But you can't reach it!");
            return 0;
        }
    }

    if (dfeature)
        Sprintf(fbuf, "There is %s here.", an(dfeature));

    if (!otmp || is_lava(u.ux, u.uy)
        || (is_pool(u.ux, u.uy) && !Underwater)) {
        if (dfeature)
            pline1(fbuf);
        read_engr_at(u.ux, u.uy); /* Eric Backus */
        if (!skip_objects && (Blind || !dfeature))
            You("%s no objects here.", verb);
        return !!Blind;
    }
    /* we know there is something here */

    if (skip_objects) {
        if (dfeature)
            pline1(fbuf);
        read_engr_at(u.ux, u.uy); /* Eric Backus */
        if (obj_cnt == 1 && otmp->quan == 1L)
            There("is %s object here.", picked_some ? "another" : "an");
        else
            There("are %s%s objects here.",
                  (obj_cnt < 5)
                      ? "a few"
                      : (obj_cnt < 10)
                          ? "several"
                          : "many",
                  picked_some ? " more" : "");
        for (; otmp; otmp = otmp->nexthere)
            if (otmp->otyp == CORPSE && will_feel_cockatrice(otmp, FALSE)) {
                pline("%s %s%s.",
                      (obj_cnt > 1)
                          ? "Including"
                          : (otmp->quan > 1L)
                              ? "They're"
                              : "It's",
                      corpse_xname(otmp, (const char *) 0, CXN_ARTICLE),
                      poly_when_stoned(youmonst.data)
                          ? ""
                          : ", unfortunately");
                feel_cockatrice(otmp, FALSE);
                break;
            }
    } else if (!otmp->nexthere) {
        /* only one object */
        if (dfeature)
            pline1(fbuf);
        read_engr_at(u.ux, u.uy); /* Eric Backus */
        You("%s here %s.", verb, doname_with_price(otmp));
        iflags.last_msg = PLNMSG_ONE_ITEM_HERE;
        if (otmp->otyp == CORPSE)
            feel_cockatrice(otmp, FALSE);
    } else {
        char buf[BUFSZ];

        display_nhwindow(WIN_MESSAGE, FALSE);
        tmpwin = create_nhwindow(NHW_MENU);
        if (dfeature) {
            putstr(tmpwin, 0, fbuf);
            putstr(tmpwin, 0, "");
        }
        Sprintf(buf, "%s that %s here:",
                picked_some ? "Other things" : "Things",
                Blind ? "you feel" : "are");
        putstr(tmpwin, 0, buf);
        for (; otmp; otmp = otmp->nexthere) {
            if (otmp->otyp == CORPSE && will_feel_cockatrice(otmp, FALSE)) {
                felt_cockatrice = TRUE;
                Sprintf(buf, "%s...", doname(otmp));
                putstr(tmpwin, 0, buf);
                break;
            }
            putstr(tmpwin, 0, doname_with_price(otmp));
        }
        display_nhwindow(tmpwin, TRUE);
        destroy_nhwindow(tmpwin);
        if (felt_cockatrice)
            feel_cockatrice(otmp, FALSE);
        read_engr_at(u.ux, u.uy); /* Eric Backus */
    }
    return !!Blind;
}

/* the ':' command - explicitly look at what is here, including all objects */
int
dolook()
{
    int res;

    /* don't let
       MSGTYPE={norep,noshow} "You see here"
       interfere with feedback from the look-here command */
    hide_unhide_msgtypes(TRUE, MSGTYP_MASK_REP_SHOW);
    res = look_here(0, FALSE);
    /* restore normal msgtype handling */
    hide_unhide_msgtypes(FALSE, MSGTYP_MASK_REP_SHOW);
    return res;
}

boolean
will_feel_cockatrice(otmp, force_touch)
struct obj *otmp;
boolean force_touch;
{
    if ((Blind || force_touch) && !uarmg && !Stone_resistance
        && (otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm])))
        return TRUE;
    return FALSE;
}

void
feel_cockatrice(otmp, force_touch)
struct obj *otmp;
boolean force_touch;
{
    char kbuf[BUFSZ];

    if (will_feel_cockatrice(otmp, force_touch)) {
        /* "the <cockatrice> corpse" */
        Strcpy(kbuf, corpse_xname(otmp, (const char *) 0, CXN_PFX_THE));

        if (poly_when_stoned(youmonst.data))
            You("touched %s with your bare %s.", kbuf,
                makeplural(body_part(HAND)));
        else
            pline("Touching %s is a fatal mistake...", kbuf);
        /* normalize body shape here; hand, not body_part(HAND) */
        Sprintf(kbuf, "touching %s bare-handed", killer_xname(otmp));
        /* will call polymon() for the poly_when_stoned() case */
        instapetrify(kbuf);
    }
}

void
stackobj(obj)
struct obj *obj;
{
    struct obj *otmp;

    for (otmp = level.objects[obj->ox][obj->oy]; otmp; otmp = otmp->nexthere)
        if (otmp != obj && merged(&obj, &otmp))
            break;
    return;
}

/* returns TRUE if obj & otmp can be merged; used in invent.c and mkobj.c */
boolean
mergable(otmp, obj)
register struct obj *otmp, *obj;
{
    int objnamelth = 0, otmpnamelth = 0;

    if (obj == otmp)
        return FALSE; /* already the same object */
    if (obj->otyp != otmp->otyp)
        return FALSE; /* different types */
    if (obj->nomerge) /* explicitly marked to prevent merge */
        return FALSE;

    /* coins of the same kind will always merge */
    if (obj->oclass == COIN_CLASS)
        return TRUE;

    if (obj->unpaid != otmp->unpaid || obj->spe != otmp->spe
        || obj->cursed != otmp->cursed || obj->blessed != otmp->blessed
        || obj->no_charge != otmp->no_charge || obj->obroken != otmp->obroken
        || obj->otrapped != otmp->otrapped || obj->lamplit != otmp->lamplit
        || obj->bypass != otmp->bypass)
        return FALSE;

    if (obj->globby)
        return TRUE;
    /* Checks beyond this point either aren't applicable to globs
     * or don't inhibit their merger.
     */

    if (obj->oclass == FOOD_CLASS
        && (obj->oeaten != otmp->oeaten || obj->orotten != otmp->orotten))
        return FALSE;

    if (obj->dknown != otmp->dknown
        || (obj->bknown != otmp->bknown && !Role_if(PM_PRIEST))
        || obj->oeroded != otmp->oeroded || obj->oeroded2 != otmp->oeroded2
        || obj->greased != otmp->greased)
        return FALSE;

    if ((obj->oclass == WEAPON_CLASS || obj->oclass == ARMOR_CLASS)
        && (obj->oerodeproof != otmp->oerodeproof
            || obj->rknown != otmp->rknown))
        return FALSE;

    if (obj->otyp == CORPSE || obj->otyp == EGG || obj->otyp == TIN) {
        if (obj->corpsenm != otmp->corpsenm)
            return FALSE;
    }

    /* hatching eggs don't merge; ditto for revivable corpses */
    if ((obj->otyp == EGG && (obj->timed || otmp->timed))
        || (obj->otyp == CORPSE && otmp->corpsenm >= LOW_PM
            && is_reviver(&mons[otmp->corpsenm])))
        return FALSE;

    /* allow candle merging only if their ages are close */
    /* see begin_burn() for a reference for the magic "25" */
    if (Is_candle(obj) && obj->age / 25 != otmp->age / 25)
        return FALSE;

    /* burning potions of oil never merge */
    if (obj->otyp == POT_OIL && obj->lamplit)
        return FALSE;

    /* don't merge surcharged item with base-cost item */
    if (obj->unpaid && !same_price(obj, otmp))
        return FALSE;

    /* if they have names, make sure they're the same */
    objnamelth = strlen(safe_oname(obj));
    otmpnamelth = strlen(safe_oname(otmp));
    if ((objnamelth != otmpnamelth
         && ((objnamelth && otmpnamelth) || obj->otyp == CORPSE))
        || (objnamelth && otmpnamelth
            && strncmp(ONAME(obj), ONAME(otmp), objnamelth)))
        return FALSE;

    /* for the moment, any additional information is incompatible */
    if (has_omonst(obj) || has_omid(obj) || has_olong(obj) || has_omonst(otmp)
        || has_omid(otmp) || has_olong(otmp))
        return FALSE;

    if (obj->oartifact != otmp->oartifact)
        return FALSE;

    if (obj->known == otmp->known || !objects[otmp->otyp].oc_uses_known) {
        return (boolean) objects[obj->otyp].oc_merge;
    } else
        return FALSE;
}

/* the '$' command */
int
doprgold()
{
    /* the messages used to refer to "carrying gold", but that didn't
       take containers into account */
    long umoney = money_cnt(invent);
    if (!umoney)
        Your("wallet is empty.");
    else
        Your("wallet contains %ld %s.", umoney, currency(umoney));
    shopper_financial_report();
    return 0;
}

/* the ')' command */
int
doprwep()
{
    if (!uwep) {
        You("are empty %s.", body_part(HANDED));
    } else {
        prinv((char *) 0, uwep, 0L);
        if (u.twoweap)
            prinv((char *) 0, uswapwep, 0L);
    }
    return 0;
}

/* caller is responsible for checking !wearing_armor() */
STATIC_OVL void
noarmor(report_uskin)
boolean report_uskin;
{
    if (!uskin || !report_uskin) {
        You("are not wearing any armor.");
    } else {
        char *p, *uskinname, buf[BUFSZ];

        uskinname = strcpy(buf, simpleonames(uskin));
        /* shorten "set of <color> dragon scales" to "<color> scales"
           and "<color> dragon scale mail" to "<color> scale mail" */
        if (!strncmpi(uskinname, "set of ", 7))
            uskinname += 7;
        if ((p = strstri(uskinname, " dragon ")) != 0)
            while ((p[1] = p[8]) != '\0')
                ++p;

        You("are not wearing armor but have %s embedded in your skin.",
            uskinname);
    }
}

/* the '[' command */
int
doprarm()
{
    char lets[8];
    register int ct = 0;
    /*
     * Note:  players sometimes get here by pressing a function key which
     * transmits ''ESC [ <something>'' rather than by pressing '[';
     * there's nothing we can--or should-do about that here.
     */

    if (!wearing_armor()) {
        noarmor(TRUE);
    } else {
        if (uarmu)
            lets[ct++] = obj_to_let(uarmu);
        if (uarm)
            lets[ct++] = obj_to_let(uarm);
        if (uarmc)
            lets[ct++] = obj_to_let(uarmc);
        if (uarmh)
            lets[ct++] = obj_to_let(uarmh);
        if (uarms)
            lets[ct++] = obj_to_let(uarms);
        if (uarmg)
            lets[ct++] = obj_to_let(uarmg);
        if (uarmf)
            lets[ct++] = obj_to_let(uarmf);
        lets[ct] = 0;
        (void) display_inventory(lets, FALSE);
    }
    return 0;
}

/* the '=' command */
int
doprring()
{
    if (!uleft && !uright)
        You("are not wearing any rings.");
    else {
        char lets[3];
        register int ct = 0;

        if (uleft)
            lets[ct++] = obj_to_let(uleft);
        if (uright)
            lets[ct++] = obj_to_let(uright);
        lets[ct] = 0;
        (void) display_inventory(lets, FALSE);
    }
    return 0;
}

/* the '"' command */
int
dopramulet()
{
    if (!uamul)
        You("are not wearing an amulet.");
    else
        prinv((char *) 0, uamul, 0L);
    return 0;
}

STATIC_OVL boolean
tool_in_use(obj)
struct obj *obj;
{
    if ((obj->owornmask & (W_TOOL | W_SADDLE)) != 0L)
        return TRUE;
    if (obj->oclass != TOOL_CLASS)
        return FALSE;
    return (boolean) (obj == uwep || obj->lamplit
                      || (obj->otyp == LEASH && obj->leashmon));
}

/* the '(' command */
int
doprtool()
{
    struct obj *otmp;
    int ct = 0;
    char lets[52 + 1];

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (tool_in_use(otmp))
            lets[ct++] = obj_to_let(otmp);
    lets[ct] = '\0';
    if (!ct)
        You("are not using any tools.");
    else
        (void) display_inventory(lets, FALSE);
    return 0;
}

/* '*' command; combines the ')' + '[' + '=' + '"' + '(' commands;
   show inventory of all currently wielded, worn, or used objects */
int
doprinuse()
{
    struct obj *otmp;
    int ct = 0;
    char lets[52 + 1];

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (is_worn(otmp) || tool_in_use(otmp))
            lets[ct++] = obj_to_let(otmp);
    lets[ct] = '\0';
    if (!ct)
        You("are not wearing or wielding anything.");
    else
        (void) display_inventory(lets, FALSE);
    return 0;
}

/*
 * uses up an object that's on the floor, charging for it as necessary
 */
void
useupf(obj, numused)
register struct obj *obj;
long numused;
{
    register struct obj *otmp;
    boolean at_u = (obj->ox == u.ux && obj->oy == u.uy);

    /* burn_floor_objects() keeps an object pointer that it tries to
     * useupf() multiple times, so obj must survive if plural */
    if (obj->quan > numused)
        otmp = splitobj(obj, numused);
    else
        otmp = obj;
    if (costly_spot(otmp->ox, otmp->oy)) {
        if (index(u.urooms, *in_rooms(otmp->ox, otmp->oy, 0)))
            addtobill(otmp, FALSE, FALSE, FALSE);
        else
            (void) stolen_value(otmp, otmp->ox, otmp->oy, FALSE, FALSE);
    }
    delobj(otmp);
    if (at_u && u.uundetected && hides_under(youmonst.data))
        (void) hideunder(&youmonst);
}

/*
 * Conversion from a class to a string for printing.
 * This must match the object class order.
 */
STATIC_VAR NEARDATA const char *names[] = {
    0, "Illegal objects", "Weapons", "Armor", "Rings", "Amulets", "Tools",
    "Comestibles", "Potions", "Scrolls", "Spellbooks", "Wands", "Coins",
    "Gems/Stones", "Boulders/Statues", "Iron balls", "Chains", "Venoms"
};
STATIC_VAR NEARDATA const char oth_symbols[] = { CONTAINED_SYM, '\0' };
STATIC_VAR NEARDATA const char *oth_names[] = { "Bagged/Boxed items" };

STATIC_VAR NEARDATA char *invbuf = (char *) 0;
STATIC_VAR NEARDATA unsigned invbufsiz = 0;

char *
let_to_name(let, unpaid, showsym)
char let;
boolean unpaid, showsym;
{
    const char *ocsymfmt = "  ('%c')";
    const int invbuf_sympadding = 8; /* arbitrary */
    const char *class_name;
    const char *pos;
    int oclass = (let >= 1 && let < MAXOCLASSES) ? let : 0;
    unsigned len;

    if (oclass)
        class_name = names[oclass];
    else if ((pos = index(oth_symbols, let)) != 0)
        class_name = oth_names[pos - oth_symbols];
    else
        class_name = names[0];

    len = strlen(class_name) + (unpaid ? sizeof "unpaid_" : sizeof "")
          + (oclass ? (strlen(ocsymfmt) + invbuf_sympadding) : 0);
    if (len > invbufsiz) {
        if (invbuf)
            free((genericptr_t) invbuf);
        invbufsiz = len + 10; /* add slop to reduce incremental realloc */
        invbuf = (char *) alloc(invbufsiz);
    }
    if (unpaid)
        Strcat(strcpy(invbuf, "Unpaid "), class_name);
    else
        Strcpy(invbuf, class_name);
    if ((oclass != 0) && showsym) {
        char *bp = eos(invbuf);
        int mlen = invbuf_sympadding - strlen(class_name);
        while (--mlen > 0) {
            *bp = ' ';
            bp++;
        }
        *bp = '\0';
        Sprintf(eos(invbuf), ocsymfmt, def_oc_syms[oclass].sym);
    }
    return invbuf;
}

/* release the static buffer used by let_to_name() */
void
free_invbuf()
{
    if (invbuf)
        free((genericptr_t) invbuf), invbuf = (char *) 0;
    invbufsiz = 0;
}

/* give consecutive letters to every item in inventory (for !fixinv mode);
   gold is always forced to '$' slot at head of list */
void
reassign()
{
    int i;
    struct obj *obj, *prevobj, *goldobj;

    /* first, remove [first instance of] gold from invent, if present */
    prevobj = goldobj = 0;
    for (obj = invent; obj; prevobj = obj, obj = obj->nobj)
        if (obj->oclass == COIN_CLASS) {
            goldobj = obj;
            if (prevobj)
                prevobj->nobj = goldobj->nobj;
            else
                invent = goldobj->nobj;
            break;
        }
    /* second, re-letter the rest of the list */
    for (obj = invent, i = 0; obj; obj = obj->nobj, i++)
        obj->invlet =
            (i < 26) ? ('a' + i) : (i < 52) ? ('A' + i - 26) : NOINVSYM;
    /* third, assign gold the "letter" '$' and re-insert it at head */
    if (goldobj) {
        goldobj->invlet = GOLD_SYM;
        goldobj->nobj = invent;
        invent = goldobj;
    }
    if (i >= 52)
        i = 52 - 1;
    lastinvnr = i;
}

/* #adjust command
 *
 *      User specifies a 'from' slot for inventory stack to move,
 *      then a 'to' slot for its destination.  Open slots and those
 *      filled by compatible stacks are listed as likely candidates
 *      but user can pick any inventory letter (including 'from').
 *
 *  to == from, 'from' has a name
 *      All compatible items (same name or no name) are gathered
 *      into the 'from' stack.  No count is allowed.
 *  to == from, 'from' does not have a name
 *      All compatible items without a name are gathered into the
 *      'from' stack.  No count is allowed.  Compatible stacks with
 *      names are left as-is.
 *  to != from, no count
 *      Move 'from' to 'to'.  If 'to' is not empty, merge 'from'
 *      into it if possible, otherwise swap it with the 'from' slot.
 *  to != from, count given
 *      If the user specifies a count when choosing the 'from' slot,
 *      and that count is less than the full size of the stack,
 *      then the stack will be split.  The 'count' portion is moved
 *      to the destination, and the only candidate for merging with
 *      it is the stack already at the 'to' slot, if any.  When the
 *      destination is non-empty but won't merge, whatever is there
 *      will be moved to an open slot; if there isn't any open slot
 *      available, the adjustment attempt fails.
 *
 *      To minimize merging for 'from == to', unnamed stacks will
 *      merge with named 'from' but named ones won't merge with
 *      unnamed 'from'.  Otherwise attempting to collect all unnamed
 *      stacks would lump the first compatible named stack with them
 *      and give them its name.
 *
 *      To maximize merging for 'from != to', compatible stacks will
 *      merge when either lacks a name (or they already have the same
 *      name).  When no count is given and one stack has a name and
 *      the other doesn't, the merged result will have that name.
 *      However, when splitting results in a merger, the name of the
 *      destination overrides that of the source, even if destination
 *      is unnamed and source is named.
 */
int
doorganize() /* inventory organizer by Del Lamb */
{
    struct obj *obj, *otmp, *splitting, *bumped;
    int ix, cur, trycnt, goldstacks;
    char let;
#define GOLD_INDX   0
#define GOLD_OFFSET 1
#define OVRFLW_INDX (GOLD_OFFSET + 52) /* past gold and 2*26 letters */
    char lets[1 + 52 + 1 + 1]; /* room for '$a-zA-Z#\0' */
    char qbuf[QBUFSZ];
    char allowall[4]; /* { ALLOW_COUNT, ALL_CLASSES, 0, 0 } */
    char *objname, *otmpname;
    const char *adj_type;
    boolean ever_mind = FALSE, collect;

    if (!invent) {
        You("aren't carrying anything to adjust.");
        return 0;
    }

    if (!flags.invlet_constant)
        reassign();
    /* get object the user wants to organize (the 'from' slot) */
    allowall[0] = ALLOW_COUNT;
    allowall[1] = ALL_CLASSES;
    allowall[2] = '\0';
    for (goldstacks = 0, otmp = invent; otmp; otmp = otmp->nobj) {
        /* gold should never end up in a letter slot, nor should two '$'
           slots occur, but if they ever do, allow #adjust to handle them
           (in the past, things like this have happened, usually due to
           bknown being erroneously set on one stack, clear on another;
           object merger isn't fooled by that anymore) */
        if (otmp->oclass == COIN_CLASS
            && (otmp->invlet != GOLD_SYM || ++goldstacks > 1)) {
            allowall[1] = COIN_CLASS;
            allowall[2] = ALL_CLASSES;
            allowall[3] = '\0';
            break;
        }
    }
    if (!(obj = getobj(allowall, "adjust")))
        return 0;

    /* figure out whether user gave a split count to getobj() */
    splitting = bumped = 0;
    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->nobj == obj) { /* knowledge of splitobj() operation */
            if (otmp->invlet == obj->invlet)
                splitting = otmp;
            break;
        }

    /* initialize the list with all lower and upper case letters */
    lets[GOLD_INDX] = (obj->oclass == COIN_CLASS) ? GOLD_SYM : ' ';
    for (ix = GOLD_OFFSET, let = 'a'; let <= 'z';)
        lets[ix++] = let++;
    for (let = 'A'; let <= 'Z';)
        lets[ix++] = let++;
    lets[OVRFLW_INDX] = ' ';
    lets[sizeof lets - 1] = '\0';
    /* for floating inv letters, truncate list after the first open slot */
    if (!flags.invlet_constant && (ix = inv_cnt(FALSE)) < 52)
        lets[ix + (splitting ? 0 : 1)] = '\0';

    /* blank out all the letters currently in use in the inventory
       except those that will be merged with the selected object */
    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp != obj && !mergable(otmp, obj)) {
            let = otmp->invlet;
            if (let >= 'a' && let <= 'z')
                lets[GOLD_OFFSET + let - 'a'] = ' ';
            else if (let >= 'A' && let <= 'Z')
                lets[GOLD_OFFSET + let - 'A' + 26] = ' ';
            /* overflow defaults to off, but it we find a stack using that
               slot, switch to on -- the opposite of normal invlet handling */
            else if (let == NOINVSYM)
                lets[OVRFLW_INDX] = NOINVSYM;
        }

    /* compact the list by removing all the blanks */
    for (ix = cur = 0; lets[ix]; ix++)
        if (lets[ix] != ' ' && cur++ < ix)
            lets[cur - 1] = lets[ix];
    lets[cur] = '\0';
    /* and by dashing runs of letters */
    if (cur > 5)
        compactify(lets);

    /* get 'to' slot to use as destination */
    Sprintf(qbuf, "Adjust letter to what [%s]%s?", lets,
            invent ? " (? see used letters)" : "");
    for (trycnt = 1; ; ++trycnt) {
        let = yn_function(qbuf, (char *) 0, '\0');
        if (let == '?' || let == '*') {
            let = display_used_invlets(splitting ? obj->invlet : 0);
            if (!let)
                continue;
            if (let == '\033')
                goto noadjust;
        }
        if (index(quitchars, let)
            /* adjusting to same slot is meaningful since all
               compatible stacks get collected along the way,
               but splitting to same slot is not */
            || (splitting && let == obj->invlet)) {
        noadjust:
            if (splitting)
                (void) merged(&splitting, &obj);
            if (!ever_mind)
                pline1(Never_mind);
            return 0;
        } else if (let == GOLD_SYM && obj->oclass != COIN_CLASS) {
            pline("Only gold coins may be moved into the '%c' slot.",
                  GOLD_SYM);
            ever_mind = TRUE;
            goto noadjust;
        }
        /* letter() classifies '@' as one; compactify() can put '-' in lets;
           the only thing of interest that index() might find is '$' or '#'
           since letter() catches everything else that we put into lets[] */
        if ((letter(let) && let != '@') || (index(lets, let) && let != '-'))
            break; /* got one */
        if (trycnt == 5)
            goto noadjust;
        pline("Select an inventory slot letter."); /* else try again */
    }

    collect = (let == obj->invlet);
    /* change the inventory and print the resulting item */
    adj_type = collect ? "Collecting" : !splitting ? "Moving:" : "Splitting:";

    /*
     * don't use freeinv/addinv to avoid double-touching artifacts,
     * dousing lamps, losing luck, cursing loadstone, etc.
     */
    extract_nobj(obj, &invent);

    for (otmp = invent; otmp;) {
        /* it's tempting to pull this outside the loop, but merged() could
           free ONAME(obj) [via obfree()] and replace it with ONAME(otmp) */
        objname = has_oname(obj) ? ONAME(obj) : (char *) 0;

        if (collect) {
            /* Collecting: #adjust an inventory stack into its same slot;
               keep it there and merge other compatible stacks into it.
               Traditional inventory behavior is to merge unnamed stacks
               with compatible named ones; we only want that if it is
               the 'from' stack (obj) with a name and candidate (otmp)
               without one, not unnamed 'from' with named candidate. */
            otmpname = has_oname(otmp) ? ONAME(otmp) : (char *) 0;
            if ((!otmpname || (objname && !strcmp(objname, otmpname)))
                && merged(&otmp, &obj)) {
                adj_type = "Merging:";
                obj = otmp;
                otmp = otmp->nobj;
                extract_nobj(obj, &invent);
                continue; /* otmp has already been updated */
            }
        } else if (otmp->invlet == let) {
            /* Moving or splitting: don't merge extra compatible stacks.
               Found 'otmp' in destination slot; merge if compatible,
               otherwise bump whatever is there to an open slot. */
            if (!splitting) {
                adj_type = "Swapping:";
                otmp->invlet = obj->invlet;
            } else {
                /* strip 'from' name if it has one */
                if (objname && !obj->oartifact)
                    ONAME(obj) = (char *) 0;
                if (!mergable(otmp, obj)) {
                    /* won't merge; put 'from' name back */
                    if (objname)
                        ONAME(obj) = objname;
                } else {
                    /* will merge; discard 'from' name */
                    if (objname)
                        free((genericptr_t) objname), objname = 0;
                }

                if (merged(&otmp, &obj)) {
                    adj_type = "Splitting and merging:";
                    obj = otmp;
                    extract_nobj(obj, &invent);
                } else if (inv_cnt(FALSE) >= 52) {
                    (void) merged(&splitting, &obj); /* undo split */
                    /* "knapsack cannot accommodate any more items" */
                    Your("pack is too full.");
                    return 0;
                } else {
                    bumped = otmp;
                    extract_nobj(bumped, &invent);
                }
            } /* moving vs splitting */
            break; /* not collecting and found 'to' slot */
        } /* collect */
        otmp = otmp->nobj;
    }

    /* inline addinv; insert loose object at beginning of inventory */
    obj->invlet = let;
    obj->nobj = invent;
    obj->where = OBJ_INVENT;
    invent = obj;
    reorder_invent();
    if (bumped) {
        /* splitting the 'from' stack is causing an incompatible
           stack in the 'to' slot to be moved into an open one;
           we need to do another inline insertion to inventory */
        assigninvlet(bumped);
        bumped->nobj = invent;
        bumped->where = OBJ_INVENT;
        invent = bumped;
        reorder_invent();
    }

    /* messages deferred until inventory has been fully reestablished */
    prinv(adj_type, obj, 0L);
    if (bumped)
        prinv("Moving:", bumped, 0L);
    if (splitting)
        clear_splitobjs(); /* reset splitobj context */
    update_inventory();
    return 0;
}

/* common to display_minventory and display_cinventory */
STATIC_OVL void
invdisp_nothing(hdr, txt)
const char *hdr, *txt;
{
    winid win;
    anything any;
    menu_item *selected;

    any = zeroany;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings, hdr,
             MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, txt, MENU_UNSELECTED);
    end_menu(win, (char *) 0);
    if (select_menu(win, PICK_NONE, &selected) > 0)
        free((genericptr_t) selected);
    destroy_nhwindow(win);
    return;
}

/* query_objlist callback: return things that are worn or wielded */
STATIC_OVL boolean
worn_wield_only(obj)
struct obj *obj;
{
#if 1
    /* check for things that *are* worn or wielded (only used for monsters,
       so we don't worry about excluding W_CHAIN, W_ARTI and the like) */
    return (boolean) (obj->owornmask != 0L);
#else
    /* this used to check for things that *might* be worn or wielded,
       but that's not particularly interesting */
    if (is_weptool(obj) || is_wet_towel(obj) || obj->otyp == MEAT_RING)
        return TRUE;
    return (boolean) (obj->oclass == WEAPON_CLASS
                      || obj->oclass == ARMOR_CLASS
                      || obj->oclass == AMULET_CLASS
                      || obj->oclass == RING_CLASS);
#endif
}

/*
 * Display a monster's inventory.
 * Returns a pointer to the object from the monster's inventory selected
 * or NULL if nothing was selected.
 *
 * By default, only worn and wielded items are displayed.  The caller
 * can pick one.  Modifier flags are:
 *
 *      PICK_NONE, PICK_ONE - standard menu control
 *      PICK_ANY            - allowed, but we only return a single object
 *      MINV_NOLET          - nothing selectable
 *      MINV_ALL            - display all inventory
 */
struct obj *
display_minventory(mon, dflags, title)
register struct monst *mon;
int dflags;
char *title;
{
    struct obj *ret;
    char tmp[QBUFSZ];
    int n;
    menu_item *selected = 0;
    int do_all = (dflags & MINV_ALL) != 0,
        incl_hero = (do_all && u.uswallow && mon == u.ustuck),
        have_inv = (mon->minvent != 0), have_any = (have_inv || incl_hero),
        pickings = (dflags & MINV_PICKMASK);

    Sprintf(tmp, "%s %s:", s_suffix(noit_Monnam(mon)),
            do_all ? "possessions" : "armament");

    if (do_all ? have_any : (mon->misc_worn_check || MON_WEP(mon))) {
        /* Fool the 'weapon in hand' routine into
         * displaying 'weapon in claw', etc. properly.
         */
        youmonst.data = mon->data;

        n = query_objlist(title ? title : tmp, &(mon->minvent),
                          (INVORDER_SORT | (incl_hero ? INCLUDE_HERO : 0)),
                          &selected, pickings,
                          do_all ? allow_all : worn_wield_only);
        set_uasmon();
    } else {
        invdisp_nothing(title ? title : tmp, "(none)");
        n = 0;
    }

    if (n > 0) {
        ret = selected[0].item.a_obj;
        free((genericptr_t) selected);
    } else
        ret = (struct obj *) 0;
    return ret;
}

/*
 * Display the contents of a container in inventory style.
 * Currently, this is only used for statues, via wand of probing.
 */
struct obj *
display_cinventory(obj)
register struct obj *obj;
{
    struct obj *ret;
    char qbuf[QBUFSZ];
    int n;
    menu_item *selected = 0;

    (void) safe_qbuf(qbuf, "Contents of ", ":", obj, doname, ansimpleoname,
                     "that");

    if (obj->cobj) {
        n = query_objlist(qbuf, &(obj->cobj), INVORDER_SORT,
                          &selected, PICK_NONE, allow_all);
    } else {
        invdisp_nothing(qbuf, "(empty)");
        n = 0;
    }
    if (n > 0) {
        ret = selected[0].item.a_obj;
        free((genericptr_t) selected);
    } else
        ret = (struct obj *) 0;
    obj->cknown = 1;
    return ret;
}

/* query objlist callback: return TRUE if obj is at given location */
static coord only;

STATIC_OVL boolean
only_here(obj)
struct obj *obj;
{
    return (obj->ox == only.x && obj->oy == only.y);
}

/*
 * Display a list of buried items in inventory style.  Return a non-zero
 * value if there were items at that spot.
 *
 * Currently, this is only used with a wand of probing zapped downwards.
 */
int
display_binventory(x, y, as_if_seen)
int x, y;
boolean as_if_seen;
{
    struct obj *obj;
    menu_item *selected = 0;
    int n;

    /* count # of objects here */
    for (n = 0, obj = level.buriedobjlist; obj; obj = obj->nobj)
        if (obj->ox == x && obj->oy == y) {
            if (as_if_seen)
                obj->dknown = 1;
            n++;
        }

    if (n) {
        only.x = x;
        only.y = y;
        if (query_objlist("Things that are buried here:",
                          &level.buriedobjlist, INVORDER_SORT,
                          &selected, PICK_NONE, only_here) > 0)
            free((genericptr_t) selected);
        only.x = only.y = 0;
    }
    return n;
}

/*invent.c*/

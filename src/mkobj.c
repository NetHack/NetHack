/* NetHack 3.7	mkobj.c	$NHDT-Date: 1654881236 2022/06/10 17:13:56 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.237 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static void mkbox_cnts(struct obj *);
static unsigned nextoid(struct obj *, struct obj *);
static int item_on_ice(struct obj *);
static void shrinking_glob_gone(struct obj *);
static void obj_timer_checks(struct obj *, coordxy, coordxy, int);
static void container_weight(struct obj *);
static struct obj *save_mtraits(struct obj *, struct monst *);
static void objlist_sanity(struct obj *, int, const char *);
static void shop_obj_sanity(struct obj *, const char *);
static void mon_obj_sanity(struct monst *, const char *);
static void insane_obj_bits(struct obj *, struct monst *);
static boolean nomerge_exception(struct obj *);
static const char *where_name(struct obj *);
static void insane_object(struct obj *, const char *, const char *,
                          struct monst *);
static void check_contained(struct obj *, const char *);
static void check_glob(struct obj *, const char *);
static void sanity_check_worn(struct obj *);

struct icp {
    int iprob;   /* probability of an item type */
    char iclass; /* item class */
};

static const struct icp mkobjprobs[] = { { 10, WEAPON_CLASS },
                                         { 10, ARMOR_CLASS },
                                         { 20, FOOD_CLASS },
                                         { 8, TOOL_CLASS },
                                         { 8, GEM_CLASS },
                                         { 16, POTION_CLASS },
                                         { 16, SCROLL_CLASS },
                                         { 4, SPBOOK_CLASS },
                                         { 4, WAND_CLASS },
                                         { 3, RING_CLASS },
                                         { 1, AMULET_CLASS } };

static const struct icp boxiprobs[] = { { 18, GEM_CLASS },
                                        { 15, FOOD_CLASS },
                                        { 18, POTION_CLASS },
                                        { 18, SCROLL_CLASS },
                                        { 12, SPBOOK_CLASS },
                                        { 7, COIN_CLASS },
                                        { 6, WAND_CLASS },
                                        { 5, RING_CLASS },
                                        { 1, AMULET_CLASS } };

static const struct icp rogueprobs[] = { { 12, WEAPON_CLASS },
                                         { 12, ARMOR_CLASS },
                                         { 22, FOOD_CLASS },
                                         { 22, POTION_CLASS },
                                         { 22, SCROLL_CLASS },
                                         { 5, WAND_CLASS },
                                         { 5, RING_CLASS } };

static const struct icp hellprobs[] = { { 20, WEAPON_CLASS },
                                        { 20, ARMOR_CLASS },
                                        { 16, FOOD_CLASS },
                                        { 12, TOOL_CLASS },
                                        { 10, GEM_CLASS },
                                        { 1, POTION_CLASS },
                                        { 1, SCROLL_CLASS },
                                        { 8, WAND_CLASS },
                                        { 8, RING_CLASS },
                                        { 4, AMULET_CLASS } };

static const struct oextra zerooextra = DUMMY;

static void
init_oextra(struct oextra *oex)
{
    *oex = zerooextra;
}

struct oextra *
newoextra(void)
{
    struct oextra *oextra;

    oextra = (struct oextra *) alloc(sizeof (struct oextra));
    init_oextra(oextra);
    return oextra;
}

void
dealloc_oextra(struct obj *o)
{
    struct oextra *x = o->oextra;

    if (x) {
        if (x->oname)
            free((genericptr_t) x->oname);
        if (x->omonst)
            free_omonst(o);     /* 'o' rather than 'x' */
        if (x->omailcmd)
            free((genericptr_t) x->omailcmd);

        free((genericptr_t) x);
        o->oextra = (struct oextra *) 0;
    }
}

void
newomonst(struct obj *otmp)
{
    if (!otmp->oextra)
        otmp->oextra = newoextra();

    if (!OMONST(otmp)) {
        struct monst *m = newmonst();

        *m = cg.zeromonst;
        OMONST(otmp) = m;
    }
}

void
free_omonst(struct obj *otmp)
{
    if (otmp->oextra) {
        struct monst *m = OMONST(otmp);

        if (m) {
            if (m->mextra)
                dealloc_mextra(m);
            free((genericptr_t) m);
            OMONST(otmp) = (struct monst *) 0;
        }
    }
}

void
newomid(struct obj *otmp)
{
    if (!otmp->oextra)
        otmp->oextra = newoextra();
    OMID(otmp) = 0;
}

void
free_omid(struct obj *otmp)
{
    OMID(otmp) = 0;
}

void
new_omailcmd(struct obj *otmp, const char *response_cmd)
{
    if (!otmp->oextra)
        otmp->oextra = newoextra();
    if (OMAILCMD(otmp))
        free_omailcmd(otmp);
    OMAILCMD(otmp) = dupstr(response_cmd);
}

void
free_omailcmd(struct obj *otmp)
{
    if (otmp->oextra && OMAILCMD(otmp)) {
        free((genericptr_t) OMAILCMD(otmp));
        OMAILCMD(otmp) = (char *) 0;
    }
}

struct obj *
mkobj_at(char let, coordxy x, coordxy y, boolean artif)
{
    struct obj *otmp;

    otmp = mkobj(let, artif);
    place_object(otmp, x, y);
    return otmp;
}

struct obj *
mksobj_at(int otyp, coordxy x, coordxy y, boolean init, boolean artif)
{
    struct obj *otmp;

    otmp = mksobj(otyp, init, artif);
    place_object(otmp, x, y);
    return otmp;
}

struct obj *
mksobj_migr_to_species(
    int otyp,
    unsigned int mflags2,
    boolean init,
    boolean artif)
{
    struct obj *otmp;

    otmp = mksobj(otyp, init, artif);
    add_to_migration(otmp);
    otmp->owornmask = (long) MIGR_TO_SPECIES;
    otmp->migr_species = mflags2;
    return otmp;
}

/* mkobj(): select a type of item from a class, use mksobj() to create it;
   result is always non-Null */
struct obj *
mkobj(int oclass, boolean artif)
{
    int tprob, i, prob;

    if (oclass == RANDOM_CLASS) {
        const struct icp *iprobs = Is_rogue_level(&u.uz)
                                   ? (const struct icp *) rogueprobs
                                   : Inhell ? (const struct icp *) hellprobs
                                            : (const struct icp *) mkobjprobs;

        for (tprob = rnd(100); (tprob -= iprobs->iprob) > 0; iprobs++)
            continue;
        oclass = iprobs->iclass;
    }

    if (oclass == SPBOOK_no_NOVEL) {
        i = rnd_class(gb.bases[SPBOOK_CLASS], SPE_BLANK_PAPER);
        oclass = SPBOOK_CLASS; /* for sanity check below */
    } else {
        prob = rnd(go.oclass_prob_totals[oclass]);
        i = gb.bases[oclass];
        while ((prob -= objects[i].oc_prob) > 0)
            ++i;
    }

    if (objects[i].oc_class != oclass || !OBJ_NAME(objects[i])) {
        impossible("probtype error, oclass=%d i=%d", (int) oclass, i);
        i = gb.bases[oclass];
    }

    return mksobj(i, TRUE, artif);
}

static void
mkbox_cnts(struct obj *box)
{
    register int n;
    register struct obj *otmp;

    box->cobj = (struct obj *) 0;

    switch (box->otyp) {
    case ICE_BOX:
        n = 20;
        break;
    case CHEST:
        n = box->olocked ? 7 : 5;
        break;
    case LARGE_BOX:
        n = box->olocked ? 5 : 3;
        break;
    case SACK:
    case OILSKIN_SACK:
        /* initial inventory: sack starts out empty */
        if (gm.moves <= 1 && !gi.in_mklev) {
            n = 0;
            break;
        }
        /*FALLTHRU*/
    case BAG_OF_HOLDING:
        n = 1;
        break;
    default:
        n = 0;
        break;
    }

    for (n = rn2(n + 1); n > 0; n--) {
        if (box->otyp == ICE_BOX) {
            otmp = mksobj(CORPSE, TRUE, FALSE);
            /* Note: setting age to 0 is correct.  Age has a different
             * from usual meaning for objects stored in ice boxes. -KAA
             */
            otmp->age = 0L;
            if (otmp->timed) {
                (void) stop_timer(ROT_CORPSE, obj_to_any(otmp));
                (void) stop_timer(REVIVE_MON, obj_to_any(otmp));
                (void) stop_timer(SHRINK_GLOB, obj_to_any(otmp));
            }
        } else {
            register int tprob;
            const struct icp *iprobs = boxiprobs;

            for (tprob = rnd(100); (tprob -= iprobs->iprob) > 0; iprobs++)
                ;
            if (!(otmp = mkobj(iprobs->iclass, FALSE)))
                continue;

            /* handle a couple of special cases */
            if (otmp->oclass == COIN_CLASS) {
                /* 2.5 x level's usual amount; weight adjusted below */
                otmp->quan = (long) (rnd(level_difficulty() + 2) * rnd(75));
                otmp->owt = weight(otmp);
            } else
                while (otmp->otyp == ROCK) {
                    otmp->otyp = rnd_class(DILITHIUM_CRYSTAL, LOADSTONE);
                    if (otmp->quan > 2L)
                        otmp->quan = 1L;
                    otmp->owt = weight(otmp);
                }
            if (box->otyp == BAG_OF_HOLDING) {
                if (Is_mbag(otmp)) {
                    otmp->otyp = SACK;
                    otmp->spe = 0;
                    otmp->owt = weight(otmp);
                } else
                    while (otmp->otyp == WAN_CANCELLATION)
                        otmp->otyp = rnd_class(WAN_LIGHT, WAN_LIGHTNING);
            }
        }
        (void) add_to_container(box, otmp);
    }
}

/* select a random, common monster type */
int
rndmonnum(void)
{
    return rndmonnum_adj(0, 0);
}

/* select a random, common monster type, with adjusted difficulty */
int
rndmonnum_adj(int minadj, int maxadj)
{
    register struct permonst *ptr;
    register int i;
    unsigned short excludeflags;

    /* Plan A: get a level-appropriate common monster */
    ptr = rndmonst_adj(minadj, maxadj);
    if (ptr)
        return monsndx(ptr);

    /* Plan B: get any common monster */
    excludeflags = G_UNIQ | G_NOGEN | (Inhell ? G_NOHELL : G_HELL);
    do {
        i = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
        ptr = &mons[i];
    } while ((ptr->geno & excludeflags) != 0);

    return i;
}

void
copy_oextra(struct obj *obj2, struct obj *obj1)
{
    if (!obj2 || !obj1 || !obj1->oextra)
        return;

    if (!obj2->oextra)
        obj2->oextra = newoextra();
    if (has_oname(obj1))
        oname(obj2, ONAME(obj1), ONAME_SKIP_INVUPD);
    if (has_omonst(obj1)) {
        if (!OMONST(obj2))
            newomonst(obj2);
        (void) memcpy((genericptr_t) OMONST(obj2),
                      (genericptr_t) OMONST(obj1), sizeof (struct monst));
        OMONST(obj2)->mextra = (struct mextra *) 0;
        OMONST(obj2)->nmon = (struct monst *) 0;
#if 0
        OMONST(obj2)->m_id = next_ident();
#endif
        if (OMONST(obj1)->mextra)
            copy_mextra(OMONST(obj2), OMONST(obj1));
    }
    if (has_omailcmd(obj1)) {
        new_omailcmd(obj2, OMAILCMD(obj1));
    }
    if (has_omid(obj1)) {
        if (!OMID(obj2))
            newomid(obj2);
        OMID(obj2) = OMID(obj1);
    }
}

/*
 * Split obj so that it gets size gets reduced by num. The quantity num is
 * put in the object structure delivered by this call.  The returned object
 * has its wornmask cleared and is positioned just following the original
 * in the nobj chain (and nexthere chain when on the floor).
 */
struct obj *
splitobj(struct obj *obj, long num)
{
    struct obj *otmp;

    if (obj->cobj || num <= 0L || obj->quan <= num)
        panic("splitobj"); /* can't split containers */
    otmp = newobj();
    *otmp = *obj; /* copies whole structure */
    otmp->oextra = (struct oextra *) 0;
    otmp->o_id = nextoid(obj, otmp);
    otmp->timed = 0;                  /* not timed, yet */
    otmp->lamplit = 0;                /* ditto */
    otmp->owornmask = 0L;             /* new object isn't worn */
    obj->quan -= num;
    obj->owt = weight(obj);
    otmp->quan = num;
    otmp->owt = weight(otmp); /* -= obj->owt ? */
    otmp->lua_ref_cnt = 0;
    otmp->pickup_prev = 0;

    gc.context.objsplit.parent_oid = obj->o_id;
    gc.context.objsplit.child_oid = otmp->o_id;
    obj->nobj = otmp;
    /* Only set nexthere when on the floor; nexthere is also used
       as a back pointer to the container object when contained.
       For either case, otmp's nexthere pointer is already pointing
       at the right thing. */
    if (obj->where == OBJ_FLOOR)
        obj->nexthere = otmp; /* insert into chain: obj -> otmp -> next */
    /* lua isn't tracking the split off portion even if it happens to
       be tracking the original */
    if (otmp->where == OBJ_LUAFREE)
        otmp->where = OBJ_FREE;
    if (obj->unpaid)
        splitbill(obj, otmp);
    copy_oextra(otmp, obj);
    if (has_omid(otmp))
        free_omid(otmp); /* only one association with m_id */
    if (obj->timed)
        obj_split_timers(obj, otmp);
    if (obj_sheds_light(obj))
        obj_split_light_source(obj, otmp);
    return otmp;
}

/* return the value of context.ident and then increment it to be ready for
   its next use; used to be simple += 1 so that every value from 1 to N got
   used but now has a random increase that skips half of potential values */
unsigned
next_ident(void)
{
    unsigned res = gc.context.ident;

    /* +rnd(2): originally just +1; changed to rnd() to avoid potential
       exploit of player using #adjust to split an object stack in a manner
       that makes most recent ident%2 known; since #adjust takes no time,
       no intervening activity like random creation of a new monster will
       take place before next user command; with former +1, o_id%2 of the
       next object to be created was knowable and player could make a wish
       under controlled circumstances for an item that is affected by the
       low bits of its obj->o_id [particularly helm of opposite alignment] */
    gc.context.ident += rnd(2); /* ready for next new object or monster */

    /* if ident has wrapped to 0, force it to be non-zero; if/when it
       ever wraps past 0 (unlikely, but possible on a configuration which
       uses 16-bit 'int'), just live with that and hope no o_id conflicts
       between objects or m_id conflicts between monsters arise */
    if (!gc.context.ident)
        gc.context.ident = rnd(2);

    return res;
}

/* when splitting a stack that has o_id-based shop prices, pick an
   o_id value for the new stack that will maintain the same price */
static unsigned
nextoid(struct obj *oldobj, struct obj *newobj)
{
    int olddif, newdif, trylimit = 256; /* limit of 4 suffices at present */
    unsigned oid = gc.context.ident - 1; /* loop increment will reverse -1 */

    olddif = oid_price_adjustment(oldobj, oldobj->o_id);
    do {
        ++oid;
        if (!oid) /* avoid using 0 (in case value wrapped) */
            ++oid;
        newdif = oid_price_adjustment(newobj, oid);
    } while (newdif != olddif && --trylimit >= 0);
    gc.context.ident = oid; /* update 'last ident used' */
    (void) next_ident(); /* increment context.ident for next use */
    return oid; /* caller will use this ident */
}

/* try to find the stack obj was split from, then merge them back together;
   returns the combined object if unsplit is successful, null otherwise */
struct obj *
unsplitobj(struct obj *obj)
{
    unsigned target_oid = 0;
    struct obj *oparent = 0, *ochild = 0, *list = 0;

    /*
     * We don't operate on floor objects (we're following o->nobj rather
     * than o->nexthere), on free objects (don't know which list to use when
     * looking for obj's parent or child), on bill objects (too complicated,
     * not needed), or on buried or migrating objects (not needed).
     * [This could be improved, but at present additional generality isn't
     * necessary.]
     */
    switch (obj->where) {
    case OBJ_FREE:
    case OBJ_FLOOR:
    case OBJ_ONBILL:
    case OBJ_MIGRATING:
    case OBJ_BURIED:
    default:
        return (struct obj *) 0;
    case OBJ_INVENT:
        list = gi.invent;
        break;
    case OBJ_MINVENT:
        list = obj->ocarry->minvent;
        break;
    case OBJ_CONTAINED:
        list = obj->ocontainer->cobj;
        break;
    }

    /* first try the expected case; obj is split from another stack */
    if (obj->o_id == gc.context.objsplit.child_oid) {
        /* parent probably precedes child and will require list traversal */
        ochild = obj;
        target_oid = gc.context.objsplit.parent_oid;
        if (obj->nobj && obj->nobj->o_id == target_oid)
            oparent = obj->nobj;
    } else if (obj->o_id == gc.context.objsplit.parent_oid) {
        /* alternate scenario: another stack was split from obj;
           child probably follows parent and will be found here */
        oparent = obj;
        target_oid = gc.context.objsplit.child_oid;
        if (obj->nobj && obj->nobj->o_id == target_oid)
            ochild = obj->nobj;
    }
    /* if we have only half the split, scan obj's list to find other half */
    if (ochild && !oparent) {
        /* expected case */
        for (obj = list; obj; obj = obj->nobj)
            if (obj->o_id == target_oid) {
                oparent = obj;
                break;
            }
    } else if (oparent && !ochild) {
        /* alternate scenario */
        for (obj = list; obj; obj = obj->nobj)
            if (obj->o_id == target_oid) {
                ochild = obj;
                break;
            }
    }
    /* if we have both parent and child, try to merge them;
       if successful, return the combined stack, otherwise return null */
    return (oparent && ochild && merged(&oparent, &ochild)) ? oparent : 0;
}

/* reset splitobj()/unsplitobj() context */
void
clear_splitobjs(void)
{
    gc.context.objsplit.parent_oid = gc.context.objsplit.child_oid = 0;
}

/*
 * Insert otmp right after obj in whatever chain(s) it is on.  Then extract
 * obj from the chain(s).  This function does a literal swap.  It is up to
 * the caller to provide a valid context for the swap.  When done, obj will
 * still exist, but not on any chain.
 *
 * Note:  Don't use use obj_extract_self() -- we are doing an in-place swap,
 * not actually moving something.
 */
void
replace_object(struct obj *obj, struct obj *otmp)
{
    otmp->where = obj->where;
    switch (obj->where) {
    case OBJ_FREE:
        /* do nothing */
        break;
    case OBJ_INVENT:
        otmp->nobj = obj->nobj;
        obj->nobj = otmp;
        extract_nobj(obj, &gi.invent);
        break;
    case OBJ_CONTAINED:
        otmp->nobj = obj->nobj;
        otmp->ocontainer = obj->ocontainer;
        obj->nobj = otmp;
        extract_nobj(obj, &obj->ocontainer->cobj);
        break;
    case OBJ_MINVENT:
        otmp->nobj = obj->nobj;
        otmp->ocarry = obj->ocarry;
        obj->nobj = otmp;
        extract_nobj(obj, &obj->ocarry->minvent);
        break;
    case OBJ_FLOOR:
        otmp->nobj = obj->nobj;
        otmp->nexthere = obj->nexthere;
        otmp->ox = obj->ox;
        otmp->oy = obj->oy;
        obj->nobj = otmp;
        obj->nexthere = otmp;
        extract_nobj(obj, &fobj);
        extract_nexthere(obj, &gl.level.objects[obj->ox][obj->oy]);
        break;
    default:
        panic("replace_object: obj position");
        break;
    }
}

/* is 'obj' inside a container whose contents aren't known?
   if so, return the outermost container meeting that criterium */
struct obj *
unknwn_contnr_contents(struct obj *obj)
{
    struct obj *result = 0, *parent;

    while (obj->where == OBJ_CONTAINED) {
        parent = obj->ocontainer;
        if (!parent->cknown)
            result = parent;
        obj = parent;
    }
    return result;
}

/*
 * Create a dummy duplicate to put on shop bill.  The duplicate exists
 * only in the billobjs chain.  This function is used when a shop object
 * is being altered, and a copy of the original is needed for billing
 * purposes.  For example, when eating, where an interruption will yield
 * an object which is different from what it started out as; the "I x"
 * command needs to display the original object.
 *
 * The caller is responsible for checking otmp->unpaid and
 * costly_spot(u.ux, u.uy).  This function will make otmp no charge.
 *
 * Note that check_unpaid_usage() should be used instead for partial
 * usage of an object.
 */
void
bill_dummy_object(struct obj *otmp)
{
    register struct obj *dummy;
    long cost = 0L;

    if (otmp->unpaid) {
        cost = unpaid_cost(otmp, FALSE);
        subfrombill(otmp, shop_keeper(*u.ushops));
    }
    dummy = newobj();
    *dummy = *otmp;
    dummy->oextra = (struct oextra *) 0;
    dummy->where = OBJ_FREE;
    dummy->o_id = nextoid(otmp, dummy);
    dummy->timed = 0;
    copy_oextra(dummy, otmp);
    if (has_omid(dummy))
        free_omid(dummy); /* only one association with m_id*/
    if (Is_candle(dummy))
        dummy->lamplit = 0;
    dummy->owornmask = 0L; /* dummy object is not worn */
    addtobill(dummy, FALSE, TRUE, TRUE);
    if (cost)
        alter_cost(dummy, -cost);
    /* no_charge is only valid for some locations */
    otmp->no_charge = (otmp->where == OBJ_FLOOR
                       || otmp->where == OBJ_CONTAINED) ? 1 : 0;
    otmp->unpaid = 0;
    return;
}

/* alteration types; must match COST_xxx macros in hack.h */
static const char *const alteration_verbs[] = {
    "cancel", "drain", "uncharge", "unbless", "uncurse", "disenchant",
    "degrade", "dilute", "erase", "burn", "neutralize", "destroy", "splatter",
    "bite", "open", "break the lock on", "rust", "rot", "tarnish"
};

/* possibly bill for an object which the player has just modified */
void
costly_alteration(struct obj *obj, int alter_type)
{
    coordxy ox, oy;
    char objroom;
    boolean learn_bknown;
    const char *those, *them;
    struct monst *shkp = 0;

    if (alter_type < 0 || alter_type >= SIZE(alteration_verbs)) {
        impossible("invalid alteration type (%d)", alter_type);
        alter_type = 0;
    }

    ox = oy = 0;    /* lint suppression */
    objroom = '\0'; /* ditto */
    if (carried(obj) || obj->where == OBJ_FREE) {
        /* OBJ_FREE catches obj_no_longer_held()'s transformation
           of crysknife back into worm tooth; the object has been
           removed from inventory but not necessarily placed at
           its new location yet--the unpaid flag will still be set
           if this item is owned by a shop */
        if (!obj->unpaid)
            return;
    } else {
        /* this get_obj_location shouldn't fail, but if it does,
           use hero's location */
        if (!get_obj_location(obj, &ox, &oy, CONTAINED_TOO))
            ox = u.ux, oy = u.uy;
        if (!costly_spot(ox, oy))
            return;
        objroom = *in_rooms(ox, oy, SHOPBASE);
        /* if no shop cares about it, we're done */
        if (!billable(&shkp, obj, objroom, FALSE))
            return;
    }

    if (obj->quan == 1L)
        those = "that", them = "it";
    else
        those = "those", them = "them";

    /* when shopkeeper describes the object as being uncursed or unblessed
       hero will know that it is now uncursed; will also make the feedback
       from `I x' after bill_dummy_object() be more specific for this item */
    learn_bknown = (alter_type == COST_UNCURS || alter_type == COST_UNBLSS);

    switch (obj->where) {
    case OBJ_FREE: /* obj_no_longer_held() */
    case OBJ_INVENT:
        if (learn_bknown)
            set_bknown(obj, 1);
        verbalize("You %s %s %s, you pay for %s!",
                  alteration_verbs[alter_type], those, simpleonames(obj),
                  them);
        bill_dummy_object(obj);
        break;
    case OBJ_FLOOR:
        if (learn_bknown)
            obj->bknown = 1; /* ok to bypass set_bknown() here */
        if (costly_spot(u.ux, u.uy) && objroom == *u.ushops) {
            verbalize("You %s %s, you pay for %s!",
                      alteration_verbs[alter_type], those, them);
            bill_dummy_object(obj);
        } else {
            (void) stolen_value(obj, ox, oy, FALSE, FALSE);
        }
        break;
    }
}

static const char dknowns[] = { WAND_CLASS,   RING_CLASS, POTION_CLASS,
                                SCROLL_CLASS, GEM_CLASS,  SPBOOK_CLASS,
                                WEAPON_CLASS, TOOL_CLASS, VENOM_CLASS, 0 };

/* set obj->dknown to 0 for most types of objects, to 1 otherwise;
   split off from unknow_object() */
void
clear_dknown(struct obj *obj)
{
    obj->dknown = strchr(dknowns, obj->oclass) ? 0 : 1;
    if ((obj->otyp >= ELVEN_SHIELD && obj->otyp <= ORCISH_SHIELD)
        || obj->otyp == SHIELD_OF_REFLECTION
        || objects[obj->otyp].oc_merge)
        obj->dknown = 0;
    /* globs always have dknown flag set (to maximize merging) but for new
       object, globby flag won't be set yet so isn't available to check */
    if (Is_pudding(obj))
        obj->dknown = 1;
}

/* some init for a brand new object, or partial re-init when hero loses
   potentially known info about an object (called when an unseen monster
   picks up or uses it); moved from invent.c to here for access to dknowns */
void
unknow_object(struct obj *obj)
{
    clear_dknown(obj); /* obj->dknown = 0; */

    obj->bknown = obj->rknown = 0;
    obj->cknown = obj->lknown = 0;
    /* for an existing object, awareness of charges or enchantment has
       gone poof...  [object types which don't use the known flag have
       it set True for some reason] */
    obj->known = objects[obj->otyp].oc_uses_known ? 0 : 1;
}

/* mksobj(): create a specific type of object; result it always non-Null */
struct obj *
mksobj(int otyp, boolean init, boolean artif)
{
    int mndx, tryct;
    struct obj *otmp;
    char let = objects[otyp].oc_class;

    otmp = newobj();
    *otmp = cg.zeroobj;
    otmp->age = gm.moves;
    otmp->o_id = next_ident();
    otmp->quan = 1L;
    otmp->oclass = let;
    otmp->otyp = otyp;
    otmp->where = OBJ_FREE;
    unknow_object(otmp); /* set up dknown and known: non-0 for some things */
    otmp->corpsenm = NON_PM;
    otmp->lua_ref_cnt = 0;
    otmp->pickup_prev = 0;

    if (init) {
        switch (let) {
        case WEAPON_CLASS:
            otmp->quan = is_multigen(otmp) ? (long) rn1(6, 6) : 1L;
            if (!rn2(11)) {
                otmp->spe = rne(3);
                otmp->blessed = rn2(2);
            } else if (!rn2(10)) {
                curse(otmp);
                otmp->spe = -rne(3);
            } else
                blessorcurse(otmp, 10);
            if (is_poisonable(otmp) && !rn2(100))
                otmp->opoisoned = 1;

            if (artif && !rn2(20 + (10 * nartifact_exist())))
                otmp = mk_artifact(otmp, (aligntyp) A_NONE);
            break;
        case FOOD_CLASS:
            otmp->oeaten = 0;
            switch (otmp->otyp) {
            case CORPSE:
                /* possibly overridden by mkcorpstat() */
                tryct = 50;
                do
                    otmp->corpsenm = undead_to_corpse(rndmonnum());
                while ((gm.mvitals[otmp->corpsenm].mvflags & G_NOCORPSE)
                       && (--tryct > 0));
                if (tryct == 0) {
                    /* perhaps rndmonnum() only wants to make G_NOCORPSE
                       monsters on this gl.level; create an adventurer's
                       corpse instead, then */
                    otmp->corpsenm = PM_HUMAN;
                }
                /* timer set below */
                break;
            case EGG:
                otmp->corpsenm = NON_PM; /* generic egg */
                if (!rn2(3))
                    for (tryct = 200; tryct > 0; --tryct) {
                        mndx = can_be_hatched(rndmonnum());
                        if (mndx != NON_PM && !dead_species(mndx, TRUE)) {
                            otmp->corpsenm = mndx; /* typed egg */
                            break;
                        }
                    }
                /* timer set below */
                break;
            case TIN:
                otmp->corpsenm = NON_PM; /* empty (so far) */
                if (!rn2(6))
                    set_tin_variety(otmp, SPINACH_TIN);
                else
                    for (tryct = 200; tryct > 0; --tryct) {
                        mndx = undead_to_corpse(rndmonnum());
                        if (mons[mndx].cnutrit
                            && !(gm.mvitals[mndx].mvflags & G_NOCORPSE)) {
                            otmp->corpsenm = mndx;
                            set_tin_variety(otmp, RANDOM_TIN);
                            break;
                        }
                    }
                blessorcurse(otmp, 10);
                break;
            case SLIME_MOLD:
                otmp->spe = gc.context.current_fruit;
                flags.made_fruit = TRUE;
                break;
            case KELP_FROND:
                otmp->quan = (long) rnd(2);
                break;
            case CANDY_BAR:
                /* set otmp->spe */
                assign_candy_wrapper(otmp);
                break;
            default:
                break;
            }
            if (Is_pudding(otmp)) {
                otmp->globby = 1;
                /* for emphasis; glob quantity is always 1 and weight varies
                   when other globs coallesce with it or this one shrinks */
                otmp->quan = 1L;
                /* 3.7: globs in 3.6.x left owt as 0 and let weight() fix
                   that up during 'obj->owt = weight(obj)' below, but now
                   we initialize glob->owt explicitly so weight() doesn't
                   need to perform any fix up and returns glob->owt as-is */
                otmp->owt = objects[otmp->otyp].oc_weight;
                otmp->known = otmp->dknown = 1;
                otmp->corpsenm = PM_GRAY_OOZE
                                 + (otmp->otyp - GLOB_OF_GRAY_OOZE);
                start_glob_timeout(otmp, 0L);
            } else {
                if (otmp->otyp != CORPSE && otmp->otyp != MEAT_RING
                    && otmp->otyp != KELP_FROND && !rn2(6)) {
                    otmp->quan = 2L;
                }
            }
            break;
        case GEM_CLASS:
            otmp->corpsenm = 0; /* LOADSTONE hack */
            if (otmp->otyp == LOADSTONE)
                curse(otmp);
            else if (otmp->otyp == ROCK)
                otmp->quan = (long) rn1(6, 6);
            else if (otmp->otyp != LUCKSTONE && !rn2(6))
                otmp->quan = 2L;
            else
                otmp->quan = 1L;
            break;
        case TOOL_CLASS:
            switch (otmp->otyp) {
            case TALLOW_CANDLE:
            case WAX_CANDLE:
                otmp->spe = 1;
                otmp->age = 20L * /* 400 or 200 */
                            (long) objects[otmp->otyp].oc_cost;
                otmp->lamplit = 0;
                otmp->quan = 1L + (long) (rn2(2) ? rn2(7) : 0);
                blessorcurse(otmp, 5);
                break;
            case BRASS_LANTERN:
            case OIL_LAMP:
                otmp->spe = 1;
                otmp->age = (long) rn1(500, 1000);
                otmp->lamplit = 0;
                blessorcurse(otmp, 5);
                break;
            case MAGIC_LAMP:
                otmp->spe = 1;
                otmp->lamplit = 0;
                blessorcurse(otmp, 2);
                break;
            case CHEST:
            case LARGE_BOX:
                otmp->olocked = !!(rn2(5));
                otmp->otrapped = !(rn2(10));
                /*FALLTHRU*/
            case ICE_BOX:
            case SACK:
            case OILSKIN_SACK:
            case BAG_OF_HOLDING:
                mkbox_cnts(otmp);
                break;
            case EXPENSIVE_CAMERA:
            case TINNING_KIT:
            case MAGIC_MARKER:
                otmp->spe = rn1(70, 30);
                break;
            case CAN_OF_GREASE:
                otmp->spe = rn1(21, 5); /* 0..20 + 5 => 5..25 */
                blessorcurse(otmp, 10);
                break;
            case CRYSTAL_BALL:
                otmp->spe = rn1(5, 3); /* 0..4 + 3 => 3..7 */
                blessorcurse(otmp, 2);
                break;
            case HORN_OF_PLENTY:
            case BAG_OF_TRICKS:
                otmp->spe = rn1(18, 3); /* 0..17 + 3 => 3..20 */
                break;
            case FIGURINE:
                tryct = 0;
                /* figurines are slightly harder monsters */
                do
                    otmp->corpsenm = rndmonnum_adj(5, 10);
                while (is_human(&mons[otmp->corpsenm]) && tryct++ < 30);
                blessorcurse(otmp, 4);
                break;
            case BELL_OF_OPENING:
                otmp->spe = 3;
                break;
            case MAGIC_FLUTE:
            case MAGIC_HARP:
            case FROST_HORN:
            case FIRE_HORN:
            case DRUM_OF_EARTHQUAKE:
                otmp->spe = rn1(5, 4);
                break;
            }
            break;
        case AMULET_CLASS:
            if (otmp->otyp == AMULET_OF_YENDOR)
                gc.context.made_amulet = TRUE;
            if (rn2(10) && (otmp->otyp == AMULET_OF_STRANGULATION
                            || otmp->otyp == AMULET_OF_CHANGE
                            || otmp->otyp == AMULET_OF_RESTFUL_SLEEP)) {
                curse(otmp);
            } else
                blessorcurse(otmp, 10);
            break;
        case VENOM_CLASS:
        case CHAIN_CLASS:
        case BALL_CLASS:
            break;
        case POTION_CLASS: /* note: potions get some additional init below */
        case SCROLL_CLASS:
#ifdef MAIL_STRUCTURES
            if (otmp->otyp != SCR_MAIL)
#endif
                blessorcurse(otmp, 4);
            break;
        case SPBOOK_CLASS:
            otmp->spestudied = 0;
            blessorcurse(otmp, 17);
            break;
        case ARMOR_CLASS:
            if (rn2(10)
                && (otmp->otyp == FUMBLE_BOOTS
                    || otmp->otyp == LEVITATION_BOOTS
                    || otmp->otyp == HELM_OF_OPPOSITE_ALIGNMENT
                    || otmp->otyp == GAUNTLETS_OF_FUMBLING || !rn2(11))) {
                curse(otmp);
                otmp->spe = -rne(3);
            } else if (!rn2(10)) {
                otmp->blessed = rn2(2);
                otmp->spe = rne(3);
            } else
                blessorcurse(otmp, 10);
            if (artif && !rn2(40 + (10 * nartifact_exist())))
                otmp = mk_artifact(otmp, (aligntyp) A_NONE);
            /* simulate lacquered armor for samurai */
            if (Role_if(PM_SAMURAI) && otmp->otyp == SPLINT_MAIL
                && (gm.moves <= 1 || In_quest(&u.uz))) {
#ifdef UNIXPC
                /* optimizer bitfield bug */
                otmp->oerodeproof = 1;
                otmp->rknown = 1;
#else
                otmp->oerodeproof = otmp->rknown = 1;
#endif
            }
            break;
        case WAND_CLASS:
            if (otmp->otyp == WAN_WISHING)
                otmp->spe = rnd(3);
            else
                otmp->spe =
                    rn1(5, (objects[otmp->otyp].oc_dir == NODIR) ? 11 : 4);
            blessorcurse(otmp, 17);
            otmp->recharged = 0; /* used to control recharging */
            break;
        case RING_CLASS:
            if (objects[otmp->otyp].oc_charged) {
                blessorcurse(otmp, 3);
                if (rn2(10)) {
                    if (rn2(10) && bcsign(otmp))
                        otmp->spe = bcsign(otmp) * rne(3);
                    else
                        otmp->spe = rn2(2) ? rne(3) : -rne(3);
                }
                /* make useless +0 rings much less common */
                if (otmp->spe == 0)
                    otmp->spe = rn2(4) - rn2(3);
                /* negative rings are usually cursed */
                if (otmp->spe < 0 && rn2(5))
                    curse(otmp);
            } else if (rn2(10) && (otmp->otyp == RIN_TELEPORTATION
                                   || otmp->otyp == RIN_POLYMORPH
                                   || otmp->otyp == RIN_AGGRAVATE_MONSTER
                                   || otmp->otyp == RIN_HUNGER || !rn2(9))) {
                curse(otmp);
            }
            break;
        case ROCK_CLASS:
            if (otmp->otyp == STATUE) {
                /* possibly overridden by mkcorpstat() */
                otmp->corpsenm = rndmonnum();
                if (!verysmall(&mons[otmp->corpsenm])
                    && rn2(level_difficulty() / 2 + 10) > 10)
                    (void) add_to_container(otmp,
                                            mkobj(SPBOOK_no_NOVEL, FALSE));
            }
            /* boulder init'd below in the 'regardless of !init' code */
            break;
        case COIN_CLASS:
            break; /* do nothing */
        default:
            /* 3.6.3: this used to be impossible() followed by return 0
               but most callers aren't prepared to deal with Null result
               and cluttering them up to do so is pointless */
            panic("mksobj tried to make type %d, class %d.",
                  (int) otmp->otyp, (int) objects[otmp->otyp].oc_class);
            /*NOTREACHED*/
        }
    }

    /* some things must get done (corpsenm, timers) even if init = 0 */
    switch ((otmp->oclass == POTION_CLASS && otmp->otyp != POT_OIL)
            ? POT_WATER
            : otmp->otyp) {
    case CORPSE:
        if (otmp->corpsenm == NON_PM) {
            otmp->corpsenm = undead_to_corpse(rndmonnum());
            if (gm.mvitals[otmp->corpsenm].mvflags & (G_NOCORPSE | G_GONE))
                otmp->corpsenm = gu.urole.mnum;
        }
        /*FALLTHRU*/
    case STATUE:
    case FIGURINE:
        if (otmp->corpsenm == NON_PM)
            otmp->corpsenm = rndmonnum();
        if (otmp->corpsenm != NON_PM) {
            struct permonst *ptr = &mons[otmp->corpsenm];

            otmp->spe = (is_neuter(ptr) ? CORPSTAT_NEUTER
                         : is_female(ptr) ? CORPSTAT_FEMALE
                           : is_male(ptr) ? CORPSTAT_MALE
                             : rn2(2) ? CORPSTAT_FEMALE : CORPSTAT_MALE);
        }
        /*FALLTHRU*/
    case EGG:
    /* case TIN: */
        set_corpsenm(otmp, otmp->corpsenm);
        break;
    case BOULDER:
        /* next_boulder overloads corpsenm so the default value is NON_PM;
           since that is non-zero, the "next boulder" case in xname() would
           happen when it shouldn't; explicitly set it to 0 */
        otmp->next_boulder = 0;
        break;
    case POT_OIL:
        otmp->age = MAX_OIL_IN_FLASK; /* amount of oil */
        /*FALLTHRU*/
    case POT_WATER: /* POTION_CLASS */
        otmp->fromsink = 0; /* overloads corpsenm, which was set to NON_PM */
        break;
    case LEASH:
        otmp->leashmon = 0; /* overloads corpsenm, which was set to NON_PM */
        break;
    case SPE_NOVEL:
        otmp->novelidx = -1; /* "none of the above"; will be changed */
        otmp = oname(otmp, noveltitle(&otmp->novelidx), ONAME_NO_FLAGS);
        break;
    }

    /* unique objects may have an associated artifact entry */
    if (objects[otyp].oc_unique && !otmp->oartifact)
        otmp = mk_artifact(otmp, (aligntyp) A_NONE);
    otmp->owt = weight(otmp);
    return otmp;
}

/*
 * Several areas of the code made direct reassignments
 * to obj->corpsenm.  Because some special handling is
 * required in certain cases, place that handling here
 * and call this routine in place of the direct assignment.
 *
 * If the object was a lizard or lichen corpse:
 *     - ensure that you don't end up with some
 *       other corpse type which has no rot-away timer.
 *
 * If the object was a troll corpse:
 *     - ensure that you don't end up with some other
 *       corpse type which resurrects from the dead.
 *
 * Re-calculates the weight of figurines and corpses to suit the
 * new species.
 *
 * Existing timeout value for egg hatch is preserved.
 *
 */
void
set_corpsenm(struct obj *obj, int id)
{
    int old_id = obj->corpsenm;
    long when = 0L;

    if (obj->timed) {
        if (obj->otyp == EGG) {
            when = stop_timer(HATCH_EGG, obj_to_any(obj));
        } else {
            when = 0L;
            obj_stop_timers(obj); /* corpse or figurine */
        }
    }
    /* oeaten is used to determine how much nutrition is left in
       multiple-bite food and also used to derive how many hit points
       a creature resurrected from a partly eaten corpse gets; latter
       is of interest when a <foo> corpse revives as a <foo> zombie
       in case they are defined with different mons[].cnutrit values */
    if (obj->otyp == CORPSE && obj->oeaten != 0
        /* when oeaten is non-zero, index old_id can't be NON_PM
           and divisor mons[old_id].cnutrit can't be zero */
        && mons[old_id].cnutrit != mons[id].cnutrit) {
        /* oeaten and cnutrit are unsigned; theoretically that could
           be 16 bits and the calculation might overflow, so force long */
        obj->oeaten = (unsigned) ((long) obj->oeaten
                                  * (long) mons[id].cnutrit
                                  / (long) mons[old_id].cnutrit);
    }

    obj->corpsenm = id;
    switch (obj->otyp) {
    case CORPSE:
        start_corpse_timeout(obj);
        obj->owt = weight(obj);
        break;
    case FIGURINE:
        if (obj->corpsenm != NON_PM && !dead_species(obj->corpsenm, TRUE)
            && (carried(obj) || mcarried(obj)))
            attach_fig_transform_timeout(obj);
        obj->owt = weight(obj);
        break;
    case EGG:
        if (obj->corpsenm != NON_PM && !dead_species(obj->corpsenm, TRUE))
            attach_egg_hatch_timeout(obj, when);
        break;
    default: /* tin, etc. */
        obj->owt = weight(obj);
        break;
    }
}

/* Return the number of turns after which a Rider corpse revives */
long
rider_revival_time(struct obj *body, boolean retry)
{
    long when;
    long minturn = retry ? 3L : (body->corpsenm == PM_DEATH) ? 6L : 12L;

    /* Riders have a 1/3 chance per turn of reviving after 12, 6, or 3 turns.
       Always revive by 67. */
    for (when = minturn; when < 67L; when++)
        if (!rn2(3))
            break;
    return when;
}

/*
 * Start a corpse decay or revive timer.
 * This takes the age of the corpse into consideration as of 3.4.0.
 */
void
start_corpse_timeout(struct obj *body)
{
    long when;       /* rot away when this old */
    long age;        /* age of corpse          */
    int rot_adjust;
    short action;

    /*
     * Note:
     *      if body->norevive is set, the corpse will rot away instead
     *      of revive when its REVIVE_MON timer finishes.
     */

    /* lizards and lichen don't rot or revive */
    if (body->corpsenm == PM_LIZARD || body->corpsenm == PM_LICHEN)
        return;

    action = ROT_CORPSE;               /* default action: rot away */
    rot_adjust = gi.in_mklev ? 25 : 10; /* give some variation */
    age = gm.moves - body->age;
    if (age > ROT_AGE)
        when = rot_adjust;
    else
        when = ROT_AGE - age;
    when += (long) (rnz(rot_adjust) - rot_adjust);

    if (is_rider(&mons[body->corpsenm])) {
        action = REVIVE_MON;
        when = rider_revival_time(body, FALSE);
    } else if (mons[body->corpsenm].mlet == S_TROLL) {
        for (age = 2; age <= TAINT_AGE; age++)
            if (!rn2(TROLL_REVIVE_CHANCE)) { /* troll revives */
                action = REVIVE_MON;
                when = age;
                break;
            }
    } else if (gz.zombify && zombie_form(&mons[body->corpsenm]) != NON_PM
               && !body->norevive) {
        action = ZOMBIFY_MON;
        when = rn1(15, 5); /* 5..19 */
    }

    (void) start_timer(when, TIMER_OBJECT, action, obj_to_any(body));
}

/* used by item_on_ice() and shrink_glob() */
enum obj_on_ice {
    NOT_ON_ICE = 0,
    SET_ON_ICE = 1,
    BURIED_UNDER_ICE = 2
};

/* used by shrink_glob(); is 'item' or enclosing container on or under ice? */
static int
item_on_ice(struct obj *item)
{
    struct obj *otmp;
    coordxy ox, oy;

    otmp = item;
    /* if in a container, it might be nested so find outermost one since
       that's the item whose location needs to be checked */
    while (otmp->where == OBJ_CONTAINED)
        otmp = otmp->ocontainer;

    if (get_obj_location(otmp, &ox, &oy, BURIED_TOO)) {
        switch (otmp->where) {
        case OBJ_FLOOR:
            if (is_ice(ox, oy))
                return SET_ON_ICE;
            break;
        case OBJ_BURIED:
            if (is_ice(ox, oy))
                return BURIED_UNDER_ICE;
            break;
        default:
            break;
        }
    }
    return NOT_ON_ICE;
}

/* schedule a timer that will shrink the target glob by 1 unit of weight */
void
start_glob_timeout(
    struct obj *obj, /* glob */
    long when)       /* when to shrink; if 0L, use random value close to 25 */
{
    if (!obj->globby) {
        impossible("start_glob_timeout for non-glob [%d: %s]?",
                   obj->otyp, simpleonames(obj));
        return; /* skip timer creation */
    }
    /* sanity precaution */
    if (obj->timed)
        (void) stop_timer(SHRINK_GLOB, obj_to_any(obj));

    if (when < 1L) /* caller usually passes 0L; should never be negative */
        when = 25L + (long) rn2(5) - 2L; /* 25+[0..4]-2 => 23..27, avg 25 */
    /* 1 new glob weighs 20 units and loses 1 unit every 25 turns,
       so lasts for 500 turns, twice as long as the average corpse */
    (void) start_timer(when, TIMER_OBJECT, SHRINK_GLOB, obj_to_any(obj));
}

/* globs have quantity 1 and size which varies by multiples of 20 in owt;
   they don't become tainted with age, but every 25 turns this timer runs
   and reduces owt by 1; when it hits 0, destroy the glob (if some other
   part of the program destroys it, the timer will be cancelled);
   note: timer keeps going if an object gets buried or scheduled to
   migrate to another level and can delete the glob in those states */
void
shrink_glob(
    anything *arg,    /* glob (in arg->a_obj) */
    long expire_time) /* turn the timer should have gone off; if less than
                       * current 'moves', we're making up for lost time
                       * after leaving and then returning to this level */
{
    char globnambuf[BUFSZ];
    struct obj *obj = arg->a_obj;
    int globloc = item_on_ice(obj);
    boolean ininv = (obj->where == OBJ_INVENT),
            shrink = FALSE, gone = FALSE, updinv = FALSE;
    struct obj *contnr = (obj->where == OBJ_CONTAINED) ? obj->ocontainer : 0,
               *topcontnr = 0;
    unsigned old_top_owt = 0;

    if (!obj->globby) {
        impossible("shrink_glob for non-glob [%d: %s]?",
                   obj->otyp, simpleonames(obj));
        return; /* old timer is gone, don't start a new one */
    }
    /* note: if check_glob() complains about a problem, the " obj " here
       will be replaced in the feedback with info about this glob */
    check_glob(obj, "shrink obj ");

    /*
     * If shrinkage occurred while we were on another level, catch up now.
     */
    if (expire_time < gm.moves && globloc != BURIED_UNDER_ICE) {
        /* number of units of weight to remove */
        long delta = (gm.moves - expire_time + 24L) / 25L,
             /* leftover amount to use for new timer */
             moddelta = 25L - (delta % 25L);

        if (globloc == SET_ON_ICE)
            delta = (delta + 2L) / 3L;

        if (delta >= (long) obj->owt) {
            /* gone; no newsym() or message here--forthcoming map update for
               level arrival is all that's needed */
            obj->owt = 0; /* not required; accurately reflects obj's state */
            shrinking_glob_gone(obj);
        } else {
            /* shrank but not gone; reduce remaining weight */
            obj->owt -= (unsigned) delta;
            /* when contained, update container's weight (recursively if
               nested); won't be in a container carried by hero (since
               catching up for lost time never applies in that situation)
               but might be in one on floor or one carried by a monster */
            if (contnr)
                container_weight(contnr);
            /* resume regular shrinking */
            start_glob_timeout(obj, moddelta);
        }
        return;
    }

    /*
     * When on ice, only shrink every third try.  If buried under ice,
     * don't shrink at all, similar to being contained in an ice box
     * except that the timer remains active.  [FIXME:  stop the timer
     * for obj in pool that becomes frozen, restart it if/when unburied.]
     *
     * If the glob is actively being eaten by hero, skip weight reduction
     * to avoid messing up the context.victual data (if/when eaten by a
     * monster, timer won't have a chance to run before meal is finished).
     */
    if (eating_glob(obj)
        || globloc == BURIED_UNDER_ICE
        || (globloc == SET_ON_ICE && (gm.moves % 3L) == 1L)) {
        /* schedule next shrink attempt; for the being eaten case, the
           glob and its timer might be deleted before this kicks in */
        start_glob_timeout(obj, 0L);
        return;
    }

    /* format "Your/Shk's/The [partly eaten] glob of <goo>" into
       globnambuf[] before shrinking the glob; Yname2() calls yname()
       which calls xname() which ordinarly leaves "partly eaten" to
       doname() rather than inserting that itself; ask xname() to add
       that when appropriate */
    iflags.partly_eaten_hack = TRUE;
    Strcpy(globnambuf, Yname2(obj));
    iflags.partly_eaten_hack = FALSE;

    if (obj->owt > 0) { /* sanity precaution */
        /* globs start out weighing 20 units; give two messages per glob,
           when going from 20 to 19 and from 10 to 9; a different message
           is given for going from 1 to 0 (gone) */
        unsigned basewt = objects[obj->otyp].oc_weight, /* 20 */
                 msgwt = (max(basewt, 1U) + 1U) / 2U; /* 10 */

        shrink = (obj->owt % msgwt) == 0;
        obj->owt -= 1;
        /* if glob is partly eaten, reduce the amount still available (but
           not all the way to 0 which would change it back to untouched) */
        if (obj->oeaten > 1)
            obj->oeaten -= 1;
    }
    gone = !obj->owt;

    /* timer might go off when the glob is migrating to another level and
       possibly delete it; messages are only given for in-open-inventory,
       inside-container-in-invent, and going away when can-see-on-floor */
    if (ininv) {
        if (shrink || gone)
            pline("%s %s.", globnambuf,
                  /* globs always have quantity 1 so we don't need otense()
                     because the verb always references a singular item */
                  gone ? "dissolves completely" : "shrinks");
        updinv = TRUE;
    } else if (contnr) {
        /* when in a container, it might be nested so find outermost one */
        topcontnr = contnr;
        while (topcontnr->where == OBJ_CONTAINED)
            topcontnr = topcontnr->ocontainer;
        /* obj's weight has been reduced, but weight(s) of enclosing
           container(s) haven't been adjusted for that yet */
        old_top_owt = topcontnr->owt;
        /* update those weights now; recursively updates nested containers */
        container_weight(contnr);

        if (topcontnr->where == OBJ_INVENT) {
            /* for regular containers, the weight will always be reduced
               when glob's weight has been reduced but we only say so
               when shrinking beneath a particular threshold (N*20 to
               (N-1)*20 + 19 or (N-1)*20 + 10 to (N-1)*20 + 9), or
               if we're going to report a change in carrying capacity;
               for a non-cursed bag of holding the total weight might not
               change because only a fraction of glob's weight is counted;
               however, always say the bag is lighter for the 'gone' case */
            if (gone || (shrink && topcontnr->owt != old_top_owt)
                || near_capacity() != go.oldcap)
                pline("%s %s%s lighter.", Yname2(topcontnr),
                      /* containers also always have quantity 1 */
                      (topcontnr->owt != old_top_owt) ? "becomes" : "seems",
                      /* TODO?  maybe also skip "slightly" if description
                         is changing (from "very large" to "large",
                         "large" to "medium", or "medium to "small") */
                      !gone ? " slightly" : "");
            updinv = TRUE;
        }
    }

    if (gone) {
        coordxy ox = 0, oy = 0;
        /* check location for visibility before destroying obj */
        boolean seeit = (obj->where == OBJ_FLOOR
                         && get_obj_location(obj, &ox, &oy, 0)
                         && cansee(ox, oy));

        /* weight has been reduced to 0 so destroy the glob */
        shrinking_glob_gone(obj);

        if (seeit) {
            newsym(ox, oy);
            if ((ox != u.ux || oy != u.uy) && !strncmp(globnambuf, "The ", 4))
                /* fortunately none of the glob adjectives warrant "An " */
                (void) strsubst(globnambuf, "The ", "A ");
            /* again, quantity is always 1 so no need for otense()/vtense() */
            pline("%s fades away.", globnambuf);
        }
    } else {
        /* schedule next shrink ~25 turns from now */
        start_glob_timeout(obj, 0L);
    }
    if (updinv) {
        update_inventory();
        (void) encumber_msg();
    }
}

/* a glob has shrunk away to nothing; handle owornmask, then delete glob */
static void
shrinking_glob_gone(struct obj *obj)
{
    xint16 owhere = obj->where;

    if (owhere == OBJ_INVENT) {
        if (obj->owornmask) {
            remove_worn_item(obj, FALSE);
            stop_occupation();
        }
        useupall(obj); /* freeinv()+obfree() */
    } else {
        if (owhere == OBJ_MIGRATING) {
            /* destination flag overloads owornmask; clear it so obfree()'s
               check for freeing a worn object doesn't get a false hit */
            obj->owornmask = 0L;
        } else if (owhere == OBJ_MINVENT) {
            /* monsters don't wield globs so this isn't strictly needed */
            if (obj->owornmask && obj == MON_WEP(obj->ocarry))
                setmnotwielded(obj->ocarry, obj); /* clears owornmask */
        }
        /* remove the glob from whatever list it's on and then delete it;
           if it's contained, obj_extract_self() will update the container's
           weight and if nested, the enclosing containers' weights too */
        obj_extract_self(obj);
        if (owhere == OBJ_FLOOR)
            maybe_unhide_at(obj->ox, obj->oy);
        obfree(obj, (struct obj *) 0);
    }
}

void
maybe_adjust_light(struct obj *obj, int old_range)
{
    char buf[BUFSZ];
    coordxy ox, oy;
    int new_range = arti_light_radius(obj), delta = new_range - old_range;

    /* radius of light emitting artifact varies by curse/bless state
       so will change after blessing or cursing */
    if (delta) {
        obj_adjust_light_radius(obj, new_range);
        /* simplifying assumptions:  hero is wielding or wearing this object;
           artifacts have to be in use to emit light and monsters' gear won't
           change bless or curse state */
        if (!Blind && get_obj_location(obj, &ox, &oy, 0)) {
            *buf = '\0';
            if (iflags.last_msg == PLNMSG_OBJ_GLOWS)
                /* we just saw "The <obj> glows <color>." from dipping */
                Strcpy(buf, (obj->quan == 1L) ? "It" : "They");
            else if (carried(obj) || cansee(ox, oy))
                Strcpy(buf, Yname2(obj));
            if (*buf) {
                /* initial activation says "dimly" if cursed,
                   "brightly" if uncursed, and "brilliantly" if blessed;
                   when changing intensity, using "less brightly" is
                   straightforward for dimming, but we need "brighter"
                   rather than "more brightly" for brightening; ugh */
                pline("%s %s %s%s.", buf, otense(obj, "shine"),
                      (abs(delta) > 1) ? "much " : "",
                      (delta > 0) ? "brighter" : "less brightly");
            }
        }
    }
}

/*
 *      bless(), curse(), unbless(), uncurse() -- any relevant message
 *      about glowing amber/black/&c should be delivered prior to calling
 *      these routines to make the actual curse/bless state change.
 */

void
bless(struct obj *otmp)
{
    int old_light = 0;

    if (otmp->oclass == COIN_CLASS)
        return;
    if (otmp->lamplit)
        old_light = arti_light_radius(otmp);
    otmp->cursed = 0;
    otmp->blessed = 1;
    if (carried(otmp) && confers_luck(otmp))
        set_moreluck();
    else if (otmp->otyp == BAG_OF_HOLDING)
        otmp->owt = weight(otmp);
    else if (otmp->otyp == FIGURINE && otmp->timed)
        (void) stop_timer(FIG_TRANSFORM, obj_to_any(otmp));
    if (otmp->lamplit)
        maybe_adjust_light(otmp, old_light);
    return;
}

void
unbless(struct obj *otmp)
{
    int old_light = 0;

    if (otmp->lamplit)
        old_light = arti_light_radius(otmp);
    otmp->blessed = 0;
    if (carried(otmp) && confers_luck(otmp))
        set_moreluck();
    else if (otmp->otyp == BAG_OF_HOLDING)
        otmp->owt = weight(otmp);
    if (otmp->lamplit)
        maybe_adjust_light(otmp, old_light);
}

void
curse(struct obj *otmp)
{
    unsigned already_cursed;
    int old_light = 0;

    if (otmp->oclass == COIN_CLASS)
        return;
    if (otmp->lamplit)
        old_light = arti_light_radius(otmp);
    already_cursed = otmp->cursed;
    otmp->blessed = 0;
    otmp->cursed = 1;
    /* welded two-handed weapon interferes with some armor removal */
    if (otmp == uwep && bimanual(uwep))
        reset_remarm();
    /* rules at top of wield.c state that twoweapon cannot be done
       with cursed alternate weapon */
    if (otmp == uswapwep && u.twoweap)
        drop_uswapwep();
    /* some cursed items need immediate updating */
    if (carried(otmp) && confers_luck(otmp)) {
        set_moreluck();
    } else if (otmp->otyp == BAG_OF_HOLDING) {
        otmp->owt = weight(otmp);
    } else if (otmp->otyp == FIGURINE) {
        if (otmp->corpsenm != NON_PM && !dead_species(otmp->corpsenm, TRUE)
            && (carried(otmp) || mcarried(otmp)))
            attach_fig_transform_timeout(otmp);
    } else if (otmp->oclass == SPBOOK_CLASS) {
        /* if book hero is reading becomes cursed, interrupt */
        if (!already_cursed)
            book_cursed(otmp);
    }
    if (otmp->lamplit)
        maybe_adjust_light(otmp, old_light);
    return;
}

void
uncurse(struct obj *otmp)
{
    int old_light = 0;

    if (otmp->lamplit)
        old_light = arti_light_radius(otmp);
    otmp->cursed = 0;
    if (carried(otmp) && confers_luck(otmp))
        set_moreluck();
    else if (otmp->otyp == BAG_OF_HOLDING)
        otmp->owt = weight(otmp);
    else if (otmp->otyp == FIGURINE && otmp->timed)
        (void) stop_timer(FIG_TRANSFORM, obj_to_any(otmp));
    if (otmp->lamplit)
        maybe_adjust_light(otmp, old_light);
    return;
}

void
blessorcurse(struct obj *otmp, int chance)
{
    if (otmp->blessed || otmp->cursed)
        return;

    if (!rn2(chance)) {
        if (!rn2(2)) {
            curse(otmp);
        } else {
            bless(otmp);
        }
    }
    return;
}

int
bcsign(struct obj *otmp)
{
    return (!!otmp->blessed - !!otmp->cursed);
}

/* set the object's bless/curse-state known flag */
void
set_bknown(
    struct obj *obj,
    unsigned int onoff) /* 1 or 0 */
{
    if (obj->bknown != onoff) {
        obj->bknown = onoff;
        if (obj->where == OBJ_INVENT && gm.moves > 1L)
            update_inventory();
    }
}

/*
 *  Calculate the weight of the given object.  This will recursively follow
 *  and calculate the weight of any containers.
 *
 *  Note:  It is possible to end up with an incorrect weight if some part
 *         of the code messes with a contained object and doesn't update the
 *         container's weight.
 *
 *  Note too: obj->owt is an unsigned int and objects[].oc_weight an
 *         unsigned short int, so weight() should probably be changed to
 *         use and return unsigned int instead of signed int.
 */
int
weight(struct obj *obj)
{
    int wt = (int) objects[obj->otyp].oc_weight; /* weight of 1 'otyp' */

    if (obj->quan < 1L) {
        impossible("Calculating weight of %ld %s?",
                   obj->quan, simpleonames(obj));
        return 0;
    }
    /* glob absorpsion means that merging globs combines their weight
       while quantity stays 1; mksobj(), obj_absorb(), and shrink_glob()
       manage glob->owt and there is nothing for weight() to do except
       return the current value as-is */
    if (obj->globby) {
        /* 3.7: in 3.6.x this checked for owt==0 and then used
           owt as-is when non-zero or objects[].oc_weight if zero;
           we don't do that anymore because it confused calculating
           the weight of a container when a glob inside shrank down
           to 0 and was about to be deleted [mksobj() now initializes
           owt for globs sooner and the subsequent o->owt = weight(o)
           general initialization is benignly redundant for globs] */
        return (int) obj->owt;
    }
    if (Is_container(obj) || obj->otyp == STATUE) {
        struct obj *contents;
        register int cwt = 0;

        if (obj->otyp == STATUE && obj->corpsenm >= LOW_PM)
            wt = (int) obj->quan * ((int) mons[obj->corpsenm].cwt * 3 / 2);

        for (contents = obj->cobj; contents; contents = contents->nobj)
            cwt += weight(contents);
        /*
         *  The weight of bags of holding is calculated as the weight
         *  of the bag plus the weight of the bag's contents modified
         *  as follows:
         *
         *      Bag status      Weight of contents
         *      ----------      ------------------
         *      cursed                  2x
         *      blessed                 x/4 [rounded up: (x+3)/4]
         *      otherwise               x/2 [rounded up: (x+1)/2]
         *
         *  The macro DELTA_CWT in pickup.c also implements these
         *  weight equations.
         */
        if (obj->otyp == BAG_OF_HOLDING)
            cwt = obj->cursed ? (cwt * 2)
                  : obj->blessed ? ((cwt + 3) / 4)
                    : ((cwt + 1) / 2); /* uncursed */

        return wt + cwt;
    }
    if (obj->otyp == CORPSE && obj->corpsenm >= LOW_PM) {
        long long_wt = obj->quan * (long) mons[obj->corpsenm].cwt;

        wt = (long_wt > LARGEST_INT) ? LARGEST_INT : (int) long_wt;
        if (obj->oeaten)
            wt = eaten_stat(wt, obj);
        return wt;
    } else if (obj->oclass == FOOD_CLASS && obj->oeaten) {
        return eaten_stat((int) obj->quan * wt, obj);
    } else if (obj->oclass == COIN_CLASS) {
        /* 3.7: always weigh at least 1 unit; used to yield 0 for 1..49 */
        wt = (int) ((obj->quan + 50L) / 100L);
        return max(wt, 1);
    } else if (obj->otyp == HEAVY_IRON_BALL && obj->owt != 0) {
        return (int) obj->owt; /* kludge for "very" heavy iron ball */
    } else if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->spe) {
        return wt + obj->spe * (int) objects[TALLOW_CANDLE].oc_weight;
    }
    return (wt ? wt * (int) obj->quan : ((int) obj->quan + 1) >> 1);
}

static const int treefruits[] = {
    APPLE, ORANGE, PEAR, BANANA, EUCALYPTUS_LEAF
};

/* called when a tree is kicked; never returns Null */
struct obj *
rnd_treefruit_at(coordxy x, coordxy y)
{
    return mksobj_at(treefruits[rn2(SIZE(treefruits))], x, y, TRUE, FALSE);
}

/* create a stack of N gold pieces; never returns Null */
struct obj *
mkgold(long amount, coordxy x, coordxy y)
{
    struct obj *gold = g_at(x, y);

    if (amount <= 0L) {
        long mul = rnd(30 / max(12-depth(&u.uz), 2));

        amount = (long) (1 + rnd(level_difficulty() + 2) * mul);
    }
    if (gold) {
        gold->quan += amount;
    } else {
        gold = mksobj_at(GOLD_PIECE, x, y, TRUE, FALSE);
        gold->quan = amount;
    }
    gold->owt = weight(gold);
    return gold;
}

/* return TRUE if the corpse has special timing;
   lizards and lichen don't rot, trolls and Riders auto-revive */
#define special_corpse(num) \
    (((num) == PM_LIZARD || (num) == PM_LICHEN)                 \
     || (mons[num].mlet == S_TROLL || is_rider(&mons[num])))

/* mkcorpstat: make a corpse or statue; never returns Null.
 *
 * OEXTRA note: Passing mtmp causes mtraits to be saved
 * even if ptr passed as well, but ptr is always used for
 * the corpse type (corpsenm). That allows the corpse type
 * to be different from the original monster,
 *      i.e.  vampire -> human corpse
 * yet still allow restoration of the original monster upon
 * resurrection.
 */
struct obj *
mkcorpstat(
    int objtype,          /* CORPSE or STATUE */
    struct monst *mtmp,   /* dead monster, might be Null */
    struct permonst *ptr, /* if non-Null, overrides mtmp->mndx */
    coordxy x, coordxy y,         /* where to place corpse; <0,0> => random */
    unsigned corpstatflags)
{
    struct obj *otmp;
    boolean init = ((corpstatflags & CORPSTAT_INIT) != 0);

    if (objtype != CORPSE && objtype != STATUE)
        impossible("making corpstat type %d", objtype);
    if (x == 0 && y == 0) { /* special case - random placement */
        otmp = mksobj(objtype, init, FALSE);
        (void) rloco(otmp);
    } else {
        otmp = mksobj_at(objtype, x, y, init, FALSE);
    }
    /* record gender and 'historic statue' in overloaded enchantment field */
    otmp->spe = (corpstatflags & CORPSTAT_SPE_VAL);
    otmp->norevive = gm.mkcorpstat_norevive; /* via envrmt rather than flags */

    /* when 'mtmp' is non-null save the monster's details with the
       corpse or statue; it will also force the 'ptr' override below */
    if (mtmp) {
        /* save_mtraits updates otmp->oextra->omonst in place */
        (void) save_mtraits(otmp, mtmp);

        if (!ptr)
            ptr = mtmp->data;

        /* don't give a revive timer to a cancelled troll's corpse */
        if (mtmp->mcan && !is_rider(ptr))
            otmp->norevive = 1;
    }

    /* when 'ptr' is non-null it comes from our caller or from 'mtmp';
       override mkobjs()'s initialization of a random monster type */
    if (ptr) {
        int old_corpsenm = otmp->corpsenm;

        otmp->corpsenm = monsndx(ptr);
        otmp->owt = weight(otmp);
        if (otmp->otyp == CORPSE && (gz.zombify || special_corpse(old_corpsenm)
                                     || special_corpse(otmp->corpsenm))) {
            obj_stop_timers(otmp);
            start_corpse_timeout(otmp);
        }
    }
    return otmp;
}

/*
 * Return the type of monster that this corpse will
 * revive as, even if it has a monster structure
 * attached to it. In that case, you can't just
 * use obj->corpsenm, because the stored monster
 * type can, and often is, different.
 * The return value is an index into mons[].
 */
int
corpse_revive_type(struct obj *obj)
{
    int revivetype = obj->corpsenm;
    struct monst *mtmp;

    if (has_omonst(obj) && ((mtmp = get_mtraits(obj, FALSE)) != 0)) {
        /* mtmp is a temporary pointer to a monster's stored
        attributes, not a real monster */
        revivetype = mtmp->mnum;
    }
    return revivetype;
}

/*
 * Attach a monster id to an object, to provide
 * a lasting association between the two.
 */
struct obj *
obj_attach_mid(struct obj *obj, unsigned int mid)
{
    if (!mid || !obj)
        return (struct obj *) 0;
    newomid(obj);
    OMID(obj) = mid;
    return obj;
}

static struct obj *
save_mtraits(struct obj *obj, struct monst *mtmp)
{
    if (mtmp->ispriest)
        forget_temple_entry(mtmp); /* EPRI() */
    if (!has_omonst(obj))
        newomonst(obj);
    if (has_omonst(obj)) {
        int baselevel = mtmp->data->mlevel;
        struct monst *mtmp2 = OMONST(obj);

        *mtmp2 = *mtmp;
        mtmp2->mextra = (struct mextra *) 0;
        if (mtmp->data)
            mtmp2->mnum = monsndx(mtmp->data);
        /* invalidate pointers */
        /* m_id is needed to know if this is a revived quest leader */
        /* but m_id must be cleared when loading bones */
        mtmp2->nmon = (struct monst *) 0;
        mtmp2->data = (struct permonst *) 0;
        mtmp2->minvent = (struct obj *) 0;
        if (mtmp->mextra)
            copy_mextra(mtmp2, mtmp);
        /* if mtmp is a long worm with segments, its saved traits will
           be one without any segments */
        mtmp2->wormno = 0;
        /* mtmp might have been killed by repeated life draining; make sure
           mtmp2 can survive if revived ('baselevel' will be 0 for 1d4 mon) */
        if (mtmp2->mhpmax <= baselevel)
            mtmp2->mhpmax = baselevel + 1;
        /* mtmp is assumed to be dead but we don't kill it or its saved
           traits, just force those to have a sane value for current HP */
        if (mtmp2->mhp > mtmp2->mhpmax)
            mtmp2->mhp = mtmp2->mhpmax;
        if (mtmp2->mhp < 1)
            mtmp2->mhp = 0;
        mtmp2->mstate &= ~MON_DETACH;
    }
    return obj;
}

/* returns a pointer to a new monst structure based on
 * the one contained within the obj.
 */
struct monst *
get_mtraits(struct obj *obj, boolean copyof)
{
    struct monst *mtmp = (struct monst *) 0;
    struct monst *mnew = (struct monst *) 0;

    if (has_omonst(obj))
        mtmp = OMONST(obj);
    if (mtmp) {
        if (copyof) {
            mnew = newmonst();
            *mnew = *mtmp;
            mnew->mextra = (struct mextra *) 0;
            if (mtmp->mextra)
                copy_mextra(mnew, mtmp);
        } else {
            /* Never insert this returned pointer into mon chains! */
            mnew = mtmp;
        }
        mnew->data = &mons[mnew->mnum];
    }
    return mnew;
}

/* make an object named after someone listed in the scoreboard file;
   never returns Null */
struct obj *
mk_tt_object(
    int objtype, /* CORPSE or STATUE */
    coordxy x, coordxy y)
{
    struct obj *otmp;
    boolean initialize_it;

    /* player statues never contain books */
    initialize_it = (objtype != STATUE);
    otmp = mksobj_at(objtype, x, y, initialize_it, FALSE);

    /* tt_oname() will return null if the scoreboard is empty, which in
       turn leaves the random corpsenm value; force it to match a player */
    if (!tt_oname(otmp)) {
        int pm = rn1(PM_WIZARD - PM_ARCHEOLOGIST + 1, PM_ARCHEOLOGIST);

        /* update weight for either, force timer sanity for corpses */
        set_corpsenm(otmp, pm);
    }

    return otmp;
}

/* make a new corpse or statue, uninitialized if a statue (i.e. no books);
   never returns Null */
struct obj *
mk_named_object(
    int objtype, /* CORPSE or STATUE */
    struct permonst *ptr,
    coordxy x, coordxy y,
    const char *nm)
{
    struct obj *otmp;
    unsigned corpstatflags = (objtype != STATUE) ? CORPSTAT_INIT
                                                 : CORPSTAT_NONE;

    otmp = mkcorpstat(objtype, (struct monst *) 0, ptr, x, y, corpstatflags);
    if (nm)
        otmp = oname(otmp, nm, ONAME_NO_FLAGS);
    return otmp;
}

boolean
is_flammable(struct obj *otmp)
{
    int otyp = otmp->otyp;
    int omat = objects[otyp].oc_material;

    /* Candles can be burned, but they're not flammable in the sense that
     * they can't get fire damage and it makes no sense for them to be
     * fireproofed.
     */
    if (Is_candle(otmp))
        return FALSE;

    if (objects[otyp].oc_oprop == FIRE_RES || otyp == WAN_FIRE)
        return FALSE;

    return (boolean) ((omat <= WOOD && omat != LIQUID) || omat == PLASTIC);
}

boolean
is_rottable(struct obj *otmp)
{
    int otyp = otmp->otyp;

    return (boolean) (objects[otyp].oc_material <= WOOD
                      && objects[otyp].oc_material != LIQUID);
}

/*
 * These routines maintain the single-linked lists headed in level.objects[][]
 * and threaded through the nexthere fields in the object-instance structure.
 */

/* put the object at the given location */
void
place_object(struct obj *otmp, coordxy x, coordxy y)
{
    register struct obj *otmp2;

    if (!isok(x, y)) { /* validate location */
        void (*func)(const char *, ...) PRINTF_F_PTR(1, 2);

        func = (x < 0 || y < 0 || x > COLNO - 1 || y > ROWNO - 1) ? panic
               : impossible;
        (*func)("place_object: \"%s\" [%d] off map <%d,%d>",
                safe_typename(otmp->otyp), otmp->where, x, y);
    }
    if (otmp->where != OBJ_FREE)
        panic("place_object: obj \"%s\" [%d] not free",
              safe_typename(otmp->otyp), otmp->where);

    otmp2 = gl.level.objects[x][y];

    obj_no_longer_held(otmp);
    if (otmp->otyp == BOULDER) {
        if (!otmp2 || otmp2->otyp != BOULDER)
            block_point(x, y); /* vision */
    }

    /* non-boulder object goes under boulders so that map will show boulder
       here without display code needing to traverse pile chain to find one */
    if (otmp2 && otmp2->otyp == BOULDER && otmp->otyp != BOULDER) {
        /* 3.6.3: put otmp under last consecutive boulder rather than under
           just the first one */
        while (otmp2->nexthere && otmp2->nexthere->otyp == BOULDER)
            otmp2 = otmp2->nexthere;
        otmp->nexthere = otmp2->nexthere;
        otmp2->nexthere = otmp;
    } else {
        /* put on top of current pile */
        otmp->nexthere = otmp2;
        gl.level.objects[x][y] = otmp;
    }

    /* set the object's new location */
    otmp->ox = x;
    otmp->oy = y;
    otmp->where = OBJ_FLOOR;

    /* if placed outside of shop, no_charge is no longer applicable */
    if (otmp->no_charge && !costly_spot(x, y)
        && !costly_adjacent(find_objowner(otmp, x, y), x, y))
        otmp->no_charge = 0;

    /* add to floor chain */
    otmp->nobj = fobj;
    fobj = otmp;
    if (otmp->timed)
        obj_timer_checks(otmp, x, y, 0);
}

/* tear down the object pile at <x,y> and create it again, so that any
   boulders which are present get forced to the top */
void
recreate_pile_at(coordxy x, coordxy y)
{
    struct obj *otmp, *next_obj, *reversed = 0;

    /* remove all objects at <x,y>, saving a reversed temporary list */
    for (otmp = gl.level.objects[x][y]; otmp; otmp = next_obj) {
        next_obj = otmp->nexthere;
        remove_object(otmp); /* obj_extract_self() for floor */
        otmp->nobj = reversed;
        reversed = otmp;
    }
    /* pile at <tx,ty> is now empty; create new one, re-reversing to restore
       original order; place_object() handles making boulders be on top */
    for (otmp = reversed; otmp; otmp = next_obj) {
        next_obj = otmp->nobj;
        otmp->nobj = 0; /* obj->where is OBJ_FREE */
        place_object(otmp, x, y);
    }
}

#define ROT_ICE_ADJUSTMENT 2 /* rotting on ice takes 2 times as long */

/* If ice was affecting any objects correct that now
 * Also used for starting ice effects too. [zap.c]
 */
void
obj_ice_effects(coordxy x, coordxy y, boolean do_buried)
{
    struct obj *otmp;

    for (otmp = gl.level.objects[x][y]; otmp; otmp = otmp->nexthere) {
        if (otmp->timed)
            obj_timer_checks(otmp, x, y, 0);
    }
    if (do_buried) {
        for (otmp = gl.level.buriedobjlist; otmp; otmp = otmp->nobj) {
            if (otmp->ox == x && otmp->oy == y) {
                if (otmp->timed)
                    obj_timer_checks(otmp, x, y, 0);
            }
        }
    }
}

/*
 * Returns an obj->age for a corpse object on ice, that would be the
 * actual obj->age if the corpse had just been lifted from the ice.
 * This is useful when just using obj->age in a check or calculation because
 * rot timers pertaining to the object don't have to be stopped and
 * restarted etc.
 */
long
peek_at_iced_corpse_age(struct obj *otmp)
{
    long age, retval = otmp->age;

    if (otmp->otyp == CORPSE && otmp->on_ice) {
        /* Adjust the age; must be same as obj_timer_checks() for off ice*/
        age = gm.moves - otmp->age;
        retval += age * (ROT_ICE_ADJUSTMENT - 1) / ROT_ICE_ADJUSTMENT;
        debugpline3(
          "The %s age has ice modifications: otmp->age = %ld, returning %ld.",
                    s_suffix(doname(otmp)), otmp->age, retval);
        debugpline1("Effective age of corpse: %ld.", gm.moves - retval);
    }
    return retval;
}

static void
obj_timer_checks(
    struct obj *otmp,
    coordxy x, coordxy y,
    int force) /* 0 = no force so do checks, <0 = force off, >0 force on */
{
    long tleft = 0L;
    short action = ROT_CORPSE;
    boolean restart_timer = FALSE;
    boolean on_floor = (otmp->where == OBJ_FLOOR);
    boolean buried = (otmp->where == OBJ_BURIED);

    /* Check for corpses just placed on or in ice */
    if (otmp->otyp == CORPSE && (on_floor || buried) && is_ice(x, y)) {
        tleft = stop_timer(action, obj_to_any(otmp));
        if (tleft == 0L) {
            action = REVIVE_MON;
            tleft = stop_timer(action, obj_to_any(otmp));
        }
        if (tleft != 0L) {
            long age;

            /* mark the corpse as being on ice */
            otmp->on_ice = 1;
            debugpline3("%s is now on ice at <%d,%d>.", The(xname(otmp)), x,
                        y);
            /* Adjust the time remaining */
            tleft *= ROT_ICE_ADJUSTMENT;
            restart_timer = TRUE;
            /* Adjust the age; time spent off ice needs to be multiplied
               by the ice adjustment and subtracted from the age so that
               later calculations behave as if it had been on ice during
               that time (longwinded way of saying this is the inverse
               of removing it from the ice and of peeking at its age). */
            age = gm.moves - otmp->age;
            otmp->age = gm.moves - (age * ROT_ICE_ADJUSTMENT);
        }

    /* Check for corpses coming off ice */
    } else if (force < 0 || (otmp->otyp == CORPSE && otmp->on_ice
                             && !((on_floor || buried) && is_ice(x, y)))) {
        tleft = stop_timer(action, obj_to_any(otmp));
        if (tleft == 0L) {
            action = REVIVE_MON;
            tleft = stop_timer(action, obj_to_any(otmp));
        }
        if (tleft != 0L) {
            long age;

            otmp->on_ice = 0;
            debugpline3("%s is no longer on ice at <%d,%d>.",
                        The(xname(otmp)), x, y);
            /* Adjust the remaining time */
            tleft /= ROT_ICE_ADJUSTMENT;
            restart_timer = TRUE;
            /* Adjust the age */
            age = gm.moves - otmp->age;
            otmp->age += age * (ROT_ICE_ADJUSTMENT - 1) / ROT_ICE_ADJUSTMENT;
        }
    }

    /* now re-start the timer with the appropriate modifications */
    if (restart_timer)
        (void) start_timer(tleft, TIMER_OBJECT, action, obj_to_any(otmp));
}

#undef ROT_ICE_ADJUSTMENT

void
remove_object(struct obj *otmp)
{
    coordxy x = otmp->ox;
    coordxy y = otmp->oy;

    if (otmp->where != OBJ_FLOOR)
        panic("remove_object: obj not on floor");
    extract_nexthere(otmp, &gl.level.objects[x][y]);
    extract_nobj(otmp, &fobj);
    /* update vision iff this was the only boulder at its spot */
    if (otmp->otyp == BOULDER && !sobj_at(BOULDER, x, y))
        unblock_point(x, y); /* vision */
    if (otmp->timed)
        obj_timer_checks(otmp, x, y, 0);
}

/* throw away all of a monster's inventory */
void
discard_minvent(struct monst *mtmp, boolean uncreate_artifacts)
{
    struct obj *otmp;

    while ((otmp = mtmp->minvent) != 0) {
        /* this has now become very similar to m_useupall()... */
        extract_from_minvent(mtmp, otmp, TRUE, TRUE);
        if (uncreate_artifacts && otmp->oartifact)
            artifact_exists(otmp, safe_oname(otmp), FALSE, ONAME_NO_FLAGS);
        obfree(otmp, (struct obj *) 0); /* dealloc_obj() isn't sufficient */
    }
}

/*
 * Free obj from whatever list it is on in preparation for deleting it
 * or moving it elsewhere; obj->where will end up set to OBJ_FREE.
 * Doesn't handle unwearing of objects in hero's or monsters' inventories.
 *
 * Object positions:
 *      OBJ_FREE        not on any list
 *      OBJ_FLOOR       fobj, level.locations[][] chains (use remove_object)
 *      OBJ_CONTAINED   cobj chain of container object
 *      OBJ_INVENT      hero's invent chain (use freeinv)
 *      OBJ_MINVENT     monster's invent chain
 *      OBJ_MIGRATING   migrating chain
 *      OBJ_BURIED      level.buriedobjs chain
 *      OBJ_ONBILL      on gb.billobjs chain
 *      OBJ_LUAFREE     obj is dealloc'd from core, but still used by lua
 */
void
obj_extract_self(struct obj *obj)
{
    switch (obj->where) {
    case OBJ_FREE:
    case OBJ_LUAFREE:
        break;
    case OBJ_FLOOR:
        remove_object(obj);
        break;
    case OBJ_CONTAINED:
        extract_nobj(obj, &obj->ocontainer->cobj);
        container_weight(obj->ocontainer);
        obj->ocontainer = (struct obj *) 0; /* clear stale back-link */
        break;
    case OBJ_INVENT:
        freeinv(obj);
        break;
    case OBJ_MINVENT:
        extract_nobj(obj, &obj->ocarry->minvent);
        obj->ocarry = (struct monst *) 0; /* clear stale back-link */
        break;
    case OBJ_MIGRATING:
        extract_nobj(obj, &gm.migrating_objs);
        break;
    case OBJ_BURIED:
        extract_nobj(obj, &gl.level.buriedobjlist);
        break;
    case OBJ_ONBILL:
        extract_nobj(obj, &gb.billobjs);
        break;
    default:
        panic("obj_extract_self");
        break;
    }
}

/* Extract the given object from the chain, following nobj chain. */
void
extract_nobj(struct obj *obj, struct obj **head_ptr)
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nobj) {
        if (curr == obj) {
            if (prev)
                prev->nobj = curr->nobj;
            else
                *head_ptr = curr->nobj;
            break;
        }
    }
    if (!curr)
        panic("extract_nobj: object lost");
    obj->where = OBJ_FREE;
    obj->nobj = (struct obj *) 0;
}

/*
 * Extract the given object from the chain, following nexthere chain.
 *
 * This does not set obj->where, this function is expected to be called
 * in tandem with extract_nobj, which does set it.
 */
void
extract_nexthere(struct obj *obj, struct obj **head_ptr)
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nexthere) {
        if (curr == obj) {
            if (prev)
                prev->nexthere = curr->nexthere;
            else
                *head_ptr = curr->nexthere;
            break;
        }
    }
    if (!curr)
        panic("extract_nexthere: object lost");
    obj->nexthere = (struct obj *) 0;
}

/*
 * Add obj to mon's inventory.  If obj is able to merge with something already
 * in the inventory, then the passed obj is deleted and 1 is returned.
 * Otherwise 0 is returned.
 */
int
add_to_minv(struct monst *mon, struct obj *obj)
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
        panic("add_to_minv: obj not free");

    /* merge if possible */
    for (otmp = mon->minvent; otmp; otmp = otmp->nobj)
        if (merged(&otmp, &obj))
            return 1; /* obj merged and then free'd */
    /* else insert; don't bother forcing it to end of chain */
    obj->where = OBJ_MINVENT;
    obj->ocarry = mon;
    obj->nobj = mon->minvent;
    mon->minvent = obj;
    return 0; /* obj on mon's inventory chain */
}

/*
 * Add obj to container, make sure obj is "free".  Returns (merged) obj.
 * The input obj may be deleted in the process.
 */
struct obj *
add_to_container(struct obj *container, struct obj *obj)
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
        panic("add_to_container: obj not free");
    if (container->where != OBJ_INVENT && container->where != OBJ_MINVENT)
        obj_no_longer_held(obj);

    /* merge if possible */
    for (otmp = container->cobj; otmp; otmp = otmp->nobj)
        if (merged(&otmp, &obj))
            return otmp;

    obj->where = OBJ_CONTAINED;
    obj->ocontainer = container;
    obj->nobj = container->cobj;
    container->cobj = obj;
    return obj;
}

void
add_to_migration(struct obj *obj)
{
    if (obj->where != OBJ_FREE)
        panic("add_to_migration: obj not free");

    if (obj->unpaid) /* caller should have changed unpaid item to stolen */
        impossible("unpaid object migrating to another level? [%s]",
                   simpleonames(obj));
    obj->no_charge = 0; /* was only relevant while inside a shop */

    /* lock picking context becomes stale if it's for this object */
    if (Is_container(obj))
        maybe_reset_pick(obj);

    obj->where = OBJ_MIGRATING;
    obj->nobj = gm.migrating_objs;
    obj->omigr_from_dnum = u.uz.dnum;
    obj->omigr_from_dlevel = u.uz.dlevel;
    gm.migrating_objs = obj;
}

void
add_to_buried(struct obj *obj)
{
    if (obj->where != OBJ_FREE)
        panic("add_to_buried: obj not free");

    obj->where = OBJ_BURIED;
    obj->nobj = gl.level.buriedobjlist;
    gl.level.buriedobjlist = obj;
}

/* Recalculate the weight of this container and all of _its_ containers. */
static void
container_weight(struct obj *container)
{
    container->owt = weight(container);
    if (container->where == OBJ_CONTAINED)
        container_weight(container->ocontainer);
    /*
        else if (container->where == OBJ_INVENT)
        recalculate load delay here ???
    */
}

/*
 * Deallocate the object.  _All_ objects should be run through here for
 * them to be deallocated.
 */
void
dealloc_obj(struct obj *obj)
{
    if (obj->where != OBJ_FREE && obj->where != OBJ_LUAFREE)
        panic("dealloc_obj: obj not free");
    if (obj->nobj)
        panic("dealloc_obj with nobj");
    if (obj->cobj)
        panic("dealloc_obj with cobj");

    /* free up any timers attached to the object */
    if (obj->timed)
        obj_stop_timers(obj);

    /*
     * Free up any light sources attached to the object.
     *
     * We may want to just call del_light_source() without any
     * checks (requires a code change there).  Otherwise this
     * list must track all objects that can have a light source
     * attached to it (and also requires lamplit to be set).
     */
    if (obj_sheds_light(obj)) {
        del_light_source(LS_OBJECT, obj_to_any(obj));
        obj->lamplit = 0;
    }

    if (obj == gt.thrownobj)
        gt.thrownobj = 0;
    if (obj == gk.kickedobj)
        gk.kickedobj = 0;

    if (obj->oextra)
        dealloc_oextra(obj);
    if (obj->lua_ref_cnt) {
        /* obj is referenced from a lua script, let lua gc free it */
        obj->where = OBJ_LUAFREE;
        return;
    }
    free((genericptr_t) obj);
}

/* create an object from a horn of plenty; mirrors bagotricks(makemon.c) */
int
hornoplenty(
    struct obj *horn,
    boolean tipping, /* caller emptying entire contents; affects shop mesgs */
    struct obj *targetbox) /* if non-Null, container to tip into */
{
    int objcount = 0;

    if (!horn || horn->otyp != HORN_OF_PLENTY) {
        impossible("bad horn o' plenty");
    } else if (horn->spe < 1) {
        pline1(nothing_happens);
        if (!horn->cknown) {
            horn->cknown = 1;
            update_inventory();
        }
    } else {
        struct obj *obj;
        const char *what;

        consume_obj_charge(horn, !tipping);
        if (!rn2(13)) {
            obj = mkobj(POTION_CLASS, FALSE);
            if (objects[obj->otyp].oc_magic)
                do {
                    obj->otyp = rnd_class(POT_BOOZE, POT_WATER);
                } while (obj->otyp == POT_SICKNESS);
            what = (obj->quan > 1L) ? "Some potions" : "A potion";
        } else {
            obj = mkobj(FOOD_CLASS, FALSE);
            if (obj->otyp == FOOD_RATION && !rn2(7))
                obj->otyp = LUMP_OF_ROYAL_JELLY;
            what = "Some food";
        }
        ++objcount;
        pline("%s %s out.", what, vtense(what, "spill"));
        obj->blessed = horn->blessed;
        obj->cursed = horn->cursed;
        obj->owt = weight(obj);
        /* using a shop's horn of plenty entails a usage fee and also
           confers ownership of the created item to the shopkeeper */
        if (horn->unpaid)
            addtobill(obj, FALSE, FALSE, tipping);
        /* if it ended up on bill, we don't want "(unpaid, N zorkids)"
           being included in its formatted name during next message */
        iflags.suppress_price++;
        if (!tipping) {
            obj = hold_another_object(obj,
                                      u.uswallow
                                        ? "Oops!  %s out of your reach!"
                                        : (Is_airlevel(&u.uz)
                                           || Is_waterlevel(&u.uz)
                                           || levl[u.ux][u.uy].typ < IRONBARS
                                           || levl[u.ux][u.uy].typ >= ICE)
                                          ? "Oops!  %s away from you!"
                                          : "Oops!  %s to the floor!",
                                      The(aobjnam(obj, "slip")), (char *) 0);
            nhUse(obj);
        } else if (targetbox) {
            add_to_container(targetbox, obj);
            /* add to container doesn't update the weight */
            targetbox->owt = weight(targetbox);
            /* item still in magic horn was weightless; when it's now in
               a carried container, hero's encumbrance could change */
            if (carried(targetbox)) {
                (void) encumber_msg();
                update_inventory(); /* for contents count or wizweight */
            }
        } else {
            /* assumes this is taking place at hero's location */
            if (!can_reach_floor(TRUE)) {
                hitfloor(obj, TRUE); /* does altar check, message, drop */
            } else {
                if (IS_ALTAR(levl[u.ux][u.uy].typ))
                    doaltarobj(obj); /* does its own drop message */
                else
                    pline("%s %s to the %s.", Doname2(obj),
                          otense(obj, "drop"), surface(u.ux, u.uy));
                dropy(obj);
            }
        }
        iflags.suppress_price--;
        if (horn->dknown)
            makeknown(HORN_OF_PLENTY);
    }
    return objcount;
}

/* support for wizard-mode's `sanity_check' option */

static const char NEARDATA /* pline formats for insane_object() */
    ofmt0[] = "%s obj %s %s: %s",
    ofmt3[] = "%s [not null] %s %s: %s",
    /* " held by mon %p (%s)" will be appended, filled by M,mon_nam(M) */
    mfmt1[] = "%s obj %s %s (%s)",
    mfmt2[] = "%s obj %s %s (%s) *not*";

/* Check all object lists for consistency. */
void
obj_sanity_check(void)
{
    coordxy x, y;
    struct obj *obj, *otop, *prevo;

    objlist_sanity(fobj, OBJ_FLOOR, "floor sanity");

    /* check that the map's record of floor objects is consistent;
       those objects should have already been sanity checked via
       the floor list so container contents are skipped here */
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            char at_fmt[BUFSZ];

            otop = gl.level.objects[x][y];
            prevo = 0;
            for (obj = otop; obj; prevo = obj, obj = prevo->nexthere) {
                /* <ox,oy> should match <x,y>; <0,*> should always be empty */
                if (obj->where != OBJ_FLOOR || x == 0
                    || obj->ox != x || obj->oy != y) {
                    Sprintf(at_fmt, "%%s obj@<%d,%d> %%s %%s: %%s@<%d,%d>",
                            x, y, obj->ox, obj->oy);
                    insane_object(obj, at_fmt, "location sanity",
                                  (struct monst *) 0);

                /* when one or more boulders are present, they should always
                   be at the top of their pile; also never in water or lava */
                } else if (obj->otyp == BOULDER) {
                    if (prevo && prevo->otyp != BOULDER) {
                        Sprintf(at_fmt,
                                "%%s boulder@<%d,%d> %%s %%s: not on top",
                                x, y);
                        insane_object(obj, at_fmt, "boulder sanity",
                                      (struct monst *) 0);
                    }
                    if (is_pool_or_lava(x, y)) {
                        Sprintf(at_fmt,
                                "%%s boulder@<%d,%d> %%s %%s: on/in %s",
                                x, y, is_pool(x, y) ? "water" : "lava");
                        insane_object(obj, at_fmt, "boulder sanity",
                                      (struct monst *) 0);
                    }
                }
            }
        }

    objlist_sanity(gi.invent, OBJ_INVENT, "invent sanity");
    objlist_sanity(gm.migrating_objs, OBJ_MIGRATING, "migrating sanity");
    objlist_sanity(gl.level.buriedobjlist, OBJ_BURIED, "buried sanity");
    objlist_sanity(gb.billobjs, OBJ_ONBILL, "bill sanity");

    mon_obj_sanity(fmon, "minvent sanity");
    mon_obj_sanity(gm.migrating_mons, "migrating minvent sanity");
    /* monsters temporarily in transit;
       they should have arrived with hero by the time we get called */
    if (gm.mydogs) {
        impossible("gm.mydogs sanity [not empty]");
        mon_obj_sanity(gm.mydogs, "mydogs minvent sanity");
    }

    /* objects temporarily freed from invent/floor lists;
       they should have arrived somewhere by the time we get called */
    if (gt.thrownobj)
        insane_object(gt.thrownobj, ofmt3, "gt.thrownobj sanity",
                      (struct monst *) 0);
    if (gk.kickedobj)
        insane_object(gk.kickedobj, ofmt3, "gk.kickedobj sanity",
                      (struct monst *) 0);
    /* gc.current_wand isn't removed from invent while in use, but should
       be Null between moves when we're called */
    if (gc.current_wand)
        insane_object(gc.current_wand, ofmt3, "gc.current_wand sanity",
                      (struct monst *) 0);
}

/* sanity check for objects on specified list (fobj, &c) */
static void
objlist_sanity(struct obj *objlist, int wheretype, const char *mesg)
{
    struct obj *obj;

    for (obj = objlist; obj; obj = obj->nobj) {
        if (obj->where != wheretype)
            insane_object(obj, ofmt0, mesg, (struct monst *) 0);
        if (Has_contents(obj)) {
            if (wheretype == OBJ_ONBILL)
                /* containers on shop bill should always be empty */
                insane_object(obj, "%s obj contains something! %s %s: %s",
                              mesg, (struct monst *) 0);
            check_contained(obj, mesg);
        }
        if (obj->unpaid || obj->no_charge) {
            shop_obj_sanity(obj, mesg);
        }
        if (obj->owornmask) {
            char maskbuf[40];
            boolean bc_ok = FALSE;

            switch (obj->where) {
            case OBJ_INVENT:
            case OBJ_MINVENT:
                sanity_check_worn(obj);
                break;
            case OBJ_MIGRATING:
                /* migrating objects overload the owornmask field
                   with a destination code; skip attempt to check it */
                break;
            case OBJ_FLOOR:
                /* note: ball and chain can also be OBJ_FREE, but not across
                   turns so this sanity check shouldn't encounter that */
                bc_ok = TRUE;
            /*FALLTHRU*/
            default:
                if ((obj != uchain && obj != uball) || !bc_ok) {
                    /* discovered an object not in inventory which
                       erroneously has worn mask set */
                    Sprintf(maskbuf, "worn mask 0x%08lx", obj->owornmask);
                    insane_object(obj, ofmt0, maskbuf, (struct monst *) 0);
                }
                break;
            }
        }
        if (obj->globby)
            check_glob(obj, mesg);
        /* temporary flags that might have been set but which should
           be clear by the time this sanity check is taking place */
        if (obj->in_use || obj->bypass || obj->nomerge
            || (obj->otyp == BOULDER && obj->next_boulder))
            insane_obj_bits(obj, (struct monst *) 0);
    }
}

/* check obj->unpaid and obj->no_charge for shop sanity; caller has
   verified that at least one of them is set */
static void
shop_obj_sanity(struct obj *obj, const char *mesg)
{
    struct obj *otop;
    struct monst *shkp;
    const char *why;
    boolean costly, costlytoo;
    coordxy x = 0, y = 0;

    /* if contained, get top-most container; we needs its location */
    otop = obj;
    while (otop->where == OBJ_CONTAINED)
        otop = otop->ocontainer;
    /* get obj's or its container's location; do not update obj->ox,oy
       or otop->ox,oy because that would cause sanity checking to
       produce side-effects that won't occur when not sanity checking;
       no need for CONTAINED_TOO because we have a top level container */
    (void) get_obj_location(otop, &x, &y, BURIED_TOO);

    /* these will always be needed for the normal case, so don't bother
       waiting until we find an insanity to fetch them */
    shkp = find_objowner(obj, x, y);
    if (shkp && obj->where == OBJ_ONBILL)
        x = shkp->mx, y = shkp->my;
    costly = costly_spot(x, y);
    costlytoo = costly_adjacent(shkp, x, y);

    why = (const char *) 0;
    if (obj->no_charge && obj->unpaid) {
        why = "%s obj both unpaid and no_charge! %s %s: %s";
    } else if (obj->unpaid) {
        /* unpaid is only applicable for directly carried objects, for
           objects inside carried containers, for used up items on the
           billobjs list, and for floor items outside the shop proper
           but within the shop boundary (walls, door, "free spot") and
           for objects moved from such spots into the shop proper by
           repair of shop walls or items buried while on boundary */
        if (otop->where != OBJ_INVENT
            && obj->where != OBJ_ONBILL /* when on bill, obj==otop */
            && ((otop->where != OBJ_FLOOR && obj->where != OBJ_BURIED)
                || !(costly || costlytoo)))
            why = "%s unpaid obj not carried! %s %s: %s";
        else if (!costly && !costlytoo)
            why = "%s unpaid obj not inside tended shop! %s %s: %s";
        else if (!shkp)
            why = "%s unpaid obj inside untended shop! %s %s: %s";
        else if (!onshopbill(obj, shkp, TRUE))
            why = "%s unpaid obj not on shop bill! %s %s: %s";
    } else if (obj->no_charge) {
        /* no_charge is only applicable for floor objects in shops, for
           objects inside floor containers in shops, and for objects buried
           beneath the shop floor or carried by a monster (usually pet) */
        if (otop->where != OBJ_FLOOR
            && otop->where != OBJ_BURIED
            && otop->where != OBJ_MINVENT)
            why = "%s no_charge obj not on floor! %s %s: %s";
        else if (!costly && !costlytoo)
            why = "%s no_charge obj not inside tended shop! %s %s: %s";
        else if (!shkp)
            why = "%s no_charge obj inside untended shop! %s %s: %s";
        else if (onshopbill(obj, shkp, TRUE))
            why = "%s no_charge obj on shop bill! %s %s: %s";
    }
    if (why)
        insane_object(obj, why, mesg,
                      mcarried(otop) ? otop->ocarry : (struct monst *) 0);
    return;
}

/* sanity check for objects carried by all monsters in specified list */
static void
mon_obj_sanity(struct monst *monlist, const char *mesg)
{
    struct monst *mon;
    struct obj *obj, *mwep;

    for (mon = monlist; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        mwep = MON_WEP(mon);
        if (mwep) {
            if (!mcarried(mwep))
                insane_object(mwep, mfmt1, mesg, mon);
            if (mwep->ocarry != mon)
                insane_object(mwep, mfmt2, mesg, mon);
        }
        for (obj = mon->minvent; obj; obj = obj->nobj) {
            if (obj->where != OBJ_MINVENT)
                insane_object(obj, mfmt1, mesg, mon);
            if (obj->ocarry != mon)
                insane_object(obj, mfmt2, mesg, mon);
            if (obj->globby)
                check_glob(obj, mesg);
            check_contained(obj, mesg);
            if (obj->unpaid || obj->no_charge)
                shop_obj_sanity(obj, mesg);
            if (obj->in_use || obj->bypass || obj->nomerge
                || (obj->otyp == BOULDER && obj->next_boulder))
                insane_obj_bits(obj, mon);
        }
    }
}

static void
insane_obj_bits(struct obj *obj, struct monst *mon)
{
    unsigned o_in_use = obj->in_use, o_bypass = obj->bypass,
             /* having obj->nomerge be set might be intentional */
             o_nomerge = (obj->nomerge && !nomerge_exception(obj)),
             /* next_boulder is only for object name formatting when
                pushing boulders and should be reset by next sanity check */
             o_boulder = (obj->otyp == BOULDER && obj->next_boulder);

    if (o_in_use || o_bypass || o_nomerge || o_boulder) {
        char infobuf[QBUFSZ];

        Sprintf(infobuf, "flagged%s%s%s%s",
                o_in_use ? " in_use" : "",
                o_bypass ? " bypass" : "",
                o_nomerge ? " nomerge" : "",
                o_boulder ? " nxtbldr" : "");
        insane_object(obj, ofmt0, infobuf, mon);
    }
}

/* does 'obj' use the 'nomerge' flag persistently? */
static boolean
nomerge_exception(struct obj *obj)
{
    /* special prize objects for achievement tracking are set 'nomerge'
       until they get picked up by the hero */
    if (is_mines_prize(obj) || is_soko_prize(obj))
        return TRUE;

    return FALSE;
}

/* This must stay consistent with the defines in obj.h. */
static const char *const obj_state_names[NOBJ_STATES] = {
    "free", "floor", "contained", "invent",
    "minvent", "migrating", "buried", "onbill",
    "luafree"
};

static const char *
where_name(struct obj *obj)
{
    static char unknown[32]; /* big enough to handle rogue 64-bit int */
    int where;

    if (!obj)
        return "nowhere";
    where = obj->where;
    if (where < 0 || where >= NOBJ_STATES || !obj_state_names[where]) {
        Sprintf(unknown, "unknown[%d]", where);
        return unknown;
    }
    return obj_state_names[where];
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
insane_object(
    struct obj *obj,
    const char *fmt,
    const char *mesg,
    struct monst *mon)
{
    const char *objnm, *monnm;
    char altfmt[BUFSZ];

    objnm = monnm = "null!";
    if (obj) {
        iflags.override_ID++;
        objnm = doname(obj);
        iflags.override_ID--;
    }
    if (mon || (strstri(mesg, "minvent") && !strstri(mesg, "contained"))) {
        Strcat(strcpy(altfmt, fmt), " held by mon %s (%s)");
        if (mon)
            monnm = x_monnam(mon, ARTICLE_A, (char *) 0, EXACT_NAME, TRUE);
        impossible(altfmt, mesg, fmt_ptr((genericptr_t) obj), where_name(obj),
                   objnm, fmt_ptr((genericptr_t) mon), monnm);
    } else {
        impossible(fmt, mesg, fmt_ptr((genericptr_t) obj), where_name(obj),
                   objnm);
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* initialize a dummy obj with just enough info to allow some of the tests in
   obj.h that take an obj pointer to work; used when applying a stethoscope
   toward a mimic mimicking an object */
struct obj *
init_dummyobj(struct obj *obj, short otyp, long oquan)
{
     if (obj) {
         *obj = cg.zeroobj;
         obj->otyp = otyp;
         obj->oclass = objects[otyp].oc_class;
         /* obj->dknown = 0; */
         /* suppress known except for amulets (needed for fakes & real AoY) */
         obj->known = (obj->oclass == AMULET_CLASS)
                       ? obj->known
                         /* default is "on" for types which don't use it */
                         : !objects[otyp].oc_uses_known;
         obj->quan = oquan ? oquan : 1L;
         obj->corpsenm = NON_PM; /* suppress statue and figurine details */
         if (obj->otyp == BOULDER)
             obj->next_boulder = 0; /* overloads corpsenm, avoid NON_PM */
         /* but suppressing fruit details leads to "bad fruit #0" */
         if (obj->otyp == SLIME_MOLD)
             obj->spe = gc.context.current_fruit;
     }
     return obj;
}

/* obj sanity check: check objects inside container */
static void
check_contained(struct obj *container, const char *mesg)
{
    struct obj *obj;
    /* big enough to work with, not too big to blow out stack in recursion */
    char mesgbuf[40], nestedmesg[120];

    if (!Has_contents(container))
        return;
    /* change "invent sanity" to "contained invent sanity"
       but leave "nested contained invent sanity" as is */
    if (!strstri(mesg, "contained"))
        mesg = strcat(strcpy(mesgbuf, "contained "), mesg);

    for (obj = container->cobj; obj; obj = obj->nobj) {
        /* catch direct cycle to avoid unbounded recursion */
        if (obj == container)
            panic("failed sanity check: container holds itself");
        if (obj->where != OBJ_CONTAINED)
            insane_object(obj, "%s obj %s %s: %s", mesg, (struct monst *) 0);
        else if (obj->ocontainer != container)
            impossible("%s obj %s in container %s, not %s", mesg,
                  fmt_ptr((genericptr_t) obj),
                  fmt_ptr((genericptr_t) obj->ocontainer),
                  fmt_ptr((genericptr_t) container));
        if (obj->globby)
            check_glob(obj, mesg);

        if (Has_contents(obj)) {
            /* catch most likely indirect cycle; we won't notice if
               parent is present when something comes before it, or
               notice more deeply embedded cycles (grandparent, &c) */
            if (obj->cobj == container)
                panic("failed sanity check: container holds its parent");
            /* change "contained... sanity" to "nested contained... sanity"
               and "nested contained..." to "nested nested contained..." */
            Strcpy(nestedmesg, "nested ");
            copynchars(eos(nestedmesg), mesg, (int) sizeof nestedmesg
                                              - (int) strlen(nestedmesg) - 1);
            /* recursively check contents */
            check_contained(obj, nestedmesg);
        }
    }
}

/* called when 'obj->globby' is set so we don't recheck it here */
static void
check_glob(struct obj *obj, const char *mesg)
{
#define LOWEST_GLOB GLOB_OF_GRAY_OOZE
#define HIGHEST_GLOB GLOB_OF_BLACK_PUDDING
    if (obj->quan != 1L || obj->owt == 0
        || obj->otyp < LOWEST_GLOB || obj->otyp > HIGHEST_GLOB
#if 0   /*
         * This was relevant before the shrink_glob timer was adopted but
         * now any glob could have a weight that isn't a multiple of 20.
         */
        /* a partially eaten glob could have any non-zero weight but an
           intact one should weigh an exact multiple of base weight (20) */
        || ((obj->owt % objects[obj->otyp].oc_weight) != 0 && !obj->oeaten)
#endif
        ) {
        char mesgbuf[BUFSZ], globbuf[QBUFSZ];

        Sprintf(globbuf, " glob %d,quan=%ld,owt=%u ",
                obj->otyp, obj->quan, obj->owt);
        mesg = strsubst(strcpy(mesgbuf, mesg), " obj ", globbuf);
        insane_object(obj, ofmt0, mesg,
                      (obj->where == OBJ_MINVENT) ? obj->ocarry : 0);
    }
}

/* check an object in hero's or monster's inventory which has worn mask set */
static void
sanity_check_worn(struct obj *obj)
{
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || defined(DEBUG)
    static unsigned long wearbits[] = {
        W_ARM,    W_ARMC,   W_ARMH,    W_ARMS, W_ARMG,  W_ARMF,  W_ARMU,
        W_WEP,    W_QUIVER, W_SWAPWEP, W_AMUL, W_RINGL, W_RINGR, W_TOOL,
        W_SADDLE, W_BALL,   W_CHAIN,   0
        /* [W_ART,W_ARTI are property bits for items which aren't worn] */
    };
    char maskbuf[60];
    const char *what;
    unsigned long owornmask, allmask = 0L;
    boolean embedded = FALSE;
    int i, n = 0;

    /* use owornmask for testing and bit twiddling, but use original
       obj->owornmask for printing */
    owornmask = obj->owornmask;
    /* figure out how many bits are set, and also which are viable */
    for (i = 0; wearbits[i]; ++i) {
        if ((owornmask & wearbits[i]) != 0L)
            ++n;
        allmask |= wearbits[i];
    }
    if (obj == uskin) {
        /* embedded dragon scales have an extra bit set;
           make sure it's set, then suppress it */
        embedded = TRUE;
        if ((owornmask & (W_ARM | I_SPECIAL)) == (W_ARM | I_SPECIAL))
            owornmask &= ~I_SPECIAL;
        else
            n = 0,  owornmask = ~0; /* force insane_object("bogus") below */
    }
    if (n == 2 && carried(obj)
        && obj == uball && (owornmask & W_BALL) != 0L
        && (owornmask & W_WEAPONS) != 0L) {
        /* chained ball can be wielded/alt-wielded/quivered; if so,
          pretend it's not chained in order to check the weapon pointer
          (we've already verified the ball pointer by successfully passing
          the if-condition to get here...) */
        owornmask &= ~W_BALL;
        n = 1;
    }
    if (n > 1) {
        /* multiple bits set */
        Sprintf(maskbuf, "worn mask (multiple) 0x%08lx", obj->owornmask);
        insane_object(obj, ofmt0, maskbuf, (struct monst *) 0);
    }
    if ((owornmask & ~allmask) != 0L
        || (carried(obj) && (owornmask & W_SADDLE) != 0L)) {
        /* non-wearable bit(s) set */
        Sprintf(maskbuf, "worn mask (bogus)) 0x%08lx", obj->owornmask);
        insane_object(obj, ofmt0, maskbuf, (struct monst *) 0);
    }
    if (n == 1 && (carried(obj) || (owornmask & (W_BALL | W_CHAIN)) != 0L)) {
        what = 0;
        /* verify that obj in hero's invent (or ball/chain elsewhere)
           with owornmask of W_foo is the object pointed to by ufoo */
        switch (owornmask) {
        case W_ARM:
            if (obj != (embedded ? uskin : uarm))
                what = embedded ? "skin" : "suit";
            break;
        case W_ARMC:
            if (obj != uarmc)
                what = "cloak";
            break;
        case W_ARMH:
            if (obj != uarmh)
                what = "helm";
            break;
        case W_ARMS:
            if (obj != uarms)
                what = "shield";
            break;
        case W_ARMG:
            if (obj != uarmg)
                what = "gloves";
            break;
        case W_ARMF:
            if (obj != uarmf)
                what = "boots";
            break;
        case W_ARMU:
            if (obj != uarmu)
                what = "shirt";
            break;
        case W_WEP:
            if (obj != uwep)
                what = "primary weapon";
            break;
        case W_QUIVER:
            if (obj != uquiver)
                what = "quiver";
            break;
        case W_SWAPWEP:
            if (obj != uswapwep)
                what = u.twoweap ? "secondary weapon" : "alternate weapon";
            break;
        case W_AMUL:
            if (obj != uamul)
                what = "amulet";
            break;
        case W_RINGL:
            if (obj != uleft)
                what = "left ring";
            break;
        case W_RINGR:
            if (obj != uright)
                what = "right ring";
            break;
        case W_TOOL:
            if (obj != ublindf)
                what = "blindfold";
            break;
        /* case W_SADDLE: */
        case W_BALL:
            if (obj != uball)
                what = "ball";
            break;
        case W_CHAIN:
            if (obj != uchain)
                what = "chain";
            break;
        default:
            break;
        }
        if (what) {
            Sprintf(maskbuf, "worn mask 0x%08lx != %s", obj->owornmask, what);
            insane_object(obj, ofmt0, maskbuf, (struct monst *) 0);
        }
    }
    if (n == 1 && (carried(obj) || (owornmask & (W_BALL | W_CHAIN)) != 0L
                   || mcarried(obj))) {
        /* check for items worn in invalid slots; practically anything can
           be wielded/alt-wielded/quivered, so tests on those are limited */
        what = 0;
        if (owornmask & W_ARMOR) {
            if (obj->oclass != ARMOR_CLASS)
                what = "armor";
            /* 3.6: dragon scale mail reverts to dragon scales when
               becoming embedded in poly'd hero's skin */
            if (embedded && !Is_dragon_scales(obj))
                what = "skin";
        } else if (owornmask & W_WEAPONS) {
            /* monsters don't maintain alternate weapon or quiver */
            if (mcarried(obj) && (owornmask & (W_SWAPWEP | W_QUIVER)) != 0L)
                what = (owornmask & W_SWAPWEP) != 0L ? "monst alt weapon?"
                                                     : "monst quiver?";
            /* hero can quiver gold but not wield it (hence not alt-wield
               it either); also catches monster wielding gold */
            else if (obj->oclass == COIN_CLASS
                     && (owornmask & (W_WEP | W_SWAPWEP)) != 0L)
                what = (owornmask & W_WEP) != 0L ? "weapon" : "alt weapon";
        } else if (owornmask & W_AMUL) {
            if (obj->oclass != AMULET_CLASS)
                what = "amulet";
        } else if (owornmask & W_RING) {
            if (obj->oclass != RING_CLASS && obj->otyp != MEAT_RING)
                what = "ring";
        } else if (owornmask & W_TOOL) {
            if (obj->otyp != BLINDFOLD && obj->otyp != TOWEL
                && obj->otyp != LENSES)
                what = "blindfold";
        } else if (owornmask & W_BALL) {
            if (obj->oclass != BALL_CLASS)
                what = "chained ball";
        } else if (owornmask & W_CHAIN) {
            if (obj->oclass != CHAIN_CLASS)
                what = "chain";
        } else if (owornmask & W_SADDLE) {
            if (obj->otyp != SADDLE)
                what = "saddle";
        }
        if (what) {
            char oclassname[30];
            struct monst *mon = mcarried(obj) ? obj->ocarry : 0;

            /* if we've found a potion worn in the amulet slot,
               this yields "worn (potion amulet)" */
            Strcpy(oclassname, def_oc_syms[(uchar) obj->oclass].name);
            Sprintf(maskbuf, "worn (%s %s)", makesingular(oclassname), what);
            insane_object(obj, ofmt0, maskbuf, mon);
        }
    }
#else /* not (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || DEBUG) */
    /* dummy use of obj to avoid "arg not used" complaint */
    if (!obj)
        insane_object(obj, ofmt0, "<null>", (struct monst *) 0);
#endif
}

/*
 * wrapper to make "near this object" convenient
 */
struct obj *
obj_nexto(struct obj *otmp)
{
    if (!otmp) {
        impossible("obj_nexto: wasn't given an object to check");
        return (struct obj *) 0;
    }
    return obj_nexto_xy(otmp, otmp->ox, otmp->oy, TRUE);
}

/*
 * looks for objects of a particular type next to x, y
 * skips over oid if found (lets us avoid ourselves if
 * we're looking for a second type of an existing object)
 *
 * TODO: return a list of all objects near us so we can more
 * reliably predict which one we want to 'find' first
 */
struct obj *
obj_nexto_xy(struct obj *obj, coordxy x, coordxy y, boolean recurs)
{
    struct obj *otmp;
    int fx, fy, ex, ey, otyp = obj->otyp;
    short dx, dy;

    /* check under our "feet" first */
    otmp = sobj_at(otyp, x, y);
    while (otmp) {
        /* don't be clever and find ourselves */
        if (otmp != obj && mergable(otmp, obj))
            return otmp;
        otmp = nxtobj(otmp, otyp, TRUE);
    }

    if (!recurs)
        return (struct obj *) 0;

    /* search in a random order */
    dx = (rn2(2) ? -1 : 1);
    dy = (rn2(2) ? -1 : 1);
    ex = x - dx;
    ey = y - dy;

    for (fx = ex; abs(fx - ex) < 3; fx += dx) {
        for (fy = ey; abs(fy - ey) < 3; fy += dy) {
            /* 0, 0 was checked above */
            if (isok(fx, fy) && (fx != x || fy != y)) {
                if ((otmp = obj_nexto_xy(obj, fx, fy, FALSE)) != 0)
                    return otmp;
            }
        }
    }
    return (struct obj *) 0;
}

/*
 * Causes one object to absorb another, increasing weight
 * accordingly.  Frees obj2; obj1 remains and is returned.
 */
struct obj *
obj_absorb(struct obj **obj1, struct obj **obj2)
{
    struct obj *otmp1, *otmp2;
    int o1wt, o2wt;
    long agetmp;

    /* don't let people dumb it up */
    if (obj1 && obj2) {
        otmp1 = *obj1;
        otmp2 = *obj2;
        if (otmp1 && otmp2 && otmp1 != otmp2) {
            globby_bill_fixup(otmp1, otmp2);
            if (otmp1->bknown != otmp2->bknown)
                otmp1->bknown = otmp2->bknown = 0;
            if (otmp1->rknown != otmp2->rknown)
                otmp1->rknown = otmp2->rknown = 0;
            if (otmp1->greased != otmp2->greased)
                otmp1->greased = otmp2->greased = 0;
            if (otmp1->orotten || otmp2->orotten)
                otmp1->orotten = otmp2->orotten = 1;
            o1wt = otmp1->oeaten ? otmp1->oeaten : otmp1->owt;
            o2wt = otmp2->oeaten ? otmp2->oeaten : otmp2->owt;
            /* averaging the relative ages is less likely to overflow
               than averaging the absolute ages directly */
            agetmp = (((gm.moves - otmp1->age) * o1wt
                       + (gm.moves - otmp2->age) * o2wt)
                      / (o1wt + o2wt));
            /* convert relative age back to absolute age */
            otmp1->age = gm.moves - agetmp;
            otmp1->owt += o2wt;
            if (otmp1->oeaten || otmp2->oeaten)
                otmp1->oeaten = o1wt + o2wt;
            otmp1->quan = 1L;
            if (otmp1->globby && otmp2->globby) {
                /* average (not weighted, no pun intended) the two globs'
                   shrink timers and use that to give otmp1 a new timer */
                long tm1 = stop_timer(SHRINK_GLOB, obj_to_any(otmp1)),
                     tm2 = stop_timer(SHRINK_GLOB, obj_to_any(otmp2));

                tm1 = ((tm1 ? tm1 : 25L) + (tm2 ? tm2 : 25L) + 1L) / 2L;
                start_glob_timeout(otmp1, tm1);
            }
            /* get rid of second glob, return augmented first one */
            obj_extract_self(otmp2);
            dealloc_obj(otmp2);
            *obj2 = (struct obj *) 0;
            return otmp1;
        }
    }

    impossible("obj_absorb: not called with two actual objects");
    return (struct obj *) 0;
}

/*
 * Causes the heavier object to absorb the lighter object in
 * most cases, but if one object is OBJ_FREE and the other is
 * on the floor, the floor object goes first.
 *
 * wrapper for obj_absorb so that floor_effects works more
 * cleanly (since we don't know which we want to stay around)
 */
struct obj *
obj_meld(struct obj** obj1, struct obj** obj2)
{
    struct obj *otmp1, *otmp2, *result = 0;
    int ox, oy;

    if (obj1 && obj2) {
        otmp1 = *obj1;
        otmp2 = *obj2;
        if (otmp1 && otmp2 && otmp1 != otmp2) {
            ox = oy = 0;
            /*
             * FIXME?
             *  If one of the objects is free because it's being dropped,
             *  we should really finish a full drop and then absorb/meld
             *  if it survives the flooreffects().  Then lighter-melds-into-
             *  heavier will be true even when heavier is the one dropped.
             *
             *  [Also, what about when one of the globs is on the shore
             *  and we drop the other into adjacent pool or vice versa?]
             */
            if (!(otmp2->where == OBJ_FLOOR && otmp1->where == OBJ_FREE)
                && (otmp1->owt > otmp2->owt
                    || (otmp1->owt == otmp2->owt && rn2(2)))) {
                if (otmp2->where == OBJ_FLOOR)
                    ox = otmp2->ox, oy = otmp2->oy;
                result = obj_absorb(obj1, obj2);
            } else {
                if (otmp1->where == OBJ_FLOOR)
                    ox = otmp1->ox, oy = otmp1->oy;
                result = obj_absorb(obj2, obj1);
            }
            /* callers really ought to take care of this; glob melding is
               a bookkeeping issue rather than a display one */
            if (ox && cansee(ox, oy))
                newsym(ox, oy);
        }
    } else {
        impossible("obj_meld: not called with two actual objects");
    }
    return result;
}

/* give a message if hero notices two globs merging [used to be in pline.c] */
void
pudding_merge_message(struct obj *otmp, struct obj *otmp2)
{
    boolean visible = (cansee(otmp->ox, otmp->oy)
                       || cansee(otmp2->ox, otmp2->oy)),
            onfloor = (otmp->where == OBJ_FLOOR || otmp2->where == OBJ_FLOOR),
            inpack = (carried(otmp) || carried(otmp2));

    /* the player will know something happened inside his own inventory */
    if ((!Blind && visible) || inpack) {
        if (Hallucination) {
            if (onfloor) {
                You_see("parts of the floor melting!");
            } else if (inpack) {
                Your("pack reaches out and grabs something!");
            }
            /* even though we can see where they should be,
             * they'll be out of our view (minvent or container)
             * so don't actually show anything */
        } else if (onfloor || inpack) {
            boolean adj = ((otmp->ox != u.ux || otmp->oy != u.uy)
                           && (otmp2->ox != u.ux || otmp2->oy != u.uy));

            pline("The %s%s coalesce%s.",
                  (onfloor && adj) ? "adjacent " : "",
                  makeplural(obj_typename(otmp->otyp)),
                  inpack ? " inside your pack" : "");
        }
    } else {
        You_hear("a faint sloshing sound.");
    }
}

/*mkobj.c*/

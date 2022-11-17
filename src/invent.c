/* NetHack 3.7	invent.c	$NHDT-Date: 1661240719 2022/08/23 07:45:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.424 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define NOINVSYM '#'
#define CONTAINED_SYM '>' /* designator for inside a container */
#define HANDS_SYM '-'

static char *loot_xname(struct obj *);
static int invletter_value(char);
static int QSORTCALLBACK sortloot_cmp(const genericptr, const genericptr);
static void reorder_invent(void);
static struct obj *addinv_core0(struct obj *, struct obj *, boolean);
static void noarmor(boolean);
static void invdisp_nothing(const char *, const char *);
static boolean worn_wield_only(struct obj *);
static char *cinv_doname(struct obj *);
static char *cinv_ansimpleoname(struct obj *);
static boolean only_here(struct obj *);
static void compactify(char *);
static boolean taking_off(const char *);
static int ckvalidcat(struct obj *);
static int ckunpaid(struct obj *);
static char *safeq_xprname(struct obj *);
static char *safeq_shortxprname(struct obj *);
static char display_pickinv(const char *, const char *, const char *,
                            boolean, long *);
static char display_used_invlets(char);
static boolean this_type_only(struct obj *);
static void dounpaid(void);
static struct obj *find_unpaid(struct obj *, struct obj **);
static void menu_identify(int);
static boolean tool_being_used(struct obj *);
static int adjust_ok(struct obj *);
static int adjust_gold_ok(struct obj *);
static int doorganize_core(struct obj *);
static char obj_to_let(struct obj *);
static boolean item_naming_classification(struct obj *, char *, char *);
static int item_reading_classification(struct obj *, char *);
static void mime_action(const char *);

/* enum and structs are defined in wintype.h */
static win_request_info zerowri = { { 0L, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0 } };
static win_request_info wri_info;
static int done_setting_perminv_flags = 0;
static boolean in_perm_invent_toggled;

/* wizards can wish for venom, which will become an invisible inventory
 * item without this.  putting it in inv_order would mean venom would
 * suddenly become a choice for all the inventory-class commands, which
 * would probably cause mass confusion.  the test for inventory venom
 * is only WIZARD and not wizard because the wizard can leave venom lying
 * around on a bones level for normal players to find.  [Note to the
 * confused:  'WIZARD' used to be a compile-time conditional so this was
 * guarded by #ifdef WIZARD/.../#endif.]
 */
static const char venom_inv[] = { VENOM_CLASS, 0 }; /* (constant) */

/* sortloot() classification; called at most once [per sort] for each object */
void
loot_classify(Loot *sort_item, struct obj *obj)
{
    /* we may eventually make this a settable option to always use
       with sortloot instead of only when the 'sortpack' option isn't
       set; it is similar to sortpack's inv_order but items most
       likely to be picked up are moved to the front */
    static char def_srt_order[MAXOCLASSES] = {
        COIN_CLASS, AMULET_CLASS, RING_CLASS, WAND_CLASS, POTION_CLASS,
        SCROLL_CLASS, SPBOOK_CLASS, GEM_CLASS, FOOD_CLASS, TOOL_CLASS,
        WEAPON_CLASS, ARMOR_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, 0,
    };
    static char armcat[8];
    const char *classorder;
    char *p;
    int k, otyp = obj->otyp, oclass = obj->oclass;
    boolean seen, discovered = objects[otyp].oc_name_known ? TRUE : FALSE;

    /*
     * For the value types assigned by this classification, sortloot()
     * will put lower valued ones before higher valued ones.
     */
    if (!Blind)
        obj->dknown = 1; /* xname(obj) does this; we want it sooner */
    seen = obj->dknown ? TRUE : FALSE,
    /* class order */
    classorder = flags.sortpack ? flags.inv_order : def_srt_order;
    p = strchr(classorder, oclass);
    if (p)
        k = 1 + (int) (p - classorder);
    else
        k = 1 + (int) strlen(classorder) + (oclass != VENOM_CLASS);
    sort_item->orderclass = (coordxy) k;
    /* subclass designation; only a few classes have subclasses
       and the non-armor ones we use are fairly arbitrary */
    switch (oclass) {
    case ARMOR_CLASS:
        if (!armcat[7]) {
            /* one-time init; we use a different order than the subclass
               values defined by objclass.h */
            armcat[ARM_HELM]   = 1; /* [2] */
            armcat[ARM_GLOVES] = 2; /* [3] */
            armcat[ARM_BOOTS]  = 3; /* [4] */
            armcat[ARM_SHIELD] = 4; /* [1] */
            armcat[ARM_CLOAK]  = 5; /* [5] */
            armcat[ARM_SHIRT]  = 6; /* [6] */
            armcat[ARM_SUIT]   = 7; /* [0] */
            armcat[7]          = 8; /* sanity protection */
        }
        k = objects[otyp].oc_armcat;
        /* oc_armcat overloads oc_subtyp which is an 'schar' so guard
           against somebody assigning something unexpected to it */
        if (k < 0 || k >= 7)
            k = 7;
        k = armcat[k];
        break;
    case WEAPON_CLASS:
        /* for weapons, group by ammo (arrows, bolts), launcher (bows),
           missile (darts, boomerangs), stackable (daggers, knives, spears),
           'other' (swords, axes, &c), polearms */
        k = objects[otyp].oc_skill;
        k = (k < 0) ? ((k >= -P_CROSSBOW && k <= -P_BOW) ? 1 : 3)
                    : ((k >= P_BOW && k <= P_CROSSBOW) ? 2
                       : (k == P_SPEAR || k == P_DAGGER || k == P_KNIFE) ? 4
                          : !is_pole(obj) ? 5 : 6);
        break;
    case TOOL_CLASS:
        if (seen && discovered
            && (otyp == BAG_OF_TRICKS || otyp == HORN_OF_PLENTY))
            k = 2; /* known pseudo-container */
        else if (Is_container(obj))
            k = 1; /* regular container or unknown bag of tricks */
        else
            switch (otyp) {
            case WOODEN_FLUTE:
            case MAGIC_FLUTE:
            case TOOLED_HORN:
            case FROST_HORN:
            case FIRE_HORN:
            case WOODEN_HARP:
            case MAGIC_HARP:
            case BUGLE:
            case LEATHER_DRUM:
            case DRUM_OF_EARTHQUAKE:
            case HORN_OF_PLENTY: /* not a musical instrument */
                k = 3; /* instrument or unknown horn of plenty */
                break;
            default:
                k = 4; /* 'other' tool */
                break;
            }
        break;
    case FOOD_CLASS:
        /* [what about separating "partly eaten" within each group?] */
        switch (otyp) {
        case SLIME_MOLD:
            k = 1;
            break;
        default:
            /* [maybe separate one-bite foods from rations and such?] */
            k = obj->globby ? 6 : 2;
            break;
        case TIN:
            k = 3;
            break;
        case EGG:
            k = 4;
            break;
        case CORPSE:
            k = 5;
            break;
        }
        break;
    case GEM_CLASS:
        /*
         * Normally subclass takes priority over discovery status, but
         * that would give away information for gems (assuming we'll
         * group them as valuable gems, next glass, then gray stones,
         * and finally rocks once they're all fully identified).
         *
         * Order:
         *  1) unseen gems and glass ("gem")
         *  2) seen but undiscovered gems and glass ("blue gem"),
         *  3) discovered gems ("sapphire"),
         *  4) discovered glass ("worthless pieced of blue glass"),
         *  5) unseen gray stones and rocks ("stone"),
         *  6) seen but undiscovered gray stones ("gray stone"),
         *  7) discovered gray stones ("touchstone"),
         *  8) seen rocks ("rock").
         */
        switch (objects[obj->otyp].oc_material) {
        case GEMSTONE:
            k = !seen ? 1 : !discovered ? 2 : 3;
            break;
        case GLASS:
            k = !seen ? 1 : !discovered ? 2 : 4;
            break;
        default: /* MINERAL */
            k = !seen ? 5 : (obj->otyp != ROCK) ? (!discovered ? 6 : 7) : 8;
            break;
        }
        break;
    default:
        /* other classes don't have subclasses; we assign a nonzero
           value because sortloot() uses 0 to mean 'not yet classified' */
        k = 1; /* any non-zero would do */
        break;
    }
    sort_item->subclass = (coordxy) k;
    /* discovery status */
    k = !seen ? 1 /* unseen */
        : (discovered || !OBJ_DESCR(objects[otyp])) ? 4
          : (objects[otyp].oc_uname) ? 3 /* named (partially discovered) */
            : 2; /* undiscovered */
    sort_item->disco = (coordxy) k;
}

/* sortloot() formatting routine; for alphabetizing, not shown to user */
static char *
loot_xname(struct obj *obj)
{
    struct obj saveo;
    boolean save_debug;
    char *res, *save_oname;

    /*
     * Deal with things that xname() includes as a prefix.  We don't
     * want such because they change alphabetical ordering.  First,
     * remember 'obj's current settings.
     */
    saveo.odiluted = obj->odiluted;
    saveo.blessed = obj->blessed, saveo.cursed = obj->cursed;
    saveo.spe = obj->spe;
    saveo.owt = obj->owt;
    save_oname = has_oname(obj) ? ONAME(obj) : 0;
    save_debug = flags.debug;
    /* suppress "diluted" for potions and "holy/unholy" for water;
       sortloot() will deal with them using other criteria than name */
    if (obj->oclass == POTION_CLASS) {
        obj->odiluted = 0;
        if (obj->otyp == POT_WATER)
            obj->blessed = 0, obj->cursed = 0;
    }
    /* make "wet towel" and "moist towel" format as "towel" so that all
       three group together */
    if (obj->otyp == TOWEL)
        obj->spe = 0;
    /* group globs by monster type rather than by size:  force all to
       have the same size adjective hence same "small glob of " prefix */
    if (obj->globby)
        obj->owt = 20; /* weight of a fresh glob (one pudding's worth) */
    /* suppress user-assigned name */
    if (save_oname && !obj->oartifact)
        ONAME(obj) = 0;
    /* avoid wizard mode formatting variations */
    if (wizard) { /* flags.debug */
        /* paranoia:  before toggling off wizard mode, guard against a
           panic in xname() producing a normal mode panic save file */
        g.program_state.something_worth_saving = 0;
        flags.debug = FALSE;
    }

    res = cxname_singular(obj);

    if (save_debug) {
        flags.debug = TRUE;
        g.program_state.something_worth_saving = 1;
    }
    /* restore the object */
    if (obj->oclass == POTION_CLASS) {
        obj->odiluted = saveo.odiluted;
        if (obj->otyp == POT_WATER)
            obj->blessed = saveo.blessed, obj->cursed = saveo.cursed;
    }
    if (obj->otyp == TOWEL) {
        obj->spe = saveo.spe;
        /* give "towel" a suffix that will force wet ones to come first,
           moist ones next, and dry ones last regardless of whether
           they've been flagged as having spe known */
        Strcat(res, is_wet_towel(obj) ? ((obj->spe >= 3) ? "x" : "y") : "z");
    }
    if (obj->globby) {
        obj->owt = saveo.owt;
        /* we've suppressed the size prefix (above); there normally won't
           be more than one of a given creature type because they coalesce,
           but globs with different bless/curse state won't merge so it is
           feasible to have multiple at the same location; add a suffix to
           get such sorted by size (small first) */
        Strcat(res, (obj->owt <= 100) ? "a"
                    : (obj->owt <= 300) ? "b"
                      : (obj->owt <= 500) ? "c"
                        : "d");
    }
    if (save_oname && !obj->oartifact)
        ONAME(obj) = save_oname;

    return res;
}

/* '$'==1, 'a'-'z'==2..27, 'A'-'Z'==28..53, '#'==54, catchall 55 */
static int
invletter_value(char c)
{
    return ('a' <= c && c <= 'z') ? (c - 'a' + 2)
           : ('A' <= c && c <= 'Z') ? (c - 'A' + 2 + 26)
             : (c == '$') ? 1
               : (c == '#') ? 1 + 52 + 1
                 : 1 + 52 + 1 + 1; /* none of the above (shouldn't happen) */
}

/* qsort comparison routine for sortloot() */
static int QSORTCALLBACK
sortloot_cmp(const genericptr vptr1, const genericptr vptr2)
{
    struct sortloot_item *sli1 = (struct sortloot_item *) vptr1,
                         *sli2 = (struct sortloot_item *) vptr2;
    struct obj *obj1 = sli1->obj,
               *obj2 = sli2->obj;
    char *nam1, *nam2;
    int val1, val2, namcmp;

    /* order by object class unless we're doing by-invlet without sortpack */
    if ((g.sortlootmode & (SORTLOOT_PACK | SORTLOOT_INVLET))
        != SORTLOOT_INVLET) {
        /* Classify each object at most once no matter how many
           comparisons it is involved in. */
        if (!sli1->orderclass)
            loot_classify(sli1, obj1);
        if (!sli2->orderclass)
            loot_classify(sli2, obj2);

        /* Sort by class. */
        val1 = sli1->orderclass;
        val2 = sli2->orderclass;
        if (val1 != val2)
            return (int) (val1 - val2);

        /* skip sub-classes when ordering by sortpack+invlet */
        if ((g.sortlootmode & SORTLOOT_INVLET) == 0) {
            /* Class matches; sort by subclass. */
            val1 = sli1->subclass;
            val2 = sli2->subclass;
            if (val1 != val2)
                return val1 - val2;

            /* Class and subclass match; sort by discovery status:
             * first unseen, then seen but not named or discovered,
             * then named, lastly discovered.
             * 1) potion
             * 2) pink potion
             * 3) dark green potion called confusion
             * 4) potion of healing
             * Multiple entries within each group will be put into
             * alphabetical order below.
             */
            val1 = sli1->disco;
            val2 = sli2->disco;
            if (val1 != val2)
                return val1 - val2;
        }
    }

    /* order by assigned inventory letter */
    if ((g.sortlootmode & SORTLOOT_INVLET) != 0) {
        val1 = invletter_value(obj1->invlet);
        val2 = invletter_value(obj2->invlet);
        if (val1 != val2)
            return val1 - val2;
    }

    if ((g.sortlootmode & SORTLOOT_LOOT) == 0)
        goto tiebreak;

    /*
     * Sort object names in lexicographical order, ignoring quantity.
     *
     * Each obj gets formatted at most once (per sort) no matter how many
     * comparisons it gets subjected to.
     */
    nam1 = sli1->str;
    if (!nam1)
        nam1 = sli1->str = dupstr(loot_xname(obj1));
    nam2 = sli2->str;
    if (!nam2)
        nam2 = sli2->str = dupstr(loot_xname(obj2));
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

/*
 * sortloot() - the story so far...
 *
 *      The original implementation constructed and returned an array
 *      of pointers to objects in the requested order.  Callers had to
 *      count the number of objects, allocate the array, pass one
 *      object at a time to the routine which populates it, traverse
 *      the objects via stepping through the array, then free the
 *      array.  The ordering process used a basic insertion sort which
 *      is fine for short lists but inefficient for long ones.
 *
 *      3.6.0 (and continuing with 3.6.1) changed all that so that
 *      sortloot was self-contained as far as callers were concerned.
 *      It reordered the linked list into the requested order and then
 *      normal list traversal was used to process it.  It also switched
 *      to qsort() on the assumption that the C library implementation
 *      put some effort into sorting efficiently.  It also checked
 *      whether the list was already sorted as it got ready to do the
 *      sorting, so re-examining inventory or a pile of objects without
 *      having changed anything would gobble up less CPU than a full
 *      sort.  But it had at least two problems (aside from the ordinary
 *      complement of bugs):
 *      1) some players wanted to get the original order back when they
 *      changed the 'sortloot' option back to 'none', but the list
 *      reordering made that infeasible;
 *      2) object identification giving the 'ID whole pack' result
 *      would call makeknown() on each newly ID'd object, that would
 *      call update_inventory() to update the persistent inventory
 *      window if one existed, the interface would call the inventory
 *      display routine which would call sortloot() which might change
 *      the order of the list being traversed by the identify code,
 *      possibly skipping the ID of some objects.  That could have been
 *      avoided by suppressing 'perm_invent' during identification
 *      (fragile) or by avoiding sortloot() during inventory display
 *      (more robust).
 *
 *      As of 3.6.2: revert to the temporary array of ordered obj pointers
 *      but have sortloot() do the counting and allocation.  Callers
 *      need to use array traversal instead of linked list traversal
 *      and need to free the temporary array when done.  And the
 *      array contains 'struct sortloot_item' (aka 'Loot') entries
 *      instead of simple 'struct obj *' entries.
 */
Loot *
sortloot(
    struct obj **olist,  /* old version might have changed *olist, we don't */
    unsigned mode,       /* flags for sortloot_cmp() */
    boolean by_nexthere, /* T: traverse via obj->nexthere, F: via obj->nobj */
    boolean (*filterfunc)(struct obj *)) /* optional filter */
{
    Loot *sliarray;
    struct obj *o;
    unsigned n, i;
    boolean augment_filter;

    for (n = 0, o = *olist; o; o = by_nexthere ? o->nexthere : o->nobj)
        ++n;
    /* note: if there is a filter function, this might overallocate */
    sliarray = (Loot *) alloc((n + 1) * sizeof *sliarray);

    /* the 'keep cockatrice corpses' flag is overloaded with sort mode */
    augment_filter = (mode & SORTLOOT_PETRIFY) ? TRUE : FALSE;
    mode &= ~SORTLOOT_PETRIFY; /* remove flag, leaving mode */
    /* populate aliarray[0..n-1] */
    for (i = 0, o = *olist; o; o = by_nexthere ? o->nexthere : o->nobj) {
        if (filterfunc && !(*filterfunc)(o)
            /* caller may be asking us to override filterfunc (in order
               to do a cockatrice corpse touch check during pickup even
               if/when the filter rejects food class) */
            && (!augment_filter || o->otyp != CORPSE
                || !touch_petrifies(&mons[o->corpsenm])))
            continue;
        sliarray[i].obj = o, sliarray[i].indx = (int) i;
        sliarray[i].str = (char *) 0;
        sliarray[i].orderclass = sliarray[i].subclass = sliarray[i].disco = 0;
        ++i;
    }
    n = i;
    /* add a terminator so that we don't have to pass 'n' back to caller */
    sliarray[n].obj = (struct obj *) 0, sliarray[n].indx = -1;
    sliarray[n].str = (char *) 0;
    sliarray[n].orderclass = sliarray[n].subclass = sliarray[n].disco = 0;

    /* do the sort; if no sorting is requested, we'll just return
       a sortloot_item array reflecting the current ordering */
    if (mode && n > 1) {
        g.sortlootmode = mode; /* extra input for sortloot_cmp() */
        qsort((genericptr_t) sliarray, n, sizeof *sliarray, sortloot_cmp);
        g.sortlootmode = 0; /* reset static mode flags */
        /* if sortloot_cmp formatted any objects, discard their strings now */
        for (i = 0; i < n; ++i)
            if (sliarray[i].str)
                free((genericptr_t) sliarray[i].str), sliarray[i].str = 0;
    }
    return sliarray;
}

/* sortloot() callers should use this to free up memory it allocates */
void
unsortloot(Loot **loot_array_p)
{
    if (*loot_array_p)
        free((genericptr_t) *loot_array_p), *loot_array_p = (Loot *) 0;
}

#if 0 /* 3.6.0 'revamp' -- simpler than current, but ultimately too simple */
void
sortloot(
    struct obj **olist,
    unsigned mode,       /* flags for sortloot_cmp() */
    boolean by_nexthere) /* T: traverse via obj->nexthere, F: via obj->nobj */
{
    struct sortloot_item *sliarray, osli, nsli;
    struct obj *o, **nxt_p;
    unsigned n, i;
    boolean already_sorted = TRUE;

    g.sortlootmode = mode; /* extra input for sortloot_cmp() */
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
    g.sortlootmode = 0;
}
#endif /*0*/

void
assigninvlet(struct obj *otmp)
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
    for (obj = g.invent; obj; obj = obj->nobj)
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
    for (i = g.lastinvnr + 1; i != g.lastinvnr; i++) {
        if (i == 52) {
            i = -1;
            continue;
        }
        if (!inuse[i])
            break;
    }
    otmp->invlet =
        (inuse[i] ? NOINVSYM : (i < 26) ? ('a' + i) : ('A' + i - 26));
    g.lastinvnr = i;
}

/* note: assumes ASCII; toggling a bit puts lowercase in front of uppercase */
#define inv_rank(o) ((o)->invlet ^ 040)

/* sort the inventory; used by addinv() and doorganize() */
static void
reorder_invent(void)
{
    struct obj *otmp, *prev, *next;
    boolean need_more_sorting;

    do {
        /*
         * We expect at most one item to be out of order, so this
         * isn't nearly as inefficient as it may first appear.
         */
        need_more_sorting = FALSE;
        for (otmp = g.invent, prev = 0; otmp; ) {
            next = otmp->nobj;
            if (next && inv_rank(next) < inv_rank(otmp)) {
                need_more_sorting = TRUE;
                if (prev)
                    prev->nobj = next;
                else
                    g.invent = next;
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
merge_choice(struct obj *objlist, struct obj *obj)
{
    struct monst *shkp;
    int save_nocharge;

    if (obj->otyp == SCR_SCARE_MONSTER) /* punt on these */
        return (struct obj *) 0;
    /* if this is an item on the shop floor, the attributes it will
       have when carried are different from what they are now; prevent
       that from eliciting an incorrect result from mergable() */
    save_nocharge = obj->no_charge;
    if (objlist == g.invent && obj->where == OBJ_FLOOR
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
merged(struct obj **potmp, struct obj **pobj)
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

        if (!otmp->globby)
            otmp->quan += obj->quan;
        /* temporary special case for gold objects!!!! */
        if (otmp->oclass == COIN_CLASS)
            otmp->owt = weight(otmp), otmp->bknown = 0;
        /* and puddings!!!1!!one! */
        else if (!Is_pudding(otmp))
            otmp->owt += obj->owt;
        if (!has_oname(otmp) && has_oname(obj))
            otmp = *potmp = oname(otmp, ONAME(obj), ONAME_SKIP_INVUPD);
        obj_extract_self(obj);

        if (obj->pickup_prev && otmp->where == OBJ_INVENT)
            otmp->pickup_prev = 1;

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

        /* mergable() no longer requires 'bypass' to match; if 'obj' has
           the bypass bit set, force the combined stack to have that too;
           primarily in case this merge is occurring because stackobj()
           is operating on an object just dropped by a monster that was
           zapped with polymorph, we want bypass set in order to inhibit
           the same zap from affecting the new combined stack when it hits
           objects at the monster's spot (but also in case we're called by
           code that's using obj->bypass to track 'already processed') */
        if (obj->bypass)
            otmp->bypass = 1;

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
addinv_core1(struct obj *obj)
{
    if (obj->oclass == COIN_CLASS) {
        g.context.botl = 1;
    } else if (obj->otyp == AMULET_OF_YENDOR) {
        if (u.uhave.amulet)
            impossible("already have amulet?");
        u.uhave.amulet = 1;
        record_achievement(ACH_AMUL);
    } else if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
        if (u.uhave.menorah)
            impossible("already have candelabrum?");
        u.uhave.menorah = 1;
        record_achievement(ACH_CNDL);
    } else if (obj->otyp == BELL_OF_OPENING) {
        if (u.uhave.bell)
            impossible("already have silver bell?");
        u.uhave.bell = 1;
        record_achievement(ACH_BELL);
    } else if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        if (u.uhave.book)
            impossible("already have the book?");
        u.uhave.book = 1;
        record_achievement(ACH_BOOK);
    } else if (obj->oartifact) {
        if (is_quest_artifact(obj)) {
            if (u.uhave.questart)
                impossible("already have quest artifact?");
            u.uhave.questart = 1;
            artitouch(obj);
        }
        set_artifact_intrinsic(obj, 1, W_ART);
    }

    /* "special achievements"; revealed in end of game disclosure and
       dumplog, originally just recorded in XLOGFILE */
    if (is_mines_prize(obj)) {
        record_achievement(ACH_MINE_PRIZE);
        g.context.achieveo.mines_prize_oid = 0; /* done with luckstone o_id */
        obj->nomerge = 0;
    } else if (is_soko_prize(obj)) {
        record_achievement(ACH_SOKO_PRIZE);
        g.context.achieveo.soko_prize_oid = 0; /* done with bag/amulet o_id */
        obj->nomerge = 0;
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
addinv_core2(struct obj *obj)
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
static struct obj *
addinv_core0(struct obj *obj, struct obj *other_obj,
             boolean update_perm_invent)
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

    if (g.loot_reset_justpicked) {
        g.loot_reset_justpicked = FALSE;
        reset_justpicked(g.invent);
    }

    addinv_core1(obj);

    /* for addinv_before(); if something has been removed and is now being
       reinserted, try to put it in the same place instead of merging or
       placing at end; for thrown-and-return weapon with !fixinv setting */
    if (other_obj) {
        for (otmp = g.invent; otmp; otmp = otmp->nobj) {
            if (otmp->nobj == other_obj) {
                obj->nobj = other_obj;
                otmp->nobj = obj;
                obj->where = OBJ_INVENT;
                goto added;
            }
        }
    }

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
    for (prev = 0, otmp = g.invent; otmp; prev = otmp, otmp = otmp->nobj)
        if (merged(&otmp, &obj)) {
            obj = otmp;
            if (!obj)
                panic("addinv: null obj after merge otyp=%d", saved_otyp);
            goto added;
        }
    /* didn't merge, so insert into chain */
    assigninvlet(obj);
    if (flags.invlet_constant || !prev) {
        obj->nobj = g.invent; /* insert at beginning */
        g.invent = obj;
        if (flags.invlet_constant)
            reorder_invent();
    } else {
        prev->nobj = obj; /* insert at end */
        obj->nobj = 0;
    }
    obj->where = OBJ_INVENT;

    /* fill empty quiver if obj was thrown */
    if (obj_was_thrown && flags.pickup_thrown && !uquiver
        /* if Mjollnir is thrown and fails to return, we want to
           auto-pick it when we move to its spot, but not into quiver
           because it needs to be wielded to be re-thrown;
           aklys likewise because player using 'f' to throw it might
           not notice that it isn't wielded until it fails to return
           several times; we never auto-wield, just omit from quiver
           so that player will be prompted for what to throw and
           possibly realize that re-wielding is necessary */
        && obj->oartifact != ART_MJOLLNIR && obj->otyp != AKLYS
        && (throwing_weapon(obj) || is_ammo(obj)))
        setuqwep(obj);
 added:
    obj->pickup_prev = 1;
    addinv_core2(obj);
    carry_obj_effects(obj); /* carrying affects the obj */
    if (update_perm_invent)
        update_inventory();
    return obj;
}

/* add obj to the hero's inventory in the default fashion */
struct obj *
addinv(struct obj *obj)
{
    return addinv_core0(obj, (struct obj *) 0, TRUE);
}

/* add obj to the hero's inventory by inserting in front of a specific item */
struct obj *
addinv_before(struct obj *obj, struct obj *other_obj)
{
    return addinv_core0(obj, other_obj, TRUE);
}

/*
 * Some objects are affected by being carried.
 * Make those adjustments here. Called _after_ the object
 * has been added to the hero's or monster's inventory,
 * and after hero's intrinsics have been updated.
 */
void
carry_obj_effects(struct obj *obj)
{
    /* Cursed figurines can spontaneously transform when carried. */
    if (obj->otyp == FIGURINE) {
        if (obj->cursed && obj->corpsenm != NON_PM
            && !dead_species(obj->corpsenm, TRUE)) {
            attach_fig_transform_timeout(obj);
        }
    }
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* Add an item to the inventory unless we're fumbling or it refuses to be
 * held (via touch_artifact), and give a message.
 * If there aren't any free inventory slots, we'll drop it instead.
 * If both success and failure messages are NULL, then we're just doing the
 * fumbling/slot-limit checking for a silent grab.  In any case,
 * touch_artifact will print its own messages if they are warranted.
 */
struct obj *
hold_another_object(struct obj *obj, const char *drop_fmt,
                    const char *drop_arg, const char *hold_msg)
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

        if (!touch_artifact(obj, &g.youmonst)) {
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
        obj->nomerge = 1;
        /* dropping expects obj to be in invent */
        obj = addinv_core0(obj, (struct obj *) 0, FALSE);
        goto drop_it;
    } else {
        long oquan = obj->quan;
        int prev_encumbr = near_capacity(); /* before addinv() */

        /* encumbrance limit is max( current_state, pickup_burden );
           this used to use hardcoded MOD_ENCUMBER (stressed) instead
           of the 'pickup_burden' option (which defaults to stressed) */
        if (prev_encumbr < flags.pickup_burden)
            prev_encumbr = flags.pickup_burden;
        /* addinv() may redraw the entire inventory, overwriting
           drop_arg when it is kept in an 'obuf' from doname();
           [should no longer be necessary now that perm_invent update is
           suppressed, but it's cheap to keep as a paranoid precaution] */
        if (drop_arg)
            drop_arg = strcpy(buf, drop_arg);

        obj = addinv_core0(obj, (struct obj *) 0, FALSE);
        if (inv_cnt(FALSE) > 52 || ((obj->otyp != LOADSTONE || !obj->cursed)
                                    && near_capacity() > prev_encumbr)) {
            /* undo any merge which took place */
            if (obj->quan > oquan)
                obj = splitobj(obj, oquan);
            goto drop_it;
        } else {
            if (flags.autoquiver && !uquiver && !obj->owornmask
                && (is_missile(obj) || ammo_and_launcher(obj, uwep)
                    || ammo_and_launcher(obj, uswapwep)))
                setuqwep(obj);
            if (hold_msg || drop_fmt)
                prinv(hold_msg, obj, oquan);
            /* obj made it into inventory and is staying there */
            update_inventory();
            (void) encumber_msg();
        }
    }
    return obj;

 drop_it:
    if (drop_fmt)
        pline(drop_fmt, drop_arg);
    obj->nomerge = 0;
    if (can_reach_floor(TRUE) || u.uswallow) {
        dropx(obj);
    } else {
        freeinv(obj);
        hitfloor(obj, FALSE);
    }
    return (struct obj *) 0; /* might be gone */
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* useup() all of an item regardless of its quantity */
void
useupall(struct obj *obj)
{
    setnotworn(obj);
    freeinv(obj);
    obfree(obj, (struct obj *) 0); /* deletes contents also */
}

/* an item in inventory is going away after being used */
void
useup(struct obj *obj)
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
consume_obj_charge(
    struct obj *obj,
    boolean maybe_unpaid) /* false if caller handles shop billing */
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
freeinv_core(struct obj *obj)
{
    if (obj->oclass == COIN_CLASS) {
        g.context.botl = 1;
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
        g.context.botl = 1;
    } else if (obj->otyp == FIGURINE && obj->timed) {
        (void) stop_timer(FIG_TRANSFORM, obj_to_any(obj));
    }
}

/* remove an object from the hero's inventory */
void
freeinv(struct obj *obj)
{
    extract_nobj(obj, &g.invent);
    obj->pickup_prev = 0;
    freeinv_core(obj);
    update_inventory();
}

/* drawbridge is destroying all objects at <x,y> */
void
delallobj(coordxy x, coordxy y)
{
    struct obj *otmp, *otmp2;

    for (otmp = g.level.objects[x][y]; otmp; otmp = otmp2) {
        if (otmp == uball)
            unpunish();
        /* after unpunish(), or might get deallocated chain */
        otmp2 = otmp->nexthere;
        if (otmp == uchain)
            continue;
        delobj(otmp);
    }
}

/* normal object deletion (if unpaid, it remains on the bill) */
void
delobj(struct obj *obj)
{
    delobj_core(obj, FALSE);
}

/* destroy object; caller has control over whether to destroy something
   that ordinarily shouldn't be destroyed */
void
delobj_core(
    struct obj *obj,
    boolean force) /* 'force==TRUE' used when reviving Rider corpses */
{
    boolean update_map;

    /* obj_resists(obj,0,0) protects the Amulet, the invocation tools,
       and Rider corspes */
    if (!force && obj_resists(obj, 0, 0)) {
        /* player might be doing something stupid, but we
         * can't guarantee that.  assume special artifacts
         * are indestructible via drawbridges, and exploding
         * chests, and golem creation, and ...
         */
        obj->in_use = 0; /* in case caller has set this to 1 */
        return;
    }
    update_map = (obj->where == OBJ_FLOOR);
    obj_extract_self(obj);
    if (update_map) { /* floor object's coordinates are always up to date */
        maybe_unhide_at(obj->ox, obj->oy);
        newsym(obj->ox, obj->oy);
    }
    obfree(obj, (struct obj *) 0); /* frees contents also */
}

/* try to find a particular type of object at designated map location */
struct obj *
sobj_at(int otyp, coordxy x, coordxy y)
{
    register struct obj *otmp;

    for (otmp = g.level.objects[x][y]; otmp; otmp = otmp->nexthere)
        if (otmp->otyp == otyp)
            break;

    return otmp;
}

/* sobj_at(&c) traversal -- find next object of specified type */
struct obj *
nxtobj(struct obj *obj, int type, boolean by_nexthere)
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

/* return inventory object of type 'type' if hero has one, otherwise Null */
struct obj *
carrying(int type)
{
    register struct obj *otmp;

    /* this could be replaced by 'return m_carrying(&g.youmonst, type);' */
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == type)
            break;
    return otmp;
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
currency(long amount)
{
    const char *res;

    res = Hallucination ? currencies[rn2(SIZE(currencies))] : "zorkmid";
    if (amount != 1L)
        res = makeplural(res);
    return res;
}

struct obj *
u_carried_gloves(void) {
    struct obj *otmp, *gloves = (struct obj *) 0;

    if (uarmg) {
        gloves = uarmg;
    } else {
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (is_gloves(otmp)) {
                gloves = otmp;
                break;
            }
    }
    return gloves;
}


/* 3.6 tribute */
struct obj *
u_have_novel(void)
{
    register struct obj *otmp;

    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == SPE_NOVEL)
            return otmp;
    return (struct obj *) 0;
}

struct obj *
o_on(unsigned int id, struct obj *objchn)
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
obj_here(struct obj *obj, coordxy x, coordxy y)
{
    register struct obj *otmp;

    for (otmp = g.level.objects[x][y]; otmp; otmp = otmp->nexthere)
        if (obj == otmp)
            return TRUE;
    return FALSE;
}

struct obj *
g_at(coordxy x, coordxy y)
{
    register struct obj *obj = g.level.objects[x][y];

    while (obj) {
        if (obj->oclass == COIN_CLASS)
            return obj;
        obj = obj->nexthere;
    }
    return (struct obj *) 0;
}

/* compact a string of inventory letters by dashing runs of letters */
static void
compactify(char *buf)
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
splittable(struct obj *obj)
{
    return !((obj->otyp == LOADSTONE && obj->cursed)
             || (obj == uwep && welded(uwep)));
}

/* match the prompt for either 'T' or 'R' command */
static boolean
taking_off(const char *action)
{
    return !strcmp(action, "take off") || !strcmp(action, "remove");
}

static void
mime_action(const char *word)
{
    char buf[BUFSZ];
    char *bp, *pfx, *sfx;

    Strcpy(buf, word);
    bp = pfx = sfx = (char *) 0;

    if ((bp = strstr(buf, " on the ")) != 0) {
        /* rub on the stone[s] */
        *bp = '\0';
        sfx = (bp + 1); /* "something <sfx>" */
    }
    if ((!strncmp(buf, "rub the ", 8) && strstr(buf + 8, " on"))
        || (!strncmp(buf, "dip ", 4) && strstr(buf + 4, " into"))) {
        /* "rub the royal jelly on" -> "rubbing the royal jelly on", or
           "dip <foo> into" => "dipping <foo> into" */
        buf[3] = '\0';
        pfx = &buf[3 + 1]; /* "<pfx> something" */
    }
    if ((bp = strstr(buf, " or ")) != 0) {
        *bp = '\0';
        bp = (rn2(2) ? buf : (bp + 4));
    } else
        bp = buf;

    You("mime %s%s%s something%s%s.", ing_suffix(bp),
        pfx ? " " : "", pfx ? pfx : "", sfx ? " " : "", sfx ? sfx : "");
}

/* getobj callback that allows any object - but not hands. */
int
any_obj_ok(struct obj *obj)
{
    if (obj)
        return GETOBJ_SUGGEST;
    return GETOBJ_EXCLUDE;
}

/*
 * getobj returns:
 *      struct obj *xxx:        object to do something with.
 *      (struct obj *) 0        error return: no object.
 *      &cg.zeroobj             explicitly no object (as in w-).
 * The obj_ok callback should not have side effects (apart from
 * abnormal-behavior things like impossible calls); it can be called multiple
 * times on the same object during the execution of this function.
 * Callbacks' argument is either a valid object pointer or a null pointer,
 * which represents the validity of doing that action on HANDS_SYM. getobj
 * won't call it with &cg.zeroobj, so its behavior can be undefined in that
 * case.
 */
struct obj *
getobj(
    const char *word,       /* usually a direct verb such as "drop" */
    int (*obj_ok)(OBJ_P),   /* callback to classify an object's suitability */
    unsigned int ctrlflags) /* some control to fine-tune the behavior */
{
    register struct obj *otmp;
    register char ilet = 0;
    char buf[BUFSZ], qbuf[QBUFSZ];
    char lets[BUFSZ], altlets[BUFSZ];
    register int suggested = 0;
    register char *bp = buf, *ap = altlets;
    boolean allowcnt = (ctrlflags & GETOBJ_ALLOWCNT),
            forceprompt = (ctrlflags & GETOBJ_PROMPT),
            allownone = FALSE;
    int inaccess = 0; /* counts GETOBJ_EXCLUDE_INACCESS items to decide
                       * between "you don't have anything to <foo>"
                       * versus "you don't have anything _else_ to <foo>"
                       * (also used for GETOBJ_EXCLUDE_NONINVENT) */
    long cnt = 0L;
    boolean cntgiven = FALSE;
    boolean msggiven = FALSE;
    boolean oneloop = FALSE;
    Loot *sortedinvent, *srtinv;
    struct _cmd_queue cq, *cmdq;
    boolean need_more_cq = FALSE;

 need_more_cq:
    if ((cmdq = cmdq_pop()) != 0) {
        cq = *cmdq;
        free(cmdq);
        /* user-input means pick something interactively now, with more
           in the command queue for after that; if not user-input, it
           has to be a key here */
        if (cq.typ != CMDQ_USER_INPUT) {
            otmp = 0; /* in case of non-key or lookup failure */
            if (cq.typ == CMDQ_KEY) {
                int v;

                if (cq.key == HANDS_SYM) {
                    /* check whether the hands/self choice is suitable */
                    v = (*obj_ok)((struct obj *) 0);
                    if (v == GETOBJ_SUGGEST || v == GETOBJ_DOWNPLAY)
                        otmp = (struct obj *) &cg.zeroobj;
                } else {
                    /* there could be more than one match if key is '#';
                       take first one which passes the obj_ok callback */
                    for (otmp = g.invent; otmp; otmp = otmp->nobj)
                        if (otmp->invlet == cq.key) {
                            v = (*obj_ok)(otmp);
                            if (v == GETOBJ_SUGGEST || v == GETOBJ_DOWNPLAY)
                                break;
                        }
                }
            } else if (cq.typ == CMDQ_INT) {
                /* getting a partial stack */
                if (!cntgiven && allowcnt) {
                    cnt = cq.intval;
                    cntgiven = TRUE;
                    goto need_more_cq; /* now, get CMDQ_KEY */
                } else {
                    cmdq_clear(CQ_CANNED); /* this should maybe clear the CQ_REPEAT too? */
                    return NULL;
                }
            }
            if (!otmp)        /* didn't find what we were looking for, */
                cmdq_clear(CQ_CANNED); /* so discard any other queued commands  */
            else if (cntgiven) {
                /* if stack is smaller than count, drop the whole stack */
                if (cnt < 1 || otmp->quan <= cnt)
                    cntgiven = FALSE;
                goto split_otmp;
            }
            return otmp;
        } /* !CMDQ_USER_INPUT */
    } else if (need_more_cq) {
        return NULL;
    }

    /* is "hands"/"self" a valid thing to do this action on? */
    switch ((*obj_ok)((struct obj *) 0)) {
    case GETOBJ_SUGGEST: /* treat as likely candidate */
        allownone = TRUE;
        *bp++ = HANDS_SYM;
        *bp++ = ' '; /* put a space after the '-' in the prompt */
        break;
    case GETOBJ_DOWNPLAY: /* acceptable but not shown as likely chioce */
    case GETOBJ_EXCLUDE_INACCESS:   /* nothing currently gives this for '-' but
                                     * theoretically could if wearing gloves */
    case GETOBJ_EXCLUDE_SELECTABLE: /* ditto, I think... */
        allownone = TRUE;
        *ap++ = HANDS_SYM;
        break;
    case GETOBJ_EXCLUDE_NONINVENT: /* player skipped some alternative that's
                                    * not in inventory, now the hands/self
                                    * possibility is telling us so */
        forceprompt = FALSE;
        inaccess++;
        break;
    default:
        break;
    }

    if (!flags.invlet_constant)
        reassign();

    /* force invent to be in invlet order before collecting candidate
       inventory letters */
    sortedinvent = sortloot(&g.invent, SORTLOOT_INVLET, FALSE,
                            (boolean (*)(OBJ_P)) 0);

    for (srtinv = sortedinvent; (otmp = srtinv->obj) != 0; ++srtinv) {
        if (&bp[suggested] == &buf[sizeof buf - 1]
            || ap == &altlets[sizeof altlets - 1]) {
            /* we must have a huge number of noinvsym items somehow */
            impossible("getobj: inventory overflow");
            break;
        }

        bp[suggested++] = otmp->invlet;
        switch ((*obj_ok)(otmp)) {
        case GETOBJ_EXCLUDE_INACCESS:
            /* remove inaccessible things */
            suggested--;
            inaccess++;
            break;
        case GETOBJ_EXCLUDE:
        case GETOBJ_EXCLUDE_SELECTABLE:
            /* remove more inappropriate things, but unlike the first it won't
               trigger an "else" in "you don't have anything else to ___" */
            suggested--;
            break;
        case GETOBJ_DOWNPLAY:
            /* acceptable but not listed as likely candidates in the prompt
               or in the inventory subset if player responds with '?' - thus,
               don't add it to lets with bp, but add it to altlets with ap */
            suggested--;
            forceprompt = TRUE;
            *ap++ = otmp->invlet;
            break;
        case GETOBJ_SUGGEST:
            break; /* adding otmp->invlet is all that's needed */
        case GETOBJ_EXCLUDE_NONINVENT: /* not applicable for invent items */
        default:
            impossible("bad return from getobj callback");
        }
    }
    unsortloot(&sortedinvent);

    bp[suggested] = '\0';
    /* If no objects were suggested but we added '- ' at the beginning for
     * hands, destroy the trailing space */
    if (suggested == 0 && bp > buf && bp[-1] == ' ')
        *--bp = '\0';
    Strcpy(lets, bp); /* necessary since we destroy buf */
    if (suggested > 5)      /* compactify string */
        compactify(bp);
    *ap = '\0';

    if (suggested == 0 && !forceprompt && !allownone) {
        You("don't have anything %sto %s.", inaccess ? "else " : "", word);
        return (struct obj *) 0;
    }
    for (;;) {
        cnt = 0;
        cntgiven = FALSE;
        Sprintf(qbuf, "What do you want to %s?", word);
        if (g.in_doagain) {
            ilet = readchar();
        } else if (iflags.force_invmenu) {
            /* don't overwrite a possible quitchars */
            if (!oneloop)
                ilet = (*lets || *altlets) ? '?' : '*';
            if (!msggiven)
                putmsghistory(qbuf, FALSE);
            msggiven = TRUE;
            oneloop = TRUE;
        } else {
            if (!buf[0])
                Strcat(qbuf, " [*]");
            else
                Sprintf(eos(qbuf), " [%s or ?*]", buf);
            ilet = yn_function(qbuf, (char *) 0, '\0', FALSE);
        }
        if (digit(ilet)) {
            long tmpcnt = 0;

            if (!allowcnt) {
                pline("No count allowed with this command.");
                continue;
            }
            ilet = get_count(NULL, ilet, LARGEST_INT, &tmpcnt, GC_SAVEHIST);
            if (tmpcnt) {
                cnt = tmpcnt;
                cntgiven = TRUE;
            }
        }
        if (strchr(quitchars, ilet)) {
            if (Verbose(1, getobj1))
                pline1(Never_mind);
            return (struct obj *) 0;
        }
        if (ilet == HANDS_SYM) { /* '-' */
            if (!allownone)
                mime_action(word);
            return (allownone ? (struct obj *) &cg.zeroobj : (struct obj *) 0);
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
                Sprintf(qbuf, "your %s", fingers_or_gloves(FALSE));
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
            if (!ilet) {
                if (oneloop)
                    return (struct obj *) 0;
                continue;
            }
            if (ilet == HANDS_SYM)
                return (struct obj *) &cg.zeroobj; /* cast away 'const' */
            if (ilet == '\033') {
                if (Verbose(1, getobj2))
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
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (otmp->invlet == ilet)
                break;
        /* some items have restrictions */
        if (ilet == GOLD_SYM
            /* guard against the [hypothetical] chance of having more
               than one invent slot of gold and picking the non-'$' one */
            || (otmp && otmp->oclass == COIN_CLASS)) {
            if (otmp && obj_ok(otmp) <= GETOBJ_EXCLUDE) {
                You("cannot %s gold.", word);
                return (struct obj *) 0;
            }
            /*
             * Historical note: early Nethack had a bug which was
             * first reported for Larn, where trying to drop 2^32-n
             * gold pieces was allowed, and did interesting things to
             * your money supply.  The LRS is the tax bureau from Larn.
             */
            if (cntgiven && cnt <= 0) {
                if (cnt < 0)
                    pline_The(
                  "LRS would be very interested to know you have that much.");
                return (struct obj *) 0;
            }
        }
        if (cntgiven && !strcmp(word, "throw")) {
            /* permit counts for throwing gold, but don't accept counts
               for other things since the throw code will split off a
               single item anyway; if populating quiver, 'word' will be
               "ready" or "fire" and this restriction doesn't apply */
            if (cnt == 0)
                return (struct obj *) 0;
            if (cnt > 1 && (ilet != def_oc_syms[COIN_CLASS].sym
                && !(otmp && otmp->oclass == COIN_CLASS))) {
                You("can only throw one item at a time.");
                continue;
            }
        }
        g.context.botl = 1; /* May have changed the amount of money */
        if (otmp && !g.in_doagain) {
            if (cntgiven && cnt > 0)
                cmdq_add_int(CQ_REPEAT, cnt);
            cmdq_add_key(CQ_REPEAT, ilet);
        }
        /* [we used to set otmp (by finding ilet in invent) here, but
           that's been moved above so that otmp can be checked earlier] */
        /* verify the chosen object */
        if (!otmp) {
            You("don't have that object.");
            if (g.in_doagain)
                return (struct obj *) 0;
            continue;
        } else if (cnt < 0 || otmp->quan < cnt) {
            You("don't have that many!  You have only %ld.", otmp->quan);
            if (g.in_doagain)
                return (struct obj *) 0;
            continue;
        }
        break;
    }
    if (obj_ok(otmp) == GETOBJ_EXCLUDE) {
        silly_thing(word, otmp);
        return (struct obj *) 0;
    }
split_otmp:
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

DISABLE_WARNING_FORMAT_NONLITERAL

void
silly_thing(const char *word,
#ifdef OBSOLETE_HANDLING
            struct obj *otmp)
#else
            struct obj *otmp UNUSED)
#endif
{
#ifdef OBSOLETE_HANDLING
    /* 'P','R' vs 'W','T' handling is obsolete */
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
    /* see comment about Amulet of Yendor in objtyp_is_callable(do_name.c);
       known fakes yield the silly thing feedback */
    if (!strcmp(word, "call")
        && (otmp->otyp == AMULET_OF_YENDOR
            || (otmp->otyp == FAKE_AMULET_OF_YENDOR && !otmp->known)))
        pline_The("Amulet doesn't like being called names.");
    else
        pline(silly_thing_to, word);
}

RESTORE_WARNING_FORMAT_NONLITERAL

static int
ckvalidcat(struct obj *otmp)
{
    /* use allow_category() from pickup.c */
    return (int) allow_category(otmp);
}

static int
ckunpaid(struct obj *otmp)
{
    return (otmp->unpaid || (Has_contents(otmp) && count_unpaid(otmp->cobj)));
}

boolean
wearing_armor(void)
{
    return (boolean) (uarm || uarmc || uarmf || uarmg
                      || uarmh || uarms || uarmu);
}

boolean
is_worn(struct obj *otmp)
{
    return (otmp->owornmask & (W_ARMOR | W_ACCESSORY | W_SADDLE | W_WEAPONS))
            ? TRUE
            : FALSE;
}

/* extra xprname() input that askchain() can't pass through safe_qbuf() */
static struct xprnctx {
    char let;
    boolean dot;
} safeq_xprn_ctx;

/* safe_qbuf() -> short_oname() callback */
static char *
safeq_xprname(struct obj *obj)
{
    return xprname(obj, (char *) 0, safeq_xprn_ctx.let, safeq_xprn_ctx.dot,
                   0L, 0L);
}

/* alternate safe_qbuf() -> short_oname() callback */
static char *
safeq_shortxprname(struct obj *obj)
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
ggetobj(const char *word, int (*fn)(OBJ_P), int mx,
        boolean combo, /* combination menu flag */
        unsigned *resultflags)
{
    int (*ckfn)(OBJ_P) = (int (*)(OBJ_P)) 0;
    boolean (*ofilter)(OBJ_P) = (boolean (*)(OBJ_P)) 0;
    boolean takeoff, ident, allflag, m_seen;
    int itemcount;
    int oletct, iletct, unpaid, oc_of_sym;
    char sym, *ip, olets[MAXOCLASSES + 6], ilets[MAXOCLASSES + 11];
    char extra_removeables[3 + 1]; /* uwep,uswapwep,uquiver */
    char buf[BUFSZ] = DUMMY, qbuf[QBUFSZ];

    if (!g.invent) {
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

    iletct = collect_obj_classes(ilets, g.invent, FALSE, ofilter, &itemcount);
    unpaid = count_unpaid(g.invent);

    if (ident && !iletct) {
        return -1; /* no further identifications */
    } else if (g.invent) {
        ilets[iletct++] = ' ';
        if (unpaid)
            ilets[iletct++] = 'u';
        if (count_buc(g.invent, BUC_BLESSED, ofilter))
            ilets[iletct++] = 'B';
        if (count_buc(g.invent, BUC_UNCURSED, ofilter))
            ilets[iletct++] = 'U';
        if (count_buc(g.invent, BUC_CURSED, ofilter))
            ilets[iletct++] = 'C';
        if (count_buc(g.invent, BUC_UNKNOWN, ofilter))
            ilets[iletct++] = 'X';
        if (count_justpicked(g.invent))
            ilets[iletct++] = 'P';
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
        if (strchr(buf, 'i')) {
            char ailets[1+26+26+1+5+1]; /* $ + a-z + A-Z + # + slop + \0 */
            struct obj *otmp;

            /* applicable inventory letters; if empty, show entire invent */
            ailets[0] = '\0';
            if (ofilter)
                for (otmp = g.invent; otmp; otmp = otmp->nobj)
                    /* strchr() check: limit overflow items to one '#' */
                    if ((*ofilter)(otmp) && !strchr(ailets, otmp->invlet))
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
            if (strchr(extra_removeables, oc_of_sym)) {
                ; /* skip rest of takeoff checks */
            } else if (!strchr(removeables, oc_of_sym)) {
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

        if (sym == 'a') {
            allflag = TRUE;
        } else if (sym == 'A') {
            ; /* same as the default */
        } else if (sym == 'u') {
            add_valid_menu_class('u');
            ckfn = ckunpaid;
        } else if (strchr("BUCXP", sym)) {
            add_valid_menu_class(sym); /* 'B','U','C','X', or 'P' */
            ckfn = ckvalidcat;
        } else if (sym == 'm') {
            m_seen = TRUE;
        } else if (oc_of_sym == MAXOCLASSES) {
            You("don't have any %c's.", sym);
        } else {
            if (!strchr(olets, oc_of_sym)) {
                add_valid_menu_class(oc_of_sym);
                olets[oletct++] = oc_of_sym;
                olets[oletct] = '\0';
            }
        }
    }

    if (m_seen) {
        return (allflag
                || (!oletct && ckfn != ckunpaid && ckfn != ckvalidcat))
               ? -2 : -3;
    } else if (flags.menu_style != MENU_TRADITIONAL && combo && !allflag) {
        return 0;
    } else {
        int cnt = askchain(&g.invent, olets, allflag, fn, ckfn, mx, word);
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
 */
int
askchain(
    struct obj **objchn, /* *objchn might change */
    const char *olets,   /* olets is an Obj Class char array */
    int allflag,         /* bypass prompting about individual items */
    int (*fn)(OBJ_P),    /* action to perform on selected items */
    int (*ckfn)(OBJ_P),  /* callback to decided if an item is selectable */
    int mx,              /* if non-0, maximum number of objects to process */
    const char *word)    /* name of the action */
{
    struct obj *otmp, *otmpo;
    char sym, ilet;
    int cnt = 0, dud = 0, tmp;
    boolean takeoff, nodot, ident, take_out, put_in, first, ininv, bycat;
    char qbuf[QBUFSZ], qpfx[QBUFSZ];
    Loot *sortedchn = 0;

    takeoff = taking_off(word);
    ident = !strcmp(word, "identify");
    take_out = !strcmp(word, "take out");
    put_in = !strcmp(word, "put in");
    nodot = (!strcmp(word, "nodot") || !strcmp(word, "drop") || ident
             || takeoff || take_out || put_in);
    ininv = (*objchn == g.invent);
    bycat = (menu_class_present('u')
             || menu_class_present('B') || menu_class_present('U')
             || menu_class_present('C') || menu_class_present('X')
             || menu_class_present('P'));

    /* someday maybe we'll sort by 'olets' too (temporarily replace
       flags.packorder and pass SORTLOOT_PACK), but not yet... */
    sortedchn = sortloot(objchn, SORTLOOT_INVLET, FALSE,
                         (boolean (*)(OBJ_P)) 0);

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
     * Multiple Drop can change the g.invent chain while it operates
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
    while ((otmp = nxt_unbypassed_loot(sortedchn, *objchn)) != 0) {
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
            if (tmp <= 0) {
                if (container_gone(fn)) {
                    /* otmp caused magic bag to explode;
                       both are now gone */
                    otmp = 0; /* and return */
                } else if (otmp && otmp != otmpo) {
                    /* split occurred, merge again */
                    (void) unsplitobj(otmp);
                }
                if (tmp < 0)
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
    unsortloot(&sortedchn);
    /* can't just clear bypass bit of items in objchn because the action
       applied to selected ones might move them to a different chain */
    /*bypass_objlist(*objchn, FALSE);*/
    clear_bypasses();
    return cnt;
}

/*
 *      Object identification routines:
 */

/* set the cknown and lknown flags on an object if they're applicable */
void
set_cknown_lknown(struct obj *obj)
{
    if (Is_container(obj) || obj->otyp == STATUE)
        obj->cknown = obj->lknown = 1;
    else if (obj->otyp == TIN)
        obj->cknown = 1;
    /* TODO? cknown might be extended to candy bar, where it would mean that
       wrapper's text was known which in turn indicates candy bar's content */
    return;
}

/* make an object actually be identified; no display updating */
void
fully_identify_obj(struct obj *otmp)
{
    makeknown(otmp->otyp);
    if (otmp->oartifact)
        discover_artifact((coordxy) otmp->oartifact);
    otmp->known = otmp->dknown = otmp->bknown = otmp->rknown = 1;
    set_cknown_lknown(otmp); /* set otmp->{cknown,lknown} if applicable */
    if (otmp->otyp == EGG && otmp->corpsenm != NON_PM)
        learn_egg_type(otmp->corpsenm);
}

/* ggetobj callback routine; identify an object and give immediate feedback */
int
identify(struct obj *otmp)
{
    fully_identify_obj(otmp);
    prinv((char *) 0, otmp, 0L);
    return 1;
}

/* menu of unidentified objects; select and identify up to id_limit of them */
static void
menu_identify(int id_limit)
{
    menu_item *pick_list;
    int n, i, first = 1, tryct = 5;
    char buf[BUFSZ];
    /* assumptions:  id_limit > 0 and at least one unID'd item is present */

    while (id_limit) {
        Sprintf(buf, "What would you like to identify %s?",
                first ? "first" : "next");
        n = query_objlist(buf, &g.invent, (SIGNAL_NOMENU | SIGNAL_ESCAPE
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
/* count the unidentified items */
int
count_unidentified(struct obj *objchn)
{
    int unid_cnt = 0;
    struct obj *obj;

    for (obj = objchn; obj; obj = obj->nobj)
        if (not_fully_identified(obj))
            ++unid_cnt;
    return unid_cnt;
}

/* dialog with user to identify a given number of items; 0 means all */
void
identify_pack(int id_limit,
              boolean learning_id) /* T: just read unknown identify scroll */
{
    struct obj *obj;
    int n, unid_cnt = count_unidentified(g.invent);

    if (!unid_cnt) {
        You("have already identified %s of your possessions.",
            !learning_id ? "all" : "the rest");
    } else if (!id_limit || id_limit >= unid_cnt) {
        /* identify everything */
        /* TODO:  use fully_identify_obj and cornline/menu/whatever here */
        for (obj = g.invent; obj; obj = obj->nobj) {
            if (not_fully_identified(obj)) {
                (void) identify(obj);
                if (unid_cnt == 1)
                    break;
            }
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
learn_unseen_invent(void)
{
    struct obj *otmp;

    if (Blind)
        return; /* sanity check */

    for (otmp = g.invent; otmp; otmp = otmp->nobj) {
        if (otmp->dknown && (otmp->bknown || !Role_if(PM_CLERIC)))
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

/* persistent inventory window is maintained by interface code;
   'update_inventory' used to be a macro for
   (*windowprocs.win_update_inventory) but the restore hackery to suppress
   screen updates was getting out of hand; this is now a central call point */
void
update_inventory(void)
{
    int save_suppress_price;

    if (!g.program_state.in_moveloop) /* not covered by suppress_map_output */
        return;
    if (suppress_map_output()) /* despite name, used for perm_invent too */
        return;

    /*
     * Ought to check (windowprocs.wincap & WC_PERM_INVENT) here....
     *
     * We currently don't skip this call when iflags.perm_invent is False
     * because curses uses that to disable a previous perm_invent window
     * (after toggle via 'O'; perhaps the options code should handle that).
     *
     * perm_invent might get updated while some code is avoiding price
     * feedback during obj name formatting for messages.  Temporarily
     * force 'normal' formatting during the perm_invent update.  (Cited
     * example was an update triggered by change in invent gold when
     * transferring some to shk during itemized billing.  A previous fix
     * attempt in the shop code handled it for unpaid items but not for
     * paying for used-up shop items; that follows a different code path.)
     */
    save_suppress_price = iflags.suppress_price;
    iflags.suppress_price = 0;
#if defined(TTY_PERM_INVENT)
    if (WINDOWPORT(tty))
        sync_perminvent();
    else
#endif
        (*windowprocs.win_update_inventory)(0);
    iflags.suppress_price = save_suppress_price;
}

/* the #perminv command - call interface's persistent inventory routine */
int
doperminv(void)
{
    /*
     * If persistent inventory window is enabled, interact with it.
     *
     * Depending on interface, might accept and execute one scrolling
     * request (MENU_{FIRST,NEXT,PREVIOUS,LAST}_PAGE) then return,
     * or might stay and handle multiple requests until user finishes
     * (typically by typing <return> or <esc> but that's up to interface).
     */

#if 0
    /* [currently this would redraw the persistent inventory window
       whether that's needed or not, so also reset any previous
       scrolling; we don't want that if the interface only accepts
       one scroll command at a time] */
    update_inventory(); /* make sure that it's up to date */
#endif

    if ((windowprocs.wincap & WC_PERM_INVENT) == 0) {
        /* [TODO? perhaps omit "by <interface>" if all the window ports
           compiled into this binary lack support for perm_invent...] */
        pline("Persistent inventory display is not supported by '%s'.",
              windowprocs.name);

    } else if (!iflags.perm_invent) {
        pline(
     "Persistent inventory ('perm_invent' option) is not presently enabled.");

    } else if (!g.invent) {
        /* [should this be left for the interface to decide?] */
        pline("Persistent inventory display is empty.");

    } else {
        /* note: we used to request a scrolling key here and pass that to
           (*win_update_inventory)(key), but that limited the functionality
           and also cluttered message history with prompt and response so
           just send non-zero and have the interface be responsible for it */
        (*windowprocs.win_update_inventory)(1);

    } /* iflags.perm_invent */

    return ECMD_OK;
}

/* should of course only be called for things in invent */
static char
obj_to_let(struct obj *obj)
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
prinv(const char *prefix, struct obj *obj, long quan)
{
    boolean total_of = (quan && (quan < obj->quan));
    char totalbuf[QBUFSZ];

    if (!prefix)
        prefix = "";

    totalbuf[0] = '\0';
    if (total_of)
        Snprintf(totalbuf, sizeof totalbuf,
                 " (%ld in total).", obj->quan);
    pline("%s%s%s%s", prefix, *prefix ? " " : "",
          xprname(obj, (char *) 0, obj_to_let(obj), !total_of, 0L, quan),
          Verbose(3, prinv) ? totalbuf : "");
}

DISABLE_WARNING_FORMAT_NONLITERAL

char *
xprname(struct obj *obj,
        const char *txt, /* text to print instead of obj */
        char let,        /* inventory letter */
        boolean dot,     /* append period; (dot && cost => Iu) */
        long cost,       /* cost (for inventory of unpaid or expended items) */
        long quan)       /* if non-0, print this quantity, not obj->quan */
{
    static char li[BUFSZ];
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

RESTORE_WARNING_FORMAT_NONLITERAL

enum item_action_actions {
    IA_NONE          = 0,
    IA_UNWIELD, /* hack for 'w-' */
    IA_APPLY_OBJ, /* 'a' */
    IA_DIP_OBJ, /* 'a' on a potion == dip */
    IA_NAME_OBJ, /* 'c' name individual item */
    IA_NAME_OTYP, /* 'C' name item's type */
    IA_DROP_OBJ, /* 'd' */
    IA_EAT_OBJ, /* 'e' */
    IA_ENGRAVE_OBJ, /* 'E' */
    IA_ADJUST_OBJ, /* 'i' #adjust inventory letter */
    IA_ADJUST_STACK, /* 'I' #adjust with count to split stack */
    IA_SACRIFICE, /* 'O' offer sacrifice */
    IA_BUY_OBJ, /* 'p' pay shopkeeper */
    IA_QUAFF_OBJ,
    IA_QUIVER_OBJ,
    IA_READ_OBJ,
    IA_RUB_OBJ,
    IA_THROW_OBJ,
    IA_TAKEOFF_OBJ,
    IA_TIP_CONTAINER,
    IA_INVOKE_OBJ,
    IA_WIELD_OBJ,
    IA_WEAR_OBJ,
    IA_SWAPWEAPON,
    IA_ZAP_OBJ,
};

/* construct text for the menu entries for IA_NAME_OBJ and IA_NAME_OTYP */
static boolean
item_naming_classification(struct obj *obj, char *onamebuf, char *ocallbuf)
{
    static const char Name[] = "Name", Rename[] = "Rename or un-name",
                      /* "re-call" seems a bit weird, but "recall" and
                         "rename" don't fit for changing a type name */
                      Call[] = "Call", Recall[] = "Re-call or un-call";

    onamebuf[0] = ocallbuf[0] = '\0';
    if (name_ok(obj) == GETOBJ_SUGGEST) {
        Sprintf(onamebuf, "%s %s %s",
                (!has_oname(obj) || !*ONAME(obj)) ? Name : Rename,
                the_unique_obj(obj) ? "the"
                : !is_plural(obj) ? "this"
                  : "these",
                simpleonames(obj));
    }
    if (call_ok(obj) == GETOBJ_SUGGEST) {
        char *callname = simpleonames(obj);

        /* prefix known unique item with "the", make all other types plural */
        if (the_unique_obj(obj)) /* treats unID'd fake amulets as if real */
            callname = the(callname);
        else if (!is_plural(obj))
            callname = makeplural(callname);
        Sprintf(ocallbuf, "%s the type for %s",
                (!objects[obj->otyp].oc_uname
                 || !*objects[obj->otyp].oc_uname) ? Call : Recall,
                callname);
    }
    return (*onamebuf || *ocallbuf) ? TRUE : FALSE;
}

/* construct text for the menu entries for IA_READ_OBJ */
static int
item_reading_classification(struct obj *obj, char *outbuf)
{
    int otyp = obj->otyp, res = IA_READ_OBJ;

    *outbuf = '\0';
    if (otyp == FORTUNE_COOKIE) {
        Strcpy(outbuf, "Read the message inside this cookie");
    } else if (otyp == T_SHIRT) {
        Strcpy(outbuf, "Read the slogan on the shirt");
    } else if (otyp == ALCHEMY_SMOCK) {
        Strcpy(outbuf, "Read the slogan on the apron");
    } else if (otyp == HAWAIIAN_SHIRT) {
        Strcpy(outbuf, "Look at the pattern on the shirt");
    } else if (obj->oclass == SCROLL_CLASS) {
        const char *magic = ((obj->dknown
#ifdef MAIL_STRUCTURES
                              && otyp != SCR_MAIL
#endif
                              && (otyp != SCR_BLANK_PAPER
                                  || !objects[otyp].oc_name_known))
                             ? " to activate its magic" : "");

        Sprintf(outbuf, "Read this scroll%s", magic);
    } else if (obj->oclass == SPBOOK_CLASS) {
        boolean novel = (otyp == SPE_NOVEL),
                blank = (otyp == SPE_BLANK_PAPER
                         && objects[otyp].oc_name_known),
                tome = (otyp == SPE_BOOK_OF_THE_DEAD
                        && objects[otyp].oc_name_known);

        Sprintf(outbuf, "%s this %s",
                (novel || blank) ? "Read" : tome ? "Examine" : "Study",
                novel ? simpleonames(obj) /* "novel" or "paperback book" */
                      : tome ? "tome" : "spellbook");
    } else {
        res = IA_NONE;
    }
    return res;
}

static void
ia_addmenu(winid win, int act, char let, const char *txt)
{
    anything any;
    int clr = 0;

    any = cg.zeroany;
    any.a_int = act;
    add_menu(win, &nul_glyphinfo, &any, let, 0,
             ATR_NONE, clr, txt, MENU_ITEMFLAGS_NONE);
}

/* Show menu of possible actions hero could do with item otmp */
static int
itemactions(struct obj *otmp)
{
    int n, act = IA_NONE;
    winid win;
    char buf[BUFSZ], buf2[BUFSZ];
    menu_item *selected;
    struct monst *mtmp;
    const char *light = otmp->lamplit ? "Extinguish" : "Light";
    boolean already_worn = (otmp->owornmask & (W_ARMOR | W_ACCESSORY)) != 0;

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);

    /* -: unwield; picking current weapon offers an opportunity for 'w-'
       to wield bare/gloved hands; likewise for 'Q-' with quivered item(s) */
    if (otmp == uwep || otmp == uswapwep || otmp == uquiver) {
        const char *verb = (otmp == uquiver) ? "Quiver" : "Wield",
                   *action = (otmp == uquiver) ? "un-ready" : "un-wield",
                   *which = is_plural(otmp) ? "these" : "this",
                   *what = ((otmp->oclass == WEAPON_CLASS || is_weptool(otmp))
                            ? "weapon" : "item");
        /*
         * TODO: if uwep is ammo, tell player that to shoot instead of toss,
         *       the corresponding launcher must be wielded;
         */
        Sprintf(buf,  "%s '%c' to %s %s %s",
                verb, HANDS_SYM, action, which,
                is_plural(otmp) ? makeplural(what) : what);
        ia_addmenu(win, IA_UNWIELD, '-', buf);
    }

    /* a: apply */
    if (otmp->otyp == CREAM_PIE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Hit yourself with this cream pie");
    else if (otmp->otyp == BULLWHIP)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Lash out with this whip");
    else if (otmp->otyp == GRAPPLING_HOOK)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Grapple something with this hook");
    else if (otmp->otyp == BAG_OF_TRICKS && objects[otmp->otyp].oc_name_known)
        /* bag of tricks skips this unless discovered */
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Reach into this bag");
    else if (Is_container(otmp))
        /* bag of tricks gets here only if not yet discovered */
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Open this container");
    else if (otmp->otyp == CAN_OF_GREASE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Use the can to grease an item");
    else if (otmp->otyp == LOCK_PICK
             || otmp->otyp == CREDIT_CARD
             || otmp->otyp == SKELETON_KEY)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Use this tool to pick a lock");
    else if (otmp->otyp == TINNING_KIT)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Use this kit to tin a corpse");
    else if (otmp->otyp == LEASH)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Tie a pet to this leash");
    else if (otmp->otyp == SADDLE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Place this saddle on a pet");
    else if (otmp->otyp == MAGIC_WHISTLE
             || otmp->otyp == TIN_WHISTLE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Blow this whistle");
    else if (otmp->otyp == EUCALYPTUS_LEAF)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Use this leaf as a whistle");
    else if (otmp->otyp == STETHOSCOPE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Listen through the stethoscope");
    else if (otmp->otyp == MIRROR)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Show something its reflection");
    else if (otmp->otyp == BELL || otmp->otyp == BELL_OF_OPENING)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Ring the bell");
    else if (otmp->otyp == CANDELABRUM_OF_INVOCATION) {
        Sprintf(buf, "%s the candelabrum", light);
        ia_addmenu(win, IA_APPLY_OBJ, 'a', buf);
    } else if (otmp->otyp == WAX_CANDLE || otmp->otyp == TALLOW_CANDLE) {
        Sprintf(buf, "%s %s %s", light,
                is_plural(otmp) ? "these" : "this",
                simpleonames(otmp));
        ia_addmenu(win, IA_APPLY_OBJ, 'a', buf);
    } else if (otmp->otyp == OIL_LAMP || otmp->otyp == MAGIC_LAMP
               || otmp->otyp == BRASS_LANTERN) {
        Sprintf(buf, "%s this light source", light);
        ia_addmenu(win, IA_APPLY_OBJ, 'a', buf);
    } else if (otmp->otyp == POT_OIL && objects[otmp->otyp].oc_name_known) {
        Sprintf(buf, "%s this oil", light);
        ia_addmenu(win, IA_APPLY_OBJ, 'a', buf);
    } else if (otmp->oclass == POTION_CLASS) {
        /* FIXME? this should probably be moved to 'D' rather than be 'a' */
        Sprintf(buf, "Dip something into %s potion%s",
                is_plural(otmp) ? "one of these" : "this", plur(otmp->quan));
        ia_addmenu(win, IA_DIP_OBJ, 'a', buf);
    } else if (otmp->otyp == EXPENSIVE_CAMERA)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Take a photograph");
    else if (otmp->otyp == TOWEL)
        ia_addmenu(win, IA_APPLY_OBJ, 'a',
                   "Clean yourself off with this towel");
    else if (otmp->otyp == CRYSTAL_BALL)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Peer into this crystal ball");
    else if (otmp->otyp == MAGIC_MARKER)
        ia_addmenu(win, IA_APPLY_OBJ, 'a',
                   "Write on something with this marker");
    else if (otmp->otyp == FIGURINE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Make this figurine transform");
    else if (otmp->otyp == UNICORN_HORN)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Use this unicorn horn");
    else if (otmp->otyp == HORN_OF_PLENTY && objects[otmp->otyp].oc_name_known)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Blow into the horn of plenty");
    else if (otmp->otyp >= WOODEN_FLUTE && otmp->otyp <= DRUM_OF_EARTHQUAKE)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Play this musical instrument");
    else if (otmp->otyp == LAND_MINE || otmp->otyp == BEARTRAP)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Arm this trap");
    else if (otmp->otyp == PICK_AXE || otmp->otyp == DWARVISH_MATTOCK)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Dig with this digging tool");
    else if (otmp->oclass == WAND_CLASS)
        ia_addmenu(win, IA_APPLY_OBJ, 'a', "Break this wand");

    /* 'c', 'C' - call an item or its type something */
    if (item_naming_classification(otmp, buf, buf2)) {
        if (*buf)
            ia_addmenu(win, IA_NAME_OBJ, 'c', buf);
        if (*buf2)
            ia_addmenu(win, IA_NAME_OTYP, 'C', buf2);
    }

    /* d: drop item, works on everything except worn items; those will
       always have a takeoff/remove choice so we don't have to worry
       about the menu maybe being empty when 'd' is suppressed */
    if (!already_worn)
        ia_addmenu(win, IA_DROP_OBJ, 'd', "Drop this item");

    /* e: eat item */
    if (otmp->otyp == TIN && uwep && uwep->otyp == TIN_OPENER)
        ia_addmenu(win, IA_EAT_OBJ, 'e',
                   "Open and eat this tin with your tin opener");
    else if (otmp->otyp == TIN)
        ia_addmenu(win, IA_EAT_OBJ, 'e', "Open and eat this tin");
    else if (is_edible(otmp))
        ia_addmenu(win, IA_EAT_OBJ, 'e', "Eat this item");

    /* E: engrave with item */
    if (otmp->otyp == TOWEL)
        ia_addmenu(win, IA_ENGRAVE_OBJ, 'E', "Wipe the floor with this towel");
    else if (otmp->otyp == MAGIC_MARKER)
        ia_addmenu(win, IA_ENGRAVE_OBJ, 'E', "Scribble graffiti on the floor");
    else if (otmp->oclass == WEAPON_CLASS || otmp->oclass == WAND_CLASS
             || otmp->oclass == GEM_CLASS || otmp->oclass == RING_CLASS)
        ia_addmenu(win, IA_ENGRAVE_OBJ, 'E',
                   "Write on the floor with this object");

    /* i: #adjust inventory letter; gold can't be adjusted unless there
       is some in a slot other than '$' (which shouldn't be possible) */
    if (otmp->oclass != COIN_CLASS || check_invent_gold("item-action"))
        ia_addmenu(win, IA_ADJUST_OBJ, 'i',
                   "Adjust inventory by assigning new letter");
    /* I: #adjust inventory item by splitting its stack  */
    if (otmp->quan > 1L && otmp->oclass != COIN_CLASS)
        ia_addmenu(win, IA_ADJUST_STACK, 'I',
                   "Adjust inventory by splitting this stack");

    /* O: offer sacrifice */
    if (IS_ALTAR(levl[u.ux][u.uy].typ) && !u.uswallow) {
        /* FIXME: this doesn't match #offer's likely candidates, which don't
           include corpses on Astral and don't include amulets off Astral */
        if (otmp->otyp == CORPSE)
            ia_addmenu(win, IA_SACRIFICE, 'O',
                       "Offer this corpse as a sacrifice at this altar");
        else if (otmp->otyp == AMULET_OF_YENDOR
                 || otmp->otyp == FAKE_AMULET_OF_YENDOR)
            ia_addmenu(win, IA_SACRIFICE, 'O',
                       "Offer this amulet as a sacrifice at this altar");
    }

    /* p: pay for unpaid utems */
    if (otmp->unpaid
        /* FIXME: should also handle player owned container (so not
           flagged 'unpaid') holding shop owned items */
        && (mtmp = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE))) != 0
        && inhishop(mtmp))
        ia_addmenu(win, IA_BUY_OBJ, 'p', "Buy this unpaid item");

    /* P: put on accessory */
    if (!already_worn) {
        if (otmp->oclass == RING_CLASS || otmp->otyp == MEAT_RING)
            ia_addmenu(win, IA_WEAR_OBJ, 'P', "Put this ring on");
        else if (otmp->oclass == AMULET_CLASS)
            ia_addmenu(win, IA_WEAR_OBJ, 'P', "Put this amulet on");
        else if (otmp->otyp == TOWEL || otmp->otyp == BLINDFOLD)
            ia_addmenu(win, IA_WEAR_OBJ, 'P',
                       "Use this to blindfold yourself");
        else if (otmp->otyp == LENSES)
            ia_addmenu(win, IA_WEAR_OBJ, 'P', "Put these lenses on");
    }

    /* q: drink item */
    if (otmp->oclass == POTION_CLASS)
        ia_addmenu(win, IA_QUAFF_OBJ, 'q', "Quaff this potion");

    /* Q: quiver throwable item */
    if ((otmp->oclass == GEM_CLASS || otmp->oclass == WEAPON_CLASS)
        && otmp != uquiver)
        ia_addmenu(win, IA_QUIVER_OBJ, 'Q',
                   "Quiver this item for easy throwing");

    /* r: read item */
    if (item_reading_classification(otmp, buf) == IA_READ_OBJ)
        ia_addmenu(win, IA_READ_OBJ, 'r', buf);

    /* R: remove accessory or rub item */
    if (otmp->owornmask & W_ACCESSORY)
        ia_addmenu(win, IA_TAKEOFF_OBJ, 'R', "Remove this accessory");
    if (otmp->otyp == OIL_LAMP || otmp->otyp == MAGIC_LAMP
        || otmp->otyp == BRASS_LANTERN) {
        Sprintf(buf, "Rub this %s", simpleonames(otmp));
        ia_addmenu(win, IA_RUB_OBJ, 'R', buf);
    } else if (otmp->oclass == GEM_CLASS && is_graystone(otmp))
        ia_addmenu(win, IA_RUB_OBJ, 'R', "Rub something on this stone");

    /* t: throw item */
    if (!already_worn) {
        const char *verb = ammo_and_launcher(otmp, uwep) ? "Shoot" : "Throw";

        /*
         * FIXME:
         *  'one of these' should be changed to 'some of these' when there
         *  is the possibility of a multi-shot volley but we don't have
         *  any way to determine that except by actually calculating the
         *  volley count and that could randomly yield 1 here and 2..N
         *  while throwing or vice versa.
         */
        Sprintf(buf, "%s %s", verb,
                (otmp->quan == 1L) ? "this item"
                : (otmp->otyp == GOLD_PIECE) ? "them"
                  : "one of these");
        ia_addmenu(win, IA_THROW_OBJ, 't', buf);
    }

    /* T: take off armor, tip carried container */
    if (otmp->owornmask & W_ARMOR)
        ia_addmenu(win, IA_TAKEOFF_OBJ, 'T', "Take off this armor");
    if ((Is_container(otmp) && (Has_contents(otmp) || !otmp->cknown))
        || (otmp->otyp == HORN_OF_PLENTY && (otmp->spe > 0 || !otmp->known)))
        ia_addmenu(win, IA_TIP_CONTAINER, 'T',
                   "Tip all the contents out of this container");

    /* V: invoke */
    if ((otmp->otyp == FAKE_AMULET_OF_YENDOR && !otmp->known)
        || otmp->oartifact || objects[otmp->otyp].oc_unique
        /* non-artifact crystal balls don't have any unique power but
           the #invoke command lists them as likely candidates */
        || otmp->otyp == CRYSTAL_BALL)
        ia_addmenu(win, IA_INVOKE_OBJ, 'V',
                   "Try to invoke a unique power of this object");

    /* w: wield, hold in hands, works on everything but with different
       advice text; not mentioned for things that are already wielded */
    if (otmp == uwep)
        ;
    else if (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)
             || is_wet_towel(otmp))
        ia_addmenu(win, IA_WIELD_OBJ, 'w', "Wield this as your weapon");
    else if (otmp->otyp == TIN_OPENER)
        ia_addmenu(win, IA_WIELD_OBJ, 'w', "Hold the tin opener to open tins");
    else if (!already_worn)
        /* FIXME: there's no concept of "holding an item" that's any
           different from having it in inventory; 'w' means wield as weapon */
        ia_addmenu(win, IA_WIELD_OBJ, 'w', "Hold this item in your hands");

    /* W: wear armor */
    if (!already_worn) {
        if (otmp->oclass == ARMOR_CLASS)
            ia_addmenu(win, IA_WEAR_OBJ, 'W', "Wear this armor");
    }

    /* x: Swap main and readied weapon */
    if (otmp == uwep && uswapwep)
        ia_addmenu(win, IA_SWAPWEAPON, 'x',
                   "Swap this with your alternate weapon");
    else if (otmp == uwep)
        ia_addmenu(win, IA_SWAPWEAPON, 'x',
                   "Ready this as an alternate weapon");
    else if (otmp == uswapwep)
        ia_addmenu(win, IA_SWAPWEAPON, 'x', "Swap this with your main weapon");

    /* z: Zap wand */
    if (otmp->oclass == WAND_CLASS)
        ia_addmenu(win, IA_ZAP_OBJ, 'z', "Zap this wand to release its magic");

    Sprintf(buf, "Do what with %s?", the(cxname(otmp)));
    end_menu(win, buf);

    n = select_menu(win, PICK_ONE, &selected);

    if (n > 0) {
        act = selected[0].item.a_int;
        free((genericptr_t) selected);

        switch (act) {
        default:
            impossible("Unknown item action");
        case IA_NONE:
            break;
        case IA_UNWIELD:
            cmdq_add_ec(CQ_CANNED, (otmp == uwep) ? dowield
                        : (otmp == uswapwep) ? remarm_swapwep
                          : (otmp == uquiver) ? dowieldquiver
                            : donull); /* can't happen */
            cmdq_add_key(CQ_CANNED, '-');
            break;
        case IA_APPLY_OBJ:
            cmdq_add_ec(CQ_CANNED, doapply);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_DIP_OBJ:
            /* #altdip instead of normal #dip - takes potion to dip into
               first (the inventory item instigating this) and item to
               be dipped second, also ignores floor features such as
               fountain/sink so we don't need to force m-prefix here */
            cmdq_add_ec(CQ_CANNED, dip_into);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_NAME_OBJ:
        case IA_NAME_OTYP:
            cmdq_add_ec(CQ_CANNED, docallcmd);
            cmdq_add_key(CQ_CANNED, (act == IA_NAME_OBJ) ? 'i' : 'o');
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_DROP_OBJ:
            cmdq_add_ec(CQ_CANNED, dodrop);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_EAT_OBJ:
            /* start with m-prefix; for #eat, it means ignore floor food
               if present and eat food from invent */
            cmdq_add_ec(CQ_CANNED, do_reqmenu);
            cmdq_add_ec(CQ_CANNED, doeat);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_ENGRAVE_OBJ:
            cmdq_add_ec(CQ_CANNED, doengrave);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_ADJUST_OBJ:
            cmdq_add_ec(CQ_CANNED, doorganize); /* #adjust */
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_ADJUST_STACK:
            cmdq_add_ec(CQ_CANNED, adjust_split); /* #altadjust */
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_SACRIFICE:
            cmdq_add_ec(CQ_CANNED, dosacrifice);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_BUY_OBJ:
            cmdq_add_ec(CQ_CANNED, dopay);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_QUAFF_OBJ:
            /* start with m-prefix; for #quaff, it means ignore fountain
               or sink if present and drink a potion from invent */
            cmdq_add_ec(CQ_CANNED, do_reqmenu);
            cmdq_add_ec(CQ_CANNED, dodrink);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_QUIVER_OBJ:
            cmdq_add_ec(CQ_CANNED, dowieldquiver);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_READ_OBJ:
            cmdq_add_ec(CQ_CANNED, doread);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_RUB_OBJ:
            cmdq_add_ec(CQ_CANNED, dorub);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_THROW_OBJ:
            cmdq_add_ec(CQ_CANNED, dothrow);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_TAKEOFF_OBJ:
            cmdq_add_ec(CQ_CANNED, dotakeoff);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_TIP_CONTAINER:
            cmdq_add_ec(CQ_CANNED, dotip);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_INVOKE_OBJ:
            cmdq_add_ec(CQ_CANNED, doinvoke);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_WIELD_OBJ:
            cmdq_add_ec(CQ_CANNED, dowield);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_WEAR_OBJ:
            cmdq_add_ec(CQ_CANNED, dowear);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        case IA_SWAPWEAPON:
            cmdq_add_ec(CQ_CANNED, doswapweapon);
            break;
        case IA_ZAP_OBJ:
            cmdq_add_ec(CQ_CANNED, dozap);
            cmdq_add_key(CQ_CANNED, otmp->invlet);
            break;
        }
    }
    destroy_nhwindow(win);

    /* finish the 'i' command:  no time elapses and cancelling without
       selecting an action doesn't matter */
    return ECMD_OK;
}


/* the #inventory command */
int
ddoinv(void)
{
    struct obj *otmp;
    char c = display_inventory((char *) 0, TRUE);

    if (c && c != '\033') {
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (otmp->invlet == c)
                return itemactions(otmp);
    }
    return ECMD_OK;
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
static struct obj *
find_unpaid(struct obj *list, struct obj **last_found)
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

void
free_pickinv_cache(void)
{
    if (g.cached_pickinv_win != WIN_ERR) {
        destroy_nhwindow(g.cached_pickinv_win);
        g.cached_pickinv_win = WIN_ERR;
    }
}

/*
 * Internal function used by display_inventory and getobj that can display
 * inventory and return a count as well as a letter.
 */
static char
display_pickinv(
    const char *lets,        /* non-compacted list of invlet values */
    const char *xtra_choice, /* non-object "bare hands" or "fingers" */
    const char *query,       /* optional; prompt string for menu */
    boolean want_reply,      /* True: select an item, False: just display */
    long *out_cnt) /* optional; count player entered when selecting an item */
{
    static const char /* potential entries for perm_invent window */
        not_carrying_anything[] = "Not carrying anything",
        not_using_anything[] = "Not using any items",
        only_carrying_gold[] = "Only carrying gold";
    struct obj *otmp, wizid_fakeobj;
    char ilet, ret, *formattedobj;
    const char *invlet = flags.inv_order;
    int n, classcount;
    winid win; /* windows being used */
    anything any;
    menu_item *selected;
    unsigned sortflags;
    Loot *sortedinvent, *srtinv;
    boolean wizid = (wizard && iflags.override_ID), gotsomething = FALSE;
    int clr = 0, menu_behavior = MENU_BEHAVE_STANDARD;
    boolean show_gold = TRUE, sparse = FALSE, inuse_only = FALSE,
            skipped_gold = FALSE, skipped_noninuse = FALSE,
            doing_perm_invent = FALSE, save_flags_sortpack = flags.sortpack;

    if (lets && !*lets)
        lets = 0; /* simplify tests: (lets) instead of (lets && *lets) */

#ifdef DUMPLOG
    if (iflags.in_dumplog) {
        win = 0; /* passed to dump_putstr() which ignores it... */
    } else
#endif
    if (lets || xtra_choice || wizid || want_reply
#ifdef TTY_PERM_INVENT
        || !g.in_sync_perminvent
#endif
        || WIN_INVEN == WIN_ERR) {
        /* partial inventory in perm_invent setting; don't operate on
           full inventory window, use an alternate one instead; create
           the first time needed and keep it for re-use as needed later */
        if (g.cached_pickinv_win == WIN_ERR)
            g.cached_pickinv_win = create_nhwindow(NHW_MENU);
        win = g.cached_pickinv_win;
    } else {
        win = WIN_INVEN;
        menu_behavior = MENU_BEHAVE_PERMINV;
        prepare_perminvent(win);
        show_gold = ((wri_info.fromcore.invmode & InvShowGold) != 0);
        sparse = ((wri_info.fromcore.invmode & InvSparse) != 0);
        inuse_only = ((wri_info.fromcore.invmode & InvInUse) != 0);
        doing_perm_invent = TRUE;
        nhUse(sparse);
    }
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
    n = (iflags.perm_invent && !lets && !want_reply) ? 2
        : lets ? (int) strlen(lets)
               : !g.invent ? 0 : !g.invent->nobj ? 1 : 2;
    /* for xtra_choice, there's another 'item' not included in initial 'n';
       for !lets (full g.invent) and for override_ID (wizard mode identify),
       skip message_menu handling of single item even if item count was 1 */
    if (xtra_choice || (n == 1 && (!lets || wizid)))
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
            for (otmp = g.invent; otmp; otmp = otmp->nobj)
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

    sortflags = (flags.sortloot == 'f') ? SORTLOOT_LOOT : SORTLOOT_INVLET;
    if (flags.sortpack)
        sortflags |= SORTLOOT_PACK;
#ifdef TTY_PERM_INVENT
    if (doing_perm_invent && WINDOWPORT(tty)) {
        sortflags = SORTLOOT_INVLET;
        save_flags_sortpack = flags.sortpack;
        flags.sortpack = FALSE;
    }
#else
    nhUse(save_flags_sortpack);
#endif
    sortedinvent = sortloot(&g.invent, sortflags, FALSE,
                            (boolean (*)(OBJ_P)) 0);
    start_menu(win, menu_behavior);
    any = cg.zeroany;
    if (wizid) {
        int unid_cnt;
        char prompt[QBUFSZ];

        unid_cnt = count_unidentified(g.invent);
        Sprintf(prompt, "Debug Identify"); /* 'title' rather than 'prompt' */
        if (unid_cnt)
            Sprintf(eos(prompt),
                    " -- unidentified or partially identified item%s",
                    plur(unid_cnt));
        add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, prompt,
                 MENU_ITEMFLAGS_NONE);
        if (!unid_cnt) {
            add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
                     "(all items are permanently identified already)",
                     MENU_ITEMFLAGS_NONE);
            gotsomething = TRUE;
        } else {
            any.a_obj = &wizid_fakeobj;
            Sprintf(prompt, "select %s to permanently identify",
                    (unid_cnt == 1) ? "it": "any or all of them");
            /* wiz_identify stuffed the wiz_identify command character (^I)
               into iflags.override_ID for our use as an accelerator;
               it could be ambiguous if player has assigned a letter to
               the #wizidentify command, so include it as a group accelator
               but use '_' as the primary selector */
            if (unid_cnt > 1)
                Sprintf(eos(prompt), " (%s for all)",
                        visctrl(iflags.override_ID));
            add_menu(win, &nul_glyphinfo, &any, '_', iflags.override_ID,
                     ATR_NONE, clr, prompt, MENU_ITEMFLAGS_SKIPINVERT);
            gotsomething = TRUE;
        }
   } else if (xtra_choice) {
        /* wizard override ID and xtra_choice are mutually exclusive */
        if (flags.sortpack)
            add_menu(win, &nul_glyphinfo, &any, 0, 0,
                     iflags.menu_headings, clr,
                    "Miscellaneous", MENU_ITEMFLAGS_NONE);
        any.a_char = HANDS_SYM; /* '-' */
        add_menu(win, &nul_glyphinfo, &any, HANDS_SYM, 0, ATR_NONE,
                 clr, xtra_choice, MENU_ITEMFLAGS_NONE);
        gotsomething = TRUE;
    }

 nextclass:
    classcount = 0;
    for (srtinv = sortedinvent; (otmp = srtinv->obj) != 0; ++srtinv) {
        int tmpglyph;
        glyph_info tmpglyphinfo = nul_glyphinfo;

        if (lets && !strchr(lets, otmp->invlet))
            continue;
        if (!flags.sortpack || otmp->oclass == *invlet) {
            if (wizid && !not_fully_identified(otmp))
                continue;
            if (doing_perm_invent) {
                /* when showing equipment in use, gold shouldn't be excluded
                   just because !show_gold is set; it might be quivered;
                   tool_being_used() matches lit lamps/candles and active
                   leashes, neither of which set owornmask */
                if (inuse_only) {
                    if (!otmp->owornmask && !tool_being_used(otmp)) {
                        skipped_noninuse = TRUE;
                        continue;
                    }
                } else if (otmp->invlet == GOLD_SYM && !show_gold) {
                    skipped_gold = TRUE;
                    continue;
                }
            }
            any = cg.zeroany; /* all bits zero */
            ilet = otmp->invlet;
            if (flags.sortpack && !classcount) {
                add_menu(win, &nul_glyphinfo, &any, 0, 0,
                         iflags.menu_headings, clr,
                         let_to_name(*invlet, FALSE,
                                     (want_reply && iflags.menu_head_objsym)),
                         MENU_ITEMFLAGS_NONE);
                classcount++;
            }
            if (wizid)
                any.a_obj = otmp;
            else
                any.a_char = ilet;
            tmpglyph = obj_to_glyph(otmp, rn2_on_display_rng);
            map_glyphinfo(0, 0, tmpglyph, 0U, &tmpglyphinfo);
            formattedobj = doname(otmp);
            add_menu(win, &tmpglyphinfo, &any, ilet,
                     wizid ? def_oc_syms[(int) otmp->oclass].sym : 0,
                     ATR_NONE, clr, formattedobj, MENU_ITEMFLAGS_NONE);
            /* doname() uses a static pool of obuf[] output buffers and
               we don't want inventory display to overwrite all of them,
               so when we've used one we release it for re-use */
            maybereleaseobuf(formattedobj);
            gotsomething = TRUE;
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
    /* default for force_invmenu is a list of likely candidates;
       add '*' for 'show all' as an extra choice unless list already
       includes everything; won't work via keyboard if current menu
       uses '*' as group accelerator for gems but might work via mouse */
    if (iflags.force_invmenu && lets && want_reply
        && (int) strlen(lets) < inv_cnt(TRUE)) {
        any = cg.zeroany;
        add_menu(win, &nul_glyphinfo, &any, 0, 0,
                 iflags.menu_headings, clr, "Special", MENU_ITEMFLAGS_NONE);
        any.a_char = '*';
        add_menu(win, &nul_glyphinfo, &any, '*', 0, ATR_NONE, clr,
                 "(list everything)", MENU_ITEMFLAGS_NONE);
        gotsomething = TRUE;
    }
    unsortloot(&sortedinvent);
    /* for permanent inventory where nothing has been listed (because
       there isn't anything applicable to list; the n==0 case above
       gets skipped for perm_invent), put something into the menu */
    if (iflags.perm_invent && !lets && !gotsomething) {
        any = cg.zeroany;
        add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
                 (inuse_only && skipped_noninuse) ? not_using_anything
                 : (!show_gold && skipped_gold) ? only_carrying_gold
                   : not_carrying_anything,
                 MENU_ITEMFLAGS_NONE);
        want_reply = FALSE;
    }
#ifdef TTY_PERM_INVENT
    if (doing_perm_invent && WINDOWPORT(tty))
        flags.sortpack = save_flags_sortpack;
#endif
    end_menu(win, (query && *query) ? query : (char *) 0);

    n = select_menu(win,
                    wizid ? PICK_ANY : want_reply ? PICK_ONE : PICK_NONE,
                    &selected);
    if (n > 0) {
        if (wizid) {
            boolean all_id = FALSE;
            int i;

            /* identifying items will update perm_invent, calling this
               routine recursively, and we don't want the nested call
               to filter on unID'd items */
            iflags.override_ID = 0;
            ret = '\0';
            for (i = 0; i < n; ++i) {
                otmp = selected[i].item.a_obj;
                if (otmp == &wizid_fakeobj) {
                    identify_pack(0, FALSE);
                    /* identify_pack() performs update_inventory() */
                    all_id = TRUE;
                    break;
                } else {
                    if (not_fully_identified(otmp))
                        (void) identify(otmp);
                    /* identify() does not perform update_inventory() */
                }
            }
            if (!all_id)
                update_inventory();
        } else {
            ret = selected[0].item.a_char;
            if (out_cnt)
                *out_cnt = selected[0].count;
        }
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
display_inventory(const char *lets, boolean want_reply)
{
    struct _cmd_queue *cmdq = cmdq_pop();

    if (cmdq) {
        if (cmdq->typ == CMDQ_KEY) {
            struct obj *otmp;

            for (otmp = g.invent; otmp; otmp = otmp->nobj)
                if (otmp->invlet == cmdq->key
                    && (!lets || !*lets
                        || strchr(lets, def_oc_syms[(int)otmp->oclass].sym))) {
                    free(cmdq);
                    return otmp->invlet;
                }
        }

        /* cmdq not a key, or did not find the object, abort */
        free(cmdq);
        cmdq_clear(CQ_CANNED);
        return '\0';
    }
    return display_pickinv(lets, (char *) 0, (char *) 0,
                           want_reply, (long *) 0);
}

/*
 * Show what is current using inventory letters.
 *
 */
static char
display_used_invlets(char avoidlet)
{
    struct obj *otmp;
    char ilet, ret = 0;
    char *invlet = flags.inv_order;
    int n, classcount, invdone = 0, tmpglyph;
    glyph_info tmpglyphinfo = nul_glyphinfo;
    winid win;
    anything any;
    menu_item *selected;
    int clr = 0;

    if (g.invent) {
        win = create_nhwindow(NHW_MENU);
        start_menu(win, MENU_BEHAVE_STANDARD);
        while (!invdone) {
            any = cg.zeroany; /* set all bits to zero */
            classcount = 0;
            for (otmp = g.invent; otmp; otmp = otmp->nobj) {
                ilet = otmp->invlet;
                if (ilet == avoidlet)
                    continue;
                if (!flags.sortpack || otmp->oclass == *invlet) {
                    if (flags.sortpack && !classcount) {
                        any = cg.zeroany; /* zero */
                        add_menu(win, &nul_glyphinfo, &any, 0, 0,
                                 iflags.menu_headings, clr,
                                 let_to_name(*invlet, FALSE, FALSE),
                                 MENU_ITEMFLAGS_NONE);
                        classcount++;
                    }
                    any.a_char = ilet;
                    tmpglyph = obj_to_glyph(otmp, rn2_on_display_rng);
                    map_glyphinfo(0, 0, tmpglyph, 0U, &tmpglyphinfo);
                    add_menu(win, &tmpglyphinfo, &any, ilet, 0,
                             ATR_NONE, clr, doname(otmp), MENU_ITEMFLAGS_NONE);
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
count_unpaid(struct obj *list)
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
count_buc(struct obj *list, int type, boolean (*filterfunc)(OBJ_P))
{
    int count = 0;

    for (; list; list = list->nobj) {
        /* priests always know bless/curse state */
        if (Role_if(PM_CLERIC))
            list->bknown = (list->oclass != COIN_CLASS);
        /* some actions exclude some or most items */
        if (filterfunc && !(*filterfunc)(list))
            continue;

        /* coins are either uncursed or unknown based upon option setting */
        if (list->oclass == COIN_CLASS) {
            if (type == (flags.goldX ? BUC_UNKNOWN : BUC_UNCURSED))
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
tally_BUCX(
    struct obj *list,
    boolean by_nexthere,
    int *bcp, int *ucp, int *ccp, int *xcp, int *ocp, int *jcp)
{
    /* Future extensions:
     *  Skip current_container when list is invent, uchain when
     *  first object of list is located on the floor.  'ocp' will then
     *  have a function again (it was a counter for having skipped gold,
     *  but that's not skipped anymore).
     */
    *bcp = *ucp = *ccp = *xcp = *ocp = *jcp = 0;
    for ( ; list; list = (by_nexthere ? list->nexthere : list->nobj)) {
        /* priests always know bless/curse state */
        if (Role_if(PM_CLERIC))
            list->bknown = (list->oclass != COIN_CLASS);
        if (list->pickup_prev)
            ++(*jcp);
        /* coins are either uncursed or unknown based upon option setting */
        if (list->oclass == COIN_CLASS) {
            if (flags.goldX)
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

/* count everything inside a container, or just shop-owned items inside */
long
count_contents(
    struct obj *container,
    boolean nested,     /* include contents of any nested containers */
    boolean quantity,   /* count all vs count separate stacks        */
    boolean everything, /* all objects vs only unpaid objects        */
    boolean newdrop)    /* on floor, but hero-owned items haven't
                         * been marked no_charge yet and shop-owned
                         * items are still marked unpaid -- used
                         * when asking the player whether to sell    */
{
    struct obj *otmp, *topc;
    boolean shoppy = FALSE;
    long count = 0L;

    if (!everything && !newdrop) {
        coordxy x, y;

        for (topc = container; topc->where == OBJ_CONTAINED;
             topc = topc->ocontainer)
            continue;
        if (topc->where == OBJ_FLOOR && get_obj_location(topc, &x, &y, 0))
            shoppy = costly_spot(x, y);
    }
    for (otmp = container->cobj; otmp; otmp = otmp->nobj) {
        if (nested && Has_contents(otmp))
            count += count_contents(otmp, nested, quantity, everything,
                                    newdrop);
        if (everything || otmp->unpaid || (shoppy && !otmp->no_charge))
            count += quantity ? otmp->quan : 1L;
    }
    return count;
}

static void
dounpaid(void)
{
    winid win;
    struct obj *otmp, *marker, *contnr;
    char ilet;
    char *invlet = flags.inv_order;
    int classcount, count, num_so_far;
    long cost, totcost;

    count = count_unpaid(g.invent);
    otmp = marker = contnr = (struct obj *) 0;

    if (count == 1) {
        otmp = find_unpaid(g.invent, &marker);
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
        for (otmp = g.invent; otmp; otmp = otmp->nobj) {
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
        for (otmp = g.invent; otmp; otmp = otmp->nobj) {
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


static boolean
this_type_only(struct obj *obj)
{
    boolean res = (obj->oclass == g.this_type);

    if (g.this_type == 'P') {
        res = obj->pickup_prev;
    } else if (obj->oclass == COIN_CLASS) {
        /* if filtering by bless/curse state, gold is classified as
           either unknown or uncursed based on user option setting */
        if (g.this_type && strchr("BUCX", g.this_type))
            res = (g.this_type == (flags.goldX ? 'X' : 'U'));
    } else {
        switch (g.this_type) {
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

/* the #inventtype command */
int
dotypeinv(void)
{
    static const char
        prompt[] = "What type of object do you want an inventory of?";
    char c = '\0';
    int n, i = 0;
    char *extra_types, types[BUFSZ], title[QBUFSZ];
    const char *before = "", *after = "";
    int class_count, oclass, unpaid_count, itemcount;
    int bcnt, ccnt, ucnt, xcnt, ocnt, jcnt;
    boolean billx = *u.ushops && doinvbill(0);
    menu_item *pick_list;
    boolean traditional = TRUE;

    g.this_type = 0;
    g.this_title = NULL;
    if (!g.invent && !billx) {
        You("aren't carrying anything.");
        goto doI_done;
    }
    title[0] = '\0';
    unpaid_count = count_unpaid(g.invent);
    tally_BUCX(g.invent, FALSE, &bcnt, &ucnt, &ccnt, &xcnt, &ocnt, &jcnt);

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
            if (jcnt)
                i |= JUSTPICKED;
            i |= INCLUDE_VENOM;
            n = query_category(prompt, g.invent, i, &pick_list, PICK_ONE);
            if (!n)
                goto doI_done;
            g.this_type = c = pick_list[0].item.a_int;
            free((genericptr_t) pick_list);
        }
    }
    if (traditional) {
        /* collect list of classes of objects carried, for use as a prompt */
        types[0] = 0;
        class_count = collect_obj_classes(types, g.invent, FALSE,
                                          (boolean (*)(OBJ_P)) 0,
                                          &itemcount);
        if (unpaid_count || billx || (bcnt + ccnt + ucnt + xcnt) != 0 || jcnt)
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
        if (jcnt)
            types[class_count++] = 'P';
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
        if (!jcnt)
            *extra_types++ = 'P';
        *extra_types = '\0'; /* for strchr() */
        for (i = 0; i < MAXOCLASSES; i++)
            if (!strchr(types, def_oc_syms[i].sym)) {
                *extra_types++ = def_oc_syms[i].sym;
                *extra_types = '\0';
            }

        if (class_count > 1) {
            c = yn_function(prompt, types, '\0', TRUE);
            if (c == '\0') {
                clear_nhwindow(WIN_MESSAGE);
                goto doI_done;
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
        goto doI_done;
    }
    if (c == 'u' || (c == 'U' && unpaid_count && !ucnt)) {
        if (unpaid_count)
            dounpaid();
        else
            You("are not carrying any unpaid objects.");
        goto doI_done;
    }

    if (strchr("BUCXP", c))
        oclass = c; /* not a class but understood by this_type_only() */
    else
        oclass = def_char_to_objclass(c); /* change to object class */
#if 0
    /* this used to be done for the 'if traditional' case but not for the
       menu case; also unlike '$', 'I$' explicitly asks about inventory,
       so we no longer handle coin class differently from other classes */
    if (oclass == COIN_CLASS) {
        return doprgold();
    }
#endif
    /* these are used for traditional when not applicable and also for
       constructing a title to be used by query_objlist() */
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
    case 'P':
        after = " that were just picked up";
        break;
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

    if (traditional) {
        if (strchr(types, c) > strchr(types, '\033')) {
            You("have no %sobjects%s.", before, after);
            goto doI_done;
        }
        g.this_type = oclass; /* extra input for this_type_only() */
    }
    if (strchr("BUCXP", c)) {
        /* the before and after phrases for "you have no..." can both be
           treated as mutually-exclusive suffices when creating a title */
        Sprintf(title, "Items %s", (before && *before) ? before : after);
        /* get rid of trailing space from 'before' and double-space from
           'after's leading space */
        (void) mungspaces(title);
        Strcat(title, ":"); /* after removing unwanted trailing space */
        g.this_title = title;
    }

    if (query_objlist((char *) 0, &g.invent,
                      ((flags.invlet_constant ? USE_INVLET : 0)
                       | INVORDER_SORT | INCLUDE_VENOM),
                      &pick_list, PICK_ONE, this_type_only) > 0) {
        struct obj *otmp = pick_list[0].item.a_obj;

        free((genericptr_t) pick_list);
        (void) itemactions(otmp); /* always returns ECMD_OK */
    }

 doI_done:
    g.this_type = 0;
    g.this_title = NULL;
    return ECMD_OK;
}

/* return a string describing the dungeon feature at <x,y> if there
   is one worth mentioning at that location; otherwise null */
const char *
dfeature_at(coordxy x, coordxy y, char *buf)
{
    struct rm *lev = &levl[x][y];
    int ltyp = lev->typ, cmap = -1;
    const char *dfeature = 0;
    static char altbuf[BUFSZ];
    stairway *stway = stairway_at(x, y);

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
                (lev->altarmask & AM_SANCTUM) ? "high " : "",
                a_gname(),
                align_str(Amask2align(lev->altarmask & ~AM_SHRINE)));
        dfeature = altbuf;
    } else if (stway) {
        dfeature = stairs_description(stway, altbuf, TRUE);
    } else if (ltyp == DRAWBRIDGE_DOWN)
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
look_here(
    int obj_cnt, /* obj_cnt > 0 implies that autopickup is in progress */
    unsigned lookhere_flags)
{
    struct obj *otmp;
    struct trap *trap;
    const char *verb = Blind ? "feel" : "see";
    const char *dfeature = (char *) 0;
    char fbuf[BUFSZ], fbuf2[BUFSZ];
    winid tmpwin;
    boolean skip_objects, felt_cockatrice = FALSE,
            picked_some = (lookhere_flags & LOOKHERE_PICKED_SOME) != 0,
            /* skip 'dfeature' if caller used describe_decor() to show it */
            skip_dfeature = (lookhere_flags & LOOKHERE_SKIP_DFEATURE) != 0;

    /* default pile_limit is 5; a value of 0 means "never skip"
       (and 1 effectively forces "always skip") */
    skip_objects = (flags.pile_limit > 0 && obj_cnt >= flags.pile_limit);
    if (u.uswallow) {
        struct monst *mtmp = u.ustuck;

        /*
         * FIXME?
         *  Engulfer's inventory can include worn items (specific case is
         *  Juiblex being created with an amulet as random defensive item)
         *  which will be flagged as "(being worn)".  This code includes
         *  such a worn item under the header "Contents of <mon>'s stomach",
         *  a nifty trick for how/where to wear stuff.  The situation is
         *  rare enough to turn a blind eye.
         *
         *  3.6.3:  Pickup has been changed to decline to pick up a worn
         *  item from inside an engulfer, but if player tries, it just
         *  says "you can't" without giving a reason why (which would be
         *  something along the lines of "because it's worn on the outside
         *  so is unreachable from in here...").
         */
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
        return (!!Blind ? ECMD_TIME : ECMD_OK);
    }
    if (!skip_objects && (trap = t_at(u.ux, u.uy)) && trap->tseen)
        There("is %s here.", an(trapname(trap->ttyp, FALSE)));

    otmp = g.level.objects[u.ux][u.uy];
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
        trap = t_at(u.ux, u.uy);
        if (!can_reach_floor(trap && is_pit(trap->ttyp))) {
            pline("But you can't reach it!");
            return ECMD_OK;
        }
    }

    if (dfeature && !skip_dfeature)
        Sprintf(fbuf, "There is %s here.", an(dfeature));

    if (!otmp || is_lava(u.ux, u.uy)
        || (is_pool(u.ux, u.uy) && !Underwater)) {
        if (dfeature && !skip_dfeature)
            pline1(fbuf);
        read_engr_at(u.ux, u.uy); /* Eric Backus */
        if (!skip_objects && (Blind || !dfeature))
            You("%s no objects here.", verb);
        return (!!Blind ? ECMD_TIME : ECMD_OK);
    }
    /* we know there is something here */

    if (skip_objects) {
        if (dfeature && !skip_dfeature)
            pline1(fbuf);
        read_engr_at(u.ux, u.uy); /* Eric Backus */
        if (obj_cnt == 1 && otmp->quan == 1L)
            There("is %s object here.", picked_some ? "another" : "an");
        else
            There("are %s%s objects here.",
                  (obj_cnt == 2) ? "two"
                  : (obj_cnt < 5) ? "a few"
                  : (obj_cnt < 10) ? "several"
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
                      poly_when_stoned(g.youmonst.data)
                          ? ""
                          : ", unfortunately");
                feel_cockatrice(otmp, FALSE);
                break;
            }
    } else if (!otmp->nexthere) {
        /* only one object */
        if (dfeature && !skip_dfeature)
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
        if (dfeature && !skip_dfeature) {
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
    return (!!Blind ? ECMD_TIME : ECMD_OK);
}

/* #look command - explicitly look at what is here, including all objects */
int
dolook(void)
{
    int res;

    /* don't let
       MSGTYPE={norep,noshow} "You see here"
       interfere with feedback from the look-here command */
    hide_unhide_msgtypes(TRUE, MSGTYP_MASK_REP_SHOW);
    res = look_here(0, LOOKHERE_NOFLAGS);
    /* restore normal msgtype handling */
    hide_unhide_msgtypes(FALSE, MSGTYP_MASK_REP_SHOW);
    return res;
}

boolean
will_feel_cockatrice(struct obj *otmp, boolean force_touch)
{
    if ((Blind || force_touch) && !uarmg && !Stone_resistance
        && (otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm])))
        return TRUE;
    return FALSE;
}

void
feel_cockatrice(struct obj *otmp, boolean force_touch)
{
    char kbuf[BUFSZ];

    if (will_feel_cockatrice(otmp, force_touch)) {
        /* "the <cockatrice> corpse" */
        Strcpy(kbuf, corpse_xname(otmp, (const char *) 0, CXN_PFX_THE));

        if (poly_when_stoned(g.youmonst.data))
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

/* 'obj' is being placed on the floor; if it can merge with something that
   is already there, combine them and discard obj as a separate object */
void
stackobj(struct obj *obj)
{
    struct obj *otmp;

    for (otmp = g.level.objects[obj->ox][obj->oy]; otmp; otmp = otmp->nexthere)
        if (otmp != obj && merged(&obj, &otmp))
            break;
    return;
}

/* returns TRUE if obj & otmp can be merged; used in invent.c and mkobj.c */
boolean
mergable(
    register struct obj *otmp, /* potential 'into' stack */
    register struct obj *obj)  /* 'combine' stack */
{
    size_t objnamelth = 0, otmpnamelth = 0;

    /* fail if already the same object, if different types, if either is
       explicitly marked to prevent merge, or if not mergable in general */
    if (obj == otmp || obj->otyp != otmp->otyp
        || obj->nomerge || otmp->nomerge || !objects[obj->otyp].oc_merge)
        return FALSE;

    /* coins of the same kind will always merge */
    if (obj->oclass == COIN_CLASS)
        return TRUE;

    if (obj->cursed != otmp->cursed || obj->blessed != otmp->blessed)
        return FALSE;
#if 0   /* don't require 'bypass' to match; that results in items dropped
         * via 'D' not stacking with compatible items already on the floor;
         * caller who wants that behavior should use 'nomerge' instead */
    if (obj->bypass != otmp->bypass)
        return FALSE;
#endif

    if (obj->globby)
        return TRUE;
    /* Checks beyond this point either aren't applicable to globs
     * or don't inhibit their merger.
     */

    if (obj->unpaid != otmp->unpaid || obj->spe != otmp->spe
        || obj->no_charge != otmp->no_charge || obj->obroken != otmp->obroken
        || obj->otrapped != otmp->otrapped || obj->lamplit != otmp->lamplit)
        return FALSE;

    if (obj->oclass == FOOD_CLASS
        && (obj->oeaten != otmp->oeaten || obj->orotten != otmp->orotten))
        return FALSE;

    if (obj->dknown != otmp->dknown
        || (obj->bknown != otmp->bknown && !Role_if(PM_CLERIC))
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

    /* some additional information is always incompatible */
    if (has_omonst(obj) || has_omid(obj)
        || has_omonst(otmp) || has_omid(otmp))
        return FALSE;

    /* if they have names, make sure they're the same */
    objnamelth = strlen(safe_oname(obj));
    otmpnamelth = strlen(safe_oname(otmp));
    if ((objnamelth != otmpnamelth
         && ((objnamelth && otmpnamelth) || obj->otyp == CORPSE))
        || (objnamelth && otmpnamelth
            && strncmp(ONAME(obj), ONAME(otmp), objnamelth)))
        return FALSE;

    /* if one has an attached mail command, other must have same command */
    if (!has_omailcmd(obj) ? has_omailcmd(otmp)
        : (!has_omailcmd(otmp) || strcmp(OMAILCMD(obj), OMAILCMD(otmp)) != 0))
        return FALSE;

#ifdef MAIL_STRUCTURES
    if (obj->otyp == SCR_MAIL
        /* wished or bones mail and hand written stamped scrolls
           each have two flavors; spe keeps them separate from each
           other but we want to keep their flavors separate too */
        && obj->spe > 0 && (obj->o_id % 2) != (otmp->o_id % 2))
        return FALSE;
#endif

    /* should be moot since matching artifacts wouldn't be unique */
    if (obj->oartifact != otmp->oartifact)
        return FALSE;

    if (obj->known == otmp->known || !objects[otmp->otyp].oc_uses_known) {
        return (boolean) objects[obj->otyp].oc_merge;
    } else
        return FALSE;
}

/* the #showgold command */
int
doprgold(void)
{
    /* Command takes containers into account. */
    long umoney = money_cnt(g.invent);

    /* Only list the money you know about.  Guards and shopkeepers
       can somehow tell if there is any gold anywhere on your
       person, but you have no such preternatural gold-sense. */
    long hmoney = hidden_gold(FALSE);

    if (Verbose(1, doprgold)) {
        char buf[BUFSZ];

        if (!umoney) {
            Strcpy(buf, "Your wallet is empty");
        } else {
            Sprintf(buf, "Your wallet contains %ld %s",
                    umoney, currency(umoney));
        }
        if (hmoney) {
            Sprintf(eos(buf),
                    ", %s you have %ld %s stashed away in your pack",
                    umoney ? "and" : "but", hmoney,
                    umoney ? "more" : currency(hmoney));
        }
        pline("%s.", buf);
    } else {
        long total = umoney + hmoney;
        if (total)
            You("are carrying a total of %ld %s.", total, currency(total));
        else
            You("have no money.");
    }
    shopper_financial_report();
    return ECMD_OK;
}

/* the #seeweapon command */
int
doprwep(void)
{
    if (!uwep) {
        You("are %s.", empty_handed());
    } else {
        prinv((char *) 0, uwep, 0L);
        if (u.twoweap)
            prinv((char *) 0, uswapwep, 0L);
    }
    return ECMD_OK;
}

/* caller is responsible for checking !wearing_armor() */
static void
noarmor(boolean report_uskin)
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

/* the #seearmor command */
int
doprarm(void)
{
    char lets[8];
    int ct = 0;
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
    return ECMD_OK;
}

/* the #seerings command */
int
doprring(void)
{
    if (!uleft && !uright) {
        You("are not wearing any rings.");
    } else {
        char lets[3];
        int ct = 0;

        if (uleft)
            lets[ct++] = obj_to_let(uleft);
        if (uright)
            lets[ct++] = obj_to_let(uright);
        lets[ct] = 0;
        (void) display_inventory(lets, FALSE);
    }
    return ECMD_OK;
}

/* the #seeamulet command */
int
dopramulet(void)
{
    if (!uamul)
        You("are not wearing an amulet.");
    else
        prinv((char *) 0, uamul, 0L);
    return ECMD_OK;
}

/* is 'obj' a tool that's in use?  can't simply check obj->owornmask */
static boolean
tool_being_used(struct obj *obj)
{
    if ((obj->owornmask & (W_TOOL | W_SADDLE)) != 0L)
        return TRUE;
    if (obj->oclass != TOOL_CLASS)
        return FALSE;
    return (boolean) (obj == uwep || obj->lamplit
                      || (obj->otyp == LEASH && obj->leashmon));
}

/* the #seetools command */
int
doprtool(void)
{
    struct obj *otmp;
    int ct = 0;
    char lets[52 + 1];

    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (tool_being_used(otmp)) {
            /* we could be carrying more than 52 items; theoretically they
               might all be lit candles so avoid potential lets[] overflow */
            if (ct >= (int) sizeof lets - 1)
                break;
            lets[ct++] = obj_to_let(otmp);
        }
    lets[ct] = '\0';
    if (!ct)
        You("are not using any tools.");
    else
        (void) display_inventory(lets, FALSE);
    return ECMD_OK;
}

/* the #seeall command; combines the ')' + '[' + '=' + '"' + '(' commands;
   show inventory of all currently wielded, worn, or used objects */
int
doprinuse(void)
{
    struct obj *otmp;
    int ct = 0;
    char lets[52 + 1];

    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (is_worn(otmp) || tool_being_used(otmp)) {
            /* we could be carrying more than 52 items; theoretically they
               might all be lit candles so avoid potential lets[] overflow */
            if (ct >= (int) sizeof lets - 1)
                break;
            lets[ct++] = obj_to_let(otmp);
        }
    lets[ct] = '\0';
    if (!ct)
        You("are not wearing or wielding anything.");
    else
        (void) display_inventory(lets, FALSE);
    return ECMD_OK;
}

/*
 * uses up an object that's on the floor, charging for it as necessary
 */
void
useupf(struct obj *obj, long numused)
{
    struct obj *otmp;
    boolean at_u = u_at(obj->ox, obj->oy);

    /* burn_floor_objects() keeps an object pointer that it tries to
     * useupf() multiple times, so obj must survive if plural */
    if (obj->quan > numused)
        otmp = splitobj(obj, numused);
    else
        otmp = obj;
    if (costly_spot(otmp->ox, otmp->oy)) {
        if (strchr(u.urooms, *in_rooms(otmp->ox, otmp->oy, 0)))
            addtobill(otmp, FALSE, FALSE, FALSE);
        else
            (void) stolen_value(otmp, otmp->ox, otmp->oy, FALSE, FALSE);
    }
    delobj(otmp);
    if (at_u && u.uundetected && hides_under(g.youmonst.data))
        (void) hideunder(&g.youmonst);
}

/*
 * Conversion from a class to a string for printing.
 * This must match the object class order.
 */
static NEARDATA const char *names[] = {
    0, "Illegal objects", "Weapons", "Armor", "Rings", "Amulets", "Tools",
    "Comestibles", "Potions", "Scrolls", "Spellbooks", "Wands", "Coins",
    "Gems/Stones", "Boulders/Statues", "Iron balls", "Chains", "Venoms"
};
static NEARDATA const char oth_symbols[] = { CONTAINED_SYM, '\0' };
static NEARDATA const char *oth_names[] = { "Bagged/Boxed items" };

DISABLE_WARNING_FORMAT_NONLITERAL

char *
let_to_name(char let, boolean unpaid, boolean showsym)
{
    const char *ocsymfmt = "  ('%c')";
    const int invbuf_sympadding = 8; /* arbitrary */
    const char *class_name;
    const char *pos;
    int oclass = (let >= 1 && let < MAXOCLASSES) ? let : 0;
    unsigned len;

    if (oclass)
        class_name = names[oclass];
    else if ((pos = strchr(oth_symbols, let)) != 0)
        class_name = oth_names[pos - oth_symbols];
    else
        class_name = names[0];

    len = Strlen(class_name) + (unpaid ? sizeof "unpaid_" : sizeof "")
          + (oclass ? (Strlen(ocsymfmt) + invbuf_sympadding) : 0);
    if (len > g.invbufsiz) {
        if (g.invbuf)
            free((genericptr_t) g.invbuf);
        g.invbufsiz = len + 10; /* add slop to reduce incremental realloc */
        g.invbuf = (char *) alloc(g.invbufsiz);
    }
    if (unpaid)
        Strcat(strcpy(g.invbuf, "Unpaid "), class_name);
    else
        Strcpy(g.invbuf, class_name);
    if ((oclass != 0) && showsym) {
        char *bp = eos(g.invbuf);
        int mlen = invbuf_sympadding - Strlen(class_name);
        while (--mlen > 0) {
            *bp = ' ';
            bp++;
        }
        *bp = '\0';
        Sprintf(eos(g.invbuf), ocsymfmt, def_oc_syms[oclass].sym);
    }
    return g.invbuf;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* release the static buffer used by let_to_name() */
void
free_invbuf(void)
{
    if (g.invbuf)
        free((genericptr_t) g.invbuf), g.invbuf = (char *) 0;
    g.invbufsiz = 0;
}

/* give consecutive letters to every item in inventory (for !fixinv mode);
   gold is always forced to '$' slot at head of list */
void
reassign(void)
{
    int i;
    struct obj *obj, *prevobj, *goldobj;

    /* first, remove [first instance of] gold from invent, if present */
    prevobj = goldobj = 0;
    for (obj = g.invent; obj; prevobj = obj, obj = obj->nobj)
        if (obj->oclass == COIN_CLASS) {
            goldobj = obj;
            if (prevobj)
                prevobj->nobj = goldobj->nobj;
            else
                g.invent = goldobj->nobj;
            break;
        }
    /* second, re-letter the rest of the list */
    for (obj = g.invent, i = 0; obj; obj = obj->nobj, i++)
        obj->invlet =
            (i < 26) ? ('a' + i) : (i < 52) ? ('A' + i - 26) : NOINVSYM;
    /* third, assign gold the "letter" '$' and re-insert it at head */
    if (goldobj) {
        goldobj->invlet = GOLD_SYM;
        goldobj->nobj = g.invent;
        g.invent = goldobj;
    }
    if (i >= 52)
        i = 52 - 1;
    g.lastinvnr = i;
}

/* invent gold sanity check; used by doorganize() to control how getobj()
   deals with gold and also by wizard mode sanity_check() */
boolean
check_invent_gold(const char *why) /* 'why' == caller in case of warning */
{
    struct obj *otmp;
    int goldstacks = 0, wrongslot = 0;

    /* there should be at most one stack of gold in invent, in slot '$' */
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (otmp->oclass == COIN_CLASS) {
            ++goldstacks;
            if (otmp->invlet != GOLD_SYM)
                ++wrongslot;
        }

    if (goldstacks > 1 || wrongslot > 0) {
        impossible("%s: %s%s%s", why,
                   (wrongslot > 1) ? "gold in wrong slots"
                      : (wrongslot > 0) ? "gold in wrong slot"
                           : "",
                   (wrongslot > 0 && goldstacks > 1) ? " and " : "",
                   (goldstacks > 1) ? "multiple gold stacks" : "");
        return TRUE; /* gold can be #adjusted */
    }

    return FALSE; /* gold can't be #adjusted */
}

/* normal getobj callback for item to #adjust; excludes gold */
static int
adjust_ok(struct obj *obj)
{
    if (!obj || obj->oclass == COIN_CLASS)
        return GETOBJ_EXCLUDE;

    return GETOBJ_SUGGEST;
}

/* getobj callback for item to #adjust if gold is wonky; allows gold */
static int
adjust_gold_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    return GETOBJ_SUGGEST;
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
 *
 *      Gold is only a candidate to adjust if we've somehow managed
 *      to get multiple stacks and/or it is in a slot other than '$'.
 *      Specifying a count to split it into two stacks is not allowed.
 */
int
doorganize(void) /* inventory organizer by Del Lamb */
{
    int (*adjust_filter)(struct obj *);
    struct obj *obj;

    /* when no invent, or just gold in '$' slot, there's nothing to adjust */
    if (!g.invent || (g.invent->oclass == COIN_CLASS
                      && g.invent->invlet == GOLD_SYM && !g.invent->nobj)) {
        You("aren't carrying anything %s.",
            !g.invent ? "to adjust" : "adjustable");
        return ECMD_OK;
    }

    if (!flags.invlet_constant)
        reassign();

    /* filter passed to getobj() depends upon gold sanity */
    adjust_filter = check_invent_gold("adjust") ? adjust_gold_ok : adjust_ok;

    /* get object the user wants to organize (the 'from' slot) */
    obj = getobj("adjust", adjust_filter, GETOBJ_PROMPT | GETOBJ_ALLOWCNT);

    return doorganize_core(obj);
}

/* alternate version of #adjust used by itemactions() for splitting */
int
adjust_split(void)
{
    struct obj *obj;
    cmdcount_nht splitamount = 0L;
    char let, dig = '\0';

    /* invlet should be queued so no getobj prompting is expected */
    obj = getobj("split", adjust_ok, GETOBJ_NOFLAGS);
    if (!obj || obj->quan < 2L || obj->otyp == GOLD_PIECE)
        return ECMD_FAIL; /* caller has set things up to avoid this */

    if (obj->quan == 2L) {
        splitamount = 1L;
    } else {
        /* get first digit; doesn't wait for <return> */
        dig = yn_function("Split off how many?", (char *) 0, '\0', TRUE);
        if (!digit(dig)) {
            pline1(Never_mind);
            return ECMD_CANCEL;
        }
        /* got first digit, get more until next non-digit (except for
           backspace/delete which will take away most recent digit and
           keep going; we expect one of ' ', '\n', or '\r') */
        let = get_count(NULL, dig, 0L, &splitamount,
                        /* yn_function() added the first digit to the
                           prompt when recording message history; have
                           get_count() display "Count: N" when waiting
                           for additional digits (ordinarily that won't be
                           shown until a second digit is entered) and also
                           add "Count: N" to message history if more than
                           one digit gets entered or the original N is
                           deleted and replaced with different digit */
                        GC_ECHOFIRST | GC_CONDHIST);
        /* \033 is in quitchars[] so we need to check for it separately
           in order to treat it as cancel rather than as accept */
        if (!let || let == '\033' || !strchr(quitchars, let)) {
            pline1(Never_mind);
            return ECMD_CANCEL;
        }
    }
    if (splitamount < 1L || splitamount >= obj->quan) {
        static const char
            Amount[] = "Amount to split from current stack must be";

        if (splitamount < 1L)
            pline("%s at least 1.", Amount);
        else
            pline("%s less than %ld.", Amount, obj->quan);
        return ECMD_CANCEL;
    }

    /* normally a split would take place in getobj() if player supplies
       a count there, so doorganize_core() figures out 'splitamount'
       from the object; it will undo the split if player cancels while
       selecting the destination slot */
    obj = splitobj(obj, (long) splitamount);
    return doorganize_core(obj);
}

static int
doorganize_core(struct obj *obj)
{
    struct obj *otmp, *splitting, *bumped;
    int ix, cur, trycnt;
    char let;
#define GOLD_INDX   0
#define GOLD_OFFSET 1
#define OVRFLW_INDX (GOLD_OFFSET + 52) /* past gold and 2*26 letters */
    char lets[1 + 52 + 1 + 1]; /* room for '$a-zA-Z#\0' */
    char qbuf[QBUFSZ];
    char *objname, *otmpname;
    const char *adj_type;
    boolean ever_mind = FALSE, collect, isgold;

    if (!obj)
        return ECMD_CANCEL;

    /* can only be gold if check_invent_gold() found a problem:  multiple '$'
       stacks and/or gold in some other slot, otherwise (*adjust_filter)()
       won't allow gold to be picked; if player has picked any stack of gold
       as #adjust 'from' slot, we'll force the 'to' slot to be '$' below */
    isgold = (obj->oclass == COIN_CLASS);

    /* figure out whether user gave a split count to getobj() */
    splitting = bumped = 0;
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
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
        lets[ix + (splitting ? 1 : 2)] = '\0';

    /* blank out all the letters currently in use in the inventory
       except those that will be merged with the selected object */
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
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
    if (!splitting)
        Strcpy(qbuf, "Adjust letter");
    else /* note: splitting->quan is the amount being left in original slot */
        Sprintf(qbuf, "Split %ld", obj->quan);
    Sprintf(eos(qbuf), " to what [%s]%s?", lets,
            g.invent ? " (? see used letters)" : "");
    for (trycnt = 1; ; ++trycnt) {
        let = !isgold ? yn_function(qbuf, (char *) 0, '\0', TRUE) : GOLD_SYM;
        if (let == '?' || let == '*') {
            let = display_used_invlets(splitting ? obj->invlet : 0);
            if (!let)
                continue;
            if (let == '\033')
                goto noadjust;
        }
        if (strchr(quitchars, let)
            /* adjusting to same slot is meaningful since all
               compatible stacks get collected along the way,
               but splitting to same slot is not */
            || (splitting && let == obj->invlet)) {
 noadjust:
            if (splitting)
                (void) merged(&splitting, &obj);
            if (!ever_mind)
                pline1(Never_mind);
            return ECMD_OK;
        } else if (let == GOLD_SYM && obj->oclass != COIN_CLASS) {
            pline("Only gold coins may be moved into the '%c' slot.",
                  GOLD_SYM);
            ever_mind = TRUE;
            goto noadjust;
        }
        /* letter() classifies '@' as one; compactify() can put '-' in lets;
           the only thing of interest that strchr() might find is '$' or '#'
           since letter() catches everything else that we put into lets[] */
        if ((letter(let) && let != '@') || (strchr(lets, let) && let != '-'))
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
    extract_nobj(obj, &g.invent);

    for (otmp = g.invent; otmp;) {
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
                extract_nobj(obj, &g.invent);
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
                    extract_nobj(obj, &g.invent);
                } else if (inv_cnt(FALSE) >= 52) {
                    (void) merged(&splitting, &obj); /* undo split */
                    /* "knapsack cannot accommodate any more items" */
                    Your("pack is too full.");
                    return ECMD_OK;
                } else {
                    bumped = otmp;
                    extract_nobj(bumped, &g.invent);
                }
            } /* moving vs splitting */
            break; /* not collecting and found 'to' slot */
        } /* collect */
        otmp = otmp->nobj;
    }

    /* inline addinv; insert loose object at beginning of inventory */
    obj->invlet = let;
    obj->nobj = g.invent;
    obj->where = OBJ_INVENT;
    g.invent = obj;
    reorder_invent();
    if (bumped) {
        /* splitting the 'from' stack is causing an incompatible
           stack in the 'to' slot to be moved into an open one;
           we need to do another inline insertion to inventory */
        assigninvlet(bumped);
        bumped->nobj = g.invent;
        bumped->where = OBJ_INVENT;
        g.invent = bumped;
        reorder_invent();
    }

    /* messages deferred until inventory has been fully reestablished */
    prinv(adj_type, obj, 0L);
    if (bumped)
        prinv("Moving:", bumped, 0L);
    if (splitting)
        clear_splitobjs(); /* reset splitobj context */
    update_inventory();
    return ECMD_OK;
}

/* common to display_minventory and display_cinventory */
static void
invdisp_nothing(const char *hdr, const char *txt)
{
    winid win;
    anything any;
    menu_item *selected;
    int clr = 0;

    any = cg.zeroany;
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    add_menu(win, &nul_glyphinfo, &any, 0, 0, iflags.menu_headings, clr,
             hdr, MENU_ITEMFLAGS_NONE);
    add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
             "", MENU_ITEMFLAGS_NONE);
    add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, txt,
             MENU_ITEMFLAGS_NONE);
    end_menu(win, (char *) 0);
    if (select_menu(win, PICK_NONE, &selected) > 0)
        free((genericptr_t) selected);
    destroy_nhwindow(win);
    return;
}

/* query_objlist callback: return things that are worn or wielded */
static boolean
worn_wield_only(struct obj *obj)
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
display_minventory(struct monst *mon, int dflags, char *title)
{
    struct obj *ret;
    char tmp[QBUFSZ];
    int n;
    menu_item *selected = 0;
    int do_all = (dflags & MINV_ALL) != 0,
        incl_hero = (do_all && engulfing_u(mon)),
        have_inv = (mon->minvent != 0), have_any = (have_inv || incl_hero),
        pickings = (dflags & MINV_PICKMASK);

    Sprintf(tmp, "%s %s:", s_suffix(noit_Monnam(mon)),
            do_all ? "possessions" : "armament");

    if (do_all ? have_any : (mon->misc_worn_check || MON_WEP(mon))) {
        /* Fool the 'weapon in hand' routine into
         * displaying 'weapon in claw', etc. properly.
         */
        g.youmonst.data = mon->data;
        /* in case inside a shop, don't append "for sale" prices */
        iflags.suppress_price++;

        n = query_objlist(title ? title : tmp, &(mon->minvent),
                          (INVORDER_SORT | (incl_hero ? INCLUDE_HERO : 0)),
                          &selected, pickings,
                          do_all ? allow_all : worn_wield_only);

        iflags.suppress_price--;
        /* was 'set_uasmon();' but that potentially has side-effects */
        g.youmonst.data = &mons[u.umonnum]; /* most basic part of set_uasmon */
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

/* format a container name for cinventory_display(), inserting "trapped"
   if that's appropriate */
static char *
cinv_doname(struct obj *obj)
{
    char *result = doname(obj);

    /*
     * If obj->tknown ever gets implemented, doname() will handle this.
     * Assumes that probing reveals the trap prior to calling us.  Since
     * we lack that flag, hero forgets about it as soon as we're done....
     */

    /* 'result' is an obuf[] but might point into the middle (&buf[PREFIX])
       rather than the beginning and we don't have access to that;
       assume that there is at least QBUFSZ available when reusing it */
    if (obj->otrapped && strlen(result) + sizeof "trapped " <= QBUFSZ) {
        /* obj->lknown has been set before calling us so either "locked" or
           "unlocked" should always be present (for a trapped container) */
        char *p = strstri(result, " locked"),
             *q = strstri(result, " unlocked");

        if (p && (!q || p < q))
            (void) strsubst(p, " locked ", " trapped locked ");
        else if (q)
            (void) strsubst(q, " unlocked ", " trapped unlocked ");
        /* might need to change "an" to "a"; when no BUC is present,
           "an unlocked" yielded "an trapped unlocked" above */
        (void) strsubst(result, "an trapped ", "a trapped ");
    }
    return result;
}

/* used by safe_qbuf() if the full doname() result is too long */
static char *
cinv_ansimpleoname(struct obj *obj)
{
    char *result = ansimpleoname(obj);

    /* result is an obuf[] so we know this will always fit */
    if (obj->otrapped) {
        if (strncmp(result, "a ", 2))
            (void) strsubst(result, "a ", "a trapped ");
        else if (strncmp(result, "an ", 3))
            (void) strsubst(result, "an ", "an trapped ");
        /* unique container? nethack doesn't have any */
        else if (strncmp(result, "the ", 4))
            (void) strsubst(result, "the ", "the trapped ");
        /* no leading article at all? shouldn't happen with ansimpleoname() */
        else
            (void) strsubst(result, "", "trapped "); /* insert at beginning */
    }
    return result;
}

/* Display the contents of a container in inventory style.
   Used for wand of probing of non-empty containers and statues. */
struct obj *
display_cinventory(struct obj *obj)
{
    struct obj *ret;
    char qbuf[QBUFSZ];
    int n;
    menu_item *selected = 0;

    (void) safe_qbuf(qbuf, "Contents of ", ":", obj,
                     /* custom formatting routines to insert "trapped"
                        into the object's name when appropriate;
                        last resort "that" won't ever get used */
                     cinv_doname, cinv_ansimpleoname, "that");

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


static boolean
only_here(struct obj *obj)
{
    return (obj->ox == g.only.x && obj->oy == g.only.y);
}

/*
 * Display a list of buried items in inventory style.  Return a non-zero
 * value if there were items at that spot.
 *
 * Currently, this is only used with a wand of probing zapped downwards.
 */
int
display_binventory(coordxy x, coordxy y, boolean as_if_seen)
{
    struct obj *obj;
    menu_item *selected = 0;
    int n;

    /* count # of objects here */
    for (n = 0, obj = g.level.buriedobjlist; obj; obj = obj->nobj)
        if (obj->ox == x && obj->oy == y) {
            if (as_if_seen)
                obj->dknown = 1;
            n++;
        }

    if (n) {
        g.only.x = x;
        g.only.y = y;
        if (query_objlist("Things that are buried here:",
                          &g.level.buriedobjlist, INVORDER_SORT,
                          &selected, PICK_NONE, only_here) > 0)
            free((genericptr_t) selected);
        g.only.x = g.only.y = 0;
    }
    return n;
}

void
prepare_perminvent(winid window)
{
    win_request_info *wri;

    if (!done_setting_perminv_flags) {
        /*TEMPORARY*/
        char *envtmp = !g.program_state.gameover ? nh_getenv("TTYINV") : 0;
        /* default for non-tty includes gold, for tty excludes gold;
           if non-tty specifies any value, gold will be excluded unless
           that value includes the show-gold bit (1) */
        int invmode = envtmp ? atoi(envtmp)
                      : !WINDOWPORT(tty) ? InvShowGold
                        : InvNormal;

        wri_info = zerowri;
        wri_info.fromcore.invmode = invmode;
        /*  relay the mode settings to the window port */
        wri = ctrl_nhwindow(window, set_mode, &wri_info);
        nhUse(wri);
        done_setting_perminv_flags = 1;
    }
}

void
sync_perminvent(void)
{
    static win_request_info *wri = 0;
    const char *wport_id;

    if (WIN_INVEN == WIN_ERR) {
        if ((g.core_invent_state
             || (wri_info.tocore.tocore_flags & prohibited))
            && !(in_perm_invent_toggled
                 && g.perm_invent_toggling_direction == toggling_on))
            return;
    }
    if (!done_setting_perminv_flags && WIN_INVEN != WIN_ERR)
        prepare_perminvent(WIN_INVEN);

    if ((!iflags.perm_invent && g.core_invent_state)) {
        /* Odd - but this could be end-of-game disclosure
         * which just sets boolean iflag.perm_invent to
         * FALSE without actually doing anything else.
         */
#ifdef TTY_PERM_INVENT
        if (WINDOWPORT(tty))
            perm_invent_toggled(TRUE); /* TRUE means negated */
#endif
        docrt();
        return;
    }

    /*
     * The following conditions can bring us to here:
     * 1. iflags.perm_invent is on
     *      AND
     *    g.core_invent_state is still zero.
     *
     * OR
     *
     * 2. iflags.perm_invent is off, but we're in the
     *    midst of toggling it on.
     */

    if ((iflags.perm_invent && !g.core_invent_state)
        || ((!iflags.perm_invent
            && (in_perm_invent_toggled
                && g.perm_invent_toggling_direction == toggling_on)))) {

        /* Send windowport a request to return the related settings to us */
        if ((iflags.perm_invent && !g.core_invent_state)
            || in_perm_invent_toggled) {
            if ((wri = ctrl_nhwindow(WIN_INVEN, request_settings, &wri_info))
                != 0) {
                if ((wri->tocore.tocore_flags & prohibited) != 0) {
                    /* sizes aren't good enough */
                    set_option_mod_status("perm_invent", set_gameview);
                    iflags.perm_invent = FALSE;
                    if (WIN_INVEN != WIN_ERR)
                        destroy_nhwindow(WIN_INVEN), WIN_INVEN = WIN_ERR;
                    if (WINDOWPORT(tty) && iflags.perm_invent)
                        wport_id = "tty perm_invent";
                    else
                        wport_id = "perm_invent";
                    pline("%s could not be enabled.", wport_id);
                    pline("%s needs a terminal that is at least %dx%d, yours "
                          "is %dx%d.",
                          wport_id, wri->tocore.needrows,
                          wri->tocore.needcols, wri->tocore.haverows,
                          wri->tocore.havecols);
                    wait_synch();
                    return;
                }
            }
            g.core_invent_state++;
        }
    }

    if (!wri || wri->tocore.maxslot == 0)
        return;

    if (in_perm_invent_toggled
        && g.perm_invent_toggling_direction == toggling_on) {
        WIN_INVEN = create_nhwindow(NHW_MENU);
    }

    if (WIN_INVEN != WIN_ERR && g.program_state.beyond_savefile_load) {
        g.in_sync_perminvent = 1;
        (void) display_inventory((char *) 0, FALSE);
        g.in_sync_perminvent = 0;
    }
}

void
perm_invent_toggled(boolean negated)
{
    in_perm_invent_toggled = TRUE;
    if (negated) {
        g.perm_invent_toggling_direction = toggling_off;
        if (WIN_INVEN != WIN_ERR)
            destroy_nhwindow(WIN_INVEN), WIN_INVEN = WIN_ERR;
        g.core_invent_state = 0;
    } else {
        g.perm_invent_toggling_direction = toggling_on;
        sync_perminvent();
    }
    g.perm_invent_toggling_direction = toggling_not;
    in_perm_invent_toggled = FALSE;
}

/*invent.c*/

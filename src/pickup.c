/* NetHack 3.7	pickup.c	$NHDT-Date: 1654760203 2022/06/09 07:36:43 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.310 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *      Contains code for picking objects up, and container use.
 */

#include "hack.h"

#define CONTAINED_SYM '>' /* from invent.c */

static void simple_look(struct obj *, boolean);
static boolean query_classes(char *, boolean *, boolean *, const char *,
                             struct obj *, boolean, int *);
static boolean fatal_corpse_mistake(struct obj *, boolean);
static boolean describe_decor(void);
static void check_here(boolean);
static boolean n_or_more(struct obj *);
static boolean all_but_uchain(struct obj *);
#if 0 /* not used */
static boolean allow_cat_no_uchain(struct obj *);
#endif
static int autopick(struct obj *, int, menu_item **);
static int count_categories(struct obj *, int);
static int delta_cwt(struct obj *, struct obj *);
static long carry_count(struct obj *, struct obj *, long, boolean, int *,
                        int *);
static int lift_object(struct obj *, struct obj *, long *, boolean);
static boolean mbag_explodes(struct obj *, int);
static boolean is_boh_item_gone(void);
static void do_boh_explosion(struct obj *, boolean);
static long boh_loss(struct obj *, boolean);
static int in_container(struct obj *);
static int out_container(struct obj *);
static long mbag_item_gone(boolean, struct obj *, boolean);
static int stash_ok(struct obj *);
static void explain_container_prompt(boolean);
static int traditional_loot(boolean);
static int menu_loot(int, boolean);
static int tip_ok(struct obj *);
static struct obj *tipcontainer_gettarget(struct obj *, boolean *);
static int tipcontainer_checks(struct obj *, struct obj *, boolean);
static char in_or_out_menu(const char *, struct obj *, boolean, boolean,
                           boolean, boolean);
static boolean able_to_loot(coordxy, coordxy, boolean);
static boolean reverse_loot(void);
static boolean mon_beside(coordxy, coordxy);
static int do_loot_cont(struct obj **, int, int);
static int doloot_core(void);
static void tipcontainer(struct obj *);

/* define for query_objlist() and autopickup() */
#define FOLLOW(curr, flags) \
    (((flags) & BY_NEXTHERE) ? (curr)->nexthere : (curr)->nobj)

#define GOLD_WT(n) (((n) + 50L) / 100L)
/* if you can figure this out, give yourself a hearty pat on the back... */
#define GOLD_CAPACITY(w, n) (((w) * -100L) - ((n) + 50L) - 1L)

#define Icebox (gc.current_container->otyp == ICE_BOX)

static const char
        moderateloadmsg[] = "You have a little trouble lifting",
        nearloadmsg[] = "You have much trouble lifting",
        overloadmsg[] = "You have extreme difficulty lifting";

/* BUG: this lets you look at cockatrice corpses while blind without
   touching them */
/* much simpler version of the look-here code; used by query_classes() */
static void
simple_look(struct obj *otmp, /* list of objects */
            boolean here)     /* flag for type of obj list linkage */
{
    /* Neither of the first two cases is expected to happen, since
     * we're only called after multiple classes of objects have been
     * detected, hence multiple objects must be present.
     */
    if (!otmp) {
        impossible("simple_look(null)");
    } else if (!(here ? otmp->nexthere : otmp->nobj)) {
        pline1(doname(otmp));
    } else {
        winid tmpwin = create_nhwindow(NHW_MENU);

        putstr(tmpwin, 0, "");
        do {
            putstr(tmpwin, 0, doname(otmp));
            otmp = here ? otmp->nexthere : otmp->nobj;
        } while (otmp);
        display_nhwindow(tmpwin, TRUE);
        destroy_nhwindow(tmpwin);
    }
}

int
collect_obj_classes(char ilets[], struct obj *otmp, boolean here,
                    boolean (*filter)(OBJ_P), int *itemcount)
{
    register int iletct = 0;
    register char c;

    *itemcount = 0;
    ilets[iletct] = '\0'; /* terminate ilets so that strchr() will work */
    while (otmp) {
        c = def_oc_syms[(int) otmp->oclass].sym;
        if (!strchr(ilets, c) && (!filter || (*filter)(otmp)))
            ilets[iletct++] = c, ilets[iletct] = '\0';
        *itemcount += 1;
        otmp = here ? otmp->nexthere : otmp->nobj;
    }

    return iletct;
}

/*
 * Suppose some '?' and '!' objects are present, but '/' objects aren't:
 *      "a" picks all items without further prompting;
 *      "A" steps through all items, asking one by one;
 *      "?" steps through '?' items, asking, and ignores '!' ones;
 *      "/" becomes 'A', since no '/' present;
 *      "?a" or "a?" picks all '?' without further prompting;
 *      "/a" or "a/" becomes 'A' since there aren't any '/'
 *          (bug fix:  3.1.0 thru 3.1.3 treated it as "a");
 *      "?/a" or "a?/" or "/a?",&c picks all '?' even though no '/'
 *          (ie, treated as if it had just been "?a").
 */
static boolean
query_classes(char oclasses[], boolean *one_at_a_time, boolean *everything,
              const char *action, struct obj *objs, boolean here,
              int *menu_on_demand)
{
    char ilets[36], inbuf[BUFSZ] = DUMMY; /* FIXME: hardcoded ilets[] length */
    int iletct, oclassct;
    boolean not_everything, filtered;
    char qbuf[QBUFSZ];
    boolean m_seen;
    int itemcount, bcnt, ucnt, ccnt, xcnt, ocnt, jcnt;

    oclasses[oclassct = 0] = '\0';
    *one_at_a_time = *everything = m_seen = FALSE;
    if (menu_on_demand)
        *menu_on_demand = 0;
    iletct = collect_obj_classes(ilets, objs, here,
                                 (boolean (*)(OBJ_P)) 0, &itemcount);
    if (iletct == 0)
        return FALSE;

    if (iletct == 1) {
        oclasses[0] = def_char_to_objclass(ilets[0]);
        oclasses[1] = '\0';
    } else { /* more than one choice available */
        /* additional choices */
        ilets[iletct++] = ' ';
        ilets[iletct++] = 'a';
        ilets[iletct++] = 'A';
        ilets[iletct++] = (objs == gi.invent ? 'i' : ':');
    }
    if (itemcount && menu_on_demand)
        ilets[iletct++] = 'm';
    if (count_unpaid(objs))
        ilets[iletct++] = 'u';

    tally_BUCX(objs, here, &bcnt, &ucnt, &ccnt, &xcnt, &ocnt, &jcnt);
    if (bcnt)
        ilets[iletct++] = 'B';
    if (ucnt)
        ilets[iletct++] = 'U';
    if (ccnt)
        ilets[iletct++] = 'C';
    if (xcnt)
        ilets[iletct++] = 'X';
    if (jcnt)
        ilets[iletct++] = 'P';
    ilets[iletct] = '\0';

    if (iletct > 1) {
        const char *where = 0;
        char sym, oc_of_sym, *p;

 ask_again:
        oclasses[oclassct = 0] = '\0';
        *one_at_a_time = *everything = FALSE;
        not_everything = filtered = FALSE;
        Sprintf(qbuf, "What kinds of thing do you want to %s? [%s]", action,
                ilets);
        getlin(qbuf, inbuf);
        if (*inbuf == '\033')
            return FALSE;

        for (p = inbuf; (sym = *p++) != 0; ) {
            if (sym == ' ')
                continue;
            else if (sym == 'A')
                *one_at_a_time = TRUE;
            else if (sym == 'a')
                *everything = TRUE;
            else if (sym == ':') {
                simple_look(objs, here); /* dumb if objs==invent */
                /* if we just scanned the contents of a container
                   then mark it as having known contents */
                if (objs->where == OBJ_CONTAINED)
                    objs->ocontainer->cknown = 1;
                goto ask_again;
            } else if (sym == 'i') {
                (void) display_inventory((char *) 0, TRUE);
                goto ask_again;
            } else if (sym == 'm') {
                m_seen = TRUE;
            } else if (strchr("uBUCXP", sym)) {
                add_valid_menu_class(sym); /* 'u' or 'B','U','C','X','P' */
                filtered = TRUE;
            } else {
                oc_of_sym = def_char_to_objclass(sym);
                if (strchr(ilets, sym)) {
                    add_valid_menu_class(oc_of_sym);
                    oclasses[oclassct++] = oc_of_sym;
                    oclasses[oclassct] = '\0';
                } else {
                    if (!where)
                        where = !strcmp(action, "pick up") ? "here"
                                : !strcmp(action, "take out") ? "inside" : "";
                    if (*where)
                        There("are no %c's %s.", sym, where);
                    else
                        You("have no %c's.", sym);
                    not_everything = TRUE;
                }
            }
        } /* for p:sym in inbuf */

        if (m_seen && menu_on_demand) {
            *menu_on_demand = (((*everything || !oclassct) && !filtered)
                               ? -2 : -3);
            return FALSE;
        }
        if (!oclassct && (!*everything || not_everything)) {
            /* didn't pick anything,
               or tried to pick something that's not present */
            *one_at_a_time = TRUE; /* force 'A' */
            *everything = FALSE;   /* inhibit 'a' */
        }
    }
    return TRUE;
}

/* check whether hero is bare-handedly touching a cockatrice corpse */
static boolean
fatal_corpse_mistake(struct obj *obj, boolean remotely)
{
    if (uarmg || remotely || obj->otyp != CORPSE
        || !touch_petrifies(&mons[obj->corpsenm]) || Stone_resistance)
        return FALSE;

    if (poly_when_stoned(gy.youmonst.data) && polymon(PM_STONE_GOLEM)) {
        display_nhwindow(WIN_MESSAGE, FALSE); /* --More-- */
        return FALSE;
    }

    pline("Touching %s is a fatal mistake.",
          corpse_xname(obj, (const char *) 0, CXN_SINGULAR | CXN_ARTICLE));
    instapetrify(killer_xname(obj));
    return TRUE;
}

/* attempting to manipulate a Rider's corpse triggers its revival */
boolean
rider_corpse_revival(struct obj *obj, boolean remotely)
{
    if (!obj || obj->otyp != CORPSE || !is_rider(&mons[obj->corpsenm]))
        return FALSE;

    pline("At your %s, the corpse suddenly moves...",
          remotely ? "attempted acquisition" : "touch");
    (void) revive_corpse(obj);
    exercise(A_WIS, FALSE);
    return TRUE;
}

void
deferred_decor(boolean setup) /* True: deferring, False: catching up */
{
    if (setup) {
        iflags.defer_decor = TRUE;
    } else {
        (void) describe_decor();
        iflags.defer_decor = FALSE;
    }
}

/* handle 'mention_decor' (when walking onto a dungeon feature such as
   stairs or altar, describe it even if it isn't covered up by an object) */
static boolean
describe_decor(void)
{
    char outbuf[BUFSZ], fbuf[QBUFSZ];
    boolean doorhere, waterhere, res = TRUE;
    const char *dfeature;
    int ltyp;

    if ((HFumbling & TIMEOUT) == 1L && !iflags.defer_decor) {
        /*
         * Work around a message sequencing issue:  avoid
         *  |You are back on floor.
         *  |You trip over <object>.  or  You flounder.
         * when the trip is being caused by moving on ice as hero
         * steps off ice onto non-ice.
         */
        deferred_decor(TRUE);
        return FALSE;
    }

    ltyp = levl[u.ux][u.uy].typ;
    if (ltyp == DRAWBRIDGE_UP) /* surface for spot in front of closed db */
        ltyp = db_under_typ(levl[u.ux][u.uy].drawbridgemask);
    dfeature = dfeature_at(u.ux, u.uy, fbuf);

    /* we don't mention "ordinary" doors but do mention broken ones (and
       closed ones, which will only happen for Passes_walls) */
    doorhere = dfeature && (!strcmp(dfeature, "open door")
                            || !strcmp(dfeature, "doorway"));
    waterhere = dfeature && !strcmp(dfeature, "pool of water");
    if (doorhere || Underwater
        || (ltyp == ICE && IS_POOL(iflags.prev_decor))) /* pooleffects() */
        dfeature = 0;

    if (ltyp == iflags.prev_decor && !IS_FURNITURE(ltyp)) {
        res = FALSE;
    } else if (dfeature) {
        if (waterhere)
            dfeature = strcpy(fbuf, waterbody_name(u.ux, u.uy));
        if (strcmp(dfeature, "swamp"))
            dfeature = an(dfeature);

        if (Verbose(2, describe_decor1)) {
            Sprintf(outbuf, "There is %s here.", dfeature);
        } else {
            if (dfeature != fbuf)
                Strcpy(fbuf, dfeature);
            Sprintf(outbuf, "%s.", upstart(fbuf));
        }
        pline("%s", outbuf);
    } else if (!Underwater) {
        if (IS_POOL(iflags.prev_decor)
            || iflags.prev_decor == LAVAPOOL
            || iflags.prev_decor == ICE) {
            const char *ground = surface(u.ux, u.uy);

            if (iflags.last_msg != PLNMSG_BACK_ON_GROUND)
                pline("%s %s %s.",
                      Verbose(2, describe_decor2) ? "You are back" : "Back",
                      (Levitation || Flying) ? "over" : "on",
                      ground);
        }
    }
    iflags.prev_decor = ltyp;
    return res;
}

/* look at the objects at our location, unless there are too many of them */
static void
check_here(boolean picked_some)
{
    register struct obj *obj;
    register int ct = 0;
    unsigned lhflags = picked_some ? LOOKHERE_PICKED_SOME : LOOKHERE_NOFLAGS;

    if (flags.mention_decor) {
        if (describe_decor())
            lhflags |= LOOKHERE_SKIP_DFEATURE;
    }

    /* count the objects here */
    for (obj = gl.level.objects[u.ux][u.uy]; obj; obj = obj->nexthere) {
        if (obj != uchain)
            ct++;
    }

    /* If there are objects here, take a look. */
    if (ct) {
        if (gc.context.run)
            nomul(0);
        flush_screen(1);
        (void) look_here(ct, lhflags);
    } else {
        read_engr_at(u.ux, u.uy);
    }
}

/* query_objlist callback: return TRUE if obj's count is >= reference value */
static boolean
n_or_more(struct obj *obj)
{
    if (obj == uchain)
        return FALSE;
    return (boolean) (obj->quan >= gv.val_for_n_or_more);
}

/* check valid_menu_classes[] for an entry; also used by askchain() */
boolean
menu_class_present(int c)
{
    return (c && strchr(gv.valid_menu_classes, c)) ? TRUE : FALSE;
}

void
add_valid_menu_class(int c)
{
    static int vmc_count = 0;

    if (c == 0) { /* reset */
        vmc_count = 0;
        gc.class_filter = gb.bucx_filter = gs.shop_filter = FALSE;
        gp.picked_filter = FALSE;
    } else if (!menu_class_present(c)) {
        gv.valid_menu_classes[vmc_count++] = (char) c;
        /* categorize the new class */
        switch (c) {
        case 'B':
        case 'U':
        case 'C': /*FALLTHRU*/
        case 'X':
            gb.bucx_filter = TRUE;
            break;
        case 'P':
            gp.picked_filter = TRUE;
            break;
        case 'u':
            gs.shop_filter = TRUE;
            break;
        default:
            gc.class_filter = TRUE;
            break;
        }
    }
    gv.valid_menu_classes[vmc_count] = '\0';
}

/* query_objlist callback: return TRUE if not uchain */
static boolean
all_but_uchain(struct obj *obj)
{
    return (boolean) (obj != uchain);
}

/* query_objlist callback: return TRUE */
/*ARGUSED*/
boolean
allow_all(struct obj *obj UNUSED)
{
    return TRUE;
}

boolean
allow_category(struct obj *obj)
{
    /* For coins, if any class filter is specified, accept if coins
     * are included regardless of whether either unpaid or BUC-status
     * is also specified since player has explicitly requested coins.
     */
    if (obj->oclass == COIN_CLASS && gc.class_filter)
        return strchr(gv.valid_menu_classes, COIN_CLASS) ? TRUE : FALSE;

    if (Role_if(PM_CLERIC) && !obj->bknown)
        set_bknown(obj, 1);

    /*
     * Version 3.6 had three types of filters possible and the first
     * and third can have more than one entry:
     *  1) object class (armor, potion, &c);
     *  2) unpaid shop item;
     *  3) bless/curse state (blessed, uncursed, cursed, BUC-unknown).
     * Version 3.7 added a fourth:
     *  4) 'novelty' ('P' for just picked up items).
     * When only one type is present, the situation is simple:
     * to be accepted, obj's status must match one of the entries.
     * When more than one type is present, the obj will now only
     * be accepted when it matches one entry of each type.
     * So ?!B will accept blessed scrolls or potions, and [u will
     * accept unpaid armor.  (In 3.4.3, an object was accepted by
     * this filter if it met any entry of any type, so ?!B resulted
     * in accepting all scrolls and potions regardless of bless/curse
     * state plus all blessed non-scroll, non-potion objects.)
     */

    /* if class is expected but obj's class is not in the list, reject */
    if (gc.class_filter && !strchr(gv.valid_menu_classes, obj->oclass))
        return FALSE;
    /* if unpaid is expected and obj isn't unpaid, reject (treat a container
       holding any unpaid object as unpaid even if isn't unpaid itself) */
    if (gs.shop_filter && !obj->unpaid
        && !(Has_contents(obj) && count_unpaid(obj->cobj) > 0))
        return FALSE;
    /* check for particular bless/curse state */
    if (gb.bucx_filter) {
        /* first categorize this object's bless/curse state */
        char bucx;
        if (obj->oclass == COIN_CLASS) {
            /* If no class filtering is specified but bless/curse state is,
               coins are treated as either unknown or uncursed based on an
               option setting. */
            bucx = flags.goldX ? 'X' : 'U';
        } else {
            bucx = !obj->bknown ? 'X'
                     : obj->blessed ? 'B'
                        : obj->cursed ? 'C'
                           : 'U';
        }

        /* if its category is not in the list, reject */
        if (!strchr(gv.valid_menu_classes, bucx))
            return FALSE;
    }
    if (gp.picked_filter && !obj->pickup_prev)
        return FALSE;
    /* obj didn't fail any of the filter checks, so accept */
    return TRUE;
}

#if 0 /* not used */
/* query_objlist callback: return TRUE if valid category (class), no uchain */
static boolean
allow_cat_no_uchain(struct obj *obj)
{
    if (obj != uchain
        && ((strchr(gv.valid_menu_classes, 'u') && obj->unpaid)
            || strchr(gv.valid_menu_classes, obj->oclass)))
        return TRUE;
    return FALSE;
}
#endif

/* query_objlist callback: return TRUE if valid class and worn */
boolean
is_worn_by_type(struct obj *otmp)
{
    return (is_worn(otmp) && allow_category(otmp)) ? TRUE : FALSE;
}

/* reset last-picked-up flags */
void
reset_justpicked(struct obj *olist)
{
    struct obj *otmp;
    /*
     * TODO?  Possible enchancement: don't reset if hero is still at same
     *  spot where most recent pickup took place.  Not resetting will be
     *  the correct behavior for autopickup immediately followed by manual
     *  pickup.  It would probably be correct for either or both pickups
     *  followed by manual pickup of a newly arrived missile after some
     *  time has elapsed.  Things becomes murkier for other activity.
     *  Taking anything out of a container ought to be treated as if
     *  having moved to another spot.
     */

    for (otmp = olist; otmp; otmp = otmp->nobj)
        otmp->pickup_prev = 0;
}

int
count_justpicked(struct obj *olist)
{
    struct obj *otmp;
    int cnt = 0;

    for (otmp = olist; otmp; otmp = otmp->nobj)
        if (otmp->pickup_prev)
            cnt++;

    return cnt;
}

struct obj *
find_justpicked(struct obj *olist)
{
    struct obj *otmp;

    for (otmp = olist; otmp; otmp = otmp->nobj)
        if (otmp->pickup_prev)
            return otmp;

    return (struct obj *) 0;
}

/*
 * Have the hero pick things from the ground
 * or a monster's inventory if swallowed.
 *
 * Arg what:
 *      >0  autopickup
 *      =0  interactive
 *      <0  pickup count of something
 *
 * Returns 1 if tried to pick something up, whether
 * or not it succeeded.
 */
int
pickup(int what) /* should be a long */
{
    int i, n, res, count, n_tried = 0, n_picked = 0;
    menu_item *pick_list = (menu_item *) 0;
    boolean autopickup = what > 0;
    struct obj **objchain_p;
    int traverse_how;

    /* we might have arrived here while fainted or sleeping, via
       random teleport or levitation timeout; if so, skip check_here
       and read_engr_at in addition to bypassing autopickup itself
       [probably ought to check whether hero is using a cockatrice
       corpse for a pillow here... (also at initial faint/sleep)] */
    if (autopickup && gm.multi < 0 && unconscious()) {
        iflags.prev_decor = STONE;
        return 0;
    }

    if (what < 0) /* pick N of something */
        count = -what;
    else /* pick anything */
        count = 0;

    if (!u.uswallow) {
        struct trap *t;

        /* no auto-pick if no-pick move, nothing there, or in a pool */
        if (autopickup && (gc.context.nopick || !OBJ_AT(u.ux, u.uy)
                           || (is_pool(u.ux, u.uy) && !Underwater)
                           || is_lava(u.ux, u.uy))) {
            if (flags.mention_decor)
                (void) describe_decor();
            read_engr_at(u.ux, u.uy);
            return 0;
        }
        /* no pickup if levitating & not on air or water level */
        t = t_at(u.ux, u.uy);
        if (!can_reach_floor(t && is_pit(t->ttyp))) {
            (void) describe_decor(); /* even when !flags.mention_decor */
            if ((gm.multi && !gc.context.run) || (autopickup && !flags.pickup)
                || (t && (uteetering_at_seen_pit(t) || uescaped_shaft(t))))
                read_engr_at(u.ux, u.uy);
            return 0;
        }
        /* multi && !gc.context.run means they are in the middle of some other
         * action, or possibly paralyzed, sleeping, etc.... and they just
         * teleported onto the object.  They shouldn't pick it up.
         */
        if ((gm.multi && !gc.context.run)
            || (autopickup && !flags.pickup)
            || notake(gy.youmonst.data)) {
            check_here(FALSE);
            if (notake(gy.youmonst.data) && OBJ_AT(u.ux, u.uy)
                && (autopickup || flags.pickup))
                You("are physically incapable of picking anything up.");
            return 0;
        }

        /* if there's anything here, stop running */
        if (OBJ_AT(u.ux, u.uy) && gc.context.run && gc.context.run != 8
            && !gc.context.nopick)
            nomul(0);
    }

    add_valid_menu_class(0); /* reset */
    if (!u.uswallow) {
        objchain_p = &gl.level.objects[u.ux][u.uy];
        traverse_how = BY_NEXTHERE;
    } else {
        objchain_p = &u.ustuck->minvent;
        traverse_how = 0; /* nobj */
    }
    /*
     * Start the actual pickup process.  This is split into two main
     * sections, the newer menu and the older "traditional" methods.
     * Automatic pickup has been split into its own menu-style routine
     * to make things less confusing.
     */
    if (autopickup) {
        n = autopick(*objchain_p, traverse_how, &pick_list);
        goto menu_pickup;
    }

    if (flags.menu_style != MENU_TRADITIONAL || iflags.menu_requested) {
        /* use menus exclusively */
        traverse_how |= AUTOSELECT_SINGLE
                        | (flags.sortpack ? INVORDER_SORT : 0);
        if (count) { /* looking for N of something */
            char qbuf[QBUFSZ];

            Sprintf(qbuf, "Pick %d of what?", count);
            gv.val_for_n_or_more = count; /* set up callback selector */
            n = query_objlist(qbuf, objchain_p, traverse_how,
                              &pick_list, PICK_ONE, n_or_more);
            /* correct counts, if any given */
            for (i = 0; i < n; i++)
                pick_list[i].count = count;
        } else {
            n = query_objlist("Pick up what?", objchain_p,
                              (traverse_how | FEEL_COCKATRICE),
                              &pick_list, PICK_ANY, all_but_uchain);
        }

 menu_pickup:
        if (n > 0)
            reset_justpicked(gi.invent);
        n_tried = n;
        for (n_picked = i = 0; i < n; i++) {
            res = pickup_object(pick_list[i].item.a_obj, pick_list[i].count,
                                FALSE);
            if (res < 0)
                break; /* can't continue */
            n_picked += res;
        }
        if (pick_list)
            free((genericptr_t) pick_list);

    } else {
        /* old style interface */
        int ct = 0;
        long lcount;
        boolean all_of_a_type, selective, bycat;
        char oclasses[MAXOCLASSES + 10]; /* +10: room for B,U,C,X plus slop */
        struct obj *obj, *obj2;

        oclasses[0] = '\0';   /* types to consider (empty for all) */
        all_of_a_type = TRUE; /* take all of considered types */
        selective = FALSE;    /* ask for each item */

        /* check for more than one object */
        for (obj = *objchain_p; obj; obj = FOLLOW(obj, traverse_how))
            ct++;

        if (ct == 1 && count) {
            /* if only one thing, then pick it */
            obj = *objchain_p;
            lcount = min(obj->quan, (long) count);
            n_tried++;
            reset_justpicked(gi.invent);
            if (pickup_object(obj, lcount, FALSE) > 0)
                n_picked++; /* picked something */
            goto end_query;

        } else if (ct >= 2) {
            int via_menu = 0;

            There("are %s objects here.", (ct <= 10) ? "several" : "many");
            if (!query_classes(oclasses, &selective, &all_of_a_type,
                               "pick up", *objchain_p,
                               (traverse_how & BY_NEXTHERE) ? TRUE : FALSE,
                               &via_menu)) {
                if (!via_menu)
                    goto pickupdone;
                if (selective)
                    traverse_how |= INVORDER_SORT;
                n = query_objlist("Pick up what?", objchain_p, traverse_how,
                                  &pick_list, PICK_ANY,
                                  (via_menu == -2) ? allow_all
                                                   : allow_category);
                goto menu_pickup;
            }
        }
        bycat = (menu_class_present('B') || menu_class_present('U')
                 || menu_class_present('C') || menu_class_present('X'));

        for (obj = *objchain_p; obj; obj = obj2) {
            obj2 = FOLLOW(obj, traverse_how);
            if (bycat ? !allow_category(obj)
                      : (!selective && oclasses[0]
                         && !strchr(oclasses, obj->oclass)))
                continue;

            lcount = -1L;
            if (!all_of_a_type) {
                char qbuf[BUFSZ];

                (void) safe_qbuf(qbuf, "Pick up ", "?", obj, doname,
                                 ansimpleoname, something);
                switch ((obj->quan < 2L) ? ynaq(qbuf) : ynNaq(qbuf)) {
                case 'q':
                    goto end_query; /* out 2 levels */
                case 'n':
                    continue;
                case 'a':
                    all_of_a_type = TRUE;
                    if (selective) {
                        selective = FALSE;
                        oclasses[0] = obj->oclass;
                        oclasses[1] = '\0';
                    }
                    break;
                case '#': /* count was entered */
                    if (!yn_number)
                        continue; /* 0 count => No */
                    lcount = (long) yn_number;
                    if (lcount > obj->quan)
                        lcount = obj->quan;
                    /*FALLTHRU*/
                default: /* 'y' */
                    break;
                }
            }
            if (lcount == -1L)
                lcount = obj->quan;

            if (!n_tried) /* reset just before the first item picked */
                reset_justpicked(gi.invent);

            n_tried++;
            if ((res = pickup_object(obj, lcount, FALSE)) < 0)
                break;
            n_picked += res;
        }
 end_query:
        ; /* statement required after label */
    }

    if (!u.uswallow) {
        if (hides_under(gy.youmonst.data))
            (void) hideunder(&gy.youmonst);

        /* position may need updating (invisible hero) */
        if (n_picked)
            newsym_force(u.ux, u.uy);

        /* check if there's anything else here after auto-pickup is done */
        if (autopickup)
            check_here(n_picked > 0);
    }
 pickupdone:
    add_valid_menu_class(0); /* reset */
    return (n_tried > 0);
}

struct autopickup_exception *
check_autopickup_exceptions(struct obj *obj)
{
    /*
     *  Does the text description of this match an exception?
     */
    struct autopickup_exception *ape = ga.apelist;

    if (ape) {
        char *objdesc = makesingular(doname(obj));

        while (ape && !regex_match(objdesc, ape->regex))
            ape = ape->next;
    }
    return ape;
}

boolean
autopick_testobj(struct obj *otmp, boolean calc_costly)
{
    struct autopickup_exception *ape;
    static boolean costly = FALSE;
    const char *otypes = flags.pickup_types;
    boolean pickit;

    /* calculate 'costly' just once for a given autopickup operation */
    if (calc_costly)
        costly = (otmp->where == OBJ_FLOOR
                  && costly_spot(otmp->ox, otmp->oy));

    /* first check: reject if an unpaid item in a shop */
    if (costly && !otmp->no_charge)
        return FALSE;

    /* check for pickup_types */
    pickit = (!*otypes || strchr(otypes, otmp->oclass));

    /* check for autopickup exceptions */
    ape = check_autopickup_exceptions(otmp);
    if (ape)
        pickit = ape->grab;

    /* pickup_thrown overrides pickup_types and exceptions */
    if (!pickit)
        pickit = (flags.pickup_thrown && otmp->was_thrown);
    return pickit;
}

/*
 * Pick from the given list using flags.pickup_types.  Return the number
 * of items picked (not counts).  Create an array that returns pointers
 * and counts of the items to be picked up.  If the number of items
 * picked is zero, the pickup list is left alone.  The caller of this
 * function must free the pickup list.
 */
static int
autopick(
    struct obj *olist,     /* the object list */
    int follow,            /* how to follow the object list */
    menu_item **pick_list) /* list of objects and counts to pick up */
{
    menu_item *pi; /* pick item */
    struct obj *curr;
    int n;
    boolean check_costly = TRUE;

    /* first count the number of eligible items */
    for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow)) {
        if (autopick_testobj(curr, check_costly))
            ++n;
        check_costly = FALSE; /* only need to check once per autopickup */
    }

    if (n) {
        *pick_list = pi = (menu_item *) alloc(sizeof (menu_item) * n);
        for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow)) {
            if (autopick_testobj(curr, FALSE)) {
                pi[n].item.a_obj = curr;
                pi[n].count = curr->quan;
                n++;
            }
        }
    }
    return n;
}

/*
 * Put up a menu using the given object list.  Only those objects on the
 * list that meet the approval of the allow function are displayed.  Return
 * a count of the number of items selected, as well as an allocated array of
 * menu_items, containing pointers to the objects selected and counts.  The
 * returned counts are guaranteed to be in bounds and non-zero.
 *
 * Query flags:
 *      BY_NEXTHERE       - Follow object list via nexthere instead of nobj.
 *      AUTOSELECT_SINGLE - Don't ask if only 1 object qualifies - just
 *                          use it.
 *      USE_INVLET        - Use object's invlet.
 *      INVORDER_SORT     - Use hero's pack order.
 *      INCLUDE_HERO      - Showing engulfer's invent; show hero too.
 *      SIGNAL_NOMENU     - Return -1 rather than 0 if nothing passes "allow".
 *      SIGNAL_ESCAPE     - Return -1 rather than 0 if player uses ESC to
 *                          pick nothing.
 *      FEEL_COCKATRICE   - touch corpse.
 */
int
query_objlist(const char *qstr,        /* query string */
              struct obj **olist_p,    /* the list to pick from */
              int qflags,              /* options to control the query */
              menu_item **pick_list,   /* return list of items picked */
              int how,                 /* type of query */
              boolean (*allow)(OBJ_P)) /* allow function */
{
    int i, n, tmpglyph;
    winid win;
    struct obj *curr, *last, fake_hero_object, *olist = *olist_p;
    char *pack, packbuf[MAXOCLASSES + 1];
    anything any;
    boolean printed_type_name, first,
            sorted = (qflags & INVORDER_SORT) != 0,
            engulfer = (qflags & INCLUDE_HERO) != 0,
            engulfer_minvent;
    unsigned sortflags;
    glyph_info tmpglyphinfo = nul_glyphinfo;
    Loot *sortedolist, *srtoli;
    int clr = 0;

    *pick_list = (menu_item *) 0;
    if (!olist && !engulfer)
        return 0;

    /* count the number of items allowed */
    for (n = 0, last = 0, curr = olist; curr; curr = FOLLOW(curr, qflags))
        if ((*allow)(curr)) {
            last = curr;
            n++;
        }
    /* can't depend upon 'engulfer' because that's used to indicate whether
       hero should be shown as an extra, fake item */
    engulfer_minvent = (olist && olist->where == OBJ_MINVENT
                        && engulfing_u(olist->ocarry));
    if (engulfer_minvent && n == 1 && olist->owornmask != 0L) {
        qflags &= ~AUTOSELECT_SINGLE;
    }
    if (engulfer) {
        ++n;
        /* don't autoselect swallowed hero if it's the only choice */
        qflags &= ~AUTOSELECT_SINGLE;
    }

    if (n == 0) /* nothing to pick here */
        return (qflags & SIGNAL_NOMENU) ? -1 : 0;

    if (n == 1 && (qflags & AUTOSELECT_SINGLE)) {
        *pick_list = (menu_item *) alloc(sizeof (menu_item));
        (*pick_list)->item.a_obj = last;
        (*pick_list)->count = last->quan;
        return 1;
    }

    sortflags = (((flags.sortloot == 'f'
                   || (flags.sortloot == 'l' && !(qflags & USE_INVLET)))
                  ? SORTLOOT_LOOT
                  : ((qflags & USE_INVLET) ? SORTLOOT_INVLET : 0))
                 | (flags.sortpack ? SORTLOOT_PACK : 0)
                 | ((qflags & FEEL_COCKATRICE) ? SORTLOOT_PETRIFY : 0));
    sortedolist = sortloot(&olist, sortflags,
                           (qflags & BY_NEXTHERE) ? TRUE : FALSE, allow);

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    if (gt.this_title) {
        /* dotypeinv() supplies gt.this_title to display as initial header;
           intentionally avoid the menu_headings highlight attribute here */
        add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, gt.this_title, MENU_ITEMFLAGS_NONE);
    }
    /*
     * Run through the list and add the objects to the menu.  If
     * INVORDER_SORT is set, we'll run through the list once for
     * each type so we can group them.  The allow function was
     * called by sortloot() and will be called once per item here.
     */
    pack = strcpy(packbuf, flags.inv_order);
    if (qflags & INCLUDE_VENOM)
        (void) strkitten(pack, VENOM_CLASS); /* venom is not in inv_order */
    first = TRUE;
    do {
        printed_type_name = FALSE;
        for (srtoli = sortedolist; ((curr = srtoli->obj) != 0); ++srtoli) {
            if (sorted && curr->oclass != *pack)
                continue;
            if ((qflags & FEEL_COCKATRICE) && curr->otyp == CORPSE
                && will_feel_cockatrice(curr, FALSE)) {
                destroy_nhwindow(win); /* stop the menu and revert */
                (void) look_here(0, LOOKHERE_NOFLAGS);
                unsortloot(&sortedolist);
                return 0;
            }
            if ((*allow)(curr)) {
                /* if sorting, print type name (once only) */
                if (sorted && !printed_type_name) {
                    any = cg.zeroany;
                    add_menu(win, &nul_glyphinfo, &any, 0, 0,
                             iflags.menu_headings, clr,
                             let_to_name(*pack, FALSE,
                                         ((how != PICK_NONE)
                                          && iflags.menu_head_objsym)),
                             MENU_ITEMFLAGS_NONE);
                    printed_type_name = TRUE;
                }

                any.a_obj = curr;
                tmpglyph = obj_to_glyph(curr, rn2_on_display_rng);
                map_glyphinfo(0, 0, tmpglyph, 0U, &tmpglyphinfo);
                add_menu(win, &tmpglyphinfo, &any,
                         (qflags & USE_INVLET) ? curr->invlet
                           : (first && curr->oclass == COIN_CLASS) ? '$' : 0,
                         def_oc_syms[(int) objects[curr->otyp].oc_class].sym,
                         ATR_NONE, clr, doname_with_price(curr),
                         MENU_ITEMFLAGS_NONE);
                first = FALSE;
            }
        }
        pack++;
    } while (sorted && *pack);
    unsortloot(&sortedolist);

    if (engulfer) {
        char buf[BUFSZ];

        any = cg.zeroany;
        if (sorted && n > 1) {
            Sprintf(buf, "%s Creatures",
                    digests(u.ustuck->data) ? "Swallowed" : "Engulfed");
            add_menu(win, &nul_glyphinfo, &any, 0, 0, iflags.menu_headings,
                     clr, buf, MENU_ITEMFLAGS_NONE);
        }
        fake_hero_object = cg.zeroobj;
        fake_hero_object.quan = 1L; /* not strictly necessary... */
        any.a_obj = &fake_hero_object;
        tmpglyph = mon_to_glyph(&gy.youmonst, rn2_on_display_rng);
        map_glyphinfo(0, 0, tmpglyph, 0U, &tmpglyphinfo);
        add_menu(win, &tmpglyphinfo, &any,
                 /* fake inventory letter, no group accelerator */
                 CONTAINED_SYM, 0, ATR_NONE, clr, an(self_lookat(buf)),
                 MENU_ITEMFLAGS_NONE);
    }

    end_menu(win, qstr);
    n = select_menu(win, how, pick_list);
    destroy_nhwindow(win);

    if (n > 0) {
        menu_item *mi;
        int k;

        /* fix up counts:  -1 means no count used => pick all;
           if fake_hero_object was picked, discard that choice */
        for (i = k = 0, mi = *pick_list; i < n; i++, mi++) {
            curr = mi->item.a_obj;
            if (curr == &fake_hero_object) {
                /* this isn't actually possible; fake item representing
                   hero is only included for look here (':'), not pickup,
                   and that's PICK_NONE so we can't get here from there */
                You_cant("pick yourself up!");
                continue;
            }
            if (engulfer_minvent && curr->owornmask != 0L) {
                You_cant("pick %s up.", ysimple_name(curr));
                continue;
            }
            if (mi->count == -1L || mi->count > curr->quan)
                mi->count = curr->quan;
            if (k < i)
                (*pick_list)[k] = *mi;
            ++k;
        }
        if (!k) {
            /* fake_hero was only choice so discard whole list */
            free((genericptr_t) *pick_list);
            *pick_list = 0;
            n = 0;
        } else if (k < n) {
            /* other stuff plus fake_hero; last slot is now unused
               (could be more than one if player tried to pick items
               worn by engulfer) */
            while (n > k) {
                --n;
                (*pick_list)[n].item = cg.zeroany;
                (*pick_list)[n].count = 0L;
            }
        }
    } else if (n < 0) {
        /* -1 is used for SIGNAL_NOMENU, so callers don't expect it
           to indicate that the player declined to make a choice */
        n = (qflags & SIGNAL_ESCAPE) ? -2 : 0;
    }
    return n;
}

/*
 * allow menu-based category (class) selection (for Drop,take off etc.)
 *
 */
int
query_category(const char *qstr,      /* query string */
               struct obj *olist,     /* the list to pick from */
               int qflags,            /* behavior modification flags */
               menu_item **pick_list, /* return list of items picked */
               int how)               /* type of query */
{
    int n;
    winid win;
    struct obj *curr;
    char *pack, packbuf[MAXOCLASSES + 1];
    anything any;
    boolean collected_type_name;
    char invlet;
    int ccount;
    boolean (*ofilter)(OBJ_P) = (boolean (*)(OBJ_P)) 0;
    boolean do_unpaid = FALSE;
    boolean do_blessed = FALSE, do_cursed = FALSE, do_uncursed = FALSE,
            do_buc_unknown = FALSE;
    int num_buc_types = 0;
    unsigned itemflags = MENU_ITEMFLAGS_NONE;
    int num_justpicked = 0;
    int clr = 0;

    *pick_list = (menu_item *) 0;
    if (!olist)
        return 0;
    if ((qflags & UNPAID_TYPES) && count_unpaid(olist))
        do_unpaid = TRUE;
    if (qflags & WORN_TYPES)
        ofilter = is_worn;
    if ((qflags & BUC_BLESSED) && count_buc(olist, BUC_BLESSED, ofilter)) {
        do_blessed = TRUE;
        num_buc_types++;
    }
    if ((qflags & BUC_CURSED) && count_buc(olist, BUC_CURSED, ofilter)) {
        do_cursed = TRUE;
        num_buc_types++;
    }
    if ((qflags & BUC_UNCURSED) && count_buc(olist, BUC_UNCURSED, ofilter)) {
        do_uncursed = TRUE;
        num_buc_types++;
    }
    if ((qflags & BUC_UNKNOWN) && count_buc(olist, BUC_UNKNOWN, ofilter)) {
        do_buc_unknown = TRUE;
        num_buc_types++;
    }
    if (qflags & JUSTPICKED) {
        num_justpicked = count_justpicked(olist);
    }

    ccount = count_categories(olist, qflags);
    /* no point in actually showing a menu for a single category */
    if (ccount == 1 && !do_unpaid && num_buc_types <= 1
        && !(qflags & BILLED_TYPES)) {
        for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
            if (ofilter && !(*ofilter)(curr))
                continue;
            break;
        }
        if (curr) {
            *pick_list = (menu_item *) alloc(sizeof(menu_item));
            (*pick_list)->item.a_int = curr->oclass;
            n = 1;
        } else {
            debugpline0("query_category: no single object match");
            n = 0;
        }
        /* early return is ok; there's no temp window yet */
        return n;
    }

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);

    pack = strcpy(packbuf, flags.inv_order);
    if (qflags & INCLUDE_VENOM)
        (void) strkitten(pack, VENOM_CLASS); /* venom is not in inv_order */

    if (qflags & CHOOSE_ALL) {
        invlet = 'A';
        any = cg.zeroany;
        any.a_int = 'A';
        itemflags = MENU_ITEMFLAGS_SKIPINVERT;
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE, clr,
                 (qflags & WORN_TYPES) ? "Auto-select every item being worn"
                                       : "Auto-select every relevant item",
                 itemflags);

        any = cg.zeroany;
        add_menu(win, &nul_glyphinfo, &any, 0, 0,
                 ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
    }

    if ((qflags & ALL_TYPES) && (ccount > 1)) {
        invlet = 'a';
        any = cg.zeroany;
        any.a_int = ALL_TYPES_SELECTED;
        itemflags = MENU_ITEMFLAGS_SKIPINVERT;
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE, clr,
                 (qflags & WORN_TYPES) ? "All worn types" : "All types",
                 itemflags);
        invlet = 'b';
    } else
        invlet = 'a';
    do {
        collected_type_name = FALSE;
        for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
            if (curr->oclass == *pack) {
                if (ofilter && !(*ofilter)(curr))
                    continue;
                if (!collected_type_name) {
                    any = cg.zeroany;
                    any.a_int = curr->oclass;
                    add_menu(
                        win, &nul_glyphinfo, &any, invlet++,
                        def_oc_syms[(int) objects[curr->otyp].oc_class].sym,
                        ATR_NONE, clr, let_to_name(*pack, FALSE,
                                                  (how != PICK_NONE)
                                                   && iflags.menu_head_objsym),
                        MENU_ITEMFLAGS_NONE);
                    collected_type_name = TRUE;
                }
            }
        }
        pack++;
        if (invlet >= 'u') {
            impossible("query_category: too many categories");
            n = 0;
            goto query_done;
        }
    } while (*pack);

    if (do_unpaid || (qflags & BILLED_TYPES) || do_blessed || do_cursed
        || do_uncursed || do_buc_unknown) {
        any = cg.zeroany;
        add_menu(win, &nul_glyphinfo, &any, 0, 0,
                 ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
    }

    /* unpaid items if there are any */
    if (do_unpaid) {
        invlet = 'u';
        any = cg.zeroany;
        any.a_int = 'u';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0,
                 ATR_NONE, clr, "Unpaid items", MENU_ITEMFLAGS_NONE);
    }
    /* billed items: checked by caller, so always include if BILLED_TYPES */
    if (qflags & BILLED_TYPES) {
        invlet = 'x';
        any = cg.zeroany;
        any.a_int = 'x';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE, clr,
                 "Unpaid items already used up", MENU_ITEMFLAGS_NONE);
    }

    /* items with b/u/c/unknown if there are any;
       this cluster of menu entries is in alphabetical order,
       reversing the usual sequence of 'U' and 'C' in BUCX */
    itemflags = MENU_ITEMFLAGS_SKIPINVERT;
    if (do_blessed) {
        invlet = 'B';
        any = cg.zeroany;
        any.a_int = 'B';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE,
                 clr, "Items known to be Blessed", itemflags);
    }
    if (do_cursed) {
        invlet = 'C';
        any = cg.zeroany;
        any.a_int = 'C';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE,
                 clr, "Items known to be Cursed", itemflags);
    }
    if (do_uncursed) {
        invlet = 'U';
        any = cg.zeroany;
        any.a_int = 'U';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE,
                 clr, "Items known to be Uncursed", itemflags);
    }
    if (do_buc_unknown) {
        invlet = 'X';
        any = cg.zeroany;
        any.a_int = 'X';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE,
                 clr, "Items of unknown Bless/Curse status", itemflags);
    }
    if (num_justpicked) {
        char tmpbuf[BUFSZ];

        if (num_justpicked == 1)
            Sprintf(tmpbuf, "Just picked up: %s",
                    doname(find_justpicked(olist)));
        else
            Sprintf(tmpbuf, "Items you just picked up");
        invlet = 'P';
        any = cg.zeroany;
        any.a_int = 'P';
        add_menu(win, &nul_glyphinfo, &any, invlet, 0, ATR_NONE,
                 clr, tmpbuf, itemflags);
    }
    end_menu(win, qstr);
    n = select_menu(win, how, pick_list);
 query_done:
    destroy_nhwindow(win);
    if (n < 0)
        n = 0; /* caller's don't expect -1 */
    return n;
}

static int
count_categories(struct obj *olist, int qflags)
{
    char *pack;
    boolean counted_category;
    int ccount = 0;
    struct obj *curr;

    pack = flags.inv_order;
    do {
        counted_category = FALSE;
        for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
            if (curr->oclass == *pack) {
                if ((qflags & WORN_TYPES)
                    && !(curr->owornmask & (W_ARMOR | W_ACCESSORY
                                            | W_WEAPONS)))
                    continue;
                if (!counted_category) {
                    ccount++;
                    counted_category = TRUE;
                }
            }
        }
        pack++;
    } while (*pack);
    return ccount;
}

/*
 *  How much the weight of the given container will change when the given
 *  object is removed from it.  Use before and after weight amounts rather
 *  than trying to match the calculation used by weight() in mkobj.c.
 */
static int
delta_cwt(struct obj *container, struct obj *obj)
{
    struct obj **prev;
    int owt, nwt;

    if (container->otyp != BAG_OF_HOLDING)
        return obj->owt;

    owt = nwt = container->owt;
    /* find the object so that we can remove it */
    for (prev = &container->cobj; *prev; prev = &(*prev)->nobj)
        if (*prev == obj)
            break;
    if (!*prev) {
        panic("delta_cwt: obj not inside container?");
    } else {
        /* temporarily remove the object and calculate resulting weight */
        *prev = obj->nobj;
        nwt = weight(container);
        *prev = obj; /* put the object back; obj->nobj is still valid */
    }
    return owt - nwt;
}

/* could we carry `obj'? if not, could we carry some of it/them? */
static long
carry_count(struct obj *obj,            /* object to pick up... */
            struct obj *container,      /* ...bag it is coming out of */
            long count,
            boolean telekinesis,
            int *wt_before, int *wt_after)
{
    boolean adjust_wt = container && carried(container),
            is_gold = obj->oclass == COIN_CLASS;
    int wt, iw, ow, oow;
    long qq, savequan, umoney;
    unsigned saveowt;
    const char *verb, *prefx1, *prefx2, *suffx;
    char obj_nambuf[BUFSZ], where[BUFSZ];

    savequan = obj->quan;
    saveowt = obj->owt;
    umoney = money_cnt(gi.invent);
    iw = max_capacity();

    if (count != savequan) {
        obj->quan = count;
        obj->owt = (unsigned) weight(obj);
    }
    wt = iw + (int) obj->owt;
    if (adjust_wt)
        wt -= delta_cwt(container, obj);
    /* This will go with silver+copper & new gold weight */
    if (is_gold) /* merged gold might affect cumulative weight */
        wt -= (GOLD_WT(umoney) + GOLD_WT(count) - GOLD_WT(umoney + count));
    if (count != savequan) {
        obj->quan = savequan;
        obj->owt = saveowt;
    }
    *wt_before = iw;
    *wt_after = wt;

    if (wt < 0)
        return count;

    /* see how many we can lift */
    if (is_gold) {
        iw -= (int) GOLD_WT(umoney);
        if (!adjust_wt) {
            qq = GOLD_CAPACITY((long) iw, umoney);
        } else {
            oow = 0;
            qq = 50L - (umoney % 100L) - 1L;
            if (qq < 0L)
                qq += 100L;
            for (; qq <= count; qq += 100L) {
                obj->quan = qq;
                obj->owt = (unsigned) GOLD_WT(qq);
                ow = (int) GOLD_WT(umoney + qq);
                ow -= delta_cwt(container, obj);
                if (iw + ow >= 0)
                    break;
                oow = ow;
            }
            iw -= oow;
            qq -= 100L;
        }
        if (qq < 0L)
            qq = 0L;
        else if (qq > count)
            qq = count;
        wt = iw + (int) GOLD_WT(umoney + qq);
    } else if (count > 1 || count < obj->quan) {
        /*
         * Ugh. Calc num to lift by changing the quan of the
         * object and calling weight.
         *
         * This works for containers only because containers
         * don't merge.  -dean
         */
        for (qq = 1L; qq <= count; qq++) {
            obj->quan = qq;
            obj->owt = (unsigned) (ow = weight(obj));
            if (adjust_wt)
                ow -= delta_cwt(container, obj);
            if (iw + ow >= 0)
                break;
            wt = iw + ow;
        }
        --qq;
    } else {
        /* there's only one, and we can't lift it */
        qq = 0L;
    }
    obj->quan = savequan;
    obj->owt = saveowt;

    if (qq < count) {
        /* some message will be given */
        Strcpy(obj_nambuf, doname(obj));
        if (container) {
            Sprintf(where, "in %s", the(xname(container)));
            verb = "carry";
        } else {
            Strcpy(where, "lying here");
            verb = telekinesis ? "acquire" : "lift";
        }
    } else {
        /* lint suppression */
        *obj_nambuf = *where = '\0';
        verb = "";
    }
    /* we can carry qq of them */
    if (qq > 0) {
        if (qq < count)
            You("can only %s %s of the %s %s.", verb,
                (qq == 1L) ? "one" : "some", obj_nambuf, where);
        *wt_after = wt;
        return qq;
    }

    if (!container)
        Strcpy(where, "here"); /* slightly shorter form */
    if (gi.invent || umoney) {
        prefx1 = "you cannot ";
        prefx2 = "";
        suffx = " any more";
    } else {
        prefx1 = (obj->quan == 1L) ? "it " : "even one ";
        prefx2 = "is too heavy for you to ";
        suffx = "";
    }
    There("%s %s %s, but %s%s%s%s.", otense(obj, "are"), obj_nambuf, where,
          prefx1, prefx2, verb, suffx);

    /* *wt_after = iw; */
    return 0L;
}

/* determine whether character is able and player is willing to carry `obj' */
static
int
lift_object(
    struct obj *obj,       /* object to pick up... */
    struct obj *container, /* ...bag it's coming out of */
    long *cnt_p,
    boolean telekinesis)
{
    int result, old_wt, new_wt, prev_encumbr, next_encumbr;

    if (obj->otyp == BOULDER && Sokoban) {
        You("cannot get your %s around this %s.", body_part(HAND),
            xname(obj));
        return -1;
    }
    /* override weight consideration for loadstone picked up by anybody
       and for boulder picked up by hero poly'd into a giant; override
       availability of open inventory slot iff not already carrying one */
    if (obj->otyp == LOADSTONE
        || (obj->otyp == BOULDER && throws_rocks(gy.youmonst.data))) {
        if (inv_cnt(FALSE) < 52 || !carrying(obj->otyp)
            || merge_choice(gi.invent, obj))
            return 1; /* lift regardless of current situation */
        /* if we reach here, we're out of slots and already have at least
           one of these, so treat this one more like a normal item */
        You("are carrying too much stuff to pick up %s %s.",
            (obj->quan == 1L) ? "another" : "more", simpleonames(obj));
        return -1;
    }

    *cnt_p = carry_count(obj, container, *cnt_p, telekinesis,
                         &old_wt, &new_wt);
    if (*cnt_p < 1L) {
        result = -1; /* nothing lifted */
    } else if (obj->oclass != COIN_CLASS
               /* [exception for gold coins will have to change
                   if silver/copper ones ever get implemented] */
               && inv_cnt(FALSE) >= 52 && !merge_choice(gi.invent, obj)) {
        /* if there is some gold here (and we haven't already skipped it),
           we aren't limited by the 52 item limit for it, but caller and
           "grandcaller" aren't prepared to skip stuff and then pickup
           just gold, so the best we can do here is vary the message */
        Your("knapsack cannot accommodate any more items%s.",
             /* floor follows by nexthere, otherwise container so by nobj */
             nxtobj(obj, GOLD_PIECE, (boolean) (obj->where == OBJ_FLOOR))
                 ? " (except gold)" : "");
        result = -1; /* nothing lifted */
    } else {
        result = 1;
        prev_encumbr = near_capacity();
        if (prev_encumbr < flags.pickup_burden)
            prev_encumbr = flags.pickup_burden;
        next_encumbr = calc_capacity(new_wt - old_wt);
        if (next_encumbr > prev_encumbr) {
            if (telekinesis) {
                result = 0; /* don't lift */
            } else {
                char qbuf[BUFSZ];
                long savequan = obj->quan;

                obj->quan = *cnt_p;
                Strcpy(qbuf, (next_encumbr > HVY_ENCUMBER)
                                 ? overloadmsg
                                 : (next_encumbr > MOD_ENCUMBER)
                                       ? nearloadmsg
                                       : moderateloadmsg);
                if (container)
                    (void) strsubst(qbuf, "lifting", "removing");
                Strcat(qbuf, " ");
                (void) safe_qbuf(qbuf, qbuf, ".  Continue?", obj, doname,
                                 ansimpleoname, something);
                obj->quan = savequan;
                switch (ynq(qbuf)) {
                case 'q':
                    result = -1;
                    break;
                case 'n':
                    result = 0;
                    break;
                default:
                    break; /* 'y' => result == 1 */
                }
                clear_nhwindow(WIN_MESSAGE);
            }
        }
    }

    if (obj->otyp == SCR_SCARE_MONSTER && result <= 0 && !container)
        obj->spe = 0;
    return result;
}

/*
 * Pick up <count> of obj from the ground and add it to the hero's inventory.
 * Returns -1 if caller should break out of its loop, 0 if nothing picked
 * up, 1 if otherwise.
 */
int
pickup_object(
    struct obj *obj,
    long count,
    boolean telekinesis) /* not picking it up directly by hand */
{
    int res, nearload;

    if (obj->quan < count) {
        impossible("pickup_object: count %ld > quan %ld?", count, obj->quan);
        return 0;
    }

    /* In case of auto-pickup, where we haven't had a chance
       to look at it yet; affects docall(SCR_SCARE_MONSTER). */
    if (!Blind)
        obj->dknown = 1;

    if (obj == uchain) { /* do not pick up attached chain */
        return 0;
    } else if (obj->where == OBJ_MINVENT && obj->owornmask != 0L
               && engulfing_u(obj->ocarry)) {
        You_cant("pick %s up.", ysimple_name(obj));
        return 0;
    } else if (obj->oartifact && !touch_artifact(obj, &gy.youmonst)) {
        return 0;
    } else if (obj->otyp == CORPSE) {
        if (fatal_corpse_mistake(obj, telekinesis)
            || rider_corpse_revival(obj, telekinesis))
            return -1;
    } else if (obj->otyp == SCR_SCARE_MONSTER) {
        int old_wt, new_wt;

        /* process a count before altering/deleting scrolls;
           tricky because being unable to lift full stack imposes an
           implicit count; unliftable ones should be treated as if
           the count excluded them so that they don't change state */
        if ((count = carry_count(obj, (struct obj *) 0,
                                 count ? count : obj->quan,
                                 FALSE, &old_wt, &new_wt)) < 1L)
            return -1; /* couldn't even pick up 1, so effectively untouched */
        /* all current callers handle a new object sanely when traversing
           a list; other half of a split will be left as-is and whatever
           already follows 'obj' will still be processed next */
        if (count > 0L && count < obj->quan)
            obj = splitobj(obj, count);

        if (obj->blessed) {
            unbless(obj);
        } else if (!obj->spe && !obj->cursed) {
            obj->spe = 1;
        } else {
            pline_The("scroll%s %s to dust as you %s %s up.", plur(obj->quan),
                      otense(obj, "turn"), telekinesis ? "raise" : "pick",
                      (obj->quan == 1L) ? "it" : "them");
            trycall(obj);
            useupf(obj, obj->quan);
            return 1; /* tried to pick something up and failed, but
                         don't want to terminate pickup loop yet   */
        }
    }

    if ((res = lift_object(obj, (struct obj *) 0, &count, telekinesis)) <= 0)
        return res;

    /* Whats left of the special case for gold :-) */
    if (obj->oclass == COIN_CLASS)
        gc.context.botl = 1;
    if (obj->quan != count && obj->otyp != LOADSTONE)
        obj = splitobj(obj, count);

    obj = pick_obj(obj);

    if (uwep && uwep == obj)
        gm.mrg_to_wielded = TRUE;
    nearload = near_capacity();
    prinv(nearload == SLT_ENCUMBER ? moderateloadmsg : (char *) 0, obj,
          count);
    gm.mrg_to_wielded = FALSE;
    return 1;
}

/*
 * Do the actual work of picking otmp from the floor or monster's interior
 * and putting it in the hero's inventory.  Take care of billing.  Return a
 * pointer to the object where otmp ends up.  This may be different
 * from otmp because of merging.
 */
struct obj *
pick_obj(struct obj *otmp)
{
    struct obj *result;
    int ox = otmp->ox, oy = otmp->oy;
    boolean robshop = (!u.uswallow && otmp != uball && costly_spot(ox, oy));

    obj_extract_self(otmp);
    newsym(ox, oy);

    /* for shop items, addinv() needs to be after addtobill() (so that
       object merger can take otmp->unpaid into account) but before
       remote_robbery() (which calls rob_shop() which calls setpaid()
       after moving costs of unpaid items to shop debt; setpaid()
       calls clear_unpaid() for lots of object chains, but 'otmp' isn't
       on any of those between obj_extract_self() and addinv(); for
       3.6.0, 'otmp' remained flagged as an unpaid item in inventory
       and triggered impossible() every time inventory was examined) */
    if (robshop) {
        char saveushops[5], fakeshop[2];

        /* addtobill cares about your location rather than the object's;
           usually they'll be the same, but not when using telekinesis
           (if ever implemented) or a grappling hook */
        Strcpy(saveushops, u.ushops);
        fakeshop[0] = *in_rooms(ox, oy, SHOPBASE);
        fakeshop[1] = '\0';
        Strcpy(u.ushops, fakeshop);
        /* sets obj->unpaid if necessary */
        addtobill(otmp, TRUE, FALSE, FALSE);
        Strcpy(u.ushops, saveushops);
        robshop = otmp->unpaid && !strchr(u.ushops, *fakeshop);
    }

    result = addinv(otmp);
    /* if you're taking a shop item from outside the shop, make shk notice */
    if (robshop)
        remote_burglary(ox, oy);

    return result;
}

/*
 * prints a message if encumbrance changed since the last check and
 * returns the new encumbrance value (from near_capacity()).
 */
int
encumber_msg(void)
{
    int newcap = near_capacity();

    if (go.oldcap < newcap) {
        switch (newcap) {
        case 1:
            Your("movements are slowed slightly because of your load.");
            break;
        case 2:
            You("rebalance your load.  Movement is difficult.");
            break;
        case 3:
            You("%s under your heavy load.  Movement is very hard.",
                stagger(gy.youmonst.data, "stagger"));
            break;
        default:
            You("%s move a handspan with this load!",
                newcap == 4 ? "can barely" : "can't even");
            break;
        }
        gc.context.botl = 1;
    } else if (go.oldcap > newcap) {
        switch (newcap) {
        case 0:
            Your("movements are now unencumbered.");
            break;
        case 1:
            Your("movements are only slowed slightly by your load.");
            break;
        case 2:
            You("rebalance your load.  Movement is still difficult.");
            break;
        case 3:
            You("%s under your load.  Movement is still very hard.",
                stagger(gy.youmonst.data, "stagger"));
            break;
        }
        gc.context.botl = 1;
    }

    go.oldcap = newcap;
    return newcap;
}

/* Is there a container at x,y. Optional: return count of containers at x,y */
int
container_at(coordxy x, coordxy y, boolean countem)
{
    struct obj *cobj, *nobj;
    int container_count = 0;

    for (cobj = gl.level.objects[x][y]; cobj; cobj = nobj) {
        nobj = cobj->nexthere;
        if (Is_container(cobj)) {
            container_count++;
            if (!countem)
                break;
        }
    }
    return container_count;
}

static boolean
able_to_loot(
    coordxy x, coordxy y,
    boolean looting) /* loot vs tip */
{
    const char *verb = looting ? "loot" : "tip";
    struct trap *t = t_at(x, y);

    if (!can_reach_floor(t && is_pit(t->ttyp))) {
        if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
            rider_cant_reach(); /* not skilled enough to reach */
        else
            cant_reach_floor(x, y, FALSE, TRUE);
        return FALSE;
    } else if ((is_pool(x, y) && (looting || !Underwater)) || is_lava(x, y)) {
        /* at present, can't loot in water even when Underwater;
           can tip underwater, but not when over--or stuck in--lava */
        You("cannot %s things that are deep in the %s.", verb,
            hliquid(is_lava(x, y) ? "lava" : "water"));
        return FALSE;
    } else if (nolimbs(gy.youmonst.data)) {
        pline("Without limbs, you cannot %s anything.", verb);
        return FALSE;
    } else if (looting && !freehand()) {
        pline("Without a free %s, you cannot loot anything.",
              body_part(HAND));
        return FALSE;
    }
    return TRUE;
}

static boolean
mon_beside(coordxy x, coordxy y)
{
    int i, j;
    coordxy nx, ny;

    for (i = -1; i <= 1; i++)
        for (j = -1; j <= 1; j++) {
            nx = x + i;
            ny = y + j;
            if (isok(nx, ny) && MON_AT(nx, ny))
                return TRUE;
        }
    return FALSE;
}

static int
do_loot_cont(
    struct obj **cobjp,
    int cindex, /* index of this container (1..N)... */
    int ccount) /* ...number of them (N) */
{
    struct obj *cobj = *cobjp;

    if (!cobj)
        return ECMD_OK;
    if (cobj->olocked) {
        int res = ECMD_OK;

#if 0
        if (ccount < 2 && (gl.level.objects[cobj->ox][cobj->oy] == cobj))
            pline("%s locked.",
                  cobj->lknown ? "It is" : "Hmmm, it turns out to be");
        else
#endif
        if (cobj->lknown)
            pline("%s is locked.", The(xname(cobj)));
        else
            pline("Hmmm, %s turns out to be locked.", the(xname(cobj)));
        cobj->lknown = 1;

        if (flags.autounlock) {
            struct obj *otmp, *unlocktool = 0;
            coordxy ox = cobj->ox, oy = cobj->oy;

            u.dz = 0; /* might be non-zero from previous command since
                       * #loot isn't a move command; pick_lock() cares */
            /* if both the untrap and apply_key bits are set, untrap
               attempt will be performed first but we need to set up
               unlocktool in case "check for trap?" is declined */
            if (((flags.autounlock & AUTOUNLOCK_APPLY_KEY) != 0
                 && (unlocktool = autokey(TRUE)) != 0)
                || (flags.autounlock & AUTOUNLOCK_UNTRAP) != 0) {
                /* pass ox and oy to avoid direction prompt */
                if (pick_lock(unlocktool, ox, oy, cobj))
                    res = ECMD_TIME;
                /* attempting to untrap or unlock might trigger a trap
                   which destroys 'cobj'; inform caller if that happens */
                for (otmp = gl.level.objects[ox][oy]; otmp;
                     otmp = otmp->nexthere)
                    if (otmp == cobj)
                        break;
                if (!otmp)
                    *cobjp = (struct obj *) 0;
                return res;
            }
            if ((flags.autounlock & AUTOUNLOCK_FORCE) != 0
                && res != ECMD_TIME
                && ccount == 1 && u_have_forceable_weapon()) {
                /* single container, and we could #force it open... */
                cmdq_add_ec(CQ_CANNED, doforce); /* doforce asks for confirmation */
                ga.abort_looting = TRUE;
            }
        }
        return res;
    }
    cobj->lknown = 1; /* floor container, so no need for update_inventory() */

    if (cobj->otyp == BAG_OF_TRICKS) {
        int tmp;

        You("carefully open %s...", the(xname(cobj)));
        pline("It develops a huge set of teeth and bites you!");
        tmp = rnd(10);
        losehp(Maybe_Half_Phys(tmp), "carnivorous bag", KILLED_BY_AN);
        makeknown(BAG_OF_TRICKS);
        ga.abort_looting = TRUE;
        return ECMD_TIME;
    }
    return use_container(cobjp, FALSE, (boolean) (cindex < ccount));
}

/* #loot extended command */
int
doloot(void)
{
    int res;

    gl.loot_reset_justpicked = TRUE;
    res = doloot_core();
    gl.loot_reset_justpicked = FALSE;
    return res;
}

/* loot a container on the floor or loot saddle from mon. */
static int
doloot_core(void)
{
    struct obj *cobj, *nobj;
    register int c = -1;
    int timepassed = 0;
    coord cc;
    boolean underfoot = TRUE;
    const char *dont_find_anything = "don't find anything";
    struct monst *mtmp;
    int prev_inquiry = 0;
    boolean prev_loot = FALSE;
    int num_conts = 0;
    int clr = 0;

    ga.abort_looting = FALSE;

    if (check_capacity((char *) 0)) {
        /* "Can't do that while carrying so much stuff." */
        return ECMD_OK;
    }
    if (nohands(gy.youmonst.data)) {
        You("have no hands!"); /* not `body_part(HAND)' */
        return ECMD_OK;
    }
    if (Confusion) {
        if (rn2(6) && reverse_loot())
            return ECMD_TIME;
        if (rn2(2)) {
            pline("Being confused, you find nothing to loot.");
            return ECMD_TIME; /* costs a turn */
        }             /* else fallthrough to normal looting */
    }
    cc.x = u.ux;
    cc.y = u.uy;

    if (iflags.menu_requested)
        goto lootmon;

 lootcont:
    if ((num_conts = container_at(cc.x, cc.y, TRUE)) > 0) {
        boolean anyfound = FALSE;

        if (!able_to_loot(cc.x, cc.y, TRUE))
            return ECMD_OK;

        if (Blind && !uarmg) {
            /* if blind and without gloves, attempting to #loot at the
               location of a cockatrice corpse is fatal before asking
               whether to manipulate any containers */
            for (nobj = sobj_at(CORPSE, cc.x, cc.y); nobj;
                 nobj = nxtobj(nobj, CORPSE, TRUE))
                if (will_feel_cockatrice(nobj, FALSE)) {
                    feel_cockatrice(nobj, FALSE);
                    /* if life-saved (or poly'd into stone golem),
                       terminate attempt to loot */
                    return ECMD_TIME;
                }
        }

        if (num_conts > 1) {
            /* use a menu to loot many containers */
            int n, i;
            winid win;
            anything any;
            menu_item *pick_list = (menu_item *) 0;

            any.a_void = 0;
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);

            for (cobj = gl.level.objects[cc.x][cc.y]; cobj;
                 cobj = cobj->nexthere)
                if (Is_container(cobj)) {
                    any.a_obj = cobj;
                    add_menu(win, &nul_glyphinfo, &any, 0, 0,
                             ATR_NONE, clr, doname(cobj), MENU_ITEMFLAGS_NONE);
                }
            end_menu(win, "Loot which containers?");
            n = select_menu(win, PICK_ANY, &pick_list);
            destroy_nhwindow(win);

            if (n > 0) {
                for (i = 1; i <= n; i++) {
                    cobj = pick_list[i - 1].item.a_obj;
                    timepassed |= do_loot_cont(&cobj, i, n);
                    if (ga.abort_looting) {
                        /* chest trap or magic bag explosion or <esc> */
                        free((genericptr_t) pick_list);
                        return (timepassed ? ECMD_TIME : ECMD_OK);
                    }
                }
                free((genericptr_t) pick_list);
            }
            if (n != 0)
                c = 'y';
        } else {
            for (cobj = gl.level.objects[cc.x][cc.y]; cobj; cobj = nobj) {
                nobj = cobj->nexthere;

                if (Is_container(cobj)) {
                    anyfound = TRUE;
                    timepassed |= do_loot_cont(&cobj, 1, 1);
                    if (ga.abort_looting)
                        /* chest trap or magic bag explosion or <esc> */
                        return (timepassed ? ECMD_TIME : ECMD_OK);
                }
            }
            if (anyfound)
                c = 'y';
        }
    } else if (IS_GRAVE(levl[cc.x][cc.y].typ)) {
        You("need to dig up the grave to effectively loot it...");
    }

    /*
     * 3.3.1 introduced directional looting for some things.
     */
 lootmon:
    if (c != 'y' && (mon_beside(u.ux, u.uy) || iflags.menu_requested)) {
        boolean looted_mon = FALSE;
        if (!get_adjacent_loc("Loot in what direction?",
                              "Invalid loot location", u.ux, u.uy, &cc))
            return ECMD_OK;
        underfoot = u_at(cc.x, cc.y);
        if (underfoot && container_at(cc.x, cc.y, FALSE))
            goto lootcont;
        if (u.dz < 0) {
            You("%s to loot on the %s.", dont_find_anything,
                ceiling(cc.x, cc.y));
            return ECMD_TIME;
        }
        mtmp = m_at(cc.x, cc.y);
        if (mtmp) {
            timepassed = loot_mon(mtmp, &prev_inquiry, &prev_loot);
            if (timepassed)
                looted_mon = TRUE;
        }
        /* always use a turn when choosing a direction is impaired,
           even if you've successfully targeted a saddled creature
           and then answered "no" to the "remove its saddle?" prompt */
        if (Confusion || Stunned)
            timepassed = 1;

        /* Preserve pre-3.3.1 behavior for containers.
         * Adjust this if-block to allow container looting
         * from one square away to change that in the future.
         */
        if (!looted_mon) {
            if (!underfoot && container_at(cc.x, cc.y, FALSE)) {
                if (mtmp) {
                    You_cant("loot anything %sthere with %s in the way.",
                             prev_inquiry ? "else " : "", mon_nam(mtmp));
                    return (timepassed ? ECMD_TIME : ECMD_OK);
                } else {
                    You("have to be at a container to loot it.");
                }
            } else {
                You("%s %s%shere to loot.", dont_find_anything,
                    (prev_inquiry || prev_loot) ? "else " : "",
                    !underfoot ? "t" : "");
                return (timepassed ? ECMD_TIME : ECMD_OK);
            }
        }
    } else if (c != 'y' && c != 'n') {
        You("%s %s to loot.", dont_find_anything,
            underfoot ? "here" : "there");
    }
    return (timepassed ? ECMD_TIME : ECMD_OK);
}

/* called when attempting to #loot while confused */
static boolean
reverse_loot(void)
{
    struct obj *goldob = 0, *coffers, *otmp, boxdummy;
    struct monst *mon;
    long contribution;
    int n, x = u.ux, y = u.uy;

    if (!rn2(3)) {
        /* n objects: 1/(n+1) chance per object plus 1/(n+1) to fall off end
         */
        for (n = inv_cnt(TRUE), otmp = gi.invent; otmp; --n, otmp = otmp->nobj)
            if (!rn2(n + 1)) {
                prinv("You find old loot:", otmp, 0L);
                return TRUE;
            }
        return FALSE;
    }

    /* find a money object to mess with */
    for (goldob = gi.invent; goldob; goldob = goldob->nobj)
        if (goldob->oclass == COIN_CLASS) {
            contribution = ((long) rnd(5) * goldob->quan + 4L) / 5L;
            if (contribution < goldob->quan)
                goldob = splitobj(goldob, contribution);
            break;
        }
    if (!goldob)
        return FALSE;

    if (!IS_THRONE(levl[x][y].typ)) {
        dropx(goldob);
        /* the dropped gold might have fallen to lower level */
        if (g_at(x, y))
            pline("Ok, now there is loot here.");
    } else {
        /* find original coffers chest if present, otherwise use nearest one */
        otmp = 0;
        for (coffers = fobj; coffers; coffers = coffers->nobj)
            if (coffers->otyp == CHEST) {
                if (coffers->spe == 2)
                    break; /* a throne room chest */
                if (!otmp
                    || (distu(coffers->ox, coffers->oy)
                        < distu(otmp->ox, otmp->oy)))
                    otmp = coffers; /* remember closest ordinary chest */
            }
        if (!coffers)
            coffers = otmp;

        if (coffers) {
            verbalize("Thank you for your contribution to reduce the debt.");
            freeinv(goldob);
            (void) add_to_container(coffers, goldob);
            coffers->owt = weight(coffers);
            coffers->cknown = 0;
            if (!coffers->olocked) {
                boxdummy = cg.zeroobj, boxdummy.otyp = SPE_WIZARD_LOCK;
                (void) boxlock(coffers, &boxdummy);
            }
        } else if (levl[x][y].looted != T_LOOTED
                   && (mon = makemon(courtmon(), x, y, NO_MM_FLAGS)) != 0) {
            freeinv(goldob);
            add_to_minv(mon, goldob);
            pline("The exchequer accepts your contribution.");
            if (!rn2(10))
                levl[x][y].looted = T_LOOTED;
        } else {
            You("drop %s.", doname(goldob));
            dropx(goldob);
        }
    }
    return TRUE;
}

/* loot_mon() returns amount of time passed.
 */
int
loot_mon(struct monst *mtmp, int *passed_info, boolean *prev_loot)
{
    int c = -1;
    int timepassed = 0;
    struct obj *otmp;
    char qbuf[QBUFSZ];

    /* 3.3.1 introduced the ability to remove saddle from a steed.
     *  *passed_info is set to TRUE if a loot query was given.
     *  *prev_loot is set to TRUE if something was actually acquired in here.
     */
    if (mtmp && mtmp != u.usteed && (otmp = which_armor(mtmp, W_SADDLE))) {
        if (passed_info)
            *passed_info = 1;
        Sprintf(qbuf, "Do you want to remove the saddle from %s?",
                x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                         SUPPRESS_SADDLE, FALSE));
        if ((c = yn_function(qbuf, ynqchars, 'n', TRUE)) == 'y') {
            if (nolimbs(gy.youmonst.data)) {
                You_cant("do that without limbs."); /* not body_part(HAND) */
                return 0;
            }
            if (otmp->cursed) {
                You("can't.  The saddle seems to be stuck to %s.",
                    x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                             SUPPRESS_SADDLE, FALSE));
                /* the attempt costs you time */
                return 1;
            }
            extract_from_minvent(mtmp, otmp, TRUE, FALSE);
            if (Verbose(2, loot_mon))
                You("take %s off of %s.",
                    thesimpleoname(otmp), mon_nam(mtmp));
            otmp = hold_another_object(otmp, "You drop %s!", doname(otmp),
                                       (const char *) 0);
            nhUse(otmp);
            timepassed = rnd(3);
            if (prev_loot)
                *prev_loot = TRUE;
        } else if (c == 'q') {
            return 0;
        }
    }
    /* 3.4.0 introduced ability to pick things up from swallower's stomach */
    if (u.uswallow) {
        int count = passed_info ? *passed_info : 0;

        timepassed = pickup(count);
    }
    return timepassed;
}

/*
 * Decide whether an object being placed into a magic bag will cause
 * it to explode.  If the object is a bag itself, check recursively.
 */
static boolean
mbag_explodes(struct obj *obj, int depthin)
{
    /* these won't cause an explosion when they're empty */
    if ((obj->otyp == WAN_CANCELLATION || obj->otyp == BAG_OF_TRICKS)
        && obj->spe <= 0)
        return FALSE;

    /* odds: 1/1, 2/2, 3/4, 4/8, 5/16, 6/32, 7/64, 8/128, 9/128, 10/128,... */
    if ((Is_mbag(obj) || obj->otyp == WAN_CANCELLATION)
        && (rn2(1 << (depthin > 7 ? 7 : depthin)) <= depthin))
        return TRUE;
    else if (Has_contents(obj)) {
        struct obj *otmp;

        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (mbag_explodes(otmp, depthin + 1))
                return TRUE;
    }
    return FALSE;
}

static boolean
is_boh_item_gone(void)
{
    return (boolean) (!rn2(13));
}

/* Scatter most of Bag of holding contents around.  Some items will be
   destroyed with the same chance as looting a cursed bag. */
static void
do_boh_explosion(struct obj *boh, boolean on_floor)
{
    struct obj *otmp, *nobj;

    boh->in_use = 1; /* in case scatter() leads to bones creation */
    for (otmp = boh->cobj; otmp; otmp = nobj) {
        nobj = otmp->nobj;
        if (is_boh_item_gone()) {
            obj_extract_self(otmp);
            mbag_item_gone(!on_floor, otmp, TRUE);
        } else {
            otmp->ox = u.ux, otmp->oy = u.uy;
            (void) scatter(u.ux, u.uy, 4, MAY_HIT | MAY_DESTROY, otmp);
        }
    }
    /* boh is about to be deleted so no need to reset its in_use flag here */
}

static long
boh_loss(struct obj *container, boolean held)
{
    /* sometimes toss objects if a cursed magic bag */
    if (Is_mbag(container) && container->cursed && Has_contents(container)) {
        long loss = 0L;
        struct obj *curr, *otmp;

        for (curr = container->cobj; curr; curr = otmp) {
            otmp = curr->nobj;
            if (is_boh_item_gone()) {
                obj_extract_self(curr);
                loss += mbag_item_gone(held, curr, FALSE);
            }
        }
        return loss;
    }
    return 0;
}

/* Returns: -1 to stop, 1 item was inserted, 0 item was not inserted. */
static int
in_container(struct obj *obj)
{
    boolean floor_container = !carried(gc.current_container);
    boolean was_unpaid = FALSE;
    char buf[BUFSZ];

    if (!gc.current_container) {
        impossible("<in> no gc.current_container?");
        return 0;
    } else if (obj == uball || obj == uchain) {
        You("must be kidding.");
        return 0;
    } else if (obj == gc.current_container) {
        pline("That would be an interesting topological exercise.");
        return 0;
    } else if (obj->owornmask & (W_ARMOR | W_ACCESSORY)) {
        Norep("You cannot %s %s you are wearing.",
              Icebox ? "refrigerate" : "stash", something);
        return 0;
    } else if ((obj->otyp == LOADSTONE) && obj->cursed) {
        set_bknown(obj, 1);
        pline_The("stone%s won't leave your person.", plur(obj->quan));
        return 0;
    } else if (obj->otyp == AMULET_OF_YENDOR
               || obj->otyp == CANDELABRUM_OF_INVOCATION
               || obj->otyp == BELL_OF_OPENING
               || obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        /* Prohibit Amulets in containers; if you allow it, monsters can't
         * steal them.  It also becomes a pain to check to see if someone
         * has the Amulet.  Ditto for the Candelabrum, the Bell and the Book.
         */
        pline("%s cannot be confined in such trappings.", The(xname(obj)));
        return 0;
    } else if (obj->otyp == LEASH && obj->leashmon != 0) {
        pline("%s attached to your pet.", Tobjnam(obj, "are"));
        return 0;
    } else if (obj == uwep) {
        if (welded(obj)) {
            weldmsg(obj);
            return 0;
        }
        setuwep((struct obj *) 0);
        /* This uwep check is obsolete.  It dates to 3.0 and earlier when
         * unwielding Firebrand would be fatal in hell if hero had no other
         * fire resistance.  Life-saving would force it to be re-wielded.
         */
        if (uwep)
            return 0; /* unwielded, died, rewielded */
    } else if (obj == uswapwep) {
        setuswapwep((struct obj *) 0);
    } else if (obj == uquiver) {
        setuqwep((struct obj *) 0);
    }

    if (fatal_corpse_mistake(obj, FALSE))
        return -1;

    /* boxes, boulders, and big statues can't fit into any container */
    if (obj->otyp == ICE_BOX || Is_box(obj) || obj->otyp == BOULDER
        || (obj->otyp == STATUE && bigmonst(&mons[obj->corpsenm]))) {
        /* consumes multiple obufs but not enough to overwrite the result */
        Strcpy(buf, the(xname(obj)));
        You("cannot fit %s into %s.", buf, the(xname(gc.current_container)));
        return 0;
    }

    freeinv(obj);

    if (obj_is_burning(obj)) /* this used to be part of freeinv() */
        (void) snuff_lit(obj);

    if (floor_container && costly_spot(u.ux, u.uy)) {
        /* defer gold until after put-in message */
        if (obj->oclass != COIN_CLASS) {
            /* sellobj() will take an unpaid item off the shop bill */
            was_unpaid = obj->unpaid ? TRUE : FALSE;
            /* don't sell when putting the item into your own container,
             * but handle billing correctly */
            sellobj_state(gc.current_container->no_charge
                          ? SELL_DONTSELL : SELL_DELIBERATE);
            sellobj(obj, u.ux, u.uy);
            sellobj_state(SELL_NORMAL);
        }
    }
    if (Icebox && !age_is_relative(obj)) {
        obj->age = gm.moves - obj->age; /* actual age */
        /* stop any corpse timeouts when frozen */
        if (obj->otyp == CORPSE) {
            if (obj->timed) {
                (void) stop_timer(ROT_CORPSE, obj_to_any(obj));
                (void) stop_timer(REVIVE_MON, obj_to_any(obj));
            }
            /* if this is the corpse of a cancelled ice troll, uncancel it */
            if (obj->corpsenm == PM_ICE_TROLL && has_omonst(obj))
                OMONST(obj)->mcan = 0;
        } else if (obj->globby && obj->timed) {
            (void) stop_timer(SHRINK_GLOB, obj_to_any(obj));
        }
    } else if (Is_mbag(gc.current_container) && mbag_explodes(obj, 0)) {
        livelog_printf(LL_ACHIEVE, "just blew up %s bag of holding", uhis());
        /* explicitly mention what item is triggering the explosion */
        urgent_pline(
              "As you put %s inside, you are blasted by a magical explosion!",
                     doname(obj));
        /* did not actually insert obj yet */
        if (was_unpaid)
            addtobill(obj, FALSE, FALSE, TRUE);
        if (obj->otyp == BAG_OF_HOLDING) /* one bag of holding into another */
            do_boh_explosion(obj, (boolean) (obj->where == OBJ_FLOOR));
        obfree(obj, (struct obj *) 0);
        /* if carried, shop goods will be flagged 'unpaid' and obfree() will
           handle bill issues, but if on floor, we need to put them on bill
           before deleting them (non-shop items will be flagged 'no_charge') */
        if (floor_container
            && costly_spot(gc.current_container->ox, gc.current_container->oy)) {
            struct obj save_no_charge;

            save_no_charge.no_charge = gc.current_container->no_charge;
            addtobill(gc.current_container, FALSE, FALSE, FALSE);
            /* addtobill() clears no charge; we need to set it back
               so that useupf() doesn't double bill */
            gc.current_container->no_charge = save_no_charge.no_charge;
        }
        do_boh_explosion(gc.current_container, floor_container);

        if (!floor_container)
            useup(gc.current_container);
        else if (obj_here(gc.current_container, u.ux, u.uy))
            useupf(gc.current_container, gc.current_container->quan);
        else
            panic("in_container:  bag not found.");

        losehp(d(6, 6), "magical explosion", KILLED_BY_AN);
        gc.current_container = 0; /* baggone = TRUE; */
    }

    if (gc.current_container) {
        Strcpy(buf, the(xname(gc.current_container)));
        You("put %s into %s.", doname(obj), buf);

        /* gold in container always needs to be added to credit */
        if (floor_container && obj->oclass == COIN_CLASS)
            sellobj(obj, gc.current_container->ox, gc.current_container->oy);
        (void) add_to_container(gc.current_container, obj);
        gc.current_container->owt = weight(gc.current_container);
    }
    /* gold needs this, and freeinv() many lines above may cause
     * the encumbrance to disappear from the status, so just always
     * update status immediately.
     */
    bot();
    return (gc.current_container ? 1 : -1);
}

/* askchain() filter used by in_container();
 * returns True if the container is intact and 'obj' isn't it, False if
 * container is gone (magic bag explosion) or 'obj' is the container itself;
 * also used by getobj() when picking a single item to stash
 */
int
ck_bag(struct obj *obj)
{
    return (gc.current_container && obj != gc.current_container);
}

/* Returns: -1 to stop, 1 item was removed, 0 item was not removed. */
static int
out_container(struct obj *obj)
{
    register struct obj *otmp;
    boolean is_gold = (obj->oclass == COIN_CLASS);
    int res, loadlev;
    long count;

    if (!gc.current_container) {
        impossible("<out> no gc.current_container?");
        return -1;
    } else if (is_gold) {
        obj->owt = weight(obj);
    }

    if (obj->oartifact && !touch_artifact(obj, &gy.youmonst))
        return 0;

    if (fatal_corpse_mistake(obj, FALSE))
        return -1;

    count = obj->quan;
    if ((res = lift_object(obj, gc.current_container, &count, FALSE)) <= 0)
        return res;

    if (obj->quan != count && obj->otyp != LOADSTONE)
        obj = splitobj(obj, count);

    /* Remove the object from the list. */
    obj_extract_self(obj);
    gc.current_container->owt = weight(gc.current_container);

    if (Icebox)
        removed_from_icebox(obj);

    if (!obj->unpaid && !carried(gc.current_container)
        && costly_spot(gc.current_container->ox, gc.current_container->oy)) {
        obj->ox = gc.current_container->ox;
        obj->oy = gc.current_container->oy;
        addtobill(obj, FALSE, FALSE, FALSE);
    }
    if (is_pick(obj))
        pick_pick(obj); /* shopkeeper feedback */

    otmp = addinv(obj);
    loadlev = near_capacity();
    prinv(loadlev ? ((loadlev < MOD_ENCUMBER)
                        ? "You have a little trouble removing"
                        : "You have much trouble removing")
                  : (char *) 0,
          otmp, count);

    if (is_gold) {
        bot(); /* update character's gold piece count immediately */
    }
    return 1;
}

/* taking a corpse out of an ice box needs a couple of adjustments */
void
removed_from_icebox(struct obj *obj)
{
    if (!age_is_relative(obj)) {
        obj->age = gm.moves - obj->age; /* actual age */
        if (obj->otyp == CORPSE) {
            struct monst *m = get_mtraits(obj, FALSE);
            boolean iceT = m ? (m->data == &mons[PM_ICE_TROLL])
                             : (obj->corpsenm == PM_ICE_TROLL);

            /* start a revive timer if this corpse is for an ice troll,
               otherwise start a rot-away timer (even for other trolls) */
            obj->norevive = iceT ? 0 : 1;
            start_corpse_timeout(obj);
        } else if (obj->globby) {
            /* non-frozen globs gradually shrink away to nothing */
            start_glob_timeout(obj, 0L);
        }
    }
}

/* an object inside a cursed bag of holding is being destroyed */
static long
mbag_item_gone(boolean held, struct obj *item, boolean silent)
{
    struct monst *shkp;
    long loss = 0L;

    if (!silent) {
        if (item->dknown)
            pline("%s %s vanished!", Doname2(item), otense(item, "have"));
        else
            You("%s %s disappear!", Blind ? "notice" : "see", doname(item));
    }

    if (*u.ushops && (shkp = shop_keeper(*u.ushops)) != 0) {
        if (held ? (boolean) item->unpaid : costly_spot(u.ux, u.uy))
            loss = stolen_value(item, u.ux, u.uy, (boolean) shkp->mpeaceful,
                                TRUE);
    }
    obfree(item, (struct obj *) 0);
    return loss;
}

/* used for #loot/apply, #tip, and final disclosure */
void
observe_quantum_cat(struct obj *box, boolean makecat, boolean givemsg)
{
    static NEARDATA const char sc[] = "Schroedinger's Cat";
    struct obj *deadcat;
    struct monst *livecat = 0;
    coordxy ox, oy;
    boolean itsalive = !rn2(2);

    if (get_obj_location(box, &ox, &oy, 0))
        box->ox = ox, box->oy = oy; /* in case it's being carried */

    /* this isn't really right, since any form of observation
       (telepathic or monster/object/food detection) ought to
       force the determination of alive vs dead state; but basing it
       just on opening or disclosing the box is much simpler to cope with */

    /* SchroedingersBox already has a cat corpse in it */
    deadcat = box->cobj;
    if (itsalive) {
        if (makecat)
            livecat = makemon(&mons[PM_HOUSECAT], box->ox, box->oy,
                              NO_MINVENT | MM_ADJACENTOK | MM_NOMSG);
        if (livecat) {
            livecat->mpeaceful = 1;
            set_malign(livecat);
            if (givemsg) {
                if (!canspotmon(livecat))
                    You("think %s brushed your %s.", something,
                        body_part(FOOT));
                else
                    pline("%s inside the box is still alive!",
                          Monnam(livecat));
            }
            (void) christen_monst(livecat, sc);
            if (deadcat) {
                obj_extract_self(deadcat);
                obfree(deadcat, (struct obj *) 0), deadcat = 0;
            }
            box->owt = weight(box);
            box->spe = 0;
        }
    } else {
        box->spe = 0; /* now an ordinary box (with a cat corpse inside) */
        if (deadcat) {
            /* set_corpsenm() will start the rot timer that was removed
               when makemon() created SchroedingersBox; start it from
               now rather than from when this special corpse got created */
            deadcat->age = gm.moves;
            set_corpsenm(deadcat, PM_HOUSECAT);
            deadcat = oname(deadcat, sc, ONAME_NO_FLAGS);
        }
        if (givemsg)
            pline_The("%s inside the box is dead!",
                      Hallucination ? rndmonnam((char *) 0) : "housecat");
    }
    nhUse(deadcat);
    return;
}

#undef Icebox

/* used by askchain() to check for magic bag explosion */
boolean
container_gone(int (*fn)(OBJ_P))
{
    /* result is only meaningful while use_container() is executing */
    return ((fn == in_container || fn == out_container)
            && !gc.current_container);
}

static void
explain_container_prompt(boolean more_containers)
{
    static const char *const explaintext[] = {
        "Container actions:",
        "",
        " : -- Look: examine contents",
        " o -- Out: take things out",
        " i -- In: put things in",
        " b -- Both: first take things out, then put things in",
        " r -- Reversed: put things in, then take things out",
        " s -- Stash: put one item in", "",
        " n -- Next: loot next selected container",
        " q -- Quit: finished",
        " ? -- Help: display this text.",
        "", 0
    };
    const char *const *txtpp;
    winid win;

    /* "Do what with <container>? [:oibrsq or ?] (q)" */
    if ((win = create_nhwindow(NHW_TEXT)) != WIN_ERR) {
        for (txtpp = explaintext; *txtpp; ++txtpp) {
            if (!more_containers && !strncmp(*txtpp, " n ", 3))
                continue;
            putstr(win, 0, *txtpp);
        }
        display_nhwindow(win, FALSE);
        destroy_nhwindow(win);
    }
}

boolean
u_handsy(void)
{
    if (nohands(gy.youmonst.data)) {
        You("have no hands!"); /* not `body_part(HAND)' */
        return FALSE;
    } else if (!freehand()) {
        You("have no free %s.", body_part(HAND));
        return FALSE;
    }
    return TRUE;
}

/* getobj callback for object to be stashed into a container */
static int
stash_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    /* downplay the container being stashed into */
    if (!ck_bag(obj))
        return GETOBJ_EXCLUDE_SELECTABLE;
    /* Possible extension: downplay things too big to fit into containers (in
     * which case extract in_container()'s logic.) */

    return GETOBJ_SUGGEST;
}

int
use_container(
    struct obj **objp,
    boolean held,
    boolean more_containers) /* True iff #loot multiple and this isn't last */
{
    struct obj *otmp, *obj = *objp;
    boolean quantum_cat, cursed_mbag, loot_out, loot_in, loot_in_first,
        stash_one, inokay, outokay, outmaybe;
    char c, emptymsg[BUFSZ], qbuf[QBUFSZ], pbuf[QBUFSZ], xbuf[QBUFSZ];
    int used = ECMD_OK;
    long loss;

    ga.abort_looting = FALSE;
    emptymsg[0] = '\0';

    if (!u_handsy())
        return ECMD_OK;

    if (!obj->lknown) { /* do this in advance */
        obj->lknown = 1;
        if (held)
            update_inventory();
    }
    if (obj->olocked) {
        pline("%s locked.", Tobjnam(obj, "are"));
        if (held)
            You("must put it down to unlock.");
        return ECMD_OK;
    } else if (obj->otrapped) {
        if (held)
            You("open %s...", the(xname(obj)));
        (void) chest_trap(obj, HAND, FALSE);
        /* even if the trap fails, you've used up this turn */
        if (gm.multi >= 0) { /* in case we didn't become paralyzed */
            nomul(-1);
            gm.multi_reason = "opening a container";
            gn.nomovemsg = "";
        }
        ga.abort_looting = TRUE;
        return ECMD_TIME;
    }

    gc.current_container = obj; /* for use by in/out_container */
    /*
     * From here on out, all early returns go through 'containerdone:'.
     */

    /* check for Schroedinger's Cat */
    quantum_cat = SchroedingersBox(gc.current_container);
    if (quantum_cat) {
        observe_quantum_cat(gc.current_container, TRUE, TRUE);
        used = ECMD_TIME;
    }

    cursed_mbag = Is_mbag(gc.current_container)
        && gc.current_container->cursed
        && Has_contents(gc.current_container);
    if (cursed_mbag
        && (loss = boh_loss(gc.current_container, held)) != 0) {
        used = ECMD_TIME;
        You("owe %ld %s for lost merchandise.", loss, currency(loss));
        gc.current_container->owt = weight(gc.current_container);
    }
    /* might put something in if carring anything other than just the
       container itself (invent is not the container or has a next object) */
    inokay = (gi.invent != 0 && (gi.invent != gc.current_container
                                || gi.invent->nobj));
    /* might take something out if container isn't empty */
    outokay = Has_contents(gc.current_container);
    if (!outokay) /* preformat the empty-container message */
        Sprintf(emptymsg, "%s is %sempty.",
                Ysimple_name2(gc.current_container),
                (quantum_cat || cursed_mbag) ? "now " : "");

    /*
     * What-to-do prompt's list of possible actions:
     * always include the look-inside choice (':');
     * include the take-out choice ('o') if container
     * has anything in it or if player doesn't yet know
     * that it's empty (latter can change on subsequent
     * iterations if player picks ':' response);
     * include the put-in choices ('i','s') if hero
     * carries any inventory (including gold) aside from
     * the container itself;
     * include do-both when 'o' is available, even if
     * inventory is empty--taking out could alter that;
     * include do-both-reversed when 'i' is available,
     * even if container is empty--for similar reason;
     * include the next container choice ('n') when
     * relevant, and make it the default;
     * always include the quit choice ('q'), and make
     * it the default if there's no next containter;
     * include the help choice (" or ?") if `cmdassist'
     * run-time option is set;
     * (Player can pick any of (o,i,b,r,n,s,?) even when
     * they're not listed among the available actions.)
     *
     * Do what with <the/your/Shk's container>? [:oibrs nq or ?] (q)
     * or
     * <The/Your/Shk's container> is empty.  Do what with it? [:irs nq or ?]
     */
    for (;;) { /* repeats iff '?' or ':' gets chosen */
        outmaybe = (outokay || !gc.current_container->cknown);
        if (!outmaybe)
            (void) safe_qbuf(qbuf, (char *) 0, " is empty.  Do what with it?",
                             gc.current_container, Yname2, Ysimple_name2,
                             "This");
        else
            (void) safe_qbuf(qbuf, "Do what with ", "?", gc.current_container,
                             yname, ysimple_name, "it");
        /* ask player about what to do with this container */
        if (flags.menu_style == MENU_PARTIAL
            || flags.menu_style == MENU_FULL) {
            if (!inokay && !outmaybe) {
                /* nothing to take out, nothing to put in;
                   trying to do both will yield proper feedback */
                c = 'b';
            } else {
                c = in_or_out_menu(qbuf, gc.current_container,
                                   outmaybe, inokay,
                                   (boolean) (used != ECMD_OK),
                                   more_containers);
            }
        } else { /* TRADITIONAL or COMBINATION */
            xbuf[0] = '\0'; /* list of extra acceptable responses */
            Strcpy(pbuf, ":");                   /* look inside */
            Strcat(outmaybe ? pbuf : xbuf, "o"); /* take out */
            Strcat(inokay ? pbuf : xbuf, "i");   /* put in */
            Strcat(outmaybe ? pbuf : xbuf, "b"); /* both */
            Strcat(inokay ? pbuf : xbuf, "rs");  /* reversed, stash */
            Strcat(pbuf, " ");                   /* separator */
            Strcat(more_containers ? pbuf : xbuf, "n"); /* next container */
            Strcat(pbuf, "q");                   /* quit */
            if (iflags.cmdassist)
                /* this unintentionally allows user to answer with 'o' or
                   'r'; fortunately, those are already valid choices here */
                Strcat(pbuf, " or ?"); /* help */
            else
                Strcat(xbuf, "?");
            if (*xbuf)
                Strcat(strcat(pbuf, "\033"), xbuf);
            c = yn_function(qbuf, pbuf, more_containers ? 'n' : 'q', TRUE);
        } /* PARTIAL|FULL vs other modes */

        if (c == '?') {
            explain_container_prompt(more_containers);
        } else if (c == ':') { /* note: will set obj->cknown */
            if (!gc.current_container->cknown)
                used = ECMD_TIME; /* gaining info */
            container_contents(gc.current_container, FALSE, FALSE, TRUE);
        } else
            break;
    } /* loop until something other than '?' or ':' is picked */

    if (c == 'q')
        ga.abort_looting = TRUE;
    if (c == 'n' || c == 'q') /* [not strictly needed; falling thru works] */
        goto containerdone;
    loot_out = (c == 'o' || c == 'b' || c == 'r');
    loot_in = (c == 'i' || c == 'b' || c == 'r');
    loot_in_first = (c == 'r'); /* both, reversed */
    stash_one = (c == 's');

    /* out-only or out before in */
    if (loot_out && !loot_in_first) {
        if (!Has_contents(gc.current_container)) {
            pline1(emptymsg); /* <whatever> is empty. */
            if (!gc.current_container->cknown)
                used = ECMD_TIME;
            gc.current_container->cknown = 1;
        } else {
            add_valid_menu_class(0); /* reset */
            if (flags.menu_style == MENU_TRADITIONAL)
                used |= traditional_loot(FALSE);
            else
                used |= (menu_loot(0, FALSE) > 0);
            add_valid_menu_class(0);
        }
        /* recalculate 'inokay' in case something was just taken out and
           inventory is no longer empty or no longer just the container */
        inokay = (gi.invent && (gi.invent != gc.current_container
                               || gi.invent->nobj));
    }

    if ((loot_in || stash_one) && !inokay) {
        You("don't have anything%s to %s.", gi.invent ? " else" : "",
            stash_one ? "stash" : "put in");
        loot_in = stash_one = FALSE;
    }

    /*
     * Gone: being nice about only selecting food if we know we are
     * putting things in an ice chest.
     */
    if (loot_in) {
        add_valid_menu_class(0); /* reset */
        if (flags.menu_style == MENU_TRADITIONAL)
            used |= traditional_loot(TRUE);
        else
            used |= (menu_loot(0, TRUE) > 0);
        add_valid_menu_class(0);
    } else if (stash_one) {
        /* put one item into container */
        if ((otmp = getobj("stash", stash_ok,
                           GETOBJ_PROMPT | GETOBJ_ALLOWCNT)) != 0) {
            if (in_container(otmp)) {
                used = 1;
            } else {
                /* couldn't put selected item into container for some
                   reason; might need to undo splitobj() */
                (void) unsplitobj(otmp);
            }
        }
    }
    /* putting something in might have triggered magic bag explosion */
    if (!gc.current_container)
        loot_out = FALSE;

    /* out after in */
    if (loot_out && loot_in_first) {
        if (!Has_contents(gc.current_container)) {
            pline1(emptymsg); /* <whatever> is empty. */
            if (!gc.current_container->cknown)
                used = 1;
            gc.current_container->cknown = 1;
        } else {
            add_valid_menu_class(0); /* reset */
            if (flags.menu_style == MENU_TRADITIONAL)
                used |= traditional_loot(FALSE);
            else
                used |= (menu_loot(0, FALSE) > 0);
            add_valid_menu_class(0);
        }
    }

 containerdone:
    if (used) {
        /* Not completely correct; if we put something in without knowing
           whatever was already inside, now we suddenly do.  That can't
           be helped unless we want to track things item by item and then
           deal with containers whose contents are "partly known". */
        if (gc.current_container)
            gc.current_container->cknown = 1;
        update_inventory();
    }

    *objp = gc.current_container; /* might have become null */
    if (gc.current_container)
        gc.current_container = 0; /* avoid hanging on to stale pointer */
    else
        ga.abort_looting = TRUE;
    return used;
}

/* loot current_container (take things out or put things in), by prompting */
static int
traditional_loot(boolean put_in)
{
    int (*actionfunc)(OBJ_P), (*checkfunc)(OBJ_P);
    struct obj **objlist;
    char selection[MAXOCLASSES + 10]; /* +10: room for B,U,C,X plus slop */
    const char *action;
    boolean one_by_one, allflag;
    int used = ECMD_OK, menu_on_request = 0;

    if (put_in) {
        action = "put in";
        objlist = &gi.invent;
        actionfunc = in_container;
        checkfunc = ck_bag;
    } else {
        action = "take out";
        objlist = &(gc.current_container->cobj);
        actionfunc = out_container;
        checkfunc = (int (*)(OBJ_P)) 0;
    }

    if (query_classes(selection, &one_by_one, &allflag, action, *objlist,
                      FALSE, &menu_on_request)) {
        if (askchain(objlist, (one_by_one ? (char *) 0 : selection), allflag,
                     actionfunc, checkfunc, 0, action))
            used = ECMD_TIME;
    } else if (menu_on_request < 0) {
        used = (menu_loot(menu_on_request, put_in) > 0);
    }
    return used;
}

/* loot current_container (take things out or put things in), using a menu */
static int
menu_loot(int retry, boolean put_in)
{
    int n, i, n_looted = 0;
    boolean all_categories = TRUE, loot_everything = FALSE, autopick = FALSE;
    char buf[BUFSZ];
    boolean loot_justpicked = FALSE;
    const char *action = put_in ? "Put in" : "Take out";
    struct obj *otmp, *otmp2;
    menu_item *pick_list;
    int mflags, res;
    long count = 0;

    if (retry) {
        all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
        all_categories = FALSE;
        Sprintf(buf, "%s what type of objects?", action);
        mflags = (ALL_TYPES | UNPAID_TYPES | BUCX_TYPES | CHOOSE_ALL
                  | JUSTPICKED );
        n = query_category(buf, put_in ? gi.invent : gc.current_container->cobj,
                           mflags, &pick_list, PICK_ANY);
        if (!n)
            return ECMD_OK;
        for (i = 0; i < n; i++) {
            if (pick_list[i].item.a_int == 'A') {
                loot_everything = autopick = TRUE;
            } else if (put_in && pick_list[i].item.a_int == 'P') {
                loot_justpicked = TRUE;
                count = max(0, pick_list[i].count);
                add_valid_menu_class(pick_list[i].item.a_int);
                loot_everything = FALSE;
            } else if (pick_list[i].item.a_int == ALL_TYPES_SELECTED) {
                all_categories = TRUE;
            } else {
                add_valid_menu_class(pick_list[i].item.a_int);
                loot_everything = FALSE;
            }
        }
        free((genericptr_t) pick_list);
    }

    if (autopick) {
        int (*inout_func)(struct obj *); /* in_container or out_container */
        struct obj *firstobj;

        if (!put_in) {
            gc.current_container->cknown = 1;
            inout_func = out_container;
            firstobj = gc.current_container->cobj;
        } else {
            inout_func = in_container;
            firstobj = gi.invent;
        }
        /*
         * Note:  for put_in, current_container might be destroyed during
         * mid-traversal by a magic bag explosion.
         * Note too:  items are processed in internal list order rather
         * than menu display order ('sortpack') or 'sortloot' order;
         * for put_in that should be item->invlet order so reasonable.
         */
        for (otmp = firstobj; otmp && gc.current_container; otmp = otmp2) {
            otmp2 = otmp->nobj;
            if (loot_everything || all_categories || allow_category(otmp)) {
                res = (*inout_func)(otmp);
                if (res < 0)
                    break;
                n_looted += res;
            }
        }
    } else if (put_in && loot_justpicked && count_justpicked(gi.invent) == 1) {
        otmp = find_justpicked(gi.invent);
        if (otmp) {
            n_looted = 1;
            if (count > 0 && count < otmp->quan) {
                otmp = splitobj(otmp, count);
            }
            (void) in_container(otmp);
            /* return value doesn't matter, even if container blew up */
        }
    } else {
        mflags = INVORDER_SORT | INCLUDE_VENOM;
        if (put_in && flags.invlet_constant)
            mflags |= USE_INVLET;
        if (put_in && loot_justpicked)
            mflags |= JUSTPICKED;
        if (!put_in)
            gc.current_container->cknown = 1;
        Sprintf(buf, "%s what?", action);
        n = query_objlist(buf,
                          put_in ? &gi.invent : &(gc.current_container->cobj),
                          mflags, &pick_list, PICK_ANY,
                          all_categories ? allow_all : allow_category);
        if (n) {
            n_looted = n;
            for (i = 0; i < n; i++) {
                otmp = pick_list[i].item.a_obj;
                count = pick_list[i].count;
                if (count > 0 && count < otmp->quan) {
                    otmp = splitobj(otmp, count);
                    /* special split case also handled by askchain() */
                }
                res = put_in ? in_container(otmp) : out_container(otmp);
                if (res <= 0) {
                    if (!gc.current_container) {
                        /* otmp caused current_container to explode;
                           both are now gone */
                        otmp = 0; /* and break loop */
                    } else if (otmp && otmp != pick_list[i].item.a_obj) {
                        /* split occurred, merge again */
                        (void) unsplitobj(otmp);
                    }
                    if (res < 0)
                        break;
                }
            }
            free((genericptr_t) pick_list);
        }
    }
    return n_looted ? ECMD_TIME : ECMD_OK;
}

static char
in_or_out_menu(
    const char *prompt,
    struct obj *obj,
    boolean outokay,     /* can take out */
    boolean inokay,      /* can put in */
    boolean alreadyused, /* controls phrasing of the decline choice */
    boolean more_containers)
{
    /* underscore is not a choice; it's used to skip element [0] */
    static const char lootchars[] = "_:oibrsnq", abc_chars[] = "_:abcdenq";
    winid win;
    anything any;
    menu_item *pick_list;
    char buf[BUFSZ];
    int n;
    const char *menuselector = flags.lootabc ? abc_chars : lootchars;
    int clr = 0;

    any = cg.zeroany;
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);

    any.a_int = 1; /* ':' */
    Sprintf(buf, "Look inside %s", thesimpleoname(obj));
    add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
             ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    if (outokay) {
        any.a_int = 2; /* 'o' */
        Sprintf(buf, "take %s out", something);
        add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    }
    if (inokay) {
        any.a_int = 3; /* 'i' */
        Sprintf(buf, "put %s in", something);
        add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    }
    if (outokay) {
        any.a_int = 4; /* 'b' */
        Sprintf(buf, "%stake out, then put in", inokay ? "both; " : "");
        add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    }
    if (inokay) {
        any.a_int = 5; /* 'r' */
        Sprintf(buf, "%sput in, then take out",
                outokay ? "both reversed; " : "");
        add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
        any.a_int = 6; /* 's' */
        Sprintf(buf, "stash one item into %s", thesimpleoname(obj));
        add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    }
    any.a_int = 0;
    add_menu(win, &nul_glyphinfo, &any, 0, 0,
             ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
    if (more_containers) {
        any.a_int = 7; /* 'n' */
        add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
                 ATR_NONE, clr, "loot next container", MENU_ITEMFLAGS_SELECTED);
    }
    any.a_int = 8; /* 'q' */
    Strcpy(buf, alreadyused ? "done" : "do nothing");
    add_menu(win, &nul_glyphinfo, &any, menuselector[any.a_int], 0,
             ATR_NONE, clr, buf,
             more_containers ? MENU_ITEMFLAGS_NONE : MENU_ITEMFLAGS_SELECTED);

    end_menu(win, prompt);
    n = select_menu(win, PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
        int k = pick_list[0].item.a_int;

        if (n > 1 && k == (more_containers ? 7 : 8))
            k = pick_list[1].item.a_int;
        free((genericptr_t) pick_list);
        return lootchars[k]; /* :,o,i,b,r,s,n,q */
    }
    return (n == 0 && more_containers) ? 'n' : 'q'; /* next or quit */
}

/* getobj callback for object to tip */
static int
tip_ok(struct obj *obj)
{
    if (!obj || obj->oclass == COIN_CLASS)
        return GETOBJ_EXCLUDE;

    if (Is_container(obj)) {
        return GETOBJ_SUGGEST;
    }

    /* include horn of plenty if sufficiently discovered */
    if (obj->otyp == HORN_OF_PLENTY && obj->dknown
        && objects[obj->otyp].oc_name_known)
        return GETOBJ_SUGGEST;

    /* allow trying anything else in inventory */
    return GETOBJ_DOWNPLAY;
}

/* #tip command -- empty container contents onto floor */
int
dotip(void)
{
    struct obj *cobj, *nobj;
    coord cc;
    int boxes;
    char c, buf[BUFSZ], qbuf[BUFSZ];
    const char *spillage = 0;

    /*
     * doesn't require free hands;
     * limbs are needed to tip floor containers
     */

    /* at present, can only tip things at current spot, not adjacent ones */
    cc.x = u.ux, cc.y = u.uy;

    /* check floor container(s) first; at most one will be accessed */
    if ((boxes = container_at(cc.x, cc.y, TRUE)) > 0) {
        Sprintf(buf, "You can't tip %s while carrying so much.",
                !Verbose(2, dotip)
                    ? "a container" : (boxes > 1) ? "one" : "it");
        if (!check_capacity(buf) && able_to_loot(cc.x, cc.y, FALSE)) {
            if (boxes > 1 && (flags.menu_style != MENU_TRADITIONAL
                              || iflags.menu_requested)) {
                /* use menu to pick a container to tip */
                int n, i;
                winid win;
                anything any;
                menu_item *pick_list = (menu_item *) 0;
                struct obj dummyobj, *otmp;
                int clr = 0;

                any = cg.zeroany;
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);

                for (cobj = gl.level.objects[cc.x][cc.y], i = 0; cobj;
                     cobj = cobj->nexthere)
                    if (Is_container(cobj)) {
                        ++i;
                        any.a_obj = cobj;
                        add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                                 clr, doname(cobj), MENU_ITEMFLAGS_NONE);
                    }
                if (gi.invent) {
                    any = cg.zeroany;
                    add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                             clr, "", MENU_ITEMFLAGS_NONE);
                    any.a_obj = &dummyobj;
                    /* use 'i' for inventory unless there are so many
                       containers that it's already being used */
                    i = (i <= 'i' - 'a' && !flags.lootabc) ? 'i' : 0;
                    add_menu(win, &nul_glyphinfo, &any, i, 0, ATR_NONE,
                             clr, "tip something being carried",
                             MENU_ITEMFLAGS_SELECTED);
                }
                end_menu(win, "Tip which container?");
                n = select_menu(win, PICK_ONE, &pick_list);
                destroy_nhwindow(win);
                /*
                 * Deal with quirk of preselected item in pick-one menu:
                 * n ==  0 => picked preselected entry, toggling it off;
                 * n ==  1 => accepted preselected choice via SPACE or RETURN;
                 * n ==  2 => picked something other than preselected entry;
                 * n == -1 => cancelled via ESC;
                 */
                otmp = (n <= 0) ? (struct obj *) 0 : pick_list[0].item.a_obj;
                if (n > 1 && otmp == &dummyobj)
                    otmp = pick_list[1].item.a_obj;
                if (pick_list)
                    free((genericptr_t) pick_list);
                if (otmp && otmp != &dummyobj) {
                    tipcontainer(otmp);
                    return ECMD_TIME;
                }
                if (n == -1)
                    return ECMD_OK;
                /* else pick-from-gi.invent below */
            } else {
                for (cobj = gl.level.objects[cc.x][cc.y]; cobj; cobj = nobj) {
                    nobj = cobj->nexthere;
                    if (!Is_container(cobj))
                        continue;
                    c = ynq(safe_qbuf(qbuf, "There is ", " here, tip it?",
                                      cobj,
                                      doname, ansimpleoname, "container"));
                    if (c == 'q')
                        return ECMD_OK;
                    if (c == 'n')
                        continue;
                    tipcontainer(cobj);
                    /* can only tip one container at a time */
                    return ECMD_TIME;
                }
            }
        }
    }

    /* either no floor container(s) or couldn't tip one or didn't tip any */
    cobj = getobj("tip", tip_ok, GETOBJ_PROMPT);
    if (!cobj)
        return ECMD_CANCEL;

    /* normal case */
    if (Is_container(cobj) || cobj->otyp == HORN_OF_PLENTY) {
        tipcontainer(cobj);
        return ECMD_TIME;
    }
    /* assorted other cases */
    if (Is_candle(cobj) && cobj->lamplit) {
        /* note "wax" even for tallow candles to avoid giving away info */
        spillage = "wax";
    } else if ((cobj->otyp == POT_OIL && cobj->lamplit)
               || (cobj->otyp == OIL_LAMP && cobj->age != 0L)
               || (cobj->otyp == MAGIC_LAMP && cobj->spe != 0)) {
        spillage = "oil";
        /* todo: reduce potion's remaining burn timer or oil lamp's fuel */
    } else if (cobj->otyp == CAN_OF_GREASE && cobj->spe > 0) {
        /* charged consumed below */
        spillage = "grease";
    } else if (cobj->otyp == FOOD_RATION || cobj->otyp == CRAM_RATION
               || cobj->otyp == LEMBAS_WAFER) {
        spillage = "crumbs";
    } else if (cobj->oclass == VENOM_CLASS) {
        spillage = "venom";
    }
    if (spillage) {
        buf[0] = '\0';
        if (is_pool(u.ux, u.uy))
            Sprintf(buf, " and gradually %s", vtense(spillage, "dissipate"));
        else if (is_lava(u.ux, u.uy))
            Sprintf(buf, " and immediately %s away",
                    vtense(spillage, "burn"));
        pline("Some %s %s onto the %s%s.", spillage,
              vtense(spillage, "spill"), surface(u.ux, u.uy), buf);
        /* shop usage message comes after the spill message */
        if (cobj->otyp == CAN_OF_GREASE && cobj->spe > 0) {
            consume_obj_charge(cobj, TRUE);
        }
        /* something [useless] happened */
        return ECMD_TIME;
    }
    /* anything not covered yet */
    if (cobj->oclass == POTION_CLASS) /* can't pour potions... */
        pline_The("%s %s securely sealed.", xname(cobj), otense(cobj, "are"));
    else if (uarmh && cobj == uarmh)
        return tiphat() ? ECMD_TIME : ECMD_OK;
    else if (cobj->otyp == STATUE)
        pline("Nothing interesting happens.");
    else
        pline1(nothing_happens);
    return ECMD_OK;
}

enum tipping_check_values {
    TIPCHECK_OK = 0,
    TIPCHECK_LOCKED,
    TIPCHECK_TRAPPED,
    TIPCHECK_CANNOT,
    TIPCHECK_EMPTY
};

static void
tipcontainer(struct obj *box) /* or bag */
{
    coordxy ox = u.ux, oy = u.uy; /* #tip only works at hero's location */
    boolean srcheld = FALSE, dstheld = FALSE, maybeshopgoods;
    struct obj *targetbox = (struct obj *) 0;
    boolean cancelled = FALSE;

    /* box is either held or on floor at hero's spot; no need to check for
       nesting; when held, we need to update its location to match hero's;
       for floor, the coordinate updating is redundant */
    if (get_obj_location(box, &ox, &oy, 0))
        box->ox = ox, box->oy = oy;

    /*
     * TODO?
     *  if 'box' is known to be empty or known to be locked, give up
     *  before choosing 'targetbox'.
     */
    targetbox = tipcontainer_gettarget(box, &cancelled);
    if (cancelled)
        return;

    /* Shop handling:  can't rely on the container's own unpaid
       or no_charge status because contents might differ with it.
       A carried container's contents will be flagged as unpaid
       or not, as appropriate, and need no special handling here.
       Items owned by the hero get sold to the shop without
       confirmation as with other uncontrolled drops.  A floor
       container's contents will be marked no_charge if owned by
       hero, otherwise they're owned by the shop.  By passing
       the contents through shop billing, they end up getting
       treated the same as in the carried case.   We do so one
       item at a time instead of doing whole container at once
       to reduce the chance of exhausting shk's billing capacity. */
    maybeshopgoods = !carried(box) && costly_spot(box->ox, box->oy);

    if (tipcontainer_checks(box, targetbox, FALSE) != TIPCHECK_OK)
        return;
    if (targetbox && tipcontainer_checks(targetbox, NULL, TRUE) != TIPCHECK_OK)
        return;

    {
        struct obj *otmp, *nobj;
        boolean terse, highdrop = !can_reach_floor(TRUE),
                altarizing = IS_ALTAR(levl[ox][oy].typ),
                cursed_mbag = (Is_mbag(box) && box->cursed);
        long loss = 0L;

        srcheld = carried(box);
        dstheld = (targetbox && carried(targetbox));
        if (u.uswallow)
            highdrop = altarizing = FALSE;
        terse = !(highdrop || altarizing || costly_spot(box->ox, box->oy));
        box->cknown = 1;
        /* Terse formatting is
         * "Objects spill out: obj1, obj2, obj3, ..., objN."
         * If any other messages intervene between objects, we revert to
         * "ObjK drops to the floor.", "ObjL drops to the floor.", &c.
         */
        if (targetbox)
            pline("%s into %s.",
                  box->cobj->nobj ? "Objects tumble" : "An object tumbles",
                  the(xname(targetbox)));
        else
            pline("%s out%c",
              box->cobj->nobj ? "Objects spill" : "An object spills",
              terse ? ':' : '.');

        for (otmp = box->cobj; otmp; otmp = nobj) {
            nobj = otmp->nobj;
            obj_extract_self(otmp);
            otmp->ox = box->ox, otmp->oy = box->oy;

            if (box->otyp == ICE_BOX) {
                removed_from_icebox(otmp); /* resume rotting for corpse */
            } else if (cursed_mbag && is_boh_item_gone()) {
                loss += mbag_item_gone(srcheld, otmp, FALSE);
                /* abbreviated drop format is no longer appropriate */
                terse = FALSE;
                continue;
            }
            if (maybeshopgoods) {
                addtobill(otmp, FALSE, FALSE, TRUE);
                iflags.suppress_price++; /* doname formatting */
            }

            if (targetbox) {
                if (Is_mbag(targetbox) && mbag_explodes(otmp, 0)) {
                    livelog_printf(LL_ACHIEVE,
                                 "just blew up %s bag of holding via tipping",
                                   uhis());
                    /* explicitly mention what item is triggering explosion */
                    urgent_pline(
                   "As %s %s inside, you are blasted by a magical explosion!",
                                 doname(otmp), otense(otmp, "tumble"));

                    /* if putting one bag of holding into another, first
                       blow up the one going in, then (below) blow up the
                       one it's going into */
                    if (otmp->otyp == BAG_OF_HOLDING) /* BoH into another */
                        do_boh_explosion(otmp, !srcheld);
                    /* always delete the item which triggered the explosion */
                    obfree(otmp, (struct obj *) 0); /* where==OBJ_FREE */

                    /* [assumes targetbox is carried, otherwise shop bill
                       handling becomes necessary here] */
                    do_boh_explosion(targetbox, !dstheld);
                    if (dstheld)
                        useup(targetbox);
                    else
                        useupf(targetbox, targetbox->quan);
                    targetbox = 0; /* it's gone */
                    nobj = 0; /* stop tipping; want loop to exit 'normally' */

                    losehp(d(6, 6), "magical explosion", KILLED_BY_AN);
                } else {
                    (void) add_to_container(targetbox, otmp);
                }
            } else if (highdrop) {
                /* might break or fall down stairs; handles altars itself */
                hitfloor(otmp, TRUE);
            } else {
                if (altarizing) {
                    doaltarobj(otmp);
                } else if (!terse) {
                    pline("%s %s to the %s.", Doname2(otmp),
                          otense(otmp, "drop"), surface(ox, oy));
                } else {
                    pline("%s%c", doname(otmp), nobj ? ',' : '.');
                    iflags.last_msg = PLNMSG_OBJNAM_ONLY;
                }
                dropy(otmp);
                if (iflags.last_msg != PLNMSG_OBJNAM_ONLY)
                    terse = FALSE; /* terse formatting has been interrupted */
            }

            if (maybeshopgoods)
                iflags.suppress_price--; /* reset */
        }
        if (loss) /* magic bag lost some shop goods */
            You("owe %ld %s for lost merchandise.", loss, currency(loss));
        box->owt = weight(box); /* mbag_item_gone() doesn't update this */
        if (targetbox)
            targetbox->owt = weight(targetbox);
        if (srcheld || dstheld)
            (void) encumber_msg();
    }

    if (srcheld || dstheld)
        update_inventory();
}

#if 0
static int count_target_containers(struct obj *, struct obj *);

/* returns number of containers in object chain; does not recurse into
   containers; skips bags of tricks when they're known */
static int
count_target_containers(
    struct obj *olist,   /* list of objects (invent) */
    struct obj *excludo) /* particular object to exclude if found in list */
{
    int ret = 0;

    while (olist) {
        if (olist != excludo && Is_container(olist)
            /* include bag of tricks when not known to be such */
            && (box->otyp != BAG_OF_TRICKS || !box->dknown
                || !objects[box->otyp].oc_name_known))
            ret++;
        olist = olist->nobj;
    }
    return ret;
}
#endif

/* ask user for a carried container into which they want box to be emptied;
   cancelled is TRUE if user cancelled the menu pick; hands aren't required
   when tipping to the floor but are when tipping into another container */
static struct obj *
tipcontainer_gettarget(
    struct obj *box,
    boolean *cancelled)
{
    int n, n_conts;
    winid win;
    anything any;
    char buf[BUFSZ];
    menu_item *pick_list = (menu_item *) 0;
    struct obj dummyobj, *otmp;
    boolean hands_available = TRUE, exclude_it;
    int clr = 0;

#if 0   /* [skip potential early return so that menu response is needed
         *  regardless of whether other containers are being carried] */
    int n_conts = count_target_containers(gi.invent, box);

    if (n_conts < 1 || !u_handsy()) {
        if (n_conts >= 1)
            pline("Tipping contents to floor only...");
        *cancelled = FALSE;
        return (struct obj *) 0;
    }
#endif

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);

    dummyobj = cg.zeroobj; /* lint suppression; only its address matters */
    any = cg.zeroany;
    any.a_obj = &dummyobj;
    /* tip to floor does not require free hands */
    add_menu(win, &nul_glyphinfo, &any, '-', 0, ATR_NONE, clr,
             /* [TODO? vary destination string depending on surface()] */
             "on the floor", MENU_ITEMFLAGS_SELECTED);
    any = cg.zeroany;
    add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
             "", MENU_ITEMFLAGS_NONE);

    n_conts = 0;
    for (otmp = gi.invent; otmp; otmp = otmp->nobj) {
        if (otmp == box)
            continue;
        /* skip non-containers; bag of tricks passes Is_container() test,
           only include it if it isn't known to be a bag of tricks */
        if (!Is_container(otmp)
            || (otmp->otyp == BAG_OF_TRICKS && otmp->dknown
                && objects[otmp->otyp].oc_name_known))
            continue;
        if (!n_conts++)
            hands_available = u_handsy(); /* might issue message */
        /* container-to-container tip requires free hands;
           exclude container as possible target when known to be locked */
        exclude_it = !hands_available || (otmp->olocked && otmp->lknown);
        any = cg.zeroany;
        any.a_obj = !exclude_it ? otmp : 0;
        Sprintf(buf, "%s%s", !exclude_it ? "" : "    ", doname(otmp));
        add_menu(win, &nul_glyphinfo, &any, !exclude_it ? otmp->invlet : 0, 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    }

    Sprintf(buf, "Where to tip the contents of %s", doname(box));
    end_menu(win, buf);
    n = select_menu(win, PICK_ONE, &pick_list);
    destroy_nhwindow(win);

    otmp = 0;
    if (pick_list) {
        otmp = pick_list[0].item.a_obj;
        /* PICK_ONE with a preselected item might return 2;
           if so, choose the one that wasn't preselected */
        if (n > 1 && otmp == &dummyobj)
            otmp = pick_list[1].item.a_obj;
        if (otmp == &dummyobj)
            otmp = 0;
        free((genericptr_t) pick_list);
    }
    *cancelled = (boolean) (n == -1);
    return otmp;
}

/* Perform check on box if we can tip it.
   Returns one of TIPCHECK_foo values.
   If allowempty if TRUE, return TIPCHECK_OK instead of TIPCHECK_EMPTY. */
static int
tipcontainer_checks(
    struct obj *box,       /* container player wants to tip */
    struct obj *targetbox, /* destination (used here for horn of plenty) */
    boolean allowempty)    /* affects result when box is empty */
{
    /* undiscovered bag of tricks is acceptable as a container-to-container
       destination but it can't receive items; it has to be opened in
       preparation so apply it once before even trying to tip source box */
    if (targetbox && targetbox->otyp == BAG_OF_TRICKS) {
        int seencount = 0;

        bagotricks(targetbox, FALSE, &seencount);
        return TIPCHECK_CANNOT;
    }

    /* caveat: this assumes that cknown, lknown, olocked, and otrapped
       fields haven't been overloaded to mean something special for the
       non-standard "container" horn of plenty */
    if (!box->lknown) {
        box->lknown = 1;
        if (carried(box))
            update_inventory(); /* jumping the gun slightly; hope that's ok */
    }

    if (box->olocked) {
        pline("%s is locked.", upstart(thesimpleoname(box)));
        return TIPCHECK_LOCKED;

    } else if (box->otrapped) {
        /* we're not reaching inside but we're still handling it... */
        (void) chest_trap(box, HAND, FALSE);
        /* even if the trap fails, you've used up this turn */
        if (gm.multi >= 0) { /* in case we didn't become paralyzed */
            nomul(-1);
            gm.multi_reason = "tipping a container";
            gn.nomovemsg = "";
        }
        return TIPCHECK_TRAPPED;

    } else if (box->otyp == BAG_OF_TRICKS || box->otyp == HORN_OF_PLENTY) {
        int res = TIPCHECK_OK;
        boolean bag = (box->otyp == BAG_OF_TRICKS);
        int old_spe = box->spe, seen, totseen;
        boolean maybeshopgoods = (!carried(box)
                                  && costly_spot(box->ox, box->oy));
        coordxy ox = u.ux, oy = u.uy;

        if (targetbox
            && ((res = tipcontainer_checks(targetbox, NULL, TRUE))
                != TIPCHECK_OK))
            return res;

        if (get_obj_location(box, &ox, &oy, 0))
            box->ox = ox, box->oy = oy;

        if (maybeshopgoods && !box->no_charge)
            addtobill(box, FALSE, FALSE, TRUE);
        /* apply this bag/horn until empty or monster/object creation fails
           (if the latter occurs, force the former...) */
        seen = totseen = 0;
        do {
            if (!(bag ? bagotricks(box, TRUE, &seen)
                      : hornoplenty(box, TRUE, targetbox)))
                break;
            totseen += seen;
        } while (box->spe > 0);

        if (box->spe < old_spe) {
            if (bag && !totseen)
                pline("Nothing seems to happen.");
            /* check_unpaid wants to see a non-zero charge count */
            box->spe = old_spe;
            check_unpaid_usage(box, TRUE);
            box->spe = 0; /* empty */
            box->cknown = 1;
        }
        if (maybeshopgoods && !box->no_charge)
            subfrombill(box, shop_keeper(*in_rooms(ox, oy, SHOPBASE)));
        return TIPCHECK_CANNOT; /* actually means 'already done' */

    } else if (SchroedingersBox(box)) {
        char yourbuf[BUFSZ];
        boolean empty_it = FALSE;

        observe_quantum_cat(box, TRUE, TRUE);
        if (!Has_contents(box)) /* evidently a live cat came out */
            /* container type of "large box" is inferred */
            pline("%sbox is now empty.", Shk_Your(yourbuf, box));
        else /* holds cat corpse */
            empty_it = TRUE;
        box->cknown = 1;
        return (empty_it || allowempty) ? TIPCHECK_OK : TIPCHECK_EMPTY;

    } else if (!allowempty && !Has_contents(box)) {
        box->cknown = 1;
        pline("%s is empty.", upstart(thesimpleoname(box)));
        return TIPCHECK_EMPTY;

    }

    return TIPCHECK_OK;
}

/*pickup.c*/

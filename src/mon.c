/* NetHack 3.7	mon.c	$NHDT-Date: 1650836671 2022/04/24 21:44:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.423 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"
#include <ctype.h>

static void sanity_check_single_mon(struct monst *, boolean, const char *);
static struct obj *make_corpse(struct monst *, unsigned);
static int minliquid_core(struct monst *);
static void m_calcdistress(struct monst *);
static boolean monlineu(struct monst *, int, int);
static long mm_2way_aggression(struct monst *, struct monst *);
static long mm_aggression(struct monst *, struct monst *);
static long mm_displacement(struct monst *, struct monst *);
static void mon_leaving_level(struct monst *);
static void m_detach(struct monst *, struct permonst *);
static void set_mon_min_mhpmax(struct monst *, int);
static void lifesaved_monster(struct monst *);
static void migrate_mon(struct monst *, xchar, xchar);
static boolean ok_to_obliterate(struct monst *);
static void deal_with_overcrowding(struct monst *);
static void m_restartcham(struct monst *);
static boolean restrap(struct monst *);
static int pick_animal(void);
static int pickvampshape(struct monst *);
static boolean isspecmon(struct monst *);
static boolean validspecmon(struct monst *, int);
static struct permonst *accept_newcham_form(struct monst *, int);
static void kill_eggs(struct obj *);
static void pacify_guard(struct monst *);

#define LEVEL_SPECIFIC_NOCORPSE(mdat) \
    (Is_rogue_level(&u.uz)            \
     || (g.level.flags.graveyard && is_undead(mdat) && rn2(3)))

/* A specific combination of x_monnam flags for livelogging. The livelog
 * shouldn't show that you killed a hallucinatory monster and not what it
 * actually is. */
#define livelog_mon_nam(mtmp) \
    x_monnam(mtmp, ARTICLE_THE, (char *) 0,                 \
             (SUPPRESS_IT | SUPPRESS_HALLUCINATION), FALSE)


#if 0
/* part of the original warning code which was replaced in 3.3.1 */
const char *warnings[] = {
    "white", "pink", "red", "ruby", "purple", "black"
};
#endif /* 0 */


static void
sanity_check_single_mon(
    struct monst *mtmp,
    boolean chk_geno,
    const char *msg)
{
    struct permonst *mptr = mtmp->data;
    int mx = mtmp->mx, my = mtmp->my;

    if (!mptr || mptr < &mons[LOW_PM] || mptr >= &mons[NUMMONS]) {
        /* most sanity checks issue warnings if they detect a problem,
           but this would be too extreme to keep going */
        panic("illegal mon data %s; mnum=%d (%s)",
              fmt_ptr((genericptr_t) mptr), mtmp->mnum, msg);
    } else {
        int mndx = monsndx(mptr);

        if (mtmp->mnum != mndx) {
            impossible("monster mnum=%d, monsndx=%d (%s)",
                       mtmp->mnum, mndx, msg);
            mtmp->mnum = mndx;
        }
#if 0   /*
         * Gremlims don't obey the (mhpmax >= m_lev) rule so disable
         * this check, at least for the time being.  We could skip it
         * when the cloned flag is set, but the original gremlim would
         * still be an issue.
         */
        /* check before DEADMONSTER() because dead monsters should still
           have sane mhpmax */
        if (mtmp->mhpmax < 1
            || mtmp->mhpmax < (int) mtmp->m_lev
            || mtmp->mhp > mtmp->mhpmax)
            impossible(
                     "%s: level %d monster #%u [%s] has %d cur HP, %d max HP",
                       msg, (int) mtmp->m_lev,
                       mtmp->m_id, fmt_ptr((genericptr_t) mtmp),
                       mtmp->mhp, mtmp->mhpmax);
#endif
        if (DEADMONSTER(mtmp)) {
#if 0
            /* bad if not fmons list or if not vault guard */
            if (strcmp(msg, "fmon") || !mtmp->isgd)
                impossible("dead monster on %s; %s at <%d,%d>",
                           msg, mons[mndx].pmnames[NEUTRAL],
                           mx, my);
#endif
            return;
        }
        if (chk_geno && (g.mvitals[mndx].mvflags & G_GENOD) != 0)
            impossible("genocided %s in play (%s)",
                        pmname(&mons[mndx], Mgender(mtmp)), msg);
        if (mtmp->mtame && !mtmp->mpeaceful)
            impossible("tame %s is not peaceful (%s)",
                        pmname(&mons[mndx], Mgender(mtmp)), msg);
    }
    if (mtmp->isshk && !has_eshk(mtmp))
        impossible("shk without eshk (%s)", msg);
    if (mtmp->ispriest && !has_epri(mtmp))
        impossible("priest without epri (%s)", msg);
    if (mtmp->isgd && !has_egd(mtmp))
        impossible("guard without egd (%s)", msg);
    if (mtmp->isminion && !has_emin(mtmp))
        impossible("minion without emin (%s)", msg);
    /* guardian angel on astral level is tame but has emin rather than edog */
    if (mtmp->mtame && !has_edog(mtmp) && !mtmp->isminion)
        impossible("pet without edog (%s)", msg);
    /* steed should be tame and saddled */
    if (mtmp == u.usteed) {
        const char *ns, *nt = !mtmp->mtame ? "not tame" : 0;

        ns = !m_carrying(mtmp, SADDLE) ? "no saddle"
             : !which_armor(mtmp, W_SADDLE) ? "saddle not worn"
               : 0;
        if (ns || nt)
            impossible("steed: %s%s%s (%s)",
                       ns ? ns : "", (ns && nt) ? ", " : "", nt ? nt : "",
                       msg);
    }

    if (mtmp->mtrapped) {
        if (mtmp->wormno) {
            /* TODO: how to check worm in trap? */
        } else if (!t_at(mx, my))
            impossible("trapped without a trap (%s)", msg);
    }

    /* monster is hiding? */
    if (mtmp->mundetected) {
        struct trap *t;

        if (!isok(mx, my)) /* caller will have checked this but not fixed it */
            mx = my = 0;
        if (mtmp == u.ustuck)
            impossible("hiding monster stuck to you (%s)", msg);
        if (m_at(mx, my) == mtmp && hides_under(mptr) && !OBJ_AT(mx, my))
            impossible("mon hiding under nonexistent obj (%s)", msg);
        if (mptr->mlet == S_EEL
            && !is_pool(mx, my) && !Is_waterlevel(&u.uz))
            impossible("eel hiding out of water (%s)", msg);
        if (ceiling_hider(mptr)
            /* normally !accessible would be overridable with passes_walls,
               but not for hiding on the ceiling */
            && (!has_ceiling(&u.uz) || !accessible(mx, my)))
            impossible("ceiling hider hiding %s (%s)",
                       !has_ceiling(&u.uz) ? "without ceiling"
                                           : "in solid stone",
                       msg);
        if (mtmp->mtrapped && (t = t_at(mx, my)) != 0
            && !(t->ttyp == PIT || t->ttyp == SPIKED_PIT))
            impossible("hiding while trapped in a non-pit (%s)", msg);
    } else if (M_AP_TYPE(mtmp) != M_AP_NOTHING) {
        boolean is_mimic = (mptr->mlet == S_MIMIC);
        const char *what = (M_AP_TYPE(mtmp) == M_AP_FURNITURE) ? "furniture"
                           : (M_AP_TYPE(mtmp) == M_AP_MONSTER) ? "a monster"
                             : (M_AP_TYPE(mtmp) == M_AP_OBJECT) ? "an object"
                               : "something strange";

        if (Protection_from_shape_changers)
            impossible(
                "mimic%s concealed as %s despite Prot-from-shape-changers %s",
                       is_mimic ? "" : "ker", what, msg);
        /* the Wizard's clone after "double trouble" starts out mimicking
           some other monster; pet's quickmimic effect can temporarily take
           on furniture, object, or monster shape, but only until the pet
           finishes eating a mimic corpse */
        if (!(is_mimic || mtmp->meating
              || (mtmp->iswiz && M_AP_TYPE(mtmp) == M_AP_MONSTER)))
            impossible("non-mimic (%s) posing as %s (%s)",
                       mptr->pmnames[NEUTRAL], what, msg);
#if 0   /* mimics who end up in strange locations do still hide while there */
        if (!(accessible(mx, my) || passes_walls(mptr))) {
            char buf[BUFSZ];
            const char *typnam = levltyp_to_name(levl[mx][my].typ);

            if (!typnam) {
                Sprintf(buf, "[%d]", levl[mx][my].typ);
                typnam = buf;
            }
            impossible("mimic%s concealed in inaccessible location: %s (%s)",
                       is_mimic ? "" : "ker", typnam, msg);
        }
#endif
    }
}

void
mon_sanity_check(void)
{
    int x, y;
    struct monst *mtmp, *m;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        /* dead monsters should still have sane data */
        sanity_check_single_mon(mtmp, TRUE, "fmon");
        if (DEADMONSTER(mtmp) && !mtmp->isgd)
            continue;

        x = mtmp->mx, y = mtmp->my;
        if (!isok(x, y) && !(mtmp->isgd && x == 0 && y == 0)) {
            impossible("mon (%s) claims to be at <%d,%d>?",
                       fmt_ptr((genericptr_t) mtmp), x, y);
        } else if (mtmp == u.usteed) {
            /* steed is in fmon list but not on the map; its
               <mx,my> coordinates should match hero's location */
            if (x != u.ux || y != u.uy)
                impossible("steed (%s) claims to be at <%d,%d>?",
                           fmt_ptr((genericptr_t) mtmp), x, y);
        } else if (g.level.monsters[x][y] != mtmp) {
            impossible("mon (%s) at <%d,%d> is not there!",
                       fmt_ptr((genericptr_t) mtmp), x, y);
        } else if (mtmp->wormno) {
            sanity_check_worm(mtmp);
        }
    }

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if ((mtmp = g.level.monsters[x][y]) != 0) {
                for (m = fmon; m; m = m->nmon)
                    if (m == mtmp)
                        break;
                if (!m)
                    impossible("map mon (%s) at <%d,%d> not in fmon list!",
                               fmt_ptr((genericptr_t) mtmp), x, y);
                else if (mtmp == u.usteed)
                    impossible("steed (%s) is on the map at <%d,%d>!",
                               fmt_ptr((genericptr_t) mtmp), x, y);
                else if ((mtmp->mx != x || mtmp->my != y)
                         && mtmp->data != &mons[PM_LONG_WORM])
                    impossible("map mon (%s) at <%d,%d> is found at <%d,%d>?",
                               fmt_ptr((genericptr_t) mtmp),
                               mtmp->mx, mtmp->my, x, y);
            }

    for (mtmp = g.migrating_mons; mtmp; mtmp = mtmp->nmon) {
        sanity_check_single_mon(mtmp, FALSE, "migr");
    }

    wormno_sanity_check(); /* test for bogus worm tail */
}

/* Would monster be OK with poison gas? */
/* Does not check for actual poison gas at the location. */
/* Returns one of M_POISONGAS_foo */
int
m_poisongas_ok(struct monst* mtmp)
{
    int px, py;
    boolean is_you = (mtmp == &g.youmonst);

    /* Non living, non breathing, immune monsters are not concerned */
    if (nonliving(mtmp->data) || is_vampshifter(mtmp)
        || breathless(mtmp->data) || immune_poisongas(mtmp->data))
        return M_POISONGAS_OK;
    /* not is_swimmer(); assume that non-fish are swimming on
       the surface and breathing the air above it periodically
       unless located at water spot on plane of water */
    px = is_you ? u.ux : mtmp->mx;
    py = is_you ? u.uy : mtmp->my;
    if ((mtmp->data->mlet == S_EEL || Is_waterlevel(&u.uz))
        && is_pool(px, py))
        return M_POISONGAS_OK;
    /* exclude monsters with poison gas breath attack:
       adult green dragon and Chromatic Dragon (and iron golem,
       but nonliving() and breathless() tests also catch that) */
    if (attacktype_fordmg(mtmp->data, AT_BREA, AD_DRST)
        || attacktype_fordmg(mtmp->data, AT_BREA, AD_RBRE))
        return M_POISONGAS_OK;
    if (is_you && (u.uinvulnerable || Breathless || Underwater))
        return M_POISONGAS_OK;
    if (is_you ? Poison_resistance : resists_poison(mtmp))
        return M_POISONGAS_MINOR;
    return M_POISONGAS_BAD;
}

/* return True if mon is capable of converting other monsters into zombies */
boolean
zombie_maker(struct monst *mon)
{
    struct permonst *pm = mon->data;

    if (mon->mcan)
        return FALSE;

    switch (pm->mlet) {
    case S_ZOMBIE:
        /* Z-class monsters that aren't actually zombies go here */
        if (pm == &mons[PM_GHOUL] || pm == &mons[PM_SKELETON])
            return FALSE;
        return TRUE;
    case S_LICH:
        /* all liches will create zombies as well */
        return TRUE;
    }
    return FALSE;
}

/* Return monster index of zombie monster which this monster could
   be turned into, or NON_PM if it doesn't have a direct counterpart.
   Sort of the zombie-specific inverse of undead_to_corpse. */
int
zombie_form(struct permonst *pm)
{
    switch (pm->mlet) {
    case S_ZOMBIE: /* when already a zombie/ghoul/skeleton, will stay as is */
        return NON_PM;
    case S_KOBOLD:
        return PM_KOBOLD_ZOMBIE;
    case S_ORC:
        return PM_ORC_ZOMBIE;
    case S_GIANT:
        if (pm == &mons[PM_ETTIN])
            return PM_ETTIN_ZOMBIE;
        return PM_GIANT_ZOMBIE;
    case S_HUMAN:
    case S_KOP:
        if (is_elf(pm))
            return PM_ELF_ZOMBIE;
        return PM_HUMAN_ZOMBIE;
    case S_HUMANOID:
        if (is_dwarf(pm))
            return PM_DWARF_ZOMBIE;
        else
            break;
    case S_GNOME:
        return PM_GNOME_ZOMBIE;
    }
    return NON_PM;
}

/* convert the monster index of an undead to its living counterpart */
int
undead_to_corpse(int mndx)
{
    switch (mndx) {
    case PM_KOBOLD_ZOMBIE:
    case PM_KOBOLD_MUMMY:
        mndx = PM_KOBOLD;
        break;
    case PM_DWARF_ZOMBIE:
    case PM_DWARF_MUMMY:
        mndx = PM_DWARF;
        break;
    case PM_GNOME_ZOMBIE:
    case PM_GNOME_MUMMY:
        mndx = PM_GNOME;
        break;
    case PM_ORC_ZOMBIE:
    case PM_ORC_MUMMY:
        mndx = PM_ORC;
        break;
    case PM_ELF_ZOMBIE:
    case PM_ELF_MUMMY:
        mndx = PM_ELF;
        break;
    case PM_VAMPIRE:
    case PM_VAMPIRE_LEADER:
#if 0 /* DEFERRED */
    case PM_VAMPIRE_MAGE:
#endif
    case PM_HUMAN_ZOMBIE:
    case PM_HUMAN_MUMMY:
        mndx = PM_HUMAN;
        break;
    case PM_GIANT_ZOMBIE:
    case PM_GIANT_MUMMY:
        mndx = PM_GIANT;
        break;
    case PM_ETTIN_ZOMBIE:
    case PM_ETTIN_MUMMY:
        mndx = PM_ETTIN;
        break;
    default:
        break;
    }
    return mndx;
}

/* Convert the monster index of some monsters (such as quest guardians)
 * to their generic species type.
 *
 * Return associated character class monster, rather than species
 * if mode is 1.
 */
int
genus(int mndx, int mode)
{
    switch (mndx) {
    /* Quest guardians */
    case PM_STUDENT:
        mndx = mode ? PM_ARCHEOLOGIST : PM_HUMAN;
        break;
    case PM_CHIEFTAIN:
        mndx = mode ? PM_BARBARIAN : PM_HUMAN;
        break;
    case PM_NEANDERTHAL:
        mndx = mode ? PM_CAVE_DWELLER : PM_HUMAN;
        break;
    case PM_ATTENDANT:
        mndx = mode ? PM_HEALER : PM_HUMAN;
        break;
    case PM_PAGE:
        mndx = mode ? PM_KNIGHT : PM_HUMAN;
        break;
    case PM_ABBOT:
        mndx = mode ? PM_MONK : PM_HUMAN;
        break;
    case PM_ACOLYTE:
        mndx = mode ? PM_CLERIC : PM_HUMAN;
        break;
    case PM_HUNTER:
        mndx = mode ? PM_RANGER : PM_HUMAN;
        break;
    case PM_THUG:
        mndx = mode ? PM_ROGUE : PM_HUMAN;
        break;
    case PM_ROSHI:
        mndx = mode ? PM_SAMURAI : PM_HUMAN;
        break;
    case PM_GUIDE:
        mndx = mode ? PM_TOURIST : PM_HUMAN;
        break;
    case PM_APPRENTICE:
        mndx = mode ? PM_WIZARD : PM_HUMAN;
        break;
    case PM_WARRIOR:
        mndx = mode ? PM_VALKYRIE : PM_HUMAN;
        break;
    default:
        if (mndx >= LOW_PM && mndx < NUMMONS) {
            struct permonst *ptr = &mons[mndx];

            if (is_human(ptr))
                mndx = PM_HUMAN;
            else if (is_elf(ptr))
                mndx = PM_ELF;
            else if (is_dwarf(ptr))
                mndx = PM_DWARF;
            else if (is_gnome(ptr))
                mndx = PM_GNOME;
            else if (is_orc(ptr))
                mndx = PM_ORC;
        }
        break;
    }
    return mndx;
}

/* return monster index if chameleon, or NON_PM if not */
int
pm_to_cham(int mndx)
{
    int mcham = NON_PM;

    /*
     * As of 3.6.0 we just check M2_SHAPESHIFTER instead of having a
     * big switch statement with hardcoded shapeshifter types here.
     */
    if (mndx >= LOW_PM && is_shapeshifter(&mons[mndx]))
        mcham = mndx;
    return mcham;
}

/* for deciding whether corpse will carry along full monster data */
#define KEEPTRAITS(mon)                                                 \
    ((mon)->isshk || (mon)->mtame || unique_corpstat((mon)->data)       \
     || is_reviver((mon)->data)                                         \
        /* normally quest leader will be unique, */                     \
        /* but he or she might have been polymorphed  */                \
     || (mon)->m_id == g.quest_status.leader_m_id                       \
        /* special cancellation handling for these */                   \
     || (dmgtype((mon)->data, AD_SEDU) || dmgtype((mon)->data, AD_SSEX)))

/* Creates a monster corpse, a "special" corpse, or nothing if it doesn't
 * leave corpses.  Monsters which leave "special" corpses should have
 * G_NOCORPSE set in order to prevent wishing for one, finding tins of one,
 * etc....
 */
static struct obj *
make_corpse(struct monst *mtmp, unsigned int corpseflags)
{
    register struct permonst *mdat = mtmp->data;
    int num;
    struct obj *obj = (struct obj *) 0;
    struct obj *otmp = (struct obj *) 0;
    int x = mtmp->mx, y = mtmp->my;
    int mndx = monsndx(mdat);
    unsigned corpstatflags = corpseflags;
    boolean burythem = ((corpstatflags & CORPSTAT_BURIED) != 0);

    if (mtmp->female)
        corpstatflags |= CORPSTAT_FEMALE;
    else if (!is_neuter(mtmp->data))
        corpstatflags |= CORPSTAT_MALE;

    switch (mndx) {
    case PM_GRAY_DRAGON:
    case PM_SILVER_DRAGON:
#if 0 /* DEFERRED */
    case PM_SHIMMERING_DRAGON:
#endif
    case PM_RED_DRAGON:
    case PM_ORANGE_DRAGON:
    case PM_WHITE_DRAGON:
    case PM_BLACK_DRAGON:
    case PM_BLUE_DRAGON:
    case PM_GREEN_DRAGON:
    case PM_YELLOW_DRAGON:
        /* Make dragon scales.  This assumes that the order of the
           dragons is the same as the order of the scales. */
        if (!rn2(mtmp->mrevived ? 20 : 3)) {
            num = GRAY_DRAGON_SCALES + monsndx(mdat) - PM_GRAY_DRAGON;
            obj = mksobj_at(num, x, y, FALSE, FALSE);
            obj->spe = 0;
            obj->cursed = obj->blessed = FALSE;
        }
        goto default_1;
    case PM_WHITE_UNICORN:
    case PM_GRAY_UNICORN:
    case PM_BLACK_UNICORN:
        if (mtmp->mrevived && rn2(2)) {
            if (canseemon(mtmp))
                pline("%s recently regrown horn crumbles to dust.",
                      s_suffix(Monnam(mtmp)));
        } else {
            obj = mksobj_at(UNICORN_HORN, x, y, TRUE, FALSE);
            if (obj && mtmp->mrevived)
                obj->degraded_horn = 1;
        }
        goto default_1;
    case PM_LONG_WORM:
        (void) mksobj_at(WORM_TOOTH, x, y, TRUE, FALSE);
        goto default_1;
    case PM_VAMPIRE:
    case PM_VAMPIRE_LEADER:
        /* include mtmp in the mkcorpstat() call */
        num = undead_to_corpse(mndx);
        corpstatflags |= CORPSTAT_INIT;
        obj = mkcorpstat(CORPSE, mtmp, &mons[num], x, y, corpstatflags);
        obj->age -= (TAINT_AGE + 1); /* this is an *OLD* corpse */
        break;
    case PM_KOBOLD_MUMMY:
    case PM_DWARF_MUMMY:
    case PM_GNOME_MUMMY:
    case PM_ORC_MUMMY:
    case PM_ELF_MUMMY:
    case PM_HUMAN_MUMMY:
    case PM_GIANT_MUMMY:
    case PM_ETTIN_MUMMY:
    case PM_KOBOLD_ZOMBIE:
    case PM_DWARF_ZOMBIE:
    case PM_GNOME_ZOMBIE:
    case PM_ORC_ZOMBIE:
    case PM_ELF_ZOMBIE:
    case PM_HUMAN_ZOMBIE:
    case PM_GIANT_ZOMBIE:
    case PM_ETTIN_ZOMBIE:
        num = undead_to_corpse(mndx);
        corpstatflags |= CORPSTAT_INIT;
        obj = mkcorpstat(CORPSE, mtmp, &mons[num], x, y, corpstatflags);
        obj->age -= (TAINT_AGE + 1); /* this is an *OLD* corpse */
        break;
    case PM_IRON_GOLEM:
        num = d(2, 6);
        while (num--)
            obj = mksobj_at(IRON_CHAIN, x, y, TRUE, FALSE);
        free_mgivenname(mtmp); /* don't christen obj */
        break;
    case PM_GLASS_GOLEM:
        num = d(2, 4); /* very low chance of creating all glass gems */
        while (num--)
            obj = mksobj_at((LAST_GEM + rnd(NUM_GLASS_GEMS)),
                            x, y, TRUE, FALSE);
        free_mgivenname(mtmp);
        break;
    case PM_CLAY_GOLEM:
        obj = mksobj_at(ROCK, x, y, FALSE, FALSE);
        obj->quan = (long) (rn2(20) + 50);
        obj->owt = weight(obj);
        free_mgivenname(mtmp);
        break;
    case PM_STONE_GOLEM:
        corpstatflags &= ~CORPSTAT_INIT;
        obj = mkcorpstat(STATUE, (struct monst *) 0, mdat, x, y,
                         corpstatflags);
        break;
    case PM_WOOD_GOLEM:
        num = d(2, 4);
        while (num--) {
            obj = mksobj_at(
                            rn2(2) ? QUARTERSTAFF
                            : rn2(3) ? SMALL_SHIELD
                            : rn2(3) ? CLUB
                            : rn2(3) ? ELVEN_SPEAR : BOOMERANG,
                            x, y, TRUE, FALSE);
        }
        free_mgivenname(mtmp);
        break;
    case PM_ROPE_GOLEM:
        num = rn2(3);
        while (num-- > 0) {
            obj = mksobj_at(rn2(2) ? LEASH : BULLWHIP, x, y, TRUE, FALSE);
        }
        free_mgivenname(mtmp);
        break;
    case PM_LEATHER_GOLEM:
        num = d(2, 4);
        while (num--)
            obj = mksobj_at(LEATHER_ARMOR, x, y, TRUE, FALSE);
        free_mgivenname(mtmp);
        break;
    case PM_GOLD_GOLEM:
        /* Good luck gives more coins */
        obj = mkgold((long) (200 - rnl(101)), x, y);
        free_mgivenname(mtmp);
        break;
    case PM_PAPER_GOLEM:
        num = rnd(4);
        while (num--)
            obj = mksobj_at(SCR_BLANK_PAPER, x, y, TRUE, FALSE);
        free_mgivenname(mtmp);
        break;
    /* expired puddings will congeal into a large blob;
       like dragons, relies on the order remaining consistent */
    case PM_GRAY_OOZE:
    case PM_BROWN_PUDDING:
    case PM_GREEN_SLIME:
    case PM_BLACK_PUDDING:
        /* we have to do this here because most other places
           expect there to be an object coming back; not this one */
        obj = mksobj_at(GLOB_OF_BLACK_PUDDING - (PM_BLACK_PUDDING - mndx),
                        x, y, TRUE, FALSE);

        while (obj && (otmp = obj_nexto(obj)) != (struct obj *) 0) {
            pudding_merge_message(obj, otmp);
            obj = obj_meld(&obj, &otmp);
        }
        free_mgivenname(mtmp);
        return obj;
    default:
 default_1:
        if (g.mvitals[mndx].mvflags & G_NOCORPSE) {
            return (struct obj *) 0;
        } else {
            corpstatflags |= CORPSTAT_INIT;
            /* preserve the unique traits of some creatures */
            obj = mkcorpstat(CORPSE, KEEPTRAITS(mtmp) ? mtmp : 0,
                             mdat, x, y, corpstatflags);
            if (burythem) {
                boolean dealloc;

                (void) bury_an_obj(obj, &dealloc);
                newsym(x, y);
                return dealloc ? (struct obj *) 0 : obj;
            }
        }
        break;
    }
    /* All special cases should precede the G_NOCORPSE check */

    if (!obj)
        return (struct obj *) 0;

    /* if polymorph or undead turning has killed this monster,
       prevent the same attack beam from hitting its corpse */
    if (g.context.bypasses)
        bypass_obj(obj);

    if (has_mgivenname(mtmp))
        obj = oname(obj, MGIVENNAME(mtmp), ONAME_NO_FLAGS);

    /*  Avoid "It was hidden under a green mold corpse!"
     *  during Blind combat. An unseen monster referred to as "it"
     *  could be killed and leave a corpse.  If a hider then hid
     *  underneath it, you could be told the corpse type of a
     *  monster that you never knew was there without this.
     *  The code in hitmu() substitutes the word "something"
     *  if the corpse's obj->dknown is 0.
     */
    if (Blind && !sensemon(mtmp))
        clear_dknown(obj); /* obj->dknown = 0; */

    stackobj(obj);
    newsym(x, y);
    return obj;
}

/* check mtmp and water/lava for compatibility, 0 (survived), 1 (died) */
int
minliquid(struct monst* mtmp)
{
    int res;

    /* set up flag for mondead() and xkilled() */
    iflags.sad_feeling = (mtmp->mtame && !canseemon(mtmp));
    res = minliquid_core(mtmp);
    /* always clear the flag */
    iflags.sad_feeling = FALSE;
    return res;
}

/* guts of minliquid() */
static int
minliquid_core(struct monst* mtmp)
{
    boolean inpool, inlava, infountain;
    boolean waterwall = is_waterwall(mtmp->mx,mtmp->my);

    /* [ceiling clingers are handled below] */
    inpool = (is_pool(mtmp->mx, mtmp->my)
              && (!(is_flyer(mtmp->data) || is_floater(mtmp->data))
                  /* there's no "above the surface" on the plane of water */
                  || Is_waterlevel(&u.uz)));
    inlava = (is_lava(mtmp->mx, mtmp->my)
              && !(is_flyer(mtmp->data) || is_floater(mtmp->data)));
    infountain = IS_FOUNTAIN(levl[mtmp->mx][mtmp->my].typ);

    /* Flying and levitation keeps our steed out of the liquid
       (but not water-walking or swimming; note: if hero is in a
       water location on the Plane of Water, flight and levitating
       are blocked so this (Flying || Levitation) test fails there
       and steed will be subject to water effects, as intended) */
    if (mtmp == u.usteed && (Flying || Levitation) && !waterwall)
        return 0;

    /* Gremlin multiplying won't go on forever since the hit points
     * keep going down, and when it gets to 1 hit point the clone
     * function will fail.
     */
    if (mtmp->data == &mons[PM_GREMLIN] && (inpool || infountain) && rn2(3)) {
        if (split_mon(mtmp, (struct monst *) 0))
            dryup(mtmp->mx, mtmp->my, FALSE);
        if (inpool)
            water_damage_chain(mtmp->minvent, FALSE);
        return 0;
    } else if (mtmp->data == &mons[PM_IRON_GOLEM] && inpool && !rn2(5)) {
        int dam = d(2, 6);

        if (cansee(mtmp->mx, mtmp->my))
            pline("%s rusts.", Monnam(mtmp));
        mtmp->mhp -= dam;
        if (mtmp->mhpmax > dam)
            mtmp->mhpmax -= dam;
        if (DEADMONSTER(mtmp)) {
            mondied(mtmp);
            if (DEADMONSTER(mtmp))
                return 1;
        }
        water_damage_chain(mtmp->minvent, FALSE);
        return 0;
    }

    if (inlava) {
        /*
         * Lava effects much as water effects. Lava likers are able to
         * protect their stuff. Fire resistant monsters can only protect
         * themselves  --ALI
         */
        if (!is_clinger(mtmp->data) && !likes_lava(mtmp->data)) {
            /* not fair...?  hero doesn't automatically teleport away
               from lava, just from water */
            if (can_teleport(mtmp->data) && !tele_restrict(mtmp)) {
                if (rloc(mtmp, RLOC_MSG))
                    return 0;
            }
            if (!resists_fire(mtmp)) {
                if (cansee(mtmp->mx, mtmp->my)) {
                    struct attack *dummy = &mtmp->data->mattk[0];
                    const char *how = on_fire(mtmp->data, dummy);

                    pline("%s %s.", Monnam(mtmp),
                          !strcmp(how, "boiling") ? "boils away"
                             : !strcmp(how, "melting") ? "melts away"
                                : "burns to a crisp");
                }
                /* unlike fire -> melt ice -> pool, there's no way for the
                   hero to create lava beneath a monster, so the !mon_moving
                   case is not expected to happen (and we haven't made a
                   player-against-monster variation of the message above) */
                if (g.context.mon_moving)
                    mondead(mtmp); /* no corpse */
                else
                    xkilled(mtmp, XKILL_NOMSG);
            } else {
                mtmp->mhp -= 1;
                if (DEADMONSTER(mtmp)) {
                    if (cansee(mtmp->mx, mtmp->my))
                        pline("%s surrenders to the fire.", Monnam(mtmp));
                    mondead(mtmp); /* no corpse */
                } else if (cansee(mtmp->mx, mtmp->my))
                    pline("%s burns slightly.", Monnam(mtmp));
            }
            if (!DEADMONSTER(mtmp)) {
                (void) fire_damage_chain(mtmp->minvent, FALSE, FALSE,
                                         mtmp->mx, mtmp->my);
                (void) rloc(mtmp, RLOC_ERR|RLOC_NOMSG);
                return 0;
            }
            return 1;
        }
    } else if (inpool || waterwall) {
        /* Most monsters drown in pools.  flooreffects() will take care of
         * water damage to dead monsters' inventory, but survivors need to
         * be handled here.  Swimmers are able to protect their stuff...
         */
        if ((waterwall || !is_clinger(mtmp->data))
            && !is_swimmer(mtmp->data) && !amphibious(mtmp->data)) {
            /* like hero with teleport intrinsic or spell, teleport away
               if possible */
            if (can_teleport(mtmp->data) && !tele_restrict(mtmp)) {
                if (rloc(mtmp, RLOC_MSG))
                    return 0;
            }
            if (cansee(mtmp->mx, mtmp->my)) {
                if (g.context.mon_moving)
                    pline("%s drowns.", Monnam(mtmp));
                else
                    /* hero used fire to melt ice that monster was on */
                    You("drown %s.", mon_nam(mtmp));
            }
            if (engulfing_u(mtmp)) {
                /* This can happen after a purple worm plucks you off a
                   flying steed while you are over water. */
                pline("%s sinks as %s rushes in and flushes you out.",
                      Monnam(mtmp), hliquid("water"));
            }
            if (g.context.mon_moving)
                mondied(mtmp); /* ok to leave corpse despite water */
            else
                xkilled(mtmp, XKILL_NOMSG);
            if (!DEADMONSTER(mtmp)) {
                water_damage_chain(mtmp->minvent, FALSE);
                if (!rloc(mtmp, RLOC_NOMSG))
                    deal_with_overcrowding(mtmp);
                return 0;
            }
            return 1;
        }
    } else {
        /* but eels have a difficult time outside */
        if (mtmp->data->mlet == S_EEL && !Is_waterlevel(&u.uz)
            && !breathless(mtmp->data)) {
            /* as mhp gets lower, the rate of further loss slows down */
            if (mtmp->mhp > 1 && rn2(mtmp->mhp) > rn2(8))
                mtmp->mhp--;
            monflee(mtmp, 2, FALSE, FALSE);
        }
    }
    return 0;
}

/* calculate 'mon's movement for current turn; called from moveloop() */
int
mcalcmove(
    struct monst *mon,
    boolean m_moving) /* True: adjust for moving; False: just adjust for speed */
{
    int mmove = mon->data->mmove;
    int mmove_adj;

    /* Note: MSLOW's `+ 1' prevents slowed speed 1 getting reduced to 0;
     *       MFAST's `+ 2' prevents hasted speed 1 from becoming a no-op;
     *       both adjustments have negligible effect on higher speeds.
     */
    if (mon->mspeed == MSLOW)
        mmove = (2 * mmove + 1) / 3;
    else if (mon->mspeed == MFAST)
        mmove = (4 * mmove + 2) / 3;

    if (mon == u.usteed && u.ugallop && g.context.mv) {
        /* increase movement by a factor of 1.5; also increase variance of
           movement speed (if it's naturally 24, we don't want it to always
           become 36) */
        mmove = ((rn2(2) ? 4 : 5) * mmove) / 3;
    }

    if (m_moving) {
        /* Randomly round the monster's speed to a multiple of NORMAL_SPEED.
           This makes it impossible for the player to predict when they'll
           get a free turn (thus preventing exploits like "melee kiting"),
           while retaining guarantees about shopkeepers not being outsped
           by a normal-speed player, normal-speed players being unable
           to open up a gap when fleeing a normal-speed monster, etc. */
        mmove_adj = mmove % NORMAL_SPEED;
        mmove -= mmove_adj;
        if (rn2(NORMAL_SPEED) < mmove_adj)
            mmove += NORMAL_SPEED;
    }
    return mmove;
}

/* actions that happen once per ``turn'', regardless of each
   individual monster's metabolism; some of these might need to
   be reclassified to occur more in proportion with movement rate */
void
mcalcdistress(void)
{
    iter_mons(m_calcdistress);
}

static void
m_calcdistress(struct monst *mtmp)
{
    /* must check non-moving monsters once/turn in case they managed
       to end up in water or lava; note: when not in liquid they regen,
       shape-shift, timeout temporary maladies just like other monsters */
    if (mtmp->data->mmove == 0) {
        if (g.vision_full_recalc)
            vision_recalc(0);
        if (minliquid(mtmp))
            return;
    }

    /* regenerate hit points */
    mon_regen(mtmp, FALSE);

    /* possibly polymorph shapechangers and lycanthropes */
    if (mtmp->cham >= LOW_PM)
        decide_to_shapeshift(mtmp, (canspotmon(mtmp)
                                    || engulfing_u(mtmp))
                             ? SHIFT_MSG : 0);
    were_change(mtmp);

    /* gradually time out temporary problems */
    if (mtmp->mblinded && !--mtmp->mblinded)
        mtmp->mcansee = 1;
    if (mtmp->mfrozen && !--mtmp->mfrozen)
        mtmp->mcanmove = 1;
    if (mtmp->mfleetim && !--mtmp->mfleetim)
        mtmp->mflee = 0;

    /* FIXME: mtmp->mlstmv ought to be updated here */
}

int
movemon(void)
{
    register struct monst *mtmp, *nmtmp;
    register boolean somebody_can_move = FALSE;

    /*
     * Some of you may remember the former assertion here that
     * because of deaths and other actions, a simple one-pass
     * algorithm wasn't possible for movemon.  Deaths are no longer
     * removed to the separate list fdmon; they are simply left in
     * the chain with hit points <= 0, to be cleaned up at the end
     * of the pass.
     *
     * The only other actions which cause monsters to be removed from
     * the chain are level migrations and losedogs().  I believe losedogs()
     * is a cleanup routine not associated with monster movements, and
     * monsters can only affect level migrations on themselves, not others
     * (hence the fetching of nmon before moving the monster).  Currently,
     * monsters can jump into traps, read cursed scrolls of teleportation,
     * and drink cursed potions of raise level to change levels.  These are
     * all reflexive at this point.  Should one monster be able to level
     * teleport another, this scheme would have problems.
     */

    for (mtmp = fmon; mtmp; mtmp = nmtmp) {
        /* end monster movement early if hero is flagged to leave the level */
        if (u.utotype
#ifdef SAFERHANGUP
            /* or if the program has lost contact with the user */
            || g.program_state.done_hup
#endif
            ) {
            somebody_can_move = FALSE;
            break;
        }
        nmtmp = mtmp->nmon;
        /* one dead monster needs to perform a move after death: vault
           guard whose temporary corridor is still on the map; live
           guards who have led the hero back to civilization get moved
           off the map too; gd_move() decides whether the temporary
           corridor can be removed and guard discarded (via clearing
           mon->isgd flag so that dmonsfree() will get rid of mon) */
        if (mtmp->isgd && !mtmp->mx) {
            /* parked at <0,0>; eventually isgd should get set to false */
            if (g.moves > mtmp->mlstmv) {
                (void) gd_move(mtmp);
                mtmp->mlstmv = g.moves;
            }
            continue;
        }
        if (DEADMONSTER(mtmp))
            continue;

        /* Find a monster that we have not treated yet. */
        if (mtmp->movement < NORMAL_SPEED)
            continue;

        mtmp->movement -= NORMAL_SPEED;
        if (mtmp->movement >= NORMAL_SPEED)
            somebody_can_move = TRUE;

        if (g.vision_full_recalc)
            vision_recalc(0); /* vision! */

        /* reset obj bypasses before next monster moves */
        if (g.context.bypasses)
            clear_bypasses();
        clear_splitobjs();
        if (minliquid(mtmp))
            continue;

        /* after losing equipment, try to put on replacement */
        if (mtmp->misc_worn_check & I_SPECIAL) {
            long oldworn;

            mtmp->misc_worn_check &= ~I_SPECIAL;
            oldworn = mtmp->misc_worn_check;
            m_dowear(mtmp, FALSE);
            if (mtmp->misc_worn_check != oldworn || !mtmp->mcanmove)
                continue;
        }

        if (is_hider(mtmp->data)) {
            /* unwatched mimics and piercers may hide again  [MRS] */
            if (restrap(mtmp))
                continue;
            if (M_AP_TYPE(mtmp) == M_AP_FURNITURE
                || M_AP_TYPE(mtmp) == M_AP_OBJECT)
                continue;
            if (mtmp->mundetected)
                continue;
        } else if (mtmp->data->mlet == S_EEL && !mtmp->mundetected
                   && (mtmp->mflee || !next2u(mtmp->mx, mtmp->my))
                   && !canseemon(mtmp) && !rn2(4)) {
            /* some eels end up stuck in isolated pools, where they
               can't--or at least won't--move, so they never reach
               their post-move chance to re-hide */
            if (hideunder(mtmp))
                continue;
        }

        /* continue if the monster died fighting */
        if (Conflict && !mtmp->iswiz && m_canseeu(mtmp)) {
            /* Note:
             *  Conflict does not take effect in the first round.
             *  Therefore, A monster when stepping into the area will
             *  get to swing at you.
             *
             *  The call to fightm() must be _last_.  The monster might
             *  have died if it returns 1.
             */
            if (cansee(mtmp->mx, mtmp->my)
                && (distu(mtmp->mx, mtmp->my) <= BOLT_LIM * BOLT_LIM)
                && fightm(mtmp))
                continue; /* mon might have died */
        }
        if (dochugw(mtmp)) /* otherwise just move the monster */
            continue;
    }

    if (any_light_source())
        g.vision_full_recalc = 1; /* in case a mon moved with a light source */
    /* reset obj bypasses after last monster has moved */
    if (g.context.bypasses)
        clear_bypasses();
    clear_splitobjs();
    /* remove dead monsters; dead vault guard will be left at <0,0>
       if temporary corridor out of vault hasn't been removed yet */
    dmonsfree();

    /* a monster may have levteleported player -dlc */
    if (u.utotype) {
        deferred_goto();
        /* changed levels, so these monsters are dormant */
        somebody_can_move = FALSE;
    }

    return somebody_can_move;
}

#define mstoning(obj)                                       \
    (ofood(obj) && (touch_petrifies(&mons[(obj)->corpsenm]) \
                    || (obj)->corpsenm == PM_MEDUSA))

/*
 * Maybe eat a metallic object (not just gold).
 * Return value: 0 => nothing happened, 1 => monster ate something,
 * 2 => monster died (it must have grown into a genocided form, but
 * that can't happen at present because nothing which eats objects
 * has young and old forms).
 */
int
meatmetal(struct monst *mtmp)
{
    struct obj *otmp;
    struct permonst *ptr;
    char *otmpname;
    int poly, grow, heal, mstone, vis = canseemon(mtmp);

    /* If a pet, eating is handled separately, in dog.c */
    if (mtmp->mtame)
        return 0;

    /* Eats topmost metal object if it is there */
    for (otmp = g.level.objects[mtmp->mx][mtmp->my]; otmp;
         otmp = otmp->nexthere) {
        /* Don't eat indigestible/choking/inappropriate objects */
        if ((mtmp->data == &mons[PM_RUST_MONSTER] && !is_rustprone(otmp))
            || (otmp->otyp == AMULET_OF_STRANGULATION)
            || (otmp->otyp == RIN_SLOW_DIGESTION))
            continue;
        if (is_metallic(otmp) && !obj_resists(otmp, 5, 95)
            && touch_artifact(otmp, mtmp)) {
            if (mtmp->data == &mons[PM_RUST_MONSTER] && otmp->oerodeproof) {
                if (vis) {
                    /* call distant_name() for its side-effects even when
                       !verbose so won't be printed */
                    otmpname = distant_name(otmp, doname);
                    if (flags.verbose)
                        pline("%s eats %s!", Monnam(mtmp), otmpname);
                }
                /* The object's rustproofing is gone now */
                otmp->oerodeproof = 0;
                mtmp->mstun = 1;
                if (vis) {
                    /* (see above; format even if it won't be printed) */
                    otmpname = distant_name(otmp, doname);
                    if (flags.verbose)
                        pline("%s spits %s out in disgust!",
                              Monnam(mtmp), otmpname);
                }
            } else {
                if (cansee(mtmp->mx, mtmp->my)) {
                    /* (see above; format even if it won't be printed) */
                    otmpname = distant_name(otmp, doname);
                    if (flags.verbose)
                        pline("%s eats %s!", Monnam(mtmp), otmpname);
                } else {
                    if (flags.verbose)
                        You_hear("a crunching sound.");
                }
                mtmp->meating = otmp->owt / 2 + 1;
                /* Heal up to the object's weight in hp */
                if (mtmp->mhp < mtmp->mhpmax) {
                    mtmp->mhp += objects[otmp->otyp].oc_weight;
                    if (mtmp->mhp > mtmp->mhpmax)
                        mtmp->mhp = mtmp->mhpmax;
                }
                if (otmp == uball) {
                    unpunish();
                    delobj(otmp);
                } else if (otmp == uchain) {
                    unpunish(); /* frees uchain */
                } else {
                    /* these can occur via eating metal if it's a tin
                       [can't be slimed; that could only happen via glob] */
                    poly = polyfodder(otmp);
                    grow = mlevelgain(otmp);
                    heal = mhealup(otmp);
                    mstone = mstoning(otmp);
                    delobj(otmp);
                    ptr = mtmp->data;
                    if (poly) {
                        if (newcham(mtmp, (struct permonst *) 0, FALSE, vis))
                            ptr = mtmp->data;
                    } else if (grow) {
                        ptr = grow_up(mtmp, (struct monst *) 0);
                    } else if (mstone) {
                        if (poly_when_stoned(ptr)) {
                            mon_to_stone(mtmp);
                            ptr = mtmp->data;
                        } else if (!resists_ston(mtmp)) {
                            if (vis)
                                pline("%s turns to stone!", Monnam(mtmp));
                            monstone(mtmp);
                            /* might be life-saved if had a previous shape
                               which was capable to putting on an amulet */
                            if (DEADMONSTER(mtmp))
                                ptr = (struct permonst *) 0;
                        }
                    } else if (heal) {
                        mtmp->mhp = mtmp->mhpmax;
                        mcureblindness(mtmp, canseemon(mtmp));
                    }
                    if (!ptr)
                        return 2; /* it died */
                }
                /* Left behind a pile? */
                if (rnd(25) < 3)
                    (void) mksobj_at(ROCK, mtmp->mx, mtmp->my, TRUE, FALSE);
                newsym(mtmp->mx, mtmp->my);
                return 1;
            }
        }
    }
    return 0;
}

/* monster eats a pile of objects */
int
meatobj(struct monst* mtmp) /* for gelatinous cubes */
{
    struct obj *otmp, *otmp2;
    struct permonst *ptr, *original_ptr = mtmp->data;
    int poly, grow, heal, eyes, count = 0, ecount = 0, vis = canseemon(mtmp);
    char buf[BUFSZ], *otmpname;

    buf[0] = '\0';
    /* If a pet, eating is handled separately, in dog.c */
    if (mtmp->mtame)
        return 0;

    /* eat organic objects, including cloth and wood, if present;
       engulf others, except huge rocks and metal attached to player
       [despite comment at top, doesn't assume that eater is a g-cube] */
    for (otmp = g.level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
        otmp2 = otmp->nexthere;

        /* avoid special items; once hero picks them up, they'll cease
           being special, becoming eligible for engulf and devore if
           dropped again */
        if (is_mines_prize(otmp) || is_soko_prize(otmp))
            continue;

        /* touch sensitive items */
        if (otmp->otyp == CORPSE && is_rider(&mons[otmp->corpsenm])) {
            int ox = otmp->ox, oy = otmp->oy;
            boolean revived_it = revive_corpse(otmp);

            newsym(ox, oy);
            /* Rider corpse isn't just inedible; can't engulf it either */
            if (!revived_it)
                continue;
            /* [should check whether revival forced 'mtmp' off the level
               and return 3 in that situation (if possible...)] */
            break;

        /* untouchable (or inaccessible) items */
        } else if ((otmp->otyp == CORPSE
                    && touch_petrifies(&mons[otmp->corpsenm])
                    && !resists_ston(mtmp))
                   /* don't engulf boulders and statues or ball&chain */
                   || otmp->oclass == ROCK_CLASS
                   || otmp == uball || otmp == uchain
                   /* normally mtmp won't have stepped onto scare monster
                      scroll, but if it does, don't eat or engulf that
                      (note: scrolls inside eaten containers will still
                      become engulfed) */
                   || otmp->otyp == SCR_SCARE_MONSTER) {
            /* do nothing--neither eaten nor engulfed */
            continue;

        /* inedible items -- engulf these */
        } else if (!is_organic(otmp) || obj_resists(otmp, 5, 95)
                   || !touch_artifact(otmp, mtmp)
                   /* redundant due to non-organic composition but
                      included for emphasis */
                   || (otmp->otyp == AMULET_OF_STRANGULATION
                       || otmp->otyp == RIN_SLOW_DIGESTION)
                   /* cockatrice corpses handled above; this
                      touch_petrifies() check catches eggs */
                   || (mstoning(otmp) && !resists_ston(mtmp))
                   || (otmp->otyp == GLOB_OF_GREEN_SLIME
                       && !slimeproof(mtmp->data))) {
            /* engulf */
            ++ecount;
            /* call distant_name() for its possible side-effects even if
               the result won't be printed */
            otmpname = distant_name(otmp, doname);
            if (ecount == 1)
                Sprintf(buf, "%s engulfs %s.", Monnam(mtmp), otmpname);
            else if (ecount == 2)
                Sprintf(buf, "%s engulfs several objects.", Monnam(mtmp));
            obj_extract_self(otmp);
            (void) mpickobj(mtmp, otmp); /* slurp */

        /* lastly, edible items; yum! */
        } else {
            /* devour */
            ++count;
            if (cansee(mtmp->mx, mtmp->my)) {
                /* (see above; distant_name() sometimes has side-effects */
                otmpname = distant_name(otmp, doname);
                if (flags.verbose)
                    pline("%s eats %s!", Monnam(mtmp), otmpname);
                /* give this one even if !verbose */
                if (otmp->oclass == SCROLL_CLASS
                    && objdescr_is(otmp, "YUM YUM"))
                    pline("Yum%c", otmp->blessed ? '!' : '.');
                if (otmp->otyp == CORPSE)
                    mon_givit(mtmp, &mons[otmp->corpsenm]);
            } else {
                if (flags.verbose)
                    You_hear("a slurping sound.");
            }
            /* Heal up to the object's weight in hp */
            if (mtmp->mhp < mtmp->mhpmax) {
                mtmp->mhp += objects[otmp->otyp].oc_weight;
                if (mtmp->mhp > mtmp->mhpmax)
                    mtmp->mhp = mtmp->mhpmax;
            }
            if (Has_contents(otmp)) {
                register struct obj *otmp3;

                /* contents of eaten containers become engulfed; this
                   is arbitrary, but otherwise g-cubes are too powerful */
                while ((otmp3 = otmp->cobj) != 0) {
                    obj_extract_self(otmp3);
                    if (otmp->otyp == ICE_BOX && otmp3->otyp == CORPSE) {
                        otmp3->age = g.moves - otmp3->age;
                        start_corpse_timeout(otmp3);
                    }
                    (void) mpickobj(mtmp, otmp3);
                }
            }
            /* possibility of being turned to stone or into slime can't
               reach here (don't touch for cockatrice corpse, engulf rather
               than eat for tin, cockatrice egg, or glob of green slime) */
            poly = polyfodder(otmp);
            grow = mlevelgain(otmp);
            heal = mhealup(otmp);
            eyes = (otmp->otyp == CARROT);
            delobj(otmp); /* munch */
            ptr = mtmp->data;
            if (poly) {
                if (newcham(mtmp, (struct permonst *) 0, FALSE, vis))
                    ptr = mtmp->data;
            } else if (grow) {
                ptr = grow_up(mtmp, (struct monst *) 0);
            } else if (heal) {
                mtmp->mhp = mtmp->mhpmax;
            }
            if ((eyes || heal) && !mtmp->mcansee)
                mcureblindness(mtmp, canseemon(mtmp));
            /* in case it polymorphed or died */
            if (ptr != original_ptr)
                return !ptr ? 2 : 1;
        }

        /* Engulf & devour is instant, so don't set meating */
        if (mtmp->minvis)
            newsym(mtmp->mx, mtmp->my);
    }

    if (ecount > 0) {
        if (cansee(mtmp->mx, mtmp->my) && flags.verbose && buf[0])
            pline1(buf);
        else if (flags.verbose)
            You_hear("%s slurping sound%s.",
                     (ecount == 1) ? "a" : "several", plur(ecount));
    }
    return (count > 0 || ecount > 0) ? 1 : 0;
}

/* Monster eats a corpse off the ground.
 * Return value is 0 = nothing eaten, 1 = ate a corpse, 2 = died */
int
meatcorpse(struct monst* mtmp) /* for purple worms and other voracious monsters */
{
    struct obj *otmp;
    struct permonst *ptr, *original_ptr = mtmp->data, *corpsepm;
    boolean poly, grow, heal, eyes = FALSE, vis = canseemon(mtmp);
    int x = mtmp->mx, y = mtmp->my;

    /* if a pet, eating is handled separately, in dog.c */
    if (mtmp->mtame)
        return 0;

    /* skips past any globs */
    for (otmp = sobj_at(CORPSE, x, y); otmp;
         /* won't get back here if otmp is split or gets used up */
         otmp = nxtobj(otmp, CORPSE, TRUE)) {

        corpsepm = &mons[otmp->corpsenm];
        /* skip some corpses */
        if (vegan(corpsepm) /* ignore veggy corpse even if omnivorous */
            /* don't eat harmful corpses */
            || (touch_petrifies(corpsepm) && !resists_ston(mtmp)))
            continue;
        if (is_rider(corpsepm)) {
            boolean revived_it = revive_corpse(otmp);

            newsym(x, y); /* corpse is gone; mtmp might be too so do this now
                             since we're bypassing the bottom of the loop */
            if (!revived_it)
                continue; /* revival failed? if so, corpse is gone */
            /* Successful Rider revival; unlike skipped corpses, don't
               just move on to next corpse as if nothing has happened.
               [Can Rider revival bump 'mtmp' off level when it's full?
               We ought to return 3 if that happens.] */
            break;
        }

        if (otmp->quan > 1)
            otmp = splitobj(otmp, 1L);

        if (cansee(x, y) && canseemon(mtmp)) {
            /* call distant_name() for its possible side-effects even if
               the result won't be printed */
            char *otmpname = distant_name(otmp, doname);

            if (flags.verbose)
                pline("%s eats %s!", Monnam(mtmp), otmpname);
        } else {
            if (flags.verbose)
                You_hear("a masticating sound.");
        }

        mon_givit(mtmp, &mons[otmp->corpsenm]);

        /* [should include quickmimic but can't handle that unless this
           gets changed to set mtmp->meating] */
        poly = polyfodder(otmp);
        grow = mlevelgain(otmp);
        heal = mhealup(otmp);
        eyes = (otmp->otyp == CARROT); /*[always false since not a corpse]*/
        ptr = original_ptr;
        delobj(otmp);
        if (poly) {
            if (newcham(mtmp, (struct permonst *) 0, FALSE, vis))
                ptr = mtmp->data;
        } else if (grow) {
            ptr = grow_up(mtmp, (struct monst *) 0);
        } else if (heal) {
            mtmp->mhp = mtmp->mhpmax;
        }
        if ((eyes || heal) && !mtmp->mcansee)
            mcureblindness(mtmp, canseemon(mtmp));
        /* in case it polymorphed or died */
        if (ptr != original_ptr)
            return !ptr ? 2 : 1;

        /* Engulf & devour is instant, so don't set meating */
        if (mtmp->minvis)
            newsym(x, y);

        return 1;
    }
    return 0;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* Maybe give an intrinsic to a monster from eating a corpse that confers it. */
void
mon_givit(struct monst* mtmp, struct permonst* ptr)
{
    int prop = corpse_intrinsic(ptr);
    boolean vis = canseemon(mtmp);
    const char* msg = NULL;
    unsigned short intrinsic = 0; /* MR_* constant */

    if (prop == 0)
        return; /* no intrinsic from this corpse */

    if (!should_givit(prop, ptr))
        return; /* failed die roll */

    /* Pets don't have all the fields that the hero does, so they can't get all
     * the same intrinsics. If it happens to choose strength gain or teleport
     * control or whatever, ignore it. */
    switch (prop) {
    case FIRE_RES:
        intrinsic = MR_FIRE;
        msg = "%s shivers slightly.";
        break;
    case COLD_RES:
        intrinsic = MR_COLD;
        msg = "%s looks quite warm.";
        break;
    case SLEEP_RES:
        intrinsic = MR_SLEEP;
        msg = "%s looks wide awake.";
        break;
    case DISINT_RES:
        intrinsic = MR_DISINT;
        msg = "%s looks very firm.";
        break;
    case SHOCK_RES:
        intrinsic = MR_ELEC;
        msg = "%s crackles with static electricity.";
        break;
    case POISON_RES:
        intrinsic = MR_POISON;
        msg = "%s looks healthy.";
        break;
    default:
        break;
    }

    /* Don't give message if it already had this property intrinsically, but
     * still do grant the intrinsic if it only had it from mresists.
     * Do print the message if it only had this property extrinsically, which is
     * why mon_resistancebits isn't used here. */
    if ((mtmp->data->mresists | mtmp->mintrinsics) & intrinsic)
        msg = (const char *) 0;

    if (intrinsic)
        mtmp->mintrinsics |= intrinsic;

    if (vis && msg)
        pline(msg, Monnam(mtmp));
}

RESTORE_WARNING_FORMAT_NONLITERAL

void
mpickgold(register struct monst* mtmp)
{
    register struct obj *gold;
    int mat_idx;

    if ((gold = g_at(mtmp->mx, mtmp->my)) != 0) {
        mat_idx = objects[gold->otyp].oc_material;
        obj_extract_self(gold);
        add_to_minv(mtmp, gold);
        if (cansee(mtmp->mx, mtmp->my)) {
            if (flags.verbose && !mtmp->isgd)
                pline("%s picks up some %s.", Monnam(mtmp),
                      mat_idx == GOLD ? "gold" : "money");
            newsym(mtmp->mx, mtmp->my);
        }
    }
}

boolean
mpickstuff(struct monst *mtmp, const char *str)
{
    register struct obj *otmp, *otmp2, *otmp3;
    int carryamt = 0;

    /* prevent shopkeepers from leaving the door of their shop */
    if (mtmp->isshk && inhishop(mtmp))
        return FALSE;

    for (otmp = g.level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
        otmp2 = otmp->nexthere;

        /* avoid special items; once hero picks them up, they'll cease
           being special, becoming eligible for normal pickup */
        if (is_mines_prize(otmp) || is_soko_prize(otmp))
            continue;

        /* Nymphs take everything.  Most monsters don't pick up corpses. */
        if (!str ? searches_for_item(mtmp, otmp)
                 : !!(index(str, otmp->oclass))) {
            if (otmp->otyp == CORPSE && mtmp->data->mlet != S_NYMPH
                /* let a handful of corpse types thru to can_carry() */
                && !touch_petrifies(&mons[otmp->corpsenm])
                && otmp->corpsenm != PM_LIZARD
                && !acidic(&mons[otmp->corpsenm]))
                continue;
            if (!touch_artifact(otmp, mtmp))
                continue;
            carryamt = can_carry(mtmp, otmp);
            if (carryamt == 0)
                continue;
            if (is_pool(mtmp->mx, mtmp->my))
                continue;
            /* handle cases where the critter can only get some */
            otmp3 = otmp;
            if (carryamt != otmp->quan) {
                otmp3 = splitobj(otmp, carryamt);
            }
            if (cansee(mtmp->mx, mtmp->my)) {
                /* call distant_name() for its possible side-effects even
                   if the result won't be printed; do it before the extract
                   from floor and subsequent pickup by mtmp */
                char *otmpname = distant_name(otmp, doname);

                if (flags.verbose)
                    pline("%s picks up %s.", Monnam(mtmp), otmpname);
            }
            obj_extract_self(otmp3);      /* remove from floor */
            (void) mpickobj(mtmp, otmp3); /* may merge and free otmp3 */
            m_dowear(mtmp, FALSE);
            newsym(mtmp->mx, mtmp->my);
            return TRUE; /* pick only one object */
        }
    }
    return FALSE;
}

int
curr_mon_load(struct monst* mtmp)
{
    int curload = 0;
    struct obj *obj;

    for (obj = mtmp->minvent; obj; obj = obj->nobj) {
        if (obj->otyp != BOULDER || !throws_rocks(mtmp->data))
            curload += obj->owt;
    }

    return curload;
}

int
max_mon_load(struct monst* mtmp)
{
    long maxload;

    /* Base monster carrying capacity is equal to human maximum
     * carrying capacity, or half human maximum if not strong.
     * (for a polymorphed player, the value used would be the
     * non-polymorphed carrying capacity instead of max/half max).
     * This is then modified by the ratio between the monster weights
     * and human weights.  Corpseless monsters are given a capacity
     * proportional to their size instead of weight.
     */
    if (!mtmp->data->cwt)
        maxload = (MAX_CARR_CAP * (long) mtmp->data->msize) / MZ_HUMAN;
    else if (!strongmonst(mtmp->data)
             || (strongmonst(mtmp->data) && (mtmp->data->cwt > WT_HUMAN)))
        maxload = (MAX_CARR_CAP * (long) mtmp->data->cwt) / WT_HUMAN;
    else
        maxload = MAX_CARR_CAP; /*strong monsters w/cwt <= WT_HUMAN*/

    if (!strongmonst(mtmp->data))
        maxload /= 2;

    if (maxload < 1)
        maxload = 1;

    return (int) maxload;
}

/* for restricting monsters' object-pickup.
 *
 * to support the new pet behavior, this now returns the max # of objects
 * that a given monster could pick up from a pile. frequently this will be
 * otmp->quan, but special cases for 'only one' now exist so.
 *
 * this will probably cause very amusing behavior with pets and gold coins.
 *
 * TODO: allow picking up 2-N objects from a pile of N based on weight.
 *       Change from 'int' to 'long' to accomate big stacks of gold.
 *       Right now we fake it by reporting a partial quantity, but the
 *       likesgold handling m_move results in picking up the whole stack.
 */
int
can_carry(struct monst* mtmp, struct obj* otmp)
{
    int iquan, otyp = otmp->otyp, newload = otmp->owt;
    struct permonst *mdat = mtmp->data;
    short nattk = 0;

    if (notake(mdat))
        return 0; /* can't carry anything */

    if (otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm])
        && !(mtmp->misc_worn_check & W_ARMG) && !resists_ston(mtmp))
        return 0;
    if (otyp == CORPSE && is_rider(&mons[otmp->corpsenm]))
        return 0;
    if (objects[otyp].oc_material == SILVER && mon_hates_silver(mtmp)
        && (otyp != BELL_OF_OPENING || !is_covetous(mdat)))
        return 0;

    /* hostile monsters who like gold will pick up the whole stack;
       tame mosnters with hands will pick up the partial stack */
    iquan = (otmp->quan > (long) LARGEST_INT)
               ? 20000 + rn2(LARGEST_INT - 20000 + 1)
               : (int) otmp->quan;

    /* monsters without hands can't pick up multiple objects at once
     * unless they have an engulfing attack
     *
     * ...dragons, of course, can always carry gold pieces and gems somehow
     */
    if (iquan > 1) {
        boolean glomper = FALSE;

        if (mtmp->data->mlet == S_DRAGON
            && (otmp->oclass == COIN_CLASS
                || otmp->oclass == GEM_CLASS))
            glomper = TRUE;
        else
            for (nattk = 0; nattk < NATTK; nattk++)
                if (mtmp->data->mattk[nattk].aatyp == AT_ENGL) {
                    glomper = TRUE;
                    break;
                }
        if ((mtmp->data->mflags1 & M1_NOHANDS) && !glomper)
            return 1;
    }

    /* steeds don't pick up stuff (to avoid shop abuse) */
    if (mtmp == u.usteed)
        return 0;
    if (mtmp->isshk)
        return iquan; /* no limit */
    if (mtmp->mpeaceful && !mtmp->mtame)
        return 0;
    /* otherwise players might find themselves obligated to violate
     * their alignment if the monster takes something they need
     */

    /* special--boulder throwers carry unlimited amounts of boulders */
    if (throws_rocks(mdat) && otyp == BOULDER)
        return iquan;

    /* nymphs deal in stolen merchandise, but not boulders or statues */
    if (mdat->mlet == S_NYMPH)
        return (otmp->oclass == ROCK_CLASS) ? 0 : iquan;

    if (curr_mon_load(mtmp) + newload > max_mon_load(mtmp))
        return 0;

    return iquan;
}

/* is <nx,ny> in direct line with where 'mon' thinks hero is? */
static boolean
monlineu(struct monst *mon, int nx, int ny)
{
    return online2(nx, ny, mon->mux, mon->muy);
}

/* return flags based on monster data, for mfndpos() */
long
mon_allowflags(struct monst* mtmp)
{
    long allowflags = 0L;
    boolean can_open = !(nohands(mtmp->data) || verysmall(mtmp->data));
    boolean can_unlock = ((can_open && monhaskey(mtmp, TRUE))
                          || mtmp->iswiz || is_rider(mtmp->data));
    boolean doorbuster = is_giant(mtmp->data);
    /* don't tunnel if on rogue level or if hostile and close enough
       to prefer a weapon; same criteria as in m_move() */
    boolean can_tunnel = (tunnels(mtmp->data) && !Is_rogue_level(&u.uz));

    if (can_tunnel && needspick(mtmp->data)
        && ((!mtmp->mpeaceful || Conflict)
            && dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 8))
        can_tunnel = FALSE;

    if (mtmp->mtame)
        allowflags |= ALLOW_M | ALLOW_TRAPS | ALLOW_SANCT | ALLOW_SSM;
    else if (mtmp->mpeaceful)
        allowflags |= ALLOW_SANCT | ALLOW_SSM;
    else
        allowflags |= ALLOW_U;
    if (Conflict && !resist_conflict(mtmp))
        allowflags |= ALLOW_U;
    if (mtmp->isshk)
        allowflags |= ALLOW_SSM;
    if (mtmp->ispriest)
        allowflags |= ALLOW_SSM | ALLOW_SANCT;
    if (passes_walls(mtmp->data))
        allowflags |= (ALLOW_ROCK | ALLOW_WALL);
    if (throws_rocks(mtmp->data))
        allowflags |= ALLOW_ROCK;
    if (can_tunnel)
        allowflags |= ALLOW_DIG;
    if (doorbuster)
        allowflags |= BUSTDOOR;
    if (can_open)
        allowflags |= OPENDOOR;
    if (can_unlock)
        allowflags |= UNLOCKDOOR;
    if (passes_bars(mtmp->data))
        allowflags |= ALLOW_BARS;
#if 0   /* can't do this here; leave it for mfndpos() */
    if (is_displacer(mtmp->data))
        allowflags |= ALLOW_MDISP;
#endif
    if (is_minion(mtmp->data) || is_rider(mtmp->data))
        allowflags |= ALLOW_SANCT;
    /* unicorn may not be able to avoid hero on a noteleport level */
    if (is_unicorn(mtmp->data) && !noteleport_level(mtmp))
        allowflags |= NOTONL;
    if (is_human(mtmp->data) || mtmp->data == &mons[PM_MINOTAUR])
        allowflags |= ALLOW_SSM;
    if ((is_undead(mtmp->data) && mtmp->data->mlet != S_GHOST)
        || is_vampshifter(mtmp))
        allowflags |= NOGARLIC;

    return allowflags;
}

/* return number of acceptable neighbour positions */
int
mfndpos(
    struct monst *mon,
    coord *poss, /* coord poss[9] */
    long *info,  /* long info[9] */
    long flag)
{
    struct permonst *mdat = mon->data;
    register struct trap *ttmp;
    xchar x, y, nx, ny;
    int cnt = 0;
    uchar ntyp;
    uchar nowtyp;
    boolean wantpool, poolok, lavaok, nodiag;
    boolean rockok = FALSE, treeok = FALSE, thrudoor;
    int maxx, maxy;
    boolean poisongas_ok, in_poisongas;
    NhRegion *gas_reg;
    int gas_glyph = cmap_to_glyph(S_poisoncloud);

    x = mon->mx;
    y = mon->my;
    nowtyp = levl[x][y].typ;

    nodiag = NODIAG(mdat - mons);
    wantpool = (mdat->mlet == S_EEL);
    poolok = ((!Is_waterlevel(&u.uz) && !grounded(mdat))
              || (is_swimmer(mdat) && !wantpool));
    /* note: floating eye is the only is_floater() so this could be
       simplified, but then adding another floater would be error prone */
    lavaok = (!grounded(mdat) || likes_lava(mdat));
    if (mdat == &mons[PM_FLOATING_EYE]) /* prefers to avoid heat */
        lavaok = FALSE;
    thrudoor = ((flag & (ALLOW_WALL | BUSTDOOR)) != 0L);
    poisongas_ok = (m_poisongas_ok(mon) == M_POISONGAS_OK);
    in_poisongas = ((gas_reg = visible_region_at(x,y)) != 0
                    && gas_reg->glyph == gas_glyph);

    if (flag & ALLOW_DIG) {
        struct obj *mw_tmp;

        /* need to be specific about what can currently be dug */
        if (!needspick(mdat)) {
            rockok = treeok = TRUE;
        } else if ((mw_tmp = MON_WEP(mon)) && mw_tmp->cursed
                   && mon->weapon_check == NO_WEAPON_WANTED) {
            rockok = is_pick(mw_tmp);
            treeok = is_axe(mw_tmp);
        } else {
            rockok = (m_carrying(mon, PICK_AXE)
                      || (m_carrying(mon, DWARVISH_MATTOCK)
                          && !which_armor(mon, W_ARMS)));
            treeok = (m_carrying(mon, AXE) || (m_carrying(mon, BATTLE_AXE)
                                               && !which_armor(mon, W_ARMS)));
        }
        if (rockok || treeok)
            thrudoor = TRUE;
    }

 nexttry: /* eels prefer the water, but if there is no water nearby,
             they will crawl over land */
    if (mon->mconf) {
        flag |= ALLOW_ALL;
        flag &= ~NOTONL;
    }
    if (!mon->mcansee)
        flag |= ALLOW_SSM;
    maxx = min(x + 1, COLNO - 1);
    maxy = min(y + 1, ROWNO - 1);
    for (nx = max(1, x - 1); nx <= maxx; nx++)
        for (ny = max(0, y - 1); ny <= maxy; ny++) {
            if (nx == x && ny == y)
                continue;
            ntyp = levl[nx][ny].typ;
            if (IS_ROCK(ntyp)
                && !((flag & ALLOW_WALL) && may_passwall(nx, ny))
                && !((IS_TREE(ntyp) ? treeok : rockok) && may_dig(nx, ny)))
                continue;
            if (IS_WATERWALL(ntyp) && !is_swimmer(mdat))
                continue;
            /* KMH -- Added iron bars */
            if (ntyp == IRONBARS
                && (!(flag & ALLOW_BARS)
                    || ((levl[nx][ny].wall_info & W_NONDIGGABLE)
                        && (dmgtype(mdat, AD_RUST)
                            || dmgtype(mdat, AD_CORR)))))
                continue;
            if (IS_DOOR(ntyp) && !(amorphous(mdat) || can_fog(mon))
                && (((levl[nx][ny].doormask & D_CLOSED) && !(flag & OPENDOOR))
                    || ((levl[nx][ny].doormask & D_LOCKED)
                        && !(flag & UNLOCKDOOR))) && !thrudoor)
                continue;
            /* avoid poison gas? */
            if (!poisongas_ok && !in_poisongas
                && (gas_reg = visible_region_at(nx,ny)) != 0
                && gas_reg->glyph == gas_glyph)
                continue;
            /* first diagonal checks (tight squeezes handled below) */
            if (nx != x && ny != y
                && (nodiag
                    || (IS_DOOR(nowtyp) && (levl[x][y].doormask & ~D_BROKEN))
                    || (IS_DOOR(ntyp) && (levl[nx][ny].doormask & ~D_BROKEN))
                    || ((IS_DOOR(nowtyp) || IS_DOOR(ntyp))
                        && Is_rogue_level(&u.uz))
                    /* mustn't pass between adjacent long worm segments,
                       but can attack that way */
                    || (m_at(x, ny) && m_at(nx, y) && worm_cross(x, y, nx, ny)
                        && !m_at(nx, ny) && (nx != u.ux || ny != u.uy))))
                continue;
            if ((poolok || is_pool(nx, ny) == wantpool)
                && (lavaok || !is_lava(nx, ny))) {
                int dispx, dispy;
                boolean monseeu = (mon->mcansee
                                   && (!Invis || perceives(mdat)));
                boolean checkobj = OBJ_AT(nx, ny);

                /* Displacement also displaces the Elbereth/scare monster,
                 * as long as you are visible.
                 */
                if (Displaced && monseeu && mon->mux == nx && mon->muy == ny) {
                    dispx = u.ux;
                    dispy = u.uy;
                } else {
                    dispx = nx;
                    dispy = ny;
                }

                info[cnt] = 0;
                if (onscary(dispx, dispy, mon)) {
                    if (!(flag & ALLOW_SSM))
                        continue;
                    info[cnt] |= ALLOW_SSM;
                }
                if (u_at(nx, ny)
                    || (nx == mon->mux && ny == mon->muy)) {
                    if (u_at(nx, ny)) {
                        /* If it's right next to you, it found you,
                         * displaced or no.  We must set mux and muy
                         * right now, so when we return we can tell
                         * that the ALLOW_U means to attack _you_ and
                         * not the image.
                         */
                        mon->mux = u.ux;
                        mon->muy = u.uy;
                    }
                    if (!(flag & ALLOW_U))
                        continue;
                    info[cnt] |= ALLOW_U;
                } else {
                    if (MON_AT(nx, ny)) {
                        struct monst *mtmp2 = m_at(nx, ny);
                        long mmflag = flag | mm_aggression(mon, mtmp2);

                        if (mmflag & ALLOW_M) {
                            info[cnt] |= ALLOW_M;
                            if (mtmp2->mtame) {
                                if (!(mmflag & ALLOW_TM))
                                    continue;
                                info[cnt] |= ALLOW_TM;
                            }
                        } else {
                            flag &= ~ALLOW_MDISP; /* depends upon defender */
                            mmflag = flag | mm_displacement(mon, mtmp2);
                            if (!(mmflag & ALLOW_MDISP))
                                continue;
                            info[cnt] |= ALLOW_MDISP;
                        }
                    }
                    /* Note: ALLOW_SANCT only prevents movement, not
                       attack, into a temple. */
                    if (g.level.flags.has_temple && *in_rooms(nx, ny, TEMPLE)
                        && !*in_rooms(x, y, TEMPLE)
                        && in_your_sanctuary((struct monst *) 0, nx, ny)) {
                        if (!(flag & ALLOW_SANCT))
                            continue;
                        info[cnt] |= ALLOW_SANCT;
                    }
                }
                if (checkobj && sobj_at(CLOVE_OF_GARLIC, nx, ny)) {
                    if (flag & NOGARLIC)
                        continue;
                    info[cnt] |= NOGARLIC;
                }
                if (checkobj && sobj_at(BOULDER, nx, ny)) {
                    if (!(flag & ALLOW_ROCK))
                        continue;
                    info[cnt] |= ALLOW_ROCK;
                }
                if (monseeu && monlineu(mon, nx, ny)) {
                    if (flag & NOTONL)
                        continue;
                    info[cnt] |= NOTONL;
                }
                /* check for diagonal tight squeeze */
                if (nx != x && ny != y && bad_rock(mdat, x, ny)
                    && bad_rock(mdat, nx, y) && cant_squeeze_thru(mon))
                    continue;
                /* The monster avoids a particular type of trap if it's
                 * familiar with the trap type.  Pets get ALLOW_TRAPS
                 * and checking is done in dogmove.c.  In either case,
                 * "harmless" traps are neither avoided nor marked in info[].
                 */
                if ((ttmp = t_at(nx, ny)) != 0) {
                    if (ttmp->ttyp >= TRAPNUM || ttmp->ttyp == 0) {
                        impossible(
                         "A monster looked at a very strange trap of type %d.",
                                   ttmp->ttyp);
                            continue;
                    }
                    if ((ttmp->ttyp != RUST_TRAP
                         || mdat == &mons[PM_IRON_GOLEM])
                        && ttmp->ttyp != STATUE_TRAP
                        && ttmp->ttyp != VIBRATING_SQUARE
                        && ((!is_pit(ttmp->ttyp) && !is_hole(ttmp->ttyp))
                            || (!is_flyer(mdat) && !is_floater(mdat)
                                && !is_clinger(mdat)) || Sokoban)
                        && (ttmp->ttyp != SLP_GAS_TRAP || !resists_sleep(mon))
                        && (ttmp->ttyp != BEAR_TRAP
                            || (mdat->msize > MZ_SMALL && !amorphous(mdat)
                                && !is_flyer(mdat) && !is_floater(mdat)
                                && !is_whirly(mdat) && !unsolid(mdat)))
                        && (ttmp->ttyp != FIRE_TRAP || !resists_fire(mon))
                        && (ttmp->ttyp != SQKY_BOARD || !is_flyer(mdat))
                        && (ttmp->ttyp != WEB
                            || (!amorphous(mdat) && !webmaker(mdat)
                                && !is_whirly(mdat) && !unsolid(mdat)))
                        && (ttmp->ttyp != ANTI_MAGIC || !resists_magm(mon))) {
                        if (!(flag & ALLOW_TRAPS)) {
                            if (mon->mtrapseen & (1L << (ttmp->ttyp - 1)))
                                continue;
                        }
                        info[cnt] |= ALLOW_TRAPS;
                    }
                }
                poss[cnt].x = nx;
                poss[cnt].y = ny;
                cnt++;
            }
        }
    if (!cnt && wantpool && !is_pool(x, y)) {
        wantpool = FALSE;
        goto nexttry;
    }
    return cnt;
}

/* Part of mm_aggression that represents two-way aggression.  To avoid
   having to code each case twice, this function contains those cases that
   ought to happen twice, and mm_aggression will call it twice. */
static long
mm_2way_aggression(struct monst *magr, struct monst *mdef)
{
    /* zombies vs things that can be zombified */
    if (zombie_maker(magr) && zombie_form(mdef->data) != NON_PM)
        return (ALLOW_M | ALLOW_TM);

    return 0;
}

/* Monster against monster special attacks; for the specified monster
   combinations, this allows one monster to attack another adjacent one
   in the absence of Conflict.  There is no provision for targetting
   other monsters; just hand to hand fighting when they happen to be
   next to each other. */
static long
mm_aggression(
    struct monst *magr, /* monster that is currently deciding where to move */
    struct monst *mdef) /* another monster which is next to it */
{
    int mndx = monsndx(magr->data);

    /* don't allow pets to fight each other */
    if (magr->mtame && mdef->mtame)
        return 0;

    /* supposedly purple worms are attracted to shrieking because they
       like to eat shriekers, so attack the latter when feasible */
    if ((mndx == PM_PURPLE_WORM || mndx == PM_BABY_PURPLE_WORM)
        && mdef->data == &mons[PM_SHRIEKER])
        return ALLOW_M | ALLOW_TM;
    /* Various other combinations such as dog vs cat, cat vs rat, and
       elf vs orc have been suggested.  For the time being we don't
       support those. */
    return (mm_2way_aggression(magr, mdef) | mm_2way_aggression(mdef, magr));
}

/* Monster displacing another monster out of the way */
static long
mm_displacement(
    struct monst *magr, /* monster that is currently deciding where to move */
    struct monst *mdef) /* another monster which is next to it */
{
    struct permonst *pa = magr->data, *pd = mdef->data;

    /* if attacker can't barge through, there's nothing to do;
       or if defender can barge through too and has a level at least
       as high as the attacker, don't let attacker do so, otherwise
       they might just end up swapping places again when defender
       gets its chance to move */
    if (is_displacer(pa) && (!is_displacer(pd) || magr->m_lev > mdef->m_lev)
        /* no displacing grid bugs diagonally */
        && !(magr->mx != mdef->mx && magr->my != mdef->my
             && NODIAG(monsndx(pd)))
        /* no displacing trapped monsters or multi-location longworms */
        && !mdef->mtrapped && (!mdef->wormno || !count_wsegs(mdef))
        /* riders can move anything; others, same size or smaller only */
        && (is_rider(pa) || pa->msize >= pd->msize))
        return ALLOW_MDISP;
    return 0L;
}

/* Is the square close enough for the monster to move or attack into? */
boolean
monnear(struct monst *mon, int x, int y)
{
    int distance = dist2(mon->mx, mon->my, x, y);

    if (distance == 2 && NODIAG(mon->data - mons))
        return 0;
    return (boolean) (distance < 3);
}

/* really free dead monsters */
void
dmonsfree(void)
{
    struct monst **mtmp, *freetmp;
    int count = 0;
    char buf[QBUFSZ];

    buf[0] = '\0';
    for (mtmp = &fmon; *mtmp;) {
        freetmp = *mtmp;
        if (DEADMONSTER(freetmp) && !freetmp->isgd) {
            *mtmp = freetmp->nmon;
            freetmp->nmon = NULL;
            dealloc_monst(freetmp);
            count++;
        } else
            mtmp = &(freetmp->nmon);
    }

    if (count != iflags.purge_monsters) {
        describe_level(buf, 2);
        impossible("dmonsfree: %d removed doesn't match %d pending on %s",
                   count, iflags.purge_monsters, buf);
    }
    iflags.purge_monsters = 0;
}

/* called when monster is moved to larger structure */
void
replmon(struct monst *mtmp, struct monst *mtmp2)
{
    struct obj *otmp;

    /* transfer the monster's inventory */
    for (otmp = mtmp2->minvent; otmp; otmp = otmp->nobj) {
        if (otmp->where != OBJ_MINVENT || otmp->ocarry != mtmp)
            impossible("replmon: minvent inconsistency");
        otmp->ocarry = mtmp2;
    }
    mtmp->minvent = 0;
    /* before relmon(mtmp), because it could clear polearm.hitmon */
    if (g.context.polearm.hitmon == mtmp)
        g.context.polearm.hitmon = mtmp2;

    /* remove the old monster from the map and from `fmon' list */
    relmon(mtmp, (struct monst **) 0);

    /* finish adding its replacement */
    if (mtmp != u.usteed) /* don't place steed onto the map */
        place_monster(mtmp2, mtmp2->mx, mtmp2->my);
    if (mtmp2->wormno)      /* update level.monsters[wseg->wx][wseg->wy] */
        place_wsegs(mtmp2, mtmp); /* locations to mtmp2 not mtmp. */
    if (emits_light(mtmp2->data)) {
        /* since this is so rare, we don't have any `mon_move_light_source' */
        new_light_source(mtmp2->mx, mtmp2->my, emits_light(mtmp2->data),
                         LS_MONSTER, monst_to_any(mtmp2));
        /* here we rely on fact that `mtmp' hasn't actually been deleted */
        del_light_source(LS_MONSTER, monst_to_any(mtmp));
    }
    mtmp2->nmon = fmon;
    fmon = mtmp2;
    if (u.ustuck == mtmp)
        set_ustuck(mtmp2);
    if (u.usteed == mtmp)
        u.usteed = mtmp2;
    if (mtmp2->isshk)
        replshk(mtmp, mtmp2);

    /* discard the old monster */
    dealloc_monst(mtmp);
}

/* release mon from the display and the map's monster list,
   maybe transfer it to one of the other monster lists */
void
relmon(
    struct monst *mon,
    struct monst **monst_list) /* &g.migrating_mons or &g.mydogs or null */
{
    if (!fmon)
        panic("relmon: no fmon available.");

    /* take 'mon' off the map */
    mon_leaving_level(mon);

    /* remove 'mon' from the 'fmon' list */
    if (mon == fmon) {
        fmon = fmon->nmon;
    } else {
        struct monst *mtmp;

        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            if (mtmp->nmon == mon) {
                mtmp->nmon = mon->nmon;
                break;
            }
        if (!mtmp)
            panic("relmon: mon not in list.");
    }

    if (monst_list) {
        /* insert into g.mydogs or g.migrating_mons */
        mon->nmon = *monst_list;
        *monst_list = mon;
    } else {
        /* orphan has no next monster */
        mon->nmon = 0;
    }
}

void
copy_mextra(struct monst* mtmp2, struct monst* mtmp1)
{
    if (!mtmp2 || !mtmp1 || !mtmp1->mextra)
        return;

    if (!mtmp2->mextra)
        mtmp2->mextra = newmextra();
    if (MGIVENNAME(mtmp1)) {
        new_mgivenname(mtmp2, (int) strlen(MGIVENNAME(mtmp1)) + 1);
        Strcpy(MGIVENNAME(mtmp2), MGIVENNAME(mtmp1));
    }
    if (EGD(mtmp1)) {
        if (!EGD(mtmp2))
            newegd(mtmp2);
        *EGD(mtmp2) = *EGD(mtmp1);
    }
    if (EPRI(mtmp1)) {
        if (!EPRI(mtmp2))
            newepri(mtmp2);
        *EPRI(mtmp2) = *EPRI(mtmp1);
    }
    if (ESHK(mtmp1)) {
        if (!ESHK(mtmp2))
            neweshk(mtmp2);
        *ESHK(mtmp2) = *ESHK(mtmp1);
    }
    if (EMIN(mtmp1)) {
        if (!EMIN(mtmp2))
            newemin(mtmp2);
        *EMIN(mtmp2) = *EMIN(mtmp1);
    }
    if (EDOG(mtmp1)) {
        if (!EDOG(mtmp2))
            newedog(mtmp2);
        *EDOG(mtmp2) = *EDOG(mtmp1);
    }
    if (has_mcorpsenm(mtmp1))
        MCORPSENM(mtmp2) = MCORPSENM(mtmp1);
}

void
dealloc_mextra(struct monst* m)
{
    struct mextra *x = m->mextra;

    if (x) {
        if (x->mgivenname)
            free((genericptr_t) x->mgivenname);
        if (x->egd)
            free((genericptr_t) x->egd);
        if (x->epri)
            free((genericptr_t) x->epri);
        if (x->eshk)
            free((genericptr_t) x->eshk);
        if (x->emin)
            free((genericptr_t) x->emin);
        if (x->edog)
            free((genericptr_t) x->edog);
        /* [no action needed for x->mcorpsenm] */

        free((genericptr_t) x);
        m->mextra = (struct mextra *) 0;
    }
}

void
dealloc_monst(struct monst *mon)
{
    char buf[QBUFSZ];

    buf[0] = '\0';
    if (mon->nmon) {
        describe_level(buf, 2);
        panic("dealloc_monst with nmon on %s", buf);
    }
    if (mon->mextra)
        dealloc_mextra(mon);
    free((genericptr_t) mon);
}

/* 'mon' is being removed from level due to migration [relmon from keepdogs
   or migrate_to_level] or due to death [m_detach from mondead or mongone] */
static void
mon_leaving_level(struct monst *mon)
{
    int mx = mon->mx, my = mon->my;
    boolean onmap = (mx > 0 && mon != u.usteed);

    /* to prevent an infinite relobj-flooreffects-hmon-killed loop */
    mon->mtrapped = 0;
    unstuck(mon); /* mon is not swallowing or holding you nor held by you */

    /* vault guard might be at <0,0> */
    if (onmap || mon == g.level.monsters[0][0]) {
        if (mon->wormno)
            remove_worm(mon);
        else
            remove_monster(mx, my);
    }
    if (onmap) {
        mon->mundetected = 0; /* for migration; doesn't matter for death */
        /* unhide mimic in case its shape has been blocking line of sight
           or it is accompanying the hero to another level */
        if (M_AP_TYPE(mon) != M_AP_NOTHING && M_AP_TYPE(mon) != M_AP_MONSTER)
            seemimic(mon);
        /* if mon is pinned by a boulder, removing mon lets boulder drop */
        fill_pit(mx, my);
        newsym(mx, my);
    }
    /* if mon is a remembered target, forget it since it isn't here anymore */
    if (mon == g.context.polearm.hitmon)
        g.context.polearm.hitmon = (struct monst *) 0;
}

/* 'mtmp' is going away; remove effects of mtmp from other data structures */
static void
m_detach(
    struct monst *mtmp,
    struct permonst *mptr) /* reflects mtmp->data _prior_ to mtmp's death */
{
    if (mtmp->mleashed)
        m_unleash(mtmp, FALSE);

    if (mtmp->mx > 0 && emits_light(mptr))
        del_light_source(LS_MONSTER, monst_to_any(mtmp));

    /* take mtmp off map but not out of fmon list yet (dmonsfree does that) */
    mon_leaving_level(mtmp);

    mtmp->mhp = 0; /* simplify some tests: force mhp to 0 */
    if (mtmp->iswiz)
        wizdead();
    if (mtmp->data->msound == MS_NEMESIS)
        nemdead();
    if (mtmp->data->msound == MS_LEADER)
        leaddead();
    if (mtmp->m_id == g.stealmid)
        thiefdead();
    /* release (drop onto map) all objects carried by mtmp */
    relobj(mtmp, 0, FALSE);

    if (mtmp->isshk)
        shkgone(mtmp);
    if (mtmp->wormno)
        wormgone(mtmp);
    if (In_endgame(&u.uz))
        mtmp->mstate |= MON_ENDGAME_FREE;

    mtmp->mstate |= MON_DETACH;
    iflags.purge_monsters++;
}

/* give a life-saved monster a reasonable mhpmax value in case it has
   been the victim of excessive life draining */
static void
set_mon_min_mhpmax(
    struct monst *mon,
    int minimum_mhpmax) /* monster life-saving has traditionally used 10 */
{
    /* can't be less than m_lev+1 (if we just used m_lev itself, level 0
       monsters would end up allowing a minimum of 0); since life draining
       reduces m_lev, this usually won't give the monster much of a boost */
    if (mon->mhpmax < (int) mon->m_lev + 1)
        mon->mhpmax = (int) mon->m_lev + 1;
    /* caller can specify an alternate minimum; we'll honor it iff it is
       greater than m_lev+1; the traditional arbitrary value of 10 always
       gives level 0 and level 1 monsters a boost and has a moderate
       chance of doing so for level 2, a tiny chance for levels 3..9 */
    if (mon->mhpmax < minimum_mhpmax)
        mon->mhpmax = minimum_mhpmax;
}

/* find the worn amulet of life saving which will save a monster */
struct obj *
mlifesaver(struct monst* mon)
{
    if (!nonliving(mon->data) || is_vampshifter(mon)) {
        struct obj *otmp = which_armor(mon, W_AMUL);

        if (otmp && otmp->otyp == AMULET_OF_LIFE_SAVING)
            return otmp;
    }
    return (struct obj *) 0;
}

static void
lifesaved_monster(struct monst* mtmp)
{
    boolean surviver;
    struct obj *lifesave = mlifesaver(mtmp);

    if (lifesave) {
        /* not canseemon; amulets are on the head, so you don't want
         * to show this for a long worm with only a tail visible.
         * Nor do you check invisibility, because glowing and
         * disintegrating amulets are always visible. */
        if (cansee(mtmp->mx, mtmp->my)) {
            pline("But wait...");
            pline("%s medallion begins to glow!", s_suffix(Monnam(mtmp)));
            makeknown(AMULET_OF_LIFE_SAVING);
            /* amulet is visible, but monster might not be */
            if (canseemon(mtmp)) {
                if (attacktype(mtmp->data, AT_EXPL)
                    || attacktype(mtmp->data, AT_BOOM))
                    pline("%s reconstitutes!", Monnam(mtmp));
                else
                    pline("%s looks much better!", Monnam(mtmp));
            }
            pline_The("medallion crumbles to dust!");
        }
        m_useup(mtmp, lifesave);
        /* equip replacement amulet, if any, on next move */
        mtmp->misc_worn_check |= I_SPECIAL;

        surviver = !(g.mvitals[monsndx(mtmp->data)].mvflags & G_GENOD);
        mtmp->mcanmove = 1;
        mtmp->mfrozen = 0;
        if (mtmp->mtame && !mtmp->isminion) {
            wary_dog(mtmp, !surviver);
        }
        set_mon_min_mhpmax(mtmp, 10); /* mtmp->mhpmax=max(mtmp->m_lev+1,10) */
        mtmp->mhp = mtmp->mhpmax;

        if (!surviver) {
            /* genocided monster can't be life-saved */
            if (cansee(mtmp->mx, mtmp->my))
                pline("Unfortunately, %s is still genocided...",
                      mon_nam(mtmp));
            mtmp->mhp = 0;
        }
    }
}

DISABLE_WARNING_FORMAT_NONLITERAL

void
mondead(register struct monst* mtmp)
{
    struct permonst *mptr;
    boolean be_sad;
    int mndx;

    /* potential pet message; always clear global flag */
    be_sad = iflags.sad_feeling;
    iflags.sad_feeling = FALSE;

    mtmp->mhp = 0; /* in case caller hasn't done this */
    lifesaved_monster(mtmp);
    if (!DEADMONSTER(mtmp))
        return;

    if (is_vampshifter(mtmp)) {
        /* this only happens if shapeshifted */
        mndx = mtmp->cham;
        if (mndx >= LOW_PM && mndx != monsndx(mtmp->data)
            && !(g.mvitals[mndx].mvflags & G_GENOD)) {
            coord new_xy;
            char buf[BUFSZ];
            boolean in_door = (amorphous(mtmp->data)
                               && closed_door(mtmp->mx, mtmp->my)),
                /* alternate message phrasing for some monster types */
                spec_mon = (nonliving(mtmp->data)
                            || noncorporeal(mtmp->data)
                            || amorphous(mtmp->data)),
                spec_death = (g.disintegested /* disintegrated or digested */
                              || noncorporeal(mtmp->data)
                              || amorphous(mtmp->data));
            int x = mtmp->mx, y = mtmp->my;

            /* construct a format string before transformation;
               will be capitalized when used, expects one %s arg */
            Sprintf(buf, "%s suddenly %s and rises as %%s!",
                    x_monnam(mtmp, ARTICLE_THE,
                             spec_mon ? (char *) 0 : "seemingly dead",
                             (SUPPRESS_INVISIBLE | SUPPRESS_IT), FALSE),
                    spec_death ? "reconstitutes" : "transforms");
            mtmp->mcanmove = 1;
            mtmp->mfrozen = 0;
            set_mon_min_mhpmax(mtmp, 10); /* mtmp->mhpmax=max(m_lev+1,10) */
            mtmp->mhp = mtmp->mhpmax;
            /* mtmp==u.ustuck can happen if previously a fog cloud
               or poly'd hero is hugging a vampire bat */
            if (mtmp == u.ustuck) {
                if (u.uswallow)
                    expels(mtmp, mtmp->data, FALSE);
                else
                    uunstick();
            }
            if (in_door) {
                if (enexto(&new_xy, mtmp->mx, mtmp->my, &mons[mndx]))
                    rloc_to(mtmp, new_xy.x, new_xy.y);
            }
            (void) newcham(mtmp, &mons[mndx], FALSE, FALSE);
            if (mtmp->data == &mons[mndx])
                mtmp->cham = NON_PM;
            else
                mtmp->cham = mndx;
            if (canspotmon(mtmp)) {
                /* 3.6.0 used a_monnam(mtmp); that was weird if mtmp was
                   named: "Dracula suddenly transforms and rises as Dracula";
                   3.6.1 used mtmp->data->mname; that ignored hallucination */
                pline(upstart(buf),
                      x_monnam(mtmp, ARTICLE_A, (char *) 0,
                               (SUPPRESS_NAME | SUPPRESS_IT
                                | SUPPRESS_INVISIBLE), FALSE));
                g.vamp_rise_msg = TRUE;
            }
            newsym(x, y);
            return;
        }
    }

    if (be_sad)
        You("have a sad feeling for a moment, then it passes.");

    /* dead vault guard is actually kept at coordinate <0,0> until
       his temporary corridor to/from the vault has been removed;
       need to do this after life-saving and before m_detach() */
    if (mtmp->isgd && !grddead(mtmp))
        return;

    /* Player is thrown from his steed when it dies */
    if (mtmp == u.usteed)
        dismount_steed(DISMOUNT_GENERIC);

    mptr = mtmp->data; /* save this for m_detach() */
    /* restore chameleon, lycanthropes to true form at death */
    if (mtmp->cham >= LOW_PM) {
        set_mon_data(mtmp, &mons[mtmp->cham]);
        mtmp->cham = NON_PM;
    } else if (mtmp->data == &mons[PM_WEREJACKAL])
        set_mon_data(mtmp, &mons[PM_HUMAN_WEREJACKAL]);
    else if (mtmp->data == &mons[PM_WEREWOLF])
        set_mon_data(mtmp, &mons[PM_HUMAN_WEREWOLF]);
    else if (mtmp->data == &mons[PM_WERERAT])
        set_mon_data(mtmp, &mons[PM_HUMAN_WERERAT]);

    /*
     * g.mvitals[].died does double duty as total number of dead monsters
     * and as experience factor for the player killing more monsters.
     * this means that a dragon dying by other means reduces the
     * experience the player gets for killing a dragon directly; this
     * is probably not too bad, since the player likely finagled the
     * first dead dragon via ring of conflict or pets, and extinguishing
     * based on only player kills probably opens more avenues of abuse
     * for rings of conflict and such.
     */
    mndx = monsndx(mtmp->data);
    if (g.mvitals[mndx].died < 255)
        g.mvitals[mndx].died++;

    /* if it's a (possibly polymorphed) quest leader, mark him as dead */
    if (mtmp->m_id == g.quest_status.leader_m_id)
        g.quest_status.leader_is_dead = TRUE;
#ifdef MAIL_STRUCTURES
    /* if the mail daemon dies, no more mail delivery.  -3. */
    if (mndx == PM_MAIL_DAEMON)
        g.mvitals[mndx].mvflags |= G_GENOD;
#endif

    if (mtmp->data->mlet == S_KOP) {
        stairway *stway = stairway_find_type_dir(FALSE, FALSE);

        /* Dead Kops may come back. */
        switch (rnd(5)) {
        case 1: /* returns near the stairs */
            if (stway) {
                (void) makemon(mtmp->data, stway->sx, stway->sy, NO_MM_FLAGS);
                break;
            }
            /* fall-through */
        case 2: /* randomly */
            (void) makemon(mtmp->data, 0, 0, NO_MM_FLAGS);
            break;
        default:
            break;
        }
    }
#if 0   /* moved to m_detach() to kick in if mongone() happens */
    if (mtmp->iswiz)
        wizdead();
    if (mtmp->data->msound == MS_NEMESIS)
        nemdead();
#endif

    if (mndx == PM_MEDUSA && g.mvitals[mndx].died == 1) {
        record_achievement(ACH_MEDU); /* also generates a livelog event */
    } else if (unique_corpstat(mtmp->data)) {
        /*
         * livelog event; unique_corpstat() includes the Wizard and
         * any High Priest even though they aren't actually unique.
         *
         * It would be nice to include shopkeepers.  Their names are
         * unique within each game but unfortunately for this potential
         * usage their kill count is lumped together in a group total.
         */
        int howmany = g.mvitals[mndx].died;

        /* killing a unique more than once doesn't get logged every time;
           the Wizard and the Riders can be killed more than once
           "naturally", others require deliberate player action such as
           use of undead turning to revive a corpse or petrification plus
           stone-to-flesh to create and revive a statue */
        if (howmany <= 3 || howmany == 5 || howmany == 10 || howmany == 25
            || (howmany % 50) == 0) { /* 50, 100, 150, 200, 250 */
            char xtra[40];
            long llevent_type = LL_UMONST;

            /* first kill of any unique is a major event; all kills of the
               Wizard and the Riders are major if they're logged but they
               still don't get logged every time */
            if (howmany == 1 || mtmp->iswiz || is_rider(mtmp->data))
                llevent_type |= LL_ACHIEVE;
            xtra[0] = '\0';
            if (howmany > 1) /* "(2nd time)" or "(50th time)" */
                Sprintf(xtra, " (%d%s time)", howmany, ordin(howmany));

            livelog_printf(llevent_type, "%s %s%s",
                           nonliving(mtmp->data) ? "destroyed" : "killed",
                           livelog_mon_nam(mtmp), xtra);
        }
    }

    if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph))
        unmap_object(mtmp->mx, mtmp->my);
    m_detach(mtmp, mptr);
    return;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* TRUE if corpse might be dropped, magr may die if mon was swallowed */
boolean
corpse_chance(
    struct monst *mon,
    struct monst *magr,    /* killer, if swallowed */
    boolean was_swallowed) /* digestion */
{
    struct permonst *mdat = mon->data;
    int i, tmp;

    if (mdat == &mons[PM_VLAD_THE_IMPALER] || mdat->mlet == S_LICH) {
        if (cansee(mon->mx, mon->my) && !was_swallowed)
            pline("%s body crumbles into dust.", s_suffix(Monnam(mon)));
        return FALSE;
    }

    /* Gas spores always explode upon death */
    for (i = 0; i < NATTK; i++) {
        if (mdat->mattk[i].aatyp == AT_BOOM) {
            if (mdat->mattk[i].damn)
                tmp = d((int) mdat->mattk[i].damn, (int) mdat->mattk[i].damd);
            else if (mdat->mattk[i].damd)
                tmp = d((int) mdat->mlevel + 1, (int) mdat->mattk[i].damd);
            else
                tmp = 0;
            if (was_swallowed && magr) {
                if (magr == &g.youmonst) {
                    There("is an explosion in your %s!", body_part(STOMACH));
                    Sprintf(g.killer.name, "%s explosion",
                            s_suffix(pmname(mdat, Mgender(mon))));
                    losehp(Maybe_Half_Phys(tmp), g.killer.name, KILLED_BY_AN);
                } else {
                    You_hear("an explosion.");
                    magr->mhp -= tmp;
                    if (DEADMONSTER(magr))
                        mondied(magr);
                    if (DEADMONSTER(magr)) { /* maybe lifesaved */
                        if (canspotmon(magr))
                            pline("%s rips open!", Monnam(magr));
                    } else if (canseemon(magr))
                        pline("%s seems to have indigestion.", Monnam(magr));
                }

                return FALSE;
            }

            mon_explodes(mon, &mdat->mattk[i]);
            return FALSE;
        }
    }

    /* must duplicate this below check in xkilled() since it results in
     * creating no objects as well as no corpse
     */
    if (LEVEL_SPECIFIC_NOCORPSE(mdat))
        return FALSE;

    if (((bigmonst(mdat) || mdat == &mons[PM_LIZARD]) && !mon->mcloned)
        || is_golem(mdat) || is_mplayer(mdat) || is_rider(mdat) || mon->isshk)
        return TRUE;
    tmp = 2 + ((mdat->geno & G_FREQ) < 2) + verysmall(mdat);
    return (boolean) !rn2(tmp);
}

/* drop (perhaps) a cadaver and remove monster */
void
mondied(register struct monst* mdef)
{
    mondead(mdef);
    if (!DEADMONSTER(mdef))
        return; /* lifesaved */

    if (corpse_chance(mdef, (struct monst *) 0, FALSE)
        && (accessible(mdef->mx, mdef->my) || is_pool(mdef->mx, mdef->my)))
        (void) make_corpse(mdef, CORPSTAT_NONE);
}

/* monster disappears, not dies */
void
mongone(struct monst* mdef)
{
    mdef->mhp = 0; /* can skip some inventory bookkeeping */

    /* dead vault guard is actually kept at coordinate <0,0> until
       his temporary corridor to/from the vault has been removed */
    if (mdef->isgd && !grddead(mdef))
        return;
    /* hero is thrown from his steed when it disappears */
    if (mdef == u.usteed)
        dismount_steed(DISMOUNT_GENERIC);
    /* stuck to you? release */
    unstuck(mdef);
    /* drop special items like the Amulet so that a dismissed Kop or nurse
       can't remove them from the game */
    mdrop_special_objs(mdef);
    /* release rest of monster's inventory--it is removed from game */
    discard_minvent(mdef, FALSE);
    m_detach(mdef, mdef->data);
}

/* drop a statue or rock and remove monster */
void
monstone(struct monst* mdef)
{
    struct obj *otmp, *obj, *oldminvent;
    xchar x = mdef->mx, y = mdef->my;
    boolean wasinside = FALSE;

    /* vampshifter reverts to vampire;
       3.6.3: also used to unshift shape-changed sandestin */
    if (!vamp_stone(mdef))
        return;

    /* we have to make the statue before calling mondead, to be able to
     * put inventory in it, and we have to check for lifesaving before
     * making the statue....
     */
    mdef->mhp = 0; /* in case caller hasn't done this */
    lifesaved_monster(mdef);
    if (!DEADMONSTER(mdef))
        return;

    mdef->mtrapped = 0; /* (see m_detach) */

    if ((int) mdef->data->msize > MZ_TINY
        || !rn2(2 + ((int) (mdef->data->geno & G_FREQ) > 2))) {
        unsigned corpstatflags = CORPSTAT_NONE;

        oldminvent = 0;
        /* some objects may end up outside the statue */
        while ((obj = mdef->minvent) != 0) {
            extract_from_minvent(mdef, obj, TRUE, TRUE);
            if (obj->otyp == BOULDER
#if 0 /* monsters don't carry statues */
                ||  (obj->otyp == STATUE
                     && mons[obj->corpsenm].msize >= mdef->data->msize)
#endif
                /* invocation tools resist even with 0% resistance */
                || obj_resists(obj, 0, 0)) {
                if (flooreffects(obj, x, y, "fall"))
                    continue;
                place_object(obj, x, y);
            } else {
                if (obj->lamplit)
                    end_burn(obj, TRUE);
                obj->nobj = oldminvent;
                oldminvent = obj;
            }
        }
        /* defer statue creation until after inventory removal
           so that saved monster traits won't retain any stale
           item-conferred attributes */
        if (mdef->female)
            corpstatflags |= CORPSTAT_FEMALE;
        else if (!is_neuter(mdef->data))
            corpstatflags |= CORPSTAT_MALE;
        /* Archeologists should not break unique statues */
        if (mdef->data->geno & G_UNIQ)
            corpstatflags |= CORPSTAT_HISTORIC;
        otmp = mkcorpstat(STATUE, mdef, mdef->data, x, y, corpstatflags);
        if (has_mgivenname(mdef))
            otmp = oname(otmp, MGIVENNAME(mdef), ONAME_NO_FLAGS);
        while ((obj = oldminvent) != 0) {
            oldminvent = obj->nobj;
            obj->nobj = 0; /* avoid merged-> obfree-> dealloc_obj-> panic */
            (void) add_to_container(otmp, obj);
        }
        otmp->owt = weight(otmp);
    } else
        otmp = mksobj_at(ROCK, x, y, TRUE, FALSE);

    stackobj(otmp);
    /* mondead() already does this, but we must do it before the newsym */
    if (glyph_is_invisible(levl[x][y].glyph))
        unmap_object(x, y);
    if (cansee(x, y))
        newsym(x, y);
    /* We don't currently trap the hero in the statue in this case but we
     * could */
    if (engulfing_u(mdef))
        wasinside = TRUE;
    mondead(mdef);
    if (wasinside) {
        if (is_animal(mdef->data))
            You("%s through an opening in the new %s.",
                u_locomotion("jump"), xname(otmp));
    }
}

/* another monster has killed the monster mdef */
void
monkilled(
    struct monst *mdef,
    const char *fltxt,
    int how)
{
    struct permonst *mptr = mdef->data;

    if (fltxt && (mdef->wormno ? worm_known(mdef)
                               : cansee(mdef->mx, mdef->my)))
        pline("%s is %s%s%s!", Monnam(mdef),
              nonliving(mptr) ? "destroyed" : "killed",
              *fltxt ? " by the " : "", fltxt);
    else
        /* sad feeling is deferred until after potential life-saving */
        iflags.sad_feeling = mdef->mtame ? TRUE : FALSE;

    /* no corpse if digested or disintegrated or flammable golem burnt up;
       no corpse for a paper golem means no scrolls; golems that rust or
       rot completely are described as "falling to pieces" so they do
       leave a corpse (which means staves for wood golem, leather armor for
       leather golem, iron chains for iron golem, not a regular corpse) */
    g.disintegested = (how == AD_DGST || how == -AD_RBRE
                       || (how == AD_FIRE && completelyburns(mptr)));
    if (g.disintegested)
        mondead(mdef); /* never leaves a corpse */
    else
        mondied(mdef); /* calls mondead() and maybe leaves a corpse */

    if (!DEADMONSTER(mdef))
        return; /* life-saved */
    /* extra message if pet golem is completely destroyed;
       if not visible, this will follow "you have a sad feeling" */
    if (mdef->mtame) {
        const char *rxt = (how == AD_FIRE && completelyburns(mptr)) ? "roast"
                          : (how == AD_RUST && completelyrusts(mptr)) ? "rust"
                            : (how == AD_DCAY && completelyrots(mptr)) ? "rot"
                              :  0;
        if (rxt)
            pline("May %s %s in peace.", noit_mon_nam(mdef), rxt);
    }
}

void
set_ustuck(struct monst* mtmp)
{
    if (iflags.sanity_check || iflags.debug_fuzzer) {
        if (mtmp && !next2u(mtmp->mx, mtmp->my))
            impossible("Sticking to %s at distu %d?",
                       mon_nam(mtmp), distu(mtmp->mx, mtmp->my));
    }

    g.context.botl = 1;
    u.ustuck = mtmp;
}

void
unstuck(struct monst* mtmp)
{
    if (u.ustuck == mtmp) {
        struct permonst *ptr = mtmp->data;

        /* do this first so that docrt()'s botl update is accurate;
           safe to do as long as u.uswallow is also cleared before docrt() */
        set_ustuck((struct monst *) 0);

        if (u.uswallow) {
            u.ux = mtmp->mx;
            u.uy = mtmp->my;
            u.uswallow = 0;
            u.uswldtim = 0;
            if (Punished && uchain->where != OBJ_FLOOR)
                placebc();
            g.vision_full_recalc = 1;
            docrt();
        }

        /* prevent holder/engulfer from immediately re-holding/re-engulfing
           [note: this call to unstuck() might be because u.ustuck has just
           changed shape and doesn't have a holding attack any more, hence
           don't set mspec_used uncondtionally] */
        if (!mtmp->mspec_used && (dmgtype(ptr, AD_STCK)
                                  || attacktype(ptr, AT_ENGL)
                                  || attacktype(ptr, AT_HUGS)))
            mtmp->mspec_used = rnd(2);
    }
}

void
killed(struct monst* mtmp)
{
    xkilled(mtmp, XKILL_GIVEMSG);
}

/* the player has killed the monster mtmp */
void
xkilled(
    struct monst *mtmp,
    int xkill_flags) /* 1: suppress message, 2: suppress corpse, 4: pacifist */
{
    int tmp, mndx, x = mtmp->mx, y = mtmp->my;
    struct permonst *mdat;
    struct obj *otmp;
    struct trap *t;
    boolean be_sad;
    boolean wasinside = engulfing_u(mtmp),
            burycorpse = FALSE,
            nomsg = (xkill_flags & XKILL_NOMSG) != 0,
            nocorpse = (xkill_flags & XKILL_NOCORPSE) != 0,
            noconduct = (xkill_flags & XKILL_NOCONDUCT) != 0;

    /* potential pet message; always clear global flag */
    be_sad = iflags.sad_feeling;
    iflags.sad_feeling = FALSE;

    mtmp->mhp = 0; /* caller will usually have already done this */
    if (!noconduct) /* KMH, conduct */
        if (!u.uconduct.killer++)
            livelog_printf(LL_CONDUCT, "killed for the first time");

    if (!nomsg) {
        boolean namedpet = has_mgivenname(mtmp) && !Hallucination;

        You("%s %s!",
            nonliving(mtmp->data) ? "destroy" : "kill",
            !(wasinside || canspotmon(mtmp)) ? "it"
              : !mtmp->mtame ? mon_nam(mtmp)
                : x_monnam(mtmp, namedpet ? ARTICLE_NONE : ARTICLE_THE,
                           "poor", namedpet ? SUPPRESS_SADDLE : 0, FALSE));
    }

    if (mtmp->mtrapped && (t = t_at(x, y)) != 0 && is_pit(t->ttyp)) {
        if (sobj_at(BOULDER, x, y))
            nocorpse = TRUE; /* Prevent corpses/treasure being created
                              * "on top" of boulder that is about to fall in.
                              * This is out of order, but cannot be helped
                              * unless this whole routine is rearranged. */
        if (m_carrying(mtmp, BOULDER))
            burycorpse = TRUE;
    }

    /* your pet knows who just killed it...watch out */
    if (mtmp->mtame && !mtmp->isminion)
        EDOG(mtmp)->killed_by_u = 1;

    if (wasinside && g.thrownobj && g.thrownobj != uball
        /* don't give to mon if missile is going to be destroyed */
        && g.thrownobj->oclass != POTION_CLASS
        /* don't give to mon if missile is going to return to hero */
        && g.thrownobj != (struct obj *) iflags.returning_missile) {
        /* thrown object has killed hero's engulfer; add it to mon's
           inventory now so that it will be placed with mon's other
           stuff prior to lookhere/autopickup when hero is expelled
           below (as a side-effect, this missile has immunity from
           being consumed [for this shot/throw only]) */
        mpickobj(mtmp, g.thrownobj);
        /* let throwing code know that missile has been disposed of */
        g.thrownobj = 0;
    }

    g.vamp_rise_msg = FALSE; /* might get set in mondead(); checked below */
    g.disintegested = nocorpse; /* alternate vamp_rise mesg needed if true */
    /* dispose of monster and make cadaver */
    if (g.stoned)
        monstone(mtmp);
    else
        mondead(mtmp);
    g.disintegested = FALSE; /* reset */

    if (!DEADMONSTER(mtmp)) { /* monster lifesaved */
        /* Cannot put the non-visible lifesaving message in
         * lifesaved_monster() since the message appears only when _you_
         * kill it (as opposed to visible lifesaving which always appears).
         */
        g.stoned = FALSE;
        if (!cansee(x, y) && !g.vamp_rise_msg)
            pline("Maybe not...");
        return;
    }

    if (be_sad)
        You("have a sad feeling for a moment, then it passes.");

    mdat = mtmp->data; /* note: mondead can change mtmp->data */
    mndx = monsndx(mdat);

    if (g.stoned) {
        g.stoned = FALSE;
        goto cleanup;
    }

    if (nocorpse || LEVEL_SPECIFIC_NOCORPSE(mdat))
        goto cleanup;

#ifdef MAIL_STRUCTURES
    if (mdat == &mons[PM_MAIL_DAEMON]) {
        stackobj(mksobj_at(SCR_MAIL, x, y, FALSE, FALSE));
    }
#endif
    if (accessible(x, y) || is_pool(x, y)) {
        struct obj *cadaver;
        int otyp;

        /* illogical but traditional "treasure drop" */
        if (!rn2(6) && !(g.mvitals[mndx].mvflags & G_NOCORPSE)
            /* no extra item from swallower or steed */
            && (x != u.ux || y != u.uy)
            /* no extra item from kops--too easy to abuse */
            && mdat->mlet != S_KOP
            /* no items from cloned monsters */
            && !mtmp->mcloned) {
            otmp = mkobj(RANDOM_CLASS, TRUE);
            /* don't create large objects from small monsters */
            otyp = otmp->otyp;
            if (mdat->msize < MZ_HUMAN && otyp != FIGURINE
                /* oc_big is also oc_bimanual and oc_bulky */
                && (otmp->owt > 30 || objects[otyp].oc_big)) {
                if (otmp->oartifact) /* un-create */
                    artifact_exists(otmp, safe_oname(otmp), FALSE,
                                    ONAME_NO_FLAGS);
                delobj(otmp);
            } else if (!flooreffects(otmp, x, y, nomsg ? "" : "fall")) {
                place_object(otmp, x, y);
                stackobj(otmp);
            }
        }
        /* corpse--none if hero was inside the monster */
        if (!wasinside && corpse_chance(mtmp, (struct monst *) 0, FALSE)) {
            g.zombify = (!g.thrownobj && !g.stoned && !uwep
                         && zombie_maker(&g.youmonst)
                         && zombie_form(mtmp->data) != NON_PM);
            cadaver = make_corpse(mtmp, burycorpse ? CORPSTAT_BURIED
                                                   : CORPSTAT_NONE);
            g.zombify = FALSE; /* reset */
            if (burycorpse && cadaver && cansee(x, y) && !mtmp->minvis
                && cadaver->where == OBJ_BURIED && !nomsg) {
                pline("%s corpse ends up buried.", s_suffix(Monnam(mtmp)));
            }
        }
    }
    if (wasinside)
        spoteffects(TRUE); /* poor man's expels() */
    /* monster is gone, corpse or other object might now be visible */
    newsym(x, y);

 cleanup:
    /* punish bad behaviour */
    if (is_human(mdat)
        && (!always_hostile(mdat) && mtmp->malign <= 0)
        && (mndx < PM_ARCHEOLOGIST || mndx > PM_WIZARD)
        && u.ualign.type != A_CHAOTIC) {
        HTelepat &= ~INTRINSIC;
        change_luck(-2);
        You("murderer!");
        if (Blind && !Blind_telepat)
            see_monsters(); /* Can't sense monsters any more. */
    }
    if ((mtmp->mpeaceful && !rn2(2)) || mtmp->mtame)
        change_luck(-1);
    if (is_unicorn(mdat) && sgn(u.ualign.type) == sgn(mdat->maligntyp)) {
        change_luck(-5);
        You_feel("guilty...");
    }

    /* give experience points */
    tmp = experience(mtmp, (int) g.mvitals[mndx].died);
    more_experienced(tmp, 0);
    newexplevel(); /* will decide if you go up */

    /* adjust alignment points */
    if (mtmp->m_id == g.quest_status.leader_m_id) { /* REAL BAD! */
        adjalign(-(u.ualign.record + (int) ALIGNLIM / 2));
        u.ugangr += 7; /* instantly become "extremely" angry */
        change_luck(-20);
        pline("That was %sa bad idea...",
              u.uevent.qcompleted ? "probably " : "");
    } else if (mdat->msound == MS_NEMESIS) { /* Real good! */
        if (!g.quest_status.killed_leader)
            adjalign((int) (ALIGNLIM / 4));
    } else if (mdat->msound == MS_GUARDIAN) { /* Bad */
        adjalign(-(int) (ALIGNLIM / 8));
        u.ugangr++;
        change_luck(-4);
        if (!Hallucination)
            pline("That was probably a bad idea...");
        else
            pline("Whoopsie-daisy!");
    } else if (mtmp->ispriest) {
        adjalign((p_coaligned(mtmp)) ? -2 : 2);
        /* cancel divine protection for killing your priest */
        if (p_coaligned(mtmp))
            u.ublessed = 0;
        if (mdat->maligntyp == A_NONE)
            adjalign((int) (ALIGNLIM / 4)); /* BIG bonus */
    } else if (mtmp->mtame) {
        adjalign(-15); /* bad!! */
        /* your god is mighty displeased... */
        if (!Hallucination)
            You_hear("the rumble of distant thunder...");
        else
            You_hear("the studio audience applaud!");
        if (!unique_corpstat(mdat)) {
            boolean mname = has_mgivenname(mtmp);

            livelog_printf(LL_KILLEDPET, "murdered %s%s%s faithful %s",
                           mname ? MGIVENNAME(mtmp) : "",
                           mname ? ", " : "",
                           uhis(), pmname(mdat, Mgender(mtmp)));
        }
    } else if (mtmp->mpeaceful)
        adjalign(-5);

    /* malign was already adjusted for u.ualign.type and randomization */
    adjalign(mtmp->malign);
}

/* changes the monster into a stone monster of the same type
   this should only be called when poly_when_stoned() is true */
void
mon_to_stone(struct monst* mtmp)
{
    if (mtmp->data->mlet == S_GOLEM) {
        /* it's a golem, and not a stone golem */
        if (canseemon(mtmp))
            pline("%s solidifies...", Monnam(mtmp));
        if (newcham(mtmp, &mons[PM_STONE_GOLEM], FALSE, FALSE)) {
            if (canseemon(mtmp))
                pline("Now it's %s.", an(pmname(mtmp->data, Mgender(mtmp))));
        } else {
            if (canseemon(mtmp))
                pline("... and returns to normal.");
        }
    } else
        impossible("Can't polystone %s!", a_monnam(mtmp));
}

boolean
vamp_stone(struct monst* mtmp)
{
    if (is_vampshifter(mtmp)) {
        int mndx = mtmp->cham;
        int x = mtmp->mx, y = mtmp->my;

        /* this only happens if shapeshifted */
        if (mndx >= LOW_PM && mndx != monsndx(mtmp->data)
            && !(g.mvitals[mndx].mvflags & G_GENOD)) {
            char buf[BUFSZ];
            boolean in_door = (amorphous(mtmp->data)
                               && closed_door(mtmp->mx, mtmp->my));

            /* construct a format string before transformation */
            Sprintf(buf, "The lapidifying %s %s %s",
                    x_monnam(mtmp, ARTICLE_NONE, (char *) 0,
                             (SUPPRESS_SADDLE | SUPPRESS_HALLUCINATION
                              | SUPPRESS_INVISIBLE | SUPPRESS_IT), FALSE),
                    amorphous(mtmp->data) ? "coalesces on the"
                       : is_flyer(mtmp->data) ? "drops to the"
                          : "writhes on the",
                    surface(x,y));
            mtmp->mcanmove = 1;
            mtmp->mfrozen = 0;
            set_mon_min_mhpmax(mtmp, 10); /* mtmp->mhpmax=max(m_lev+1,10) */
            mtmp->mhp = mtmp->mhpmax;
            /* this can happen if previously a fog cloud */
            if (engulfing_u(mtmp))
                expels(mtmp, mtmp->data, FALSE);
            if (in_door) {
                coord new_xy;

                if (enexto(&new_xy, mtmp->mx, mtmp->my, &mons[mndx])) {
                    rloc_to(mtmp, new_xy.x, new_xy.y);
                }
            }
            if (canspotmon(mtmp)) {
                pline("%s!", buf);
                display_nhwindow(WIN_MESSAGE, FALSE);
            }
            (void) newcham(mtmp, &mons[mndx], FALSE, FALSE);
            if (mtmp->data == &mons[mndx])
                mtmp->cham = NON_PM;
            else
                mtmp->cham = mndx;
            if (canspotmon(mtmp)) {
                pline("%s rises from the %s with renewed agility!",
                      Amonnam(mtmp), surface(mtmp->mx, mtmp->my));
            }
            newsym(mtmp->mx, mtmp->my);
            return FALSE;   /* didn't petrify */
        }
    } else if (mtmp->cham >= LOW_PM
               && (mons[mtmp->cham].mresists & MR_STONE)) {
        /* sandestins are stoning-immune so if hit by stoning damage
           they revert to innate shape rather than become a statue */
        mtmp->mcanmove = 1;
        mtmp->mfrozen = 0;
        set_mon_min_mhpmax(mtmp, 10); /* mtmp->mhpmax=max(mtmp->m_lev+1,10) */
        mtmp->mhp = mtmp->mhpmax;
        (void) newcham(mtmp, &mons[mtmp->cham], FALSE, TRUE);
        newsym(mtmp->mx, mtmp->my);
        return FALSE;   /* didn't petrify */
    }
    return TRUE;
}

/* drop monster into "limbo" - that is, migrate to the current level */
void
m_into_limbo(struct monst *mtmp)
{
    xchar target_lev = ledger_no(&u.uz), xyloc = MIGR_APPROX_XY;

    mtmp->mstate |= MON_LIMBO;
    migrate_mon(mtmp, target_lev, xyloc);
}

static void
migrate_mon(struct monst *mtmp, xchar target_lev, xchar xyloc)
{
    unstuck(mtmp);
    mdrop_special_objs(mtmp);
    migrate_to_level(mtmp, target_lev, xyloc, (coord *) 0);
}

static boolean
ok_to_obliterate(struct monst *mtmp)
{
    /*
     * Add checks for monsters that should not be obliterated
     * here (return FALSE).
     */
    if (mtmp->data == &mons[PM_WIZARD_OF_YENDOR] || is_rider(mtmp->data)
        || has_emin(mtmp) || has_epri(mtmp) || has_eshk(mtmp)
        || mtmp == u.ustuck || mtmp == u.usteed)
        return FALSE;
    return TRUE;
}

void
elemental_clog(struct monst *mon)
{
    static long msgmv = 0L;
    int m_lev = 0;
    struct monst *mtmp, *m1, *m2, *m3, *m4, *m5, *zm;

    if (In_endgame(&u.uz)) {
        m1 = m2 = m3 = m4 = m5 = zm = (struct monst *) 0;
        if (!msgmv || (g.moves - msgmv) > 200L) {
            if (!msgmv || rn2(2))
                You_feel("besieged.");
            msgmv = g.moves;
        }
        /*
         * m1 an elemental from another plane.
         * m2 an elemental from this plane.
         * m3 the least powerful monst encountered in loop so far.
         * m4 some other non-tame monster.
         * m5 a pet.
         */
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp) || mtmp == mon)
                continue;
            if (mtmp->mx == 0 && mtmp->my == 0)
                continue;
            if (mon_has_amulet(mtmp) || !ok_to_obliterate(mtmp))
                continue;
            if (mtmp->data->mlet == S_ELEMENTAL) {
                if (!is_home_elemental(mtmp->data)) {
                    if (!m1)
                        m1 = mtmp;
                } else {
                    if (!m2)
                        m2 = mtmp;
                }
            } else {
                if (!mtmp->mtame) {
                    if (!m_lev || mtmp->m_lev < m_lev) {
                        m_lev = mtmp->m_lev;
                        m3 = mtmp;
                    } else if (!m4) {
                        m4 = mtmp;
                    }
                } else {
                    if (!m5)
                        m5 = mtmp;
                    break;
                }
            }
        }
        mtmp = m1 ? m1 : m2 ? m2 : m3 ? m3 : m4 ? m4 : m5 ? m5 : zm;
        if (mtmp) {
            int mx = mtmp->mx, my = mtmp->my;

            mtmp->mstate |= MON_OBLITERATE;
            mongone(mtmp);
            /* places in the code might still reference mtmp->mx, mtmp->my */
            /* mtmp->mx = mtmp->my = 0; */
            rloc_to(mon, mx, my);           /* note: mon, not mtmp */

        /* last resort - migrate mon to the next plane */
        } else if (!Is_astralevel(&u.uz)) {
            d_level dest;
            xchar target_lev;

            dest = u.uz;
            dest.dlevel--;
            target_lev = ledger_no(&dest);
            mon->mstate |= MON_ENDGAME_MIGR;
            migrate_mon(mon, target_lev, MIGR_RANDOM);
        }
    }
}

/* make monster mtmp next to you (if possible);
   might place monst on far side of a wall or boulder */
void
mnexto(struct monst* mtmp, unsigned int rlocflags)
{
    coord mm;

    if (mtmp == u.usteed) {
        /* Keep your steed in sync with you instead */
        mtmp->mx = u.ux;
        mtmp->my = u.uy;
        return;
    }

    if (!enexto(&mm, u.ux, u.uy, mtmp->data) || !isok(mm.x, mm.y)) {
        deal_with_overcrowding(mtmp);
        return;
    }
    rloc_to_flag(mtmp, mm.x, mm.y, rlocflags);
    return;
}

static void
deal_with_overcrowding(struct monst* mtmp)
{
    if (In_endgame(&u.uz)) {
        debugpline1("overcrowding: elemental_clog on %s", m_monnam(mtmp));
        elemental_clog(mtmp);
    } else {
        debugpline1("overcrowding: sending %s into limbo", m_monnam(mtmp));
        m_into_limbo(mtmp);
    }
}

/* like mnexto() but requires destination to be directly accessible */
void
maybe_mnexto(struct monst* mtmp)
{
    coord mm;
    struct permonst *ptr = mtmp->data;
    boolean diagok = !NODIAG(ptr - mons);
    int tryct = 20;

    do {
        if (!enexto(&mm, u.ux, u.uy, ptr))
            return;
        if (couldsee(mm.x, mm.y)
            /* don't move grid bugs diagonally */
            && (diagok || mm.x == mtmp->mx || mm.y == mtmp->my)) {
            rloc_to(mtmp, mm.x, mm.y);
            return;
        }
    } while (--tryct > 0);
}

/* mnearto()
 * Put monster near (or at) location if possible.
 * Returns:
 *  2 if another monster was moved out of this one's way;
 *  1 if relocation was successful (without moving another one);
 *  0 otherwise.
 * Note: if already at the target spot, result is 1 rather than 0.
 *
 * Might be called recursively if 'move_other' is True; if so, that argument
 * will be False on the nested call so there won't be any further recursion.
 */
int
mnearto(
    register struct monst *mtmp,
    xchar x,
    xchar y,
    boolean move_other, /* make sure mtmp gets to x, y! so move m_at(x, y) */
    unsigned int rlocflags)
{
    struct monst *othermon = (struct monst *) 0;
    xchar newx, newy;
    coord mm;
    int res = 1;

    if (mtmp->mx == x && mtmp->my == y && m_at(x, y) == mtmp)
        return res;

    if (move_other && (othermon = m_at(x, y)) != 0) {
        if (othermon->wormno)
            remove_worm(othermon);
        else
            remove_monster(x, y);

        othermon->mx = othermon->my = 0; /* 'othermon' is not on the map */
        othermon->mstate |= MON_OFFMAP;
    }

    newx = x;
    newy = y;
    if (!goodpos(newx, newy, mtmp, 0)) {
        /* Actually we have real problems if enexto ever fails.
         * Migrating_mons that need to be placed will cause
         * no end of trouble.
         */
        if (!enexto(&mm, newx, newy, mtmp->data) || !isok(mm.x, mm.y)) {
            if (othermon) {
                /* othermon already had its mx, my set to 0 above
                 * and this would shortly cause a sanity check to fail
                 * if we just return 0 here. The caller only possesses
                 * awareness of mtmp, not othermon. */
                deal_with_overcrowding(othermon);
            }
            return 0;
        }
        newx = mm.x;
        newy = mm.y;
    }
    rloc_to_flag(mtmp, newx, newy, rlocflags);

    if (move_other && othermon) {
        res = 2; /* moving another monster out of the way */
        if (!mnearto(othermon, x, y, FALSE, rlocflags))  /* no 'move_other' this time */
            deal_with_overcrowding(othermon);
    }

    return res;
}

/* monster responds to player action; not the same as a passive attack;
   assumes reason for response has been tested, and response _must_ be made */
void
m_respond(struct monst* mtmp)
{
    if (mtmp->data->msound == MS_SHRIEK) {
        if (!Deaf) {
            pline("%s shrieks.", Monnam(mtmp));
            stop_occupation();
        }
        if (!rn2(10)) {
            if (!rn2(13)) {
                /* don't generate purple worms if too difficult */
                int pm = montoostrong(PM_PURPLE_WORM, monmax_difficulty_lev())
                         ? PM_BABY_PURPLE_WORM : PM_PURPLE_WORM;

                (void) makemon(&mons[pm], 0, 0, NO_MM_FLAGS);
            } else
                (void) makemon((struct permonst *) 0, 0, 0, NO_MM_FLAGS);
        }
        aggravate();
    }
    if (mtmp->data == &mons[PM_MEDUSA]) {
        register int i;

        for (i = 0; i < NATTK; i++)
            if (mtmp->data->mattk[i].aatyp == AT_GAZE) {
                (void) gazemu(mtmp, &mtmp->data->mattk[i]);
                break;
            }
    }
}

/* Called whenever the player attacks mtmp; also called in other situations
   where mtmp gets annoyed at the player. Handles mtmp getting annoyed at the
   attack and any ramifications that might have. Useful also in situations
   where mtmp was already hostile; it checks for situations where the player
   shouldn't be attacking and any ramifications /that/ might have. */
void
setmangry(struct monst* mtmp, boolean via_attack)
{
    if (via_attack && sengr_at("Elbereth", u.ux, u.uy, TRUE)
        /* only hypocritical if monster is vulnerable to Elbereth (or
           peaceful--not vulnerable but attacking it is hypocritical) */
        && (onscary(u.ux, u.uy, mtmp) || mtmp->mpeaceful)) {
        You_feel("like a hypocrite.");
        /* AIS: Yes, I know alignment penalties and bonuses aren't balanced
           at the moment. This is about correct relative to other "small"
           penalties; it should be fairly large, as attacking while standing
           on an Elbereth means that you're requesting peace and then
           violating your own request. I know 5 isn't actually large, but
           it's intentionally larger than the 1s and 2s that are normally
           given for this sort of thing. */
        /* reduce to 3 (average) when alignment is already very low */
        adjalign((u.ualign.record > 5) ? -5 : -rnd(5));

        if (!Blind)
            pline("The engraving beneath you fades.");
        del_engr_at(u.ux, u.uy);
    }

    /* AIS: Should this be in both places, or just in wakeup()? */
    mtmp->mstrategy &= ~STRAT_WAITMASK;
    if (!mtmp->mpeaceful)
        return;
    /* [FIXME: this logic seems wrong; peaceful humanoids gasp or exclaim
       when they see you attack a peaceful monster but they just casually
       look the other way when you attack a pet?] */
    if (mtmp->mtame)
        return;
    mtmp->mpeaceful = 0;
    if (mtmp->ispriest) {
        if (p_coaligned(mtmp))
            adjalign(-5); /* very bad */
        else
            adjalign(2);
    } else
        adjalign(-1); /* attacking peaceful monsters is bad */
    if (humanoid(mtmp->data) || mtmp->isshk || mtmp->isgd) {
        if (couldsee(mtmp->mx, mtmp->my))
            pline("%s gets angry!", Monnam(mtmp));
    } else {
        growl(mtmp);
    }

    /* attacking your own quest leader will anger his or her guardians */
    if (mtmp->data == &mons[quest_info(MS_LEADER)]) {
        struct monst *mon;
        struct permonst *q_guardian = &mons[quest_info(MS_GUARDIAN)];
        int got_mad = 0;

        /* guardians will sense this attack even if they can't see it */
        for (mon = fmon; mon; mon = mon->nmon) {
            if (DEADMONSTER(mon))
                continue;
            if (mon->data == q_guardian && mon->mpeaceful) {
                mon->mpeaceful = 0;
                if (canseemon(mon))
                    ++got_mad;
            }
        }
        if (got_mad && !Hallucination) {
            const char *who = q_guardian->pmnames[NEUTRAL];

            if (got_mad > 1)
                who = makeplural(who);
            pline_The("%s %s to be angry too...",
                      who, vtense(who, "appear"));
        }
    }

    /* make other peaceful monsters react */
    if (!g.context.mon_moving) {
        struct monst *mon;
        int mndx = monsndx(mtmp->data);

        for (mon = fmon; mon; mon = mon->nmon) {
            if (DEADMONSTER(mon))
                continue;
            if (mon == mtmp) /* the mpeaceful test catches this since mtmp */
                continue;    /* is no longer peaceful, but be explicit...  */

            if (!mindless(mon->data) && mon->mpeaceful
                && couldsee(mon->mx, mon->my) && !mon->msleeping
                && mon->mcansee && m_canseeu(mon)) {
                char buf[BUFSZ];
                boolean exclaimed = FALSE, needpunct = FALSE, alreadyfleeing;

                buf[0] = '\0';
                if (humanoid(mon->data) || mon->isshk || mon->ispriest) {
                    if (is_watch(mon->data)) {
                        verbalize("Halt!  You're under arrest!");
                        (void) angry_guards(!!Deaf);
                    } else {
                        if (!Deaf && !rn2(5)) {
                            const char *gasp = maybe_gasp(mon);

                            if (gasp) {
                                if (!strncmpi(gasp, "gasp", 4)) {
                                    Sprintf(buf, "%s gasps", Monnam(mon));
                                    needpunct = TRUE;
                                } else {
                                    Sprintf(buf, "%s exclaims \"%s\"",
                                            Monnam(mon), gasp);
                                }
                                exclaimed = TRUE;
                            }
                        }
                        /* shopkeepers and temple priests might gasp in
                           surprise, but they won't become angry here */
                        if (mon->isshk || mon->ispriest) {
                            if (exclaimed)
                                pline("%s%s", buf, " then shrugs.");
                            continue;
                        }

                        if (mon->data->mlevel < rn2(10)) {
                            alreadyfleeing = (mon->mflee || mon->mfleetim);
                            monflee(mon, rn2(50) + 25, TRUE, !exclaimed);
                            if (exclaimed) {
                                if (flags.verbose && !alreadyfleeing) {
                                    Strcat(buf, " and then turns to flee.");
                                    needpunct = FALSE;
                                }
                            } else
                                exclaimed = TRUE; /* got msg from monflee() */
                        }
                        if (*buf)
                            pline("%s%s", buf, needpunct ? "." : "");
                        if (mon->mtame) {
                            ; /* mustn't set mpeaceful to 0 as below;
                               * perhaps reduce tameness? */
                        } else {
                            mon->mpeaceful = 0;
                            adjalign(-1);
                            if (!exclaimed)
                                pline("%s gets angry!", Monnam(mon));
                        }
                    }
                } else if (mon->data->mlet == mtmp->data->mlet
                           && big_little_match(mndx, monsndx(mon->data))
                           && !rn2(3)) {
                    if (!rn2(4)) {
                        growl(mon);
                        exclaimed = (iflags.last_msg == PLNMSG_GROWL);
                    }
                    if (rn2(6)) {
                        alreadyfleeing = (mon->mflee || mon->mfleetim);
                        monflee(mon, rn2(25) + 15, TRUE, !exclaimed);
                        if (exclaimed && !alreadyfleeing)
                            /* word like a separate sentence so that we
                               don't have to poke around inside growl() */
                            pline("And then starts to flee.");
                    }
                }
            }
        }
    }
}

/* Indicate via message that a monster has awoken. */
void
wake_msg(struct monst *mtmp, boolean interesting)
{
    if (mtmp->msleeping && canseemon(mtmp)) {
        pline("%s wakes up%s%s", Monnam(mtmp), interesting ? "!" : ".",
              mtmp->data == &mons[PM_FLESH_GOLEM] ? " It's alive!" : "");
    }
}

/* wake up a monster, possibly making it angry in the process */
void
wakeup(struct monst* mtmp, boolean via_attack)
{
    wake_msg(mtmp, via_attack);
    mtmp->msleeping = 0;
    if (M_AP_TYPE(mtmp) != M_AP_NOTHING) {
        /* mimics come out of hiding, but disguised Wizard doesn't
           have to lose his disguise */
        if (M_AP_TYPE(mtmp) != M_AP_MONSTER)
            seemimic(mtmp);
    } else if (g.context.forcefight && !g.context.mon_moving
               && mtmp->mundetected) {
        mtmp->mundetected = 0;
        newsym(mtmp->mx, mtmp->my);
    }
    finish_meating(mtmp);
    if (via_attack)
        setmangry(mtmp, TRUE);
}

/* Wake up nearby monsters without angering them. */
void
wake_nearby(void)
{
    wake_nearto(u.ux, u.uy, u.ulevel * 20);
}

/* Wake up monsters near some particular location. */
void
wake_nearto(int x, int y, int distance)
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (distance == 0 || dist2(mtmp->mx, mtmp->my, x, y) < distance) {
            /* sleep for N turns uses mtmp->mfrozen, but so does paralysis
               so we leave mfrozen monsters alone */
            wake_msg(mtmp, FALSE);
            mtmp->msleeping = 0; /* wake indeterminate sleep */
            if (!(mtmp->data->geno & G_UNIQ))
                mtmp->mstrategy &= ~STRAT_WAITMASK; /* wake 'meditation' */
            if (g.context.mon_moving)
                continue;
            if (mtmp->mtame) {
                if (!mtmp->isminion)
                    EDOG(mtmp)->whistletime = g.moves;
                /* Clear mtrack. This is to fix up a pet who is
                   stuck "fleeing" its master. */
                memset(mtmp->mtrack, 0, sizeof mtmp->mtrack);
            }
        }
    }
}

/* NOTE: we must check for mimicry before calling this routine */
void
seemimic(register struct monst* mtmp)
{
    boolean is_blocker_appear = (is_lightblocker_mappear(mtmp));

    if (has_mcorpsenm(mtmp))
        freemcorpsenm(mtmp);

    mtmp->m_ap_type = M_AP_NOTHING;
    mtmp->mappearance = 0;

    /*
     *  Discovered mimics don't block light.
     */
    if (is_blocker_appear
        && !does_block(mtmp->mx, mtmp->my, &levl[mtmp->mx][mtmp->my]))
        unblock_point(mtmp->mx, mtmp->my);

    newsym(mtmp->mx, mtmp->my);
}

/* [taken out of rescham() in order to be shared by restore_cham()] */
void
normal_shape(struct monst *mon)
{
    int mcham = (int) mon->cham;

    if (mcham >= LOW_PM) {
        unsigned mcan = mon->mcan;

        (void) newcham(mon, &mons[mcham], FALSE, FALSE);
        mon->cham = NON_PM;
        /* newcham() may uncancel a polymorphing monster; override that */
        if (mcan)
            mon->mcan = 1;
        newsym(mon->mx, mon->my);
    }
    if (is_were(mon->data) && mon->data->mlet != S_HUMAN) {
        new_were(mon);
    }
    if (M_AP_TYPE(mon) != M_AP_NOTHING) {
        /* this used to include a cansee() check but Protection_from_
           _shape_changers shouldn't be trumped by being unseen */
        if (!mon->meating) {
            /* make revealed mimic fall asleep in lieu of shape change */
            if (M_AP_TYPE(mon) != M_AP_MONSTER)
                mon->msleeping = 1;
            seemimic(mon);
        } else {
            /* quickmimic: pet is midst of eating a mimic corpse;
               this terminates the meal early */
            finish_meating(mon);
        }
    }
}

/* iterate all living monsters on current level, calling func for each. */
void
iter_mons(void (*func)(struct monst *))
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        func(mtmp);
    }
}


/* iterate all living monsters on current level, calling func for each.
   if func returns TRUE, stop and return that monster. */
struct monst *
get_iter_mons(boolean (*func)(struct monst *))
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (func(mtmp))
            return mtmp;
    }
    return (struct monst *) 0;
}

/* iterate all living monsters on current level, calling func for each,
   passing x,y to the function.
   if func returns TRUE, stop and return that monster. */
struct monst *
get_iter_mons_xy(boolean (*func)(struct monst *, xchar, xchar),
                xchar x, xchar y)
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (func(mtmp, x, y))
            return mtmp;
    }
    return (struct monst *) 0;
}


/* force all chameleons and mimics to become themselves and werecreatures
   to revert to human form; called when Protection_from_shape_changers gets
   activated via wearing or eating ring */
void
rescham(void)
{
    iter_mons(normal_shape);
}

static void
m_restartcham(struct monst *mtmp)
{
    if (!mtmp->mcan)
        mtmp->cham = pm_to_cham(monsndx(mtmp->data));
    if (mtmp->data->mlet == S_MIMIC && mtmp->msleeping) {
        set_mimic_sym(mtmp);
        newsym(mtmp->mx, mtmp->my);
    }
}

/* let chameleons change and mimics hide again; called when taking off
   ring of protection from shape changers */
void
restartcham(void)
{
    iter_mons(m_restartcham);
}

/* called when restoring a monster from a saved level; protection
   against shape-changing might be different now than it was at the
   time the level was saved. */
void
restore_cham(struct monst *mon)
{
    if (Protection_from_shape_changers || mon->mcan) {
        /* force chameleon or mimic to revert to its natural shape */
        normal_shape(mon);
    } else if (mon->cham == NON_PM) {
        /* chameleon doesn't change shape here, just gets allowed to do so */
        mon->cham = pm_to_cham(monsndx(mon->data));
    }
}

/* unwatched hiders may hide again; if so, returns True */
static boolean
restrap(struct monst *mtmp)
{
    struct trap *t;

    if (mtmp->mcan || M_AP_TYPE(mtmp) || cansee(mtmp->mx, mtmp->my)
        || rn2(3) || mtmp == u.ustuck
        /* can't hide while trapped except in pits */
        || (mtmp->mtrapped && (t = t_at(mtmp->mx, mtmp->my)) != 0
            && !is_pit(t->ttyp))
        || (sensemon(mtmp) && next2u(mtmp->mx, mtmp->my)))
        return FALSE;

    if (mtmp->data->mlet == S_MIMIC) {
        set_mimic_sym(mtmp);
        return TRUE;
    } else if (levl[mtmp->mx][mtmp->my].typ == ROOM) {
        mtmp->mundetected = 1;
        return TRUE;
    }

    return FALSE;
}

/* reveal a monster at x,y hiding under an object,
   if there are no objects there */
void
maybe_unhide_at(xchar x, xchar y)
{
    struct monst *mtmp;

    if (OBJ_AT(x, y))
        return;
    if ((mtmp = m_at(x, y)) == 0 && u_at(x, y))
        mtmp = &g.youmonst;
    if (mtmp && mtmp->mundetected && hides_under(mtmp->data))
        (void) hideunder(mtmp);
}

/* monster/hero tries to hide under something at the current location */
boolean
hideunder(struct monst *mtmp)
{
    struct trap *t;
    boolean oldundetctd, undetected = FALSE, is_u = (mtmp == &g.youmonst);
    xchar x = is_u ? u.ux : mtmp->mx, y = is_u ? u.uy : mtmp->my;

    if (mtmp == u.ustuck) {
        ; /* can't hide if holding you or held by you */
    } else if (is_u ? (u.utrap && u.utraptype != TT_PIT)
                    : (mtmp->mtrapped && (t = t_at(x, y)) != 0
                       && !is_pit(t->ttyp))) {
        ; /* can't hide while stuck in a non-pit trap */
    } else if (mtmp->data->mlet == S_EEL) {
        undetected = (is_pool(x, y) && !Is_waterlevel(&u.uz));
    } else if (hides_under(mtmp->data) && OBJ_AT(x, y)) {
        struct obj *otmp = g.level.objects[x][y];

        /* most monsters won't hide under cockatrice corpse but they
           can hide under a pile containing more than just such corpses */
        while (otmp && otmp->otyp == CORPSE
               && touch_petrifies(&mons[otmp->corpsenm]))
            otmp = otmp->nexthere;
        if (otmp != 0 || ((mtmp == &g.youmonst) ? Stone_resistance
                                                : resists_ston(mtmp)))
            undetected = TRUE;
    }

    if (is_u) {
        oldundetctd = u.uundetected != 0;
        u.uundetected = undetected ? 1 : 0;
    } else {
        oldundetctd = mtmp->mundetected != 0;
        mtmp->mundetected = undetected ? 1 : 0;
    }
    if (undetected != oldundetctd)
        newsym(x, y);
    return undetected;
}

/* called when returning to a previously visited level */
void
hide_monst(struct monst* mon)
{
    boolean hider_under = hides_under(mon->data) || mon->data->mlet == S_EEL;

    if ((is_hider(mon->data) || hider_under)
        && !(mon->mundetected || M_AP_TYPE(mon))) {
        xchar x = mon->mx, y = mon->my;
        char save_viz = g.viz_array[y][x];

        /* override vision, forcing hero to be unable to see monster's spot */
        g.viz_array[y][x] &= ~(IN_SIGHT | COULD_SEE);
        if (is_hider(mon->data))
            (void) restrap(mon);
        /* try again if mimic missed its 1/3 chance to hide */
        if (mon->data->mlet == S_MIMIC && !M_AP_TYPE(mon))
            (void) restrap(mon);
        g.viz_array[y][x] = save_viz;
        if (hider_under)
            (void) hideunder(mon);
    }
}

void
mon_animal_list(boolean construct)
{
    if (construct) {
        short animal_temp[SPECIAL_PM];
        int i, n;

        /* if (animal_list) impossible("animal_list already exists"); */

        for (n = 0, i = LOW_PM; i < SPECIAL_PM; i++)
            if (is_animal(&mons[i]))
                animal_temp[n++] = i;
        /* if (n == 0) animal_temp[n++] = NON_PM; */

        g.animal_list = (short *) alloc(n * sizeof *g.animal_list);
        (void) memcpy((genericptr_t) g.animal_list, (genericptr_t) animal_temp,
                      n * sizeof *g.animal_list);
        g.animal_list_count = n;
    } else { /* release */
        if (g.animal_list)
            free((genericptr_t) g.animal_list), g.animal_list = 0;
        g.animal_list_count = 0;
    }
}

static int
pick_animal(void)
{
    int res;

    if (!g.animal_list)
        mon_animal_list(TRUE);

    res = g.animal_list[rn2(g.animal_list_count)];
    /* rogue level should use monsters represented by uppercase letters
       only, but since chameleons aren't generated there (not uppercase!)
       we don't perform a lot of retries */
    if (Is_rogue_level(&u.uz) && !isupper((uchar) mons[res].mlet))
        res = g.animal_list[rn2(g.animal_list_count)];
    return res;
}

void
decide_to_shapeshift(struct monst* mon, int shiftflags)
{
    struct permonst *ptr = 0;
    int mndx;
    unsigned was_female = mon->female;
    boolean msg = FALSE, dochng = FALSE;

    if ((shiftflags & SHIFT_MSG)
        || ((shiftflags & SHIFT_SEENMSG) && sensemon(mon)))
        msg = TRUE;

    if (!is_vampshifter(mon)) {
        /* regular shapeshifter */
        if (!rn2(6))
            dochng = TRUE;
    } else {
        /* The vampire has to be in good health (mhp) to maintain
         * its shifted form.
         *
         * If we're shifted and getting low on hp, maybe shift back, or
         * if we're a fog cloud at full hp, maybe pick a different shape.
         * If we're not already shifted and in good health, maybe shift.
         */
        if (mon->data->mlet != S_VAMPIRE) {
            if ((mon->mhp <= (mon->mhpmax + 5) / 6) && rn2(4)
                && mon->cham >= LOW_PM) {
                ptr = &mons[mon->cham];
                dochng = TRUE;
            } else if (mon->data == &mons[PM_FOG_CLOUD]
                     && mon->mhp == mon->mhpmax && !rn2(4)
                     && (!canseemon(mon)
                         || distu(mon->mx, mon->my) > BOLT_LIM * BOLT_LIM)) {
                /* if a fog cloud, maybe change to wolf or vampire bat;
                   those are more likely to take damage--at least when
                   tame--and then switch back to vampire; they'll also
                   switch to fog cloud if they encounter a closed door */
                mndx = pickvampshape(mon);
                if (mndx >= LOW_PM) {
                    ptr = &mons[mndx];
                    dochng = (ptr != mon->data);
                }
            }
        } else {
            if (mon->mhp >= 9 * mon->mhpmax / 10 && !rn2(6)
                && (!canseemon(mon)
                    || distu(mon->mx, mon->my) > BOLT_LIM * BOLT_LIM))
                dochng = TRUE; /* 'ptr' stays Null */
        }
    }
    if (dochng) {
        if (newcham(mon, ptr, FALSE, msg) && is_vampshifter(mon)) {
            /* for vampshift, override the 10% chance for sex change */
            ptr = mon->data;
            if (!is_male(ptr) && !is_female(ptr) && !is_neuter(ptr))
                mon->female = was_female;
        }
    }
}

static int
pickvampshape(struct monst* mon)
{
    int mndx = mon->cham, wolfchance = 10;
    /* avoid picking monsters with lowercase display symbols ('d' for wolf
       and 'v' for fog cloud) on rogue level*/
    boolean uppercase_only = Is_rogue_level(&u.uz);

    switch (mndx) {
    case PM_VLAD_THE_IMPALER:
        /* ensure Vlad can keep carrying the Candelabrum */
        if (mon_has_special(mon))
            break; /* leave mndx as is */
        wolfchance = 3;
    /*FALLTHRU*/
    case PM_VAMPIRE_LEADER: /* vampire lord or Vlad can become wolf */
        if (!rn2(wolfchance) && !uppercase_only) {
            mndx = PM_WOLF;
            break;
        }
    /*FALLTHRU*/
    case PM_VAMPIRE: /* any vampire can become fog or bat */
        mndx = (!rn2(4) && !uppercase_only) ? PM_FOG_CLOUD : PM_VAMPIRE_BAT;
        break;
    }

    if (g.mvitals[mndx].mvflags & G_GENOD)
        return mon->cham;

    return mndx;
}

/* nonshapechangers who warrant special polymorph handling */
static boolean
isspecmon(struct monst* mon)
{
    return (mon->isshk || mon->ispriest || mon->isgd
            || mon->m_id == g.quest_status.leader_m_id);
}

/* restrict certain special monsters (shopkeepers, aligned priests,
   vault guards) to forms that allow them to behave sensibly (catching
   gold, speaking?) so that they don't need too much extra code */
static boolean
validspecmon(struct monst* mon, int mndx)
{
    if (mndx == NON_PM)
        return TRUE; /* caller wants random */

    if (!accept_newcham_form(mon, mndx))
        return FALSE; /* geno'd or !polyok */

    if (isspecmon(mon)) {
        struct permonst *ptr = &mons[mndx];

        /* reject notake because object manipulation is expected
           and nohead because speech capability is expected */
        if (notake(ptr) || !has_head(ptr))
            return FALSE;
        /* [should we check ptr->msound here too?] */
    }
    return TRUE; /* potential new form is ok */
}

/* used for hero polyself handling */
boolean
valid_vampshiftform(int base, int form)
{
    if (base >= LOW_PM && is_vampire(&mons[base])) {
        if (form == PM_VAMPIRE_BAT || form == PM_FOG_CLOUD
            || (form == PM_WOLF && base != PM_VAMPIRE))
            return TRUE;
    }
    return FALSE;
}

/* prevent wizard mode user from specifying invalid vampshifter shape
   when using monpolycontrol to assign a new form to a vampshifter */
boolean
validvamp(struct monst* mon, int* mndx_p, int monclass)
{
    /* simplify caller's usage */
    if (!is_vampshifter(mon))
        return validspecmon(mon, *mndx_p);

    if (mon->cham == PM_VLAD_THE_IMPALER && mon_has_special(mon)) {
        /* Vlad with Candelabrum; override choice, then accept it */
        *mndx_p = PM_VLAD_THE_IMPALER;
        return TRUE;
    }
    if (*mndx_p >= LOW_PM && is_shapeshifter(&mons[*mndx_p])) {
        /* player picked some type of shapeshifter; use mon's self
           (vampire or chameleon) */
        *mndx_p = mon->cham;
        return TRUE;
    }
    /* basic vampires can't become wolves; any can become fog or bat
       (we don't enforce upper-case only for rogue level here) */
    if (*mndx_p == PM_WOLF)
        return (boolean) (mon->cham != PM_VAMPIRE);
    if (*mndx_p == PM_FOG_CLOUD || *mndx_p == PM_VAMPIRE_BAT)
        return TRUE;

    /* if we get here, specific type was no good; try by class */
    switch (monclass) {
    case S_VAMPIRE:
        *mndx_p = mon->cham;
        break;
    case S_BAT:
        *mndx_p = PM_VAMPIRE_BAT;
        break;
    case S_VORTEX:
        *mndx_p = PM_FOG_CLOUD;
        break;
    case S_DOG:
        if (mon->cham != PM_VAMPIRE) {
            *mndx_p = PM_WOLF;
            break;
        }
        /*FALLTHRU*/
    default:
        *mndx_p = NON_PM;
        break;
    }
    return (boolean) (*mndx_p != NON_PM);
}

int
select_newcham_form(struct monst* mon)
{
    int mndx = NON_PM, tryct;

    switch (mon->cham) {
    case PM_SANDESTIN:
        if (rn2(7))
            mndx = pick_nasty(mons[PM_ARCHON].difficulty - 1);
        break;
    case PM_DOPPELGANGER:
        if (!rn2(7)) {
            mndx = pick_nasty(mons[PM_JABBERWOCK].difficulty - 1);
        } else if (rn2(3)) { /* role monsters */
            mndx = rn1(PM_WIZARD - PM_ARCHEOLOGIST + 1, PM_ARCHEOLOGIST);
        } else if (!rn2(3)) { /* quest guardians */
            mndx = rn1(PM_APPRENTICE - PM_STUDENT + 1, PM_STUDENT);
            /* avoid own role's guardian */
            if (mndx == g.urole.guardnum)
                mndx = NON_PM;
        } else { /* general humanoids */
            tryct = 5;
            do {
                mndx = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
                if (humanoid(&mons[mndx]) && polyok(&mons[mndx]))
                    break;
            } while (--tryct > 0);
            if (!tryct)
                mndx = NON_PM;
        }
        break;
    case PM_CHAMELEON:
        if (!rn2(3))
            mndx = pick_animal();
        break;
    case PM_VLAD_THE_IMPALER:
    case PM_VAMPIRE_LEADER:
    case PM_VAMPIRE:
        mndx = pickvampshape(mon);
        break;
    case NON_PM: /* ordinary */
      {
        struct obj *m_armr = which_armor(mon, W_ARM);

        if (m_armr && Is_dragon_scales(m_armr))
            mndx = (int) (Dragon_scales_to_pm(m_armr) - mons);
        else if (m_armr && Is_dragon_mail(m_armr))
            mndx = (int) (Dragon_mail_to_pm(m_armr) - mons);
      }
        break;
    }

    /* for debugging: allow control of polymorphed monster */
    if (wizard && iflags.mon_polycontrol) {
        char pprompt[BUFSZ], parttwo[QBUFSZ], buf[BUFSZ];
        int monclass, len;

        /* construct prompt in pieces */
        Sprintf(pprompt, "Change %s", noit_mon_nam(mon));
        Sprintf(parttwo, " @ %s into what?",
                coord_desc((int) mon->mx, (int) mon->my, buf,
                           (iflags.getpos_coords != GPCOORDS_NONE)
                              ? iflags.getpos_coords : GPCOORDS_MAP));
        /* combine the two parts, not exceeding QBUFSZ-1 in overall length;
           if combined length is too long it has to be due to monster's
           name so we'll chop enough of that off to fit the second part */
        if ((len = (int) strlen(pprompt) + (int) strlen(parttwo)) >= QBUFSZ)
            /* strlen(parttwo) is less than QBUFSZ/2 so strlen(pprompt) is
               more than QBUFSZ/2 and excess amount being truncated can't
               exceed pprompt's length and back up to before &pprompt[0]) */
            *(eos(pprompt) - (len - (QBUFSZ - 1))) = '\0';
        Strcat(pprompt, parttwo);

        buf[0] = '\0'; /* clear buffer for EDIT_GETLIN */
#define TRYLIMIT 5
        tryct = TRYLIMIT;
        do {
            if (tryct == TRYLIMIT - 1) { /* first retry */
                /* change "into what?" to "into what kind of monster?" */
                if (strlen(pprompt) + sizeof " kind of monster" - 1 < QBUFSZ)
                    Strcpy(eos(pprompt) - 1, " kind of monster?");
            }
#undef TRYLIMIT
            monclass = 0;
            getlin(pprompt, buf);
            mungspaces(buf);
            /* for ESC, take form selected above (might be NON_PM) */
            if (*buf == '\033')
                break;
            /* for "*", use NON_PM to pick an arbitrary shape below */
            if (!strcmp(buf, "*") || !strcmpi(buf, "random")) {
                mndx = NON_PM;
                break;
            }
            mndx = name_to_mon(buf, (int *) 0);
            if (mndx == NON_PM) {
                /* didn't get a type, so check whether it's a class
                   (single letter or text match with def_monsyms[]) */
                monclass = name_to_monclass(buf, &mndx);
                if (monclass && mndx == NON_PM)
                    mndx = mkclass_poly(monclass);
            }
            if (mndx >= LOW_PM) {
                /* got a specific type of monster; use it if we can */
                if (validvamp(mon, &mndx, monclass))
                    break;
                /* can't; revert to random in case we exhaust tryct */
                mndx = NON_PM;
            }

            pline("It can't become that.");
        } while (--tryct > 0);

        if (!tryct)
            pline1(thats_enough_tries);
        if (is_vampshifter(mon) && !validvamp(mon, &mndx, monclass))
            mndx = pickvampshape(mon); /* don't resort to arbitrary */
    }

    /* if no form was specified above, pick one at random now */
    if (mndx == NON_PM) {
        tryct = 50;
        do {
            mndx = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
        } while (--tryct > 0 && !validspecmon(mon, mndx)
                 /* try harder to select uppercase monster on rogue level */
                 && (tryct > 40 && Is_rogue_level(&u.uz)
                     && !isupper((uchar) mons[mndx].mlet)));
    }
    return mndx;
}

/* this used to be inline within newcham() but monpolycontrol needs it too */
static struct permonst *
accept_newcham_form(struct monst* mon, int mndx)
{
    struct permonst *mdat;

    if (mndx == NON_PM)
        return 0;
    mdat = &mons[mndx];
    if ((g.mvitals[mndx].mvflags & G_GENOD) != 0)
        return 0;
    if (is_placeholder(mdat))
        return 0;
    /* select_newcham_form() might deliberately pick a player
       character type (random selection never does) which
       polyok() rejects, so we need a special case here */
    if (is_mplayer(mdat))
        return mdat;
    /* shapeshifters are rejected by polyok() but allow a shapeshifter
       to take on its 'natural' form */
    if (is_shapeshifter(mdat)
        && mon->cham >= LOW_PM && mdat == &mons[mon->cham])
        return mdat;
    /* polyok() rules out M2_PNAME, M2_WERE, and all humans except Kops */
    return polyok(mdat) ? mdat : 0;
}

void
mgender_from_permonst(struct monst* mtmp, struct permonst* mdat)
{
    if (is_male(mdat)) {
        if (mtmp->female)
            mtmp->female = FALSE;
    } else if (is_female(mdat)) {
        if (!mtmp->female)
            mtmp->female = TRUE;
    } else if (!is_neuter(mdat)) {
        if (!rn2(10))
            mtmp->female = !mtmp->female;
    }
}

/* make a chameleon take on another shape, or a polymorph target
   (possibly self-inflicted) become a different monster;
   returns 1 if it actually changes form */
int
newcham(
    struct monst *mtmp,
    struct permonst *mdat,
    boolean polyspot, /* change is the result of wand or spell of polymorph */
    boolean msg)      /* "The oldmon turns into a newmon!" */
{
    int hpn, hpd;
    int mndx, tryct;
    struct permonst *olddata = mtmp->data;
    char *p, oldname[BUFSZ], l_oldname[BUFSZ], newname[BUFSZ];

    /* Riders are immune to polymorph and green slime
       (but apparent Rider might actually be a doppelganger) */
    if (mtmp->cham == NON_PM) { /* not a shapechanger */
        if (is_rider(olddata))
            return 0;
        /* make Nazgul and erinyes immune too, to reduce chance of
           anomalous extinction feedback during final disclsoure */
        if (mbirth_limit(monsndx(olddata)) < MAXMONNO)
            return 0;
        /* cancelled shapechangers become uncancelled prior
           to being given a new shape */
        if (mtmp->mcan && !Protection_from_shape_changers) {
            mtmp->cham = pm_to_cham(monsndx(mtmp->data));
            if (mtmp->cham != NON_PM)
                mtmp->mcan = 0;
        }
    }

    if (msg) {
        /* like Monnam() but never mention saddle */
        Strcpy(oldname, x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                                 SUPPRESS_SADDLE, FALSE));
        oldname[0] = highc(oldname[0]);
    }
    /* we need this one whether msg is true or not */
    Strcpy(l_oldname, x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                               has_mgivenname(mtmp) ? SUPPRESS_SADDLE : 0,
                               FALSE));

    /* mdat = 0 -> caller wants a random monster shape */
    if (mdat == 0) {
        /* select_newcham_form() loops when resorting to random but
           it doesn't always pick that so we still retry here too */
        tryct = 20;
        do {
            mndx = select_newcham_form(mtmp);
            mdat = accept_newcham_form(mtmp, mndx);
            /* for the first several tries we require upper-case on
               the rogue level (after that, we take whatever we get) */
            if (tryct > 15 && Is_rogue_level(&u.uz)
                && mdat && !isupper((uchar) mdat->mlet))
                mdat = 0;
            if (mdat)
                break;
        } while (--tryct > 0);
        if (!tryct)
            return 0;
    } else if (g.mvitals[monsndx(mdat)].mvflags & G_GENOD)
        return 0; /* passed in mdat is genocided */

    if (mdat == olddata)
        return 0; /* still the same monster */

    mgender_from_permonst(mtmp, mdat);
    /* Endgame mplayers start out as "Foo the Bar", but some of the
     * titles are inappropriate when polymorphed, particularly into
     * the opposite sex.  Player characters don't use ranks when
     * polymorphed, so dropping rank for mplayers seems reasonable.
     */
    if (In_endgame(&u.uz) && is_mplayer(olddata)
        && has_mgivenname(mtmp)
        && (p = strstr(MGIVENNAME(mtmp), " the ")) != 0)
        *p = '\0';

    if (mtmp->wormno) { /* throw tail away */
        wormgone(mtmp);
        place_monster(mtmp, mtmp->mx, mtmp->my);
    }
    if (M_AP_TYPE(mtmp) && mdat->mlet != S_MIMIC)
        seemimic(mtmp); /* revert to normal monster */

    /* (this code used to try to adjust the monster's health based on
       a normal one of its type but there are too many special cases
       which need to handled in order to do that correctly, so just
       give the new form the same proportion of HP as its old one had) */
    hpn = mtmp->mhp;
    hpd = mtmp->mhpmax;
    /* set level and hit points */
    newmonhp(mtmp, monsndx(mdat));
    /* new hp: same fraction of max as before */
#ifndef LINT
    mtmp->mhp = (int) (((long) hpn * (long) mtmp->mhp) / (long) hpd);
#endif
    /* sanity check (potential overflow) */
    if (mtmp->mhp < 0 || mtmp->mhp > mtmp->mhpmax)
        mtmp->mhp = mtmp->mhpmax;
    /* unlikely but not impossible; a 1HD creature with 1HP that changes
       into a 0HD creature will require this statement */
    if (!mtmp->mhp)
        mtmp->mhp = 1;

    /* take on the new form... */
    set_mon_data(mtmp, mdat);

    if (mtmp->mleashed) {
        if (!leashable(mtmp))
            m_unleash(mtmp, TRUE);
        else
            /* if leashed, persistent inventory window needs updating
               (really only when mon_nam() is going to yield "a frog"
               rather than "Kermit" but no need to micromanage here) */
            update_inventory(); /* x - leash (attached to a <mon>) */
    }

    if (emits_light(olddata) != emits_light(mtmp->data)) {
        /* used to give light, now doesn't, or vice versa,
           or light's range has changed */
        if (emits_light(olddata))
            del_light_source(LS_MONSTER, monst_to_any(mtmp));
        if (emits_light(mtmp->data))
            new_light_source(mtmp->mx, mtmp->my, emits_light(mtmp->data),
                             LS_MONSTER, monst_to_any(mtmp));
    }
    if (!mtmp->perminvis || pm_invisible(olddata))
        mtmp->perminvis = pm_invisible(mdat);
    mtmp->minvis = mtmp->invis_blkd ? 0 : mtmp->perminvis;
    if (mtmp->mundetected)
        (void) hideunder(mtmp);
    if (u.ustuck == mtmp) {
        if (u.uswallow) {
            if (!attacktype(mdat, AT_ENGL)) {
                /* Does mdat care? */
                if (!noncorporeal(mdat) && !amorphous(mdat)
                    && !is_whirly(mdat) && (mdat != &mons[PM_YELLOW_LIGHT])) {
                    char msgtrail[BUFSZ];

                    if (is_vampshifter(mtmp)) {
                        Sprintf(msgtrail, " which was a shapeshifted %s",
                                noname_monnam(mtmp, ARTICLE_NONE));
                    } else if (is_animal(mdat)) {
                        Strcpy(msgtrail, "'s stomach");
                    } else {
                        msgtrail[0] = '\0';
                    }

                    /* Do this even if msg is FALSE */
                    You("%s %s%s!",
                        (amorphous(olddata) || is_whirly(olddata))
                            ? "emerge from" : "break out of",
                        l_oldname, msgtrail);
                    msg = FALSE; /* message has been given */
                    mtmp->mhp = 1; /* almost dead */
                }
                expels(mtmp, olddata, FALSE);
            } else {
                /* update swallow glyphs for new monster */
                swallowed(0);
            }
        } else if (!sticks(mdat) && !sticks(g.youmonst.data))
            unstuck(mtmp);
    }

    if (mdat == &mons[PM_LONG_WORM] && (mtmp->wormno = get_wormno()) != 0) {
        initworm(mtmp, rn2(5));
        place_worm_tail_randomly(mtmp, mtmp->mx, mtmp->my);
    }

    newsym(mtmp->mx, mtmp->my);

    if (msg) {
        Strcpy(newname, noname_monnam(mtmp, ARTICLE_A));
        /* oldname was capitalized above; newname will be lower case */
        if (!strcmpi(newname, "it")) { /* can't see or sense it now */
            if (!!strcmpi(oldname, "it")) /* could see or sense it before */
                pline("%s disappears!", oldname);
            (void) usmellmon(mdat);
        } else { /* can see or sense it now */
            if (!strcmpi(oldname, "it")) /* couldn't see or sense it before */
                pline("%s appears!", upstart(newname));
            else
                pline("%s turns into %s!", oldname, newname);
        }
    }

    /* when polymorph trap/wand/potion produces a vampire, turn in into
       a full-fledged vampshifter unless shape-changing is blocked */
    if (mtmp->cham == NON_PM && mdat->mlet == S_VAMPIRE
        && !Protection_from_shape_changers)
        mtmp->cham = pm_to_cham(monsndx(mdat));

    possibly_unwield(mtmp, polyspot); /* might lose use of weapon */
    mon_break_armor(mtmp, polyspot);
    if (!(mtmp->misc_worn_check & W_ARMG))
        mselftouch(mtmp, "No longer petrify-resistant, ",
                   !g.context.mon_moving);
    m_dowear(mtmp, FALSE);

    /* This ought to re-test can_carry() on each item in the inventory
     * rather than just checking ex-giants & boulders, but that'd be
     * pretty expensive to perform.  If implemented, then perhaps
     * minvent should be sorted in order to drop heaviest items first.
     */
    /* former giants can't continue carrying boulders */
    if (mtmp->minvent && !throws_rocks(mdat)) {
        register struct obj *otmp, *otmp2;

        for (otmp = mtmp->minvent; otmp; otmp = otmp2) {
            otmp2 = otmp->nobj;
            if (otmp->otyp == BOULDER) {
                /* this keeps otmp from being polymorphed in the
                   same zap that the monster that held it is polymorphed */
                if (polyspot)
                    bypass_obj(otmp);
                obj_extract_self(otmp);
                /* probably ought to give some "drop" message here */
                if (flooreffects(otmp, mtmp->mx, mtmp->my, ""))
                    continue;
                place_object(otmp, mtmp->mx, mtmp->my);
            }
        }
    }

    return 1;
}

/* sometimes an egg will be special */
#define BREEDER_EGG (!rn2(77))

/*
 * Determine if the given monster number can be hatched from an egg.
 * Return the monster number to use as the egg's corpsenm.  Return
 * NON_PM if the given monster can't be hatched.
 */
int
can_be_hatched(int mnum)
{
    /* ranger quest nemesis has the oviparous bit set, making it
       be possible to wish for eggs of that unique monster; turn
       such into ordinary eggs rather than forbidding them outright */
    if (mnum == PM_SCORPIUS)
        mnum = PM_SCORPION;

    mnum = little_to_big(mnum);
    /*
     * Queen bees lay killer bee eggs (usually), but killer bees don't
     * grow into queen bees.  Ditto for [winged-]gargoyles.
     */
    if (mnum == PM_KILLER_BEE || mnum == PM_GARGOYLE
        || (lays_eggs(&mons[mnum])
            && (BREEDER_EGG
                || (mnum != PM_QUEEN_BEE && mnum != PM_WINGED_GARGOYLE))))
        return mnum;
    return NON_PM;
}

/* type of egg laid by #sit; usually matches parent */
int
egg_type_from_parent(
    int mnum, /* parent monster; caller must handle lays_eggs() check */
    boolean force_ordinary)
{
    if (force_ordinary || !BREEDER_EGG) {
        if (mnum == PM_QUEEN_BEE)
            mnum = PM_KILLER_BEE;
        else if (mnum == PM_WINGED_GARGOYLE)
            mnum = PM_GARGOYLE;
    }
    return mnum;
}

/* decide whether an egg of the indicated monster type is viable;
   also used to determine whether an egg or tin can be created... */
boolean
dead_species(int m_idx, boolean egg)
{
    int alt_idx;

    /* generic eggs are unhatchable and have corpsenm of NON_PM */
    if (m_idx < LOW_PM)
        return TRUE;
    /*
     * For monsters with both baby and adult forms, genociding either
     * form kills all eggs of that monster.  Monsters with more than
     * two forms (small->large->giant mimics) are more or less ignored;
     * fortunately, none of them have eggs.  Species extinction due to
     * overpopulation does not kill eggs.
     */
    alt_idx = egg ? big_to_little(m_idx) : m_idx;
    return (boolean) ((g.mvitals[m_idx].mvflags & G_GENOD) != 0
                      || (g.mvitals[alt_idx].mvflags & G_GENOD) != 0);
}

/* kill off any eggs of genocided monsters */
static void
kill_eggs(struct obj* obj_list)
{
    struct obj *otmp;

    for (otmp = obj_list; otmp; otmp = otmp->nobj)
        if (otmp->otyp == EGG) {
            if (dead_species(otmp->corpsenm, TRUE)) {
                /*
                 * It seems we could also just catch this when
                 * it attempted to hatch, so we wouldn't have to
                 * search all of the objlists.. or stop all
                 * hatch timers based on a corpsenm.
                 */
                kill_egg(otmp);
            }
#if 0 /* not used */
        } else if (otmp->otyp == TIN) {
            if (dead_species(otmp->corpsenm, FALSE))
                otmp->corpsenm = NON_PM; /* empty tin */
        } else if (otmp->otyp == CORPSE) {
            if (dead_species(otmp->corpsenm, FALSE))
                ; /* not yet implemented... */
#endif
        } else if (Has_contents(otmp)) {
            kill_eggs(otmp->cobj);
        }
}

/* kill all members of genocided species */
void
kill_genocided_monsters(void)
{
    struct monst *mtmp, *mtmp2;
    boolean kill_cham;
    int mndx;

    /*
     * Called during genocide, and again upon level change.  The latter
     * catches up with any migrating monsters as they finally arrive at
     * their intended destinations, so possessions get deposited there.
     *
     * Chameleon handling:
     *  1) if chameleons have been genocided, destroy them
     *     regardless of current form;
     *  2) otherwise, force every chameleon which is imitating
     *     any genocided species to take on a new form.
     */
    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        mndx = monsndx(mtmp->data);
        kill_cham = (mtmp->cham >= LOW_PM
                     && (g.mvitals[mtmp->cham].mvflags & G_GENOD));
        if ((g.mvitals[mndx].mvflags & G_GENOD) || kill_cham) {
            if (mtmp->cham >= LOW_PM && !kill_cham)
                (void) newcham(mtmp, (struct permonst *) 0, FALSE, TRUE);
            else
                mondead(mtmp);
        }
        if (mtmp->minvent)
            kill_eggs(mtmp->minvent);
    }

    kill_eggs(g.invent);
    kill_eggs(fobj);
    kill_eggs(g.migrating_objs);
    kill_eggs(g.level.buriedobjlist);
}

void
golemeffects(register struct monst* mon, int damtype, int dam)
{
    int heal = 0, slow = 0;

    if (mon->data == &mons[PM_FLESH_GOLEM]) {
        if (damtype == AD_ELEC)
            heal = (dam + 5) / 6;
        else if (damtype == AD_FIRE || damtype == AD_COLD)
            slow = 1;
    } else if (mon->data == &mons[PM_IRON_GOLEM]) {
        if (damtype == AD_ELEC)
            slow = 1;
        else if (damtype == AD_FIRE)
            heal = dam;
    } else {
        return;
    }
    if (slow) {
        if (mon->mspeed != MSLOW)
            mon_adjust_speed(mon, -1, (struct obj *) 0);
    }
    if (heal) {
        if (mon->mhp < mon->mhpmax) {
            mon->mhp += heal;
            if (mon->mhp > mon->mhpmax)
                mon->mhp = mon->mhpmax;
            if (cansee(mon->mx, mon->my))
                pline("%s seems healthier.", Monnam(mon));
        }
    }
}

/* anger the Minetown watch */
boolean
angry_guards(boolean silent)
{
    struct monst *mtmp;
    int ct = 0, nct = 0, sct = 0, slct = 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (is_watch(mtmp->data) && mtmp->mpeaceful) {
            ct++;
            if (canspotmon(mtmp) && mtmp->mcanmove) {
                if (next2u(mtmp->mx, mtmp->my))
                    nct++;
                else
                    sct++;
            }
            if (mtmp->msleeping || mtmp->mfrozen) {
                slct++;
                mtmp->msleeping = mtmp->mfrozen = 0;
            }
            mtmp->mpeaceful = 0;
        }
    }
    if (ct) {
        if (!silent) { /* do we want pline msgs? */
            char buf[BUFSZ];

            if (slct) { /* sleeping guard(s) */
                Sprintf(buf, "guard%s", plur(slct));
                pline_The("%s %s up.", buf, vtense(buf, "wake"));
            }

            if (nct) { /* seen/sensed adjacent guard(s) */
                Sprintf(buf, "guard%s", plur(nct));
                pline_The("%s %s angry!", buf, vtense(buf, "get"));
            } else if (sct) { /* seen/sensed non-adjcent guard(s) */
                Sprintf(buf, "guard%s", plur(sct));
                pline("%s %s %s approaching!",
                      (sct == 1) ? "An angry" : "Angry",
                      buf, vtense(buf, "are"));
            } else {
                Strcpy(buf, (ct == 1) ? "a guard's" : "guards'");
                You_hear("the shrill sound of %s whistle%s.", buf, plur(ct));
            }
        }
        return TRUE;
    }
    return FALSE;
}

static void
pacify_guard(struct monst *mtmp)
{
    if (is_watch(mtmp->data))
        mtmp->mpeaceful = 1;
}

void
pacify_guards(void)
{
    iter_mons(pacify_guard);
}

void
mimic_hit_msg(struct monst* mtmp, short otyp)
{
    short ap = mtmp->mappearance;

    switch (M_AP_TYPE(mtmp)) {
    case M_AP_NOTHING:
    case M_AP_FURNITURE:
    case M_AP_MONSTER:
        break;
    case M_AP_OBJECT:
        if (otyp == SPE_HEALING || otyp == SPE_EXTRA_HEALING) {
            pline("%s seems a more vivid %s than before.",
                  The(simple_typename(ap)),
                  c_obj_colors[objects[ap].oc_color]);
        }
        break;
    }
}

boolean
usmellmon(struct permonst* mdat)
{
    int mndx;
    boolean nonspecific = FALSE;
    boolean msg_given = FALSE;

    if (mdat) {
        if (!olfaction(g.youmonst.data))
            return FALSE;
        mndx = monsndx(mdat);
        switch (mndx) {
        case PM_ROTHE:
        case PM_MINOTAUR:
            You("notice a bovine smell.");
            msg_given = TRUE;
            break;
        case PM_CAVE_DWELLER:
        case PM_BARBARIAN:
        case PM_NEANDERTHAL:
            You("smell body odor.");
            msg_given = TRUE;
            break;
        /*
        case PM_PESTILENCE:
        case PM_FAMINE:
        case PM_DEATH:
            break;
        */
        case PM_HORNED_DEVIL:
        case PM_BALROG:
        case PM_ASMODEUS:
        case PM_DISPATER:
        case PM_YEENOGHU:
        case PM_ORCUS:
            break;
        case PM_HUMAN_WEREJACKAL:
        case PM_HUMAN_WERERAT:
        case PM_HUMAN_WEREWOLF:
        case PM_WEREJACKAL:
        case PM_WERERAT:
        case PM_WEREWOLF:
        case PM_OWLBEAR:
            You("detect an odor reminiscent of an animal's den.");
            msg_given = TRUE;
            break;
        /*
        case PM_PURPLE_WORM:
            break;
        */
        case PM_STEAM_VORTEX:
            You("smell steam.");
            msg_given = TRUE;
            break;
        case PM_GREEN_SLIME:
            pline("%s stinks.", Something);
            msg_given = TRUE;
            break;
        case PM_VIOLET_FUNGUS:
        case PM_SHRIEKER:
            You("smell mushrooms.");
            msg_given = TRUE;
            break;
        /* These are here to avoid triggering the
           nonspecific treatment through the default case below*/
        case PM_WHITE_UNICORN:
        case PM_GRAY_UNICORN:
        case PM_BLACK_UNICORN:
        case PM_JELLYFISH:
            break;
        default:
            nonspecific = TRUE;
            break;
        }

        if (nonspecific)
            switch (mdat->mlet) {
            case S_DOG:
                You("notice a dog smell.");
                msg_given = TRUE;
                break;
            case S_DRAGON:
                You("smell a dragon!");
                msg_given = TRUE;
                break;
            case S_FUNGUS:
                pline("%s smells moldy.", Something);
                msg_given = TRUE;
                break;
            case S_UNICORN:
                You("detect a%s odor reminiscent of a stable.",
                    (mndx == PM_PONY) ? "n" : " strong");
                msg_given = TRUE;
                break;
            case S_ZOMBIE:
                You("smell rotting flesh.");
                msg_given = TRUE;
                break;
            case S_EEL:
                You("smell fish.");
                msg_given = TRUE;
                break;
            case S_ORC:
                if (maybe_polyd(is_orc(g.youmonst.data), Race_if(PM_ORC)))
                    You("notice an attractive smell.");
                else
                    pline("A foul stench makes you feel a little nauseated.");
                msg_given = TRUE;
                break;
            default:
                break;
            }
    }
    return msg_given ? TRUE : FALSE;
}

/*mon.c*/

/* NetHack 3.6	eat.c	$NHDT-Date: 1574900825 2019/11/28 00:27:05 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.206 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(eatmdone);
STATIC_PTR int NDECL(eatfood);
STATIC_PTR struct obj *FDECL(costly_tin, (int));
STATIC_PTR int NDECL(opentin);
STATIC_PTR int NDECL(unfaint);

STATIC_DCL const char *FDECL(food_xname, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(choke, (struct obj *));
STATIC_DCL void NDECL(recalc_wt);
STATIC_DCL unsigned FDECL(obj_nutrition, (struct obj *));
STATIC_DCL struct obj *FDECL(touchfood, (struct obj *));
STATIC_DCL void NDECL(do_reset_eat);
STATIC_DCL void FDECL(done_eating, (BOOLEAN_P));
STATIC_DCL void FDECL(cprefx, (int));
STATIC_DCL int FDECL(intrinsic_possible, (int, struct permonst *));
STATIC_DCL void FDECL(givit, (int, struct permonst *));
STATIC_DCL void FDECL(cpostfx, (int));
STATIC_DCL void FDECL(consume_tin, (const char *));
STATIC_DCL void FDECL(start_tin, (struct obj *));
STATIC_DCL int FDECL(eatcorpse, (struct obj *));
STATIC_DCL void FDECL(start_eating, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(fprefx, (struct obj *));
STATIC_DCL void FDECL(fpostfx, (struct obj *));
STATIC_DCL int NDECL(bite);
STATIC_DCL int FDECL(edibility_prompts, (struct obj *));
STATIC_DCL int FDECL(rottenfood, (struct obj *));
STATIC_DCL void NDECL(eatspecial);
STATIC_DCL int FDECL(bounded_increase, (int, int, int));
STATIC_DCL void FDECL(accessory_has_effect, (struct obj *));
STATIC_DCL void FDECL(eataccessory, (struct obj *));
STATIC_DCL const char *FDECL(foodword, (struct obj *));
STATIC_DCL int FDECL(tin_variety, (struct obj *, BOOLEAN_P));
STATIC_DCL boolean FDECL(maybe_cannibal, (int, BOOLEAN_P));

char msgbuf[BUFSZ];

/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC))

/* monster types that cause hero to be turned into stone if eaten */
#define flesh_petrifies(pm) (touch_petrifies(pm) || (pm) == &mons[PM_MEDUSA])

/* Rider corpses are treated as non-rotting so that attempting to eat one
   will be sure to reach the stage of eating where that meal is fatal */
#define nonrotting_corpse(mnum) \
    ((mnum) == PM_LIZARD || (mnum) == PM_LICHEN || is_rider(&mons[mnum]))

/* non-rotting non-corpses; unlike lizard corpses, these items will behave
   as if rotten if they are cursed (fortune cookies handled elsewhere) */
#define nonrotting_food(otyp) \
    ((otyp) == LEMBAS_WAFER || (otyp) == CRAM_RATION)

STATIC_OVL NEARDATA const char comestibles[] = { FOOD_CLASS, 0 };
STATIC_OVL NEARDATA const char offerfodder[] = { FOOD_CLASS, AMULET_CLASS,
                                                 0 };

/* Gold must come first for getobj(). */
STATIC_OVL NEARDATA const char allobj[] = {
    COIN_CLASS,   WEAPON_CLASS, ARMOR_CLASS,  POTION_CLASS,
    SCROLL_CLASS, WAND_CLASS,   RING_CLASS,   AMULET_CLASS,
    FOOD_CLASS,   TOOL_CLASS,   GEM_CLASS,    ROCK_CLASS,
    BALL_CLASS,   CHAIN_CLASS,  SPBOOK_CLASS, 0
};

STATIC_OVL boolean force_save_hs = FALSE;

/* see hunger states in hack.h - texts used on bottom line */
const char *hu_stat[] = { "Satiated", "        ", "Hungry  ", "Weak    ",
                          "Fainting", "Fainted ", "Starved " };

/*
 * Decide whether a particular object can be eaten by the possibly
 * polymorphed character.  Not used for monster checks.
 */
boolean
is_edible(obj)
register struct obj *obj;
{
    /* protect invocation tools but not Rider corpses (handled elsewhere)*/
    /* if (obj->oclass != FOOD_CLASS && obj_resists(obj, 0, 0)) */
    if (objects[obj->otyp].oc_unique)
        return FALSE;
    /* above also prevents the Amulet from being eaten, so we must never
       allow fake amulets to be eaten either [which is already the case] */

    if (metallivorous(youmonst.data) && is_metallic(obj)
        && (youmonst.data != &mons[PM_RUST_MONSTER] || is_rustprone(obj)))
        return TRUE;

    /* Ghouls only eat non-veggy corpses or eggs (see dogfood()) */
    if (u.umonnum == PM_GHOUL)
        return (boolean)((obj->otyp == CORPSE
                          && !vegan(&mons[obj->corpsenm]))
                         || (obj->otyp == EGG));

    if (u.umonnum == PM_GELATINOUS_CUBE && is_organic(obj)
        /* [g.cubes can eat containers and retain all contents
            as engulfed items, but poly'd player can't do that] */
        && !Has_contents(obj))
        return TRUE;

    /* return (boolean) !!index(comestibles, obj->oclass); */
    return (boolean) (obj->oclass == FOOD_CLASS);
}

/* used for hero init, life saving (if choking), and prayer results of fix
   starving, fix weak from hunger, or golden glow boon (if u.uhunger < 900) */
void
init_uhunger()
{
    context.botl = (u.uhs != NOT_HUNGRY || ATEMP(A_STR) < 0);
    u.uhunger = 900;
    u.uhs = NOT_HUNGRY;
    if (ATEMP(A_STR) < 0) {
        ATEMP(A_STR) = 0;
        (void) encumber_msg();
    }
}

/* tin types [SPINACH_TIN = -1, overrides corpsenm, nut==600] */
static const struct {
    const char *txt;                      /* description */
    int nut;                              /* nutrition */
    Bitfield(fodder, 1);                  /* stocked by health food shops */
    Bitfield(greasy, 1);                  /* causes slippery fingers */
} tintxts[] = { { "rotten", -50, 0, 0 },  /* ROTTEN_TIN = 0 */
                { "homemade", 50, 1, 0 }, /* HOMEMADE_TIN = 1 */
                { "soup made from", 20, 1, 0 },
                { "french fried", 40, 0, 1 },
                { "pickled", 40, 1, 0 },
                { "boiled", 50, 1, 0 },
                { "smoked", 50, 1, 0 },
                { "dried", 55, 1, 0 },
                { "deep fried", 60, 0, 1 },
                { "szechuan", 70, 1, 0 },
                { "broiled", 80, 0, 0 },
                { "stir fried", 80, 0, 1 },
                { "sauteed", 95, 0, 0 },
                { "candied", 100, 1, 0 },
                { "pureed", 500, 1, 0 },
                { "", 0, 0, 0 } };
#define TTSZ SIZE(tintxts)

static char *eatmbuf = 0; /* set by cpostfx() */

/* called after mimicing is over */
STATIC_PTR int
eatmdone(VOID_ARGS)
{
    /* release `eatmbuf' */
    if (eatmbuf) {
        if (nomovemsg == eatmbuf)
            nomovemsg = 0;
        free((genericptr_t) eatmbuf), eatmbuf = 0;
    }
    /* update display */
    if (U_AP_TYPE) {
        youmonst.m_ap_type = M_AP_NOTHING;
        newsym(u.ux, u.uy);
    }
    return 0;
}

/* called when hallucination is toggled */
void
eatmupdate()
{
    const char *altmsg = 0;
    int altapp = 0; /* lint suppression */

    if (!eatmbuf || nomovemsg != eatmbuf)
        return;

    if (is_obj_mappear(&youmonst,ORANGE) && !Hallucination) {
        /* revert from hallucinatory to "normal" mimicking */
        altmsg = "You now prefer mimicking yourself.";
        altapp = GOLD_PIECE;
    } else if (is_obj_mappear(&youmonst,GOLD_PIECE) && Hallucination) {
        /* won't happen; anything which might make immobilized
           hero begin hallucinating (black light attack, theft
           of Grayswandir) will terminate the mimicry first */
        altmsg = "Your rind escaped intact.";
        altapp = ORANGE;
    }

    if (altmsg) {
        /* replace end-of-mimicking message */
        if (strlen(altmsg) > strlen(eatmbuf)) {
            free((genericptr_t) eatmbuf);
            eatmbuf = (char *) alloc(strlen(altmsg) + 1);
        }
        nomovemsg = strcpy(eatmbuf, altmsg);
        /* update current image */
        youmonst.mappearance = altapp;
        newsym(u.ux, u.uy);
    }
}

/* ``[the(] singular(food, xname) [)]'' */
STATIC_OVL const char *
food_xname(food, the_pfx)
struct obj *food;
boolean the_pfx;
{
    const char *result;

    if (food->otyp == CORPSE) {
        result = corpse_xname(food, (const char *) 0,
                              CXN_SINGULAR | (the_pfx ? CXN_PFX_THE : 0));
        /* not strictly needed since pname values are capitalized
           and the() is a no-op for them */
        if (type_is_pname(&mons[food->corpsenm]))
            the_pfx = FALSE;
    } else {
        /* the ordinary case */
        result = singular(food, xname);
    }
    if (the_pfx)
        result = the(result);
    return result;
}

/* Created by GAN 01/28/87
 * Amended by AKP 09/22/87: if not hard, don't choke, just vomit.
 * Amended by 3.  06/12/89: if not hard, sometimes choke anyway, to keep risk.
 *                11/10/89: if hard, rarely vomit anyway, for slim chance.
 *
 * To a full belly all food is bad. (It.)
 */
STATIC_OVL void
choke(food)
struct obj *food;
{
    /* only happens if you were satiated */
    if (u.uhs != SATIATED) {
        if (!food || food->otyp != AMULET_OF_STRANGULATION)
            return;
    } else if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL) {
        adjalign(-1); /* gluttony is unchivalrous */
        You_feel("like a glutton!");
    }

    exercise(A_CON, FALSE);

    if (Breathless || (!Strangled && !rn2(20))) {
        /* choking by eating AoS doesn't involve stuffing yourself */
        if (food && food->otyp == AMULET_OF_STRANGULATION) {
            You("choke, but recover your composure.");
            return;
        }
        You("stuff yourself and then vomit voluminously.");
        morehungry(1000); /* you just got *very* sick! */
        vomit();
    } else {
        killer.format = KILLED_BY_AN;
        /*
         * Note all "killer"s below read "Choked on %s" on the
         * high score list & tombstone.  So plan accordingly.
         */
        if (food) {
            You("choke over your %s.", foodword(food));
            if (food->oclass == COIN_CLASS) {
                Strcpy(killer.name, "very rich meal");
            } else {
                killer.format = KILLED_BY;
                Strcpy(killer.name, killer_xname(food));
            }
        } else {
            You("choke over it.");
            Strcpy(killer.name, "quick snack");
        }
        You("die...");
        done(CHOKING);
    }
}

/* modify object wt. depending on time spent consuming it */
STATIC_OVL void
recalc_wt()
{
    struct obj *piece = context.victual.piece;
    if (!piece) {
        impossible("recalc_wt without piece");
        return;
    }
    debugpline1("Old weight = %d", piece->owt);
    debugpline2("Used time = %d, Req'd time = %d", context.victual.usedtime,
                context.victual.reqtime);
    piece->owt = weight(piece);
    debugpline1("New weight = %d", piece->owt);
}

/* called when eating interrupted by an event */
void
reset_eat()
{
    /* we only set a flag here - the actual reset process is done after
     * the round is spent eating.
     */
    if (context.victual.eating && !context.victual.doreset) {
        debugpline0("reset_eat...");
        context.victual.doreset = TRUE;
    }
    return;
}

/* base nutrition of a food-class object */
STATIC_OVL unsigned
obj_nutrition(otmp)
struct obj *otmp;
{
    unsigned nut = (otmp->otyp == CORPSE) ? mons[otmp->corpsenm].cnutrit
                      : otmp->globby ? otmp->owt
                         : (unsigned) objects[otmp->otyp].oc_nutrition;

    if (otmp->otyp == LEMBAS_WAFER) {
        if (maybe_polyd(is_elf(youmonst.data), Race_if(PM_ELF)))
            nut += nut / 4; /* 800 -> 1000 */
        else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC)))
            nut -= nut / 4; /* 800 -> 600 */
        /* prevent polymorph making a partly eaten wafer
           become more nutritious than an untouched one */
        if (otmp->oeaten >= nut)
            otmp->oeaten = (otmp->oeaten < objects[LEMBAS_WAFER].oc_nutrition)
                              ? (nut - 1) : nut;
    } else if (otmp->otyp == CRAM_RATION) {
        if (maybe_polyd(is_dwarf(youmonst.data), Race_if(PM_DWARF)))
            nut += nut / 6; /* 600 -> 700 */
    }
    return nut;
}

STATIC_OVL struct obj *
touchfood(otmp)
struct obj *otmp;
{
    if (otmp->quan > 1L) {
        if (!carried(otmp))
            (void) splitobj(otmp, otmp->quan - 1L);
        else
            otmp = splitobj(otmp, 1L);
        debugpline0("split object,");
    }

    if (!otmp->oeaten) {
        costly_alteration(otmp, COST_BITE);
        otmp->oeaten = obj_nutrition(otmp);
    }

    if (carried(otmp)) {
        freeinv(otmp);
        if (inv_cnt(FALSE) >= 52) {
            sellobj_state(SELL_DONTSELL);
            dropy(otmp);
            sellobj_state(SELL_NORMAL);
        } else {
            otmp->nomerge = 1; /* used to prevent merge */
            otmp = addinv(otmp);
            otmp->nomerge = 0;
        }
    }
    return otmp;
}

/* When food decays, in the middle of your meal, we don't want to dereference
 * any dangling pointers, so set it to null (which should still trigger
 * do_reset_eat() at the beginning of eatfood()) and check for null pointers
 * in do_reset_eat().
 */
void
food_disappears(obj)
struct obj *obj;
{
    if (obj == context.victual.piece) {
        context.victual.piece = (struct obj *) 0;
        context.victual.o_id = 0;
    }
    if (obj->timed)
        obj_stop_timers(obj);
}

/* renaming an object used to result in it having a different address,
   so the sequence start eating/opening, get interrupted, name the food,
   resume eating/opening would restart from scratch */
void
food_substitution(old_obj, new_obj)
struct obj *old_obj, *new_obj;
{
    if (old_obj == context.victual.piece) {
        context.victual.piece = new_obj;
        context.victual.o_id = new_obj->o_id;
    }
    if (old_obj == context.tin.tin) {
        context.tin.tin = new_obj;
        context.tin.o_id = new_obj->o_id;
    }
}

STATIC_OVL void
do_reset_eat()
{
    debugpline0("do_reset_eat...");
    if (context.victual.piece) {
        context.victual.o_id = 0;
        context.victual.piece = touchfood(context.victual.piece);
        if (context.victual.piece)
            context.victual.o_id = context.victual.piece->o_id;
        recalc_wt();
    }
    context.victual.fullwarn = context.victual.eating =
        context.victual.doreset = FALSE;
    /* Do not set canchoke to FALSE; if we continue eating the same object
     * we need to know if canchoke was set when they started eating it the
     * previous time.  And if we don't continue eating the same object
     * canchoke always gets recalculated anyway.
     */
    stop_occupation();
    newuhs(FALSE);
}

/* called each move during eating process */
STATIC_PTR int
eatfood(VOID_ARGS)
{
    if (!context.victual.piece
        || (!carried(context.victual.piece)
            && !obj_here(context.victual.piece, u.ux, u.uy))) {
        /* maybe it was stolen? */
        do_reset_eat();
        return 0;
    }
    if (!context.victual.eating)
        return 0;

    if (++context.victual.usedtime <= context.victual.reqtime) {
        if (bite())
            return 0;
        return 1; /* still busy */
    } else {        /* done */
        done_eating(TRUE);
        return 0;
    }
}

STATIC_OVL void
done_eating(message)
boolean message;
{
    struct obj *piece = context.victual.piece;

    piece->in_use = TRUE;
    occupation = 0; /* do this early, so newuhs() knows we're done */
    newuhs(FALSE);
    if (nomovemsg) {
        if (message)
            pline1(nomovemsg);
        nomovemsg = 0;
    } else if (message)
        You("finish eating %s.", food_xname(piece, TRUE));

    if (piece->otyp == CORPSE || piece->globby)
        cpostfx(piece->corpsenm);
    else
        fpostfx(piece);

    if (carried(piece))
        useup(piece);
    else
        useupf(piece, 1L);
    context.victual.piece = (struct obj *) 0;
    context.victual.o_id = 0;
    context.victual.fullwarn = context.victual.eating =
        context.victual.doreset = FALSE;
}

void
eating_conducts(pd)
struct permonst *pd;
{
    u.uconduct.food++;
    if (!vegan(pd))
        u.uconduct.unvegan++;
    if (!vegetarian(pd))
        violated_vegetarian();
}

/* handle side-effects of mind flayer's tentacle attack */
int
eat_brains(magr, mdef, visflag, dmg_p)
struct monst *magr, *mdef;
boolean visflag;
int *dmg_p; /* for dishing out extra damage in lieu of Int loss */
{
    struct permonst *pd = mdef->data;
    boolean give_nutrit = FALSE;
    int result = MM_HIT, xtra_dmg = rnd(10);

    if (noncorporeal(pd)) {
        if (visflag)
            pline("%s brain is unharmed.",
                  (mdef == &youmonst) ? "Your" : s_suffix(Monnam(mdef)));
        return MM_MISS; /* side-effects can't occur */
    } else if (magr == &youmonst) {
        You("eat %s brain!", s_suffix(mon_nam(mdef)));
    } else if (mdef == &youmonst) {
        Your("brain is eaten!");
    } else { /* monster against monster */
        if (visflag && canspotmon(mdef))
            pline("%s brain is eaten!", s_suffix(Monnam(mdef)));
    }

    if (flesh_petrifies(pd)) {
        /* mind flayer has attempted to eat the brains of a petrification
           inducing critter (most likely Medusa; attacking a cockatrice via
           tentacle-touch should have been caught before reaching this far) */
        if (magr == &youmonst) {
            if (!Stone_resistance && !Stoned)
                make_stoned(5L, (char *) 0, KILLED_BY_AN, pd->mname);
        } else {
            /* no need to check for poly_when_stoned or Stone_resistance;
               mind flayers don't have those capabilities */
            if (visflag && canseemon(magr))
                pline("%s turns to stone!", Monnam(magr));
            monstone(magr);
            if (!DEADMONSTER(magr)) {
                /* life-saved; don't continue eating the brains */
                return MM_MISS;
            } else {
                if (magr->mtame && !visflag)
                    /* parallels mhitm.c's brief_feeling */
                    You("have a sad thought for a moment, then it passes.");
                return MM_AGR_DIED;
            }
        }
    }

    if (magr == &youmonst) {
        /*
         * player mind flayer is eating something's brain
         */
        eating_conducts(pd);
        if (mindless(pd)) { /* (cannibalism not possible here) */
            pline("%s doesn't notice.", Monnam(mdef));
            /* all done; no extra harm inflicted upon target */
            return MM_MISS;
        } else if (is_rider(pd)) {
            pline("Ingesting that is fatal.");
            Sprintf(killer.name, "unwisely ate the brain of %s", pd->mname);
            killer.format = NO_KILLER_PREFIX;
            done(DIED);
            /* life-saving needed to reach here */
            exercise(A_WIS, FALSE);
            *dmg_p += xtra_dmg; /* Rider takes extra damage */
        } else {
            morehungry(-rnd(30)); /* cannot choke */
            if (ABASE(A_INT) < AMAX(A_INT)) {
                /* recover lost Int; won't increase current max */
                ABASE(A_INT) += rnd(4);
                if (ABASE(A_INT) > AMAX(A_INT))
                    ABASE(A_INT) = AMAX(A_INT);
                context.botl = 1;
            }
            exercise(A_WIS, TRUE);
            *dmg_p += xtra_dmg;
        }
        /* targetting another mind flayer or your own underlying species
           is cannibalism */
        (void) maybe_cannibal(monsndx(pd), TRUE);

    } else if (mdef == &youmonst) {
        /*
         * monster mind flayer is eating hero's brain
         */
        /* no such thing as mindless players */
        if (ABASE(A_INT) <= ATTRMIN(A_INT)) {
            static NEARDATA const char brainlessness[] = "brainlessness";

            if (Lifesaved) {
                Strcpy(killer.name, brainlessness);
                killer.format = KILLED_BY;
                done(DIED);
                /* amulet of life saving has now been used up */
                pline("Unfortunately your brain is still gone.");
                /* sanity check against adding other forms of life-saving */
                u.uprops[LIFESAVED].extrinsic =
                    u.uprops[LIFESAVED].intrinsic = 0L;
            } else {
                Your("last thought fades away.");
            }
            Strcpy(killer.name, brainlessness);
            killer.format = KILLED_BY;
            done(DIED);
            /* can only get here when in wizard or explore mode and user has
               explicitly chosen not to die; arbitrarily boost intelligence */
            ABASE(A_INT) = ATTRMIN(A_INT) + 2;
            You_feel("like a scarecrow.");
        }
        give_nutrit = TRUE; /* in case a conflicted pet is doing this */
        exercise(A_WIS, FALSE);
        /* caller handles Int and memory loss */

    } else { /* mhitm */
        /*
         * monster mind flayer is eating another monster's brain
         */
        if (mindless(pd)) {
            if (visflag && canspotmon(mdef))
                pline("%s doesn't notice.", Monnam(mdef));
            return MM_MISS;
        } else if (is_rider(pd)) {
            mondied(magr);
            if (DEADMONSTER(magr))
                result = MM_AGR_DIED;
            /* Rider takes extra damage regardless of whether attacker dies */
            *dmg_p += xtra_dmg;
        } else {
            *dmg_p += xtra_dmg;
            give_nutrit = TRUE;
            if (*dmg_p >= mdef->mhp && visflag && canspotmon(mdef))
                pline("%s last thought fades away...",
                      s_suffix(Monnam(mdef)));
        }
    }

    if (give_nutrit && magr->mtame && !magr->isminion) {
        EDOG(magr)->hungrytime += rnd(60);
        magr->mconf = 0;
    }

    return result;
}

/* eating a corpse or egg of one's own species is usually naughty */
STATIC_OVL boolean
maybe_cannibal(pm, allowmsg)
int pm;
boolean allowmsg;
{
    static NEARDATA long ate_brains = 0L;
    struct permonst *fptr = &mons[pm]; /* food type */

    /* when poly'd into a mind flayer, multiple tentacle hits in one
       turn cause multiple digestion checks to occur; avoid giving
       multiple luck penalties for the same attack */
    if (moves == ate_brains)
        return FALSE;
    ate_brains = moves; /* ate_anything, not just brains... */

    if (!CANNIBAL_ALLOWED()
        /* non-cannibalistic heroes shouldn't eat own species ever
           and also shouldn't eat current species when polymorphed
           (even if having the form of something which doesn't care
           about cannibalism--hero's innate traits aren't altered) */
        && (your_race(fptr)
            || (Upolyd && same_race(youmonst.data, fptr))
            || (u.ulycn >= LOW_PM && were_beastie(pm) == u.ulycn))) {
        if (allowmsg) {
            if (Upolyd && your_race(fptr))
                You("have a bad feeling deep inside.");
            You("cannibal!  You will regret this!");
        }
        HAggravate_monster |= FROMOUTSIDE;
        change_luck(-rn1(4, 2)); /* -5..-2 */
        return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
cprefx(pm)
register int pm;
{
    (void) maybe_cannibal(pm, TRUE);
    if (flesh_petrifies(&mons[pm])) {
        if (!Stone_resistance
            && !(poly_when_stoned(youmonst.data)
                 && polymon(PM_STONE_GOLEM))) {
            Sprintf(killer.name, "tasting %s meat", mons[pm].mname);
            killer.format = KILLED_BY;
            You("turn to stone.");
            done(STONING);
            if (context.victual.piece)
                context.victual.eating = FALSE;
            return; /* lifesaved */
        }
    }

    switch (pm) {
    case PM_LITTLE_DOG:
    case PM_DOG:
    case PM_LARGE_DOG:
    case PM_KITTEN:
    case PM_HOUSECAT:
    case PM_LARGE_CAT:
        /* cannibals are allowed to eat domestic animals without penalty */
        if (!CANNIBAL_ALLOWED()) {
            You_feel("that eating the %s was a bad idea.", mons[pm].mname);
            HAggravate_monster |= FROMOUTSIDE;
        }
        break;
    case PM_LIZARD:
        if (Stoned)
            fix_petrification();
        break;
    case PM_DEATH:
    case PM_PESTILENCE:
    case PM_FAMINE: {
        pline("Eating that is instantly fatal.");
        Sprintf(killer.name, "unwisely ate the body of %s", mons[pm].mname);
        killer.format = NO_KILLER_PREFIX;
        done(DIED);
        /* life-saving needed to reach here */
        exercise(A_WIS, FALSE);
        /* It so happens that since we know these monsters */
        /* cannot appear in tins, context.victual.piece will always */
        /* be what we want, which is not generally true. */
        if (revive_corpse(context.victual.piece)) {
            context.victual.piece = (struct obj *) 0;
            context.victual.o_id = 0;
        }
        return;
    }
    case PM_GREEN_SLIME:
        if (!Slimed && !Unchanging && !slimeproof(youmonst.data)) {
            You("don't feel very well.");
            make_slimed(10L, (char *) 0);
            delayed_killer(SLIMED, KILLED_BY_AN, "");
        }
    /* Fall through */
    default:
        if (acidic(&mons[pm]) && Stoned)
            fix_petrification();
        break;
    }
}

void
fix_petrification()
{
    char buf[BUFSZ];

    if (Hallucination)
        Sprintf(buf, "What a pity--you just ruined a future piece of %sart!",
                ACURR(A_CHA) > 15 ? "fine " : "");
    else
        Strcpy(buf, "You feel limber!");
    make_stoned(0L, buf, 0, (char *) 0);
}

/*
 * If you add an intrinsic that can be gotten by eating a monster, add it
 * to intrinsic_possible() and givit().  (It must already be in prop.h to
 * be an intrinsic property.)
 * It would be very easy to make the intrinsics not try to give you one
 * that you already had by checking to see if you have it in
 * intrinsic_possible() instead of givit(), but we're not that nice.
 */

/* intrinsic_possible() returns TRUE iff a monster can give an intrinsic. */
STATIC_OVL int
intrinsic_possible(type, ptr)
int type;
register struct permonst *ptr;
{
    int res = 0;

#ifdef DEBUG
#define ifdebugresist(Msg)      \
    do {                        \
        if (res)                \
            debugpline0(Msg);   \
    } while (0)
#else
#define ifdebugresist(Msg) /*empty*/
#endif
    switch (type) {
    case FIRE_RES:
        res = (ptr->mconveys & MR_FIRE) != 0;
        ifdebugresist("can get fire resistance");
        break;
    case SLEEP_RES:
        res = (ptr->mconveys & MR_SLEEP) != 0;
        ifdebugresist("can get sleep resistance");
        break;
    case COLD_RES:
        res = (ptr->mconveys & MR_COLD) != 0;
        ifdebugresist("can get cold resistance");
        break;
    case DISINT_RES:
        res = (ptr->mconveys & MR_DISINT) != 0;
        ifdebugresist("can get disintegration resistance");
        break;
    case SHOCK_RES: /* shock (electricity) resistance */
        res = (ptr->mconveys & MR_ELEC) != 0;
        ifdebugresist("can get shock resistance");
        break;
    case POISON_RES:
        res = (ptr->mconveys & MR_POISON) != 0;
        ifdebugresist("can get poison resistance");
        break;
    case TELEPORT:
        res = can_teleport(ptr);
        ifdebugresist("can get teleport");
        break;
    case TELEPORT_CONTROL:
        res = control_teleport(ptr);
        ifdebugresist("can get teleport control");
        break;
    case TELEPAT:
        res = telepathic(ptr);
        ifdebugresist("can get telepathy");
        break;
    default:
        /* res stays 0 */
        break;
    }
#undef ifdebugresist
    return res;
}

/* givit() tries to give you an intrinsic based on the monster's level
 * and what type of intrinsic it is trying to give you.
 */
STATIC_OVL void
givit(type, ptr)
int type;
register struct permonst *ptr;
{
    register int chance;

    debugpline1("Attempting to give intrinsic %d", type);
    /* some intrinsics are easier to get than others */
    switch (type) {
    case POISON_RES:
        if ((ptr == &mons[PM_KILLER_BEE] || ptr == &mons[PM_SCORPION])
            && !rn2(4))
            chance = 1;
        else
            chance = 15;
        break;
    case TELEPORT:
        chance = 10;
        break;
    case TELEPORT_CONTROL:
        chance = 12;
        break;
    case TELEPAT:
        chance = 1;
        break;
    default:
        chance = 15;
        break;
    }

    if (ptr->mlevel <= rn2(chance))
        return; /* failed die roll */

    switch (type) {
    case FIRE_RES:
        debugpline0("Trying to give fire resistance");
        if (!(HFire_resistance & FROMOUTSIDE)) {
            You(Hallucination ? "be chillin'." : "feel a momentary chill.");
            HFire_resistance |= FROMOUTSIDE;
        }
        break;
    case SLEEP_RES:
        debugpline0("Trying to give sleep resistance");
        if (!(HSleep_resistance & FROMOUTSIDE)) {
            You_feel("wide awake.");
            HSleep_resistance |= FROMOUTSIDE;
        }
        break;
    case COLD_RES:
        debugpline0("Trying to give cold resistance");
        if (!(HCold_resistance & FROMOUTSIDE)) {
            You_feel("full of hot air.");
            HCold_resistance |= FROMOUTSIDE;
        }
        break;
    case DISINT_RES:
        debugpline0("Trying to give disintegration resistance");
        if (!(HDisint_resistance & FROMOUTSIDE)) {
            You_feel(Hallucination ? "totally together, man." : "very firm.");
            HDisint_resistance |= FROMOUTSIDE;
        }
        break;
    case SHOCK_RES: /* shock (electricity) resistance */
        debugpline0("Trying to give shock resistance");
        if (!(HShock_resistance & FROMOUTSIDE)) {
            if (Hallucination)
                You_feel("grounded in reality.");
            else
                Your("health currently feels amplified!");
            HShock_resistance |= FROMOUTSIDE;
        }
        break;
    case POISON_RES:
        debugpline0("Trying to give poison resistance");
        if (!(HPoison_resistance & FROMOUTSIDE)) {
            You_feel(Poison_resistance ? "especially healthy." : "healthy.");
            HPoison_resistance |= FROMOUTSIDE;
        }
        break;
    case TELEPORT:
        debugpline0("Trying to give teleport");
        if (!(HTeleportation & FROMOUTSIDE)) {
            You_feel(Hallucination ? "diffuse." : "very jumpy.");
            HTeleportation |= FROMOUTSIDE;
        }
        break;
    case TELEPORT_CONTROL:
        debugpline0("Trying to give teleport control");
        if (!(HTeleport_control & FROMOUTSIDE)) {
            You_feel(Hallucination ? "centered in your personal space."
                                   : "in control of yourself.");
            HTeleport_control |= FROMOUTSIDE;
        }
        break;
    case TELEPAT:
        debugpline0("Trying to give telepathy");
        if (!(HTelepat & FROMOUTSIDE)) {
            You_feel(Hallucination ? "in touch with the cosmos."
                                   : "a strange mental acuity.");
            HTelepat |= FROMOUTSIDE;
            /* If blind, make sure monsters show up. */
            if (Blind)
                see_monsters();
        }
        break;
    default:
        debugpline0("Tried to give an impossible intrinsic");
        break;
    }
}

/* called after completely consuming a corpse */
STATIC_OVL void
cpostfx(pm)
int pm;
{
    int tmp = 0;
    int catch_lycanthropy = NON_PM;
    boolean check_intrinsics = FALSE;

    /* in case `afternmv' didn't get called for previously mimicking
       gold, clean up now to avoid `eatmbuf' memory leak */
    if (eatmbuf)
        (void) eatmdone();

    switch (pm) {
    case PM_NEWT:
        /* MRKR: "eye of newt" may give small magical energy boost */
        if (rn2(3) || 3 * u.uen <= 2 * u.uenmax) {
            int old_uen = u.uen;

            u.uen += rnd(3);
            if (u.uen > u.uenmax) {
                if (!rn2(3))
                    u.uenmax++;
                u.uen = u.uenmax;
            }
            if (old_uen != u.uen) {
                You_feel("a mild buzz.");
                context.botl = 1;
            }
        }
        break;
    case PM_WRAITH:
        pluslvl(FALSE);
        break;
    case PM_HUMAN_WERERAT:
        catch_lycanthropy = PM_WERERAT;
        break;
    case PM_HUMAN_WEREJACKAL:
        catch_lycanthropy = PM_WEREJACKAL;
        break;
    case PM_HUMAN_WEREWOLF:
        catch_lycanthropy = PM_WEREWOLF;
        break;
    case PM_NURSE:
        if (Upolyd)
            u.mh = u.mhmax;
        else
            u.uhp = u.uhpmax;
        make_blinded(0L, !u.ucreamed);
        context.botl = 1;
        check_intrinsics = TRUE; /* might also convey poison resistance */
        break;
    case PM_STALKER:
        if (!Invis) {
            set_itimeout(&HInvis, (long) rn1(100, 50));
            if (!Blind && !BInvis)
                self_invis_message();
        } else {
            if (!(HInvis & INTRINSIC))
                You_feel("hidden!");
            HInvis |= FROMOUTSIDE;
            HSee_invisible |= FROMOUTSIDE;
        }
        newsym(u.ux, u.uy);
        /*FALLTHRU*/
    case PM_YELLOW_LIGHT:
    case PM_GIANT_BAT:
        make_stunned((HStun & TIMEOUT) + 30L, FALSE);
        /*FALLTHRU*/
    case PM_BAT:
        make_stunned((HStun & TIMEOUT) + 30L, FALSE);
        break;
    case PM_GIANT_MIMIC:
        tmp += 10;
        /*FALLTHRU*/
    case PM_LARGE_MIMIC:
        tmp += 20;
        /*FALLTHRU*/
    case PM_SMALL_MIMIC:
        tmp += 20;
        if (youmonst.data->mlet != S_MIMIC && !Unchanging) {
            char buf[BUFSZ];

            u.uconduct.polyselfs++; /* you're changing form */
            You_cant("resist the temptation to mimic %s.",
                     Hallucination ? "an orange" : "a pile of gold");
            /* A pile of gold can't ride. */
            if (u.usteed)
                dismount_steed(DISMOUNT_FELL);
            nomul(-tmp);
            multi_reason = "pretending to be a pile of gold";
            Sprintf(buf,
                    Hallucination
                       ? "You suddenly dread being peeled and mimic %s again!"
                       : "You now prefer mimicking %s again.",
                    an(Upolyd ? youmonst.data->mname : urace.noun));
            eatmbuf = dupstr(buf);
            nomovemsg = eatmbuf;
            afternmv = eatmdone;
            /* ??? what if this was set before? */
            youmonst.m_ap_type = M_AP_OBJECT;
            youmonst.mappearance = Hallucination ? ORANGE : GOLD_PIECE;
            newsym(u.ux, u.uy);
            curs_on_u();
            /* make gold symbol show up now */
            display_nhwindow(WIN_MAP, TRUE);
        }
        break;
    case PM_QUANTUM_MECHANIC:
        Your("velocity suddenly seems very uncertain!");
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
            You("seem slower.");
        } else {
            HFast |= FROMOUTSIDE;
            You("seem faster.");
        }
        break;
    case PM_LIZARD:
        if ((HStun & TIMEOUT) > 2)
            make_stunned(2L, FALSE);
        if ((HConfusion & TIMEOUT) > 2)
            make_confused(2L, FALSE);
        break;
    case PM_CHAMELEON:
    case PM_DOPPELGANGER:
    case PM_SANDESTIN: /* moot--they don't leave corpses */
        if (Unchanging) {
            You_feel("momentarily different."); /* same as poly trap */
        } else {
            You_feel("a change coming over you.");
            polyself(0);
        }
        break;
    case PM_DISENCHANTER:
        /* picks an intrinsic at random and removes it; there's
           no feedback if hero already lacks the chosen ability */
        debugpline0("using attrcurse to strip an intrinsic");
        attrcurse();
        break;
    case PM_MIND_FLAYER:
    case PM_MASTER_MIND_FLAYER:
        if (ABASE(A_INT) < ATTRMAX(A_INT)) {
            if (!rn2(2)) {
                pline("Yum!  That was real brain food!");
                (void) adjattrib(A_INT, 1, FALSE);
                break; /* don't give them telepathy, too */
            }
        } else {
            pline("For some reason, that tasted bland.");
        }
    /*FALLTHRU*/
    default:
        check_intrinsics = TRUE;
        break;
    }

    /* possibly convey an intrinsic */
    if (check_intrinsics) {
        struct permonst *ptr = &mons[pm];
        boolean conveys_STR = is_giant(ptr);
        int i, count;

        if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU)
            || pm == PM_VIOLET_FUNGUS) {
            pline("Oh wow!  Great stuff!");
            (void) make_hallucinated((HHallucination & TIMEOUT) + 200L, FALSE,
                                     0L);
        }

        /* Check the monster for all of the intrinsics.  If this
         * monster can give more than one, pick one to try to give
         * from among all it can give.
         *
         * Strength from giants is now treated like an intrinsic
         * rather than being given unconditionally.
         */
        count = 0; /* number of possible intrinsics */
        tmp = 0;   /* which one we will try to give */
        if (conveys_STR) {
            count = 1;
            tmp = -1; /* use -1 as fake prop index for STR */
            debugpline1("\"Intrinsic\" strength, %d", tmp);
        }
        for (i = 1; i <= LAST_PROP; i++) {
            if (!intrinsic_possible(i, ptr))
                continue;
            ++count;
            /* a 1 in count chance of replacing the old choice
               with this one, and a count-1 in count chance
               of keeping the old choice (note that 1 in 1 and
               0 in 1 are what we want for the first candidate) */
            if (!rn2(count)) {
                debugpline2("Intrinsic %d replacing %d", i, tmp);
                tmp = i;
            }
        }
        /* if strength is the only candidate, give it 50% chance */
        if (conveys_STR && count == 1 && !rn2(2))
            tmp = 0;
        /* if something was chosen, give it now (givit() might fail) */
        if (tmp == -1)
            gainstr((struct obj *) 0, 0, TRUE);
        else if (tmp > 0)
            givit(tmp, ptr);
    } /* check_intrinsics */

    if (catch_lycanthropy >= LOW_PM) {
        set_ulycn(catch_lycanthropy);
        retouch_equipment(2);
    }
    return;
}

void
violated_vegetarian()
{
    u.uconduct.unvegetarian++;
    if (Role_if(PM_MONK)) {
        You_feel("guilty.");
        adjalign(-1);
    }
    return;
}

/* common code to check and possibly charge for 1 context.tin.tin,
 * will split() context.tin.tin if necessary */
STATIC_PTR struct obj *
costly_tin(alter_type)
int alter_type; /* COST_xxx */
{
    struct obj *tin = context.tin.tin;

    if (carried(tin) ? tin->unpaid
                     : (costly_spot(tin->ox, tin->oy) && !tin->no_charge)) {
        if (tin->quan > 1L) {
            tin = context.tin.tin = splitobj(tin, 1L);
            context.tin.o_id = tin->o_id;
        }
        costly_alteration(tin, alter_type);
    }
    return tin;
}

int
tin_variety_txt(s, tinvariety)
char *s;
int *tinvariety;
{
    int k, l;

    if (s && tinvariety) {
        *tinvariety = -1;
        for (k = 0; k < TTSZ - 1; ++k) {
            l = (int) strlen(tintxts[k].txt);
            if (!strncmpi(s, tintxts[k].txt, l) && ((int) strlen(s) > l)
                && s[l] == ' ') {
                *tinvariety = k;
                return (l + 1);
            }
        }
    }
    return 0;
}

/*
 * This assumes that buf already contains the word "tin",
 * as is the case with caller xname().
 */
void
tin_details(obj, mnum, buf)
struct obj *obj;
int mnum;
char *buf;
{
    char buf2[BUFSZ];
    int r = tin_variety(obj, TRUE);

    if (obj && buf) {
        if (r == SPINACH_TIN)
            Strcat(buf, " of spinach");
        else if (mnum == NON_PM)
            Strcpy(buf, "empty tin");
        else {
            if ((obj->cknown || iflags.override_ID) && obj->spe < 0) {
                if (r == ROTTEN_TIN || r == HOMEMADE_TIN) {
                    /* put these before the word tin */
                    Sprintf(buf2, "%s %s of ", tintxts[r].txt, buf);
                    Strcpy(buf, buf2);
                } else {
                    Sprintf(eos(buf), " of %s ", tintxts[r].txt);
                }
            } else {
                Strcpy(eos(buf), " of ");
            }
            if (vegetarian(&mons[mnum]))
                Sprintf(eos(buf), "%s", mons[mnum].mname);
            else
                Sprintf(eos(buf), "%s meat", mons[mnum].mname);
        }
    }
}

void
set_tin_variety(obj, forcetype)
struct obj *obj;
int forcetype;
{
    register int r;

    if (forcetype == SPINACH_TIN
        || (forcetype == HEALTHY_TIN
            && (obj->corpsenm == NON_PM /* empty or already spinach */
                || !vegetarian(&mons[obj->corpsenm])))) { /* replace meat */
        obj->corpsenm = NON_PM; /* not based on any monster */
        obj->spe = 1;           /* spinach */
        return;
    } else if (forcetype == HEALTHY_TIN) {
        r = tin_variety(obj, FALSE);
        if (r < 0 || r >= TTSZ)
            r = ROTTEN_TIN; /* shouldn't happen */
        while ((r == ROTTEN_TIN && !obj->cursed) || !tintxts[r].fodder)
            r = rn2(TTSZ - 1);
    } else if (forcetype >= 0 && forcetype < TTSZ - 1) {
        r = forcetype;
    } else {               /* RANDOM_TIN */
        r = rn2(TTSZ - 1); /* take your pick */
        if (r == ROTTEN_TIN && nonrotting_corpse(obj->corpsenm))
            r = HOMEMADE_TIN; /* lizards don't rot */
    }
    obj->spe = -(r + 1); /* offset by 1 to allow index 0 */
}

STATIC_OVL int
tin_variety(obj, disp)
struct obj *obj;
boolean disp; /* we're just displaying so leave things alone */
{
    register int r;

    if (obj->spe == 1) {
        r = SPINACH_TIN;
    } else if (obj->cursed) {
        r = ROTTEN_TIN; /* always rotten if cursed */
    } else if (obj->spe < 0) {
        r = -(obj->spe);
        --r; /* get rid of the offset */
    } else
        r = rn2(TTSZ - 1);

    if (!disp && r == HOMEMADE_TIN && !obj->blessed && !rn2(7))
        r = ROTTEN_TIN; /* some homemade tins go bad */

    if (r == ROTTEN_TIN && nonrotting_corpse(obj->corpsenm))
        r = HOMEMADE_TIN; /* lizards don't rot */
    return r;
}

STATIC_OVL void
consume_tin(mesg)
const char *mesg;
{
    const char *what;
    int which, mnum, r;
    struct obj *tin = context.tin.tin;

    r = tin_variety(tin, FALSE);
    if (tin->otrapped || (tin->cursed && r != HOMEMADE_TIN && !rn2(8))) {
        b_trapped("tin", 0);
        tin = costly_tin(COST_DSTROY);
        goto use_up_tin;
    }

    pline1(mesg); /* "You succeed in opening the tin." */

    if (r != SPINACH_TIN) {
        mnum = tin->corpsenm;
        if (mnum == NON_PM) {
            pline("It turns out to be empty.");
            tin->dknown = tin->known = 1;
            tin = costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        which = 0; /* 0=>plural, 1=>as-is, 2=>"the" prefix */
        if ((mnum == PM_COCKATRICE || mnum == PM_CHICKATRICE)
            && (Stone_resistance || Hallucination)) {
            what = "chicken";
            which = 1; /* suppress pluralization */
        } else if (Hallucination) {
            what = rndmonnam(NULL);
        } else {
            what = mons[mnum].mname;
            if (the_unique_pm(&mons[mnum]))
                which = 2;
            else if (type_is_pname(&mons[mnum]))
                which = 1;
        }
        if (which == 0)
            what = makeplural(what);
        else if (which == 2)
            what = the(what);

        pline("It smells like %s.", what);
        if (yn("Eat it?") == 'n') {
            if (flags.verbose)
                You("discard the open tin.");
            if (!Hallucination)
                tin->dknown = tin->known = 1;
            tin = costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        /* in case stop_occupation() was called on previous meal */
        context.victual.piece = (struct obj *) 0;
        context.victual.o_id = 0;
        context.victual.fullwarn = context.victual.eating =
            context.victual.doreset = FALSE;

        You("consume %s %s.", tintxts[r].txt, mons[mnum].mname);

        eating_conducts(&mons[mnum]);

        tin->dknown = tin->known = 1;
        cprefx(mnum);
        cpostfx(mnum);

        /* charge for one at pre-eating cost */
        tin = costly_tin(COST_OPEN);

        if (tintxts[r].nut < 0) /* rotten */
            make_vomiting((long) rn1(15, 10), FALSE);
        else
            lesshungry(tintxts[r].nut);

        if (tintxts[r].greasy) {
            /* Assume !Glib, because you can't open tins when Glib. */
            make_glib(rn1(11, 5)); /* 5..15 */
            pline("Eating %s food made your %s very slippery.",
                  tintxts[r].txt, fingers_or_gloves(TRUE));
        }

    } else { /* spinach... */
        if (tin->cursed) {
            pline("It contains some decaying%s%s substance.",
                  Blind ? "" : " ", Blind ? "" : hcolor(NH_GREEN));
        } else {
            pline("It contains spinach.");
            tin->dknown = tin->known = 1;
        }

        if (yn("Eat it?") == 'n') {
            if (flags.verbose)
                You("discard the open tin.");
            tin = costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        /*
         * Same order as with non-spinach above:
         * conduct update, side-effects, shop handling, and nutrition.
         */
        u.uconduct.food++; /* don't need vegetarian checks for spinach */
        if (!tin->cursed)
            pline("This makes you feel like %s!",
                  /* "Swee'pea" is a character from the Popeye cartoons */
                  Hallucination ? "Swee'pea"
                  /* "feel like Popeye" unless sustain ability suppresses
                     any attribute change; this slightly oversimplifies
                     things:  we want "Popeye" if no strength increase
                     occurs due to already being at maximum, but we won't
                     get it if at-maximum and fixed-abil both apply */
                  : !Fixed_abil ? "Popeye"
                  /* no gain, feel like another character from Popeye */
                  : (flags.female ? "Olive Oyl" : "Bluto"));
        gainstr(tin, 0, FALSE);

        tin = costly_tin(COST_OPEN);
        lesshungry(tin->blessed ? 600                   /* blessed */
                   : !tin->cursed ? (400 + rnd(200))    /* uncursed */
                     : (200 + rnd(400)));               /* cursed */
    }

 use_up_tin:
    if (carried(tin))
        useup(tin);
    else
        useupf(tin, 1L);
    context.tin.tin = (struct obj *) 0;
    context.tin.o_id = 0;
}

/* called during each move whilst opening a tin */
STATIC_PTR int
opentin(VOID_ARGS)
{
    /* perhaps it was stolen (although that should cause interruption) */
    if (!carried(context.tin.tin)
        && (!obj_here(context.tin.tin, u.ux, u.uy) || !can_reach_floor(TRUE)))
        return 0; /* %% probably we should use tinoid */
    if (context.tin.usedtime++ >= 50) {
        You("give up your attempt to open the tin.");
        return 0;
    }
    if (context.tin.usedtime < context.tin.reqtime)
        return 1; /* still busy */

    consume_tin("You succeed in opening the tin.");
    return 0;
}

/* called when starting to open a tin */
STATIC_OVL void
start_tin(otmp)
struct obj *otmp;
{
    const char *mesg = 0;
    register int tmp;

    if (metallivorous(youmonst.data)) {
        mesg = "You bite right into the metal tin...";
        tmp = 0;
    } else if (cantwield(youmonst.data)) { /* nohands || verysmall */
        You("cannot handle the tin properly to open it.");
        return;
    } else if (otmp->blessed) {
        /* 50/50 chance for immediate access vs 1 turn delay (unless
           wielding blessed tin opener which always yields immediate
           access); 1 turn delay case is non-deterministic:  getting
           interrupted and retrying might yield another 1 turn delay
           or might open immediately on 2nd (or 3rd, 4th, ...) try */
        tmp = (uwep && uwep->blessed && uwep->otyp == TIN_OPENER) ? 0 : rn2(2);
        if (!tmp)
            mesg = "The tin opens like magic!";
        else
            pline_The("tin seems easy to open.");
    } else if (uwep) {
        switch (uwep->otyp) {
        case TIN_OPENER:
            mesg = "You easily open the tin."; /* iff tmp==0 */
            tmp = rn2(uwep->cursed ? 3 : !uwep->blessed ? 2 : 1);
            break;
        case DAGGER:
        case SILVER_DAGGER:
        case ELVEN_DAGGER:
        case ORCISH_DAGGER:
        case ATHAME:
        case KNIFE:
        case STILETTO:
        case CRYSKNIFE:
            tmp = 3;
            break;
        case PICK_AXE:
        case AXE:
            tmp = 6;
            break;
        default:
            goto no_opener;
        }
        pline("Using %s you try to open the tin.", yobjnam(uwep, (char *) 0));
    } else {
 no_opener:
        pline("It is not so easy to open this tin.");
        if (Glib) {
            pline_The("tin slips from your %s.", fingers_or_gloves(FALSE));
            if (otmp->quan > 1L) {
                otmp = splitobj(otmp, 1L);
            }
            if (carried(otmp))
                dropx(otmp);
            else
                stackobj(otmp);
            return;
        }
        tmp = rn1(1 + 500 / ((int) (ACURR(A_DEX) + ACURRSTR)), 10);
    }

    context.tin.tin = otmp;
    context.tin.o_id = otmp->o_id;
    if (!tmp) {
        consume_tin(mesg); /* begin immediately */
    } else {
        context.tin.reqtime = tmp;
        context.tin.usedtime = 0;
        set_occupation(opentin, "opening the tin", 0);
    }
    return;
}

/* called when waking up after fainting */
int
Hear_again(VOID_ARGS)
{
    /* Chance of deafness going away while fainted/sleeping/etc. */
    if (!rn2(2)) {
        make_deaf(0L, FALSE);
        context.botl = TRUE;
    }
    return 0;
}

/* called on the "first bite" of rotten food */
STATIC_OVL int
rottenfood(obj)
struct obj *obj;
{
    pline("Blecch!  Rotten %s!", foodword(obj));
    if (!rn2(4)) {
        if (Hallucination)
            You_feel("rather trippy.");
        else
            You_feel("rather %s.", body_part(LIGHT_HEADED));
        make_confused(HConfusion + d(2, 4), FALSE);
    } else if (!rn2(4) && !Blind) {
        pline("Everything suddenly goes dark.");
        /* hero is not Blind, but Blinded timer might be nonzero if
           blindness is being overridden by the Eyes of the Overworld */
        make_blinded((Blinded & TIMEOUT) + (long) d(2, 10), FALSE);
        if (!Blind)
            Your1(vision_clears);
    } else if (!rn2(3)) {
        const char *what, *where;
        int duration = rnd(10);

        if (!Blind)
            what = "goes", where = "dark";
        else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
            what = "you lose control of", where = "yourself";
        else
            what = "you slap against the",
            where = (u.usteed) ? "saddle" : surface(u.ux, u.uy);
        pline_The("world spins and %s %s.", what, where);
        incr_itimeout(&HDeaf, duration);
        context.botl = TRUE;
        nomul(-duration);
        multi_reason = "unconscious from rotten food";
        nomovemsg = "You are conscious again.";
        afternmv = Hear_again;
        return 1;
    }
    return 0;
}

/* called when a corpse is selected as food */
STATIC_OVL int
eatcorpse(otmp)
struct obj *otmp;
{
    int retcode = 0, tp = 0, mnum = otmp->corpsenm;
    long rotted = 0L;
    boolean stoneable = (flesh_petrifies(&mons[mnum]) && !Stone_resistance
                         && !poly_when_stoned(youmonst.data)),
            slimeable = (mnum == PM_GREEN_SLIME && !Slimed && !Unchanging
                         && !slimeproof(youmonst.data)),
            glob = otmp->globby ? TRUE : FALSE;

    /* KMH, conduct */
    if (!vegan(&mons[mnum]))
        u.uconduct.unvegan++;
    if (!vegetarian(&mons[mnum]))
        violated_vegetarian();

    if (!nonrotting_corpse(mnum)) {
        long age = peek_at_iced_corpse_age(otmp);

        rotted = (monstermoves - age) / (10L + rn2(20));
        if (otmp->cursed)
            rotted += 2L;
        else if (otmp->blessed)
            rotted -= 2L;
    }

    if (mnum != PM_ACID_BLOB && !stoneable && !slimeable && rotted > 5L) {
        boolean cannibal = maybe_cannibal(mnum, FALSE);

        pline("Ulch - that %s was tainted%s!",
              (mons[mnum].mlet == S_FUNGUS) ? "fungoid vegetation"
                  : glob ? "glob"
                      : vegetarian(&mons[mnum]) ? "protoplasm"
                          : "meat",
              cannibal ? ", you cannibal" : "");
        if (Sick_resistance) {
            pline("It doesn't seem at all sickening, though...");
        } else {
            long sick_time;

            sick_time = (long) rn1(10, 10);
            /* make sure new ill doesn't result in improvement */
            if (Sick && (sick_time > Sick))
                sick_time = (Sick > 1L) ? Sick - 1L : 1L;
            make_sick(sick_time, corpse_xname(otmp, "rotted", CXN_NORMAL),
                      TRUE, SICK_VOMITABLE);

            pline("(It must have died too long ago to be safe to eat.)");
        }
        if (carried(otmp))
            useup(otmp);
        else
            useupf(otmp, 1L);
        return 2;
    } else if (acidic(&mons[mnum]) && !Acid_resistance) {
        tp++;
        You("have a very bad case of stomach acid.");   /* not body_part() */
        losehp(rnd(15), !glob ? "acidic corpse" : "acidic glob",
               KILLED_BY_AN); /* acid damage */
    } else if (poisonous(&mons[mnum]) && rn2(5)) {
        tp++;
        pline("Ecch - that must have been poisonous!");
        if (!Poison_resistance) {
            losestr(rnd(4));
            losehp(rnd(15), !glob ? "poisonous corpse" : "poisonous glob",
                   KILLED_BY_AN);
        } else
            You("seem unaffected by the poison.");
    /* now any corpse left too long will make you mildly ill */
    } else if ((rotted > 5L || (rotted > 3L && rn2(5))) && !Sick_resistance) {
        tp++;
        You_feel("%ssick.", (Sick) ? "very " : "");
        losehp(rnd(8), !glob ? "cadaver" : "rotted glob", KILLED_BY_AN);
    }

    /* delay is weight dependent */
    context.victual.reqtime = 3 + ((!glob ? mons[mnum].cwt : otmp->owt) >> 6);

    if (!tp && !nonrotting_corpse(mnum) && (otmp->orotten || !rn2(7))) {
        if (rottenfood(otmp)) {
            otmp->orotten = TRUE;
            (void) touchfood(otmp);
            retcode = 1;
        }

        if (!mons[otmp->corpsenm].cnutrit) {
            /* no nutrition: rots away, no message if you passed out */
            if (!retcode)
                pline_The("corpse rots away completely.");
            if (carried(otmp))
                useup(otmp);
            else
                useupf(otmp, 1L);
            retcode = 2;
        }

        if (!retcode)
            consume_oeaten(otmp, 2); /* oeaten >>= 2 */
    } else if ((mnum == PM_COCKATRICE || mnum == PM_CHICKATRICE)
               && (Stone_resistance || Hallucination)) {
        pline("This tastes just like chicken!");
    } else if (mnum == PM_FLOATING_EYE && u.umonnum == PM_RAVEN) {
        You("peck the eyeball with delight.");
    } else {
        /* yummy is always False for omnivores, palatable always True */
        boolean yummy = (vegan(&mons[mnum])
                            ? (!carnivorous(youmonst.data)
                               && herbivorous(youmonst.data))
                            : (carnivorous(youmonst.data)
                               && !herbivorous(youmonst.data))),
            palatable = ((vegetarian(&mons[mnum])
                          ? herbivorous(youmonst.data)
                          : carnivorous(youmonst.data))
                         && rn2(10)
                         && ((rotted < 1) ? TRUE : !rn2(rotted+1)));
        const char *pmxnam = food_xname(otmp, FALSE);

        if (!strncmpi(pmxnam, "the ", 4))
            pmxnam += 4;
        pline("%s%s %s %s%c",
              type_is_pname(&mons[mnum])
                 ? "" : the_unique_pm(&mons[mnum]) ? "The " : "This ",
              pmxnam,
              Hallucination ? "is" : "tastes",
                  /* tiger reference is to TV ads for "Frosted Flakes",
                     breakfast cereal targeted at kids by "Tony the tiger" */
              Hallucination
                 ? (yummy ? ((u.umonnum == PM_TIGER) ? "gr-r-reat" : "gnarly")
                          : palatable ? "copacetic" : "grody")
                 : (yummy ? "delicious" : palatable ? "okay" : "terrible"),
              (yummy || !palatable) ? '!' : '.');
    }

    return retcode;
}

/* called as you start to eat */
STATIC_OVL void
start_eating(otmp, already_partly_eaten)
struct obj *otmp;
boolean already_partly_eaten;
{
    const char *old_nomovemsg, *save_nomovemsg;

    debugpline2("start_eating: %s (victual = %s)",
                /* note: fmt_ptr() returns a static buffer but supports
                   several such so we don't need to copy the first result
                   before calling it a second time */
                fmt_ptr((genericptr_t) otmp),
                fmt_ptr((genericptr_t) context.victual.piece));
    debugpline1("reqtime = %d", context.victual.reqtime);
    debugpline1("(original reqtime = %d)", objects[otmp->otyp].oc_delay);
    debugpline1("nmod = %d", context.victual.nmod);
    debugpline1("oeaten = %d", otmp->oeaten);
    context.victual.fullwarn = context.victual.doreset = FALSE;
    context.victual.eating = TRUE;

    if (otmp->otyp == CORPSE || otmp->globby) {
        cprefx(context.victual.piece->corpsenm);
        if (!context.victual.piece || !context.victual.eating) {
            /* rider revived, or died and lifesaved */
            return;
        }
    }

    old_nomovemsg = nomovemsg;
    if (bite()) {
        /* survived choking, finish off food that's nearly done;
           need this to handle cockatrice eggs, fortune cookies, etc */
        if (++context.victual.usedtime >= context.victual.reqtime) {
            /* don't want done_eating() to issue nomovemsg if it
               is due to vomit() called by bite() */
            save_nomovemsg = nomovemsg;
            if (!old_nomovemsg)
                nomovemsg = 0;
            done_eating(FALSE);
            if (!old_nomovemsg)
                nomovemsg = save_nomovemsg;
        }
        return;
    }

    if (++context.victual.usedtime >= context.victual.reqtime) {
        /* print "finish eating" message if they just resumed -dlc */
        done_eating((context.victual.reqtime > 1
                     || already_partly_eaten) ? TRUE : FALSE);
        return;
    }

    Sprintf(msgbuf, "eating %s", food_xname(otmp, TRUE));
    set_occupation(eatfood, msgbuf, 0);
}

/*
 * Called on "first bite" of (non-corpse) food, after touchfood() has
 * marked it 'partly eaten'.  Used for non-rotten non-tin non-corpse food.
 * Messages should use present tense since multi-turn food won't be
 * finishing at the time they're issued.
 */
STATIC_OVL void
fprefx(otmp)
struct obj *otmp;
{
    switch (otmp->otyp) {
    case FOOD_RATION: /* nutrition 800 */
        /* 200+800 remains below 1000+1, the satiation threshold */
        if (u.uhunger <= 200)
            pline("%s!", Hallucination ? "Oh wow, like, superior, man"
                                       : "This food really hits the spot");

        /* 700-1+800 remains below 1500, the choking threshold which
           triggers "you're having a hard time getting it down" feedback */
        else if (u.uhunger < 700)
            pline("This satiates your %s!", body_part(STOMACH));
        /* [satiation message may be inaccurate if eating gets interrupted] */
        break;
    case TRIPE_RATION:
        if (carnivorous(youmonst.data) && !humanoid(youmonst.data)) {
            pline("This tripe ration is surprisingly good!");
        } else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC))) {
            pline(Hallucination ? "Tastes great!  Less filling!"
                                : "Mmm, tripe... not bad!");
        } else {
            pline("Yak - dog food!");
            more_experienced(1, 0);
            newexplevel();
            /* not cannibalism, but we use similar criteria
               for deciding whether to be sickened by this meal */
            if (rn2(2) && !CANNIBAL_ALLOWED())
                make_vomiting((long) rn1(context.victual.reqtime, 14), FALSE);
        }
        break;
    case LEMBAS_WAFER:
        if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC))) {
            pline("%s", "!#?&* elf kibble!");
            break;
        } else if (maybe_polyd(is_elf(youmonst.data), Race_if(PM_ELF))) {
            pline("A little goes a long way.");
            break;
        }
        goto give_feedback;
    case MEATBALL:
    case MEAT_STICK:
    case HUGE_CHUNK_OF_MEAT:
    case MEAT_RING:
        goto give_feedback;
    case CLOVE_OF_GARLIC:
        if (is_undead(youmonst.data)) {
            make_vomiting((long) rn1(context.victual.reqtime, 5), FALSE);
            break;
        }
        /*FALLTHRU*/
    default:
        if (otmp->otyp == SLIME_MOLD && !otmp->cursed
            && otmp->spe == context.current_fruit) {
            pline("My, this is a %s %s!",
                  Hallucination ? "primo" : "yummy",
                  singular(otmp, xname));
        } else if (otmp->otyp == APPLE && otmp->cursed && !Sleep_resistance) {
            ; /* skip core joke; feedback deferred til fpostfx() */

#if defined(MAC) || defined(MACOSX)
        /* KMH -- Why should Unix have all the fun?
           We check MACOSX before UNIX to get the Apple-specific apple
           message; the '#if UNIX' code will still kick in for pear. */
        } else if (otmp->otyp == APPLE) {
            pline("Delicious!  Must be a Macintosh!");
#endif

#ifdef UNIX
        } else if (otmp->otyp == APPLE || otmp->otyp == PEAR) {
            if (!Hallucination) {
                pline("Core dumped.");
            } else {
                /* based on an old Usenet joke, a fake a.out manual page */
                int x = rnd(100);

                pline("%s -- core dumped.",
                      (x <= 75)
                         ? "Segmentation fault"
                         : (x <= 99)
                            ? "Bus error"
                            : "Yo' mama");
            }
#endif
        } else if (otmp->otyp == EGG && stale_egg(otmp)) {
            pline("Ugh.  Rotten egg."); /* perhaps others like it */
            /* increasing existing nausea means that it will take longer
               before eventual vomit, but also means that constitution
               will be abused more times before illness completes */
            make_vomiting((Vomiting & TIMEOUT) + (long) d(10, 4), TRUE);
        } else {
 give_feedback:
            pline("This %s is %s", singular(otmp, xname),
                  otmp->cursed
                     ? (Hallucination ? "grody!" : "terrible!")
                     : (otmp->otyp == CRAM_RATION
                        || otmp->otyp == K_RATION
                        || otmp->otyp == C_RATION)
                        ? "bland."
                        : Hallucination ? "gnarly!" : "delicious!");
        }
        break; /* default */
    } /* switch */
}

/* increment a combat intrinsic with limits on its growth */
STATIC_OVL int
bounded_increase(old, inc, typ)
int old, inc, typ;
{
    int absold, absinc, sgnold, sgninc;

    /* don't include any amount coming from worn rings (caller handles
       'protection' differently) */
    if (uright && uright->otyp == typ && typ != RIN_PROTECTION)
        old -= uright->spe;
    if (uleft && uleft->otyp == typ && typ != RIN_PROTECTION)
        old -= uleft->spe;
    absold = abs(old), absinc = abs(inc);
    sgnold = sgn(old), sgninc = sgn(inc);

    if (absinc == 0 || sgnold != sgninc || absold + absinc < 10) {
        ; /* use inc as-is */
    } else if (absold + absinc < 20) {
        absinc = rnd(absinc); /* 1..n */
        if (absold + absinc < 10)
            absinc = 10 - absold;
        inc = sgninc * absinc;
    } else if (absold + absinc < 40) {
        absinc = rn2(absinc) ? 1 : 0;
        if (absold + absinc < 20)
            absinc = rnd(20 - absold);
        inc = sgninc * absinc;
    } else {
        inc = 0; /* no further increase allowed via this method */
    }
    /* put amount from worn rings back */
    if (uright && uright->otyp == typ && typ != RIN_PROTECTION)
        old += uright->spe;
    if (uleft && uleft->otyp == typ && typ != RIN_PROTECTION)
        old += uleft->spe;
    return old + inc;
}

STATIC_OVL void
accessory_has_effect(otmp)
struct obj *otmp;
{
    pline("Magic spreads through your body as you digest the %s.",
          (otmp->oclass == RING_CLASS) ? "ring" : "amulet");
}

STATIC_OVL void
eataccessory(otmp)
struct obj *otmp;
{
    int typ = otmp->otyp;
    long oldprop;

    /* Note: rings are not so common that this is unbalancing. */
    /* (How often do you even _find_ 3 rings of polymorph in a game?) */
    oldprop = u.uprops[objects[typ].oc_oprop].intrinsic;
    if (otmp == uleft || otmp == uright) {
        Ring_gone(otmp);
        if (u.uhp <= 0)
            return; /* died from sink fall */
    }
    otmp->known = otmp->dknown = 1; /* by taste */
    if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5)) {
        switch (otmp->otyp) {
        default:
            if (!objects[typ].oc_oprop)
                break; /* should never happen */

            if (!(u.uprops[objects[typ].oc_oprop].intrinsic & FROMOUTSIDE))
                accessory_has_effect(otmp);

            u.uprops[objects[typ].oc_oprop].intrinsic |= FROMOUTSIDE;

            switch (typ) {
            case RIN_SEE_INVISIBLE:
                set_mimic_blocking();
                see_monsters();
                if (Invis && !oldprop && !ESee_invisible
                    && !perceives(youmonst.data) && !Blind) {
                    newsym(u.ux, u.uy);
                    pline("Suddenly you can see yourself.");
                    makeknown(typ);
                }
                break;
            case RIN_INVISIBILITY:
                if (!oldprop && !EInvis && !BInvis && !See_invisible
                    && !Blind) {
                    newsym(u.ux, u.uy);
                    Your("body takes on a %s transparency...",
                         Hallucination ? "normal" : "strange");
                    makeknown(typ);
                }
                break;
            case RIN_PROTECTION_FROM_SHAPE_CHAN:
                rescham();
                break;
            case RIN_LEVITATION:
                /* undo the `.intrinsic |= FROMOUTSIDE' done above */
                u.uprops[LEVITATION].intrinsic = oldprop;
                if (!Levitation) {
                    float_up();
                    incr_itimeout(&HLevitation, d(10, 20));
                    makeknown(typ);
                }
                break;
            } /* inner switch */
            break; /* default case of outer switch */

        case RIN_ADORNMENT:
            accessory_has_effect(otmp);
            if (adjattrib(A_CHA, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_GAIN_STRENGTH:
            accessory_has_effect(otmp);
            if (adjattrib(A_STR, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_GAIN_CONSTITUTION:
            accessory_has_effect(otmp);
            if (adjattrib(A_CON, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_INCREASE_ACCURACY:
            accessory_has_effect(otmp);
            u.uhitinc = (schar) bounded_increase((int) u.uhitinc, otmp->spe,
                                                 RIN_INCREASE_ACCURACY);
            break;
        case RIN_INCREASE_DAMAGE:
            accessory_has_effect(otmp);
            u.udaminc = (schar) bounded_increase((int) u.udaminc, otmp->spe,
                                                 RIN_INCREASE_DAMAGE);
            break;
        case RIN_PROTECTION:
            accessory_has_effect(otmp);
            HProtection |= FROMOUTSIDE;
            u.ublessed = bounded_increase(u.ublessed, otmp->spe,
                                          RIN_PROTECTION);
            context.botl = 1;
            break;
        case RIN_FREE_ACTION:
            /* Give sleep resistance instead */
            if (!(HSleep_resistance & FROMOUTSIDE))
                accessory_has_effect(otmp);
            if (!Sleep_resistance)
                You_feel("wide awake.");
            HSleep_resistance |= FROMOUTSIDE;
            break;
        case AMULET_OF_CHANGE:
            accessory_has_effect(otmp);
            makeknown(typ);
            change_sex();
            You("are suddenly very %s!",
                flags.female ? "feminine" : "masculine");
            context.botl = 1;
            break;
        case AMULET_OF_UNCHANGING:
            /* un-change: it's a pun */
            if (!Unchanging && Upolyd) {
                accessory_has_effect(otmp);
                makeknown(typ);
                rehumanize();
            }
            break;
        case AMULET_OF_STRANGULATION: /* bad idea! */
            /* no message--this gives no permanent effect */
            choke(otmp);
            break;
        case AMULET_OF_RESTFUL_SLEEP: { /* another bad idea! */
            long newnap = (long) rnd(100), oldnap = (HSleepy & TIMEOUT);

            if (!(HSleepy & FROMOUTSIDE))
                accessory_has_effect(otmp);
            HSleepy |= FROMOUTSIDE;
            /* might also be wearing one; use shorter of two timeouts */
            if (newnap < oldnap || oldnap == 0L)
                HSleepy = (HSleepy & ~TIMEOUT) | newnap;
            break;
        }
        case RIN_SUSTAIN_ABILITY:
        case AMULET_OF_LIFE_SAVING:
        case AMULET_OF_REFLECTION: /* nice try */
            /* can't eat Amulet of Yendor or fakes,
             * and no oc_prop even if you could -3.
             */
            break;
        }
    }
}

/* called after eating non-food */
STATIC_OVL void
eatspecial()
{
    struct obj *otmp = context.victual.piece;

    /* lesshungry wants an occupation to handle choke messages correctly */
    set_occupation(eatfood, "eating non-food", 0);
    lesshungry(context.victual.nmod);
    occupation = 0;
    context.victual.piece = (struct obj *) 0;
    context.victual.o_id = 0;
    context.victual.eating = 0;
    if (otmp->oclass == COIN_CLASS) {
        if (carried(otmp))
            useupall(otmp);
        else
            useupf(otmp, otmp->quan);
        vault_gd_watching(GD_EATGOLD);
        return;
    }
    if (objects[otmp->otyp].oc_material == PAPER) {
#ifdef MAIL
        if (otmp->otyp == SCR_MAIL)
            /* no nutrition */
            pline("This junk mail is less than satisfying.");
        else
#endif
        if (otmp->otyp == SCR_SCARE_MONSTER)
            /* to eat scroll, hero is currently polymorphed into a monster */
            pline("Yuck%c", otmp->blessed ? '!' : '.');
        else if (otmp->oclass == SCROLL_CLASS
                 /* check description after checking for specific scrolls */
                 && !strcmpi(OBJ_DESCR(objects[otmp->otyp]), "YUM YUM"))
            pline("Yum%c", otmp->blessed ? '!' : '.');
        else
            pline("Needs salt...");
    }
    if (otmp->oclass == POTION_CLASS) {
        otmp->quan++; /* dopotion() does a useup() */
        (void) dopotion(otmp);
    } else if (otmp->oclass == RING_CLASS || otmp->oclass == AMULET_CLASS) {
        eataccessory(otmp);
    } else if (otmp->otyp == LEASH && otmp->leashmon) {
        o_unleash(otmp);
    }

    /* KMH -- idea by "Tommy the Terrorist" */
    if (otmp->otyp == TRIDENT && !otmp->cursed) {
        /* sugarless chewing gum which used to be heavily advertised on TV */
        pline(Hallucination ? "Four out of five dentists agree."
                            : "That was pure chewing satisfaction!");
        exercise(A_WIS, TRUE);
    }
    if (otmp->otyp == FLINT && !otmp->cursed) {
        /* chewable vitamin for kids based on "The Flintstones" TV cartoon */
        pline("Yabba-dabba delicious!");
        exercise(A_CON, TRUE);
    }

    if (otmp == uwep && otmp->quan == 1L)
        uwepgone();
    if (otmp == uquiver && otmp->quan == 1L)
        uqwepgone();
    if (otmp == uswapwep && otmp->quan == 1L)
        uswapwepgone();

    if (otmp == uball)
        unpunish();
    if (otmp == uchain)
        unpunish(); /* but no useup() */
    else if (carried(otmp))
        useup(otmp);
    else
        useupf(otmp, 1L);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char *foodwords[] = {
    "meal",    "liquid",  "wax",       "food", "meat",     "paper",
    "cloth",   "leather", "wood",      "bone", "scale",    "metal",
    "metal",   "metal",   "silver",    "gold", "platinum", "mithril",
    "plastic", "glass",   "rich food", "stone"
};

STATIC_OVL const char *
foodword(otmp)
struct obj *otmp;
{
    if (otmp->oclass == FOOD_CLASS)
        return "food";
    if (otmp->oclass == GEM_CLASS && objects[otmp->otyp].oc_material == GLASS
        && otmp->dknown)
        makeknown(otmp->otyp);
    return foodwords[objects[otmp->otyp].oc_material];
}

/* called after consuming (non-corpse) food */
STATIC_OVL void
fpostfx(otmp)
struct obj *otmp;
{
    switch (otmp->otyp) {
    case SPRIG_OF_WOLFSBANE:
        if (u.ulycn >= LOW_PM || is_were(youmonst.data))
            you_unwere(TRUE);
        break;
    case CARROT:
        if (!u.uswallow
            || !attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND))
            make_blinded((long) u.ucreamed, TRUE);
        break;
    case FORTUNE_COOKIE:
        outrumor(bcsign(otmp), BY_COOKIE);
        if (!Blind)
            u.uconduct.literate++;
        break;
    case LUMP_OF_ROYAL_JELLY:
        /* This stuff seems to be VERY healthy! */
        gainstr(otmp, 1, TRUE);
        if (Upolyd) {
            u.mh += otmp->cursed ? -rnd(20) : rnd(20);
            if (u.mh > u.mhmax) {
                if (!rn2(17))
                    u.mhmax++;
                u.mh = u.mhmax;
            } else if (u.mh <= 0) {
                rehumanize();
            }
        } else {
            u.uhp += otmp->cursed ? -rnd(20) : rnd(20);
            if (u.uhp > u.uhpmax) {
                if (!rn2(17))
                    u.uhpmax++;
                u.uhp = u.uhpmax;
            } else if (u.uhp <= 0) {
                killer.format = KILLED_BY_AN;
                Strcpy(killer.name, "rotten lump of royal jelly");
                done(POISONING);
            }
        }
        if (!otmp->cursed)
            heal_legs(0);
        break;
    case EGG:
        if (flesh_petrifies(&mons[otmp->corpsenm])) {
            if (!Stone_resistance
                && !(poly_when_stoned(youmonst.data)
                     && polymon(PM_STONE_GOLEM))) {
                if (!Stoned) {
                    Sprintf(killer.name, "%s egg",
                            mons[otmp->corpsenm].mname);
                    make_stoned(5L, (char *) 0, KILLED_BY_AN, killer.name);
                }
            }
            /* note: no "tastes like chicken" message for eggs */
        }
        break;
    case EUCALYPTUS_LEAF:
        if (Sick && !otmp->cursed)
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
        if (Vomiting && !otmp->cursed)
            make_vomiting(0L, TRUE);
        break;
    case APPLE:
        if (otmp->cursed && !Sleep_resistance) {
            /* Snow White; 'poisoned' applies to [a subset of] weapons,
               not food, so we substitute cursed; fortunately our hero
               won't have to wait for a prince to be rescued/revived */
            if (Race_if(PM_DWARF) && Hallucination)
                verbalize("Heigh-ho, ho-hum, I think I'll skip work today.");
            else if (Deaf || !flags.acoustics)
                You("fall asleep.");
            else
                You_hear("sinister laughter as you fall asleep...");
            fall_asleep(-rn1(11, 20), TRUE);
        }
        break;
    }
    return;
}

#if 0
/* intended for eating a spellbook while polymorphed, but not used;
   "leather" applied to appearance, not composition, and has been
   changed to "leathery" to reflect that */
STATIC_DCL boolean FDECL(leather_cover, (struct obj *));

STATIC_OVL boolean
leather_cover(otmp)
struct obj *otmp;
{
    const char *odesc = OBJ_DESCR(objects[otmp->otyp]);

    if (odesc && (otmp->oclass == SPBOOK_CLASS)) {
        if (!strcmp(odesc, "leather"))
            return TRUE;
    }
    return FALSE;
}
#endif

/*
 * return 0 if the food was not dangerous.
 * return 1 if the food was dangerous and you chose to stop.
 * return 2 if the food was dangerous and you chose to eat it anyway.
 */
STATIC_OVL int
edibility_prompts(otmp)
struct obj *otmp;
{
    /* Blessed food detection grants hero a one-use
     * ability to detect food that is unfit for consumption
     * or dangerous and avoid it.
     */
    char buf[BUFSZ], foodsmell[BUFSZ],
         it_or_they[QBUFSZ], eat_it_anyway[QBUFSZ];
    boolean cadaver = (otmp->otyp == CORPSE || otmp->globby),
            stoneorslime = FALSE;
    int material = objects[otmp->otyp].oc_material, mnum = otmp->corpsenm;
    long rotted = 0L;

    Strcpy(foodsmell, Tobjnam(otmp, "smell"));
    Strcpy(it_or_they, (otmp->quan == 1L) ? "it" : "they");
    Sprintf(eat_it_anyway, "Eat %s anyway?",
            (otmp->quan == 1L) ? "it" : "one");

    if (cadaver || otmp->otyp == EGG || otmp->otyp == TIN) {
        /* These checks must match those in eatcorpse() */
        stoneorslime = (flesh_petrifies(&mons[mnum]) && !Stone_resistance
                        && !poly_when_stoned(youmonst.data));

        if (mnum == PM_GREEN_SLIME || otmp->otyp == GLOB_OF_GREEN_SLIME)
            stoneorslime = (!Unchanging && !slimeproof(youmonst.data));

        if (cadaver && !nonrotting_corpse(mnum)) {
            long age = peek_at_iced_corpse_age(otmp);

            /* worst case rather than random
               in this calculation to force prompt */
            rotted = (monstermoves - age) / (10L + 0 /* was rn2(20) */);
            if (otmp->cursed)
                rotted += 2L;
            else if (otmp->blessed)
                rotted -= 2L;
        }
    }

    /*
     * These problems with food should be checked in
     * order from most detrimental to least detrimental.
     */
    if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && !Sick_resistance) {
        /* Tainted meat */
        Sprintf(buf, "%s like %s could be tainted!  %s", foodsmell, it_or_they,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (stoneorslime) {
        Sprintf(buf, "%s like %s could be something very dangerous!  %s",
                foodsmell, it_or_they, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (otmp->orotten || (cadaver && rotted > 3L)) {
        /* Rotten */
        Sprintf(buf, "%s like %s could be rotten! %s",  foodsmell, it_or_they,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (cadaver && poisonous(&mons[mnum]) && !Poison_resistance) {
        /* poisonous */
        Sprintf(buf, "%s like %s might be poisonous!  %s", foodsmell,
                it_or_they, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (otmp->otyp == APPLE && otmp->cursed && !Sleep_resistance) {
        /* causes sleep, for long enough to be dangerous */
        Sprintf(buf, "%s like %s might have been poisoned.  %s", foodsmell,
                it_or_they, eat_it_anyway);
        return (yn_function(buf, ynchars, 'n') == 'n') ? 1 : 2;
    }
    if (cadaver && !vegetarian(&mons[mnum]) && !u.uconduct.unvegetarian
        && Role_if(PM_MONK)) {
        Sprintf(buf, "%s unhealthy.  %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (cadaver && acidic(&mons[mnum]) && !Acid_resistance) {
        Sprintf(buf, "%s rather acidic.  %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (Upolyd && u.umonnum == PM_RUST_MONSTER && is_metallic(otmp)
        && otmp->oerodeproof) {
        Sprintf(buf, "%s disgusting to you right now.  %s", foodsmell,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }

    /*
     * Breaks conduct, but otherwise safe.
     */
    if (!u.uconduct.unvegan
        && ((material == LEATHER || material == BONE
             || material == DRAGON_HIDE || material == WAX)
            || (cadaver && !vegan(&mons[mnum])))) {
        Sprintf(buf, "%s foul and unfamiliar to you.  %s", foodsmell,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (!u.uconduct.unvegetarian
        && ((material == LEATHER || material == BONE
             || material == DRAGON_HIDE)
            || (cadaver && !vegetarian(&mons[mnum])))) {
        Sprintf(buf, "%s unfamiliar to you.  %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }

    if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && Sick_resistance) {
        /* Tainted meat with Sick_resistance */
        Sprintf(buf, "%s like %s could be tainted!  %s",
                foodsmell, it_or_they, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    return 0;
}

/* 'e' command */
int
doeat()
{
    struct obj *otmp;
    int basenutrit; /* nutrition of full item */
    boolean dont_start = FALSE, nodelicious = FALSE,
            already_partly_eaten;

    if (Strangled) {
        pline("If you can't breathe air, how can you consume solids?");
        return 0;
    }
    if (!(otmp = floorfood("eat", 0)))
        return 0;
    if (check_capacity((char *) 0))
        return 0;

    if (u.uedibility) {
        int res = edibility_prompts(otmp);

        if (res) {
            Your(
               "%s stops tingling and your sense of smell returns to normal.",
                 body_part(NOSE));
            u.uedibility = 0;
            if (res == 1)
                return 0;
        }
    }

    /* We have to make non-foods take 1 move to eat, unless we want to
     * do ridiculous amounts of coding to deal with partly eaten plate
     * mails, players who polymorph back to human in the middle of their
     * metallic meal, etc....
     */
    if (!is_edible(otmp)) {
        You("cannot eat that!");
        return 0;
    } else if ((otmp->owornmask & (W_ARMOR | W_TOOL | W_AMUL | W_SADDLE))
               != 0) {
        /* let them eat rings */
        You_cant("eat %s you're wearing.", something);
        return 0;
    } else if (!(carried(otmp) ? retouch_object(&otmp, FALSE)
                               : touch_artifact(otmp, &youmonst))) {
        return 1; /* got blasted so use a turn */
    }
    if (is_metallic(otmp) && u.umonnum == PM_RUST_MONSTER
        && otmp->oerodeproof) {
        otmp->rknown = TRUE;
        if (otmp->quan > 1L) {
            if (!carried(otmp))
                (void) splitobj(otmp, otmp->quan - 1L);
            else
                otmp = splitobj(otmp, 1L);
        }
        pline("Ulch - that %s was rustproofed!", xname(otmp));
        /* The regurgitated object's rustproofing is gone now */
        otmp->oerodeproof = 0;
        make_stunned((HStun & TIMEOUT) + (long) rn2(10), TRUE);
        /*
         * We don't expect rust monsters to be wielding welded weapons
         * or wearing cursed rings which were rustproofed, but guard
         * against the possibility just in case.
         */
        if (welded(otmp) || (otmp->cursed && (otmp->owornmask & W_RING))) {
            set_bknown(otmp, 1); /* for ring; welded() does this for weapon */
            You("spit out %s.", the(xname(otmp)));
        } else {
            You("spit %s out onto the %s.", the(xname(otmp)),
                surface(u.ux, u.uy));
            if (carried(otmp)) {
                /* no need to check for leash in use; it's not metallic */
                if (otmp->owornmask)
                    remove_worn_item(otmp, FALSE);
                freeinv(otmp);
                dropy(otmp);
            }
            stackobj(otmp);
        }
        return 1;
    }
    /* KMH -- Slow digestion is... indigestible */
    if (otmp->otyp == RIN_SLOW_DIGESTION) {
        pline("This ring is indigestible!");
        (void) rottenfood(otmp);
        if (otmp->dknown && !objects[otmp->otyp].oc_name_known
            && !objects[otmp->otyp].oc_uname)
            docall(otmp);
        return 1;
    }
    if (otmp->oclass != FOOD_CLASS) {
        int material;

        context.victual.reqtime = 1;
        context.victual.piece = otmp;
        context.victual.o_id = otmp->o_id;
        /* Don't split it, we don't need to if it's 1 move */
        context.victual.usedtime = 0;
        context.victual.canchoke = (u.uhs == SATIATED);
        /* Note: gold weighs 1 pt. for each 1000 pieces (see
           pickup.c) so gold and non-gold is consistent. */
        if (otmp->oclass == COIN_CLASS)
            basenutrit = ((otmp->quan > 200000L)
                             ? 2000
                             : (int) (otmp->quan / 100L));
        else if (otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS)
            basenutrit = weight(otmp);
        /* oc_nutrition is usually weight anyway */
        else
            basenutrit = objects[otmp->otyp].oc_nutrition;
#ifdef MAIL
        if (otmp->otyp == SCR_MAIL) {
            basenutrit = 0;
            nodelicious = TRUE;
        }
#endif
        context.victual.nmod = basenutrit;
        context.victual.eating = TRUE; /* needed for lesshungry() */

        material = objects[otmp->otyp].oc_material;
        if (material == LEATHER || material == BONE
            || material == DRAGON_HIDE) {
            u.uconduct.unvegan++;
            violated_vegetarian();
        } else if (material == WAX)
            u.uconduct.unvegan++;
        u.uconduct.food++;

        if (otmp->cursed) {
            (void) rottenfood(otmp);
            nodelicious = TRUE;
        } else if (objects[otmp->otyp].oc_material == PAPER)
            nodelicious = TRUE;

        if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) {
            pline("Ecch - that must have been poisonous!");
            if (!Poison_resistance) {
                losestr(rnd(4));
                losehp(rnd(15), xname(otmp), KILLED_BY_AN);
            } else
                You("seem unaffected by the poison.");
        } else if (!nodelicious) {
            pline("%s%s is delicious!",
                  (obj_is_pname(otmp)
                   && otmp->oartifact < ART_ORB_OF_DETECTION)
                      ? ""
                      : "This ",
                  (otmp->oclass == COIN_CLASS)
                      ? foodword(otmp)
                      : singular(otmp, xname));
        }
        eatspecial();
        return 1;
    }

    if (otmp == context.victual.piece) {
        /* If they weren't able to choke, they don't suddenly become able to
         * choke just because they were interrupted.  On the other hand, if
         * they were able to choke before, if they lost food it's possible
         * they shouldn't be able to choke now.
         */
        if (u.uhs != SATIATED)
            context.victual.canchoke = FALSE;
        context.victual.o_id = 0;
        context.victual.piece = touchfood(otmp);
        if (context.victual.piece)
            context.victual.o_id = context.victual.piece->o_id;
        You("resume %syour meal.",
            (context.victual.usedtime + 1 >= context.victual.reqtime)
            ? "the last bite of " : "");
        start_eating(context.victual.piece, FALSE);
        return 1;
    }

    /* nothing in progress - so try to find something. */
    /* tins are a special case */
    /* tins must also check conduct separately in case they're discarded */
    if (otmp->otyp == TIN) {
        start_tin(otmp);
        return 1;
    }

    /* KMH, conduct */
    u.uconduct.food++;

    already_partly_eaten = otmp->oeaten ? TRUE : FALSE;
    context.victual.o_id = 0;
    context.victual.piece = otmp = touchfood(otmp);
    if (context.victual.piece)
        context.victual.o_id = context.victual.piece->o_id;
    context.victual.usedtime = 0;

    /* Now we need to calculate delay and nutritional info.
     * The base nutrition calculated here and in eatcorpse() accounts
     * for normal vs. rotten food.  The reqtime and nutrit values are
     * then adjusted in accordance with the amount of food left.
     */
    if (otmp->otyp == CORPSE || otmp->globby) {
        int tmp = eatcorpse(otmp);

        if (tmp == 2) {
            /* used up */
            context.victual.piece = (struct obj *) 0;
            context.victual.o_id = 0;
            return 1;
        } else if (tmp)
            dont_start = TRUE;
        /* if not used up, eatcorpse sets up reqtime and may modify oeaten */
    } else {
        /* No checks for WAX, LEATHER, BONE, DRAGON_HIDE.  These are
         * all handled in the != FOOD_CLASS case, above.
         */
        switch (objects[otmp->otyp].oc_material) {
        case FLESH:
            u.uconduct.unvegan++;
            if (otmp->otyp != EGG) {
                violated_vegetarian();
            }
            break;

        default:
            if (otmp->otyp == PANCAKE || otmp->otyp == FORTUNE_COOKIE /*eggs*/
                || otmp->otyp == CREAM_PIE || otmp->otyp == CANDY_BAR /*milk*/
                || otmp->otyp == LUMP_OF_ROYAL_JELLY)
                u.uconduct.unvegan++;
            break;
        }

        context.victual.reqtime = objects[otmp->otyp].oc_delay;
        if (otmp->otyp != FORTUNE_COOKIE
            && (otmp->cursed || (!nonrotting_food(otmp->otyp)
                                 && (monstermoves - otmp->age)
                                        > (otmp->blessed ? 50L : 30L)
                                 && (otmp->orotten || !rn2(7))))) {
            if (rottenfood(otmp)) {
                otmp->orotten = TRUE;
                dont_start = TRUE;
            }
            consume_oeaten(otmp, 1); /* oeaten >>= 1 */
        } else if (!already_partly_eaten) {
            fprefx(otmp);
        } else {
            You("%s %s.",
                (context.victual.reqtime == 1) ? "eat" : "begin eating",
                doname(otmp));
        }
    }

    /* re-calc the nutrition */
    basenutrit = (int) obj_nutrition(otmp);

    debugpline3(
     "before rounddiv: victual.reqtime == %d, oeaten == %d, basenutrit == %d",
                context.victual.reqtime, otmp->oeaten, basenutrit);

    context.victual.reqtime = (basenutrit == 0) ? 0
        : rounddiv(context.victual.reqtime * (long) otmp->oeaten, basenutrit);

    debugpline1("after rounddiv: victual.reqtime == %d",
                context.victual.reqtime);
    /*
     * calculate the modulo value (nutrit. units per round eating)
     * note: this isn't exact - you actually lose a little nutrition due
     *       to this method.
     * TODO: add in a "remainder" value to be given at the end of the meal.
     */
    if (context.victual.reqtime == 0 || otmp->oeaten == 0)
        /* possible if most has been eaten before */
        context.victual.nmod = 0;
    else if ((int) otmp->oeaten >= context.victual.reqtime)
        context.victual.nmod = -((int) otmp->oeaten
                                 / context.victual.reqtime);
    else
        context.victual.nmod = context.victual.reqtime % otmp->oeaten;
    context.victual.canchoke = (u.uhs == SATIATED);

    if (!dont_start)
        start_eating(otmp, already_partly_eaten);
    return 1;
}

int
use_tin_opener(obj)
struct obj *obj;
{
    struct obj *otmp;
    int res = 0;

    if (!carrying(TIN)) {
        You("have no tin to open.");
        return 0;
    }

    if (obj != uwep) {
        if (obj->cursed && obj->bknown) {
            char qbuf[QBUFSZ];

            if (ynq(safe_qbuf(qbuf, "Really wield ", "?",
                              obj, doname, thesimpleoname, "that")) != 'y')
                return 0;
        }
        if (!wield_tool(obj, "use"))
            return 0;
        res = 1;
    }

    otmp = getobj(comestibles, "open");
    if (!otmp)
        return res;

    start_tin(otmp);
    return 1;
}

/* Take a single bite from a piece of food, checking for choking and
 * modifying usedtime.  Returns 1 if they choked and survived, 0 otherwise.
 */
STATIC_OVL int
bite()
{
    if (context.victual.canchoke && u.uhunger >= 2000) {
        choke(context.victual.piece);
        return 1;
    }
    if (context.victual.doreset) {
        do_reset_eat();
        return 0;
    }
    force_save_hs = TRUE;
    if (context.victual.nmod < 0) {
        lesshungry(-context.victual.nmod);
        consume_oeaten(context.victual.piece,
                       context.victual.nmod); /* -= -nmod */
    } else if (context.victual.nmod > 0
               && (context.victual.usedtime % context.victual.nmod)) {
        lesshungry(1);
        consume_oeaten(context.victual.piece, -1); /* -= 1 */
    }
    force_save_hs = FALSE;
    recalc_wt();
    return 0;
}

/* as time goes by - called by moveloop(every move) & domove(melee attack) */
void
gethungry()
{
    if (u.uinvulnerable)
        return; /* you don't feel hungrier */

    /* being polymorphed into a creature which doesn't eat prevents
       this first uhunger decrement, but to stay in such form the hero
       will need to wear an Amulet of Unchanging so still burn a small
       amount of nutrition in the 'moves % 20' ring/amulet check below */
    if ((!Unaware || !rn2(10)) /* slow metabolic rate while asleep */
        && (carnivorous(youmonst.data)
            || herbivorous(youmonst.data)
            || metallivorous(youmonst.data))
        && !Slow_digestion)
        u.uhunger--; /* ordinary food consumption */

    if (moves % 2) { /* odd turns */
        /* Regeneration uses up food, unless due to an artifact */
        if ((HRegeneration & ~FROMFORM)
            || (ERegeneration & ~(W_ARTI | W_WEP)))
            u.uhunger--;
        if (near_capacity() > SLT_ENCUMBER)
            u.uhunger--;
    } else { /* even turns */
        if (Hunger)
            u.uhunger--;
        /* Conflict uses up food too */
        if (HConflict || (EConflict & (~W_ARTI)))
            u.uhunger--;
        /* +0 charged rings don't do anything, so don't affect hunger.
           Slow digestion cancels move hunger but still causes ring hunger. */
        switch ((int) (moves % 20)) { /* note: use even cases only */
        case 4:
            if (uleft && (uleft->spe || !objects[uleft->otyp].oc_charged))
                u.uhunger--;
            break;
        case 8:
            if (uamul)
                u.uhunger--;
            break;
        case 12:
            if (uright && (uright->spe || !objects[uright->otyp].oc_charged))
                u.uhunger--;
            break;
        case 16:
            if (u.uhave.amulet)
                u.uhunger--;
            break;
        default:
            break;
        }
    }
    newuhs(TRUE);
}

/* called after vomiting and after performing feats of magic */
void
morehungry(num)
int num;
{
    u.uhunger -= num;
    newuhs(TRUE);
}

/* called after eating (and after drinking fruit juice) */
void
lesshungry(num)
int num;
{
    /* See comments in newuhs() for discussion on force_save_hs */
    boolean iseating = (occupation == eatfood) || force_save_hs;

    debugpline1("lesshungry(%d)", num);
    u.uhunger += num;
    if (u.uhunger >= 2000) {
        if (!iseating || context.victual.canchoke) {
            if (iseating) {
                choke(context.victual.piece);
                reset_eat();
            } else
                choke(occupation == opentin ? context.tin.tin
                                            : (struct obj *) 0);
            /* no reset_eat() */
        }
    } else {
        /* Have lesshungry() report when you're nearly full so all eating
         * warns when you're about to choke.
         */
        if (u.uhunger >= 1500
            && (!context.victual.eating
                || (context.victual.eating && !context.victual.fullwarn))) {
            pline("You're having a hard time getting all of it down.");
            nomovemsg = "You're finally finished.";
            if (!context.victual.eating) {
                multi = -2;
            } else {
                context.victual.fullwarn = TRUE;
                if (context.victual.canchoke && context.victual.reqtime > 1) {
                    /* a one-gulp food will not survive a stop */
                    if (!paranoid_query(ParanoidEating, "Continue eating?")) {
                        reset_eat();
                        nomovemsg = (char *) 0;
                    }
                }
            }
        }
    }
    newuhs(FALSE);
}

STATIC_PTR
int
unfaint(VOID_ARGS)
{
    (void) Hear_again();
    if (u.uhs > FAINTING)
        u.uhs = FAINTING;
    stop_occupation();
    context.botl = 1;
    return 0;
}

boolean
is_fainted()
{
    return (boolean) (u.uhs == FAINTED);
}

/* call when a faint must be prematurely terminated */
void
reset_faint()
{
    if (afternmv == unfaint)
        unmul("You revive.");
}

/* compute and comment on your (new?) hunger status */
void
newuhs(incr)
boolean incr;
{
    unsigned newhs;
    static unsigned save_hs;
    static boolean saved_hs = FALSE;
    int h = u.uhunger;

    newhs = (h > 1000)
                ? SATIATED
                : (h > 150) ? NOT_HUNGRY
                            : (h > 50) ? HUNGRY : (h > 0) ? WEAK : FAINTING;

    /* While you're eating, you may pass from WEAK to HUNGRY to NOT_HUNGRY.
     * This should not produce the message "you only feel hungry now";
     * that message should only appear if HUNGRY is an endpoint.  Therefore
     * we check to see if we're in the middle of eating.  If so, we save
     * the first hunger status, and at the end of eating we decide what
     * message to print based on the _entire_ meal, not on each little bit.
     */
    /* It is normally possible to check if you are in the middle of a meal
     * by checking occupation == eatfood, but there is one special case:
     * start_eating() can call bite() for your first bite before it
     * sets the occupation.
     * Anyone who wants to get that case to work _without_ an ugly static
     * force_save_hs variable, feel free.
     */
    /* Note: If you become a certain hunger status in the middle of the
     * meal, and still have that same status at the end of the meal,
     * this will incorrectly print the associated message at the end of
     * the meal instead of the middle.  Such a case is currently
     * impossible, but could become possible if a message for SATIATED
     * were added or if HUNGRY and WEAK were separated by a big enough
     * gap to fit two bites.
     */
    if (occupation == eatfood || force_save_hs) {
        if (!saved_hs) {
            save_hs = u.uhs;
            saved_hs = TRUE;
        }
        u.uhs = newhs;
        return;
    } else {
        if (saved_hs) {
            u.uhs = save_hs;
            saved_hs = FALSE;
        }
    }

    if (newhs == FAINTING) {
        /* u,uhunger is likely to be negative at this point */
        int uhunger_div_by_10 = sgn(u.uhunger) * ((abs(u.uhunger) + 5) / 10);

        if (is_fainted())
            newhs = FAINTED;
        if (u.uhs <= WEAK || rn2(20 - uhunger_div_by_10) >= 19) {
            if (!is_fainted() && multi >= 0 /* %% */) {
                int duration = 10 - uhunger_div_by_10;

                /* stop what you're doing, then faint */
                stop_occupation();
                You("faint from lack of food.");
                incr_itimeout(&HDeaf, duration);
                context.botl = TRUE;
                nomul(-duration);
                multi_reason = "fainted from lack of food";
                nomovemsg = "You regain consciousness.";
                afternmv = unfaint;
                newhs = FAINTED;
                if (!Levitation)
                    selftouch("Falling, you");
            }

        /* this used to be -(200 + 20 * Con) but that was when being asleep
           suppressed per-turn uhunger decrement but being fainted didn't;
           now uhunger becomes more negative at a slower rate */
        } else if (u.uhunger < -(100 + 10 * (int) ACURR(A_CON))) {
            u.uhs = STARVED;
            context.botl = 1;
            bot();
            You("die from starvation.");
            killer.format = KILLED_BY;
            Strcpy(killer.name, "starvation");
            done(STARVING);
            /* if we return, we lifesaved, and that calls newuhs */
            return;
        }
    }

    if (newhs != u.uhs) {
        if (newhs >= WEAK && u.uhs < WEAK) {
            /* this used to be losestr(1) which had the potential to
               be fatal (still handled below) by reducing HP if it
               tried to take base strength below minimum of 3 */
            ATEMP(A_STR) = -1; /* temporary loss overrides Fixed_abil */
            /* defer context.botl status update until after hunger message */
        } else if (newhs < WEAK && u.uhs >= WEAK) {
            /* this used to be losestr(-1) which could be abused by
               becoming weak while wearing ring of sustain ability,
               removing ring, eating to 'restore' strength which boosted
               strength by a point each time the cycle was performed;
               substituting "while polymorphed" for sustain ability and
               "rehumanize" for ring removal might have done that too */
            ATEMP(A_STR) = 0; /* repair of loss also overrides Fixed_abil */
            /* defer context.botl status update until after hunger message */
        }

        switch (newhs) {
        case HUNGRY:
            if (Hallucination) {
                You((!incr) ? "now have a lesser case of the munchies."
                            : "are getting the munchies.");
            } else
                You((!incr) ? "only feel hungry now."
                            : (u.uhunger < 145)
                                  ? "feel hungry."
                                  : "are beginning to feel hungry.");
            if (incr && occupation
                && (occupation != eatfood && occupation != opentin))
                stop_occupation();
            context.travel = context.travel1 = context.mv = context.run = 0;
            break;
        case WEAK:
            if (Hallucination)
                pline((!incr) ? "You still have the munchies."
              : "The munchies are interfering with your motor capabilities.");
            else if (incr && (Role_if(PM_WIZARD) || Race_if(PM_ELF)
                              || Role_if(PM_VALKYRIE)))
                pline("%s needs food, badly!",
                      (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE))
                          ? urole.name.m
                          : "Elf");
            else
                You((!incr)
                        ? "feel weak now."
                        : (u.uhunger < 45) ? "feel weak."
                                           : "are beginning to feel weak.");
            if (incr && occupation
                && (occupation != eatfood && occupation != opentin))
                stop_occupation();
            context.travel = context.travel1 = context.mv = context.run = 0;
            break;
        }
        u.uhs = newhs;
        context.botl = 1;
        bot();
        if ((Upolyd ? u.mh : u.uhp) < 1) {
            You("die from hunger and exhaustion.");
            killer.format = KILLED_BY;
            Strcpy(killer.name, "exhaustion");
            done(STARVING);
            return;
        }
    }
}

/* Returns an object representing food.
 * Object may be either on floor or in inventory.
 */
struct obj *
floorfood(verb, corpsecheck)
const char *verb;
int corpsecheck; /* 0, no check, 1, corpses, 2, tinnable corpses */
{
    register struct obj *otmp;
    char qbuf[QBUFSZ];
    char c;
    boolean feeding = !strcmp(verb, "eat"),    /* corpsecheck==0 */
        offering = !strcmp(verb, "sacrifice"); /* corpsecheck==1 */

    /* if we can't touch floor objects then use invent food only */
    if (iflags.menu_requested /* command was preceded by 'm' prefix */
        || !can_reach_floor(TRUE) || (feeding && u.usteed)
        || (is_pool_or_lava(u.ux, u.uy)
            && (Wwalking || is_clinger(youmonst.data)
                || (Flying && !Breathless))))
        goto skipfloor;

    if (feeding && metallivorous(youmonst.data)) {
        struct obj *gold;
        struct trap *ttmp = t_at(u.ux, u.uy);

        if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
            boolean u_in_beartrap = (u.utrap && u.utraptype == TT_BEARTRAP);

            /* If not already stuck in the trap, perhaps there should
               be a chance to becoming trapped?  Probably not, because
               then the trap would just get eaten on the _next_ turn... */
            Sprintf(qbuf, "There is a bear trap here (%s); eat it?",
                    u_in_beartrap ? "holding you" : "armed");
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
                deltrap(ttmp);
                if (u_in_beartrap)
                    reset_utrap(TRUE);
                return mksobj(BEARTRAP, TRUE, FALSE);
            } else if (c == 'q') {
                return (struct obj *) 0;
            }
        }

        if (youmonst.data != &mons[PM_RUST_MONSTER]
            && (gold = g_at(u.ux, u.uy)) != 0) {
            if (gold->quan == 1L)
                Sprintf(qbuf, "There is 1 gold piece here; eat it?");
            else
                Sprintf(qbuf, "There are %ld gold pieces here; eat them?",
                        gold->quan);
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
                return gold;
            } else if (c == 'q') {
                return (struct obj *) 0;
            }
        }
    }

    /* Is there some food (probably a heavy corpse) here on the ground? */
    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
        if (corpsecheck
                ? (otmp->otyp == CORPSE
                   && (corpsecheck == 1 || tinnable(otmp)))
                : feeding ? (otmp->oclass != COIN_CLASS && is_edible(otmp))
                          : otmp->oclass == FOOD_CLASS) {
            char qsfx[QBUFSZ];
            boolean one = (otmp->quan == 1L);

            /* if blind and without gloves, attempting to eat (or tin or
               offer) a cockatrice corpse is fatal before asking whether
               or not to use it; otherwise, 'm<dir>' followed by 'e' could
               be used to locate cockatrice corpses without touching them */
            if (otmp->otyp == CORPSE && will_feel_cockatrice(otmp, FALSE)) {
                feel_cockatrice(otmp, FALSE);
                /* if life-saved (or poly'd into stone golem), terminate
                   attempt to eat off floor */
                return (struct obj *) 0;
            }
            /* "There is <an object> here; <verb> it?" or
               "There are <N objects> here; <verb> one?" */
            Sprintf(qbuf, "There %s ", otense(otmp, "are"));
            Sprintf(qsfx, " here; %s %s?", verb, one ? "it" : "one");
            (void) safe_qbuf(qbuf, qbuf, qsfx, otmp, doname, ansimpleoname,
                             one ? something : (const char *) "things");
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y')
                return  otmp;
            else if (c == 'q')
                return (struct obj *) 0;
        }
    }

 skipfloor:
    /* We cannot use ALL_CLASSES since that causes getobj() to skip its
     * "ugly checks" and we need to check for inedible items.
     */
    otmp = getobj(feeding ? allobj : offering ? offerfodder : comestibles,
                  verb);
    if (corpsecheck && otmp && !(offering && otmp->oclass == AMULET_CLASS))
        if (otmp->otyp != CORPSE || (corpsecheck == 2 && !tinnable(otmp))) {
            You_cant("%s that!", verb);
            return (struct obj *) 0;
        }
    return otmp;
}

/* Side effects of vomiting */
/* added nomul (MRS) - it makes sense, you're too busy being sick! */
void
vomit() /* A good idea from David Neves */
{
    if (cantvomit(youmonst.data)) {
        /* doesn't cure food poisoning; message assumes that we aren't
           dealing with some esoteric body_part() */
        Your("jaw gapes convulsively.");
    } else {
        if (Sick && (u.usick_type & SICK_VOMITABLE) != 0)
            make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
        /* if not enough in stomach to actually vomit then dry heave;
           vomiting_dialog() gives a vomit message when its countdown
           reaches 0, but only if u.uhs < FAINTING (and !cantvomit()) */
        if (u.uhs >= FAINTING)
            Your("%s heaves convulsively!", body_part(STOMACH));
    }

    /* nomul()/You_can_move_again used to be unconditional, which was
       viable while eating but not for Vomiting countdown where hero might
       be immobilized for some other reason at the time vomit() is called */
    if (multi >= -2) {
        nomul(-2);
        multi_reason = "vomiting";
        nomovemsg = You_can_move_again;
    }
}

int
eaten_stat(base, obj)
int base;
struct obj *obj;
{
    long uneaten_amt, full_amount;

    /* get full_amount first; obj_nutrition() might modify obj->oeaten */
    full_amount = (long) obj_nutrition(obj);
    uneaten_amt = (long) obj->oeaten;
    if (uneaten_amt > full_amount) {
        impossible(
          "partly eaten food (%ld) more nutritious than untouched food (%ld)",
                   uneaten_amt, full_amount);
        uneaten_amt = full_amount;
    }

    base = (int) (full_amount ? (long) base * uneaten_amt / full_amount : 0L);
    return (base < 1) ? 1 : base;
}

/* reduce obj's oeaten field, making sure it never hits or passes 0 */
void
consume_oeaten(obj, amt)
struct obj *obj;
int amt;
{
    /*
     * This is a hack to try to squelch several long standing mystery
     * food bugs.  A better solution would be to rewrite the entire
     * victual handling mechanism from scratch using a less complex
     * model.  Alternatively, this routine could call done_eating()
     * or food_disappears() but its callers would need revisions to
     * cope with context.victual.piece unexpectedly going away.
     *
     * Multi-turn eating operates by setting the food's oeaten field
     * to its full nutritional value and then running a counter which
     * independently keeps track of whether there is any food left.
     * The oeaten field can reach exactly zero on the last turn, and
     * the object isn't removed from inventory until the next turn
     * when the "you finish eating" message gets delivered, so the
     * food would be restored to the status of untouched during that
     * interval.  This resulted in unexpected encumbrance messages
     * at the end of a meal (if near enough to a threshold) and would
     * yield full food if there was an interruption on the critical
     * turn.  Also, there have been reports over the years of food
     * becoming massively heavy or producing unlimited satiation;
     * this would occur if reducing oeaten via subtraction attempted
     * to drop it below 0 since its unsigned type would produce a
     * huge positive value instead.  So far, no one has figured out
     * _why_ that inappropriate subtraction might sometimes happen.
     */

    if (amt > 0) {
        /* bit shift to divide the remaining amount of food */
        obj->oeaten >>= amt;
    } else {
        /* simple decrement; value is negative so we actually add it */
        if ((int) obj->oeaten > -amt)
            obj->oeaten += amt;
        else
            obj->oeaten = 0;
    }

    if (obj->oeaten == 0) {
        if (obj == context.victual.piece) /* always true unless wishing... */
            context.victual.reqtime =
                context.victual.usedtime; /* no bites left */
        obj->oeaten = 1; /* smallest possible positive value */
    }
}

/* called when eatfood occupation has been interrupted,
   or in the case of theft, is about to be interrupted */
boolean
maybe_finished_meal(stopping)
boolean stopping;
{
    /* in case consume_oeaten() has decided that the food is all gone */
    if (occupation == eatfood
        && context.victual.usedtime >= context.victual.reqtime) {
        if (stopping)
            occupation = 0; /* for do_reset_eat */
        (void) eatfood();   /* calls done_eating() to use up
                               context.victual.piece */
        return TRUE;
    }
    return FALSE;
}

/* Tin of <something> to the rescue?  Decide whether current occupation
   is an attempt to eat a tin of something capable of saving hero's life.
   We don't care about consumption of non-tinned food here because special
   effects there take place on first bite rather than at end of occupation.
   [Popeye the Sailor gets out of trouble by eating tins of spinach. :-] */
boolean
Popeye(threat)
int threat;
{
    struct obj *otin;
    int mndx;

    if (occupation != opentin)
        return FALSE;
    otin = context.tin.tin;
    /* make sure hero still has access to tin */
    if (!carried(otin)
        && (!obj_here(otin, u.ux, u.uy) || !can_reach_floor(TRUE)))
        return FALSE;
    /* unknown tin is assumed to be helpful */
    if (!otin->known)
        return TRUE;
    /* known tin is helpful if it will stop life-threatening problem */
    mndx = otin->corpsenm;
    switch (threat) {
    /* note: not used; hunger code bypasses stop_occupation() when eating */
    case HUNGER:
        return (boolean) (mndx != NON_PM || otin->spe == 1);
    /* flesh from lizards and acidic critters stops petrification */
    case STONED:
        return (boolean) (mndx >= LOW_PM
                          && (mndx == PM_LIZARD || acidic(&mons[mndx])));
    /* no tins can cure these (yet?) */
    case SLIMED:
    case SICK:
    case VOMITING:
        break;
    default:
        break;
    }
    return FALSE;
}

/*eat.c*/

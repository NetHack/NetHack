/* NetHack 3.6	attrib.c	$NHDT-Date: 1575245050 2019/12/02 00:04:10 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.66 $ */
/*      Copyright 1988, 1989, 1990, 1992, M. Stephenson           */
/* NetHack may be freely redistributed.  See license for details. */

/*  attribute modification routines. */

#include "hack.h"
#include <ctype.h>

/* part of the output on gain or loss of attribute */
static const char
    *const plusattr[] = { "strong", "smart", "wise",
                          "agile",  "tough", "charismatic" },
    *const minusattr[] = { "weak",    "stupid",
                           "foolish", "clumsy",
                           "fragile", "repulsive" };
/* also used by enlightenment for non-abbreviated status info */
const char
    *const attrname[] = { "strength", "intelligence", "wisdom",
                          "dexterity", "constitution", "charisma" };

static const struct innate {
    schar ulevel;
    long *ability;
    const char *gainstr, *losestr;
} arc_abil[] = { { 1, &(HStealth), "", "" },
                 { 1, &(HFast), "", "" },
                 { 10, &(HSearching), "perceptive", "" },
                 { 0, 0, 0, 0 } },

  bar_abil[] = { { 1, &(HPoison_resistance), "", "" },
                 { 7, &(HFast), "quick", "slow" },
                 { 15, &(HStealth), "stealthy", "" },
                 { 0, 0, 0, 0 } },

  cav_abil[] = { { 7, &(HFast), "quick", "slow" },
                 { 15, &(HWarning), "sensitive", "" },
                 { 0, 0, 0, 0 } },

  hea_abil[] = { { 1, &(HPoison_resistance), "", "" },
                 { 15, &(HWarning), "sensitive", "" },
                 { 0, 0, 0, 0 } },

  kni_abil[] = { { 7, &(HFast), "quick", "slow" }, { 0, 0, 0, 0 } },

  mon_abil[] = { { 1, &(HFast), "", "" },
                 { 1, &(HSleep_resistance), "", "" },
                 { 1, &(HSee_invisible), "", "" },
                 { 3, &(HPoison_resistance), "healthy", "" },
                 { 5, &(HStealth), "stealthy", "" },
                 { 7, &(HWarning), "sensitive", "" },
                 { 9, &(HSearching), "perceptive", "unaware" },
                 { 11, &(HFire_resistance), "cool", "warmer" },
                 { 13, &(HCold_resistance), "warm", "cooler" },
                 { 15, &(HShock_resistance), "insulated", "conductive" },
                 { 17, &(HTeleport_control), "controlled", "uncontrolled" },
                 { 0, 0, 0, 0 } },

  pri_abil[] = { { 15, &(HWarning), "sensitive", "" },
                 { 20, &(HFire_resistance), "cool", "warmer" },
                 { 0, 0, 0, 0 } },

  ran_abil[] = { { 1, &(HSearching), "", "" },
                 { 7, &(HStealth), "stealthy", "" },
                 { 15, &(HSee_invisible), "", "" },
                 { 0, 0, 0, 0 } },

  rog_abil[] = { { 1, &(HStealth), "", "" },
                 { 10, &(HSearching), "perceptive", "" },
                 { 0, 0, 0, 0 } },

  sam_abil[] = { { 1, &(HFast), "", "" },
                 { 15, &(HStealth), "stealthy", "" },
                 { 0, 0, 0, 0 } },

  tou_abil[] = { { 10, &(HSearching), "perceptive", "" },
                 { 20, &(HPoison_resistance), "hardy", "" },
                 { 0, 0, 0, 0 } },

  val_abil[] = { { 1, &(HCold_resistance), "", "" },
                 { 1, &(HStealth), "", "" },
                 { 7, &(HFast), "quick", "slow" },
                 { 0, 0, 0, 0 } },

  wiz_abil[] = { { 15, &(HWarning), "sensitive", "" },
                 { 17, &(HTeleport_control), "controlled", "uncontrolled" },
                 { 0, 0, 0, 0 } },

  /* Intrinsics conferred by race */
  dwa_abil[] = { { 1, &HInfravision, "", "" },
                 { 0, 0, 0, 0 } },

  elf_abil[] = { { 1, &HInfravision, "", "" },
                 { 4, &HSleep_resistance, "awake", "tired" },
                 { 0, 0, 0, 0 } },

  gno_abil[] = { { 1, &HInfravision, "", "" },
                 { 0, 0, 0, 0 } },

  orc_abil[] = { { 1, &HInfravision, "", "" },
                 { 1, &HPoison_resistance, "", "" },
                 { 0, 0, 0, 0 } },

  hum_abil[] = { { 0, 0, 0, 0 } };

static void NDECL(exerper);
static void FDECL(postadjabil, (long *));
static const struct innate *FDECL(role_abil, (int));
static const struct innate *FDECL(check_innate_abil, (long *, long));
static int FDECL(innately, (long *));

/* adjust an attribute; return TRUE if change is made, FALSE otherwise */
boolean
adjattrib(ndx, incr, msgflg)
int ndx, incr;
int msgflg; /* positive => no message, zero => message, and */
{           /* negative => conditional (msg if change made) */
    int old_acurr, old_abase, old_amax, decr;
    boolean abonflg;
    const char *attrstr;

    if (Fixed_abil || !incr)
        return FALSE;

    if ((ndx == A_INT || ndx == A_WIS) && uarmh && uarmh->otyp == DUNCE_CAP) {
        if (msgflg == 0)
            Your("cap constricts briefly, then relaxes again.");
        return FALSE;
    }

    old_acurr = ACURR(ndx);
    old_abase = ABASE(ndx);
    old_amax = AMAX(ndx);
    ABASE(ndx) += incr; /* when incr is negative, this reduces ABASE() */
    if (incr > 0) {
        if (ABASE(ndx) > AMAX(ndx)) {
            AMAX(ndx) = ABASE(ndx);
            if (AMAX(ndx) > ATTRMAX(ndx))
                ABASE(ndx) = AMAX(ndx) = ATTRMAX(ndx);
        }
        attrstr = plusattr[ndx];
        abonflg = (ABON(ndx) < 0);
    } else { /* incr is negative */
        if (ABASE(ndx) < ATTRMIN(ndx)) {
            /*
             * If base value has dropped so low that it is trying to be
             * taken below the minimum, reduce max value (peak reached)
             * instead.  That means that restore ability and repeated
             * applications of unicorn horn will not be able to recover
             * all the lost value.  As of 3.6.2, we only take away
             * some (average half, possibly zero) of the excess from max
             * instead of all of it, but without intervening recovery, it
             * can still eventually drop to the minimum allowed.  After
             * that, it can't be recovered, only improved with new gains.
             *
             * This used to assign a new negative value to incr and then
             * add it, but that could affect messages below, possibly
             * making a large decrease be described as a small one.
             *
             * decr = rn2(-(ABASE - ATTRMIN) + 1);
             */
            decr = rn2(ATTRMIN(ndx) - ABASE(ndx) + 1);
            ABASE(ndx) = ATTRMIN(ndx);
            AMAX(ndx) -= decr;
            if (AMAX(ndx) < ATTRMIN(ndx))
                AMAX(ndx) = ATTRMIN(ndx);
        }
        attrstr = minusattr[ndx];
        abonflg = (ABON(ndx) > 0);
    }
    if (ACURR(ndx) == old_acurr) {
        if (msgflg == 0 && flags.verbose) {
            if (ABASE(ndx) == old_abase && AMAX(ndx) == old_amax) {
                pline("You're %s as %s as you can get.",
                      abonflg ? "currently" : "already", attrstr);
            } else {
                /* current stayed the same but base value changed, or
                   base is at minimum and reduction caused max to drop */
                Your("innate %s has %s.", attrname[ndx],
                     (incr > 0) ? "improved" : "declined");
            }
        }
        return FALSE;
    }

    if (msgflg <= 0)
        You_feel("%s%s!", (incr > 1 || incr < -1) ? "very " : "", attrstr);
    g.context.botl = TRUE;
    if (g.program_state.in_moveloop && (ndx == A_STR || ndx == A_CON))
        (void) encumber_msg();
    return TRUE;
}

void
gainstr(otmp, incr, givemsg)
struct obj *otmp;
int incr;
boolean givemsg;
{
    int num = incr;

    if (!num) {
        if (ABASE(A_STR) < 18)
            num = (rn2(4) ? 1 : rnd(6));
        else if (ABASE(A_STR) < STR18(85))
            num = rnd(10);
        else
            num = 1;
    }
    (void) adjattrib(A_STR, (otmp && otmp->cursed) ? -num : num,
                     givemsg ? -1 : 1);
}

/* may kill you; cause may be poison or monster like 'a' */
void
losestr(num)
register int num;
{
    int ustr = ABASE(A_STR) - num;

    while (ustr < 3) {
        ++ustr;
        --num;
        if (Upolyd) {
            u.mh -= 6;
            u.mhmax -= 6;
        } else {
            u.uhp -= 6;
            u.uhpmax -= 6;
        }
    }
    (void) adjattrib(A_STR, -num, 1);
}

static const struct poison_effect_message {
    void VDECL((*delivery_func), (const char *, ...));
    const char *effect_msg;
} poiseff[] = {
    { You_feel, "weaker" },             /* A_STR */
    { Your, "brain is on fire" },       /* A_INT */
    { Your, "judgement is impaired" },  /* A_WIS */
    { Your, "muscles won't obey you" }, /* A_DEX */
    { You_feel, "very sick" },          /* A_CON */
    { You, "break out in hives" }       /* A_CHA */
};

/* feedback for attribute loss due to poisoning */
void
poisontell(typ, exclaim)
int typ;         /* which attribute */
boolean exclaim; /* emphasis */
{
    void VDECL((*func), (const char *, ...)) = poiseff[typ].delivery_func;
    const char *msg_txt = poiseff[typ].effect_msg;

    /*
     * "You feel weaker" or "you feel very sick" aren't appropriate when
     * wearing or wielding something (gauntlets of power, Ogresmasher)
     * which forces the attribute to maintain its maximum value.
     * Phrasing for other attributes which might have fixed values
     * (dunce cap) is such that we don't need message fixups for them.
     */
    if (typ == A_STR && ACURR(A_STR) == STR19(25))
        msg_txt = "innately weaker";
    else if (typ == A_CON && ACURR(A_CON) == 25)
        msg_txt = "sick inside";

    (*func)("%s%c", msg_txt, exclaim ? '!' : '.');
}

/* called when an attack or trap has poisoned hero (used to be in mon.c) */
void
poisoned(reason, typ, pkiller, fatal, thrown_weapon)
const char *reason,    /* controls what messages we display */
           *pkiller;   /* for score+log file if fatal */
int typ, fatal;        /* if fatal is 0, limit damage to adjattrib */
boolean thrown_weapon; /* thrown weapons are less deadly */
{
    int i, loss, kprefix = KILLED_BY_AN;

    /* inform player about being poisoned unless that's already been done;
       "blast" has given a "blast of poison gas" message; "poison arrow",
       "poison dart", etc have implicitly given poison messages too... */
    if (strcmp(reason, "blast") && !strstri(reason, "poison")) {
        boolean plural = (reason[strlen(reason) - 1] == 's') ? 1 : 0;

        /* avoid "The" Orcus's sting was poisoned... */
        pline("%s%s %s poisoned!",
              isupper((uchar) *reason) ? "" : "The ", reason,
              plural ? "were" : "was");
    }
    if (Poison_resistance) {
        if (!strcmp(reason, "blast"))
            shieldeff(u.ux, u.uy);
        pline_The("poison doesn't seem to affect you.");
        return;
    }

    /* suppress killer prefix if it already has one */
    i = name_to_mon(pkiller);
    if (i >= LOW_PM && (mons[i].geno & G_UNIQ)) {
        kprefix = KILLED_BY;
        if (!type_is_pname(&mons[i]))
            pkiller = the(pkiller);
    } else if (!strncmpi(pkiller, "the ", 4) || !strncmpi(pkiller, "an ", 3)
               || !strncmpi(pkiller, "a ", 2)) {
        /*[ does this need a plural check too? ]*/
        kprefix = KILLED_BY;
    }

    i = !fatal ? 1 : rn2(fatal + (thrown_weapon ? 20 : 0));
    if (i == 0 && typ != A_CHA) {
        /* instant kill */
        u.uhp = -1;
        g.context.botl = TRUE;
        pline_The("poison was deadly...");
    } else if (i > 5) {
        /* HP damage; more likely--but less severe--with missiles */
        loss = thrown_weapon ? rnd(6) : rn1(10, 6);
        losehp(loss, pkiller, kprefix); /* poison damage */
    } else {
        /* attribute loss; if typ is A_STR, reduction in current and
           maximum HP will occur once strength has dropped down to 3 */
        loss = (thrown_weapon || !fatal) ? 1 : d(2, 2); /* was rn1(3,3) */
        /* check that a stat change was made */
        if (adjattrib(typ, -loss, 1))
            poisontell(typ, TRUE);
    }

    if (u.uhp < 1) {
        g.killer.format = kprefix;
        Strcpy(g.killer.name, pkiller);
        /* "Poisoned by a poisoned ___" is redundant */
        done(strstri(pkiller, "poison") ? DIED : POISONING);
    }
    (void) encumber_msg();
}

void
change_luck(n)
register schar n;
{
    u.uluck += n;
    if (u.uluck < 0 && u.uluck < LUCKMIN)
        u.uluck = LUCKMIN;
    if (u.uluck > 0 && u.uluck > LUCKMAX)
        u.uluck = LUCKMAX;
}

int
stone_luck(parameter)
boolean parameter; /* So I can't think up of a good name.  So sue me. --KAA */
{
    register struct obj *otmp;
    register long bonchance = 0;

    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (confers_luck(otmp)) {
            if (otmp->cursed)
                bonchance -= otmp->quan;
            else if (otmp->blessed)
                bonchance += otmp->quan;
            else if (parameter)
                bonchance += otmp->quan;
        }

    return sgn((int) bonchance);
}

/* there has just been an inventory change affecting a luck-granting item */
void
set_moreluck()
{
    int luckbon = stone_luck(TRUE);

    if (!luckbon && !carrying(LUCKSTONE))
        u.moreluck = 0;
    else if (luckbon >= 0)
        u.moreluck = LUCKADD;
    else
        u.moreluck = -LUCKADD;
}

void
restore_attrib()
{
    int i, equilibrium;;

    /*
     * Note:  this gets called on every turn but ATIME() is never set
     * to non-zero anywhere, and ATEMP() is only used for strength loss
     * from hunger, so it doesn't actually do anything.
     */

    for (i = 0; i < A_MAX; i++) { /* all temporary losses/gains */
        equilibrium = (i == A_STR && u.uhs >= WEAK) ? -1 : 0;
        if (ATEMP(i) != equilibrium && ATIME(i) != 0) {
            if (!(--(ATIME(i)))) { /* countdown for change */
                ATEMP(i) += (ATEMP(i) > 0) ? -1 : 1;
                g.context.botl = TRUE;
                if (ATEMP(i)) /* reset timer */
                    ATIME(i) = 100 / ACURR(A_CON);
            }
        }
    }
    if (g.context.botl)
        (void) encumber_msg();
}

#define AVAL 50 /* tune value for exercise gains */

void
exercise(i, inc_or_dec)
int i;
boolean inc_or_dec;
{
    debugpline0("Exercise:");
    if (i == A_INT || i == A_CHA)
        return; /* can't exercise these */

    /* no physical exercise while polymorphed; the body's temporary */
    if (Upolyd && i != A_WIS)
        return;

    if (abs(AEXE(i)) < AVAL) {
        /*
         *      Law of diminishing returns (Part I):
         *
         *      Gain is harder at higher attribute values.
         *      79% at "3" --> 0% at "18"
         *      Loss is even at all levels (50%).
         *
         *      Note: *YES* ACURR is the right one to use.
         */
        AEXE(i) += (inc_or_dec) ? (rn2(19) > ACURR(i)) : -rn2(2);
        debugpline3("%s, %s AEXE = %d",
                    (i == A_STR) ? "Str" : (i == A_WIS) ? "Wis" : (i == A_DEX)
                                                                      ? "Dex"
                                                                      : "Con",
                    (inc_or_dec) ? "inc" : "dec", AEXE(i));
    }
    if (g.moves > 0 && (i == A_STR || i == A_CON))
        (void) encumber_msg();
}

static void
exerper()
{
    if (!(g.moves % 10)) {
        /* Hunger Checks */

        int hs = (u.uhunger > 1000) ? SATIATED : (u.uhunger > 150)
                                                     ? NOT_HUNGRY
                                                     : (u.uhunger > 50)
                                                           ? HUNGRY
                                                           : (u.uhunger > 0)
                                                                 ? WEAK
                                                                 : FAINTING;

        debugpline0("exerper: Hunger checks");
        switch (hs) {
        case SATIATED:
            exercise(A_DEX, FALSE);
            if (Role_if(PM_MONK))
                exercise(A_WIS, FALSE);
            break;
        case NOT_HUNGRY:
            exercise(A_CON, TRUE);
            break;
        case WEAK:
            exercise(A_STR, FALSE);
            if (Role_if(PM_MONK)) /* fasting */
                exercise(A_WIS, TRUE);
            break;
        case FAINTING:
        case FAINTED:
            exercise(A_CON, FALSE);
            break;
        }

        /* Encumbrance Checks */
        debugpline0("exerper: Encumber checks");
        switch (near_capacity()) {
        case MOD_ENCUMBER:
            exercise(A_STR, TRUE);
            break;
        case HVY_ENCUMBER:
            exercise(A_STR, TRUE);
            exercise(A_DEX, FALSE);
            break;
        case EXT_ENCUMBER:
            exercise(A_DEX, FALSE);
            exercise(A_CON, FALSE);
            break;
        }
    }

    /* status checks */
    if (!(g.moves % 5)) {
        debugpline0("exerper: Status checks");
        if ((HClairvoyant & (INTRINSIC | TIMEOUT)) && !BClairvoyant)
            exercise(A_WIS, TRUE);
        if (HRegeneration)
            exercise(A_STR, TRUE);

        if (Sick || Vomiting)
            exercise(A_CON, FALSE);
        if (Confusion || Hallucination)
            exercise(A_WIS, FALSE);
        if ((Wounded_legs && !u.usteed) || Fumbling || HStun)
            exercise(A_DEX, FALSE);
    }
}

/* exercise/abuse text (must be in attribute order, not botl order);
   phrased as "You must have been [][0]." or "You haven't been [][1]." */
static NEARDATA const char *const exertext[A_MAX][2] = {
    { "exercising diligently", "exercising properly" },           /* Str */
    { 0, 0 },                                                     /* Int */
    { "very observant", "paying attention" },                     /* Wis */
    { "working on your reflexes", "working on reflexes lately" }, /* Dex */
    { "leading a healthy life-style", "watching your health" },   /* Con */
    { 0, 0 },                                                     /* Cha */
};

void
exerchk()
{
    int i, ax, mod_val, lolim, hilim;

    /*  Check out the periodic accumulations */
    exerper();

    if (g.moves >= g.context.next_attrib_check) {
        debugpline1("exerchk: ready to test. multi = %d.", g.multi);
    }
    /*  Are we ready for a test? */
    if (g.moves >= g.context.next_attrib_check && !g.multi) {
        debugpline0("exerchk: testing.");
        /*
         *      Law of diminishing returns (Part II):
         *
         *      The effects of "exercise" and "abuse" wear
         *      off over time.  Even if you *don't* get an
         *      increase/decrease, you lose some of the
         *      accumulated effects.
         */
        for (i = 0; i < A_MAX; ++i) {
            ax = AEXE(i);
            /* nothing to do here if no exercise or abuse has occurred
               (Int and Cha always fall into this category) */
            if (!ax)
                continue; /* ok to skip nextattrib */

            mod_val = sgn(ax); /* +1 or -1; used below */
            /* no further effect for exercise if at max or abuse if at min;
               can't exceed 18 via exercise even if actual max is higher */
            lolim = ATTRMIN(i); /* usually 3; might be higher */
            hilim = ATTRMAX(i); /* usually 18; maybe lower or higher */
            if (hilim > 18)
                hilim = 18;
            if ((ax < 0) ? (ABASE(i) <= lolim) : (ABASE(i) >= hilim))
                goto nextattrib;
            /* can't exercise non-Wisdom while polymorphed; previous
               exercise/abuse gradually wears off without impact then */
            if (Upolyd && i != A_WIS)
                goto nextattrib;

            debugpline2("exerchk: testing %s (%d).",
                        (i == A_STR)
                            ? "Str"
                            : (i == A_INT)
                                  ? "Int?"
                                  : (i == A_WIS)
                                        ? "Wis"
                                        : (i == A_DEX)
                                              ? "Dex"
                                              : (i == A_CON)
                                                    ? "Con"
                                                    : (i == A_CHA)
                                                          ? "Cha?"
                                                          : "???",
                        ax);
            /*
             *  Law of diminishing returns (Part III):
             *
             *  You don't *always* gain by exercising.
             *  [MRS 92/10/28 - Treat Wisdom specially for balance.]
             */
            if (rn2(AVAL) > ((i != A_WIS) ? (abs(ax) * 2 / 3) : abs(ax)))
                goto nextattrib;

            debugpline1("exerchk: changing %d.", i);
            if (adjattrib(i, mod_val, -1)) {
                debugpline1("exerchk: changed %d.", i);
                /* if you actually changed an attrib - zero accumulation */
                AEXE(i) = ax = 0;
                /* then print an explanation */
                You("%s %s.",
                    (mod_val > 0) ? "must have been" : "haven't been",
                    exertext[i][(mod_val > 0) ? 0 : 1]);
            }
 nextattrib:
            /* this used to be ``AEXE(i) /= 2'' but that would produce
               platform-dependent rounding/truncation for negative vals */
            AEXE(i) = (abs(ax) / 2) * mod_val;
        }
        g.context.next_attrib_check += rn1(200, 800);
        debugpline1("exerchk: next check at %ld.", g.context.next_attrib_check);
    }
}

void
init_attr(np)
register int np;
{
    register int i, x, tryct;

    for (i = 0; i < A_MAX; i++) {
        ABASE(i) = AMAX(i) = g.urole.attrbase[i];
        ATEMP(i) = ATIME(i) = 0;
        np -= g.urole.attrbase[i];
    }

    tryct = 0;
    while (np > 0 && tryct < 100) {
        x = rn2(100);
        for (i = 0; (i < A_MAX) && ((x -= g.urole.attrdist[i]) > 0); i++)
            ;
        if (i >= A_MAX)
            continue; /* impossible */

        if (ABASE(i) >= ATTRMAX(i)) {
            tryct++;
            continue;
        }
        tryct = 0;
        ABASE(i)++;
        AMAX(i)++;
        np--;
    }

    tryct = 0;
    while (np < 0 && tryct < 100) { /* for redistribution */

        x = rn2(100);
        for (i = 0; (i < A_MAX) && ((x -= g.urole.attrdist[i]) > 0); i++)
            ;
        if (i >= A_MAX)
            continue; /* impossible */

        if (ABASE(i) <= ATTRMIN(i)) {
            tryct++;
            continue;
        }
        tryct = 0;
        ABASE(i)--;
        AMAX(i)--;
        np++;
    }
}

void
redist_attr()
{
    register int i, tmp;

    for (i = 0; i < A_MAX; i++) {
        if (i == A_INT || i == A_WIS)
            continue;
        /* Polymorphing doesn't change your mind */
        tmp = AMAX(i);
        AMAX(i) += (rn2(5) - 2);
        if (AMAX(i) > ATTRMAX(i))
            AMAX(i) = ATTRMAX(i);
        if (AMAX(i) < ATTRMIN(i))
            AMAX(i) = ATTRMIN(i);
        ABASE(i) = ABASE(i) * AMAX(i) / tmp;
        /* ABASE(i) > ATTRMAX(i) is impossible */
        if (ABASE(i) < ATTRMIN(i))
            ABASE(i) = ATTRMIN(i);
    }
    (void) encumber_msg();
}

static
void
postadjabil(ability)
long *ability;
{
    if (!ability)
        return;
    if (ability == &(HWarning) || ability == &(HSee_invisible))
        see_monsters();
}

static const struct innate *
role_abil(r)
int r;
{
    const struct {
        short role;
        const struct innate *abil;
    } roleabils[] = {
        { PM_ARCHEOLOGIST, arc_abil },
        { PM_BARBARIAN, bar_abil },
        { PM_CAVEMAN, cav_abil },
        { PM_HEALER, hea_abil },
        { PM_KNIGHT, kni_abil },
        { PM_MONK, mon_abil },
        { PM_PRIEST, pri_abil },
        { PM_RANGER, ran_abil },
        { PM_ROGUE, rog_abil },
        { PM_SAMURAI, sam_abil },
        { PM_TOURIST, tou_abil },
        { PM_VALKYRIE, val_abil },
        { PM_WIZARD, wiz_abil },
        { 0, 0 }
    };
    int i;

    for (i = 0; roleabils[i].abil && roleabils[i].role != r; i++)
        continue;
    return roleabils[i].abil;
}

static const struct innate *
check_innate_abil(ability, frommask)
long *ability;
long frommask;
{
    const struct innate *abil = 0;

    if (frommask == FROMEXPER)
        abil = role_abil(Role_switch);
    else if (frommask == FROMRACE)
        switch (Race_switch) {
        case PM_DWARF:
            abil = dwa_abil;
            break;
        case PM_ELF:
            abil = elf_abil;
            break;
        case PM_GNOME:
            abil = gno_abil;
            break;
        case PM_ORC:
            abil = orc_abil;
            break;
        case PM_HUMAN:
            abil = hum_abil;
            break;
        default:
            break;
        }

    while (abil && abil->ability) {
        if ((abil->ability == ability) && (u.ulevel >= abil->ulevel))
            return abil;
        abil++;
    }
    return (struct innate *) 0;
}

/* reasons for innate ability */
#define FROM_NONE 0
#define FROM_ROLE 1 /* from experience at level 1 */
#define FROM_RACE 2
#define FROM_INTR 3 /* intrinsically (eating some corpse or prayer reward) */
#define FROM_EXP  4 /* from experience for some level > 1 */
#define FROM_FORM 5
#define FROM_LYCN 6

/* check whether particular ability has been obtained via innate attribute */
static int
innately(ability)
long *ability;
{
    const struct innate *iptr;

    if ((iptr = check_innate_abil(ability, FROMEXPER)) != 0)
        return (iptr->ulevel == 1) ? FROM_ROLE : FROM_EXP;
    if ((iptr = check_innate_abil(ability, FROMRACE)) != 0)
        return FROM_RACE;
    if ((*ability & FROMOUTSIDE) != 0L)
        return FROM_INTR;
    if ((*ability & FROMFORM) != 0L)
        return FROM_FORM;
    return FROM_NONE;
}

int
is_innate(propidx)
int propidx;
{
    int innateness;

    /* innately() would report FROM_FORM for this; caller wants specificity */
    if (propidx == DRAIN_RES && u.ulycn >= LOW_PM)
        return FROM_LYCN;
    if (propidx == FAST && Very_fast)
        return FROM_NONE; /* can't become very fast innately */
    if ((innateness = innately(&u.uprops[propidx].intrinsic)) != FROM_NONE)
        return innateness;
    if (propidx == JUMPING && Role_if(PM_KNIGHT)
        /* knight has intrinsic jumping, but extrinsic is more versatile so
           ignore innateness if equipment is going to claim responsibility */
        && !u.uprops[propidx].extrinsic)
        return FROM_ROLE;
    if (propidx == BLINDED && !haseyes(g.youmonst.data))
        return FROM_FORM;
    return FROM_NONE;
}

char *
from_what(propidx)
int propidx; /* special cases can have negative values */
{
    static char buf[BUFSZ];

    buf[0] = '\0';
    /*
     * Restrict the source of the attributes just to debug mode for now
     */
    if (wizard) {
        static NEARDATA const char because_of[] = " because of %s";

        if (propidx >= 0) {
            char *p;
            struct obj *obj = (struct obj *) 0;
            int innateness = is_innate(propidx);

            /*
             * Properties can be obtained from multiple sources and we
             * try to pick the most significant one.  Classification
             * priority is not set in stone; current precedence is:
             * "from the start" (from role or race at level 1),
             * "from outside" (eating corpse, divine reward, blessed potion),
             * "from experience" (from role or race at level 2+),
             * "from current form" (while polymorphed),
             * "from timed effect" (potion or spell),
             * "from worn/wielded equipment" (Firebrand, elven boots, &c),
             * "from carried equipment" (mainly quest artifacts).
             * There are exceptions.  Versatile jumping from spell or boots
             * takes priority over knight's innate but limited jumping.
             */
            if (propidx == BLINDED && u.uroleplay.blind)
                Sprintf(buf, " from birth");
            else if (innateness == FROM_ROLE || innateness == FROM_RACE)
                Strcpy(buf, " innately");
            else if (innateness == FROM_INTR) /* [].intrinsic & FROMOUTSIDE */
                Strcpy(buf, " intrinsically");
            else if (innateness == FROM_EXP)
                Strcpy(buf, " because of your experience");
            else if (innateness == FROM_LYCN)
                Strcpy(buf, " due to your lycanthropy");
            else if (innateness == FROM_FORM)
                Strcpy(buf, " from current creature form");
            else if (propidx == FAST && Very_fast)
                Sprintf(buf, because_of,
                        ((HFast & TIMEOUT) != 0L) ? "a potion or spell"
                          : ((EFast & W_ARMF) != 0L && uarmf->dknown
                             && objects[uarmf->otyp].oc_name_known)
                              ? ysimple_name(uarmf) /* speed boots */
                                : EFast ? "worn equipment"
                                  : something);
            else if (wizard
                     && (obj = what_gives(&u.uprops[propidx].extrinsic)) != 0)
                Sprintf(buf, because_of, obj->oartifact
                                             ? bare_artifactname(obj)
                                             : ysimple_name(obj));
            else if (propidx == BLINDED && Blindfolded_only)
                Sprintf(buf, because_of, ysimple_name(ublindf));

            /* remove some verbosity and/or redundancy */
            if ((p = strstri(buf, " pair of ")) != 0)
                copynchars(p + 1, p + 9, BUFSZ); /* overlapping buffers ok */
            else if (propidx == STRANGLED
                     && (p = strstri(buf, " of strangulation")) != 0)
                *p = '\0';

        } else { /* negative property index */
            /* if more blocking capabilities get implemented we'll need to
               replace this with what_blocks() comparable to what_gives() */
            switch (-propidx) {
            case BLINDED:
                if (ublindf
                    && ublindf->oartifact == ART_EYES_OF_THE_OVERWORLD)
                    Sprintf(buf, because_of, bare_artifactname(ublindf));
                break;
            case INVIS:
                if (u.uprops[INVIS].blocked & W_ARMC)
                    Sprintf(buf, because_of,
                            ysimple_name(uarmc)); /* mummy wrapping */
                break;
            case CLAIRVOYANT:
                if (wizard && (u.uprops[CLAIRVOYANT].blocked & W_ARMH))
                    Sprintf(buf, because_of,
                            ysimple_name(uarmh)); /* cornuthaum */
                break;
            }
        }

    } /*wizard*/
    return buf;
}

void
adjabil(oldlevel, newlevel)
int oldlevel, newlevel;
{
    register const struct innate *abil, *rabil;
    long prevabil, mask = FROMEXPER;

    abil = role_abil(Role_switch);

    switch (Race_switch) {
    case PM_ELF:
        rabil = elf_abil;
        break;
    case PM_ORC:
        rabil = orc_abil;
        break;
    case PM_HUMAN:
    case PM_DWARF:
    case PM_GNOME:
    default:
        rabil = 0;
        break;
    }

    while (abil || rabil) {
        /* Have we finished with the intrinsics list? */
        if (!abil || !abil->ability) {
            /* Try the race intrinsics */
            if (!rabil || !rabil->ability)
                break;
            abil = rabil;
            rabil = 0;
            mask = FROMRACE;
        }
        prevabil = *(abil->ability);
        if (oldlevel < abil->ulevel && newlevel >= abil->ulevel) {
            /* Abilities gained at level 1 can never be lost
             * via level loss, only via means that remove _any_
             * sort of ability.  A "gain" of such an ability from
             * an outside source is devoid of meaning, so we set
             * FROMOUTSIDE to avoid such gains.
             */
            if (abil->ulevel == 1)
                *(abil->ability) |= (mask | FROMOUTSIDE);
            else
                *(abil->ability) |= mask;
            if (!(*(abil->ability) & INTRINSIC & ~mask)) {
                if (*(abil->gainstr))
                    You_feel("%s!", abil->gainstr);
            }
        } else if (oldlevel >= abil->ulevel && newlevel < abil->ulevel) {
            *(abil->ability) &= ~mask;
            if (!(*(abil->ability) & INTRINSIC)) {
                if (*(abil->losestr))
                    You_feel("%s!", abil->losestr);
                else if (*(abil->gainstr))
                    You_feel("less %s!", abil->gainstr);
            }
        }
        if (prevabil != *(abil->ability)) /* it changed */
            postadjabil(abil->ability);
        abil++;
    }

    if (oldlevel > 0) {
        if (newlevel > oldlevel)
            add_weapon_skill(newlevel - oldlevel);
        else
            lose_weapon_skill(oldlevel - newlevel);
    }
}

int
newhp()
{
    int hp, conplus;

    if (u.ulevel == 0) {
        /* Initialize hit points */
        hp = g.urole.hpadv.infix + g.urace.hpadv.infix;
        if (g.urole.hpadv.inrnd > 0)
            hp += rnd(g.urole.hpadv.inrnd);
        if (g.urace.hpadv.inrnd > 0)
            hp += rnd(g.urace.hpadv.inrnd);
        if (g.moves <= 1L) { /* initial hero; skip for polyself to new man */
            /* Initialize alignment stuff */
            u.ualign.type = aligns[flags.initalign].value;
            u.ualign.record = g.urole.initrecord;
        }
        /* no Con adjustment for initial hit points */
    } else {
        if (u.ulevel < g.urole.xlev) {
            hp = g.urole.hpadv.lofix + g.urace.hpadv.lofix;
            if (g.urole.hpadv.lornd > 0)
                hp += rnd(g.urole.hpadv.lornd);
            if (g.urace.hpadv.lornd > 0)
                hp += rnd(g.urace.hpadv.lornd);
        } else {
            hp = g.urole.hpadv.hifix + g.urace.hpadv.hifix;
            if (g.urole.hpadv.hirnd > 0)
                hp += rnd(g.urole.hpadv.hirnd);
            if (g.urace.hpadv.hirnd > 0)
                hp += rnd(g.urace.hpadv.hirnd);
        }
        if (ACURR(A_CON) <= 3)
            conplus = -2;
        else if (ACURR(A_CON) <= 6)
            conplus = -1;
        else if (ACURR(A_CON) <= 14)
            conplus = 0;
        else if (ACURR(A_CON) <= 16)
            conplus = 1;
        else if (ACURR(A_CON) == 17)
            conplus = 2;
        else if (ACURR(A_CON) == 18)
            conplus = 3;
        else
            conplus = 4;
        hp += conplus;
    }
    if (hp <= 0)
        hp = 1;
    if (u.ulevel < MAXULEV)
        u.uhpinc[u.ulevel] = (xchar) hp;
    return hp;
}

schar
acurr(x)
int x;
{
    register int tmp = (u.abon.a[x] + u.atemp.a[x] + u.acurr.a[x]);

    if (x == A_STR) {
        if (tmp >= 125 || (uarmg && uarmg->otyp == GAUNTLETS_OF_POWER))
            return (schar) 125;
        else
#ifdef WIN32_BUG
            return (x = ((tmp <= 3) ? 3 : tmp));
#else
            return (schar) ((tmp <= 3) ? 3 : tmp);
#endif
    } else if (x == A_CHA) {
        if (tmp < 18
            && (g.youmonst.data->mlet == S_NYMPH || u.umonnum == PM_SUCCUBUS
                || u.umonnum == PM_INCUBUS))
            return (schar) 18;
    } else if (x == A_CON) {
        if (uwep && uwep->oartifact == ART_OGRESMASHER)
            return (schar) 25;
    } else if (x == A_INT || x == A_WIS) {
        /* yes, this may raise int/wis if player is sufficiently
         * stupid.  there are lower levels of cognition than "dunce".
         */
        if (uarmh && uarmh->otyp == DUNCE_CAP)
            return (schar) 6;
    }
#ifdef WIN32_BUG
    return (x = ((tmp >= 25) ? 25 : (tmp <= 3) ? 3 : tmp));
#else
    return (schar) ((tmp >= 25) ? 25 : (tmp <= 3) ? 3 : tmp);
#endif
}

/* condense clumsy ACURR(A_STR) value into value that fits into game formulas
 */
schar
acurrstr()
{
    register int str = ACURR(A_STR);

    if (str <= 18)
        return (schar) str;
    if (str <= 121)
        return (schar) (19 + str / 50); /* map to 19..21 */
    else
        return (schar) (min(str, 125) - 100); /* 22..25 */
}

/* when wearing (or taking off) an unID'd item, this routine is used
   to distinguish between observable +0 result and no-visible-effect
   due to an attribute not being able to exceed maximum or minimum */
boolean
extremeattr(attrindx) /* does attrindx's value match its max or min? */
int attrindx;
{
    /* Fixed_abil and racial MINATTR/MAXATTR aren't relevant here */
    int lolimit = 3, hilimit = 25, curval = ACURR(attrindx);

    /* upper limit for Str is 25 but its value is encoded differently */
    if (attrindx == A_STR) {
        hilimit = STR19(25); /* 125 */
        /* lower limit for Str can also be 25 */
        if (uarmg && uarmg->otyp == GAUNTLETS_OF_POWER)
            lolimit = hilimit;
    } else if (attrindx == A_CON) {
        if (uwep && uwep->oartifact == ART_OGRESMASHER)
            lolimit = hilimit;
    }
    /* this exception is hypothetical; the only other worn item affecting
       Int or Wis is another helmet so can't be in use at the same time */
    if (attrindx == A_INT || attrindx == A_WIS) {
        if (uarmh && uarmh->otyp == DUNCE_CAP)
            hilimit = lolimit = 6;
    }

    /* are we currently at either limit? */
    return (curval == lolimit || curval == hilimit) ? TRUE : FALSE;
}

/* avoid possible problems with alignment overflow, and provide a centralized
   location for any future alignment limits */
void
adjalign(n)
int n;
{
    int newalign = u.ualign.record + n;

    if (n < 0) {
        if (newalign < u.ualign.record)
            u.ualign.record = newalign;
    } else if (newalign > u.ualign.record) {
        u.ualign.record = newalign;
        if (u.ualign.record > ALIGNLIM)
            u.ualign.record = ALIGNLIM;
    }
}

/* change hero's alignment type, possibly losing use of artifacts */
void
uchangealign(newalign, reason)
int newalign;
int reason; /* 0==conversion, 1==helm-of-OA on, 2==helm-of-OA off */
{
    aligntyp oldalign = u.ualign.type;

    u.ublessed = 0; /* lose divine protection */
    /* You/Your/pline message with call flush_screen(), triggering bot(),
       so the actual data change needs to come before the message */
    g.context.botl = TRUE; /* status line needs updating */
    if (reason == 0) {
        /* conversion via altar */
        u.ualignbase[A_CURRENT] = (aligntyp) newalign;
        /* worn helm of opposite alignment might block change */
        if (!uarmh || uarmh->otyp != HELM_OF_OPPOSITE_ALIGNMENT)
            u.ualign.type = u.ualignbase[A_CURRENT];
        You("have a %ssense of a new direction.",
            (u.ualign.type != oldalign) ? "sudden " : "");
    } else {
        /* putting on or taking off a helm of opposite alignment */
        u.ualign.type = (aligntyp) newalign;
        if (reason == 1)
            Your("mind oscillates %s.", Hallucination ? "wildly" : "briefly");
        else if (reason == 2)
            Your("mind is %s.", Hallucination
                                    ? "much of a muchness"
                                    : "back in sync with your body");
    }
    if (u.ualign.type != oldalign) {
        u.ualign.record = 0; /* slate is wiped clean */
        retouch_equipment(0);
    }
}

/*attrib.c*/

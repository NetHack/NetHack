/* NetHack 3.7	pray.c	$NHDT-Date: 1649454525 2022/04/08 21:48:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.168 $ */
/* Copyright (c) Benson I. Margulies, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int prayer_done(void);
static struct obj *worst_cursed_item(void);
static int in_trouble(void);
static void fix_curse_trouble(struct obj *, const char *);
static void fix_worst_trouble(int);
static void angrygods(aligntyp);
static void at_your_feet(const char *);
static void gcrownu(void);
static void give_spell(void);
static void pleased(aligntyp);
static void godvoice(aligntyp, const char *);
static void god_zaps_you(aligntyp);
static void fry_by_god(aligntyp, boolean);
static void gods_angry(aligntyp);
static void gods_upset(aligntyp);
static void consume_offering(struct obj *);
static void offer_too_soon(aligntyp);
static void desecrate_high_altar(aligntyp);
static boolean pray_revive(void);
static boolean water_prayer(boolean);
static boolean blocked_boulder(int, int);

/* simplify a few tests */
#define Cursed_obj(obj, typ) ((obj) && (obj)->otyp == (typ) && (obj)->cursed)

/*
 * Logic behind deities and altars and such:
 * + prayers are made to your god if not on an altar, and to the altar's god
 *   if you are on an altar
 * + If possible, your god answers all prayers, which is why bad things happen
 *   if you try to pray on another god's altar
 * + sacrifices work basically the same way, but the other god may decide to
 *   accept your allegiance, after which they are your god.  If rejected,
 *   your god takes over with your punishment.
 * + if you're in Gehennom, all messages come from Moloch
 */

/*
 *      Moloch, who dwells in Gehennom, is the "renegade" cruel god
 *      responsible for the theft of the Amulet from Marduk, the Creator.
 *      Moloch is unaligned.
 */
static const char *const Moloch = "Moloch";

static const char *const godvoices[] = {
    "booms out", "thunders", "rings out", "booms",
};

#define PIOUS 20
#define DEVOUT 14
#define FERVENT 9
#define STRIDENT 4

/*
 * The actual trouble priority is determined by the order of the
 * checks performed in in_trouble() rather than by these numeric
 * values, so keep that code and these values synchronized in
 * order to have the values be meaningful.
 */

#define TROUBLE_STONED 14
#define TROUBLE_SLIMED 13
#define TROUBLE_STRANGLED 12
#define TROUBLE_LAVA 11
#define TROUBLE_SICK 10
#define TROUBLE_STARVING 9
#define TROUBLE_REGION 8 /* stinking cloud */
#define TROUBLE_HIT 7
#define TROUBLE_LYCANTHROPE 6
#define TROUBLE_COLLAPSING 5
#define TROUBLE_STUCK_IN_WALL 4
#define TROUBLE_CURSED_LEVITATION 3
#define TROUBLE_UNUSEABLE_HANDS 2
#define TROUBLE_CURSED_BLINDFOLD 1

#define TROUBLE_PUNISHED (-1)
#define TROUBLE_FUMBLING (-2)
#define TROUBLE_CURSED_ITEMS (-3)
#define TROUBLE_SADDLE (-4)
#define TROUBLE_BLIND (-5)
#define TROUBLE_POISONED (-6)
#define TROUBLE_WOUNDED_LEGS (-7)
#define TROUBLE_HUNGRY (-8)
#define TROUBLE_STUNNED (-9)
#define TROUBLE_CONFUSED (-10)
#define TROUBLE_HALLUCINATION (-11)


#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar() IS_ALTAR(levl[u.ux][u.uy].typ)
#define on_shrine() ((levl[u.ux][u.uy].altarmask & AM_SHRINE) != 0)
#define a_align(x, y) ((aligntyp) Amask2align(levl[x][y].altarmask & AM_MASK))

/* critically low hit points if hp <= 5 or hp <= maxhp/N for some N */
boolean
critically_low_hp(boolean only_if_injured) /* determines whether maxhp <= 5
                                              matters */
{
    int divisor, hplim, curhp = Upolyd ? u.mh : u.uhp,
                        maxhp = Upolyd ? u.mhmax : u.uhpmax;

    if (only_if_injured && !(curhp < maxhp))
        return FALSE;
    /* if maxhp is extremely high, use lower threshold for the division test
       (golden glow cuts off at 11+5*lvl, nurse interaction at 25*lvl; this
       ought to use monster hit dice--and a smaller multiplier--rather than
       ulevel when polymorphed, but polyself doesn't maintain that) */
    hplim = 15 * u.ulevel;
    if (maxhp > hplim)
        maxhp = hplim;
    /* 7 used to be the unconditional divisor */
    switch (xlev_to_rank(u.ulevel)) { /* maps 1..30 into 0..8 */
    case 0:
    case 1:
        divisor = 5;
        break; /* explvl 1 to 5 */
    case 2:
    case 3:
        divisor = 6;
        break; /* explvl 6 to 13 */
    case 4:
    case 5:
        divisor = 7;
        break; /* explvl 14 to 21 */
    case 6:
    case 7:
        divisor = 8;
        break; /* explvl 22 to 29 */
    default:
        divisor = 9;
        break; /* explvl 30+ */
    }
    /* 5 is a magic number in TROUBLE_HIT handling below */
    return (boolean) (curhp <= 5 || curhp * divisor <= maxhp);
}

/* return True if surrounded by impassible rock, regardless of the state
   of your own location (for example, inside a doorless closet) */
boolean
stuck_in_wall(void)
{
    int i, j, x, y, count = 0;

    if (Passes_walls)
        return FALSE;
    for (i = -1; i <= 1; i++) {
        x = u.ux + i;
        for (j = -1; j <= 1; j++) {
            if (!i && !j)
                continue;
            y = u.uy + j;
            if (!isok(x, y)
                || (IS_ROCK(levl[x][y].typ)
                    && (levl[x][y].typ != SDOOR && levl[x][y].typ != SCORR))
                || (blocked_boulder(i, j) && !throws_rocks(g.youmonst.data)))
                ++count;
        }
    }
    return (count == 8) ? TRUE : FALSE;
}

/*
 * Return 0 if nothing particular seems wrong, positive numbers for
 * serious trouble, and negative numbers for comparative annoyances.
 * This returns the worst problem. There may be others, and the gods
 * may fix more than one.
 *
 * This could get as bizarre as noting surrounding opponents, (or
 * hostile dogs), but that's really hard.
 *
 * We could force rehumanize of polyselfed people, but we can't tell
 * unintentional shape changes from the other kind. Oh well.
 * 3.4.2: make an exception if polymorphed into a form which lacks
 * hands; that's a case where the ramifications override this doubt.
 */
static int
in_trouble(void)
{
    struct obj *otmp;
    int i;

    /*
     * major troubles
     */
    if (Stoned)
        return TROUBLE_STONED;
    if (Slimed)
        return TROUBLE_SLIMED;
    if (Strangled)
        return TROUBLE_STRANGLED;
    if (u.utrap && u.utraptype == TT_LAVA)
        return TROUBLE_LAVA;
    if (Sick)
        return TROUBLE_SICK;
    if (u.uhs >= WEAK)
        return TROUBLE_STARVING;
    if (region_danger())
        return TROUBLE_REGION;
    if (critically_low_hp(FALSE))
        return TROUBLE_HIT;
    if (u.ulycn >= LOW_PM)
        return TROUBLE_LYCANTHROPE;
    if (near_capacity() >= EXT_ENCUMBER && AMAX(A_STR) - ABASE(A_STR) > 3)
        return TROUBLE_COLLAPSING;
    if (stuck_in_wall())
        return TROUBLE_STUCK_IN_WALL;
    if (Cursed_obj(uarmf, LEVITATION_BOOTS)
        || stuck_ring(uleft, RIN_LEVITATION)
        || stuck_ring(uright, RIN_LEVITATION))
        return TROUBLE_CURSED_LEVITATION;
    if (nohands(g.youmonst.data) || !freehand()) {
        /* for bag/box access [cf use_container()]...
           make sure it's a case that we know how to handle;
           otherwise "fix all troubles" would get stuck in a loop */
        if (welded(uwep))
            return TROUBLE_UNUSEABLE_HANDS;
        if (Upolyd && nohands(g.youmonst.data)
            && (!Unchanging || ((otmp = unchanger()) != 0 && otmp->cursed)))
            return TROUBLE_UNUSEABLE_HANDS;
    }
    if (Blindfolded && ublindf->cursed)
        return TROUBLE_CURSED_BLINDFOLD;

    /*
     * minor troubles
     */
    if (Punished || (u.utrap && u.utraptype == TT_BURIEDBALL))
        return TROUBLE_PUNISHED;
    if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING)
        || Cursed_obj(uarmf, FUMBLE_BOOTS))
        return TROUBLE_FUMBLING;
    if (worst_cursed_item())
        return TROUBLE_CURSED_ITEMS;
    if (u.usteed) { /* can't voluntarily dismount from a cursed saddle */
        otmp = which_armor(u.usteed, W_SADDLE);
        if (Cursed_obj(otmp, SADDLE))
            return TROUBLE_SADDLE;
    }

    if (Blinded > 1 && haseyes(g.youmonst.data)
        && (!u.uswallow
            || !attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        return TROUBLE_BLIND;
    /* deafness isn't it's own trouble; healing magic cures deafness
       when it cures blindness, so do the same with trouble repair */
    if ((HDeaf & TIMEOUT) > 1L)
        return TROUBLE_BLIND;

    for (i = 0; i < A_MAX; i++)
        if (ABASE(i) < AMAX(i))
            return TROUBLE_POISONED;
    if (Wounded_legs && !u.usteed)
        return TROUBLE_WOUNDED_LEGS;
    if (u.uhs >= HUNGRY)
        return TROUBLE_HUNGRY;
    if (HStun & TIMEOUT)
        return TROUBLE_STUNNED;
    if (HConfusion & TIMEOUT)
        return TROUBLE_CONFUSED;
    if (HHallucination & TIMEOUT)
        return TROUBLE_HALLUCINATION;
    return 0;
}

/* select an item for TROUBLE_CURSED_ITEMS */
static struct obj *
worst_cursed_item(void)
{
    register struct obj *otmp;

    /* if strained or worse, check for loadstone first */
    if (near_capacity() >= HVY_ENCUMBER) {
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (Cursed_obj(otmp, LOADSTONE))
                return otmp;
    }
    /* weapon takes precedence if it is interfering
       with taking off a ring or putting on a shield */
    if (welded(uwep) && (uright || bimanual(uwep))) { /* weapon */
        otmp = uwep;
    /* gloves come next, due to rings */
    } else if (uarmg && uarmg->cursed) { /* gloves */
        otmp = uarmg;
    /* then shield due to two handed weapons and spells */
    } else if (uarms && uarms->cursed) { /* shield */
        otmp = uarms;
    /* then cloak due to body armor */
    } else if (uarmc && uarmc->cursed) { /* cloak */
        otmp = uarmc;
    } else if (uarm && uarm->cursed) { /* suit */
        otmp = uarm;
    /* if worn helmet of opposite alignment is making you an adherent
       of the current god, he/she/it won't uncurse that for you */
    } else if (uarmh && uarmh->cursed /* helmet */
               && uarmh->otyp != HELM_OF_OPPOSITE_ALIGNMENT) {
        otmp = uarmh;
    } else if (uarmf && uarmf->cursed) { /* boots */
        otmp = uarmf;
    } else if (uarmu && uarmu->cursed) { /* shirt */
        otmp = uarmu;
    } else if (uamul && uamul->cursed) { /* amulet */
        otmp = uamul;
    } else if (uleft && uleft->cursed) { /* left ring */
        otmp = uleft;
    } else if (uright && uright->cursed) { /* right ring */
        otmp = uright;
    } else if (ublindf && ublindf->cursed) { /* eyewear */
        otmp = ublindf; /* must be non-blinding lenses */
    /* if weapon wasn't handled above, do it now */
    } else if (welded(uwep)) { /* weapon */
        otmp = uwep;
    /* active secondary weapon even though it isn't welded */
    } else if (uswapwep && uswapwep->cursed && u.twoweap) {
        otmp = uswapwep;
    /* all worn items ought to be handled by now */
    } else {
        for (otmp = g.invent; otmp; otmp = otmp->nobj) {
            if (!otmp->cursed)
                continue;
            if (otmp->otyp == LOADSTONE || confers_luck(otmp))
                break;
        }
    }
    return otmp;
}

static void
fix_curse_trouble(struct obj *otmp, const char *what)
{
    if (!otmp) {
        impossible("fix_curse_trouble: nothing to uncurse.");
        return;
    }
    if (otmp == uarmg && Glib) {
        make_glib(0);
        Your("%s are no longer slippery.", gloves_simple_name(uarmg));
        if (!otmp->cursed)
            return;
    }
    if (!Blind || (otmp == ublindf && Blindfolded_only)) {
        pline("%s %s.",
                what ? what : (const char *) Yobjnam2(otmp, "softly glow"),
                hcolor(NH_AMBER));
        iflags.last_msg = PLNMSG_OBJ_GLOWS;
        otmp->bknown = !Hallucination; /* ok to skip set_bknown() */
    }
    uncurse(otmp);
    update_inventory();
}

static void
fix_worst_trouble(int trouble)
{
    int i;
    struct obj *otmp = 0;
    const char *what = (const char *) 0;
    static NEARDATA const char leftglow[] = "Your left ring softly glows",
                               rightglow[] = "Your right ring softly glows";

    switch (trouble) {
    case TROUBLE_STONED:
        make_stoned(0L, "You feel more limber.", 0, (char *) 0);
        break;
    case TROUBLE_SLIMED:
        make_slimed(0L, "The slime disappears.");
        break;
    case TROUBLE_STRANGLED:
        if (uamul && uamul->otyp == AMULET_OF_STRANGULATION) {
            Your("amulet vanishes!");
            useup(uamul);
        }
        You("can breathe again.");
        Strangled = 0;
        g.context.botl = 1;
        break;
    case TROUBLE_LAVA:
        You("are back on solid ground.");
        /* teleport should always succeed, but if not, just untrap them */
        if (!safe_teleds(FALSE))
            reset_utrap(TRUE);
        break;
    case TROUBLE_STARVING:
        /* temporarily lost strength recovery now handled by init_uhunger() */
        /*FALLTHRU*/
    case TROUBLE_HUNGRY:
        Your("%s feels content.", body_part(STOMACH));
        init_uhunger();
        g.context.botl = 1;
        break;
    case TROUBLE_SICK:
        You_feel("better.");
        make_sick(0L, (char *) 0, FALSE, SICK_ALL);
        break;
    case TROUBLE_REGION:
        /* stinking cloud, with hero vulnerable to HP loss */
        region_safety();
        break;
    case TROUBLE_HIT:
        /* "fix all troubles" will keep trying if hero has
           5 or less hit points, so make sure they're always
           boosted to be more than that */
        You_feel("much better.");
        if (Upolyd) {
            u.mhmax += rnd(5);
            if (u.mhmax <= 5)
                u.mhmax = 5 + 1;
            u.mh = u.mhmax;
        }
        if (u.uhpmax < u.ulevel * 5 + 11)
            u.uhpmax += rnd(5);
        if (u.uhpmax <= 5)
            u.uhpmax = 5 + 1;
        if (u.uhpmax > u.uhppeak)
            u.uhppeak = u.uhpmax;
        u.uhp = u.uhpmax;
        g.context.botl = 1;
        break;
    case TROUBLE_COLLAPSING:
        /* override Fixed_abil; uncurse that if feasible */
        You_feel("%sstronger.",
                 (AMAX(A_STR) - ABASE(A_STR) > 6) ? "much " : "");
        ABASE(A_STR) = AMAX(A_STR);
        g.context.botl = 1;
        if (Fixed_abil) {
            if ((otmp = stuck_ring(uleft, RIN_SUSTAIN_ABILITY)) != 0) {
                if (otmp == uleft)
                    what = leftglow;
            } else if ((otmp = stuck_ring(uright, RIN_SUSTAIN_ABILITY)) != 0) {
                if (otmp == uright)
                    what = rightglow;
            }
            if (otmp) {
                fix_curse_trouble(otmp, what);
                break;
            }
        }
        break;
    case TROUBLE_STUCK_IN_WALL:
        /* no control, but works on no-teleport levels */
        if (safe_teleds(FALSE)) {
            Your("surroundings change.");
        } else {
            /* safe_teleds() couldn't find a safe place; perhaps the
               level is completely full.  As a last resort, confer
               intrinsic wall/rock-phazing.  Hero might get stuck
               again fairly soon....
               Without something like this, fix_all_troubles can get
               stuck in an infinite loop trying to fix STUCK_IN_WALL
               and repeatedly failing. */
            set_itimeout(&HPasses_walls, (long) (d(4, 4) + 4)); /* 8..20 */
            /* how else could you move between packed rocks or among
               lattice forming "solid" rock? */
            You_feel("much slimmer.");
        }
        break;
    case TROUBLE_CURSED_LEVITATION:
        if (Cursed_obj(uarmf, LEVITATION_BOOTS)) {
            otmp = uarmf;
        } else if ((otmp = stuck_ring(uleft, RIN_LEVITATION)) != 0) {
            if (otmp == uleft)
                what = leftglow;
        } else if ((otmp = stuck_ring(uright, RIN_LEVITATION)) != 0) {
            if (otmp == uright)
                what = rightglow;
        }
        fix_curse_trouble(otmp, what);
        break;
    case TROUBLE_UNUSEABLE_HANDS:
        if (welded(uwep)) {
            otmp = uwep;
            fix_curse_trouble(otmp, what);
            break;
        }
        if (Upolyd && nohands(g.youmonst.data)) {
            if (!Unchanging) {
                Your("shape becomes uncertain.");
                rehumanize(); /* "You return to {normal} form." */
            } else if ((otmp = unchanger()) != 0 && otmp->cursed) {
                /* otmp is an amulet of unchanging */
                fix_curse_trouble(otmp, what);
                break;
            }
        }
        if (nohands(g.youmonst.data) || !freehand())
            impossible("fix_worst_trouble: couldn't cure hands.");
        break;
    case TROUBLE_CURSED_BLINDFOLD:
        otmp = ublindf;
        fix_curse_trouble(otmp, what);
        break;
    case TROUBLE_LYCANTHROPE:
        you_unwere(TRUE);
        break;
    /*
     */
    case TROUBLE_PUNISHED:
        Your("chain disappears.");
        if (u.utrap && u.utraptype == TT_BURIEDBALL)
            buried_ball_to_freedom();
        else
            unpunish();
        break;
    case TROUBLE_FUMBLING:
        if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING))
            otmp = uarmg;
        else if (Cursed_obj(uarmf, FUMBLE_BOOTS))
            otmp = uarmf;
        fix_curse_trouble(otmp, what);
        break;
    case TROUBLE_CURSED_ITEMS:
        otmp = worst_cursed_item();
        if (otmp == uright)
            what = rightglow;
        else if (otmp == uleft)
            what = leftglow;
        fix_curse_trouble(otmp, what);
        break;
    case TROUBLE_POISONED:
        /* override Fixed_abil; ignore items which confer that */
        if (Hallucination)
            pline("There's a tiger in your tank.");
        else
            You_feel("in good health again.");
        for (i = 0; i < A_MAX; i++) {
            if (ABASE(i) < AMAX(i)) {
                ABASE(i) = AMAX(i);
                g.context.botl = 1;
            }
        }
        (void) encumber_msg();
        break;
    case TROUBLE_BLIND: { /* handles deafness as well as blindness */
        char msgbuf[BUFSZ];
        const char *eyes = body_part(EYE);
        boolean cure_deaf = (HDeaf & TIMEOUT) ? TRUE : FALSE;

        msgbuf[0] = '\0';
        if (Blinded) {
            if (eyecount(g.youmonst.data) != 1)
                eyes = makeplural(eyes);
            Sprintf(msgbuf, "Your %s %s better", eyes, vtense(eyes, "feel"));
            u.ucreamed = 0;
            make_blinded(0L, FALSE);
        }
        if (cure_deaf) {
            make_deaf(0L, FALSE);
            if (!Deaf)
                Sprintf(eos(msgbuf), "%s can hear again",
                        !*msgbuf ? "You" : " and you");
        }
        if (*msgbuf)
            pline("%s.", msgbuf);
        break;
    }
    case TROUBLE_WOUNDED_LEGS:
        heal_legs(0);
        break;
    case TROUBLE_STUNNED:
        make_stunned(0L, TRUE);
        break;
    case TROUBLE_CONFUSED:
        make_confused(0L, TRUE);
        break;
    case TROUBLE_HALLUCINATION:
        pline("Looks like you are back in Kansas.");
        (void) make_hallucinated(0L, FALSE, 0L);
        break;
    case TROUBLE_SADDLE:
        otmp = which_armor(u.usteed, W_SADDLE);
        if (!Blind) {
            pline("%s %s.", Yobjnam2(otmp, "softly glow"), hcolor(NH_AMBER));
            set_bknown(otmp, 1);
        }
        uncurse(otmp);
        break;
    }
}

/* "I am sometimes shocked by... the nuns who never take a bath without
 * wearing a bathrobe all the time.  When asked why, since no man can see them,
 * they reply 'Oh, but you forget the good God'.  Apparently they conceive of
 * the Deity as a Peeping Tom, whose omnipotence enables Him to see through
 * bathroom walls, but who is foiled by bathrobes." --Bertrand Russell, 1943
 * Divine wrath, dungeon walls, and armor follow the same principle.
 */
static void
god_zaps_you(aligntyp resp_god)
{
    if (u.uswallow) {
        pline(
          "Suddenly a bolt of lightning comes down at you from the heavens!");
        pline("It strikes %s!", mon_nam(u.ustuck));
        if (!resists_elec(u.ustuck)) {
            pline("%s fries to a crisp!", Monnam(u.ustuck));
            /* Yup, you get experience.  It takes guts to successfully
             * pull off this trick on your god, anyway.
             * Other credit/blame applies (luck or alignment adjustments),
             * but not direct kill count (pacifist conduct).
             */
            xkilled(u.ustuck, XKILL_NOMSG | XKILL_NOCONDUCT);
        } else
            pline("%s seems unaffected.", Monnam(u.ustuck));
    } else {
        pline("Suddenly, a bolt of lightning strikes you!");
        if (Reflecting) {
            shieldeff(u.ux, u.uy);
            if (Blind)
                pline("For some reason you're unaffected.");
            else
                (void) ureflects("%s reflects from your %s.", "It");
            monstseesu(M_SEEN_REFL);
        } else if (Shock_resistance) {
            shieldeff(u.ux, u.uy);
            pline("It seems not to affect you.");
            monstseesu(M_SEEN_ELEC);
        } else
            fry_by_god(resp_god, FALSE);
    }

    pline("%s is not deterred...", align_gname(resp_god));
    if (u.uswallow) {
        pline("A wide-angle disintegration beam aimed at you hits %s!",
              mon_nam(u.ustuck));
        if (!resists_disint(u.ustuck)) {
            pline("%s disintegrates into a pile of dust!", Monnam(u.ustuck));
            xkilled(u.ustuck, XKILL_NOMSG | XKILL_NOCORPSE | XKILL_NOCONDUCT);
        } else
            pline("%s seems unaffected.", Monnam(u.ustuck));
    } else {
        pline("A wide-angle disintegration beam hits you!");

        /* disintegrate shield and body armor before disintegrating
         * the impudent mortal, like black dragon breath -3.
         */
        if (uarms && !(EReflecting & W_ARMS)
            && !(EDisint_resistance & W_ARMS))
            (void) destroy_arm(uarms);
        if (uarmc && !(EReflecting & W_ARMC)
            && !(EDisint_resistance & W_ARMC))
            (void) destroy_arm(uarmc);
        if (uarm && !(EReflecting & W_ARM) && !(EDisint_resistance & W_ARM)
            && !uarmc)
            (void) destroy_arm(uarm);
        if (uarmu && !uarm && !uarmc)
            (void) destroy_arm(uarmu);
        if (!Disint_resistance) {
            fry_by_god(resp_god, TRUE);
        } else {
            You("bask in its %s glow for a minute...", NH_BLACK);
            godvoice(resp_god, "I believe it not!");
            monstseesu(M_SEEN_DISINT);
        }
        if (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)) {
            /* one more try for high altars */
            verbalize("Thou cannot escape my wrath, mortal!");
            summon_minion(resp_god, FALSE);
            summon_minion(resp_god, FALSE);
            summon_minion(resp_god, FALSE);
            verbalize("Destroy %s, my servants!", uhim());
        }
    }
}

static void
fry_by_god(aligntyp resp_god, boolean via_disintegration)
{
    You("%s!", !via_disintegration ? "fry to a crisp"
                                   : "disintegrate into a pile of dust");
    g.killer.format = KILLED_BY;
    Sprintf(g.killer.name, "the wrath of %s", align_gname(resp_god));
    done(DIED);
}

static void
angrygods(aligntyp resp_god)
{
    int maxanger, new_ublesscnt;

    if (Inhell)
        resp_god = A_NONE;
    u.ublessed = 0; /* lose divine protection */

    /* changed from tmp = u.ugangr + abs (u.uluck) -- rph */
    /* added test for alignment diff -dlc */
    if (resp_god != u.ualign.type)
        maxanger = u.ualign.record / 2 + (Luck > 0 ? -Luck / 3 : -Luck);
    else
        maxanger = 3 * u.ugangr + ((Luck > 0 || u.ualign.record >= STRIDENT)
                                   ? -Luck / 3
                                   : -Luck);
    if (maxanger < 1)
        maxanger = 1; /* possible if bad align & good luck */
    else if (maxanger > 15)
        maxanger = 15; /* be reasonable */

    switch (rn2(maxanger)) {
    case 0:
    case 1:
        You_feel("that %s is %s.", align_gname(resp_god),
                 Hallucination ? "bummed" : "displeased");
        break;
    case 2:
    case 3:
        godvoice(resp_god, (char *) 0);
        pline("\"Thou %s, %s.\"",
              (ugod_is_angry() && resp_god == u.ualign.type)
                  ? "hast strayed from the path"
                  : "art arrogant",
              g.youmonst.data->mlet == S_HUMAN ? "mortal" : "creature");
        verbalize("Thou must relearn thy lessons!");
        (void) adjattrib(A_WIS, -1, FALSE);
        losexp((char *) 0);
        break;
    case 6:
        if (!Punished) {
            gods_angry(resp_god);
            punish((struct obj *) 0);
            break;
        } /* else fall thru */
    case 4:
    case 5:
        gods_angry(resp_god);
        if (!Blind && !Antimagic)
            pline("%s glow surrounds you.", An(hcolor(NH_BLACK)));
        rndcurse();
        break;
    case 7:
    case 8:
        godvoice(resp_god, (char *) 0);
        verbalize("Thou durst %s me?",
                  (on_altar() && (a_align(u.ux, u.uy) != resp_god))
                      ? "scorn"
                      : "call upon");
        /* [why isn't this using verbalize()?] */
        pline("\"Then die, %s!\"",
              (g.youmonst.data->mlet == S_HUMAN) ? "mortal" : "creature");
        summon_minion(resp_god, FALSE);
        break;

    default:
        gods_angry(resp_god);
        god_zaps_you(resp_god);
        break;
    }
    /* even though this might not be in response to prayer, set pray timer */
    new_ublesscnt = rnz(300);
    if (new_ublesscnt > u.ublesscnt)
        u.ublesscnt = new_ublesscnt;
    return;
}

/* helper to print "str appears at your feet", or appropriate */
static void
at_your_feet(const char *str)
{
    if (Blind)
        str = Something;
    if (u.uswallow) {
        /* barrier between you and the floor */
        pline("%s %s into %s %s.", str, vtense(str, "drop"),
              s_suffix(mon_nam(u.ustuck)), mbodypart(u.ustuck, STOMACH));
    } else {
        pline("%s %s %s your %s!", str,
              vtense(str, Blind ? "land" : "appear"),
              Levitation ? "beneath" : "at",
              makeplural(body_part(FOOT)));
    }
}

static void
gcrownu(void)
{
    struct obj *obj;
    const char *what;
    boolean already_exists, in_hand;
    short class_gift;
#define ok_wep(o) ((o) && ((o)->oclass == WEAPON_CLASS || is_weptool(o)))

    HSee_invisible |= FROMOUTSIDE;
    HFire_resistance |= FROMOUTSIDE;
    HCold_resistance |= FROMOUTSIDE;
    HShock_resistance |= FROMOUTSIDE;
    HSleep_resistance |= FROMOUTSIDE;
    HPoison_resistance |= FROMOUTSIDE;
    godvoice(u.ualign.type, (char *) 0);

    class_gift = STRANGE_OBJECT;
    /* 3.3.[01] had this in the A_NEUTRAL case,
       preventing chaotic wizards from receiving a spellbook */
    if (Role_if(PM_WIZARD)
        && (!uwep || (uwep->oartifact != ART_VORPAL_BLADE
                      && uwep->oartifact != ART_STORMBRINGER))
        && !carrying(SPE_FINGER_OF_DEATH)) {
        class_gift = SPE_FINGER_OF_DEATH;
    } else if (Role_if(PM_MONK) && (!uwep || !uwep->oartifact)
               && !carrying(SPE_RESTORE_ABILITY)) {
        /* monks rarely wield a weapon */
        class_gift = SPE_RESTORE_ABILITY;
    }

    obj = ok_wep(uwep) ? uwep : 0;
    already_exists = in_hand = FALSE; /* lint suppression */
    switch (u.ualign.type) {
    case A_LAWFUL:
        u.uevent.uhand_of_elbereth = 1;
        verbalize("I crown thee...  The Hand of Elbereth!");
        livelog_printf(LL_DIVINEGIFT,
                       "was crowned \"The Hand of Elbereth\" by %s",
                       u_gname());
        break;
    case A_NEUTRAL:
        u.uevent.uhand_of_elbereth = 2;
        in_hand = (uwep && uwep->oartifact == ART_VORPAL_BLADE);
        already_exists = exist_artifact(LONG_SWORD,
                                        artiname(ART_VORPAL_BLADE));
        verbalize("Thou shalt be my Envoy of Balance!");
        livelog_printf(LL_DIVINEGIFT, "became %s Envoy of Balance",
                       s_suffix(u_gname()));
        break;
    case A_CHAOTIC:
        u.uevent.uhand_of_elbereth = 3;
        in_hand = (uwep && uwep->oartifact == ART_STORMBRINGER);
        already_exists = exist_artifact(RUNESWORD, artiname(ART_STORMBRINGER));
        what = (((already_exists && !in_hand) || class_gift != STRANGE_OBJECT)
                ? "take lives"
                : "steal souls");
        verbalize("Thou art chosen to %s for My Glory!", what);
        livelog_printf(LL_DIVINEGIFT, "was chosen to %s for the Glory of %s",
                       what, u_gname());
        break;
    }

    if (objects[class_gift].oc_class == SPBOOK_CLASS) {
        char bbuf[BUFSZ];

        obj = mksobj(class_gift, TRUE, FALSE);
        /* get book type before dropping (don't think that could destroy
           the book because we need to be on an altar in order to become
           crowned, but be paranoid about it) */
        Strcpy(bbuf, actualoname(obj)); /* for livelog; "spellbook of <foo>"
                                         * even if hero doesn't know book */
        bless(obj);
        obj->bknown = 1; /* ok to skip set_bknown() */
        obj->dknown = 1;
        at_your_feet(upstart(ansimpleoname(obj)));
        dropy(obj);
        u.ugifts++;
        /* not an artifact, but treat like one for this situation;
           classify as a spoiler in case player hasn't IDed the book yet */
        livelog_printf(LL_DIVINEGIFT | LL_ARTIFACT | LL_SPOILER,
                       "was bestowed with %s", bbuf);

        /* when getting a new book for known spell, enhance
           currently wielded weapon rather than the book */
        if (known_spell(class_gift) != spe_Unknown && ok_wep(uwep))
            obj = uwep; /* to be blessed,&c */
    }

    switch (u.ualign.type) {
    case A_LAWFUL:
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && obj->otyp == LONG_SWORD && !obj->oartifact) {
            char lbuf[BUFSZ];

            Strcpy(lbuf, simpleonames(obj)); /* before transformation */
            if (!Blind)
                Your("sword shines brightly for a moment.");
            obj = oname(obj, artiname(ART_EXCALIBUR),
                        ONAME_GIFT | ONAME_KNOW_ARTI);
            if (obj && obj->oartifact == ART_EXCALIBUR) {
                u.ugifts++;
                livelog_printf(LL_DIVINEGIFT | LL_ARTIFACT,
                               "had %s wielded %s transformed into %s",
                               uhis(), lbuf, artiname(ART_EXCALIBUR));
            }
        }
        /* acquire Excalibur's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_LONG_SWORD);
        if (obj && obj->oartifact == ART_EXCALIBUR)
            discover_artifact(ART_EXCALIBUR);
        break;
    case A_NEUTRAL:
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && in_hand) {
            Your("%s goes snicker-snack!", xname(obj));
            obj->dknown = 1;
        } else if (!already_exists) {
            obj = mksobj(LONG_SWORD, FALSE, FALSE);
            obj = oname(obj, artiname(ART_VORPAL_BLADE),
                        ONAME_GIFT | ONAME_KNOW_ARTI);
            obj->spe = 1;
            at_your_feet("A sword");
            dropy(obj);
            u.ugifts++;
            livelog_printf(LL_DIVINEGIFT | LL_ARTIFACT,
                           "was bestowed with %s",
                           artiname(ART_VORPAL_BLADE));
        }
        /* acquire Vorpal Blade's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_LONG_SWORD);
        if (obj && obj->oartifact == ART_VORPAL_BLADE)
            discover_artifact(ART_VORPAL_BLADE);
        break;
    case A_CHAOTIC: {
        char swordbuf[BUFSZ];

        Sprintf(swordbuf, "%s sword", hcolor(NH_BLACK));
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && in_hand) {
            Your("%s hums ominously!", swordbuf);
            obj->dknown = 1;
        } else if (!already_exists) {
            obj = mksobj(RUNESWORD, FALSE, FALSE);
            obj = oname(obj, artiname(ART_STORMBRINGER),
                        ONAME_GIFT | ONAME_KNOW_ARTI);
            obj->spe = 1;
            at_your_feet(An(swordbuf));
            dropy(obj);
            u.ugifts++;
            livelog_printf(LL_DIVINEGIFT | LL_ARTIFACT,
                           "was bestowed with %s",
                           artiname(ART_STORMBRINGER));
        }
        /* acquire Stormbringer's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_BROAD_SWORD);
        if (obj && obj->oartifact == ART_STORMBRINGER)
            discover_artifact(ART_STORMBRINGER);
        break;
    }
    default:
        obj = 0; /* lint */
        break;
    }

    /* enhance weapon regardless of alignment or artifact status */
    if (ok_wep(obj)) {
        bless(obj);
        obj->oeroded = obj->oeroded2 = 0;
        obj->oerodeproof = TRUE;
        obj->bknown = obj->rknown = 1; /* ok to skip set_bknown() */
        if (obj->spe < 1)
            obj->spe = 1;
        /* acquire skill in this weapon */
        unrestrict_weapon_skill(weapon_type(obj));
    } else if (class_gift == STRANGE_OBJECT) {
        /* opportunity knocked, but there was nobody home... */
        You_feel("unworthy.");
    }
    update_inventory();

    /* lastly, confer an extra skill slot/credit beyond the
       up-to-29 you can get from gaining experience levels */
    add_weapon_skill(1);
    return;
}

static void
give_spell(void)
{
    struct obj *otmp;
    char spe_let;
    int spe_knowledge, trycnt = u.ulevel + 1;

    /* not yet known spells and forgotten spells are given preference over
       usable ones; also, try to grant spell that hero could gain skill in
       (even though being restricted doesn't prevent learning and casting) */
    otmp = mkobj(SPBOOK_no_NOVEL, TRUE);
    while (--trycnt > 0) {
        if (otmp->otyp != SPE_BLANK_PAPER) {
            if (known_spell(otmp->otyp) <= spe_Unknown
                && !P_RESTRICTED(spell_skilltype(otmp->otyp)))
                break; /* forgotten or not yet known */
        } else {
            /* blank paper is acceptable if not discovered yet or
               if hero has a magic marker to write something on it
               (doesn't matter if marker is out of charges); it will
               become discovered (below) without needing to be read */
            if (!objects[SPE_BLANK_PAPER].oc_name_known
                || carrying(MAGIC_MARKER))
                break;
        }
        otmp->otyp = rnd_class(g.bases[SPBOOK_CLASS], SPE_BLANK_PAPER);
    }
    /*
     * 25% chance of learning the spell directly instead of
     * receiving the book for it, unless it's already well known.
     * The chance is not influenced by whether hero is illiterate.
     */
    if (otmp->otyp != SPE_BLANK_PAPER && !rn2(4)
        && (spe_knowledge = known_spell(otmp->otyp)) != spe_Fresh) {
        /* force_learn_spell() should only return '\0' if the book
           is blank paper or the spell is known and has retention
           of spe_Fresh, so no 'else' case is needed here */
        if ((spe_let = force_learn_spell(otmp->otyp)) != '\0') {
            /* for spellbook class, OBJ_NAME() yields the name of
               the spell rather than "spellbook of <spell-name>" */
            const char *spe_name = OBJ_NAME(objects[otmp->otyp]);

            if (spe_knowledge == spe_Unknown) /* prior to learning */
                /* appending "spell 'a'" seems slightly silly but
                   is similar to "added to your repertoire, as 'a'"
                   and without any spellbook on hand a novice player
                   might not recognize that 'spe_name' is a spell */
                pline("Divine knowledge of %s fills your mind!  Spell '%c'.",
                      spe_name, spe_let);
            else
                Your("knowledge of spell '%c' - %s is %s.",
                     spe_let, spe_name,
                     (spe_knowledge == spe_Forgotten) ? "restored"
                                                      : "refreshed");
        }
        obfree(otmp, (struct obj *) 0); /* discard the book */
    } else {
        otmp->dknown = 1; /* not bknown */
        /* discovering blank paper will make it less likely to
           be given again; small chance to arbitrarily discover
           some other book type without having to read it first */
        if (otmp->otyp == SPE_BLANK_PAPER || !rn2(100))
            makeknown(otmp->otyp);
        bless(otmp);
        at_your_feet(upstart(ansimpleoname(otmp)));
        place_object(otmp, u.ux, u.uy);
        newsym(u.ux, u.uy);
    }
    return;
}

static void
pleased(aligntyp g_align)
{
    /* don't use p_trouble, worst trouble may get fixed while praying */
    int trouble = in_trouble(); /* what's your worst difficulty? */
    int pat_on_head = 0, kick_on_butt;

    You_feel("that %s is %s.", align_gname(g_align),
             (u.ualign.record >= DEVOUT)
                 ? Hallucination ? "pleased as punch" : "well-pleased"
                 : (u.ualign.record >= STRIDENT)
                       ? Hallucination ? "ticklish" : "pleased"
                       : Hallucination ? "full" : "satisfied");

    /* not your deity */
    if (on_altar() && g.p_aligntyp != u.ualign.type) {
        adjalign(-1);
        return;
    } else if (u.ualign.record < 2 && trouble <= 0)
        adjalign(1);

    /*
     * Depending on your luck & align level, the god you prayed to will:
     *  - fix your worst problem if it's major;
     *  - fix all your major problems;
     *  - fix your worst problem if it's minor;
     *  - fix all of your problems;
     *  - do you a gratuitous favor.
     *
     * If you make it to the last category, you roll randomly again
     * to see what they do for you.
     *
     * If your luck is at least 0, then you are guaranteed rescued from
     * your worst major problem.
     */
    if (!trouble && u.ualign.record >= DEVOUT) {
        /* if hero was in trouble, but got better, no special favor */
        if (g.p_trouble == 0)
            pat_on_head = 1;
    } else {
        int action, prayer_luck;
        int tryct = 0;

        /* Negative luck is normally impossible here (can_pray() forces
           prayer failure in that situation), but it's possible for
           Luck to drop during the period of prayer occupation and
           become negative by the time we get here.  [Reported case
           was lawful character whose stinking cloud caused a delayed
           killing of a peaceful human, triggering the "murderer"
           penalty while successful prayer was in progress.  It could
           also happen due to inconvenient timing on Friday 13th, but
           the magnitude there (-1) isn't big enough to cause trouble.]
           We don't bother remembering start-of-prayer luck, just make
           sure it's at least -1 so that Luck+2 is big enough to avoid
           a divide by zero crash when generating a random number.  */
        prayer_luck = max(Luck, -1); /* => (prayer_luck + 2 > 0) */
        action = rn1(prayer_luck + (on_altar() ? 3 + on_shrine() : 2), 1);
        if (!on_altar())
            action = min(action, 3);
        if (u.ualign.record < STRIDENT)
            action = (u.ualign.record > 0 || !rnl(2)) ? 1 : 0;

        switch (min(action, 5)) {
        case 5:
            pat_on_head = 1;
            /*FALLTHRU*/
        case 4:
            do
                fix_worst_trouble(trouble);
            while ((trouble = in_trouble()) != 0);
            break;

        case 3:
            fix_worst_trouble(trouble);
        case 2:
            /* arbitrary number of tries */
            while ((trouble = in_trouble()) > 0 && (++tryct < 10))
                fix_worst_trouble(trouble);
            break;

        case 1:
            if (trouble > 0)
                fix_worst_trouble(trouble);
        case 0:
            break; /* your god blows you off, too bad */
        }
    }

    /* note: can't get pat_on_head unless all troubles have just been
       fixed or there were no troubles to begin with; hallucination
       won't be in effect so special handling for it is superfluous */
    if (pat_on_head)
        switch (rn2((Luck + 6) >> 1)) {
        case 0:
            break;
        case 1:
            if (uwep && (welded(uwep) || uwep->oclass == WEAPON_CLASS
                         || is_weptool(uwep))) {
                char repair_buf[BUFSZ];

                *repair_buf = '\0';
                if (uwep->oeroded || uwep->oeroded2)
                    Sprintf(repair_buf, " and %s now as good as new",
                            otense(uwep, "are"));

                if (uwep->cursed) {
                    if (!Blind) {
                        pline("%s %s%s.", Yobjnam2(uwep, "softly glow"),
                              hcolor(NH_AMBER), repair_buf);
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                    } else
                        You_feel("the power of %s over %s.", u_gname(),
                                 yname(uwep));
                    uncurse(uwep);
                    uwep->bknown = 1; /* ok to bypass set_bknown() */
                    *repair_buf = '\0';
                } else if (!uwep->blessed) {
                    if (!Blind) {
                        pline("%s with %s aura%s.",
                              Yobjnam2(uwep, "softly glow"),
                              an(hcolor(NH_LIGHT_BLUE)), repair_buf);
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                    } else
                        You_feel("the blessing of %s over %s.", u_gname(),
                                 yname(uwep));
                    bless(uwep);
                    uwep->bknown = 1; /* ok to bypass set_bknown() */
                    *repair_buf = '\0';
                }

                /* fix any rust/burn/rot damage, but don't protect
                   against future damage */
                if (uwep->oeroded || uwep->oeroded2) {
                    uwep->oeroded = uwep->oeroded2 = 0;
                    /* only give this message if we didn't just bless
                       or uncurse (which has already given a message) */
                    if (*repair_buf)
                        pline("%s as good as new!",
                              Yobjnam2(uwep, Blind ? "feel" : "look"));
                }
                update_inventory();
            }
            break;
        case 3:
            /* takes 2 hints to get the music to enter the stronghold;
               skip if you've solved it via mastermind or destroyed the
               drawbridge (both set uopened_dbridge) or if you've already
               travelled past the Valley of the Dead (gehennom_entered) */
            if (!u.uevent.uopened_dbridge && !u.uevent.gehennom_entered) {
                if (u.uevent.uheard_tune < 1) {
                    godvoice(g_align, (char *) 0);
                    verbalize("Hark, %s!", (g.youmonst.data->mlet == S_HUMAN)
                                               ? "mortal"
                                               : "creature");
                    verbalize(
                       "To enter the castle, thou must play the right tune!");
                    u.uevent.uheard_tune++;
                    break;
                } else if (u.uevent.uheard_tune < 2) {
                    You_hear("a divine music...");
                    pline("It sounds like:  \"%s\".", g.tune);
                    u.uevent.uheard_tune++;
                    record_achievement(ACH_TUNE);
                    break;
                }
            }
            /*FALLTHRU*/
        case 2:
            if (!Blind)
                You("are surrounded by %s glow.", an(hcolor(NH_GOLDEN)));
            /* if any levels have been lost (and not yet regained),
               treat this effect like blessed full healing */
            if (u.ulevel < u.ulevelmax) {
                u.ulevelmax -= 1; /* see potion.c */
                pluslvl(FALSE);
            } else {
                u.uhpmax += 5;
                if (u.uhpmax > u.uhppeak)
                    u.uhppeak = u.uhpmax;
                if (Upolyd)
                    u.mhmax += 5;
            }
            u.uhp = u.uhpmax;
            if (Upolyd)
                u.mh = u.mhmax;
            if (ABASE(A_STR) < AMAX(A_STR)) {
                ABASE(A_STR) = AMAX(A_STR);
                g.context.botl = 1; /* before potential message */
                (void) encumber_msg();
            }
            if (u.uhunger < 900)
                init_uhunger();
            /* luck couldn't have been negative at start of prayer because
               the prayer would have failed, but might have been decremented
               due to a timed event (delayed death of peaceful monster hit
               by hero-created stinking cloud) during the praying interval */
            if (u.uluck < 0)
                u.uluck = 0;
            /* superfluous; if hero was blinded we'd be handling trouble
               rather than issuing a pat-on-head */
            u.ucreamed = 0;
            make_blinded(0L, TRUE);
            g.context.botl = 1;
            break;
        case 4: {
            register struct obj *otmp;
            int any = 0;

            if (Blind)
                You_feel("the power of %s.", u_gname());
            else
                You("are surrounded by %s aura.", an(hcolor(NH_LIGHT_BLUE)));
            for (otmp = g.invent; otmp; otmp = otmp->nobj) {
                if (otmp->cursed
                    && (otmp != uarmh /* [see worst_cursed_item()] */
                        || uarmh->otyp != HELM_OF_OPPOSITE_ALIGNMENT)) {
                    if (!Blind) {
                        pline("%s %s.", Yobjnam2(otmp, "softly glow"),
                              hcolor(NH_AMBER));
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                        otmp->bknown = 1; /* ok to bypass set_bknown() */
                        ++any;
                    }
                    uncurse(otmp);
                }
            }
            if (any)
                update_inventory();
            break;
        }
        case 5: {
            static NEARDATA const char msg[] =
                "\"and thus I grant thee the gift of %s!\"";

            godvoice(u.ualign.type,
                     "Thou hast pleased me with thy progress,");
            if (!(HTelepat & INTRINSIC)) {
                HTelepat |= FROMOUTSIDE;
                pline(msg, "Telepathy");
                if (Blind)
                    see_monsters();
            } else if (!(HFast & INTRINSIC)) {
                HFast |= FROMOUTSIDE;
                pline(msg, "Speed");
            } else if (!(HStealth & INTRINSIC)) {
                HStealth |= FROMOUTSIDE;
                pline(msg, "Stealth");
            } else {
                if (!(HProtection & INTRINSIC)) {
                    HProtection |= FROMOUTSIDE;
                    if (!u.ublessed)
                        u.ublessed = rn1(3, 2);
                } else
                    u.ublessed++;
                pline(msg, "my protection");
            }
            verbalize("Use it wisely in my name!");
            break;
        }
        case 7:
        case 8:
            if (u.ualign.record >= PIOUS && !u.uevent.uhand_of_elbereth) {
                gcrownu();
                break;
            }
            /*FALLTHRU*/
        case 6:
            give_spell();
            break;
        default:
            impossible("Confused deity!");
            break;
        }

    u.ublesscnt = rnz(350);
    kick_on_butt = u.uevent.udemigod ? 1 : 0;
    if (u.uevent.uhand_of_elbereth)
        kick_on_butt++;
    if (kick_on_butt)
        u.ublesscnt += kick_on_butt * rnz(1000);

    /* Avoid games that go into infinite loops of copy-pasted commands
       with no human interaction; this is a DoS vector against the
       computer running NetHack.  Once the turn counter is over 100000,
       every additional 100 turns increases the prayer timeout by 1,
       thus eventually hunger prayers will fail and some other source
       of nutrition will be required.  The increase gets throttled if
       it ever reaches 32K so that configurations using 16-bit ints are
       still viable. */
    if (g.moves > 100000L) {
        long incr = (g.moves - 100000L) / 100L,
             largest_ublesscnt_incr = (long) (LARGEST_INT - u.ublesscnt);

        if (incr > largest_ublesscnt_incr)
            incr = largest_ublesscnt_incr;
        u.ublesscnt += (int) incr;
    }

    return;
}

/* either blesses or curses water on the altar,
 * returns true if it found any water here.
 */
static boolean
water_prayer(boolean bless_water)
{
    register struct obj *otmp;
    register long changed = 0;
    boolean other = FALSE, bc_known = !(Blind || Hallucination);

    for (otmp = g.level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
        /* turn water into (un)holy water */
        if (otmp->otyp == POT_WATER
            && (bless_water ? !otmp->blessed : !otmp->cursed)) {
            otmp->blessed = bless_water;
            otmp->cursed = !bless_water;
            otmp->bknown = bc_known; /* ok to bypass set_bknown() */
            changed += otmp->quan;
        } else if (otmp->oclass == POTION_CLASS)
            other = TRUE;
    }
    if (!Blind && changed) {
        pline("%s potion%s on the altar glow%s %s for a moment.",
              ((other && changed > 1L) ? "Some of the"
                                       : (other ? "One of the" : "The")),
              ((other || changed > 1L) ? "s" : ""), (changed > 1L ? "" : "s"),
              (bless_water ? hcolor(NH_LIGHT_BLUE) : hcolor(NH_BLACK)));
    }
    return (boolean) (changed > 0L);
}

static void
godvoice(aligntyp g_align, const char *words)
{
    const char *quot = "";

    if (words)
        quot = "\"";
    else
        words = "";

    pline_The("voice of %s %s: %s%s%s", align_gname(g_align),
              godvoices[rn2(SIZE(godvoices))], quot, words, quot);
}

static void
gods_angry(aligntyp g_align)
{
    godvoice(g_align, "Thou hast angered me.");
}

/* The g_align god is upset with you. */
static void
gods_upset(aligntyp g_align)
{
    if (g_align == u.ualign.type)
        u.ugangr++;
    else if (u.ugangr)
        u.ugangr--;
    angrygods(g_align);
}

static void
consume_offering(struct obj *otmp)
{
    if (Hallucination)
        switch (rn2(3)) {
        case 0:
            Your("sacrifice sprouts wings and a propeller and roars away!");
            break;
        case 1:
            Your("sacrifice puffs up, swelling bigger and bigger, and pops!");
            break;
        case 2:
            Your(
     "sacrifice collapses into a cloud of dancing particles and fades away!");
            break;
        }
    else if (Blind && u.ualign.type == A_LAWFUL)
        Your("sacrifice disappears!");
    else
        Your("sacrifice is consumed in a %s!",
             u.ualign.type == A_LAWFUL
                ? "flash of light"
                : u.ualign.type == A_NEUTRAL
                    ? "cloud of smoke"
                    : "burst of flame");
    if (carried(otmp))
        useup(otmp);
    else
        useupf(otmp, 1L);
    exercise(A_WIS, TRUE);
}

/* feedback when attempting to offer the Amulet on a "low altar" (not one of
   the high altars in the temples on the Astral Plane or Moloch's Sanctum) */
static void
offer_too_soon(aligntyp altaralign)
{
    if (altaralign == A_NONE && Inhell) {
        /* offering on an unaligned altar in Gehennom;
           hero has left Moloch's Sanctum (caller handles that)
           so is in the process of getting away with the Amulet;
           for any unaligned altar outside of Gehennom, give the
           "you feel ashamed" feedback for wrong alignment below */
        gods_upset(A_NONE); /* Moloch becomes angry */
        return;
    }
    You_feel("%s.", Hallucination
                    ? "homesick"
                    /* if on track, give a big hint */
                    : (altaralign == u.ualign.type)
                        ? "an urge to return to the surface"
                        /* else headed towards celestial disgrace */
                        : "ashamed");
}

static void
desecrate_high_altar(aligntyp altaralign)
{
    /*
     * REAL BAD NEWS!!! High altars cannot be converted.  Even an attempt
     * gets the god who owns it truly pissed off.
     */
    You_feel("the air around you grow charged...");
    pline("Suddenly, you realize that %s has noticed you...", a_gname());
    godvoice(altaralign,
                "So, mortal!  You dare desecrate my High Temple!");
    /* Throw everything we have at the player */
    god_zaps_you(altaralign);
}

/* the #offer command - sacrifice something to the gods */
int
dosacrifice(void)
{
    static NEARDATA const char
        cloud_of_smoke[] = "A cloud of %s smoke surrounds you...";
    register struct obj *otmp;
    int value = 0, pm;
    boolean highaltar;
    aligntyp altaralign = a_align(u.ux, u.uy);

    if (!on_altar() || u.uswallow) {
        You("are not standing on an altar.");
        return ECMD_OK;
    }
    highaltar = (levl[u.ux][u.uy].altarmask & AM_SANCTUM);

    otmp = floorfood("sacrifice", 1);
    if (!otmp)
        return ECMD_OK;
    /*
     * Was based on nutritional value and aging behavior (< 50 moves).
     * Sacrificing a food ration got you max luck instantly, making the
     * gods as easy to please as an angry dog!
     *
     * Now only accepts corpses, based on the game's evaluation of their
     * toughness.  Human and pet sacrifice, as well as sacrificing unicorns
     * of your alignment, is strongly discouraged.
     */
#define MAXVALUE 24 /* Highest corpse value (besides Wiz) */

    if (otmp->otyp == CORPSE) {
        register struct permonst *ptr = &mons[otmp->corpsenm];
        struct monst *mtmp;

        /* KMH, conduct */
        if (!u.uconduct.gnostic++)
            livelog_printf(LL_CONDUCT,
                           "rejected atheism by offering %s on an altar of %s",
                           corpse_xname(otmp, (const char *) 0, CXN_ARTICLE),
                           a_gname());

        /* you're handling this corpse, even if it was killed upon the altar
         */
        feel_cockatrice(otmp, TRUE);
        if (rider_corpse_revival(otmp, FALSE))
            return ECMD_TIME;

        if (otmp->corpsenm == PM_ACID_BLOB
            || (g.moves <= peek_at_iced_corpse_age(otmp) + 50)) {
            value = mons[otmp->corpsenm].difficulty + 1;
            if (otmp->oeaten)
                value = eaten_stat(value, otmp);
        }

        /* same race or former pet results apply even if the corpse is
           too old (value==0) */
        if (your_race(ptr)) {
            if (is_demon(g.youmonst.data)) {
                You("find the idea very satisfying.");
                exercise(A_WIS, TRUE);
            } else if (u.ualign.type != A_CHAOTIC) {
                pline("You'll regret this infamous offense!");
                exercise(A_WIS, FALSE);
            }

            if (highaltar
                && (altaralign != A_CHAOTIC || u.ualign.type != A_CHAOTIC)) {
                desecrate_high_altar(altaralign);
                return ECMD_TIME;
            } else if (altaralign != A_CHAOTIC && altaralign != A_NONE) {
                /* curse the lawful/neutral altar */
                pline_The("altar is stained with %s blood.", g.urace.adj);
                levl[u.ux][u.uy].altarmask = AM_CHAOTIC;
                newsym(u.ux, u.uy); /* in case Invisible to self */
                angry_priest();
            } else {
                struct monst *dmon;
                const char *demonless_msg;

                /* Human sacrifice on a chaotic or unaligned altar */
                /* is equivalent to demon summoning */
                if (altaralign == A_CHAOTIC && u.ualign.type != A_CHAOTIC) {
                    pline(
                    "The blood floods the altar, which vanishes in %s cloud!",
                          an(hcolor(NH_BLACK)));
                    levl[u.ux][u.uy].typ = ROOM;
                    levl[u.ux][u.uy].altarmask = 0;
                    newsym(u.ux, u.uy);
                    angry_priest();
                    demonless_msg = "cloud dissipates";
                } else {
                    /* either you're chaotic or altar is Moloch's or both */
                    pline_The("blood covers the altar!");
                    change_luck(altaralign == A_NONE ? -2 : 2);
                    demonless_msg = "blood coagulates";
                }
                if ((pm = dlord(altaralign)) != NON_PM
                    && (dmon = makemon(&mons[pm], u.ux, u.uy, MM_NOMSG))
                           != 0) {
                    char dbuf[BUFSZ];

                    Strcpy(dbuf, a_monnam(dmon));
                    if (!strcmpi(dbuf, "it"))
                        Strcpy(dbuf, "something dreadful");
                    else
                        dmon->mstrategy &= ~STRAT_APPEARMSG;
                    You("have summoned %s!", dbuf);
                    if (sgn(u.ualign.type) == sgn(dmon->data->maligntyp))
                        dmon->mpeaceful = TRUE;
                    You("are terrified, and unable to move.");
                    nomul(-3);
                    g.multi_reason = "being terrified of a demon";
                    g.nomovemsg = 0;
                } else
                    pline_The("%s.", demonless_msg);
            }

            if (u.ualign.type != A_CHAOTIC) {
                adjalign(-5);
                u.ugangr += 3;
                (void) adjattrib(A_WIS, -1, TRUE);
                if (!Inhell)
                    angrygods(u.ualign.type);
                change_luck(-5);
            } else
                adjalign(5);
            if (carried(otmp))
                useup(otmp);
            else
                useupf(otmp, 1L);
            return ECMD_TIME;
        } else if (has_omonst(otmp)
                   && (mtmp = get_mtraits(otmp, FALSE)) != 0
                   && mtmp->mtame) {
                /* mtmp is a temporary pointer to a tame monster's attributes,
                 * not a real monster */
            pline("So this is how you repay loyalty?");
            adjalign(-3);
            value = -1;
            HAggravate_monster |= FROMOUTSIDE;
        } else if (!value) {
            ; /* too old; don't give undead or unicorn bonus or penalty */
        } else if (is_undead(ptr)) { /* Not demons--no demon corpses */
            /* most undead that leave a corpse yield 'human' (or other race)
               corpse so won't get here; the exception is wraith; give the
               bonus for wraith to chaotics too because they are sacrificing
               something valuable (unless hero refuses to eat such things) */
            if (u.ualign.type != A_CHAOTIC
                /* reaching this side of the 'or' means hero is chaotic */
                || (ptr == &mons[PM_WRAITH] && u.uconduct.unvegetarian))
                value += 1;
        } else if (is_unicorn(ptr)) {
            int unicalign = sgn(ptr->maligntyp);

            if (unicalign == altaralign) {
                /* When same as altar, always a very bad action.
                 */
                pline("Such an action is an insult to %s!",
                      (unicalign == A_CHAOTIC) ? "chaos"
                         : unicalign ? "law" : "balance");
                (void) adjattrib(A_WIS, -1, TRUE);
                value = -5;
            } else if (u.ualign.type == altaralign) {
                /* When different from altar, and altar is same as yours,
                 * it's a very good action.
                 */
                if (u.ualign.record < ALIGNLIM)
                    You_feel("appropriately %s.", align_str(u.ualign.type));
                else
                    You_feel("you are thoroughly on the right path.");
                adjalign(5);
                value += 3;
            } else if (unicalign == u.ualign.type) {
                /* When sacrificing unicorn of your alignment to altar not of
                 * your alignment, your god gets angry and it's a conversion.
                 */
                u.ualign.record = -1;
                value = 1;
            } else {
                /* Otherwise, unicorn's alignment is different from yours
                 * and different from the altar's.  It's an ordinary (well,
                 * with a bonus) sacrifice on a cross-aligned altar.
                 */
                value += 3;
            }
        }
    } /* corpse */

    if (otmp->otyp == AMULET_OF_YENDOR) {
        if (!highaltar) {
            offer_too_soon(altaralign);
            return ECMD_TIME;
        } else {
            /* The final Test.  Did you win? */
            if (uamul == otmp)
                Amulet_off();
            if (carried(otmp))
                useup(otmp); /* well, it's gone now */
            else
                useupf(otmp, 1L);
            You("offer the Amulet of Yendor to %s...", a_gname());
            if (altaralign == A_NONE) {
                /* Moloch's high altar */
                if (u.ualign.record > -99)
                    u.ualign.record = -99;
                pline(
              "An invisible choir chants, and you are bathed in darkness...");
                /*[apparently shrug/snarl can be sensed without being seen]*/
                pline("%s shrugs and retains dominion over %s,", Moloch,
                      u_gname());
                pline("then mercilessly snuffs out your life.");
                Sprintf(g.killer.name, "%s indifference", s_suffix(Moloch));
                g.killer.format = KILLED_BY;
                done(DIED);
                /* life-saved (or declined to die in wizard/explore mode) */
                pline("%s snarls and tries again...", Moloch);
                fry_by_god(A_NONE, TRUE); /* wrath of Moloch */
                /* declined to die in wizard or explore mode */
                pline(cloud_of_smoke, hcolor(NH_BLACK));
                done(ESCAPED);
            } else if (u.ualign.type != altaralign) {
                /* And the opposing team picks you up and
                   carries you off on their shoulders */
                adjalign(-99);
                pline("%s accepts your gift, and gains dominion over %s...",
                      a_gname(), u_gname());
                pline("%s is enraged...", u_gname());
                pline("Fortunately, %s permits you to live...", a_gname());
                pline(cloud_of_smoke, hcolor(NH_ORANGE));
                done(ESCAPED);
            } else { /* super big win */
                u.uevent.ascended = 1;
                adjalign(10);
                pline(
               "An invisible choir sings, and you are bathed in radiance...");
                godvoice(altaralign, "Mortal, thou hast done well!");
                display_nhwindow(WIN_MESSAGE, FALSE);
                verbalize(
          "In return for thy service, I grant thee the gift of Immortality!");
                You("ascend to the status of Demigod%s...",
                    flags.female ? "dess" : "");
                done(ASCENDED);
            }
            /*NOTREACHED*/
        }
    } /* real Amulet */

    if (otmp->otyp == FAKE_AMULET_OF_YENDOR) {
        if (!highaltar && !otmp->known) {
            offer_too_soon(altaralign);
            return ECMD_TIME;
        }
        You_hear("a nearby thunderclap.");
        if (!otmp->known) {
            You("realize you have made a %s.",
                Hallucination ? "boo-boo" : "mistake");
            otmp->known = TRUE;
            change_luck(-1);
            return ECMD_TIME;
        } else {
            /* don't you dare try to fool the gods */
            if (Deaf)
                pline("Oh, no."); /* didn't hear thunderclap */
            change_luck(-3);
            adjalign(-1);
            u.ugangr += 3;
            value = -3;
        }
    } /* fake Amulet */

    if (value == 0) {
        pline1(nothing_happens);
        return ECMD_TIME;
    }

    if (altaralign != u.ualign.type && highaltar) {
        desecrate_high_altar(altaralign);
    } else if (value < 0) { /* don't think the gods are gonna like this... */
        gods_upset(altaralign);
    } else {
        int saved_anger = u.ugangr;
        int saved_cnt = u.ublesscnt;
        int saved_luck = u.uluck;

        /* Sacrificing at an altar of a different alignment */
        if (u.ualign.type != altaralign) {
            /* Is this a conversion ? */
            /* An unaligned altar in Gehennom will always elicit rejection. */
            if (ugod_is_angry() || (altaralign == A_NONE && Inhell)) {
                if (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL]
                    && altaralign != A_NONE) {
                    You("have a strong feeling that %s is angry...",
                        u_gname());
                    consume_offering(otmp);
                    pline("%s accepts your allegiance.", a_gname());

                    uchangealign(altaralign, 0);
                    /* Beware, Conversion is costly */
                    change_luck(-3);
                    u.ublesscnt += 300;
                } else {
                    u.ugangr += 3;
                    adjalign(-5);
                    pline("%s rejects your sacrifice!", a_gname());
                    godvoice(altaralign, "Suffer, infidel!");
                    change_luck(-5);
                    (void) adjattrib(A_WIS, -2, TRUE);
                    if (!Inhell)
                        angrygods(u.ualign.type);
                }
                return ECMD_TIME;
            } else {
                consume_offering(otmp);
                You("sense a conflict between %s and %s.", u_gname(),
                    a_gname());
                if (rn2(8 + u.ulevel) > 5) {
                    struct monst *pri;
                    boolean shrine;

                    You_feel("the power of %s increase.", u_gname());
                    exercise(A_WIS, TRUE);
                    change_luck(1);
                    shrine = on_shrine();
                    levl[u.ux][u.uy].altarmask = Align2amask(u.ualign.type);
                    if (shrine)
                        levl[u.ux][u.uy].altarmask |= AM_SHRINE;
                    newsym(u.ux, u.uy); /* in case Invisible to self */
                    if (!Blind)
                        pline_The("altar glows %s.",
                                  hcolor((u.ualign.type == A_LAWFUL)
                                            ? NH_WHITE
                                            : u.ualign.type
                                               ? NH_BLACK
                                               : (const char *) "gray"));

                    if (rnl(u.ulevel) > 6 && u.ualign.record > 0
                        && rnd(u.ualign.record) > (3 * ALIGNLIM) / 4)
                        summon_minion(altaralign, TRUE);
                    /* anger priest; test handles bones files */
                    if ((pri = findpriest(temple_occupied(u.urooms)))
                        && !p_coaligned(pri))
                        angry_priest();
                } else {
                    pline("Unluckily, you feel the power of %s decrease.",
                          u_gname());
                    change_luck(-1);
                    exercise(A_WIS, FALSE);
                    if (rnl(u.ulevel) > 6 && u.ualign.record > 0
                        && rnd(u.ualign.record) > (7 * ALIGNLIM) / 8)
                        summon_minion(altaralign, TRUE);
                }
                return ECMD_TIME;
            }
        }

        consume_offering(otmp);
        /* OK, you get brownie points. */
        if (u.ugangr) {
            u.ugangr -= ((value * (u.ualign.type == A_CHAOTIC ? 2 : 3))
                         / MAXVALUE);
            if (u.ugangr < 0)
                u.ugangr = 0;
            if (u.ugangr != saved_anger) {
                if (u.ugangr) {
                    pline("%s seems %s.", u_gname(),
                          Hallucination ? "groovy" : "slightly mollified");

                    if ((int) u.uluck < 0)
                        change_luck(1);
                } else {
                    pline("%s seems %s.", u_gname(),
                          Hallucination ? "cosmic (not a new fact)"
                                        : "mollified");

                    if ((int) u.uluck < 0)
                        u.uluck = 0;
                }
            } else { /* not satisfied yet */
                if (Hallucination)
                    pline_The("gods seem tall.");
                else
                    You("have a feeling of inadequacy.");
            }
        } else if (ugod_is_angry()) {
            if (value > MAXVALUE)
                value = MAXVALUE;
            if (value > -u.ualign.record)
                value = -u.ualign.record;
            adjalign(value);
            You_feel("partially absolved.");
        } else if (u.ublesscnt > 0) {
            u.ublesscnt -= ((value * (u.ualign.type == A_CHAOTIC ? 500 : 300))
                            / MAXVALUE);
            if (u.ublesscnt < 0)
                u.ublesscnt = 0;
            if (u.ublesscnt != saved_cnt) {
                if (u.ublesscnt) {
                    if (Hallucination)
                        You("realize that the gods are not like you and I.");
                    else
                        You("have a hopeful feeling.");
                    if ((int) u.uluck < 0)
                        change_luck(1);
                } else {
                    if (Hallucination)
                        pline("Overall, there is a smell of fried onions.");
                    else
                        You("have a feeling of reconciliation.");
                    if ((int) u.uluck < 0)
                        u.uluck = 0;
                }
            }
        } else {
            int nartifacts = nartifact_exist();

            /* you were already in pretty good standing */
            /* The player can gain an artifact */
            /* The chance goes down as the number of artifacts goes up */
            if (u.ulevel > 2 && u.uluck >= 0
                && !rn2(10 + (2 * u.ugifts * nartifacts))) {
                otmp = mk_artifact((struct obj *) 0, a_align(u.ux, u.uy));
                if (otmp) {
                    char buf[BUFSZ];

                    artifact_origin(otmp, ONAME_GIFT | ONAME_KNOW_ARTI);
                    if (otmp->spe < 0)
                        otmp->spe = 0;
                    if (otmp->cursed)
                        uncurse(otmp);
                    otmp->oerodeproof = TRUE;
                    Strcpy(buf, (Hallucination ? "a doodad"
                                 : Blind ? "an object"
                                   : ansimpleoname(otmp)));
                    if (!Blind)
                        Sprintf(eos(buf), " named %s",
                                bare_artifactname(otmp));
                    at_your_feet(upstart(buf));
                    dropy(otmp);
                    godvoice(u.ualign.type, "Use my gift wisely!");
                    u.ugifts++;
                    u.ublesscnt = rnz(300 + (50 * nartifacts));
                    exercise(A_WIS, TRUE);
                    livelog_printf (LL_DIVINEGIFT | LL_ARTIFACT,
                                    "was bestowed with %s by %s",
                                    artiname(otmp->oartifact),
                                    align_gname(u.ualign.type));
                    /* make sure we can use this weapon */
                    unrestrict_weapon_skill(weapon_type(otmp));
                    if (!Hallucination && !Blind) {
                        otmp->dknown = 1;
                        makeknown(otmp->otyp);
                        discover_artifact(otmp->oartifact);
                    }
                    return ECMD_TIME;
                }
            }
            change_luck((value * LUCKMAX) / (MAXVALUE * 2));
            if ((int) u.uluck < 0)
                u.uluck = 0;
            if (u.uluck != saved_luck) {
                if (Blind)
                    You("think %s brushed your %s.", something,
                        body_part(FOOT));
                else
                    You(Hallucination
                    ? "see crabgrass at your %s.  A funny thing in a dungeon."
                            : "glimpse a four-leaf clover at your %s.",
                        makeplural(body_part(FOOT)));
            }
        }
    }
    return ECMD_TIME;
}

/* determine prayer results in advance; also used for enlightenment */
boolean
can_pray(boolean praying) /* false means no messages should be given */
{
    int alignment;

    g.p_aligntyp = on_altar() ? a_align(u.ux, u.uy) : u.ualign.type;
    g.p_trouble = in_trouble();

    if (is_demon(g.youmonst.data) /* ok if chaotic or none (Moloch) */
        && (g.p_aligntyp == A_LAWFUL || g.p_aligntyp != A_NEUTRAL)) {
        if (praying)
            pline_The("very idea of praying to a %s god is repugnant to you.",
                      g.p_aligntyp ? "lawful" : "neutral");
        return FALSE;
    }

    if (praying)
        You("begin praying to %s.", align_gname(g.p_aligntyp));

    if (u.ualign.type && u.ualign.type == -g.p_aligntyp)
        alignment = -u.ualign.record; /* Opposite alignment altar */
    else if (u.ualign.type != g.p_aligntyp)
        alignment = u.ualign.record / 2; /* Different alignment altar */
    else
        alignment = u.ualign.record;

    if (g.p_aligntyp == A_NONE) /* praying to Moloch */
        g.p_type = -2;
    else if ((g.p_trouble > 0) ? (u.ublesscnt > 200) /* big trouble */
             : (g.p_trouble < 0) ? (u.ublesscnt > 100) /* minor difficulties */
               : (u.ublesscnt > 0))                  /* not in trouble */
        g.p_type = 0;                     /* too soon... */
    else if ((int) Luck < 0 || u.ugangr || alignment < 0)
        g.p_type = 1; /* too naughty... */
    else /* alignment >= 0 */ {
        if (on_altar() && u.ualign.type != g.p_aligntyp)
            g.p_type = 2;
        else
            g.p_type = 3;
    }

    if (is_undead(g.youmonst.data) && !Inhell
        && (g.p_aligntyp == A_LAWFUL
            || (g.p_aligntyp == A_NEUTRAL && !rn2(10))))
        g.p_type = -1;
    /* Note:  when !praying, the random factor for neutrals makes the
       return value a non-deterministic approximation for enlightenment.
       This case should be uncommon enough to live with... */

    return !praying ? (boolean) (g.p_type == 3 && !Inhell) : TRUE;
}

/* return TRUE if praying revived a pet corpse */
static boolean
pray_revive(void)
{
    struct obj *otmp;

    for (otmp = g.level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
        if (otmp->otyp == CORPSE && has_omonst(otmp)
            && OMONST(otmp)->mtame && !OMONST(otmp)->isminion)
            break;

    if (!otmp)
        return FALSE;

    return (revive(otmp, TRUE) != NULL);
}

/* #pray commmand */
int
dopray(void)
{
    /* Confirm accidental slips of Alt-P */
    if (ParanoidPray && yn("Are you sure you want to pray?") != 'y')
        return ECMD_OK;

    if (!u.uconduct.gnostic++)
        /* breaking conduct should probably occur in can_pray() at
         * "You begin praying to %s", as demons who find praying repugnant
         * should not break conduct.  Also we can add more detail to the
         * livelog message as p_aligntyp will be known.
         */
        livelog_printf(LL_CONDUCT, "rejected atheism with a prayer");

    /* set up p_type and p_alignment */
    if (!can_pray(TRUE))
        return ECMD_OK;

    if (wizard && g.p_type >= 0) {
        if (yn("Force the gods to be pleased?") == 'y') {
            u.ublesscnt = 0;
            if (u.uluck < 0)
                u.uluck = 0;
            if (u.ualign.record <= 0)
                u.ualign.record = 1;
            u.ugangr = 0;
            if (g.p_type < 2)
                g.p_type = 3;
        }
    }
    nomul(-3);
    g.multi_reason = "praying";
    g.nomovemsg = "You finish your prayer.";
    g.afternmv = prayer_done;

    if (g.p_type == 3 && !Inhell) {
        /* if you've been true to your god you can't die while you pray */
        if (!Blind)
            You("are surrounded by a shimmering light.");
        u.uinvulnerable = TRUE;
    }

    return ECMD_TIME;
}

static int
prayer_done(void) /* M. Stephenson (1.0.3b) */
{
    aligntyp alignment = g.p_aligntyp;

    u.uinvulnerable = FALSE;
    if (g.p_type == -2) {
        /* praying at an unaligned altar, not necessarily in Gehennom */
        You("%s diabolical laughter all around you...",
            !Deaf ? "hear" : "intuit");
        wake_nearby();
        adjalign(-2);
        exercise(A_WIS, FALSE);
        if (!Inhell) {
            /* hero's god[dess] seems to be keeping his/her head down */
            pline("Nothing else happens."); /* not actually true... */
            return 1;
        } /* else use regular Inhell result below */
    } else if (g.p_type == -1) {
        /* praying while poly'd into an undead creature while non-chaotic */
        godvoice(alignment,
                 (alignment == A_LAWFUL)
                    ? "Vile creature, thou durst call upon me?"
                    : "Walk no more, perversion of nature!");
        You_feel("like you are falling apart.");
        /* KMH -- Gods have mastery over unchanging */
        rehumanize();
        /* no Half_physical_damage adjustment here */
        losehp(rnd(20), "residual undead turning effect", KILLED_BY_AN);
        exercise(A_CON, FALSE);
        return 1;
    }
    if (Inhell) {
        pline("Since you are in Gehennom, %s can't help you.",
              align_gname(alignment));
        /* haltingly aligned is least likely to anger */
        if (u.ualign.record <= 0 || rnl(u.ualign.record))
            angrygods(u.ualign.type);
        return 0;
    }

    if (g.p_type == 0) {
        if (on_altar() && u.ualign.type != alignment)
            (void) water_prayer(FALSE);
        u.ublesscnt += rnz(250);
        change_luck(-3);
        gods_upset(u.ualign.type);
    } else if (g.p_type == 1) {
        if (on_altar() && u.ualign.type != alignment)
            (void) water_prayer(FALSE);
        angrygods(u.ualign.type); /* naughty */
    } else if (g.p_type == 2) {
        if (water_prayer(FALSE)) {
            /* attempted water prayer on a non-coaligned altar */
            u.ublesscnt += rnz(250);
            change_luck(-3);
            gods_upset(u.ualign.type);
        } else
            pleased(alignment);
    } else {
        /* coaligned */
        if (on_altar()) {
            (void) pray_revive();
            (void) water_prayer(TRUE);
        }
        pleased(alignment); /* nice */
    }
    return 1;
}

/* #turn command */
int
doturn(void)
{
    /* Knights & Priest(esse)s only please */
    struct monst *mtmp, *mtmp2;
    const char *Gname;
    int once, range, xlev;

    if (!Role_if(PM_CLERIC) && !Role_if(PM_KNIGHT)) {
        /* Try to use the "turn undead" spell. */
        if (known_spell(SPE_TURN_UNDEAD))
            return spelleffects(SPE_TURN_UNDEAD, FALSE);
        You("don't know how to turn undead!");
        return ECMD_OK;
    }
    if (!u.uconduct.gnostic++)
        livelog_printf(LL_CONDUCT, "rejected atheism by turning undead");

    Gname = halu_gname(u.ualign.type);

    /* [What about needing free hands (does #turn involve any gesturing)?] */
    if (!can_chant(&g.youmonst)) {
        /* "evilness": "demons and undead" is too verbose and too precise */
        You("are %s upon %s to turn aside evilness.",
            Strangled ? "not able to call" : "incapable of calling", Gname);
        /* violates agnosticism due to intent; conduct tracking is not
           supposed to affect play but we make an exception here:  use a
           move if this is the first time agnostic conduct has been broken */
        return (u.uconduct.gnostic == 1) ? ECMD_TIME : ECMD_OK;
    }
    if ((u.ualign.type != A_CHAOTIC
         && (is_demon(g.youmonst.data)
             || is_undead(g.youmonst.data) || is_vampshifter(&g.youmonst)))
        || u.ugangr > 6) { /* "Die, mortal!" */
        pline("For some reason, %s seems to ignore you.", Gname);
        aggravate();
        exercise(A_WIS, FALSE);
        return ECMD_TIME;
    }
    if (Inhell) {
        pline("Since you are in Gehennom, %s %s help you.",
              /* not actually calling upon Moloch but use alternate
                 phrasing anyway if hallucinatory feedback says it's him */
              Gname, !strcmp(Gname, Moloch) ? "won't" : "can't");
        aggravate();
        return ECMD_TIME;
    }
    pline("Calling upon %s, you chant an arcane formula.", Gname);
    exercise(A_WIS, TRUE);

    /* note: does not perform unturn_dead() on victims' inventories */
    range = BOLT_LIM + (u.ulevel / 5); /* 8 to 14 */
    range *= range;
    once = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        /* 3.6.3: used to use cansee() here but the purpose is to prevent
           #turn operating through walls, not to require that the hero be
           able to see the target location */
        if (!couldsee(mtmp->mx, mtmp->my)
            || distu(mtmp->mx, mtmp->my) > range)
            continue;

        if (!mtmp->mpeaceful
            && (is_undead(mtmp->data) || is_vampshifter(mtmp)
                || (is_demon(mtmp->data) && (u.ulevel > (MAXULEV / 2))))) {
            mtmp->msleeping = 0;
            if (Confusion) {
                if (!once++)
                    pline("Unfortunately, your voice falters.");
                mtmp->mflee = 0;
                mtmp->mfrozen = 0;
                mtmp->mcanmove = 1;
            } else if (!resist(mtmp, '\0', 0, TELL)) {
                xlev = 6;
                switch (mtmp->data->mlet) {
                /* this is intentional, lichs are tougher
                   than zombies. */
                case S_LICH:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_GHOST:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_VAMPIRE:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_WRAITH:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_MUMMY:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_ZOMBIE:
                    if (u.ulevel >= xlev && !resist(mtmp, '\0', 0, NOTELL)) {
                        if (u.ualign.type == A_CHAOTIC) {
                            mtmp->mpeaceful = 1;
                            set_malign(mtmp);
                        } else { /* damn them */
                            killed(mtmp);
                        }
                        break;
                    } /* else flee */
                /*FALLTHRU*/
                default:
                    monflee(mtmp, 0, FALSE, TRUE);
                    break;
                }
            }
        }
    }

    /*
     *  There is no detrimental effect on self for successful #turn
     *  while in demon or undead form.  That can only be done while
     *  chaotic oneself (see "For some reason" above) and chaotic
     *  turning only makes targets peaceful.
     *
     *  Paralysis duration probably ought to be based on the strengh
     *  of turned creatures rather than on turner's level.
     *  Why doesn't this honor Free_action?  [Because being able to
     *  repeat #turn every turn would be too powerful.  Maybe instead
     *  of nomul(-N) we should add the equivalent of mon->mspec_used
     *  for the hero and refuse to #turn when it's non-zero?  Or have
     *  both and u.uspec_used only matters when Free_action prevents
     *  the brief paralysis?]
     */
    nomul(-(5 - ((u.ulevel - 1) / 6))); /* -5 .. -1 */
    g.multi_reason = "trying to turn the monsters";
    g.nomovemsg = You_can_move_again;
    return ECMD_TIME;
}

int
altarmask_at(coordxy x, coordxy y)
{
    int res = 0;

    if (isok(x, y)) {
        struct monst *mon = m_at(x, y);

        if (mon && M_AP_TYPE(mon) == M_AP_FURNITURE
            && mon->mappearance == S_altar)
            res = has_mcorpsenm(mon) ? MCORPSENM(mon) : 0;
        else if (IS_ALTAR(levl[x][y].typ))
            res = levl[x][y].altarmask;
    }
    return res;
}

const char *
a_gname(void)
{
    return a_gname_at(u.ux, u.uy);
}

/* returns the name of an altar's deity */
const char *
a_gname_at(coordxy x, coordxy y)
{
    if (!IS_ALTAR(levl[x][y].typ))
        return (char *) 0;

    return align_gname(a_align(x, y));
}

/* returns the name of the hero's deity */
const char *
u_gname(void)
{
    return align_gname(u.ualign.type);
}

const char *
align_gname(aligntyp alignment)
{
    const char *gnam;

    switch (alignment) {
    case A_NONE:
        gnam = Moloch;
        break;
    case A_LAWFUL:
        gnam = g.urole.lgod;
        break;
    case A_NEUTRAL:
        gnam = g.urole.ngod;
        break;
    case A_CHAOTIC:
        gnam = g.urole.cgod;
        break;
    default:
        impossible("unknown alignment.");
        gnam = "someone";
        break;
    }
    if (*gnam == '_')
        ++gnam;
    return gnam;
}

static const char *const hallu_gods[] = {
    "the Flying Spaghetti Monster", /* Church of the FSM */
    "Eris",                         /* Discordianism */
    "the Martians",                 /* every science fiction ever */
    "Xom",                          /* Crawl */
    "AnDoR dRaKoN",                 /* ADOM */
    "the Central Bank of Yendor",   /* economics */
    "Tooth Fairy",                  /* real world(?) */
    "Om",                           /* Discworld */
    "Yawgmoth",                     /* Magic: the Gathering */
    "Morgoth",                      /* LoTR */
    "Cthulhu",                      /* Lovecraft */
    "the Ori",                      /* Stargate */
    "destiny",                      /* why not? */
    "your Friend the Computer",     /* Paranoia */
};

/* hallucination handling for priest/minion names: select a random god
   iff character is hallucinating */
const char *
halu_gname(aligntyp alignment)
{
    const char *gnam = NULL;
    int which;

    if (!Hallucination)
        return align_gname(alignment);

    /* Some roles (Priest) don't have a pantheon unless we're playing as
       that role, so keep trying until we get a role which does have one.
       [If playing a Priest, the current pantheon will be twice as likely
       to get picked as any of the others.  That's not significant enough
       to bother dealing with.] */
    do
        which = randrole(TRUE);
    while (!roles[which].lgod);

    switch (rn2_on_display_rng(9)) {
    case 0:
    case 1:
        gnam = roles[which].lgod;
        break;
    case 2:
    case 3:
        gnam = roles[which].ngod;
        break;
    case 4:
    case 5:
        gnam = roles[which].cgod;
        break;
    case 6:
    case 7:
        gnam = hallu_gods[rn2_on_display_rng(SIZE(hallu_gods))];
        break;
    case 8:
        gnam = Moloch;
        break;
    default:
        impossible("rn2 broken in halu_gname?!?");
    }
    if (!gnam) {
        impossible("No random god name?");
        gnam = "your Friend the Computer"; /* Paranoia */
    }
    if (*gnam == '_')
        ++gnam;
    return gnam;
}

/* deity's title */
const char *
align_gtitle(aligntyp alignment)
{
    const char *gnam, *result = "god";

    switch (alignment) {
    case A_LAWFUL:
        gnam = g.urole.lgod;
        break;
    case A_NEUTRAL:
        gnam = g.urole.ngod;
        break;
    case A_CHAOTIC:
        gnam = g.urole.cgod;
        break;
    default:
        gnam = 0;
        break;
    }
    if (gnam && *gnam == '_')
        result = "goddess";
    return result;
}

void
altar_wrath(coordxy x, coordxy y)
{
    aligntyp altaralign = a_align(x, y);

    if (u.ualign.type == altaralign && u.ualign.record > -rn2(4)) {
        godvoice(altaralign, "How darest thou desecrate my altar!");
        (void) adjattrib(A_WIS, -1, FALSE);
        u.ualign.record--;
    } else {
        pline("%s %s%s:",
              !Deaf ? "A voice (could it be"
                    : "Despite your deafness, you seem to hear",
              align_gname(altaralign),
              !Deaf ? "?) whispers" : " say");
        verbalize("Thou shalt pay, infidel!");
        /* higher luck is more likely to be reduced; as it approaches -5
           the chance to lose another point drops down, eventually to 0 */
        if (Luck > -5 && rn2(Luck + 6))
            change_luck(rn2(20) ? -1 : -2);
    }
}

/* assumes isok() at one space away, but not necessarily at two */
static boolean
blocked_boulder(int dx, int dy)
{
    register struct obj *otmp;
    int nx, ny;
    long count = 0L;

    for (otmp = g.level.objects[u.ux + dx][u.uy + dy]; otmp;
         otmp = otmp->nexthere) {
        if (otmp->otyp == BOULDER)
            count += otmp->quan;
    }

    nx = u.ux + 2 * dx, ny = u.uy + 2 * dy; /* next spot beyond boulder(s) */
    switch (count) {
    case 0:
        /* no boulders--not blocked */
        return FALSE;
    case 1:
        /* possibly blocked depending on if it's pushable */
        break;
    case 2:
        /* this is only approximate since multiple boulders might sink */
        if (is_pool_or_lava(nx, ny)) /* does its own isok() check */
            break; /* still need Sokoban check below */
        /*FALLTHRU*/
    default:
        /* more than one boulder--blocked after they push the top one;
           don't force them to push it first to find out */
        return TRUE;
    }

    if (dx && dy && Sokoban) /* can't push boulder diagonally in Sokoban */
        return TRUE;
    if (!isok(nx, ny))
        return TRUE;
    if (IS_ROCK(levl[nx][ny].typ))
        return TRUE;
    if (sobj_at(BOULDER, nx, ny))
        return TRUE;

    return FALSE;
}

/*pray.c*/

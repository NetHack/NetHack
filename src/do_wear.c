/* NetHack 3.7	do_wear.c	$NHDT-Date: 1598958650 2020/09/01 11:10:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.135 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static NEARDATA const char see_yourself[] = "see yourself";
static NEARDATA const char unknown_type[] = "Unknown type of %s (%d)";
static NEARDATA const char c_armor[] = "armor", c_suit[] = "suit",
                           c_shirt[] = "shirt", c_cloak[] = "cloak",
                           c_gloves[] = "gloves", c_boots[] = "boots",
                           c_helmet[] = "helmet", c_shield[] = "shield",
                           c_weapon[] = "weapon", c_sword[] = "sword",
                           c_axe[] = "axe", c_that_[] = "that";

static NEARDATA const long takeoff_order[] = {
    WORN_BLINDF, W_WEP,      WORN_SHIELD, WORN_GLOVES, LEFT_RING,
    RIGHT_RING,  WORN_CLOAK, WORN_HELMET, WORN_AMUL,   WORN_ARMOR,
    WORN_SHIRT,  WORN_BOOTS, W_SWAPWEP,   W_QUIVER,    0L
};

static void FDECL(on_msg, (struct obj *));
static void FDECL(toggle_stealth, (struct obj *, long, BOOLEAN_P));
static int NDECL(Armor_on);
/* int NDECL(Boots_on); -- moved to extern.h */
static int NDECL(Cloak_on);
static int NDECL(Helmet_on);
static int NDECL(Gloves_on);
static void FDECL(wielding_corpse, (struct obj *, BOOLEAN_P));
static int NDECL(Shield_on);
static int NDECL(Shirt_on);
static void NDECL(Amulet_on);
static void FDECL(learnring, (struct obj *, BOOLEAN_P));
static void FDECL(Ring_off_or_gone, (struct obj *, BOOLEAN_P));
static int FDECL(select_off, (struct obj *));
static struct obj *NDECL(do_takeoff);
static int NDECL(take_off);
static int FDECL(menu_remarm, (int));
static void FDECL(count_worn_stuff, (struct obj **, BOOLEAN_P));
static int FDECL(armor_or_accessory_off, (struct obj *));
static int FDECL(accessory_or_armor_on, (struct obj *));
static void FDECL(already_wearing, (const char *));
static void FDECL(already_wearing2, (const char *, const char *));

/* plural "fingers" or optionally "gloves" */
const char *
fingers_or_gloves(check_gloves)
boolean check_gloves;
{
    return ((check_gloves && uarmg)
            ? gloves_simple_name(uarmg) /* "gloves" or "gauntlets" */
            : makeplural(body_part(FINGER))); /* "fingers" */
}

void
off_msg(otmp)
struct obj *otmp;
{
    if (flags.verbose)
        You("were wearing %s.", doname(otmp));
}

/* for items that involve no delay */
static void
on_msg(otmp)
struct obj *otmp;
{
    if (flags.verbose) {
        char how[BUFSZ];
        /* call xname() before obj_is_pname(); formatting obj's name
           might set obj->dknown and that affects the pname test */
        const char *otmp_name = xname(otmp);

        how[0] = '\0';
        if (otmp->otyp == TOWEL)
            Sprintf(how, " around your %s", body_part(HEAD));
        You("are now wearing %s%s.",
            obj_is_pname(otmp) ? the(otmp_name) : an(otmp_name), how);
    }
}

/* putting on or taking off an item which confers stealth;
   give feedback and discover it iff stealth state is changing */
static
void
toggle_stealth(obj, oldprop, on)
struct obj *obj;
long oldprop; /* prop[].extrinsic, with obj->owornmask stripped by caller */
boolean on;
{
    if (on ? g.initial_don : g.context.takeoff.cancelled_don)
        return;

    if (!oldprop /* extrinsic stealth from something else */
        && !HStealth /* intrinsic stealth */
        && !BStealth) { /* stealth blocked by something */
        if (obj->otyp == RIN_STEALTH)
            learnring(obj, TRUE);
        else
            makeknown(obj->otyp);

        if (on) {
            if (!is_boots(obj))
                You("move very quietly.");
            else if (Levitation || Flying)
                You("float imperceptibly.");
            else
                You("walk very quietly.");
        } else {
            You("sure are noisy.");
        }
    }
}

/* putting on or taking off an item which confers displacement, or gaining
   or losing timed displacement after eating a displacer beast corpse or tin;
   give feedback and discover it iff displacement state is changing *and*
   hero is able to see self (or sense monsters); for timed, 'obj' is Null
   and this is only called for the message */
void
toggle_displacement(obj, oldprop, on)
struct obj *obj;
long oldprop; /* prop[].extrinsic, with obj->owornmask stripped by caller */
boolean on;
{
    if (on ? g.initial_don : g.context.takeoff.cancelled_don)
        return;

    if (!oldprop /* extrinsic displacement from something else */
        && !(u.uprops[DISPLACED].intrinsic) /* timed, from eating */
        && !(u.uprops[DISPLACED].blocked) /* (theoretical) */
        /* we don't use canseeself() here because it augments vision
           with touch, which isn't appropriate for deciding whether
           we'll notice that monsters have trouble spotting the hero */
        && ((!Blind         /* see anything */
             && !u.uswallow /* see surroundings */
             && !Invisible) /* see self */
            /* actively sensing nearby monsters via telepathy or extended
               monster detection overrides vision considerations because
               hero also senses self in this situation */
            || (Unblind_telepat
                || (Blind_telepat && Blind)
                || Detect_monsters))) {
        if (obj)
            makeknown(obj->otyp);

        You_feel("that monsters%s have difficulty pinpointing your location.",
                 on ? "" : " no longer");
    }
}

/*
 * The Type_on() functions should be called *after* setworn().
 * The Type_off() functions call setworn() themselves.
 * [Blindf_on() is an exception and calls setworn() itself.]
 */

int
Boots_on(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmf->otyp].oc_oprop].extrinsic & ~WORN_BOOTS;

    switch (uarmf->otyp) {
    case LOW_BOOTS:
    case IRON_SHOES:
    case HIGH_BOOTS:
    case JUMPING_BOOTS:
    case KICKING_BOOTS:
        break;
    case WATER_WALKING_BOOTS:
        if (u.uinwater)
            spoteffects(TRUE);
        /* (we don't need a lava check here since boots can't be
           put on while feet are stuck) */
        break;
    case SPEED_BOOTS:
        /* Speed boots are still better than intrinsic speed, */
        /* though not better than potion speed */
        if (!oldprop && !(HFast & TIMEOUT)) {
            makeknown(uarmf->otyp);
            You_feel("yourself speed up%s.",
                     (oldprop || HFast) ? " a bit more" : "");
        }
        break;
    case ELVEN_BOOTS:
        toggle_stealth(uarmf, oldprop, TRUE);
        break;
    case FUMBLE_BOOTS:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            incr_itimeout(&HFumbling, rnd(20));
        break;
    case LEVITATION_BOOTS:
        if (!oldprop && !HLevitation && !(BLevitation & FROMOUTSIDE)) {
            uarmf->known = 1; /* might come off if putting on over a sink,
                               * so uarmf could be Null below; status line
                               * gets updated during brief interval they're
                               * worn so hero and player learn enchantment */
            g.context.botl = 1; /* status hilites might mark AC changed */
            makeknown(uarmf->otyp);
            float_up();
            if (Levitation)
                spoteffects(FALSE); /* for sink effect */
        } else {
            float_vs_flight(); /* maybe toggle BFlying's I_SPECIAL */
        }
        break;
    default:
        impossible(unknown_type, c_boots, uarmf->otyp);
    }
    if (uarmf) /* could be Null here (levitation boots put on over a sink) */
        uarmf->known = 1; /* boots' +/- evident because of status line AC */
    return 0;
}

int
Boots_off(VOID_ARGS)
{
    struct obj *otmp = uarmf;
    int otyp = otmp->otyp;
    long oldprop = u.uprops[objects[otyp].oc_oprop].extrinsic & ~WORN_BOOTS;

    g.context.takeoff.mask &= ~W_ARMF;
    /* For levitation, float_down() returns if Levitation, so we
     * must do a setworn() _before_ the levitation case.
     */
    setworn((struct obj *) 0, W_ARMF);
    switch (otyp) {
    case SPEED_BOOTS:
        if (!Very_fast && !g.context.takeoff.cancelled_don) {
            makeknown(otyp);
            You_feel("yourself slow down%s.", Fast ? " a bit" : "");
        }
        break;
    case WATER_WALKING_BOOTS:
        /* check for lava since fireproofed boots make it viable */
        if ((is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy))
            && !Levitation && !Flying && !is_clinger(g.youmonst.data)
            && !g.context.takeoff.cancelled_don
            /* avoid recursive call to lava_effects() */
            && !iflags.in_lava_effects) {
            /* make boots known in case you survive the drowning */
            makeknown(otyp);
            spoteffects(TRUE);
        }
        break;
    case ELVEN_BOOTS:
        toggle_stealth(otmp, oldprop, FALSE);
        break;
    case FUMBLE_BOOTS:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            HFumbling = EFumbling = 0;
        break;
    case LEVITATION_BOOTS:
        if (!oldprop && !HLevitation && !(BLevitation & FROMOUTSIDE)
            && !g.context.takeoff.cancelled_don) {
            (void) float_down(0L, 0L);
            makeknown(otyp);
        } else {
            float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        }
        break;
    case LOW_BOOTS:
    case IRON_SHOES:
    case HIGH_BOOTS:
    case JUMPING_BOOTS:
    case KICKING_BOOTS:
        break;
    default:
        impossible(unknown_type, c_boots, otyp);
    }
    g.context.takeoff.cancelled_don = FALSE;
    return 0;
}

static int
Cloak_on(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmc->otyp].oc_oprop].extrinsic & ~WORN_CLOAK;

    switch (uarmc->otyp) {
    case ORCISH_CLOAK:
    case DWARVISH_CLOAK:
    case CLOAK_OF_MAGIC_RESISTANCE:
    case ROBE:
    case LEATHER_CLOAK:
        break;
    case CLOAK_OF_PROTECTION:
        makeknown(uarmc->otyp);
        break;
    case ELVEN_CLOAK:
        toggle_stealth(uarmc, oldprop, TRUE);
        break;
    case CLOAK_OF_DISPLACEMENT:
        toggle_displacement(uarmc, oldprop, TRUE);
        break;
    case MUMMY_WRAPPING:
        /* Note: it's already being worn, so we have to cheat here. */
        if ((HInvis || EInvis) && !Blind) {
            newsym(u.ux, u.uy);
            You("can %s!", See_invisible ? "no longer see through yourself"
                                         : see_yourself);
        }
        break;
    case CLOAK_OF_INVISIBILITY:
        /* since cloak of invisibility was worn, we know mummy wrapping
           wasn't, so no need to check `oldprop' against blocked */
        if (!oldprop && !HInvis && !Blind) {
            makeknown(uarmc->otyp);
            newsym(u.ux, u.uy);
            pline("Suddenly you can%s yourself.",
                  See_invisible ? " see through" : "not see");
        }
        break;
    case OILSKIN_CLOAK:
        pline("%s very tightly.", Tobjnam(uarmc, "fit"));
        break;
    /* Alchemy smock gives poison _and_ acid resistance */
    case ALCHEMY_SMOCK:
        EAcid_resistance |= WORN_CLOAK;
        break;
    default:
        impossible(unknown_type, c_cloak, uarmc->otyp);
    }
    if (uarmc) /* no known instance of !uarmc here but play it safe */
        uarmc->known = 1; /* cloak's +/- evident because of status line AC */
    return 0;
}

int
Cloak_off(VOID_ARGS)
{
    struct obj *otmp = uarmc;
    int otyp = otmp->otyp;
    long oldprop = u.uprops[objects[otyp].oc_oprop].extrinsic & ~WORN_CLOAK;

    g.context.takeoff.mask &= ~W_ARMC;
    /* For mummy wrapping, taking it off first resets `Invisible'. */
    setworn((struct obj *) 0, W_ARMC);
    switch (otyp) {
    case ORCISH_CLOAK:
    case DWARVISH_CLOAK:
    case CLOAK_OF_PROTECTION:
    case CLOAK_OF_MAGIC_RESISTANCE:
    case OILSKIN_CLOAK:
    case ROBE:
    case LEATHER_CLOAK:
        break;
    case ELVEN_CLOAK:
        toggle_stealth(otmp, oldprop, FALSE);
        break;
    case CLOAK_OF_DISPLACEMENT:
        toggle_displacement(otmp, oldprop, FALSE);
        break;
    case MUMMY_WRAPPING:
        if (Invis && !Blind) {
            newsym(u.ux, u.uy);
            You("can %s.", See_invisible ? "see through yourself"
                                         : "no longer see yourself");
        }
        break;
    case CLOAK_OF_INVISIBILITY:
        if (!oldprop && !HInvis && !Blind) {
            makeknown(CLOAK_OF_INVISIBILITY);
            newsym(u.ux, u.uy);
            pline("Suddenly you can %s.",
                  See_invisible ? "no longer see through yourself"
                                : see_yourself);
        }
        break;
    /* Alchemy smock gives poison _and_ acid resistance */
    case ALCHEMY_SMOCK:
        EAcid_resistance &= ~WORN_CLOAK;
        break;
    default:
        impossible(unknown_type, c_cloak, otyp);
    }
    return 0;
}

static
int
Helmet_on(VOID_ARGS)
{
    switch (uarmh->otyp) {
    case FEDORA:
    case HELMET:
    case DENTED_POT:
    case ELVEN_LEATHER_HELM:
    case DWARVISH_IRON_HELM:
    case ORCISH_HELM:
    case HELM_OF_TELEPATHY:
        break;
    case HELM_OF_BRILLIANCE:
        adj_abon(uarmh, uarmh->spe);
        break;
    case CORNUTHAUM:
        /* people think marked wizards know what they're talking about,
           but it takes trained arrogance to pull it off, and the actual
           enchantment of the hat is irrelevant */
        ABON(A_CHA) += (Role_if(PM_WIZARD) ? 1 : -1);
        g.context.botl = 1;
        makeknown(uarmh->otyp);
        break;
    case HELM_OF_OPPOSITE_ALIGNMENT:
        uarmh->known = 1; /* do this here because uarmh could get cleared */
        /* changing alignment can toggle off active artifact properties,
           including levitation; uarmh could get dropped or destroyed here
           by hero falling onto a polymorph trap or into water (emergency
           disrobe) or maybe lava (probably not, helm isn't 'organic') */
        uchangealign((u.ualign.type != A_NEUTRAL)
                         ? -u.ualign.type
                         : (uarmh->o_id % 2) ? A_CHAOTIC : A_LAWFUL,
                     1);
        /* makeknown(HELM_OF_OPPOSITE_ALIGNMENT); -- below, after Tobjnam() */
    /*FALLTHRU*/
    case DUNCE_CAP:
        if (uarmh && !uarmh->cursed) {
            if (Blind)
                pline("%s for a moment.", Tobjnam(uarmh, "vibrate"));
            else
                pline("%s %s for a moment.", Tobjnam(uarmh, "glow"),
                      hcolor(NH_BLACK));
            curse(uarmh);
        }
        g.context.botl = 1; /* reveal new alignment or INT & WIS */
        if (Hallucination) {
            pline("My brain hurts!"); /* Monty Python's Flying Circus */
        } else if (uarmh && uarmh->otyp == DUNCE_CAP) {
            You_feel("%s.", /* track INT change; ignore WIS */
                     ACURR(A_INT)
                             <= (ABASE(A_INT) + ABON(A_INT) + ATEMP(A_INT))
                         ? "like sitting in a corner"
                         : "giddy");
        } else {
            /* [message formerly given here moved to uchangealign()] */
            makeknown(HELM_OF_OPPOSITE_ALIGNMENT);
        }
        break;
    default:
        impossible(unknown_type, c_helmet, uarmh->otyp);
    }
    /* uarmh could be Null due to uchangealign() */
    if (uarmh)
        uarmh->known = 1; /* helmet's +/- evident because of status line AC */
    return 0;
}

int
Helmet_off(VOID_ARGS)
{
    g.context.takeoff.mask &= ~W_ARMH;

    switch (uarmh->otyp) {
    case FEDORA:
    case HELMET:
    case DENTED_POT:
    case ELVEN_LEATHER_HELM:
    case DWARVISH_IRON_HELM:
    case ORCISH_HELM:
        break;
    case DUNCE_CAP:
        g.context.botl = 1;
        break;
    case CORNUTHAUM:
        if (!g.context.takeoff.cancelled_don) {
            ABON(A_CHA) += (Role_if(PM_WIZARD) ? -1 : 1);
            g.context.botl = 1;
        }
        break;
    case HELM_OF_TELEPATHY:
        /* need to update ability before calling see_monsters() */
        setworn((struct obj *) 0, W_ARMH);
        see_monsters();
        return 0;
    case HELM_OF_BRILLIANCE:
        if (!g.context.takeoff.cancelled_don)
            adj_abon(uarmh, -uarmh->spe);
        break;
    case HELM_OF_OPPOSITE_ALIGNMENT:
        /* changing alignment can toggle off active artifact
           properties, including levitation; uarmh could get
           dropped or destroyed here */
        uchangealign(u.ualignbase[A_CURRENT], 2);
        break;
    default:
        impossible(unknown_type, c_helmet, uarmh->otyp);
    }
    setworn((struct obj *) 0, W_ARMH);
    g.context.takeoff.cancelled_don = FALSE;
    return 0;
}

static
int
Gloves_on(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmg->otyp].oc_oprop].extrinsic & ~WORN_GLOVES;

    switch (uarmg->otyp) {
    case LEATHER_GLOVES:
        break;
    case GAUNTLETS_OF_FUMBLING:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            incr_itimeout(&HFumbling, rnd(20));
        break;
    case GAUNTLETS_OF_POWER:
        makeknown(uarmg->otyp);
        g.context.botl = 1; /* taken care of in attrib.c */
        break;
    case GAUNTLETS_OF_DEXTERITY:
        adj_abon(uarmg, uarmg->spe);
        break;
    default:
        impossible(unknown_type, c_gloves, uarmg->otyp);
    }
    if (uarmg) /* no known instance of !uarmg here but play it safe */
        uarmg->known = 1; /* gloves' +/- evident because of status line AC */
    return 0;
}

static void
wielding_corpse(obj, voluntary)
struct obj *obj;
boolean voluntary; /* taking gloves off on purpose? */
{
    char kbuf[BUFSZ];

    if (!obj || obj->otyp != CORPSE)
        return;
    if (obj != uwep && (obj != uswapwep || !u.twoweap))
        return;

    if (touch_petrifies(&mons[obj->corpsenm]) && !Stone_resistance) {
        You("now wield %s in your bare %s.",
            corpse_xname(obj, (const char *) 0, CXN_ARTICLE),
            makeplural(body_part(HAND)));
        Sprintf(kbuf, "%s gloves while wielding %s",
                voluntary ? "removing" : "losing", killer_xname(obj));
        instapetrify(kbuf);
        /* life-saved; can't continue wielding cockatrice corpse though */
        remove_worn_item(obj, FALSE);
    }
}

int
Gloves_off(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmg->otyp].oc_oprop].extrinsic & ~WORN_GLOVES;
    boolean on_purpose = !g.context.mon_moving && !uarmg->in_use;

    g.context.takeoff.mask &= ~W_ARMG;

    switch (uarmg->otyp) {
    case LEATHER_GLOVES:
        break;
    case GAUNTLETS_OF_FUMBLING:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            HFumbling = EFumbling = 0;
        break;
    case GAUNTLETS_OF_POWER:
        makeknown(uarmg->otyp);
        g.context.botl = 1; /* taken care of in attrib.c */
        break;
    case GAUNTLETS_OF_DEXTERITY:
        if (!g.context.takeoff.cancelled_don)
            adj_abon(uarmg, -uarmg->spe);
        break;
    default:
        impossible(unknown_type, c_gloves, uarmg->otyp);
    }
    setworn((struct obj *) 0, W_ARMG);
    g.context.takeoff.cancelled_don = FALSE;
    (void) encumber_msg(); /* immediate feedback for GoP */

    /* usually can't remove gloves when they're slippery but it can
       be done by having them fall off (polymorph), stolen, or
       destroyed (scroll, overenchantment, monster spell); if that
       happens, 'cure' slippery fingers so that it doesn't transfer
       from gloves to bare hands */
    if (Glib)
        make_glib(0); /* for update_inventory() */

    /* prevent wielding cockatrice when not wearing gloves */
    if (uwep && uwep->otyp == CORPSE)
        wielding_corpse(uwep, on_purpose);

    /* KMH -- ...or your secondary weapon when you're wielding it
       [This case can't actually happen; twoweapon mode won't
       engage if a corpse has been set up as the alternate weapon.] */
    if (u.twoweap && uswapwep && uswapwep->otyp == CORPSE)
        wielding_corpse(uswapwep, on_purpose);
    if (condtests[bl_bareh].enabled)
        g.context.botl = 1;

    return 0;
}

static int
Shield_on(VOID_ARGS)
{
    /* no shield currently requires special handling when put on, but we
       keep this uncommented in case somebody adds a new one which does
       [reflection is handled by setting u.uprops[REFLECTION].extrinsic
       in setworn() called by armor_or_accessory_on() before Shield_on()] */
    switch (uarms->otyp) {
    case SMALL_SHIELD:
    case ELVEN_SHIELD:
    case URUK_HAI_SHIELD:
    case ORCISH_SHIELD:
    case DWARVISH_ROUNDSHIELD:
    case LARGE_SHIELD:
    case SHIELD_OF_REFLECTION:
        break;
    default:
        impossible(unknown_type, c_shield, uarms->otyp);
    }
    if (uarms) /* no known instance of !uarmgs here but play it safe */
        uarms->known = 1; /* shield's +/- evident because of status line AC */
    return 0;
}

int
Shield_off(VOID_ARGS)
{
    g.context.takeoff.mask &= ~W_ARMS;

    /* no shield currently requires special handling when taken off, but we
       keep this uncommented in case somebody adds a new one which does */
    switch (uarms->otyp) {
    case SMALL_SHIELD:
    case ELVEN_SHIELD:
    case URUK_HAI_SHIELD:
    case ORCISH_SHIELD:
    case DWARVISH_ROUNDSHIELD:
    case LARGE_SHIELD:
    case SHIELD_OF_REFLECTION:
        break;
    default:
        impossible(unknown_type, c_shield, uarms->otyp);
    }

    setworn((struct obj *) 0, W_ARMS);
    return 0;
}

static int
Shirt_on(VOID_ARGS)
{
    /* no shirt currently requires special handling when put on, but we
       keep this uncommented in case somebody adds a new one which does */
    switch (uarmu->otyp) {
    case HAWAIIAN_SHIRT:
    case T_SHIRT:
        break;
    default:
        impossible(unknown_type, c_shirt, uarmu->otyp);
    }
    if (uarmu) /* no known instances of !uarmu here but play it safe */
        uarmu->known = 1; /* shirt's +/- evident because of status line AC */
    return 0;
}

int
Shirt_off(VOID_ARGS)
{
    g.context.takeoff.mask &= ~W_ARMU;

    /* no shirt currently requires special handling when taken off, but we
       keep this uncommented in case somebody adds a new one which does */
    switch (uarmu->otyp) {
    case HAWAIIAN_SHIRT:
    case T_SHIRT:
        break;
    default:
        impossible(unknown_type, c_shirt, uarmu->otyp);
    }

    setworn((struct obj *) 0, W_ARMU);
    return 0;
}

static
int
Armor_on(VOID_ARGS)
{
    /*
     * No suits require special handling.  Special properties conferred by
     * suits are set up as intrinsics (actually 'extrinsics') by setworn()
     * which is called by armor_or_accessory_on() before Armor_on().
     */
    if (uarm) /* no known instances of !uarm here but play it safe */
        uarm->known = 1; /* suit's +/- evident because of status line AC */
    return 0;
}

int
Armor_off(VOID_ARGS)
{
    g.context.takeoff.mask &= ~W_ARM;
    setworn((struct obj *) 0, W_ARM);
    g.context.takeoff.cancelled_don = FALSE;
    return 0;
}

/* The gone functions differ from the off functions in that if you die from
 * taking it off and have life saving, you still die.  [Obsolete reference
 * to lack of fire resistance being fatal in hell (nethack 3.0) and life
 * saving putting a removed item back on to prevent that from immediately
 * repeating.]
 */
int
Armor_gone()
{
    g.context.takeoff.mask &= ~W_ARM;
    setnotworn(uarm);
    g.context.takeoff.cancelled_don = FALSE;
    return 0;
}

static void
Amulet_on()
{
    /* make sure amulet isn't wielded; can't use remove_worn_item()
       here because it has already been set worn in amulet slot */
    if (uamul == uwep)
        setuwep((struct obj *) 0);
    else if (uamul == uswapwep)
        setuswapwep((struct obj *) 0);
    else if (uamul == uquiver)
        setuqwep((struct obj *) 0);

    switch (uamul->otyp) {
    case AMULET_OF_ESP:
    case AMULET_OF_LIFE_SAVING:
    case AMULET_VERSUS_POISON:
    case AMULET_OF_REFLECTION:
    case AMULET_OF_MAGICAL_BREATHING:
    case FAKE_AMULET_OF_YENDOR:
        break;
    case AMULET_OF_UNCHANGING:
        if (Slimed)
            make_slimed(0L, (char *) 0);
        break;
    case AMULET_OF_CHANGE: {
        int orig_sex = poly_gender();

        if (Unchanging)
            break;
        change_sex();
        /* Don't use same message as polymorph */
        if (orig_sex != poly_gender()) {
            makeknown(AMULET_OF_CHANGE);
            You("are suddenly very %s!",
                flags.female ? "feminine" : "masculine");
            g.context.botl = 1;
        } else
            /* already polymorphed into single-gender monster; only
               changed the character's base sex */
            You("don't feel like yourself.");
        pline_The("amulet disintegrates!");
        if (orig_sex == poly_gender() && uamul->dknown
            && !objects[AMULET_OF_CHANGE].oc_name_known
            && !objects[AMULET_OF_CHANGE].oc_uname)
            docall(uamul);
        useup(uamul);
        break;
    }
    case AMULET_OF_STRANGULATION:
        if (can_be_strangled(&g.youmonst)) {
            makeknown(AMULET_OF_STRANGULATION);
            Strangled = 6L;
            g.context.botl = TRUE;
            pline("It constricts your throat!");
        }
        break;
    case AMULET_OF_RESTFUL_SLEEP: {
        long newnap = (long) rnd(100), oldnap = (HSleepy & TIMEOUT);

        /* avoid clobbering FROMOUTSIDE bit, which might have
           gotten set by previously eating one of these amulets */
        if (newnap < oldnap || oldnap == 0L)
            HSleepy = (HSleepy & ~TIMEOUT) | newnap;
        break;
    }
    case AMULET_OF_FLYING:
        /* setworn() has already set extrinisic flying */
        float_vs_flight(); /* block flying if levitating */
        if (Flying) {
            boolean already_flying;

            /* to determine whether this flight is new we have to muck
               about in the Flying intrinsic (actually extrinsic) */
            EFlying &= ~W_AMUL;
            already_flying = !!Flying;
            EFlying |= W_AMUL;

            if (!already_flying) {
                makeknown(AMULET_OF_FLYING);
                g.context.botl = TRUE; /* status: 'Fly' On */
                You("are now in flight.");
            }
        }
        break;
    case AMULET_OF_GUARDING:
        makeknown(AMULET_OF_GUARDING);
        find_ac();
        break;
    case AMULET_OF_YENDOR:
        break;
    }
}

void
Amulet_off()
{
    g.context.takeoff.mask &= ~W_AMUL;

    switch (uamul->otyp) {
    case AMULET_OF_ESP:
        /* need to update ability before calling see_monsters() */
        setworn((struct obj *) 0, W_AMUL);
        see_monsters();
        return;
    case AMULET_OF_LIFE_SAVING:
    case AMULET_VERSUS_POISON:
    case AMULET_OF_REFLECTION:
    case AMULET_OF_CHANGE:
    case AMULET_OF_UNCHANGING:
    case FAKE_AMULET_OF_YENDOR:
        break;
    case AMULET_OF_MAGICAL_BREATHING:
        if (Underwater) {
            /* HMagical_breathing must be set off
                before calling drown() */
            setworn((struct obj *) 0, W_AMUL);
            if (!breathless(g.youmonst.data) && !amphibious(g.youmonst.data)
                && !Swimming) {
                You("suddenly inhale an unhealthy amount of %s!",
                    hliquid("water"));
                (void) drown();
            }
            return;
        }
        break;
    case AMULET_OF_STRANGULATION:
        if (Strangled) {
            Strangled = 0L;
            g.context.botl = TRUE;
            if (Breathless)
                Your("%s is no longer constricted!", body_part(NECK));
            else
                You("can breathe more easily!");
        }
        break;
    case AMULET_OF_RESTFUL_SLEEP:
        setworn((struct obj *) 0, W_AMUL);
        /* HSleepy = 0L; -- avoid clobbering FROMOUTSIDE bit */
        if (!ESleepy && !(HSleepy & ~TIMEOUT))
            HSleepy &= ~TIMEOUT; /* clear timeout bits */
        return;
    case AMULET_OF_FLYING: {
        boolean was_flying = !!Flying;

        /* remove amulet 'early' to determine whether Flying changes */
        setworn((struct obj *) 0, W_AMUL);
        float_vs_flight(); /* probably not needed here */
        if (was_flying && !Flying) {
            makeknown(AMULET_OF_FLYING);
            g.context.botl = TRUE; /* status: 'Fly' Off */
            You("%s.", (is_pool_or_lava(u.ux, u.uy)
                        || Is_waterlevel(&u.uz) || Is_airlevel(&u.uz))
                          ? "stop flying"
                          : "land");
            spoteffects(TRUE);
        }
        break;
    }
    case AMULET_OF_GUARDING:
        find_ac();
        break;
    case AMULET_OF_YENDOR:
        break;
    }
    setworn((struct obj *) 0, W_AMUL);
    return;
}

/* handle ring discovery; comparable to learnwand() */
static void
learnring(ring, observed)
struct obj *ring;
boolean observed;
{
    int ringtype = ring->otyp;

    /* if effect was observeable then we usually discover the type */
    if (observed) {
        /* if we already know the ring type which accomplishes this
           effect (assumes there is at most one type for each effect),
           mark this ring as having been seen (no need for makeknown);
           otherwise if we have seen this ring, discover its type */
        if (objects[ringtype].oc_name_known)
            ring->dknown = 1;
        else if (ring->dknown)
            makeknown(ringtype);
#if 0 /* see learnwand() */
        else
            ring->eknown = 1;
#endif
    }

    /* make enchantment of charged ring known (might be +0) and update
       perm invent window if we've seen this ring and know its type */
    if (ring->dknown && objects[ringtype].oc_name_known) {
        if (objects[ringtype].oc_charged)
            ring->known = 1;
        update_inventory();
    }
}

void
Ring_on(obj)
register struct obj *obj;
{
    long oldprop = u.uprops[objects[obj->otyp].oc_oprop].extrinsic;
    int old_attrib, which;
    boolean observable;

    /* make sure ring isn't wielded; can't use remove_worn_item()
       here because it has already been set worn in a ring slot */
    if (obj == uwep)
        setuwep((struct obj *) 0);
    else if (obj == uswapwep)
        setuswapwep((struct obj *) 0);
    else if (obj == uquiver)
        setuqwep((struct obj *) 0);

    /* only mask out W_RING when we don't have both
       left and right rings of the same type */
    if ((oldprop & W_RING) != W_RING)
        oldprop &= ~W_RING;

    switch (obj->otyp) {
    case RIN_TELEPORTATION:
    case RIN_REGENERATION:
    case RIN_SEARCHING:
    case RIN_HUNGER:
    case RIN_AGGRAVATE_MONSTER:
    case RIN_POISON_RESISTANCE:
    case RIN_FIRE_RESISTANCE:
    case RIN_COLD_RESISTANCE:
    case RIN_SHOCK_RESISTANCE:
    case RIN_CONFLICT:
    case RIN_TELEPORT_CONTROL:
    case RIN_POLYMORPH:
    case RIN_POLYMORPH_CONTROL:
    case RIN_FREE_ACTION:
    case RIN_SLOW_DIGESTION:
    case RIN_SUSTAIN_ABILITY:
    case MEAT_RING:
        break;
    case RIN_STEALTH:
        toggle_stealth(obj, oldprop, TRUE);
        break;
    case RIN_WARNING:
        see_monsters();
        break;
    case RIN_SEE_INVISIBLE:
        /* can now see invisible monsters */
        set_mimic_blocking(); /* do special mimic handling */
        see_monsters();

        if (Invis && !oldprop && !HSee_invisible && !Blind) {
            newsym(u.ux, u.uy);
            pline("Suddenly you are transparent, but there!");
            learnring(obj, TRUE);
        }
        break;
    case RIN_INVISIBILITY:
        if (!oldprop && !HInvis && !BInvis && !Blind) {
            learnring(obj, TRUE);
            newsym(u.ux, u.uy);
            self_invis_message();
        }
        break;
    case RIN_LEVITATION:
        if (!oldprop && !HLevitation && !(BLevitation & FROMOUTSIDE)) {
            float_up();
            learnring(obj, TRUE);
            if (Levitation)
                spoteffects(FALSE); /* for sinks */
        } else {
            float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        }
        break;
    case RIN_GAIN_STRENGTH:
        which = A_STR;
        goto adjust_attrib;
    case RIN_GAIN_CONSTITUTION:
        which = A_CON;
        goto adjust_attrib;
    case RIN_ADORNMENT:
        which = A_CHA;
 adjust_attrib:
        old_attrib = ACURR(which);
        ABON(which) += obj->spe;
        observable = (old_attrib != ACURR(which));
        /* if didn't change, usually means ring is +0 but might
           be because nonzero couldn't go below min or above max;
           learn +0 enchantment if attribute value is not stuck
           at a limit [and ring has been seen and its type is
           already discovered, both handled by learnring()] */
        if (observable || !extremeattr(which))
            learnring(obj, observable);
        g.context.botl = 1;
        break;
    case RIN_INCREASE_ACCURACY: /* KMH */
        u.uhitinc += obj->spe;
        break;
    case RIN_INCREASE_DAMAGE:
        u.udaminc += obj->spe;
        break;
    case RIN_PROTECTION_FROM_SHAPE_CHAN:
        rescham();
        break;
    case RIN_PROTECTION:
        /* usually learn enchantment and discover type;
           won't happen if ring is unseen or if it's +0
           and the type hasn't been discovered yet */
        observable = (obj->spe != 0);
        learnring(obj, observable);
        if (obj->spe)
            find_ac(); /* updates botl */
        break;
    }
}

static void
Ring_off_or_gone(obj, gone)
register struct obj *obj;
boolean gone;
{
    long mask = (obj->owornmask & W_RING);
    int old_attrib, which;
    boolean observable;

    g.context.takeoff.mask &= ~mask;
    if (!(u.uprops[objects[obj->otyp].oc_oprop].extrinsic & mask))
        impossible("Strange... I didn't know you had that ring.");
    if (gone)
        setnotworn(obj);
    else
        setworn((struct obj *) 0, obj->owornmask);

    switch (obj->otyp) {
    case RIN_TELEPORTATION:
    case RIN_REGENERATION:
    case RIN_SEARCHING:
    case RIN_HUNGER:
    case RIN_AGGRAVATE_MONSTER:
    case RIN_POISON_RESISTANCE:
    case RIN_FIRE_RESISTANCE:
    case RIN_COLD_RESISTANCE:
    case RIN_SHOCK_RESISTANCE:
    case RIN_CONFLICT:
    case RIN_TELEPORT_CONTROL:
    case RIN_POLYMORPH:
    case RIN_POLYMORPH_CONTROL:
    case RIN_FREE_ACTION:
    case RIN_SLOW_DIGESTION:
    case RIN_SUSTAIN_ABILITY:
    case MEAT_RING:
        break;
    case RIN_STEALTH:
        toggle_stealth(obj, (EStealth & ~mask), FALSE);
        break;
    case RIN_WARNING:
        see_monsters();
        break;
    case RIN_SEE_INVISIBLE:
        /* Make invisible monsters go away */
        if (!See_invisible) {
            set_mimic_blocking(); /* do special mimic handling */
            see_monsters();
        }

        if (Invisible && !Blind) {
            newsym(u.ux, u.uy);
            pline("Suddenly you cannot see yourself.");
            learnring(obj, TRUE);
        }
        break;
    case RIN_INVISIBILITY:
        if (!Invis && !BInvis && !Blind) {
            newsym(u.ux, u.uy);
            Your("body seems to unfade%s.",
                 See_invisible ? " completely" : "..");
            learnring(obj, TRUE);
        }
        break;
    case RIN_LEVITATION:
        if (!(BLevitation & FROMOUTSIDE)) {
            (void) float_down(0L, 0L);
            if (!Levitation)
                learnring(obj, TRUE);
        } else {
            float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        }
        break;
    case RIN_GAIN_STRENGTH:
        which = A_STR;
        goto adjust_attrib;
    case RIN_GAIN_CONSTITUTION:
        which = A_CON;
        goto adjust_attrib;
    case RIN_ADORNMENT:
        which = A_CHA;
 adjust_attrib:
        old_attrib = ACURR(which);
        ABON(which) -= obj->spe;
        observable = (old_attrib != ACURR(which));
        /* same criteria as Ring_on() */
        if (observable || !extremeattr(which))
            learnring(obj, observable);
        g.context.botl = 1;
        break;
    case RIN_INCREASE_ACCURACY: /* KMH */
        u.uhitinc -= obj->spe;
        break;
    case RIN_INCREASE_DAMAGE:
        u.udaminc -= obj->spe;
        break;
    case RIN_PROTECTION:
        /* might have been put on while blind and we can now see
           or perhaps been forgotten due to amnesia */
        observable = (obj->spe != 0);
        learnring(obj, observable);
        if (obj->spe)
            find_ac(); /* updates botl */
        break;
    case RIN_PROTECTION_FROM_SHAPE_CHAN:
        /* If you're no longer protected, let the chameleons
         * change shape again -dgk
         */
        restartcham();
        break;
    }
}

void
Ring_off(obj)
struct obj *obj;
{
    Ring_off_or_gone(obj, FALSE);
}

void
Ring_gone(obj)
struct obj *obj;
{
    Ring_off_or_gone(obj, TRUE);
}

void
Blindf_on(otmp)
struct obj *otmp;
{
    boolean already_blind = Blind, changed = FALSE;

    /* blindfold might be wielded; release it for wearing */
    if (otmp->owornmask & W_WEAPONS)
        remove_worn_item(otmp, FALSE);
    setworn(otmp, W_TOOL);
    on_msg(otmp);

    if (Blind && !already_blind) {
        changed = TRUE;
        if (flags.verbose)
            You_cant("see any more.");
        /* set ball&chain variables before the hero goes blind */
        if (Punished)
            set_bc(0);
    } else if (already_blind && !Blind) {
        changed = TRUE;
        /* "You are now wearing the Eyes of the Overworld." */
        if (u.uroleplay.blind) {
            /* this can only happen by putting on the Eyes of the Overworld;
               that shouldn't actually produce a permanent cure, but we
               can't let the "blind from birth" conduct remain intact */
            pline("For the first time in your life, you can see!");
            u.uroleplay.blind = FALSE;
        } else
            You("can see!");
    }
    if (changed) {
        toggle_blindness(); /* potion.c */
    }
}

void
Blindf_off(otmp)
struct obj *otmp;
{
    boolean was_blind = Blind, changed = FALSE,
            nooffmsg = !otmp;

    if (!otmp)
        otmp = ublindf;
    if (!otmp) {
        impossible("Blindf_off without eyewear?");
        return;
    }
    g.context.takeoff.mask &= ~W_TOOL;
    setworn((struct obj *) 0, otmp->owornmask);
    if (!nooffmsg)
        off_msg(otmp);

    if (Blind) {
        if (was_blind) {
            /* "still cannot see" makes no sense when removing lenses
               since they can't have been the cause of your blindness */
            if (otmp->otyp != LENSES)
                You("still cannot see.");
        } else {
            changed = TRUE; /* !was_blind */
            /* "You were wearing the Eyes of the Overworld." */
            You_cant("see anything now!");
            /* set ball&chain variables before the hero goes blind */
            if (Punished)
                set_bc(0);
        }
    } else if (was_blind) {
        if (!gulp_blnd_check()) {
            changed = TRUE; /* !Blind */
            You("can see again.");
        }
    }
    if (changed) {
        toggle_blindness(); /* potion.c */
    }
}

/* called in moveloop()'s prologue to set side-effects of worn start-up items;
   also used by poly_obj() when a worn item gets transformed */
void
set_wear(obj)
struct obj *obj; /* if null, do all worn items; otherwise just obj itself */
{
    g.initial_don = !obj;

    if (!obj ? ublindf != 0 : (obj == ublindf))
        (void) Blindf_on(ublindf);
    if (!obj ? uright != 0 : (obj == uright))
        (void) Ring_on(uright);
    if (!obj ? uleft != 0 : (obj == uleft))
        (void) Ring_on(uleft);
    if (!obj ? uamul != 0 : (obj == uamul))
        (void) Amulet_on();

    if (!obj ? uarmu != 0 : (obj == uarmu))
        (void) Shirt_on();
    if (!obj ? uarm != 0 : (obj == uarm))
        (void) Armor_on();
    if (!obj ? uarmc != 0 : (obj == uarmc))
        (void) Cloak_on();
    if (!obj ? uarmf != 0 : (obj == uarmf))
        (void) Boots_on();
    if (!obj ? uarmg != 0 : (obj == uarmg))
        (void) Gloves_on();
    if (!obj ? uarmh != 0 : (obj == uarmh))
        (void) Helmet_on();
    if (!obj ? uarms != 0 : (obj == uarms))
        (void) Shield_on();

    g.initial_don = FALSE;
}

/* check whether the target object is currently being put on (or taken off--
   also checks for doffing--[why?]) */
boolean
donning(otmp)
struct obj *otmp;
{
    boolean result = FALSE;

    /* 'W' (or 'P' used for armor) sets g.afternmv */
    if (doffing(otmp))
        result = TRUE;
    else if (otmp == uarm)
        result = (g.afternmv == Armor_on);
    else if (otmp == uarmu)
        result = (g.afternmv == Shirt_on);
    else if (otmp == uarmc)
        result = (g.afternmv == Cloak_on);
    else if (otmp == uarmf)
        result = (g.afternmv == Boots_on);
    else if (otmp == uarmh)
        result = (g.afternmv == Helmet_on);
    else if (otmp == uarmg)
        result = (g.afternmv == Gloves_on);
    else if (otmp == uarms)
        result = (g.afternmv == Shield_on);

    return result;
}

/* check whether the target object is currently being taken off,
   so that stop_donning() and steal() can vary messages and doname()
   can vary "(being worn)" suffix */
boolean
doffing(otmp)
struct obj *otmp;
{
    long what = g.context.takeoff.what;
    boolean result = FALSE;

    /* 'T' (or 'R' used for armor) sets g.afternmv, 'A' sets takeoff.what */
    if (otmp == uarm)
        result = (g.afternmv == Armor_off || what == WORN_ARMOR);
    else if (otmp == uarmu)
        result = (g.afternmv == Shirt_off || what == WORN_SHIRT);
    else if (otmp == uarmc)
        result = (g.afternmv == Cloak_off || what == WORN_CLOAK);
    else if (otmp == uarmf)
        result = (g.afternmv == Boots_off || what == WORN_BOOTS);
    else if (otmp == uarmh)
        result = (g.afternmv == Helmet_off || what == WORN_HELMET);
    else if (otmp == uarmg)
        result = (g.afternmv == Gloves_off || what == WORN_GLOVES);
    else if (otmp == uarms)
        result = (g.afternmv == Shield_off || what == WORN_SHIELD);
    /* these 1-turn items don't need 'g.afternmv' checks */
    else if (otmp == uamul)
        result = (what == WORN_AMUL);
    else if (otmp == uleft)
        result = (what == LEFT_RING);
    else if (otmp == uright)
        result = (what == RIGHT_RING);
    else if (otmp == ublindf)
        result = (what == WORN_BLINDF);
    else if (otmp == uwep)
        result = (what == W_WEP);
    else if (otmp == uswapwep)
        result = (what == W_SWAPWEP);
    else if (otmp == uquiver)
        result = (what == W_QUIVER);

    return result;
}

/* despite their names, cancel_don() and cancel_doff() both apply to both
   donning and doffing... */
void
cancel_doff(obj, slotmask)
struct obj *obj;
long slotmask;
{
    /* Called by setworn() for old item in specified slot or by setnotworn()
     * for specified item.  We don't want to call cancel_don() if we got
     * here via <X>_off() -> setworn((struct obj *)0) -> cancel_doff()
     * because that would stop the 'A' command from continuing with next
     * selected item.  So do_takeoff() sets a flag in takeoff.mask for us.
     * [For taking off an individual item with 'T'/'R'/'w-', it doesn't
     * matter whether cancel_don() gets called here--the item has already
     * been removed by now.]
     */
    if (!(g.context.takeoff.mask & I_SPECIAL) && donning(obj))
        cancel_don(); /* applies to doffing too */
    g.context.takeoff.mask &= ~slotmask;
}

/* despite their names, cancel_don() and cancel_doff() both apply to both
   donning and doffing... */
void
cancel_don()
{
    /* the piece of armor we were donning/doffing has vanished, so stop
     * wasting time on it (and don't dereference it when donning would
     * otherwise finish)
     */
    g.context.takeoff.cancelled_don =
        (g.afternmv == Boots_on || g.afternmv == Helmet_on
         || g.afternmv == Gloves_on || g.afternmv == Armor_on);
    g.afternmv = (int NDECL((*))) 0;
    g.nomovemsg = (char *) 0;
    g.multi = 0;
    g.context.takeoff.delay = 0;
    g.context.takeoff.what = 0L;
}

/* called by steal() during theft from hero; interrupt donning/doffing */
int
stop_donning(stolenobj)
struct obj *stolenobj; /* no message if stolenobj is already being doffing */
{
    char buf[BUFSZ];
    struct obj *otmp;
    boolean putting_on;
    int result = 0;

    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if ((otmp->owornmask & W_ARMOR) && donning(otmp))
            break;
    /* at most one item will pass donning() test at any given time */
    if (!otmp)
        return 0;

    /* donning() returns True when doffing too; doffing() is more specific */
    putting_on = !doffing(otmp);
    /* cancel_don() looks at afternmv; it can also cancel doffing */
    cancel_don();
    /* don't want <armor>_on() or <armor>_off() being called
       by unmul() since the on or off action isn't completing */
    g.afternmv = (int NDECL((*))) 0;
    if (putting_on || otmp != stolenobj) {
        Sprintf(buf, "You stop %s %s.",
                putting_on ? "putting on" : "taking off",
                thesimpleoname(otmp));
    } else {
        buf[0] = '\0';   /* silently stop doffing stolenobj */
        result = -g.multi; /* remember this before calling unmul() */
    }
    unmul(buf);
    /* while putting on, item becomes worn immediately but side-effects are
       deferred until the delay expires; when interrupted, make it unworn
       (while taking off, item stays worn until the delay expires; when
       interrupted, leave it worn) */
    if (putting_on)
        remove_worn_item(otmp, FALSE);

    return result;
}

/* both 'clothes' and 'accessories' now include both armor and accessories;
   TOOL_CLASS is for eyewear, FOOD_CLASS is for MEAT_RING */
static NEARDATA const char clothes[] = {
    ARMOR_CLASS, RING_CLASS, AMULET_CLASS, TOOL_CLASS, FOOD_CLASS, 0
};
static NEARDATA const char accessories[] = {
    RING_CLASS, AMULET_CLASS, TOOL_CLASS, FOOD_CLASS, ARMOR_CLASS, 0
};
static NEARDATA int Narmorpieces, Naccessories;

/* assign values to Narmorpieces and Naccessories */
static void
count_worn_stuff(which, accessorizing)
struct obj **which; /* caller wants this when count is 1 */
boolean accessorizing;
{
    struct obj *otmp;

    Narmorpieces = Naccessories = 0;

#define MOREWORN(x,wtyp) do { if (x) { wtyp++; otmp = x; } } while (0)
    otmp = 0;
    MOREWORN(uarmh, Narmorpieces);
    MOREWORN(uarms, Narmorpieces);
    MOREWORN(uarmg, Narmorpieces);
    MOREWORN(uarmf, Narmorpieces);
    /* for cloak/suit/shirt, we only count the outermost item so that it
       can be taken off without confirmation if final count ends up as 1 */
    if (uarmc)
        MOREWORN(uarmc, Narmorpieces);
    else if (uarm)
        MOREWORN(uarm, Narmorpieces);
    else if (uarmu)
        MOREWORN(uarmu, Narmorpieces);
    if (!accessorizing)
        *which = otmp; /* default item iff Narmorpieces is 1 */

    otmp = 0;
    MOREWORN(uleft, Naccessories);
    MOREWORN(uright, Naccessories);
    MOREWORN(uamul, Naccessories);
    MOREWORN(ublindf, Naccessories);
    if (accessorizing)
        *which = otmp; /* default item iff Naccessories is 1 */
#undef MOREWORN
}

/* take off one piece or armor or one accessory;
   shared by dotakeoff('T') and doremring('R') */
static int
armor_or_accessory_off(obj)
struct obj *obj;
{
    if (!(obj->owornmask & (W_ARMOR | W_ACCESSORY))) {
        You("are not wearing that.");
        return 0;
    }
    if (obj == uskin
        || ((obj == uarm) && uarmc)
        || ((obj == uarmu) && (uarmc || uarm))) {
        char why[QBUFSZ], what[QBUFSZ];

        why[0] = what[0] = '\0';
        if (obj != uskin) {
            if (uarmc)
                Strcat(what, cloak_simple_name(uarmc));
            if ((obj == uarmu) && uarm) {
                if (uarmc)
                    Strcat(what, " and ");
                Strcat(what, suit_simple_name(uarm));
            }
            Sprintf(why, " without taking off your %s first", what);
        } else {
            Strcpy(why, "; it's embedded");
        }
        You_cant("take that off%s.", why);
        return 0;
    }

    reset_remarm(); /* clear context.takeoff.mask and context.takeoff.what */
    (void) select_off(obj);
    if (!g.context.takeoff.mask)
        return 0;
    /* none of armoroff()/Ring_/Amulet/Blindf_off() use context.takeoff.mask */
    reset_remarm();

    if (obj->owornmask & W_ARMOR) {
        (void) armoroff(obj);
    } else if (obj == uright || obj == uleft) {
        /* Sometimes we want to give the off_msg before removing and
         * sometimes after; for instance, "you were wearing a moonstone
         * ring (on right hand)" is desired but "you were wearing a
         * square amulet (being worn)" is not because of the redundant
         * "being worn".
         */
        off_msg(obj);
        Ring_off(obj);
    } else if (obj == uamul) {
        Amulet_off();
        off_msg(obj);
    } else if (obj == ublindf) {
        Blindf_off(obj); /* does its own off_msg */
    } else {
        impossible("removing strange accessory?");
        if (obj->owornmask)
            remove_worn_item(obj, FALSE);
    }
    return 1;
}

/* the 'T' command */
int
dotakeoff()
{
    struct obj *otmp = (struct obj *) 0;

    count_worn_stuff(&otmp, FALSE);
    if (!Narmorpieces && !Naccessories) {
        /* assert( GRAY_DRAGON_SCALES > YELLOW_DRAGON_SCALE_MAIL ); */
        if (uskin)
            pline_The("%s merged with your skin!",
                      uskin->otyp >= GRAY_DRAGON_SCALES
                          ? "dragon scales are"
                          : "dragon scale mail is");
        else
            pline("Not wearing any armor or accessories.");
        return 0;
    }
    if (Narmorpieces != 1 || ParanoidRemove)
        otmp = getobj(clothes, "take off");
    if (!otmp)
        return 0;

    return armor_or_accessory_off(otmp);
}

/* the 'R' command */
int
doremring()
{
    struct obj *otmp = 0;

    count_worn_stuff(&otmp, TRUE);
    if (!Naccessories && !Narmorpieces) {
        pline("Not wearing any accessories or armor.");
        return 0;
    }
    if (Naccessories != 1 || ParanoidRemove)
        otmp = getobj(accessories, "remove");
    if (!otmp)
        return 0;

    return armor_or_accessory_off(otmp);
}

/* Check if something worn is cursed _and_ unremovable. */
int
cursed(otmp)
struct obj *otmp;
{
    if (!otmp) {
        impossible("cursed without otmp");
        return 0;
    }
    /* Curses, like chickens, come home to roost. */
    if ((otmp == uwep) ? welded(otmp) : (int) otmp->cursed) {
        boolean use_plural = (is_boots(otmp) || is_gloves(otmp)
                              || otmp->otyp == LENSES || otmp->quan > 1L);

        /* might be trying again after applying grease to hands */
        if (Glib && otmp->bknown
            /* for weapon, we'll only get here via 'A )' */
            && (uarmg ? (otmp == uwep)
                      : ((otmp->owornmask & (W_WEP | W_RING)) != 0)))
            pline("Despite your slippery %s, you can't.",
                  fingers_or_gloves(TRUE));
        else
            You("can't.  %s cursed.", use_plural ? "They are" : "It is");
        set_bknown(otmp, 1);
        return 1;
    }
    return 0;
}

int
armoroff(otmp)
struct obj *otmp;
{
    static char offdelaybuf[60];
    int delay = -objects[otmp->otyp].oc_delay;
    const char *what = 0;

    if (cursed(otmp))
        return 0;
    /* this used to make assumptions about which types of armor had
       delays and which didn't; now both are handled for all types */
    if (delay) {
        nomul(delay);
        g.multi_reason = "disrobing";
        switch (objects[otmp->otyp].oc_armcat) {
        case ARM_SUIT:
            what = suit_simple_name(otmp);
            g.afternmv = Armor_off;
            break;
        case ARM_SHIELD:
            what = shield_simple_name(otmp);
            g.afternmv = Shield_off;
            break;
        case ARM_HELM:
            what = helm_simple_name(otmp);
            g.afternmv = Helmet_off;
            break;
        case ARM_GLOVES:
            what = gloves_simple_name(otmp);
            g.afternmv = Gloves_off;
            break;
        case ARM_BOOTS:
            what = boots_simple_name(otmp);
            g.afternmv = Boots_off;
            break;
        case ARM_CLOAK:
            what = cloak_simple_name(otmp);
            g.afternmv = Cloak_off;
            break;
        case ARM_SHIRT:
            what = shirt_simple_name(otmp);
            g.afternmv = Shirt_off;
            break;
        default:
            impossible("Taking off unknown armor (%d: %d), delay %d",
                       otmp->otyp, objects[otmp->otyp].oc_armcat, delay);
            break;
        }
        if (what) {
            /* sizeof offdelaybuf == 60; increase it if this becomes longer */
            Sprintf(offdelaybuf, "You finish taking off your %s.", what);
            g.nomovemsg = offdelaybuf;
        }
    } else {
        /* no delay so no '(*afternmv)()' or 'nomovemsg' */
        switch (objects[otmp->otyp].oc_armcat) {
        case ARM_SUIT:
            (void) Armor_off();
            break;
        case ARM_SHIELD:
            (void) Shield_off();
            break;
        case ARM_HELM:
            (void) Helmet_off();
            break;
        case ARM_GLOVES:
            (void) Gloves_off();
            break;
        case ARM_BOOTS:
            (void) Boots_off();
            break;
        case ARM_CLOAK:
            (void) Cloak_off();
            break;
        case ARM_SHIRT:
            (void) Shirt_off();
            break;
        default:
            impossible("Taking off unknown armor (%d: %d), no delay",
                       otmp->otyp, objects[otmp->otyp].oc_armcat);
            break;
        }
        /* We want off_msg() after removing the item to
           avoid "You were wearing ____ (being worn)." */
        off_msg(otmp);
    }
    g.context.takeoff.mask = g.context.takeoff.what = 0L;
    return 1;
}

static void
already_wearing(cc)
const char *cc;
{
    You("are already wearing %s%c", cc, (cc == c_that_) ? '!' : '.');
}

static void
already_wearing2(cc1, cc2)
const char *cc1, *cc2;
{
    You_cant("wear %s because you're wearing %s there already.", cc1, cc2);
}

/*
 * canwearobj checks to see whether the player can wear a piece of armor
 *
 * inputs: otmp (the piece of armor)
 *         noisy (if TRUE give error messages, otherwise be quiet about it)
 * output: mask (otmp's armor type)
 */
int
canwearobj(otmp, mask, noisy)
struct obj *otmp;
long *mask;
boolean noisy;
{
    int err = 0;
    const char *which;

    /* this is the same check as for 'W' (dowear), but different message,
       in case we get here via 'P' (doputon) */
    if (verysmall(g.youmonst.data) || nohands(g.youmonst.data)) {
        if (noisy)
            You("can't wear any armor in your current form.");
        return 0;
    }

    which = is_cloak(otmp)
                ? c_cloak
                : is_shirt(otmp)
                    ? c_shirt
                    : is_suit(otmp)
                        ? c_suit
                        : 0;
    if (which && cantweararm(g.youmonst.data)
        /* same exception for cloaks as used in m_dowear() */
        && (which != c_cloak || g.youmonst.data->msize != MZ_SMALL)
        && (racial_exception(&g.youmonst, otmp) < 1)) {
        if (noisy)
            pline_The("%s will not fit on your body.", which);
        return 0;
    } else if (otmp->owornmask & W_ARMOR) {
        if (noisy)
            already_wearing(c_that_);
        return 0;
    }

    if (welded(uwep) && bimanual(uwep) && (is_suit(otmp) || is_shirt(otmp))) {
        if (noisy)
            You("cannot do that while holding your %s.",
                is_sword(uwep) ? c_sword : c_weapon);
        return 0;
    }

    if (is_helmet(otmp)) {
        if (uarmh) {
            if (noisy)
                already_wearing(an(helm_simple_name(uarmh)));
            err++;
        } else if (Upolyd && has_horns(g.youmonst.data) && !is_flimsy(otmp)) {
            /* (flimsy exception matches polyself handling) */
            if (noisy)
                pline_The("%s won't fit over your horn%s.",
                          helm_simple_name(otmp),
                          plur(num_horns(g.youmonst.data)));
            err++;
        } else
            *mask = W_ARMH;
    } else if (is_shield(otmp)) {
        if (uarms) {
            if (noisy)
                already_wearing(an(c_shield));
            err++;
        } else if (uwep && bimanual(uwep)) {
            if (noisy)
                You("cannot wear a shield while wielding a two-handed %s.",
                    is_sword(uwep) ? c_sword : (uwep->otyp == BATTLE_AXE)
                                                   ? c_axe
                                                   : c_weapon);
            err++;
        } else if (u.twoweap) {
            if (noisy)
                You("cannot wear a shield while wielding two weapons.");
            err++;
        } else
            *mask = W_ARMS;
    } else if (is_boots(otmp)) {
        if (uarmf) {
            if (noisy)
                already_wearing(c_boots);
            err++;
        } else if (Upolyd && slithy(g.youmonst.data)) {
            if (noisy)
                You("have no feet..."); /* not body_part(FOOT) */
            err++;
        } else if (Upolyd && g.youmonst.data->mlet == S_CENTAUR) {
            /* break_armor() pushes boots off for centaurs,
               so don't let dowear() put them back on... */
            if (noisy)
                pline("You have too many hooves to wear %s.",
                      c_boots); /* makeplural(body_part(FOOT)) yields
                                   "rear hooves" which sounds odd */
            err++;
        } else if (u.utrap
                   && (u.utraptype == TT_BEARTRAP || u.utraptype == TT_INFLOOR
                       || u.utraptype == TT_LAVA
                       || u.utraptype == TT_BURIEDBALL)) {
            if (u.utraptype == TT_BEARTRAP) {
                if (noisy)
                    Your("%s is trapped!", body_part(FOOT));
            } else if (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA) {
                if (noisy)
                    Your("%s are stuck in the %s!",
                         makeplural(body_part(FOOT)), surface(u.ux, u.uy));
            } else { /*TT_BURIEDBALL*/
                if (noisy)
                    Your("%s is attached to the buried ball!",
                         body_part(LEG));
            }
            err++;
        } else
            *mask = W_ARMF;
    } else if (is_gloves(otmp)) {
        if (uarmg) {
            if (noisy)
                already_wearing(c_gloves);
            err++;
        } else if (welded(uwep)) {
            if (noisy)
                You("cannot wear gloves over your %s.",
                    is_sword(uwep) ? c_sword : c_weapon);
            err++;
        } else if (Glib) {
            /* prevent slippery bare fingers from transferring to
               gloved fingers */
            if (noisy)
                Your("%s are too slippery to pull on %s.",
                     fingers_or_gloves(FALSE), gloves_simple_name(otmp));
            err++;
        } else
            *mask = W_ARMG;
    } else if (is_shirt(otmp)) {
        if (uarm || uarmc || uarmu) {
            if (uarmu) {
                if (noisy)
                    already_wearing(an(c_shirt));
            } else {
                if (noisy)
                    You_cant("wear that over your %s.",
                             (uarm && !uarmc) ? c_armor
                                              : cloak_simple_name(uarmc));
            }
            err++;
        } else
            *mask = W_ARMU;
    } else if (is_cloak(otmp)) {
        if (uarmc) {
            if (noisy)
                already_wearing(an(cloak_simple_name(uarmc)));
            err++;
        } else
            *mask = W_ARMC;
    } else if (is_suit(otmp)) {
        if (uarmc) {
            if (noisy)
                You("cannot wear armor over a %s.", cloak_simple_name(uarmc));
            err++;
        } else if (uarm) {
            if (noisy)
                already_wearing("some armor");
            err++;
        } else
            *mask = W_ARM;
    } else {
        /* getobj can't do this after setting its allow_all flag; that
           happens if you have armor for slots that are covered up or
           extra armor for slots that are filled */
        if (noisy)
            silly_thing("wear", otmp);
        err++;
    }
    /* Unnecessary since now only weapons and special items like pick-axes get
     * welded to your hand, not armor
        if (welded(otmp)) {
            if (!err++) {
                if (noisy) weldmsg(otmp);
            }
        }
     */
    return !err;
}

static int
accessory_or_armor_on(obj)
struct obj *obj;
{
    long mask = 0L;
    boolean armor, ring, eyewear;

    if (obj->owornmask & (W_ACCESSORY | W_ARMOR)) {
        already_wearing(c_that_);
        return 0;
    }
    armor = (obj->oclass == ARMOR_CLASS);
    ring = (obj->oclass == RING_CLASS || obj->otyp == MEAT_RING);
    eyewear = (obj->otyp == BLINDFOLD || obj->otyp == TOWEL
               || obj->otyp == LENSES);
    /* checks which are performed prior to actually touching the item */
    if (armor) {
        if (!canwearobj(obj, &mask, TRUE))
            return 0;

        if (obj->otyp == HELM_OF_OPPOSITE_ALIGNMENT
            && qstart_level.dnum == u.uz.dnum) { /* in quest */
            if (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL])
                You("narrowly avoid losing all chance at your goal.");
            else /* converted */
                You("are suddenly overcome with shame and change your mind.");
            u.ublessed = 0; /* lose your god's protection */
            makeknown(obj->otyp);
            g.context.botl = 1; /*for AC after zeroing u.ublessed */
            return 1;
        }
    } else {
        /*
         * FIXME:
         *  except for the rings/nolimbs case, this allows you to put on
         *  accessories without having any hands to manipulate them, and
         *  to put them on when poly'd into a tiny or huge form where
         *  they shouldn't fit.  [If the latter situation changes, make
         *  comparable change to break_armor(polyself.c).]
         */

        /* accessory */
        if (ring) {
            char answer, qbuf[QBUFSZ];
            int res = 0;

            if (nolimbs(g.youmonst.data)) {
                You("cannot make the ring stick to your body.");
                return 0;
            }
            if (uleft && uright) {
                There("are no more %s%s to fill.",
                      humanoid(g.youmonst.data) ? "ring-" : "",
                      fingers_or_gloves(FALSE));
                return 0;
            }
            if (uleft) {
                mask = RIGHT_RING;
            } else if (uright) {
                mask = LEFT_RING;
            } else {
                do {
                    Sprintf(qbuf, "Which %s%s, Right or Left?",
                            humanoid(g.youmonst.data) ? "ring-" : "",
                            body_part(FINGER));
                    answer = yn_function(qbuf, "rl", '\0');
                    switch (answer) {
                    case '\0':
                    case '\033':
                        return 0;
                    case 'l':
                    case 'L':
                        mask = LEFT_RING;
                        break;
                    case 'r':
                    case 'R':
                        mask = RIGHT_RING;
                        break;
                    }
                } while (!mask);
            }
            if (uarmg && Glib) {
                Your(
              "%s are too slippery to remove, so you cannot put on the ring.",
                     gloves_simple_name(uarmg));
                return 1; /* always uses move */
            }
            if (uarmg && uarmg->cursed) {
                res = !uarmg->bknown;
                set_bknown(uarmg, 1);
                You("cannot remove your %s to put on the ring.", c_gloves);
                return res; /* uses move iff we learned gloves are cursed */
            }
            if (uwep) {
                res = !uwep->bknown; /* check this before calling welded() */
                if ((mask == RIGHT_RING || bimanual(uwep)) && welded(uwep)) {
                    const char *hand = body_part(HAND);

                    /* welded will set bknown */
                    if (bimanual(uwep))
                        hand = makeplural(hand);
                    You("cannot free your weapon %s to put on the ring.",
                        hand);
                    return res; /* uses move iff we learned weapon is cursed */
                }
            }
        } else if (obj->oclass == AMULET_CLASS) {
            if (uamul) {
                already_wearing("an amulet");
                return 0;
            }
        } else if (eyewear) {
            if (!has_head(g.youmonst.data)) {
                You("have no head to wear %s on.", ansimpleoname(obj));
                return 0;
            }

            if (ublindf) {
                if (ublindf->otyp == TOWEL)
                    Your("%s is already covered by a towel.",
                         body_part(FACE));
                else if (ublindf->otyp == BLINDFOLD) {
                    if (obj->otyp == LENSES)
                        already_wearing2("lenses", "a blindfold");
                    else
                        already_wearing("a blindfold");
                } else if (ublindf->otyp == LENSES) {
                    if (obj->otyp == BLINDFOLD)
                        already_wearing2("a blindfold", "some lenses");
                    else
                        already_wearing("some lenses");
                } else {
                    already_wearing(something); /* ??? */
                }
                return 0;
            }
        } else {
            /* neither armor nor accessory */
            You_cant("wear that!");
            return 0;
        }
    }

    if (!retouch_object(&obj, FALSE))
        return 1; /* costs a turn even though it didn't get worn */

    if (armor) {
        int delay;

        /* if the armor is wielded, release it for wearing (won't be
           welded even if cursed; that only happens for weapons/weptools) */
        if (obj->owornmask & W_WEAPONS)
            remove_worn_item(obj, FALSE);
        /*
         * Setting obj->known=1 is done because setworn() causes hero's AC
         * to change so armor's +/- value is evident via the status line.
         * We used to set it here because of that, but then it would stick
         * if a nymph stole the armor before it was fully worn.  Delay it
         * until the afternmv action.  The player may still know this armor's
         * +/- amount if donning gets interrupted, but the hero won't.
         *
        obj->known = 1;
         */
        setworn(obj, mask);
        /* if there's no delay, we'll execute 'afternmv' immediately */
        if (obj == uarm)
            g.afternmv = Armor_on;
        else if (obj == uarmh)
            g.afternmv = Helmet_on;
        else if (obj == uarmg)
            g.afternmv = Gloves_on;
        else if (obj == uarmf)
            g.afternmv = Boots_on;
        else if (obj == uarms)
            g.afternmv = Shield_on;
        else if (obj == uarmc)
            g.afternmv = Cloak_on;
        else if (obj == uarmu)
            g.afternmv = Shirt_on;
        else
            panic("wearing armor not worn as armor? [%08lx]", obj->owornmask);

        delay = -objects[obj->otyp].oc_delay;
        if (delay) {
            nomul(delay);
            g.multi_reason = "dressing up";
            g.nomovemsg = "You finish your dressing maneuver.";
        } else {
            unmul(""); /* call afternmv, clear it+nomovemsg+multi_reason */
            on_msg(obj);
        }
        g.context.takeoff.mask = g.context.takeoff.what = 0L;
    } else { /* not armor */
        boolean give_feedback = FALSE;

        /* [releasing wielded accessory handled in Xxx_on()] */
        if (ring) {
            setworn(obj, mask);
            Ring_on(obj);
            give_feedback = TRUE;
        } else if (obj->oclass == AMULET_CLASS) {
            setworn(obj, W_AMUL);
            Amulet_on();
            /* no feedback here if amulet of change got used up */
            give_feedback = (uamul != 0);
        } else if (eyewear) {
            /* setworn() handled by Blindf_on() */
            Blindf_on(obj);
            /* message handled by Blindf_on(); leave give_feedback False */
        }
        /* feedback for ring or for amulet other than 'change' */
        if (give_feedback && is_worn(obj))
            prinv((char *) 0, obj, 0L);
    }
    return 1;
}

/* the 'W' command */
int
dowear()
{
    struct obj *otmp;

    /* cantweararm() checks for suits of armor, not what we want here;
       verysmall() or nohands() checks for shields, gloves, etc... */
    if (verysmall(g.youmonst.data) || nohands(g.youmonst.data)) {
        pline("Don't even bother.");
        return 0;
    }
    if (uarm && uarmu && uarmc && uarmh && uarms && uarmg && uarmf
        && uleft && uright && uamul && ublindf) {
        /* 'W' message doesn't mention accessories */
        You("are already wearing a full complement of armor.");
        return 0;
    }
    otmp = getobj(clothes, "wear");
    return otmp ? accessory_or_armor_on(otmp) : 0;
}

/* the 'P' command */
int
doputon()
{
    struct obj *otmp;

    if (uleft && uright && uamul && ublindf
        && uarm && uarmu && uarmc && uarmh && uarms && uarmg && uarmf) {
        /* 'P' message doesn't mention armor */
        Your("%s%s are full, and you're already wearing an amulet and %s.",
             humanoid(g.youmonst.data) ? "ring-" : "",
             fingers_or_gloves(FALSE),
             (ublindf->otyp == LENSES) ? "some lenses" : "a blindfold");
        return 0;
    }
    otmp = getobj(accessories, "put on");
    return otmp ? accessory_or_armor_on(otmp) : 0;
}

/* calculate current armor class */
void
find_ac()
{
    int uac = mons[u.umonnum].ac; /* base armor class for current form */

    /* armor class from worn gear */
    if (uarm)
        uac -= ARM_BONUS(uarm);
    if (uarmc)
        uac -= ARM_BONUS(uarmc);
    if (uarmh)
        uac -= ARM_BONUS(uarmh);
    if (uarmf)
        uac -= ARM_BONUS(uarmf);
    if (uarms)
        uac -= ARM_BONUS(uarms);
    if (uarmg)
        uac -= ARM_BONUS(uarmg);
    if (uarmu)
        uac -= ARM_BONUS(uarmu);
    if (uleft && uleft->otyp == RIN_PROTECTION)
        uac -= uleft->spe;
    if (uright && uright->otyp == RIN_PROTECTION)
        uac -= uright->spe;
    if (uamul && uamul->otyp == AMULET_OF_GUARDING)
        uac -= 2; /* fixed amount; main benefit is to MC */

    /* armor class from other sources */
    if (HProtection & INTRINSIC)
        uac -= u.ublessed;
    uac -= u.uspellprot;

    /* [The magic binary numbers 127 and -128 should be replaced with the
     * mystic decimal numbers 99 and -99 which require no explanation to
     * the uninitiated and would cap the width of a status line value at
     * one less character.]
     */
    if (uac < -128)
        uac = -128; /* u.uac is an schar */
    else if (uac > 127)
        uac = 127; /* for completeness */

    if (uac != u.uac) {
        u.uac = uac;
        g.context.botl = 1;
    }
}

void
glibr()
{
    register struct obj *otmp;
    int xfl = 0;
    boolean leftfall, rightfall, wastwoweap = FALSE;
    const char *otherwep = 0, *thiswep, *which, *hand;

    leftfall = (uleft && !uleft->cursed
                && (!uwep || !welded(uwep) || !bimanual(uwep)));
    rightfall = (uright && !uright->cursed && (!welded(uwep)));
    if (!uarmg && (leftfall || rightfall) && !nolimbs(g.youmonst.data)) {
        /* changed so cursed rings don't fall off, GAN 10/30/86 */
        Your("%s off your %s.",
             (leftfall && rightfall) ? "rings slip" : "ring slips",
             (leftfall && rightfall) ? fingers_or_gloves(FALSE)
                                     : body_part(FINGER));
        xfl++;
        if (leftfall) {
            otmp = uleft;
            Ring_off(uleft);
            dropx(otmp);
        }
        if (rightfall) {
            otmp = uright;
            Ring_off(uright);
            dropx(otmp);
        }
    }

    otmp = uswapwep;
    if (u.twoweap && otmp) {
        /* secondary weapon doesn't need nearly as much handling as
           primary; when in two-weapon mode, we know it's one-handed
           with something else in the other hand and also that it's
           a weapon or weptool rather than something unusual, plus
           we don't need to compare its type with the primary */
        otherwep = is_sword(otmp) ? c_sword : weapon_descr(otmp);
        if (otmp->quan > 1L)
            otherwep = makeplural(otherwep);
        hand = body_part(HAND);
        which = "left ";
        Your("%s %s%s from your %s%s.", otherwep, xfl ? "also " : "",
             otense(otmp, "slip"), which, hand);
        xfl++;
        wastwoweap = TRUE;
        setuswapwep((struct obj *) 0); /* clears u.twoweap */
        if (canletgo(otmp, ""))
            dropx(otmp);
    }
    otmp = uwep;
    if (otmp && !welded(otmp)) {
        long savequan = otmp->quan;

        /* nice wording if both weapons are the same type */
        thiswep = is_sword(otmp) ? c_sword : weapon_descr(otmp);
        if (otherwep && strcmp(thiswep, makesingular(otherwep)))
            otherwep = 0;
        if (otmp->quan > 1L) {
            /* most class names for unconventional wielded items
               are ok, but if wielding multiple apples or rations
               we don't want "your foods slip", so force non-corpse
               food to be singular; skipping makeplural() isn't
               enough--we need to fool otense() too */
            if (!strcmp(thiswep, "food"))
                otmp->quan = 1L;
            else
                thiswep = makeplural(thiswep);
        }
        hand = body_part(HAND);
        which = "";
        if (bimanual(otmp))
            hand = makeplural(hand);
        else if (wastwoweap)
            which = "right "; /* preceding msg was about left */
        pline("%s %s%s %s%s from your %s%s.",
              !strncmp(thiswep, "corpse", 6) ? "The" : "Your",
              otherwep ? "other " : "", thiswep, xfl ? "also " : "",
              otense(otmp, "slip"), which, hand);
        /* xfl++; */
        otmp->quan = savequan;
        setuwep((struct obj *) 0);
        if (canletgo(otmp, ""))
            dropx(otmp);
    }
}

struct obj *
some_armor(victim)
struct monst *victim;
{
    register struct obj *otmph, *otmp;

    otmph = (victim == &g.youmonst) ? uarmc : which_armor(victim, W_ARMC);
    if (!otmph)
        otmph = (victim == &g.youmonst) ? uarm : which_armor(victim, W_ARM);
    if (!otmph)
        otmph = (victim == &g.youmonst) ? uarmu : which_armor(victim, W_ARMU);

    otmp = (victim == &g.youmonst) ? uarmh : which_armor(victim, W_ARMH);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    otmp = (victim == &g.youmonst) ? uarmg : which_armor(victim, W_ARMG);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    otmp = (victim == &g.youmonst) ? uarmf : which_armor(victim, W_ARMF);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    otmp = (victim == &g.youmonst) ? uarms : which_armor(victim, W_ARMS);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    return otmph;
}

/* used for praying to check and fix levitation trouble */
struct obj *
stuck_ring(ring, otyp)
struct obj *ring;
int otyp;
{
    if (ring != uleft && ring != uright) {
        impossible("stuck_ring: neither left nor right?");
        return (struct obj *) 0;
    }

    if (ring && ring->otyp == otyp) {
        /* reasons ring can't be removed match those checked by select_off();
           limbless case has extra checks because ordinarily it's temporary */
        if (nolimbs(g.youmonst.data) && uamul
            && uamul->otyp == AMULET_OF_UNCHANGING && uamul->cursed)
            return uamul;
        if (welded(uwep) && (ring == uright || bimanual(uwep)))
            return uwep;
        if (uarmg && uarmg->cursed)
            return uarmg;
        if (ring->cursed)
            return ring;
        /* normally outermost layer is processed first, but slippery gloves
           wears off quickly so uncurse ring itself before handling those */
        if (uarmg && Glib)
            return uarmg;
    }
    /* either no ring or not right type or nothing prevents its removal */
    return (struct obj *) 0;
}

/* also for praying; find worn item that confers "Unchanging" attribute */
struct obj *
unchanger()
{
    if (uamul && uamul->otyp == AMULET_OF_UNCHANGING)
        return uamul;
    return 0;
}

static
int
select_off(otmp)
register struct obj *otmp;
{
    struct obj *why;
    char buf[BUFSZ];

    if (!otmp)
        return 0;
    *buf = '\0'; /* lint suppression */

    /* special ring checks */
    if (otmp == uright || otmp == uleft) {
        struct obj glibdummy;

        if (nolimbs(g.youmonst.data)) {
            pline_The("ring is stuck.");
            return 0;
        }
        glibdummy = cg.zeroobj;
        why = 0; /* the item which prevents ring removal */
        if (welded(uwep) && (otmp == uright || bimanual(uwep))) {
            Sprintf(buf, "free a weapon %s", body_part(HAND));
            why = uwep;
        } else if (uarmg && (uarmg->cursed || Glib)) {
            Sprintf(buf, "take off your %s%s",
                    Glib ? "slippery " : "", gloves_simple_name(uarmg));
            why = !Glib ? uarmg : &glibdummy;
        }
        if (why) {
            You("cannot %s to remove the ring.", buf);
            set_bknown(why, 1);
            return 0;
        }
    }
    /* special glove checks */
    if (otmp == uarmg) {
        if (welded(uwep)) {
            You("are unable to take off your %s while wielding that %s.",
                c_gloves, is_sword(uwep) ? c_sword : c_weapon);
            set_bknown(uwep, 1);
            return 0;
        } else if (Glib) {
            pline("%s %s are too slippery to take off.",
                  uarmg->unpaid ? "The" : "Your", /* simplified Shk_Your() */
                  gloves_simple_name(uarmg));
            return 0;
        }
    }
    /* special boot checks */
    if (otmp == uarmf) {
        if (u.utrap && u.utraptype == TT_BEARTRAP) {
            pline_The("bear trap prevents you from pulling your %s out.",
                      body_part(FOOT));
            return 0;
        } else if (u.utrap && u.utraptype == TT_INFLOOR) {
            You("are stuck in the %s, and cannot pull your %s out.",
                surface(u.ux, u.uy), makeplural(body_part(FOOT)));
            return 0;
        }
    }
    /* special suit and shirt checks */
    if (otmp == uarm || otmp == uarmu) {
        why = 0; /* the item which prevents disrobing */
        if (uarmc && uarmc->cursed) {
            Sprintf(buf, "remove your %s", cloak_simple_name(uarmc));
            why = uarmc;
        } else if (otmp == uarmu && uarm && uarm->cursed) {
            Sprintf(buf, "remove your %s", c_suit);
            why = uarm;
        } else if (welded(uwep) && bimanual(uwep)) {
            Sprintf(buf, "release your %s",
                    is_sword(uwep) ? c_sword : (uwep->otyp == BATTLE_AXE)
                                                   ? c_axe
                                                   : c_weapon);
            why = uwep;
        }
        if (why) {
            You("cannot %s to take off %s.", buf, the(xname(otmp)));
            set_bknown(why, 1);
            return 0;
        }
    }
    /* basic curse check */
    if (otmp == uquiver || (otmp == uswapwep && !u.twoweap)) {
        ; /* some items can be removed even when cursed */
    } else {
        /* otherwise, this is fundamental */
        if (cursed(otmp))
            return 0;
    }

    if (otmp == uarm)
        g.context.takeoff.mask |= WORN_ARMOR;
    else if (otmp == uarmc)
        g.context.takeoff.mask |= WORN_CLOAK;
    else if (otmp == uarmf)
        g.context.takeoff.mask |= WORN_BOOTS;
    else if (otmp == uarmg)
        g.context.takeoff.mask |= WORN_GLOVES;
    else if (otmp == uarmh)
        g.context.takeoff.mask |= WORN_HELMET;
    else if (otmp == uarms)
        g.context.takeoff.mask |= WORN_SHIELD;
    else if (otmp == uarmu)
        g.context.takeoff.mask |= WORN_SHIRT;
    else if (otmp == uleft)
        g.context.takeoff.mask |= LEFT_RING;
    else if (otmp == uright)
        g.context.takeoff.mask |= RIGHT_RING;
    else if (otmp == uamul)
        g.context.takeoff.mask |= WORN_AMUL;
    else if (otmp == ublindf)
        g.context.takeoff.mask |= WORN_BLINDF;
    else if (otmp == uwep)
        g.context.takeoff.mask |= W_WEP;
    else if (otmp == uswapwep)
        g.context.takeoff.mask |= W_SWAPWEP;
    else if (otmp == uquiver)
        g.context.takeoff.mask |= W_QUIVER;

    else
        impossible("select_off: %s???", doname(otmp));

    return 0;
}

static struct obj *
do_takeoff()
{
    struct obj *otmp = (struct obj *) 0;
    boolean was_twoweap = u.twoweap;
    struct takeoff_info *doff = &g.context.takeoff;

    g.context.takeoff.mask |= I_SPECIAL; /* set flag for cancel_doff() */
    if (doff->what == W_WEP) {
        if (!cursed(uwep)) {
            setuwep((struct obj *) 0);
            if (was_twoweap)
                You("are no longer wielding either weapon.");
            else
                You("are empty %s.", body_part(HANDED));
        }
    } else if (doff->what == W_SWAPWEP) {
        setuswapwep((struct obj *) 0);
        You("%sno longer %s.", was_twoweap ? "are " : "",
            was_twoweap ? "wielding two weapons at once"
                        : "have a second weapon readied");
    } else if (doff->what == W_QUIVER) {
        setuqwep((struct obj *) 0);
        You("no longer have ammunition readied.");
    } else if (doff->what == WORN_ARMOR) {
        otmp = uarm;
        if (!cursed(otmp))
            (void) Armor_off();
    } else if (doff->what == WORN_CLOAK) {
        otmp = uarmc;
        if (!cursed(otmp))
            (void) Cloak_off();
    } else if (doff->what == WORN_BOOTS) {
        otmp = uarmf;
        if (!cursed(otmp))
            (void) Boots_off();
    } else if (doff->what == WORN_GLOVES) {
        otmp = uarmg;
        if (!cursed(otmp))
            (void) Gloves_off();
    } else if (doff->what == WORN_HELMET) {
        otmp = uarmh;
        if (!cursed(otmp))
            (void) Helmet_off();
    } else if (doff->what == WORN_SHIELD) {
        otmp = uarms;
        if (!cursed(otmp))
            (void) Shield_off();
    } else if (doff->what == WORN_SHIRT) {
        otmp = uarmu;
        if (!cursed(otmp))
            (void) Shirt_off();
    } else if (doff->what == WORN_AMUL) {
        otmp = uamul;
        if (!cursed(otmp))
            Amulet_off();
    } else if (doff->what == LEFT_RING) {
        otmp = uleft;
        if (!cursed(otmp))
            Ring_off(uleft);
    } else if (doff->what == RIGHT_RING) {
        otmp = uright;
        if (!cursed(otmp))
            Ring_off(uright);
    } else if (doff->what == WORN_BLINDF) {
        if (!cursed(ublindf))
            Blindf_off(ublindf);
    } else {
        impossible("do_takeoff: taking off %lx", doff->what);
    }
    g.context.takeoff.mask &= ~I_SPECIAL; /* clear cancel_doff() flag */

    return otmp;
}

/* occupation callback for 'A' */
static
int
take_off(VOID_ARGS)
{
    register int i;
    register struct obj *otmp;
    struct takeoff_info *doff = &g.context.takeoff;

    if (doff->what) {
        if (doff->delay > 0) {
            doff->delay--;
            return 1; /* still busy */
        }
        if ((otmp = do_takeoff()) != 0)
            off_msg(otmp);
        doff->mask &= ~doff->what;
        doff->what = 0L;
    }

    for (i = 0; takeoff_order[i]; i++)
        if (doff->mask & takeoff_order[i]) {
            doff->what = takeoff_order[i];
            break;
        }

    otmp = (struct obj *) 0;
    doff->delay = 0;

    if (doff->what == 0L) {
        You("finish %s.", doff->disrobing);
        return 0;
    } else if (doff->what == W_WEP) {
        doff->delay = 1;
    } else if (doff->what == W_SWAPWEP) {
        doff->delay = 1;
    } else if (doff->what == W_QUIVER) {
        doff->delay = 1;
    } else if (doff->what == WORN_ARMOR) {
        otmp = uarm;
        /* If a cloak is being worn, add the time to take it off and put
         * it back on again.  Kludge alert! since that time is 0 for all
         * known cloaks, add 1 so that it actually matters...
         */
        if (uarmc)
            doff->delay += 2 * objects[uarmc->otyp].oc_delay + 1;
    } else if (doff->what == WORN_CLOAK) {
        otmp = uarmc;
    } else if (doff->what == WORN_BOOTS) {
        otmp = uarmf;
    } else if (doff->what == WORN_GLOVES) {
        otmp = uarmg;
    } else if (doff->what == WORN_HELMET) {
        otmp = uarmh;
    } else if (doff->what == WORN_SHIELD) {
        otmp = uarms;
    } else if (doff->what == WORN_SHIRT) {
        otmp = uarmu;
        /* add the time to take off and put back on armor and/or cloak */
        if (uarm)
            doff->delay += 2 * objects[uarm->otyp].oc_delay;
        if (uarmc)
            doff->delay += 2 * objects[uarmc->otyp].oc_delay + 1;
    } else if (doff->what == WORN_AMUL) {
        doff->delay = 1;
    } else if (doff->what == LEFT_RING) {
        doff->delay = 1;
    } else if (doff->what == RIGHT_RING) {
        doff->delay = 1;
    } else if (doff->what == WORN_BLINDF) {
        /* [this used to be 2, but 'R' (and 'T') only require 1 turn to
           remove a blindfold, so 'A' shouldn't have been requiring 2] */
        doff->delay = 1;
    } else {
        impossible("take_off: taking off %lx", doff->what);
        return 0; /* force done */
    }

    if (otmp)
        doff->delay += objects[otmp->otyp].oc_delay;

    /* Since setting the occupation now starts the counter next move, that
     * would always produce a delay 1 too big per item unless we subtract
     * 1 here to account for it.
     */
    if (doff->delay > 0)
        doff->delay--;

    set_occupation(take_off, doff->disrobing, 0);
    return 1; /* get busy */
}

/* clear saved context to avoid inappropriate resumption of interrupted 'A' */
void
reset_remarm()
{
    g.context.takeoff.what = g.context.takeoff.mask = 0L;
    g.context.takeoff.disrobing[0] = '\0';
}

/* the 'A' command -- remove multiple worn items */
int
doddoremarm()
{
    int result = 0;

    if (g.context.takeoff.what || g.context.takeoff.mask) {
        You("continue %s.", g.context.takeoff.disrobing);
        set_occupation(take_off, g.context.takeoff.disrobing, 0);
        return 0;
    } else if (!uwep && !uswapwep && !uquiver && !uamul && !ublindf && !uleft
               && !uright && !wearing_armor()) {
        You("are not wearing anything.");
        return 0;
    }

    add_valid_menu_class(0); /* reset */
    if (flags.menu_style != MENU_TRADITIONAL
        || (result = ggetobj("take off", select_off, 0, FALSE,
                             (unsigned *) 0)) < -1)
        result = menu_remarm(result);

    if (g.context.takeoff.mask) {
        (void) strncpy(g.context.takeoff.disrobing,
                       (((g.context.takeoff.mask & ~W_WEAPONS) != 0)
                        /* default activity for armor and/or accessories,
                           possibly combined with weapons */
                        ? "disrobing"
                        /* specific activity when handling weapons only */
                        : "disarming"), CONTEXTVERBSZ);
        (void) take_off();
    }
    /* The time to perform the command is already completely accounted for
     * in take_off(); if we return 1, that would add an extra turn to each
     * disrobe.
     */
    return 0;
}

static int
menu_remarm(retry)
int retry;
{
    int n, i = 0;
    menu_item *pick_list;
    boolean all_worn_categories = TRUE;

    if (retry) {
        all_worn_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
        all_worn_categories = FALSE;
        n = query_category("What type of things do you want to take off?",
                           g.invent, (WORN_TYPES | ALL_TYPES
                                    | UNPAID_TYPES | BUCX_TYPES),
                           &pick_list, PICK_ANY);
        if (!n)
            return 0;
        for (i = 0; i < n; i++) {
            if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
                all_worn_categories = TRUE;
            else
                add_valid_menu_class(pick_list[i].item.a_int);
        }
        free((genericptr_t) pick_list);
    } else if (flags.menu_style == MENU_COMBINATION) {
        unsigned ggofeedback = 0;

        i = ggetobj("take off", select_off, 0, TRUE, &ggofeedback);
        if (ggofeedback & ALL_FINISHED)
            return 0;
        all_worn_categories = (i == -2);
    }
    if (menu_class_present('u')
        || menu_class_present('B') || menu_class_present('U')
        || menu_class_present('C') || menu_class_present('X'))
        all_worn_categories = FALSE;

    n = query_objlist("What do you want to take off?", &g.invent,
                      (SIGNAL_NOMENU | USE_INVLET | INVORDER_SORT),
                      &pick_list, PICK_ANY,
                      all_worn_categories ? is_worn : is_worn_by_type);
    if (n > 0) {
        for (i = 0; i < n; i++)
            (void) select_off(pick_list[i].item.a_obj);
        free((genericptr_t) pick_list);
    } else if (n < 0 && flags.menu_style != MENU_COMBINATION) {
        There("is nothing else you can remove or unwield.");
    }
    return 0;
}

/* hit by destroy armor scroll/black dragon breath/monster spell */
int
destroy_arm(atmp)
register struct obj *atmp;
{
    register struct obj *otmp;
#define DESTROY_ARM(o)                            \
    ((otmp = (o)) != 0 && (!atmp || atmp == otmp) \
             && (!obj_resists(otmp, 0, 90))       \
         ? (otmp->in_use = TRUE)                  \
         : FALSE)

    if (DESTROY_ARM(uarmc)) {
        if (donning(otmp))
            cancel_don();
        Your("%s crumbles and turns to dust!", cloak_simple_name(uarmc));
        (void) Cloak_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarm)) {
        if (donning(otmp))
            cancel_don();
        Your("armor turns to dust and falls to the %s!", surface(u.ux, u.uy));
        (void) Armor_gone();
        useup(otmp);
    } else if (DESTROY_ARM(uarmu)) {
        if (donning(otmp))
            cancel_don();
        Your("shirt crumbles into tiny threads and falls apart!");
        (void) Shirt_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarmh)) {
        if (donning(otmp))
            cancel_don();
        Your("%s turns to dust and is blown away!", helm_simple_name(uarmh));
        (void) Helmet_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarmg)) {
        if (donning(otmp))
            cancel_don();
        Your("gloves vanish!");
        (void) Gloves_off();
        useup(otmp);
        selftouch("You");
    } else if (DESTROY_ARM(uarmf)) {
        if (donning(otmp))
            cancel_don();
        Your("boots disintegrate!");
        (void) Boots_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarms)) {
        if (donning(otmp))
            cancel_don();
        Your("shield crumbles away!");
        (void) Shield_off();
        useup(otmp);
    } else {
        return 0; /* could not destroy anything */
    }

#undef DESTROY_ARM
    stop_occupation();
    return 1;
}

void
adj_abon(otmp, delta)
register struct obj *otmp;
register schar delta;
{
    if (uarmg && uarmg == otmp && otmp->otyp == GAUNTLETS_OF_DEXTERITY) {
        if (delta) {
            makeknown(uarmg->otyp);
            ABON(A_DEX) += (delta);
        }
        g.context.botl = 1;
    }
    if (uarmh && uarmh == otmp && otmp->otyp == HELM_OF_BRILLIANCE) {
        if (delta) {
            makeknown(uarmh->otyp);
            ABON(A_INT) += (delta);
            ABON(A_WIS) += (delta);
        }
        g.context.botl = 1;
    }
}

/* decide whether a worn item is covered up by some other worn item,
   used for dipping into liquid and applying grease;
   some criteria are different than select_off()'s */
boolean
inaccessible_equipment(obj, verb, only_if_known_cursed)
struct obj *obj;
const char *verb; /* "dip" or "grease", or null to avoid messages */
boolean only_if_known_cursed; /* ignore covering unless known to be cursed */
{
    static NEARDATA const char need_to_take_off_outer_armor[] =
        "need to take off %s to %s %s.";
    char buf[BUFSZ];
    boolean anycovering = !only_if_known_cursed; /* more comprehensible... */
#define BLOCKSACCESS(x) (anycovering || ((x)->cursed && (x)->bknown))

    if (!obj || !obj->owornmask)
        return FALSE; /* not inaccessible */

    /* check for suit covered by cloak */
    if (obj == uarm && uarmc && BLOCKSACCESS(uarmc)) {
        if (verb) {
            Strcpy(buf, yname(uarmc));
            You(need_to_take_off_outer_armor, buf, verb, yname(obj));
        }
        return TRUE;
    }
    /* check for shirt covered by suit and/or cloak */
    if (obj == uarmu
        && ((uarm && BLOCKSACCESS(uarm)) || (uarmc && BLOCKSACCESS(uarmc)))) {
        if (verb) {
            char cloaktmp[QBUFSZ], suittmp[QBUFSZ];
            /* if sameprefix, use yname and xname to get "your cloak and suit"
               or "Manlobbi's cloak and suit"; otherwise, use yname and yname
               to get "your cloak and Manlobbi's suit" or vice versa */
            boolean sameprefix = (uarm && uarmc
                                  && !strcmp(shk_your(cloaktmp, uarmc),
                                             shk_your(suittmp, uarm)));

            *buf = '\0';
            if (uarmc)
                Strcat(buf, yname(uarmc));
            if (uarm && uarmc)
                Strcat(buf, " and ");
            if (uarm)
                Strcat(buf, sameprefix ? xname(uarm) : yname(uarm));
            You(need_to_take_off_outer_armor, buf, verb, yname(obj));
        }
        return TRUE;
    }
    /* check for ring covered by gloves */
    if ((obj == uleft || obj == uright) && uarmg && BLOCKSACCESS(uarmg)) {
        if (verb) {
            Strcpy(buf, yname(uarmg));
            You(need_to_take_off_outer_armor, buf, verb, yname(obj));
        }
        return TRUE;
    }
    /* item is not inaccessible */
    return FALSE;
}

/*do_wear.c*/

/* NetHack 3.6	wield.c	$NHDT-Date: 1525012623 2018/04/29 14:37:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.56 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* KMH -- Differences between the three weapon slots.
 *
 * The main weapon (uwep):
 * 1.  Is filled by the (w)ield command.
 * 2.  Can be filled with any type of item.
 * 3.  May be carried in one or both hands.
 * 4.  Is used as the melee weapon and as the launcher for
 *     ammunition.
 * 5.  Only conveys intrinsics when it is a weapon, weapon-tool,
 *     or artifact.
 * 6.  Certain cursed items will weld to the hand and cannot be
 *     unwielded or dropped.  See erodeable_wep() and will_weld()
 *     below for the list of which items apply.
 *
 * The secondary weapon (uswapwep):
 * 1.  Is filled by the e(x)change command, which swaps this slot
 *     with the main weapon.  If the "pushweapon" option is set,
 *     the (w)ield command will also store the old weapon in the
 *     secondary slot.
 * 2.  Can be filled with anything that will fit in the main weapon
 *     slot; that is, any type of item.
 * 3.  Is usually NOT considered to be carried in the hands.
 *     That would force too many checks among the main weapon,
 *     second weapon, shield, gloves, and rings; and it would
 *     further be complicated by bimanual weapons.  A special
 *     exception is made for two-weapon combat.
 * 4.  Is used as the second weapon for two-weapon combat, and as
 *     a convenience to swap with the main weapon.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld (see #3 for reasons), but they also
 *     prevent two-weapon combat.
 *
 * The quiver (uquiver):
 * 1.  Is filled by the (Q)uiver command.
 * 2.  Can be filled with any type of item.
 * 3.  Is considered to be carried in a special part of the pack.
 * 4.  Is used as the item to throw with the (f)ire command.
 *     This is a convenience over the normal (t)hrow command.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld; their effect is handled by the normal
 *     throwing code.
 * 7.  The autoquiver option will fill it with something deemed
 *     suitable if (f)ire is used when it's empty.
 *
 * No item may be in more than one of these slots.
 */

STATIC_DCL boolean FDECL(cant_wield_corpse, (struct obj *));
STATIC_DCL int FDECL(ready_weapon, (struct obj *));

/* used by will_weld() */
/* probably should be renamed */
#define erodeable_wep(optr)                             \
    ((optr)->oclass == WEAPON_CLASS || is_weptool(optr) \
     || (optr)->otyp == HEAVY_IRON_BALL || (optr)->otyp == IRON_CHAIN)

/* used by welded(), and also while wielding */
#define will_weld(optr) \
    ((optr)->cursed && (erodeable_wep(optr) || (optr)->otyp == TIN_OPENER))

/*** Functions that place a given item in a slot ***/
/* Proper usage includes:
 * 1.  Initializing the slot during character generation or a
 *     restore.
 * 2.  Setting the slot due to a player's actions.
 * 3.  If one of the objects in the slot are split off, these
 *     functions can be used to put the remainder back in the slot.
 * 4.  Putting an item that was thrown and returned back into the slot.
 * 5.  Emptying the slot, by passing a null object.  NEVER pass
 *     zeroobj!
 *
 * If the item is being moved from another slot, it is the caller's
 * responsibility to handle that.  It's also the caller's responsibility
 * to print the appropriate messages.
 */
void
setuwep(obj)
register struct obj *obj;
{
    struct obj *olduwep = uwep;

    if (obj == uwep)
        return; /* necessary to not set unweapon */
    /* This message isn't printed in the caller because it happens
     * *whenever* Sunsword is unwielded, from whatever cause.
     */
    setworn(obj, W_WEP);
    if (uwep == obj && artifact_light(olduwep) && olduwep->lamplit) {
        end_burn(olduwep, FALSE);
        if (!Blind)
            pline("%s shining.", Tobjnam(olduwep, "stop"));
    }
    if (uwep == obj
        && ((uwep && uwep->oartifact == ART_OGRESMASHER)
            || (olduwep && olduwep->oartifact == ART_OGRESMASHER)))
        context.botl = 1;
    /* Note: Explicitly wielding a pick-axe will not give a "bashing"
     * message.  Wielding one via 'a'pplying it will.
     * 3.2.2:  Wielding arbitrary objects will give bashing message too.
     */
    if (obj) {
        unweapon = (obj->oclass == WEAPON_CLASS)
                       ? is_launcher(obj) || is_ammo(obj) || is_missile(obj)
                             || (is_pole(obj) && !u.usteed)
                       : !is_weptool(obj) && !is_wet_towel(obj);
    } else
        unweapon = TRUE; /* for "bare hands" message */
}

STATIC_OVL boolean
cant_wield_corpse(obj)
struct obj *obj;
{
    char kbuf[BUFSZ];

    if (uarmg || obj->otyp != CORPSE || !touch_petrifies(&mons[obj->corpsenm])
        || Stone_resistance)
        return FALSE;

    /* Prevent wielding cockatrice when not wearing gloves --KAA */
    You("wield %s in your bare %s.",
        corpse_xname(obj, (const char *) 0, CXN_PFX_THE),
        makeplural(body_part(HAND)));
    Sprintf(kbuf, "wielding %s bare-handed", killer_xname(obj));
    instapetrify(kbuf);
    return TRUE;
}

STATIC_OVL int
ready_weapon(wep)
struct obj *wep;
{
    /* Separated function so swapping works easily */
    int res = 0;

    if (!wep) {
        /* No weapon */
        if (uwep) {
            You("are empty %s.", body_part(HANDED));
            setuwep((struct obj *) 0);
            res++;
        } else
            You("are already empty %s.", body_part(HANDED));
    } else if (wep->otyp == CORPSE && cant_wield_corpse(wep)) {
        /* hero must have been life-saved to get here; use a turn */
        res++; /* corpse won't be wielded */
    } else if (uarms && bimanual(wep)) {
        You("cannot wield a two-handed %s while wearing a shield.",
            is_sword(wep) ? "sword" : wep->otyp == BATTLE_AXE ? "axe"
                                                              : "weapon");
    } else if (!retouch_object(&wep, FALSE)) {
        res++; /* takes a turn even though it doesn't get wielded */
    } else {
        /* Weapon WILL be wielded after this point */
        res++;
        if (will_weld(wep)) {
            const char *tmp = xname(wep), *thestr = "The ";

            if (strncmp(tmp, thestr, 4) && !strncmp(The(tmp), thestr, 4))
                tmp = thestr;
            else
                tmp = "";
            pline("%s%s %s to your %s!", tmp, aobjnam(wep, "weld"),
                  (wep->quan == 1L) ? "itself" : "themselves", /* a3 */
                  bimanual(wep) ? (const char *) makeplural(body_part(HAND))
                                : body_part(HAND));
            wep->bknown = TRUE;
        } else {
            /* The message must be printed before setuwep (since
             * you might die and be revived from changing weapons),
             * and the message must be before the death message and
             * Lifesaved rewielding.  Yet we want the message to
             * say "weapon in hand", thus this kludge.
             * [That comment is obsolete.  It dates from the days (3.0)
             * when unwielding Firebrand could cause hero to be burned
             * to death in Hell due to loss of fire resistance.
             * "Lifesaved re-wielding or re-wearing" is ancient history.]
             */
            long dummy = wep->owornmask;

            wep->owornmask |= W_WEP;
            if (wep->otyp == AKLYS && (wep->owornmask & W_WEP) != 0)
                You("secure the tether.");
            prinv((char *) 0, wep, 0L);
            wep->owornmask = dummy;
        }
        setuwep(wep);

        /* KMH -- Talking artifacts are finally implemented */
        arti_speak(wep);

        if (artifact_light(wep) && !wep->lamplit) {
            begin_burn(wep, FALSE);
            if (!Blind)
                pline("%s to shine %s!", Tobjnam(wep, "begin"),
                      arti_light_description(wep));
        }
#if 0
        /* we'll get back to this someday, but it's not balanced yet */
        if (Race_if(PM_ELF) && !wep->oartifact
            && objects[wep->otyp].oc_material == IRON) {
            /* Elves are averse to wielding cold iron */
            You("have an uneasy feeling about wielding cold iron.");
            change_luck(-1);
        }
#endif
        if (wep->unpaid) {
            struct monst *this_shkp;

            if ((this_shkp = shop_keeper(inside_shop(u.ux, u.uy)))
                != (struct monst *) 0) {
                pline("%s says \"You be careful with my %s!\"",
                      shkname(this_shkp), xname(wep));
            }
        }
    }
    return res;
}

void
setuqwep(obj)
register struct obj *obj;
{
    setworn(obj, W_QUIVER);
    /* no extra handling needed; this used to include a call to
       update_inventory() but that's already performed by setworn() */
    return;
}

void
setuswapwep(obj)
register struct obj *obj;
{
    setworn(obj, W_SWAPWEP);
    return;
}

/*** Commands to change particular slot(s) ***/

static NEARDATA const char wield_objs[] = {
    ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, TOOL_CLASS, 0
};
static NEARDATA const char ready_objs[] = {
    ALLOW_COUNT, COIN_CLASS, ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, 0
};
static NEARDATA const char bullets[] = { /* (note: different from dothrow.c) */
    ALLOW_COUNT, COIN_CLASS, ALL_CLASSES, ALLOW_NONE,
    GEM_CLASS, WEAPON_CLASS, 0
};

int
dowield()
{
    register struct obj *wep, *oldwep;
    int result;

    /* May we attempt this? */
    multi = 0;
    if (cantwield(youmonst.data)) {
        pline("Don't be ridiculous!");
        return 0;
    }

    /* Prompt for a new weapon */
    if (!(wep = getobj(wield_objs, "wield")))
        /* Cancelled */
        return 0;
    else if (wep == uwep) {
        You("are already wielding that!");
        if (is_weptool(wep) || is_wet_towel(wep))
            unweapon = FALSE; /* [see setuwep()] */
        return 0;
    } else if (welded(uwep)) {
        weldmsg(uwep);
        /* previously interrupted armor removal mustn't be resumed */
        reset_remarm();
        return 0;
    }

    /* Handle no object, or object in other slot */
    if (wep == &zeroobj)
        wep = (struct obj *) 0;
    else if (wep == uswapwep)
        return doswapweapon();
    else if (wep == uquiver)
        setuqwep((struct obj *) 0);
    else if (wep->owornmask & (W_ARMOR | W_ACCESSORY | W_SADDLE)) {
        You("cannot wield that!");
        return 0;
    }

    /* Set your new primary weapon */
    oldwep = uwep;
    result = ready_weapon(wep);
    if (flags.pushweapon && oldwep && uwep != oldwep)
        setuswapwep(oldwep);
    untwoweapon();

    return result;
}

int
doswapweapon()
{
    register struct obj *oldwep, *oldswap;
    int result = 0;

    /* May we attempt this? */
    multi = 0;
    if (cantwield(youmonst.data)) {
        pline("Don't be ridiculous!");
        return 0;
    }
    if (welded(uwep)) {
        weldmsg(uwep);
        return 0;
    }

    /* Unwield your current secondary weapon */
    oldwep = uwep;
    oldswap = uswapwep;
    setuswapwep((struct obj *) 0);

    /* Set your new primary weapon */
    result = ready_weapon(oldswap);

    /* Set your new secondary weapon */
    if (uwep == oldwep) {
        /* Wield failed for some reason */
        setuswapwep(oldswap);
    } else {
        setuswapwep(oldwep);
        if (uswapwep)
            prinv((char *) 0, uswapwep, 0L);
        else
            You("have no secondary weapon readied.");
    }

    if (u.twoweap && !can_twoweapon())
        untwoweapon();

    return result;
}

int
dowieldquiver()
{
    char qbuf[QBUFSZ];
    struct obj *newquiver;
    const char *quivee_types;
    int res;
    boolean finish_splitting = FALSE,
            was_uwep = FALSE, was_twoweap = u.twoweap;

    /* Since the quiver isn't in your hands, don't check cantwield(), */
    /* will_weld(), touch_petrifies(), etc. */
    multi = 0;
    /* forget last splitobj() before calling getobj() with ALLOW_COUNT */
    context.objsplit.child_oid = context.objsplit.parent_oid = 0;

    /* Prompt for a new quiver: "What do you want to ready?"
       (Include gems/stones as likely candidates if either primary
       or secondary weapon is a sling.) */
    quivee_types = (uslinging()
                    || (uswapwep
                        && objects[uswapwep->otyp].oc_skill == P_SLING))
                   ? bullets
                   : ready_objs;
    newquiver = getobj(quivee_types, "ready");

    if (!newquiver) {
        /* Cancelled */
        return 0;
    } else if (newquiver == &zeroobj) { /* no object */
        /* Explicitly nothing */
        if (uquiver) {
            You("now have no ammunition readied.");
            /* skip 'quivering: prinv()' */
            setuqwep((struct obj *) 0);
        } else {
            You("already have no ammunition readied!");
        }
        return 0;
    } else if (newquiver->o_id == context.objsplit.child_oid) {
        /* if newquiver is the result of supplying a count to getobj()
           we don't want to split something already in the quiver;
           for any other item, we need to give it its own inventory slot */
        if (uquiver && uquiver->o_id == context.objsplit.parent_oid) {
            unsplitobj(newquiver);
            goto already_quivered;
        }
        finish_splitting = TRUE;
    } else if (newquiver == uquiver) {
    already_quivered:
        pline("That ammunition is already readied!");
        return 0;
    } else if (newquiver->owornmask & (W_ARMOR | W_ACCESSORY | W_SADDLE)) {
        You("cannot ready that!");
        return 0;
    } else if (newquiver == uwep) {
        int weld_res = !uwep->bknown;

        if (welded(uwep)) {
            weldmsg(uwep);
            reset_remarm(); /* same as dowield() */
            return weld_res;
        }
        /* offer to split stack if wielding more than 1 */
        if (uwep->quan > 1L && inv_cnt(FALSE) < 52 && splittable(uwep)) {
            Sprintf(qbuf, "You are wielding %ld %s.  Ready %ld of them?",
                    uwep->quan, simpleonames(uwep), uwep->quan - 1L);
            switch (ynq(qbuf)) {
            case 'q':
                return 0;
            case 'y':
                /* leave 1 wielded, split rest off and put into quiver */
                newquiver = splitobj(uwep, uwep->quan - 1L);
                finish_splitting = TRUE;
                goto quivering;
            default:
                break;
            }
            Strcpy(qbuf, "Ready all of them instead?");
        } else {
            boolean use_plural = (is_plural(uwep) || pair_of(uwep));

            Sprintf(qbuf, "You are wielding %s.  Ready %s instead?",
                    !use_plural ? "that" : "those",
                    !use_plural ? "it" : "them");
        }
        /* require confirmation to ready the main weapon */
        if (ynq(qbuf) != 'y') {
            (void) Shk_Your(qbuf, uwep); /* replace qbuf[] contents */
            pline("%s%s %s wielded.", qbuf,
                  simpleonames(uwep), otense(uwep, "remain"));
            return 0;
        }
        /* quivering main weapon, so no longer wielding it */
        setuwep((struct obj *) 0);
        untwoweapon();
        was_uwep = TRUE;
    } else if (newquiver == uswapwep) {
        if (uswapwep->quan > 1L && inv_cnt(FALSE) < 52
            && splittable(uswapwep)) {
            Sprintf(qbuf, "%s %ld %s.  Ready %ld of them?",
                    u.twoweap ? "You are dual wielding"
                              : "Your alternate weapon is",
                    uswapwep->quan, simpleonames(uswapwep),
                    uswapwep->quan - 1L);
            switch (ynq(qbuf)) {
            case 'q':
                return 0;
            case 'y':
                /* leave 1 alt-wielded, split rest off and put into quiver */
                newquiver = splitobj(uswapwep, uswapwep->quan - 1L);
                finish_splitting = TRUE;
                goto quivering;
            default:
                break;
            }
            Strcpy(qbuf, "Ready all of them instead?");
        } else {
            boolean use_plural = (is_plural(uswapwep) || pair_of(uswapwep));

            Sprintf(qbuf, "%s your %s weapon.  Ready %s instead?",
                    !use_plural ? "That is" : "Those are",
                    u.twoweap ? "second" : "alternate",
                    !use_plural ? "it" : "them");
        }
        /* require confirmation to ready the alternate weapon */
        if (ynq(qbuf) != 'y') {
            (void) Shk_Your(qbuf, uswapwep); /* replace qbuf[] contents */
            pline("%s%s %s %s.", qbuf,
                  simpleonames(uswapwep), otense(uswapwep, "remain"),
                  u.twoweap ? "wielded" : "as secondary weapon");
            return 0;
        }
        /* quivering alternate weapon, so no more uswapwep */
        setuswapwep((struct obj *) 0);
        untwoweapon();
    }

 quivering:
    if (finish_splitting) {
        freeinv(newquiver);
        newquiver->nomerge = 1;
        addinv(newquiver);
        newquiver->nomerge = 0;
    }
    /* place item in quiver before printing so that inventory feedback
       includes "(at the ready)" */
    setuqwep(newquiver);
    prinv((char *) 0, newquiver, 0L);

    /* quiver is a convenience slot and manipulating it ordinarily
       consumes no time, but unwielding primary or secondary weapon
       should take time (perhaps we're adjacent to a rust monster
       or disenchanter and want to hit it immediately, but not with
       something we're wielding that's vulnerable to its damage) */
    res = 0;
    if (was_uwep) {
        You("are now empty %s.", body_part(HANDED));
        res = 1;
    } else if (was_twoweap && !u.twoweap) {
        You("are no longer wielding two weapons at once.");
        res = 1;
    }
    return res;
}

/* used for #rub and for applying pick-axe, whip, grappling hook or polearm */
boolean
wield_tool(obj, verb)
struct obj *obj;
const char *verb; /* "rub",&c */
{
    const char *what;
    boolean more_than_1;

    if (obj == uwep)
        return TRUE; /* nothing to do if already wielding it */

    if (!verb)
        verb = "wield";
    what = xname(obj);
    more_than_1 = (obj->quan > 1L || strstri(what, "pair of ") != 0
                   || strstri(what, "s of ") != 0);

    if (obj->owornmask & (W_ARMOR | W_ACCESSORY)) {
        You_cant("%s %s while wearing %s.", verb, yname(obj),
                 more_than_1 ? "them" : "it");
        return FALSE;
    }
    if (welded(uwep)) {
        if (flags.verbose) {
            const char *hand = body_part(HAND);

            if (bimanual(uwep))
                hand = makeplural(hand);
            if (strstri(what, "pair of ") != 0)
                more_than_1 = FALSE;
            pline(
               "Since your weapon is welded to your %s, you cannot %s %s %s.",
                  hand, verb, more_than_1 ? "those" : "that", xname(obj));
        } else {
            You_cant("do that.");
        }
        return FALSE;
    }
    if (cantwield(youmonst.data)) {
        You_cant("hold %s strongly enough.", more_than_1 ? "them" : "it");
        return FALSE;
    }
    /* check shield */
    if (uarms && bimanual(obj)) {
        You("cannot %s a two-handed %s while wearing a shield.", verb,
            (obj->oclass == WEAPON_CLASS) ? "weapon" : "tool");
        return FALSE;
    }

    if (uquiver == obj)
        setuqwep((struct obj *) 0);
    if (uswapwep == obj) {
        (void) doswapweapon();
        /* doswapweapon might fail */
        if (uswapwep == obj)
            return FALSE;
    } else {
        struct obj *oldwep = uwep;

        You("now wield %s.", doname(obj));
        setuwep(obj);
        if (flags.pushweapon && oldwep && uwep != oldwep)
            setuswapwep(oldwep);
    }
    if (uwep != obj)
        return FALSE; /* rewielded old object after dying */
    /* applying weapon or tool that gets wielded ends two-weapon combat */
    if (u.twoweap)
        untwoweapon();
    if (obj->oclass != WEAPON_CLASS)
        unweapon = TRUE;
    return TRUE;
}

int
can_twoweapon()
{
    struct obj *otmp;

#define NOT_WEAPON(obj) (!is_weptool(obj) && obj->oclass != WEAPON_CLASS)
    if (!could_twoweap(youmonst.data)) {
        if (Upolyd)
            You_cant("use two weapons in your current form.");
        else
            pline("%s aren't able to use two weapons at once.",
                  makeplural((flags.female && urole.name.f) ? urole.name.f
                                                            : urole.name.m));
    } else if (!uwep || !uswapwep)
        Your("%s%s%s empty.", uwep ? "left " : uswapwep ? "right " : "",
             body_part(HAND), (!uwep && !uswapwep) ? "s are" : " is");
    else if (NOT_WEAPON(uwep) || NOT_WEAPON(uswapwep)) {
        otmp = NOT_WEAPON(uwep) ? uwep : uswapwep;
        pline("%s %s.", Yname2(otmp),
              is_plural(otmp) ? "aren't weapons" : "isn't a weapon");
    } else if (bimanual(uwep) || bimanual(uswapwep)) {
        otmp = bimanual(uwep) ? uwep : uswapwep;
        pline("%s isn't one-handed.", Yname2(otmp));
    } else if (uarms)
        You_cant("use two weapons while wearing a shield.");
    else if (uswapwep->oartifact)
        pline("%s being held second to another weapon!",
              Yobjnam2(uswapwep, "resist"));
    else if (uswapwep->otyp == CORPSE && cant_wield_corpse(uswapwep)) {
        /* [Note: NOT_WEAPON() check prevents ever getting here...] */
        ; /* must be life-saved to reach here; return FALSE */
    } else if (Glib || uswapwep->cursed) {
        if (!Glib)
            uswapwep->bknown = TRUE;
        drop_uswapwep();
    } else
        return TRUE;
    return FALSE;
}

void
drop_uswapwep()
{
    char str[BUFSZ];
    struct obj *obj = uswapwep;

    /* Avoid trashing makeplural's static buffer */
    Strcpy(str, makeplural(body_part(HAND)));
    pline("%s from your %s!", Yobjnam2(obj, "slip"), str);
    dropx(obj);
}

int
dotwoweapon()
{
    /* You can always toggle it off */
    if (u.twoweap) {
        You("switch to your primary weapon.");
        u.twoweap = 0;
        update_inventory();
        return 0;
    }

    /* May we use two weapons? */
    if (can_twoweapon()) {
        /* Success! */
        You("begin two-weapon combat.");
        u.twoweap = 1;
        update_inventory();
        return (rnd(20) > ACURR(A_DEX));
    }
    return 0;
}

/*** Functions to empty a given slot ***/
/* These should be used only when the item can't be put back in
 * the slot by life saving.  Proper usage includes:
 * 1.  The item has been eaten, stolen, burned away, or rotted away.
 * 2.  Making an item disappear for a bones pile.
 */
void
uwepgone()
{
    if (uwep) {
        if (artifact_light(uwep) && uwep->lamplit) {
            end_burn(uwep, FALSE);
            if (!Blind)
                pline("%s shining.", Tobjnam(uwep, "stop"));
        }
        setworn((struct obj *) 0, W_WEP);
        unweapon = TRUE;
        update_inventory();
    }
}

void
uswapwepgone()
{
    if (uswapwep) {
        setworn((struct obj *) 0, W_SWAPWEP);
        update_inventory();
    }
}

void
uqwepgone()
{
    if (uquiver) {
        setworn((struct obj *) 0, W_QUIVER);
        update_inventory();
    }
}

void
untwoweapon()
{
    if (u.twoweap) {
        You("can no longer use two weapons at once.");
        u.twoweap = FALSE;
        update_inventory();
    }
    return;
}

int
chwepon(otmp, amount)
register struct obj *otmp;
register int amount;
{
    const char *color = hcolor((amount < 0) ? NH_BLACK : NH_BLUE);
    const char *xtime, *wepname = "";
    boolean multiple;
    int otyp = STRANGE_OBJECT;

    if (!uwep || (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))) {
        char buf[BUFSZ];

        if (amount >= 0 && uwep && will_weld(uwep)) { /* cursed tin opener */
            if (!Blind) {
                Sprintf(buf, "%s with %s aura.",
                        Yobjnam2(uwep, "glow"), an(hcolor(NH_AMBER)));
                uwep->bknown = !Hallucination;
            } else {
                /* cursed tin opener is wielded in right hand */
                Sprintf(buf, "Your right %s tingles.", body_part(HAND));
            }
            uncurse(uwep);
            update_inventory();
        } else {
            Sprintf(buf, "Your %s %s.", makeplural(body_part(HAND)),
                    (amount >= 0) ? "twitch" : "itch");
        }
        strange_feeling(otmp, buf); /* pline()+docall()+useup() */
        exercise(A_DEX, (boolean) (amount >= 0));
        return 0;
    }

    if (otmp && otmp->oclass == SCROLL_CLASS)
        otyp = otmp->otyp;

    if (uwep->otyp == WORM_TOOTH && amount >= 0) {
        multiple = (uwep->quan > 1L);
        /* order: message, transformation, shop handling */
        Your("%s %s much sharper now.", simpleonames(uwep),
             multiple ? "fuse, and become" : "is");
        uwep->otyp = CRYSKNIFE;
        uwep->oerodeproof = 0;
        if (multiple) {
            uwep->quan = 1L;
            uwep->owt = weight(uwep);
        }
        if (uwep->cursed)
            uncurse(uwep);
        /* update shop bill to reflect new higher value */
        if (uwep->unpaid)
            alter_cost(uwep, 0L);
        if (otyp != STRANGE_OBJECT)
            makeknown(otyp);
        if (multiple)
            encumber_msg();
        return 1;
    } else if (uwep->otyp == CRYSKNIFE && amount < 0) {
        multiple = (uwep->quan > 1L);
        /* order matters: message, shop handling, transformation */
        Your("%s %s much duller now.", simpleonames(uwep),
             multiple ? "fuse, and become" : "is");
        costly_alteration(uwep, COST_DEGRD); /* DECHNT? other? */
        uwep->otyp = WORM_TOOTH;
        uwep->oerodeproof = 0;
        if (multiple) {
            uwep->quan = 1L;
            uwep->owt = weight(uwep);
        }
        if (otyp != STRANGE_OBJECT && otmp->bknown)
            makeknown(otyp);
        if (multiple)
            encumber_msg();
        return 1;
    }

    if (has_oname(uwep))
        wepname = ONAME(uwep);
    if (amount < 0 && uwep->oartifact && restrict_name(uwep, wepname)) {
        if (!Blind)
            pline("%s %s.", Yobjnam2(uwep, "faintly glow"), color);
        return 1;
    }
    /* there is a (soft) upper and lower limit to uwep->spe */
    if (((uwep->spe > 5 && amount >= 0) || (uwep->spe < -5 && amount < 0))
        && rn2(3)) {
        if (!Blind)
            pline("%s %s for a while and then %s.",
                  Yobjnam2(uwep, "violently glow"), color,
                  otense(uwep, "evaporate"));
        else
            pline("%s.", Yobjnam2(uwep, "evaporate"));

        useupall(uwep); /* let all of them disappear */
        return 1;
    }
    if (!Blind) {
        xtime = (amount * amount == 1) ? "moment" : "while";
        pline("%s %s for a %s.",
              Yobjnam2(uwep, amount == 0 ? "violently glow" : "glow"), color,
              xtime);
        if (otyp != STRANGE_OBJECT && uwep->known
            && (amount > 0 || (amount < 0 && otmp->bknown)))
            makeknown(otyp);
    }
    if (amount < 0)
        costly_alteration(uwep, COST_DECHNT);
    uwep->spe += amount;
    if (amount > 0) {
        if (uwep->cursed)
            uncurse(uwep);
        /* update shop bill to reflect new higher price */
        if (uwep->unpaid)
            alter_cost(uwep, 0L);
    }

    /*
     * Enchantment, which normally improves a weapon, has an
     * addition adverse reaction on Magicbane whose effects are
     * spe dependent.  Give an obscure clue here.
     */
    if (uwep->oartifact == ART_MAGICBANE && uwep->spe >= 0) {
        Your("right %s %sches!", body_part(HAND),
             (((amount > 1) && (uwep->spe > 1)) ? "flin" : "it"));
    }

    /* an elven magic clue, cookie@keebler */
    /* elven weapons vibrate warningly when enchanted beyond a limit */
    if ((uwep->spe > 5)
        && (is_elven_weapon(uwep) || uwep->oartifact || !rn2(7)))
        pline("%s unexpectedly.", Yobjnam2(uwep, "suddenly vibrate"));

    return 1;
}

int
welded(obj)
register struct obj *obj;
{
    if (obj && obj == uwep && will_weld(obj)) {
        obj->bknown = TRUE;
        return 1;
    }
    return 0;
}

void
weldmsg(obj)
register struct obj *obj;
{
    long savewornmask;

    savewornmask = obj->owornmask;
    pline("%s welded to your %s!", Yobjnam2(obj, "are"),
          bimanual(obj) ? (const char *) makeplural(body_part(HAND))
                        : body_part(HAND));
    obj->owornmask = savewornmask;
}

/* test whether monster's wielded weapon is stuck to hand/paw/whatever */
boolean
mwelded(obj)
struct obj *obj;
{
    /* caller is responsible for making sure this is a monster's item */
    if (obj && (obj->owornmask & W_WEP) && will_weld(obj))
        return TRUE;
    return FALSE;
}

/*wield.c*/

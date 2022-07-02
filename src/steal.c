/* NetHack 3.7	steal.c	$NHDT-Date: 1646688070 2022/03/07 21:21:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.98 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int stealarm(void);
static int unstolenarm(void);
static const char *equipname(struct obj *);

static const char *
equipname(register struct obj* otmp)
{
    return ((otmp == uarmu) ? shirt_simple_name(otmp)
            : (otmp == uarmf) ? boots_simple_name(otmp)
              : (otmp == uarms) ? shield_simple_name(otmp)
                : (otmp == uarmg) ? gloves_simple_name(otmp)
                  : (otmp == uarmc) ? cloak_simple_name(otmp)
                    : (otmp == uarmh) ? helm_simple_name(otmp)
                      : suit_simple_name(otmp));
}

/* proportional subset of gold; return value actually fits in an int */
long
somegold(long lmoney)
{
#ifdef LINT /* long conv. ok */
    int igold = 0;
#else
    int igold = (lmoney >= (long) LARGEST_INT) ? LARGEST_INT : (int) lmoney;
#endif

    if (igold < 50)
        ; /* all gold */
    else if (igold < 100)
        igold = rn1(igold - 25 + 1, 25);
    else if (igold < 500)
        igold = rn1(igold - 50 + 1, 50);
    else if (igold < 1000)
        igold = rn1(igold - 100 + 1, 100);
    else if (igold < 5000)
        igold = rn1(igold - 500 + 1, 500);
    else if (igold < 10000)
        igold = rn1(igold - 1000 + 1, 1000);
    else
        igold = rn1(igold - 5000 + 1, 5000);

    return (long) igold;
}

/*
 * Find the first (and hopefully only) gold object in a chain.
 * Used when leprechaun (or you as leprechaun) looks for
 * someone else's gold.  Returns a pointer so the gold may
 * be seized without further searching.
 * May search containers too.
 * Deals in gold only, as leprechauns don't care for lesser coins.
*/
struct obj *
findgold(register struct obj* chain)
{
    while (chain && chain->otyp != GOLD_PIECE)
        chain = chain->nobj;
    return chain;
}

/*
 * Steal gold coins only.  Leprechauns don't care for lesser coins.
*/
void
stealgold(register struct monst* mtmp)
{
    register struct obj *fgold = g_at(u.ux, u.uy);
    register struct obj *ygold;
    register long tmp;
    struct monst *who;
    const char *whose, *what;

    /* skip lesser coins on the floor */
    while (fgold && fgold->otyp != GOLD_PIECE)
        fgold = fgold->nexthere;

    /* Do you have real gold? */
    ygold = findgold(g.invent);

    if (fgold && (!ygold || fgold->quan > ygold->quan || !rn2(5))) {
        obj_extract_self(fgold);
        add_to_minv(mtmp, fgold);
        newsym(u.ux, u.uy);
        if (u.usteed) {
            who = u.usteed;
            whose = s_suffix(y_monnam(who));
            what = makeplural(mbodypart(who, FOOT));
        } else {
            who = &g.youmonst;
            whose = "your";
            what = makeplural(body_part(FOOT));
        }
        /* [ avoid "between your rear regions" :-] */
        if (slithy(who->data))
            what = "coils";
        /* reduce "rear hooves/claws" to "hooves/claws" */
        if (!strncmp(what, "rear ", 5))
            what += 5;
        pline("%s quickly snatches some gold from %s %s %s!", Monnam(mtmp),
              (Levitation || Flying) ? "beneath" : "between", whose, what);
        if (!ygold || !rn2(5)) {
            if (!tele_restrict(mtmp))
                (void) rloc(mtmp, RLOC_MSG);
            monflee(mtmp, 0, FALSE, FALSE);
        }
    } else if (ygold) {
        const int gold_price = objects[GOLD_PIECE].oc_cost;

        tmp = (somegold(money_cnt(g.invent)) + gold_price - 1) / gold_price;
        tmp = min(tmp, ygold->quan);
        if (tmp < ygold->quan)
            ygold = splitobj(ygold, tmp);
        else
            setnotworn(ygold);
        freeinv(ygold);
        add_to_minv(mtmp, ygold);
        Your("purse feels lighter.");
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, RLOC_MSG);
        monflee(mtmp, 0, FALSE, FALSE);
        g.context.botl = 1;
    }
}

/* monster who was stealing from hero has just died */
void
thiefdead(void)
{
    /* hero is busy taking off an item of armor which takes multiple turns */
    g.stealmid = 0;
    if (g.afternmv == stealarm) {
        g.afternmv = unstolenarm;
        g.nomovemsg = (char *) 0;
    }
}

/* called via (*g.afternmv)() when hero finishes taking off armor that
   was slated to be stolen but the thief died in the interim */
static int
unstolenarm(void)
{
    struct obj *obj;

    /* find the object before clearing stealoid; it has already become
       not-worn and is still in hero's inventory */
    for (obj = g.invent; obj; obj = obj->nobj)
        if (obj->o_id == g.stealoid)
            break;
    g.stealoid = 0;
    if (obj) {
        You("finish taking off your %s.", equipname(obj));
    }
    return 0;
}

static int
stealarm(void)
{
    register struct monst *mtmp;
    register struct obj *otmp;

    if (!g.stealoid || !g.stealmid)
        goto botm;

    for (otmp = g.invent; otmp; otmp = otmp->nobj) {
        if (otmp->o_id == g.stealoid) {
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (mtmp->m_id == g.stealmid) {
                    if (DEADMONSTER(mtmp))
                        impossible("stealarm(): dead monster stealing");
                    if (!dmgtype(mtmp->data, AD_SITM)) /* polymorphed */
                        goto botm;
                    if (otmp->unpaid)
                        subfrombill(otmp, shop_keeper(*u.ushops));
                    freeinv(otmp);
                    pline("%s steals %s!", Monnam(mtmp), doname(otmp));
                    (void) mpickobj(mtmp, otmp); /* may free otmp */
                    /* Implies seduction, "you gladly hand over ..."
                       so we don't set mavenge bit here. */
                    monflee(mtmp, 0, FALSE, FALSE);
                    if (!tele_restrict(mtmp))
                        (void) rloc(mtmp, RLOC_MSG);
                    break;
                }
            }
            break;
        }
    }
 botm:
    g.stealoid = g.stealmid = 0; /* in case only one has been reset so far */
    return 0;
}

/* An object you're wearing has been taken off by a monster (theft or
   seduction).  Also used if a worn item gets transformed (stone to flesh). */
void
remove_worn_item(
    struct obj *obj,
    boolean unchain_ball) /* whether to unpunish or just unwield */
{
    if (donning(obj))
        cancel_don();
    if (!obj->owornmask)
        return;

    if (obj->owornmask & W_ARMOR) {
        if (obj == uskin) {
            impossible("Removing embedded scales?");
            skinback(TRUE); /* uarm = uskin; uskin = 0; */
        }
        if (obj == uarm)
            (void) Armor_off();
        else if (obj == uarmc)
            (void) Cloak_off();
        else if (obj == uarmf)
            (void) Boots_off();
        else if (obj == uarmg)
            (void) Gloves_off();
        else if (obj == uarmh)
            (void) Helmet_off();
        else if (obj == uarms)
            (void) Shield_off();
        else if (obj == uarmu)
            (void) Shirt_off();
        /* catchall -- should never happen */
        else
            setworn((struct obj *) 0, obj->owornmask & W_ARMOR);
    } else if (obj->owornmask & W_AMUL) {
        Amulet_off();
    } else if (obj->owornmask & W_RING) {
        Ring_gone(obj);
    } else if (obj->owornmask & W_TOOL) {
        Blindf_off(obj);
    } else if (obj->owornmask & W_WEAPONS) {
        if (obj == uwep)
            uwepgone();
        if (obj == uswapwep)
            uswapwepgone();
        if (obj == uquiver)
            uqwepgone();
    }

    if (obj->owornmask & (W_BALL | W_CHAIN)) {
        if (unchain_ball)
            unpunish();
    } else if (obj->owornmask) {
        /* catchall */
        setnotworn(obj);
    }
}

/* Returns 1 when something was stolen (or at least, when N should flee now),
 * returns -1 if the monster died in the attempt.
 * Avoid stealing the object 'stealoid'.
 * Nymphs and monkeys won't steal coins.
 */
int
steal(struct monst* mtmp, char* objnambuf)
{
    struct obj *otmp;
    int tmp, could_petrify, armordelay, olddelay, icnt,
        named = 0, retrycnt = 0;
    boolean monkey_business, /* true iff an animal is doing the thievery */
            was_doffing, was_punished = Punished;

    if (objnambuf)
        *objnambuf = '\0';
    /* the following is true if successful on first of two attacks. */
    if (!monnear(mtmp, u.ux, u.uy))
        return 0;

    /* food being eaten might already be used up but will not have
       been removed from inventory yet; we don't want to steal that,
       so this will cause it to be removed now */
    if (g.occupation)
        (void) maybe_finished_meal(FALSE);

    icnt = inv_cnt(FALSE); /* don't include gold */
    if (!icnt || (icnt == 1 && uskin)) {
 nothing_to_steal:
        /* Not even a thousand men in armor can strip a naked man. */
        if (Blind)
            pline("Somebody tries to rob you, but finds nothing to steal.");
        else if (inv_cnt(TRUE) > inv_cnt(FALSE)) /* ('icnt' might be stale) */
            pline("%s tries to rob you, but isn't interested in gold.",
                  Monnam(mtmp));
        else
            pline("%s tries to rob you, but there is nothing to steal!",
                  Monnam(mtmp));
        return 1; /* let her flee */
    }

    monkey_business = is_animal(mtmp->data);
    if (monkey_business || uarmg) {
        ; /* skip ring special cases */
    } else if (Adornment & LEFT_RING) {
        otmp = uleft;
        goto gotobj;
    } else if (Adornment & RIGHT_RING) {
        otmp = uright;
        goto gotobj;
    }

 retry:
    tmp = 0;
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if ((!uarm || otmp != uarmc) && otmp != uskin
            && otmp->oclass != COIN_CLASS)
            tmp += (otmp->owornmask & (W_ARMOR | W_ACCESSORY)) ? 5 : 1;
    if (!tmp)
        goto nothing_to_steal;
    tmp = rn2(tmp);
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if ((!uarm || otmp != uarmc) && otmp != uskin
            && otmp->oclass != COIN_CLASS) {
            tmp -= (otmp->owornmask & (W_ARMOR | W_ACCESSORY)) ? 5 : 1;
            if (tmp < 0)
                break;
        }
    if (!otmp) {
        impossible("Steal fails!");
        return 0;
    }
    /* can't steal ring(s) while wearing gloves */
    if ((otmp == uleft || otmp == uright) && uarmg)
        otmp = uarmg;
    /* can't steal gloves while wielding - so steal the wielded item. */
    if (otmp == uarmg && uwep)
        otmp = uwep;
    /* can't steal armor while wearing cloak - so steal the cloak. */
    else if (otmp == uarm && uarmc)
        otmp = uarmc;
    /* can't steal shirt while wearing cloak or suit */
    else if (otmp == uarmu && uarmc)
        otmp = uarmc;
    else if (otmp == uarmu && uarm)
        otmp = uarm;

 gotobj:
    if (otmp->o_id == g.stealoid)
        return 0;

    if (otmp->otyp == BOULDER && !throws_rocks(mtmp->data)) {
        if (!retrycnt++)
            goto retry;
        goto cant_take;
    }
    /* animals can't overcome curse stickiness nor unlock chains */
    if (monkey_business) {
        boolean ostuck;

        /* is the player prevented from voluntarily giving up this item?
           (ignores loadstones; the !can_carry() check will catch those) */
        if (otmp == uball)
            ostuck = TRUE; /* effectively worn; curse is implicit */
        else if (otmp == uquiver || (otmp == uswapwep && !u.twoweap))
            ostuck = FALSE; /* not really worn; curse doesn't matter */
        else
            ostuck = ((otmp->cursed && otmp->owornmask)
                      /* nymphs can steal rings from under
                         cursed weapon but animals can't */
                      || (otmp == uright && welded(uwep))
                      || (otmp == uleft && welded(uwep) && bimanual(uwep)));

        if (ostuck || can_carry(mtmp, otmp) == 0) {
            static const char *const how[] = { "steal", "snatch", "grab",
                                               "take" };
 cant_take:
            pline("%s tries to %s %s%s but gives up.", Monnam(mtmp),
                  how[rn2(SIZE(how))],
                  (otmp->owornmask & W_ARMOR) ? "your " : "",
                  (otmp->owornmask & W_ARMOR) ? equipname(otmp)
                                              : yname(otmp));
            /* the fewer items you have, the less likely the thief
               is going to stick around to try again (0) instead of
               running away (1) */
            return !rn2(inv_cnt(FALSE) / 5 + 2);
        }
    }

    if (otmp->otyp == LEASH && otmp->leashmon) {
        if (monkey_business && otmp->cursed)
            goto cant_take;
        o_unleash(otmp);
    }

    was_doffing = doffing(otmp);
    /* stop donning/doffing now so that afternmv won't be clobbered
       below; stop_occupation doesn't handle donning/doffing */
    olddelay = stop_donning(otmp);
    /* you're going to notice the theft... */
    stop_occupation();

    if (otmp->owornmask & (W_ARMOR | W_ACCESSORY)) {
        switch (otmp->oclass) {
        case TOOL_CLASS:
        case AMULET_CLASS:
        case RING_CLASS:
        case FOOD_CLASS: /* meat ring */
            remove_worn_item(otmp, TRUE);
            break;
        case ARMOR_CLASS:
            armordelay = objects[otmp->otyp].oc_delay;
            if (olddelay > 0 && olddelay < armordelay)
                armordelay = olddelay;
            if (monkey_business) {
                /* animals usually don't have enough patience
                   to take off items which require extra time */
                if (armordelay >= 1 && !olddelay && rn2(10))
                    goto cant_take;
                remove_worn_item(otmp, TRUE);
                break;
            } else {
                int curssv = otmp->cursed;
                int slowly;
                boolean seen = canspotmon(mtmp);

                otmp->cursed = 0;
                /* can't charm you without first waking you */
                if (Unaware)
                    unmul((char *) 0);
                slowly = (armordelay >= 1 || g.multi < 0);
                if (flags.female)
                    urgent_pline("%s charms you.  You gladly %s your %s.",
                                 !seen ? "She" : Monnam(mtmp),
                                 curssv ? "let her take"
                                 : !slowly ? "hand over"
                                   : was_doffing ? "continue removing"
                                     : "start removing",
                                 equipname(otmp));
                else
                    urgent_pline("%s seduces you and %s off your %s.",
                                 !seen ? "She" : Adjmonnam(mtmp, "beautiful"),
                                 curssv ? "helps you to take"
                                 : !slowly ? "you take"
                                   : was_doffing ? "you continue taking"
                                     : "you start taking",
                                 equipname(otmp));
                named++;
                /* the following is to set multi for later on */
                nomul(-armordelay);
                g.multi_reason = "taking off clothes";
                g.nomovemsg = 0;
                remove_worn_item(otmp, TRUE);
                otmp->cursed = curssv;
                if (g.multi < 0) {
                    g.stealoid = otmp->o_id;
                    g.stealmid = mtmp->m_id;
                    g.afternmv = stealarm;
                    return 0;
                }
            }
            break;
        default:
            impossible("Tried to steal a strange worn thing. [%d]",
                       otmp->oclass);
        }
    } else if (otmp->owornmask) /* weapon or ball&chain */
        remove_worn_item(otmp, TRUE);

    /* do this before removing it from inventory */
    if (objnambuf)
        Strcpy(objnambuf, yname(otmp));
    /* usually set mavenge bit so knights won't suffer an alignment penalty
       during retaliation; not applicable for removing attached iron ball */
    if (!Conflict && !(was_punished && !Punished))
        mtmp->mavenge = 1;

    if (otmp->unpaid)
        subfrombill(otmp, shop_keeper(*u.ushops));
    freeinv(otmp);
    /* if attached ball was taken, uball and uchain are now Null */
    urgent_pline("%s%s stole %s.", named ? "She" : Monnam(mtmp),
                 (was_punished && !Punished) ? " removed your chain and" : "",
                 doname(otmp));
    (void) encumber_msg();
    could_petrify = (otmp->otyp == CORPSE
                     && touch_petrifies(&mons[otmp->corpsenm]));
    (void) mpickobj(mtmp, otmp); /* may free otmp */
    if (could_petrify && !(mtmp->misc_worn_check & W_ARMG)) {
        minstapetrify(mtmp, TRUE);
        return -1;
    }
    return (g.multi < 0) ? 0 : 1;
}

/* Returns 1 if otmp is free'd, 0 otherwise. */
int
mpickobj(struct monst *mtmp, struct obj *otmp)
{
    int freed_otmp;
    boolean snuff_otmp = FALSE;

    if (!otmp) {
        impossible("monster (%s) taking or picking up nothing?",
                   pmname(mtmp->data, Mgender(mtmp)));
        return 1;
    } else if (otmp == uball || otmp == uchain) {
        impossible("monster (%s) taking or picking up attached %s (%s)?",
                   pmname(mtmp->data, Mgender(mtmp)),
                   (otmp == uchain) ? "chain" : "ball", simpleonames(otmp));
        return 0;
    }
    /* if monster is acquiring a thrown or kicked object, the throwing
       or kicking code shouldn't continue to track and place it */
    if (otmp == g.thrownobj)
        g.thrownobj = 0;
    else if (otmp == g.kickedobj)
        g.kickedobj = 0;
    /* don't want hidden light source inside the monster; assumes that
       engulfers won't have external inventories; whirly monsters cause
       the light to be extinguished rather than letting it shine thru */
    if (obj_sheds_light(otmp) && attacktype(mtmp->data, AT_ENGL)) {
        /* this is probably a burning object that you dropped or threw */
        if (engulfing_u(mtmp) && !Blind)
            pline("%s out.", Tobjnam(otmp, "go"));
        snuff_otmp = TRUE;
    }
    /* for hero owned object on shop floor, mtmp is taking possession
       and if it's eventually dropped in a shop, shk will claim it */
    if (!mtmp->mtame)
        otmp->no_charge = 0;
    /* if monster is unseen, info hero knows about this object becomes lost;
       continual pickup and drop by pets makes this too annoying if it is
       applied to them; when engulfed (where monster can't be seen because
       vision is disabled), or when held (or poly'd and holding) while blind,
       behave as if the monster can be 'seen' by touch */
    if (!mtmp->mtame && !(canseemon(mtmp) || mtmp == u.ustuck))
        unknow_object(otmp);
    /* Must do carrying effects on object prior to add_to_minv() */
    carry_obj_effects(otmp);
    /* add_to_minv() might free otmp [if merged with something else],
       so we have to call it after doing the object checks */
    freed_otmp = add_to_minv(mtmp, otmp);
    /* and we had to defer this until object is in mtmp's inventory */
    if (snuff_otmp)
        snuff_light_source(mtmp->mx, mtmp->my);
    return freed_otmp;
}

/* called for AD_SAMU (the Wizard and quest nemeses) */
void
stealamulet(struct monst* mtmp)
{
    char buf[BUFSZ];
    struct obj *otmp = 0, *obj = 0;
    int real = 0, fake = 0, n;

    /* target every quest artifact, not just current role's;
       if hero has more than one, choose randomly so that player
       can't use inventory ordering to influence the theft */
    for (n = 0, obj = g.invent; obj; obj = obj->nobj)
        if (any_quest_artifact(obj))
            ++n, otmp = obj;
    if (n > 1) {
        n = rnd(n);
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (any_quest_artifact(otmp) && !--n)
                break;
    }

    if (!otmp) {
        /* if we didn't find any quest arifact, find another valuable item */
        if (u.uhave.amulet) {
            real = AMULET_OF_YENDOR;
            fake = FAKE_AMULET_OF_YENDOR;
        } else if (u.uhave.bell) {
            real = BELL_OF_OPENING;
            fake = BELL;
        } else if (u.uhave.book) {
            real = SPE_BOOK_OF_THE_DEAD;
        } else if (u.uhave.menorah) {
            real = CANDELABRUM_OF_INVOCATION;
        } else
            return; /* you have nothing of special interest */

        /* If we get here, real and fake have been set up. */
        for (n = 0, obj = g.invent; obj; obj = obj->nobj)
            if (obj->otyp == real || (obj->otyp == fake && !mtmp->iswiz))
                ++n, otmp = obj;
        if (n > 1) {
            n = rnd(n);
            for (otmp = g.invent; otmp; otmp = otmp->nobj)
                if ((otmp->otyp == real
                     || (otmp->otyp == fake && !mtmp->iswiz)) && !--n)
                    break;
        }
    }

    if (otmp) { /* we have something to snatch */
        /* take off outer gear if we're targetting [hypothetical]
           quest artifact suit, shirt, gloves, or rings */
        if ((otmp == uarm || otmp == uarmu) && uarmc)
            remove_worn_item(uarmc, FALSE);
        if (otmp == uarmu && uarm)
            remove_worn_item(uarm, FALSE);
        if ((otmp == uarmg || ((otmp == uright || otmp == uleft) && uarmg))
            && uwep) {
            /* gloves are about to be unworn; unwield weapon(s) first */
            if (u.twoweap)    /* remove_worn_item(uswapwep) indirectly */
                remove_worn_item(uswapwep, FALSE); /* clears u.twoweap */
            remove_worn_item(uwep, FALSE);
        }
        if ((otmp == uright || otmp == uleft) && uarmg)
            /* calls Gloves_off() to handle wielded cockatrice corpse */
            remove_worn_item(uarmg, FALSE);

        /* finally, steal the target item */
        if (otmp->owornmask)
            remove_worn_item(otmp, TRUE);
        if (otmp->unpaid)
            subfrombill(otmp, shop_keeper(*u.ushops));
        freeinv(otmp);
        Strcpy(buf, doname(otmp));
        (void) mpickobj(mtmp, otmp); /* could merge and free otmp but won't */
        pline("%s steals %s!", Monnam(mtmp), buf);
        if (can_teleport(mtmp->data) && !tele_restrict(mtmp))
            (void) rloc(mtmp, RLOC_MSG);
        (void) encumber_msg();
    }
}

/* when a mimic gets poked with something, it might take that thing
   (at present, only implemented for when the hero does the poking) */
void
maybe_absorb_item(
    struct monst *mon,
    struct obj *obj,
    int ochance, int achance) /* percent chance for ordinary item, artifact */
{
    if (obj == uball || obj == uchain || obj->oclass == ROCK_CLASS
        || obj_resists(obj, 100 - ochance, 100 - achance)
        || !touch_artifact(obj, mon))
        return;

    if (carried(obj)) {
        if (obj->owornmask)
            remove_worn_item(obj, TRUE);
        if (obj->unpaid)
            subfrombill(obj, shop_keeper(*u.ushops));
        if (cansee(mon->mx, mon->my)) {
            /* Some_Monnam() avoids "It pulls ... and absorbs it!"
               if hero can see the location but not the monster */
            pline("%s pulls %s away from you and absorbs %s!",
                  Some_Monnam(mon), /* Monnam() or "Something" */
                  yname(obj), (obj->quan > 1L) ? "them" : "it");
        } else {
            const char *hand_s = body_part(HAND);

            if (bimanual(obj))
                hand_s = makeplural(hand_s);
            pline("%s %s pulled from your %s!", upstart(yname(obj)),
                  otense(obj, "are"), hand_s);
        }
        freeinv(obj);
        (void) encumber_msg();
    } else {
        /* not carried; presumably thrown or kicked */
        if (canspotmon(mon))
            pline("%s absorbs %s!", Monnam(mon), yname(obj));
    }
    /* add to mon's inventory */
    (void) mpickobj(mon, obj);
}

/* drop one object taken from a (possibly dead) monster's inventory */
void
mdrop_obj(
    struct monst *mon,
    struct obj *obj,
    boolean verbosely)
{
    coordxy omx = mon->mx, omy = mon->my;
    long unwornmask = obj->owornmask;
    /* call distant_name() for its possible side-effects even if the result
       might not be printed, and do it before extracing obj from minvent */
    char *obj_name = distant_name(obj, doname);

    extract_from_minvent(mon, obj, FALSE, TRUE);
    /* don't charge for an owned saddle on dead steed (provided
        that the hero is within the same shop at the time) */
    if (unwornmask && mon->mtame && (unwornmask & W_SADDLE) != 0L
        && !obj->unpaid && costly_spot(omx, omy)
        /* being at costly_spot guarantees lev->roomno is not 0 */
        && index(in_rooms(u.ux, u.uy, SHOPBASE), levl[omx][omy].roomno)) {
        obj->no_charge = 1;
    }
    /* obj_no_longer_held(obj); -- done by place_object */
    if (verbosely && cansee(omx, omy))
        pline("%s drops %s.", Monnam(mon), obj_name);
    if (!flooreffects(obj, omx, omy, "fall")) {
        place_object(obj, omx, omy);
        stackobj(obj);
    }
    /* do this last, after placing obj on floor; removing steed's saddle
       throws rider, possibly inflicting fatal damage and producing bones; this
       is why we had to call extract_from_minvent() with do_intrinsics=FALSE */
    if (!DEADMONSTER(mon) && unwornmask)
        update_mon_intrinsics(mon, obj, FALSE, TRUE);
}

/* some monsters bypass the normal rules for moving between levels or
   even leaving the game entirely; when that happens, prevent them from
   taking the Amulet, invocation items, or quest artifact with them */
void
mdrop_special_objs(struct monst *mon)
{
    struct obj *obj, *otmp;

    for (obj = mon->minvent; obj; obj = otmp) {
        otmp = obj->nobj;
        /* the Amulet, invocation tools, and Rider corpses resist even when
           artifacts and ordinary objects are given 0% resistance chance;
           current role's quest artifact is rescued too--quest artifacts
           for the other roles are not */
        if (obj_resists(obj, 0, 0) || is_quest_artifact(obj)) {
            if (mon->mx) {
                mdrop_obj(mon, obj, FALSE);
            } else { /* migrating monster not on map */
                extract_from_minvent(mon, obj, TRUE, TRUE);
                rloco(obj);
            }
        }
    }
}

/* release the objects the creature is carrying */
void
relobj(
    struct monst *mtmp,
    int show,
    boolean is_pet) /* If true, pet should keep wielded/worn items */
{
    struct obj *otmp;
    int omx = mtmp->mx, omy = mtmp->my;

    /* vault guard's gold goes away rather than be dropped... */
    if (mtmp->isgd && (otmp = findgold(mtmp->minvent)) != 0) {
        if (canspotmon(mtmp))
            pline("%s gold %s.", s_suffix(Monnam(mtmp)),
                  canseemon(mtmp) ? "vanishes" : "seems to vanish");
        obj_extract_self(otmp);
        obfree(otmp, (struct obj *) 0);
    } /* isgd && has gold */

    while ((otmp = (is_pet ? droppables(mtmp) : mtmp->minvent)) != 0) {
        mdrop_obj(mtmp, otmp, is_pet && Verbose(1, relobj));
    }

    if (show && cansee(omx, omy))
        newsym(omx, omy);
}

/*steal.c*/

/* NetHack 3.7	write.c	$NHDT-Date: 1596498232 2020/08/03 23:43:52 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.26 $ */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int cost(struct obj *);
static boolean label_known(int, struct obj *);
static int write_ok(struct obj *);
static char *new_book_description(int, char *);

/*
 * returns basecost of a scroll or a spellbook
 */
static int
cost(struct obj *otmp)
{
    if (otmp->oclass == SPBOOK_CLASS)
        return (10 * objects[otmp->otyp].oc_level);

    switch (otmp->otyp) {
#ifdef MAIL_STRUCTURES
    case SCR_MAIL:
        return 2;
#endif
    case SCR_LIGHT:
    case SCR_GOLD_DETECTION:
    case SCR_FOOD_DETECTION:
    case SCR_MAGIC_MAPPING:
    case SCR_AMNESIA:
    case SCR_FIRE:
    case SCR_EARTH:
        return 8;
    case SCR_DESTROY_ARMOR:
    case SCR_CREATE_MONSTER:
    case SCR_PUNISHMENT:
        return 10;
    case SCR_CONFUSE_MONSTER:
        return 12;
    case SCR_IDENTIFY:
        return 14;
    case SCR_ENCHANT_ARMOR:
    case SCR_REMOVE_CURSE:
    case SCR_ENCHANT_WEAPON:
    case SCR_CHARGING:
        return 16;
    case SCR_SCARE_MONSTER:
    case SCR_STINKING_CLOUD:
    case SCR_TAMING:
    case SCR_TELEPORTATION:
        return 20;
    case SCR_GENOCIDE:
        return 30;
    case SCR_BLANK_PAPER:
    default:
        impossible("You can't write such a weird scroll!");
    }
    return 1000;
}

/* decide whether the hero knowns a particular scroll's label;
   unfortunately, we can't track things that haven't been added to
   the discoveries list and aren't present in current inventory,
   so some scrolls with ought to yield True will end up False */
static boolean
label_known(int scrolltype, struct obj *objlist)
{
    struct obj *otmp;

    /* only scrolls */
    if (objects[scrolltype].oc_class != SCROLL_CLASS)
        return FALSE;
    /* type known implies full discovery; otherwise,
       user-assigned name implies partial discovery */
    if (objects[scrolltype].oc_name_known || objects[scrolltype].oc_uname)
        return TRUE;
    /* check inventory, including carried containers with known contents */
    for (otmp = objlist; otmp; otmp = otmp->nobj) {
        if (otmp->otyp == scrolltype && otmp->dknown)
            return TRUE;
        if (Has_contents(otmp) && otmp->cknown
            && label_known(scrolltype, otmp->cobj))
            return TRUE;
    }
    /* not found */
    return FALSE;
}

/* getobj callback for object to write on */
static int
write_ok(struct obj *obj)
{
    if (!obj || (obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS))
        return GETOBJ_EXCLUDE;

    if (obj->otyp == SCR_BLANK_PAPER || obj->otyp == SPE_BLANK_PAPER)
        return GETOBJ_SUGGEST;

    return GETOBJ_DOWNPLAY;
}

/* write -- applying a magic marker */
int
dowrite(struct obj *pen)
{
    register struct obj *paper;
    char namebuf[BUFSZ] = DUMMY, *nm, *bp;
    register struct obj *new_obj;
    int basecost, actualcost;
    int curseval;
    char qbuf[QBUFSZ];
    int first, last, i, deferred, deferralchance, real;
    boolean by_descr = FALSE;
    const char *typeword;

    if (nohands(g.youmonst.data)) {
        You("need hands to be able to write!");
        return ECMD_OK;
    } else if (Glib) {
        pline("%s from your %s.", Tobjnam(pen, "slip"),
              fingers_or_gloves(FALSE));
        dropx(pen);
        return ECMD_TIME;
    }

    /* get paper to write on */
    paper = getobj("write on", write_ok, GETOBJ_NOFLAGS);
    if (!paper)
        return ECMD_CANCEL;
    /* can't write on a novel (unless/until it's been converted into a blank
       spellbook), but we want messages saying so to avoid "spellbook" */
    typeword = (paper->otyp == SPE_NOVEL) ? "book"
               : (paper->oclass == SPBOOK_CLASS) ? "spellbook"
                 : "scroll";
    if (Blind) {
        if (!paper->dknown) {
            You("don't know if that %s is blank or not.", typeword);
            return ECMD_OK;
        } else if (paper->oclass == SPBOOK_CLASS) {
            /* can't write a magic book while blind */
            pline("%s can't create braille text.",
                  upstart(ysimple_name(pen)));
            return ECMD_OK;
        }
    }
    paper->dknown = 1;
    if (paper->otyp != SCR_BLANK_PAPER && paper->otyp != SPE_BLANK_PAPER) {
        pline("That %s is not blank!", typeword);
        exercise(A_WIS, FALSE);
        return ECMD_TIME;
    }

    /* what to write */
    Sprintf(qbuf, "What type of %s do you want to write?", typeword);
    getlin(qbuf, namebuf);
    (void) mungspaces(namebuf); /* remove any excess whitespace */
    if (namebuf[0] == '\033' || !namebuf[0])
        return ECMD_TIME;
    nm = namebuf;
    if (!strncmpi(nm, "scroll ", 7))
        nm += 7;
    else if (!strncmpi(nm, "spellbook ", 10))
        nm += 10;
    if (!strncmpi(nm, "of ", 3))
        nm += 3;

    if ((bp = strstri(nm, " armour")) != 0) {
        memcpy(bp, " armor ", 7);
        (void) mungspaces(bp + 1);        /* remove the extra space */
    }

    deferred = real = 0; /* not any scroll or book */
    deferralchance = 0;  /* incremented for each oc_uname match */
    first = g.bases[(int) paper->oclass];
    last = g.bases[(int) paper->oclass + 1] - 1;
    /* first loop: look for match with name/description */
    for (i = first; i <= last; i++) {
        /* extra shufflable descr not representing a real object */
        if (!OBJ_NAME(objects[i]))
            continue;

        if (!strcmpi(OBJ_NAME(objects[i]), nm)) {
            if (objects[i].oc_name_known
                /* spellbooks can only be written by_name, so no need to
                   hold out for a 'better' by_descr match */
                || paper->oclass == SPBOOK_CLASS) {
                goto found;
            } else {
                /* save item in case there are no better by_descr matches */
                real = deferred = i;
                break;
            }
        }

        if (!strcmpi(OBJ_DESCR(objects[i]), nm)) {
            by_descr = TRUE;
            goto found;
        }
    }
    /* second loop: look for match with user-assigned name */
    /* we will get here if 'nm' isn't a real scroll name/descr, or is the name
     * of a real scroll that hasn't been formally IDed. */
    for (i = first; i <= last; i++) {
        /* player might assign same name multiple times and if so,
           we choose one of those matches randomly */
        if (objects[i].oc_uname && !strcmpi(objects[i].oc_uname, nm)
            /* prefer attempting to write the real scroll type if
               the typename clobbers a real scroll and is known to
               be incorrect */
            && !(real && objects[i].oc_name_known)
            /*
             * First match: chance incremented to 1,
             *   !rn2(1) is 1, we remember i;
             * second match: chance incremented to 2,
             *   !rn2(2) has 1/2 chance to replace i;
             * third match: chance incremented to 3,
             *   !rn2(3) has 1/3 chance to replace i
             *   and 2/3 chance to keep previous 50:50
             *   choice; so on for higher match counts.
             */
            && !rn2(++deferralchance)) {
            deferred = i;
            /* writing by user-assigned name is same as by description:
               fails for books, works for scrolls (having an assigned
               type name guarantees presence on discoveries list) */
            by_descr = TRUE;
        }
    }

    if (deferred) {
        i = deferred;
        goto found;
    }

    There("is no such %s!", typeword);
    return ECMD_TIME;
 found:

    if (i == SCR_BLANK_PAPER || i == SPE_BLANK_PAPER) {
        You_cant("write that!");
        pline("It's obscene!");
        return ECMD_TIME;
    } else if (i == SPE_NOVEL) {
        boolean fanfic = !rn2(3), tearup = !rn2(3);

        if (!fanfic) {
            You("%s to write the Great Yendorian Novel, but %s inspiration.",
                !tearup ? "prepare" : "try",
                !Hallucination ? "lack" : "have too much");
        } else {
            You("%sproduce really %s fan-fiction.",
                !tearup ? "start to " : "",
                !Hallucination ? "lame" : "awesome");
        }
        if (!tearup) {
            You("give up on the idea.");
        } else {
            You("tear it up.");
            useup(paper);
        }
        return ECMD_TIME;
    } else if (i == SPE_BOOK_OF_THE_DEAD) {
        pline("No mere dungeon adventurer could write that.");
        return ECMD_TIME;
    } else if (by_descr && paper->oclass == SPBOOK_CLASS
               && !objects[i].oc_name_known) {
        /* can't write unknown spellbooks by description */
        pline("Unfortunately you don't have enough information to go on.");
        return ECMD_TIME;
    }

    /* KMH, conduct */
    if (!u.uconduct.literate++)
        livelog_printf(LL_CONDUCT,
                       "became literate by writing %s", an(typeword));

    new_obj = mksobj(i, FALSE, FALSE);
    new_obj->bknown = (paper->bknown && pen->bknown);

    /* shk imposes a flat rate per use, not based on actual charges used */
    check_unpaid(pen);

    /* see if there's enough ink */
    basecost = cost(new_obj);
    if (pen->spe < basecost / 2) {
        Your("marker is too dry to write that!");
        obfree(new_obj, (struct obj *) 0);
        return ECMD_TIME;
    }

    /* we're really going to write now, so calculate cost
     */
    actualcost = rn1(basecost / 2, basecost / 2);
    curseval = bcsign(pen) + bcsign(paper);
    exercise(A_WIS, TRUE);
    /* dry out marker */
    if (pen->spe < actualcost) {
        pen->spe = 0;
        Your("marker dries out!");
        /* scrolls disappear, spellbooks don't */
        if (paper->oclass == SPBOOK_CLASS) {
            pline_The("spellbook is left unfinished and your writing fades.");
            update_inventory(); /* pen charges */
        } else {
            pline_The("scroll is now useless and disappears!");
            useup(paper);
        }
        obfree(new_obj, (struct obj *) 0);
        return ECMD_TIME;
    }
    pen->spe -= actualcost;

    /*
     * Writing by name requires that the hero knows the scroll or
     * book type.  One has previously been read (and its effect
     * was evident) or been ID'd via scroll/spell/throne and it
     * will be on the discoveries list.
     * (Previous versions allowed scrolls and books to be written
     * by type name if they were on the discoveries list via being
     * given a user-assigned name, even though doing the latter
     * doesn't--and shouldn't--make the actual type become known.)
     *
     * Writing by description requires that the hero knows the
     * description (a scroll's label, that is, since books by_descr
     * are rejected above).  BUG:  We can only do this for known
     * scrolls and for the case where the player has assigned a
     * name to put it onto the discoveries list; we lack a way to
     * track other scrolls which have been seen closely enough to
     * read the label without then being ID'd or named.  The only
     * exception is for currently carried inventory, where we can
     * check for one [with its dknown bit set] of the same type.
     *
     * Normal requirements can be overridden if hero is Lucky.
     */

    /* if known, then either by-name or by-descr works */
    if (!objects[new_obj->otyp].oc_name_known
        /* else if named, then only by-descr works */
        && !(by_descr && label_known(new_obj->otyp, g.invent))
        /* and Luck might override after both checks have failed */
        && rnl(Role_if(PM_WIZARD) ? 5 : 15)) {
        You("%s to write that.", by_descr ? "fail" : "don't know how");
        /* scrolls disappear, spellbooks don't */
        if (paper->oclass == SPBOOK_CLASS) {
            You(
      "write in your best handwriting:  \"My Diary\", but it quickly fades.");
            update_inventory(); /* pen charges */
        } else {
            if (by_descr) {
                Strcpy(namebuf, OBJ_DESCR(objects[new_obj->otyp]));
                wipeout_text(namebuf, (6 + MAXULEV - u.ulevel) / 6, 0);
            } else
                Sprintf(namebuf, "%s was here!", g.plname);
            You("write \"%s\" and the scroll disappears.", namebuf);
            useup(paper);
        }
        obfree(new_obj, (struct obj *) 0);
        return ECMD_TIME;
    }
    /* can write scrolls when blind, but requires luck too;
       attempts to write books when blind are caught above */
    if (Blind && rnl(3)) {
        /* writing while blind usually fails regardless of
           whether the target scroll is known; even if we
           have passed the write-an-unknown scroll test
           above we can still fail this one, so it's doubly
           hard to write an unknown scroll while blind */
        You("fail to write the scroll correctly and it disappears.");
        useup(paper);
        obfree(new_obj, (struct obj *) 0);
        return ECMD_TIME;
    }

    /* useup old scroll / spellbook */
    useup(paper);

    /* success */
    if (new_obj->oclass == SPBOOK_CLASS) {
        /* acknowledge the change in the object's description... */
        pline_The("spellbook warps strangely, then turns %s.",
                  new_book_description(new_obj->otyp, namebuf));
    }
    new_obj->blessed = (curseval > 0);
    new_obj->cursed = (curseval < 0);
#ifdef MAIL_STRUCTURES
    if (new_obj->otyp == SCR_MAIL)
        /* 0: delivered in-game via external event (or randomly for fake mail);
           1: from bones or wishing; 2: written with marker */
        new_obj->spe = 2;
#endif
    /* unlike alchemy, for example, a successful result yields the
       specifically chosen item so hero recognizes it even if blind;
       the exception is for being lucky writing an undiscovered scroll,
       where the label associated with the type-name isn't known yet */
    new_obj->dknown = label_known(new_obj->otyp, g.invent) ? 1 : 0;

    new_obj = hold_another_object(new_obj, "Oops!  %s out of your grasp!",
                                  The(aobjnam(new_obj, "slip")),
                                  (const char *) 0);
    nhUse(new_obj); /* try to avoid complaint about dead assignment */
    return ECMD_TIME;
}

/* most book descriptions refer to cover appearance, so we can issue a
   message for converting a plain book into one of those with something
   like "the spellbook turns red" or "the spellbook turns ragged";
   but some descriptions refer to composition and "the book turns vellum"
   looks funny, so we want to insert "into " prior to such descriptions;
   even that's rather iffy, indicating that such descriptions probably
   ought to be eliminated (especially "cloth"!) */
static char *
new_book_description(int booktype, char *outbuf)
{
    /* subset of description strings from objects.c; if it grows
       much, we may need to add a new flag field to objects[] instead */
    static const char *const compositions[] = {
        "parchment",
        "vellum",
        "cloth",
#if 0
        "canvas", "hardcover", /* not used */
        "papyrus", /* not applicable--can't be produced via writing */
#endif /*0*/
        0
    };
    const char *descr, *const *comp_p;

    descr = OBJ_DESCR(objects[booktype]);
    for (comp_p = compositions; *comp_p; ++comp_p)
        if (!strcmpi(descr, *comp_p))
            break;

    Sprintf(outbuf, "%s%s", *comp_p ? "into " : "", descr);
    return outbuf;
}

/*write.c*/

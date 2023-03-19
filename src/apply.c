/* NetHack 3.7	apply.c	$NHDT-Date: 1655631557 2022/06/19 09:39:17 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.381 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int use_camera(struct obj *);
static int use_towel(struct obj *);
static boolean its_dead(coordxy, coordxy, int *);
static int use_stethoscope(struct obj *);
static void use_whistle(struct obj *);
static void use_magic_whistle(struct obj *);
static void magic_whistled(struct obj *);
static int use_leash(struct obj *);
static boolean mleashed_next2u(struct monst *);
static int use_mirror(struct obj *);
static void use_bell(struct obj **);
static void use_candelabrum(struct obj *);
static void use_candle(struct obj **);
static void use_lamp(struct obj *);
static void light_cocktail(struct obj **);
static int rub_ok(struct obj *);
static void display_jump_positions(int);
static void use_tinning_kit(struct obj *);
static int use_figurine(struct obj **);
static int grease_ok(struct obj *);
static int use_grease(struct obj *);
static void use_trap(struct obj *);
static int touchstone_ok(struct obj *);
static int use_stone(struct obj *);
static int set_trap(void); /* occupation callback */
static void display_polearm_positions(int);
static int use_cream_pie(struct obj *);
static int jelly_ok(struct obj *);
static int use_royal_jelly(struct obj *);
static int grapple_range(void);
static boolean can_grapple_location(coordxy, coordxy);
static int use_grapple(struct obj *);
static int do_break_wand(struct obj *);
static int apply_ok(struct obj *);
static int flip_through_book(struct obj *);
static boolean figurine_location_checks(struct obj *, coord *, boolean);
static boolean check_jump(genericptr_t, coordxy, coordxy);
static boolean is_valid_jump_pos(coordxy, coordxy, int, boolean);
static boolean get_valid_jump_position(coordxy, coordxy);
static boolean get_valid_polearm_position(coordxy, coordxy);
static boolean find_poleable_mon(coord *, int, int);

static const char
    Nothing_seems_to_happen[] = "Nothing seems to happen.",
    no_elbow_room[] = "don't have enough elbow-room to maneuver.";

static int
use_camera(struct obj *obj)
{
    struct monst *mtmp;

    if (Underwater) {
        pline("Using your camera underwater would void the warranty.");
        return ECMD_OK;
    }
    if (!getdir((char *) 0))
        return ECMD_CANCEL;

    if (obj->spe <= 0) {
        pline1(nothing_happens);
        return ECMD_TIME;
    }
    consume_obj_charge(obj, TRUE);

    if (obj->cursed && !rn2(2)) {
        (void) zapyourself(obj, TRUE);
    } else if (u.uswallow) {
        You("take a picture of %s %s.", s_suffix(mon_nam(u.ustuck)),
            mbodypart(u.ustuck, STOMACH));
    } else if (u.dz) {
        You("take a picture of the %s.",
            (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
    } else if (!u.dx && !u.dy) {
        (void) zapyourself(obj, TRUE);
    } else {
        mtmp = bhit(u.dx, u.dy, COLNO, FLASHED_LIGHT,
                    (int (*) (MONST_P, OBJ_P)) 0,
                    (int (*) (OBJ_P, OBJ_P)) 0, &obj);
        obj->ox = u.ux, obj->oy = u.uy; /* flash_hits_mon() wants this */
        if (mtmp)
            (void) flash_hits_mon(mtmp, obj);
        /* normally bhit() would do this but for FLASHED_LIGHT we want it
           to be deferred until after flash_hits_mon() */
        transient_light_cleanup();
    }
    return ECMD_TIME;
}

static int
use_towel(struct obj *obj)
{
    boolean drying_feedback = (obj == uwep);

    if (!freehand()) {
        You("have no free %s!", body_part(HAND));
        return ECMD_OK;
    } else if (obj == ublindf) {
        You("cannot use it while you're wearing it!");
        return ECMD_OK;
    } else if (obj->cursed) {
        long old;

        switch (rn2(3)) {
        case 2:
            old = (Glib & TIMEOUT);
            make_glib((int) old + rn1(10, 3)); /* + 3..12 */
            Your("%s %s!", makeplural(body_part(HAND)),
                 (old ? "are filthier than ever" : "get slimy"));
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return ECMD_TIME;
        case 1:
            if (!ublindf) {
                old = u.ucreamed;
                u.ucreamed += rn1(10, 3);
                pline("Yecch!  Your %s %s gunk on it!", body_part(FACE),
                      (old ? "has more" : "now has"));
                make_blinded(Blinded + (long) u.ucreamed - old, TRUE);
            } else {
                const char *what;

                what = (ublindf->otyp == LENSES)
                           ? "lenses"
                           : (obj->otyp == ublindf->otyp) ? "other towel"
                                                          : "blindfold";
                if (ublindf->cursed) {
                    You("push your %s %s.", what,
                        rn2(2) ? "cock-eyed" : "crooked");
                } else {
                    struct obj *saved_ublindf = ublindf;
                    You("push your %s off.", what);
                    Blindf_off(ublindf);
                    dropx(saved_ublindf);
                }
            }
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return ECMD_TIME;
        case 0:
            break;
        }
    }

    if (Glib) {
        make_glib(0);
        You("wipe off your %s.",
            !uarmg ? makeplural(body_part(HAND)) : gloves_simple_name(uarmg));
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return ECMD_TIME;
    } else if (u.ucreamed) {
        Blinded -= u.ucreamed;
        u.ucreamed = 0;
        if (!Blinded) {
            pline("You've got the glop off.");
            if (!gulp_blnd_check()) {
                Blinded = 1;
                make_blinded(0L, TRUE);
            }
        } else {
            Your("%s feels clean now.", body_part(FACE));
        }
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return ECMD_TIME;
    }

    Your("%s and %s are already clean.", body_part(FACE),
         makeplural(body_part(HAND)));

    return ECMD_OK;
}

/* maybe give a stethoscope message based on floor objects */
static boolean
its_dead(coordxy rx, coordxy ry, int *resp)
{
    char buf[BUFSZ];
    boolean more_corpses;
    struct permonst *mptr;
    struct obj *corpse = sobj_at(CORPSE, rx, ry),
               *statue = sobj_at(STATUE, rx, ry);

    if (!can_reach_floor(TRUE)) { /* levitation or unskilled riding */
        corpse = 0;               /* can't reach corpse on floor */
        /* you can't reach tiny statues (even though you can fight
           tiny monsters while levitating--consistency, what's that?) */
        while (statue && mons[statue->corpsenm].msize == MZ_TINY)
            statue = nxtobj(statue, STATUE, TRUE);
    }
    /* when both corpse and statue are present, pick the uppermost one */
    if (corpse && statue) {
        if (nxtobj(statue, CORPSE, TRUE) == corpse)
            corpse = 0; /* corpse follows statue; ignore it */
        else
            statue = 0; /* corpse precedes statue; ignore statue */
    }
    more_corpses = (corpse && nxtobj(corpse, CORPSE, TRUE));

    /* additional stethoscope messages from jyoung@apanix.apana.org.au */
    if (!corpse && !statue) {
        ; /* nothing to do */

    } else if (Hallucination) {
        if (!corpse) {
            /* it's a statue */
            Strcpy(buf, "You're both stoned");
        } else if (corpse->quan == 1L && !more_corpses) {
            int gndr = 2; /* neuter: "it" */
            struct monst *mtmp = get_mtraits(corpse, FALSE);

            /* (most corpses don't retain the monster's sex, so
               we're usually forced to use generic pronoun here) */
            if (mtmp) {
                mptr = mtmp->data = &mons[mtmp->mnum];
                /* TRUE: override visibility check--it's not on the map */
                gndr = pronoun_gender(mtmp, PRONOUN_NO_IT);
            } else {
                mptr = &mons[corpse->corpsenm];
                if (is_female(mptr))
                    gndr = 1;
                else if (is_male(mptr))
                    gndr = 0;
            }
            Sprintf(buf, "%s's dead", genders[gndr].he); /* "he"/"she"/"it" */
            buf[0] = highc(buf[0]);
        } else { /* plural */
            Strcpy(buf, "They're dead");
        }
        /* variations on "He's dead, Jim." (Star Trek's Dr McCoy) */
        You_hear("a voice say, \"%s, Jim.\"", buf);
        *resp = ECMD_TIME;
        return TRUE;

    } else if (corpse) {
        boolean here = u_at(rx, ry),
                one = (corpse->quan == 1L && !more_corpses), reviver = FALSE;
        int visglyph, corpseglyph;

        visglyph = glyph_at(rx, ry);
        corpseglyph = obj_to_glyph(corpse, rn2);

        if (Blind && (visglyph != corpseglyph))
            map_object(corpse, TRUE);

        if (Role_if(PM_HEALER)) {
            /* ok to reset `corpse' here; we're done with it */
            do {
                if (obj_has_timer(corpse, REVIVE_MON))
                    reviver = TRUE;
                else
                    corpse = nxtobj(corpse, CORPSE, TRUE);
            } while (corpse && !reviver);
        }
        You("determine that %s unfortunate being%s %s%s dead.",
            one ? (here ? "this" : "that") : (here ? "these" : "those"),
            one ? "" : "s", one ? "is" : "are", reviver ? " mostly" : "");
        return TRUE;

    } else { /* statue */
        const char *what, *how;

        mptr = &mons[statue->corpsenm];
        if (Blind) { /* ignore statue->dknown; it'll always be set */
            Sprintf(buf, "%s %s",
                    u_at(rx, ry) ? "This" : "That",
                    humanoid(mptr) ? "person" : "creature");
            what = buf;
        } else {
            what = obj_pmname(statue);
            if (!type_is_pname(mptr))
                what = The(what);
        }
        how = "fine";
        if (Role_if(PM_HEALER)) {
            struct trap *ttmp = t_at(rx, ry);

            if (ttmp && ttmp->ttyp == STATUE_TRAP)
                how = "extraordinary";
            else if (Has_contents(statue))
                how = "remarkable";
        }

        pline("%s is in %s health for a statue.", what, how);
        return TRUE;
    }
    return FALSE; /* no corpse or statue */
}

static const char hollow_str[] = "a hollow sound.  This must be a secret %s!";

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
static int
use_stethoscope(struct obj *obj)
{
    struct monst *mtmp;
    struct rm *lev;
    int res;
    coordxy rx, ry;
    boolean interference = (u.uswallow && is_whirly(u.ustuck->data)
                            && !rn2(Role_if(PM_HEALER) ? 10 : 3));

    if (nohands(gy.youmonst.data)) {
        You("have no hands!"); /* not `body_part(HAND)' */
        return ECMD_OK;
    } else if (Deaf) {
        You_cant("hear anything!");
        return ECMD_OK;
    } else if (!freehand()) {
        You("have no free %s.", body_part(HAND));
        return ECMD_OK;
    }
    if (!getdir((char *) 0))
        return ECMD_CANCEL;

    res = (gh.hero_seq == gc.context.stethoscope_seq) ? ECMD_TIME : ECMD_OK;
    gc.context.stethoscope_seq = gh.hero_seq;

    gb.bhitpos.x = u.ux, gb.bhitpos.y = u.uy; /* tentative, reset below */
    gn.notonhead = u.uswallow;
    if (u.usteed && u.dz > 0) {
        if (interference) {
            pline("%s interferes.", Monnam(u.ustuck));
            mstatusline(u.ustuck);
        } else
            mstatusline(u.usteed);
        return res;
    } else if (u.uswallow && (u.dx || u.dy || u.dz)) {
        mstatusline(u.ustuck);
        return res;
    } else if (u.uswallow && interference) {
        pline("%s interferes.", Monnam(u.ustuck));
        mstatusline(u.ustuck);
        return res;
    } else if (u.dz) {
        if (Underwater) {
            Soundeffect(se_faint_splashing, 35);
            You_hear("faint splashing.");
        } else if (u.dz < 0 || !can_reach_floor(TRUE)) {
            cant_reach_floor(u.ux, u.uy, (u.dz < 0), TRUE);
        } else if (its_dead(u.ux, u.uy, &res)) {
            ; /* message already given */
        } else if (Is_stronghold(&u.uz)) {
            Soundeffect(se_crackling_of_hellfire, 35);
            You_hear("the crackling of hellfire.");
        } else {
            pline_The("%s seems healthy enough.", surface(u.ux, u.uy));
        }
        return res;
    } else if (obj->cursed && !rn2(2)) {
        Soundeffect(se_heart_beat, 100);
        You_hear("your heart beat.");
        return res;
    }
    confdir(FALSE);
    if (!u.dx && !u.dy) {
        ustatusline();
        return res;
    }
    rx = u.ux + u.dx;
    ry = u.uy + u.dy;
    if (!isok(rx, ry)) {
        Soundeffect(se_typing_noise, 100);
        You_hear("a faint typing noise.");
        return ECMD_OK;
    }
    if ((mtmp = m_at(rx, ry)) != 0) {
        const char *mnm = x_monnam(mtmp, ARTICLE_A, (const char *) 0,
                                   SUPPRESS_IT | SUPPRESS_INVISIBLE, FALSE);

        /* gb.bhitpos needed by mstatusline() iff mtmp is a long worm */
        gb.bhitpos.x = rx, gb.bhitpos.y = ry;
        gn.notonhead = (mtmp->mx != rx || mtmp->my != ry);

        if (mtmp->mundetected) {
            if (!canspotmon(mtmp))
                There("is %s hidden there.", mnm);
            mtmp->mundetected = 0;
            newsym(mtmp->mx, mtmp->my);
        } else if (mtmp->mappearance) {
            const char *what = "thing";
            boolean use_plural = FALSE;
            struct obj dummyobj, *odummy;

            switch (M_AP_TYPE(mtmp)) {
            case M_AP_OBJECT:
                /* FIXME?
                 *  we should probably be using object_from_map() here
                 */
                odummy = init_dummyobj(&dummyobj, mtmp->mappearance, 1L);
                /* simple_typename() yields "fruit" for any named fruit;
                   we want the same thing '//' or ';' shows: "slime mold"
                   or "grape" or "slice of pizza" */
                if (odummy->otyp == SLIME_MOLD && has_mcorpsenm(mtmp)) {
                    odummy->spe = MCORPSENM(mtmp);
                    what = simpleonames(odummy);
                } else {
                    what = simple_typename(odummy->otyp);
                }
                use_plural = (is_boots(odummy) || is_gloves(odummy)
                              || odummy->otyp == LENSES);
                break;
            case M_AP_MONSTER: /* ignore Hallucination here */
                what = pmname(&mons[mtmp->mappearance], Mgender(mtmp));
                break;
            case M_AP_FURNITURE:
                what = defsyms[mtmp->mappearance].explanation;
                break;
            }
            seemimic(mtmp);
            pline("%s %s %s really %s.",
                  use_plural ? "Those" : "That", what,
                  use_plural ? "are" : "is", mnm);
        } else if (Verbose(0, use_stethoscope) && !canspotmon(mtmp)) {
            There("is %s there.", mnm);
        }

        mstatusline(mtmp);
        if (!canspotmon(mtmp))
            map_invisible(rx, ry);
        return res;
    }
    if (unmap_invisible(rx,ry))
        pline_The("invisible monster must have moved.");

    lev = &levl[rx][ry];
    switch (lev->typ) {
    case SDOOR:
        Soundeffect(se_hollow_sound, 100);
        You_hear(hollow_str, "door");
        cvt_sdoor_to_door(lev); /* ->typ = DOOR */
        feel_newsym(rx, ry);
        return res;
    case SCORR:
        You_hear(hollow_str, "passage");
        lev->typ = CORR, lev->flags = 0;
        unblock_point(rx, ry);
        feel_newsym(rx, ry);
        return res;
    }

    if (!its_dead(rx, ry, &res))
        You("hear nothing special."); /* not You_hear()  */
    return res;
}

static const char whistle_str[] = "produce a %s whistling sound.",
                  alt_whistle_str[] = "produce a %s, sharp vibration.";

static void
use_whistle(struct obj *obj)
{
    if (!can_blow(&gy.youmonst)) {
        You("are incapable of using the whistle.");
    } else if (Underwater) {
        You("blow bubbles through %s.", yname(obj));
    } else {
        if (Deaf)
            You_feel("rushing air tickle your %s.", body_part(NOSE));
        else
            You(whistle_str, obj->cursed ? "shrill" : "high");
        Soundeffect(se_shrill_whistle, 50);
        wake_nearby();
        if (obj->cursed)
            vault_summon_gd();
    }
}

static void
use_magic_whistle(struct obj *obj)
{
    if (!can_blow(&gy.youmonst)) {
        You("are incapable of using the whistle.");
    } else if (obj->cursed && !rn2(2)) {
        You("produce a %shigh-%s.", Underwater ? "very " : "",
            Deaf ? "frequency vibration" : "pitched humming noise");
        wake_nearby();
    } else {
        /* it's magic!  it works underwater too (at a higher pitch) */
        You(Deaf ? alt_whistle_str : whistle_str,
            Hallucination ? "normal"
            : (Underwater && !Deaf) ? "strange, high-pitched"
              : "strange");
        Soundeffect(se_shrill_whistle, 80);
        magic_whistled(obj);
    }
}

/* 'obj' is assumed to be a magic whistle */
static void
magic_whistled(struct obj *obj)
{
    struct monst *mtmp, *nextmon;
    char buf[BUFSZ], *mnam = 0,
         shiftbuf[BUFSZ + sizeof "shifts location"],
         appearbuf[BUFSZ + sizeof "appears"],
         disappearbuf[BUFSZ + sizeof "disappears"];
    boolean oseen, nseen,
            already_discovered = objects[obj->otyp].oc_name_known != 0;
    int omx, omy, shift = 0, appear = 0, disappear = 0, trapped = 0;

    /* need to copy (up to 3) names as they're collected rather than just
       save pointers to them, otherwise churning through every mbuf[] might
       clobber the ones we care about */
    shiftbuf[0] = appearbuf[0] = disappearbuf[0] = '\0';

    for (mtmp = fmon; mtmp; mtmp = nextmon) {
        nextmon = mtmp->nmon; /* trap might kill mon */
        if (DEADMONSTER(mtmp))
            continue;
        /* only tame monsters are affected;
           steed is already at your location, so not affected;
           this avoids trap issues if you're on a trap location */
        if (!mtmp->mtame || mtmp == u.usteed)
            continue;
        if (mtmp->mtrapped) {
            /* no longer in previous trap (affects mintrap) */
            mtmp->mtrapped = 0;
            fill_pit(mtmp->mx, mtmp->my);
        }

        oseen = canspotmon(mtmp); /* old 'seen' status */
        if (oseen) /* get name in case it's one we'll remember */
            mnam = y_monnam(mtmp); /* before mnexto(); it might disappear */
        /* mimic must be revealed before we know whether it
           actually moves because line-of-sight may change */
        if (M_AP_TYPE(mtmp))
            seemimic(mtmp);
        omx = mtmp->mx, omy = mtmp->my;
        mnexto(mtmp, !already_discovered ? RLOC_MSG : RLOC_NONE);

        if (mtmp->mx != omx || mtmp->my != omy) {
            mtmp->mundetected = 0; /* reveal non-mimic hider iff it moved */
            /*
             * FIXME:
             *  All relocated monsters should change positions essentially
             *  simultaneously but we're dealing with them sequentially.
             *  That could kill some off in the process, each time leaving
             *  their target position (which should be occupied at least
             *  momentarily) available as a potential death trap for others.
             *
             *  Also, teleporting onto a trap introduces message sequencing
             *  issues.  We try to avoid the most obvious non sequiturs by
             *  checking whether pline() got called during mintrap().
             *  iflags.last_msg will be changed from the value we set here
             *  to PLNMSG_UNKNOWN in that situation.
             */
            iflags.last_msg = PLNMSG_enum; /* not a specific message */
            if (mintrap(mtmp, NO_TRAP_FLAGS) == Trap_Killed_Mon)
                change_luck(-1);
            if (iflags.last_msg != PLNMSG_enum) {
                ++trapped;
                continue;
            }
            /* dying while seen would have issued a message and not get here;
               being sent to an unseen location and dying there should be
               included in the disappeared case */
            nseen = DEADMONSTER(mtmp) ? FALSE : canspotmon(mtmp);

            if (nseen) {
                mnam = y_monnam(mtmp);
                if (oseen) {
                    if (++shift == 1)
                        Sprintf(shiftbuf, "%s shifts location", mnam);
                } else {
                    if (++appear == 1)
                        Sprintf(appearbuf, "%s appears", mnam);
                }
            } else if (oseen) {
                if (++disappear == 1)
                    Sprintf(disappearbuf, "%s disappears", mnam);
            }
        }
    }

    /*
     * If any pets changed location, (1) they might have been in view
     * before and still in view after, (2) out of view before but in
     * view after, (3) in view before but out of view after (perhaps
     * on the far side of a boulder/door/wall), or (4) out of view
     * before and still out of view after.  The first two cases are
     * the usual ones; the fourth will happen if the hero can't see.
     *
     * If the magic whistle hasn't been discovered yet, rloc() issued
     * any applicable vanishing and/or appearing messages, and we make
     * it become discovered now if any pets moved within or into view.
     * If it has already been discovered, we told rloc() not to issue
     * messages and will issue one cumulative message now (for any of
     * the first three cases, not the fourth) to reduce verbosity for
     * the first case of a single pet (avoid "vanishes and reappears")
     * and greatly reduce verbosity for multiple pets regardless of
     * each one's case.
     */
    buf[0] = '\0';
    if (!already_discovered) {
        /* message(s) were handled by rloc(); if only noticeable change was
           pet(s) disappearing, the magic whistle won't become discovered */
        if (shift + appear + trapped > 0)
            makeknown(obj->otyp);
    } else {
        /* could use array of cardinal number names like wishcmdassist() but
           extra precision above 3 or 4 seems pedantic; not used for 0 or 1 */
#define HowMany(n) (((n) < 2) ? "sqrt(-1)"          \
                    : ((n) == 2) ? "two"            \
                      : ((n) == 3) ? "three"        \
                        : ((n) == 4) ? "four"       \
                          : ((n) <= 7) ? "several"  \
                            : "many")
        /* magic whistle is already discovered so rloc() message(s)
           were suppressed above; if any discernible relocation occurred,
           construct a message now and issue it below */
        if (shift > 0) {
            if (shift > 1)
                Sprintf(shiftbuf, "%s creatures shift locations",
                        HowMany(shift));
            copynchars(buf, upstart(shiftbuf), (int) sizeof buf - 1);
        }
        if (appear > 0) {
            if (appear > 1)
                /* shift==0: N creatures appear;
                   shift==1: Foo shifts location and N other creatures appear;
                   shift >1: M creatures shift locations and N others appear */
                Sprintf(appearbuf, "%s %s appear", HowMany(appear),
                        (shift == 0) ? "creatures"
                        : (shift == 1) ? "other creatures"
                          : "others");
            if (shift == 0)
                copynchars(buf, upstart(appearbuf), (int) sizeof buf - 1);
            else
                Snprintf(eos(buf), sizeof buf - strlen(buf), "%s %s",
                         /* to get here:  appear > 0 and shift != 0,
                            so "shifters, appearers" if disappear != 0
                            with ", and disappearers" yet to be appended,
                            or "shifters and appearers" otherwise */
                         disappear ? "," : " and", appearbuf);
        }
        if (disappear > 0) {
            if (disappear > 1)
                Sprintf(disappearbuf, "%s %s disappear", HowMany(disappear),
                        (shift == 0 && appear == 0) ? "creatures"
                        : (shift < 2 && appear < 2) ? "other creatures"
                          : "others");
            if (shift + appear == 0)
                copynchars(buf, upstart(disappearbuf), (int) sizeof buf - 1);
            else
                Snprintf(eos(buf), sizeof buf - strlen(buf), "%s and %s",
                         (shift && appear) ? "," : "", disappearbuf);
        }
    }
    if (*buf)
        pline("%s.", buf);
    return;
}

boolean
um_dist(coordxy x, coordxy y, xint16 n)
{
    return (boolean) (abs(u.ux - x) > n || abs(u.uy - y) > n);
}

int
number_leashed(void)
{
    int i = 0;
    struct obj *obj;

    for (obj = gi.invent; obj; obj = obj->nobj)
        if (obj->otyp == LEASH && obj->leashmon != 0)
            i++;
    return i;
}

/* otmp is about to be destroyed or stolen */
void
o_unleash(struct obj *otmp)
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (mtmp->m_id == (unsigned) otmp->leashmon) {
            mtmp->mleashed = 0;
            otmp->leashmon = 0;
            update_inventory();
            break;
        }
}

/* mtmp is about to die, or become untame */
void
m_unleash(struct monst *mtmp, boolean feedback)
{
    register struct obj *otmp;

    if (feedback) {
        if (canseemon(mtmp))
            pline("%s pulls free of %s leash!", Monnam(mtmp), mhis(mtmp));
        else
            Your("leash falls slack.");
    }
    if ((otmp = get_mleash(mtmp)) != 0) {
        otmp->leashmon = 0;
        update_inventory();
    }
    mtmp->mleashed = 0;
}

/* player is about to die (for bones) */
void
unleash_all(void)
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = gi.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH)
            otmp->leashmon = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        mtmp->mleashed = 0;
}

#define MAXLEASHED 2

boolean
leashable(struct monst *mtmp)
{
    return (boolean) (mtmp->mnum != PM_LONG_WORM
                       && !unsolid(mtmp->data)
                       && (!nolimbs(mtmp->data) || has_head(mtmp->data)));
}

/* ARGSUSED */
static int
use_leash(struct obj *obj)
{
    coord cc;
    struct monst *mtmp;
    int spotmon;

    if (u.uswallow) {
        /* if the leash isn't in use, assume we're trying to leash
           the engulfer; if it is use, distinguish between removing
           it from the engulfer versus from some other creature
           (note: the two in-use cases can't actually occur; all
           leashes are released when the hero gets engulfed) */
        You_cant((!obj->leashmon
                  ? "leash %s from inside."
                  : (obj->leashmon == (int) u.ustuck->m_id)
                    ? "unleash %s from inside."
                    : "unleash anything from inside %s."),
                 noit_mon_nam(u.ustuck));
        return ECMD_OK;
    }
    if (!obj->leashmon && number_leashed() >= MAXLEASHED) {
        You("cannot leash any more pets.");
        return ECMD_OK;
    }

    if (!get_adjacent_loc((char *) 0, (char *) 0, u.ux, u.uy, &cc))
        return ECMD_OK;

    if (u_at(cc.x, cc.y)) {
        if (u.usteed && u.dz > 0) {
            mtmp = u.usteed;
            spotmon = 1;
            goto got_target;
        }
        pline("Leash yourself?  Very funny...");
        return ECMD_OK;
    }

    /*
     * From here on out, return value is 1 == a move is used.
     */

    if (!(mtmp = m_at(cc.x, cc.y))) {
        There("is no creature there.");
        (void) unmap_invisible(cc.x, cc.y);
        return ECMD_TIME;
    }

    spotmon = canspotmon(mtmp);
 got_target:

    if (!spotmon && !glyph_is_invisible(levl[cc.x][cc.y].glyph)) {
        /* for the unleash case, we don't verify whether this unseen
           monster is the creature attached to the current leash */
        You("fail to %sleash something.", obj->leashmon ? "un" : "");
        /* trying again will work provided the monster is tame
           (and also that it doesn't change location by retry time) */
        map_invisible(cc.x, cc.y);
    } else if (!mtmp->mtame) {
        pline("%s %s leashed!", Monnam(mtmp),
              (!obj->leashmon) ? "cannot be" : "is not");
    } else if (!obj->leashmon) {
        /* applying a leash which isn't currently in use */
        if (mtmp->mleashed) {
            pline("This %s is already leashed.",
                  spotmon ? l_monnam(mtmp) : "creature");
        } else if (unsolid(mtmp->data)) {
            pline("The leash would just fall off.");
        } else if (nolimbs(mtmp->data) && !has_head(mtmp->data)) {
            pline("%s has no extremities the leash would fit.",
                  Monnam(mtmp));
        } else if (!leashable(mtmp)) {
            char lmonbuf[BUFSZ];
            char *lmonnam = l_monnam(mtmp);

            if (cc.x != mtmp->mx || cc.y != mtmp->my) {
                Sprintf(lmonbuf, "%s tail", s_suffix(lmonnam));
                lmonnam = lmonbuf;
            }
            pline("The leash won't fit onto %s%s.", spotmon ? "your " : "",
                  lmonnam);
        } else {
            You("slip the leash around %s%s.", spotmon ? "your " : "",
                l_monnam(mtmp));
            mtmp->mleashed = 1;
            obj->leashmon = (int) mtmp->m_id;
            mtmp->msleeping = 0;
            update_inventory();
        }
    } else {
        /* applying a leash which is currently in use */
        if (obj->leashmon != (int) mtmp->m_id) {
            pline("This leash is not attached to that creature.");
        } else if (obj->cursed) {
            pline_The("leash would not come off!");
            set_bknown(obj, 1);
        } else {
            mtmp->mleashed = 0;
            obj->leashmon = 0;
            update_inventory();
            You("remove the leash from %s%s.",
                spotmon ? "your " : "", l_monnam(mtmp));
        }
    }
    return ECMD_TIME;
}

/* assuming mtmp->mleashed has been checked */
struct obj *
get_mleash(struct monst *mtmp)
{
    struct obj *otmp;

    for (otmp = gi.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH && (unsigned) otmp->leashmon == mtmp->m_id)
            break;
    return otmp;
}

static boolean
mleashed_next2u(struct monst *mtmp)
{
    if (mtmp->mleashed) {
        if (!next2u(mtmp->mx, mtmp->my))
            mnexto(mtmp, RLOC_NOMSG);
        if (!next2u(mtmp->mx, mtmp->my)) {
            struct obj *otmp = get_mleash(mtmp);

            if (!otmp) {
                impossible("leashed-unleashed mon?");
                return TRUE;
            }

            if (otmp->cursed)
                return TRUE;
            mtmp->mleashed = 0;
            otmp->leashmon = 0;
            update_inventory();
            You_feel("%s leash go slack.",
                     (number_leashed() > 1) ? "a" : "the");
        }
    }
    return FALSE;
}

boolean
next_to_u(void)
{
    if (get_iter_mons(mleashed_next2u))
        return FALSE;

    /* no pack mules for the Amulet */
    if (u.usteed && mon_has_amulet(u.usteed))
        return FALSE;
    return TRUE;
}

void
check_leash(coordxy x, coordxy y)
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = gi.invent; otmp; otmp = otmp->nobj) {
        if (otmp->otyp != LEASH || otmp->leashmon == 0)
            continue;
        mtmp = find_mid(otmp->leashmon, FM_FMON);
        if (!mtmp) {
            impossible("leash in use isn't attached to anything?");
            otmp->leashmon = 0;
            continue;
        }
        if (dist2(u.ux, u.uy, mtmp->mx, mtmp->my)
            > dist2(x, y, mtmp->mx, mtmp->my)) {
            if (!um_dist(mtmp->mx, mtmp->my, 3)) {
                ; /* still close enough */
            } else if (otmp->cursed && !breathless(mtmp->data)) {
                if (um_dist(mtmp->mx, mtmp->my, 5)
                    || (mtmp->mhp -= rnd(2)) <= 0) {
                    long save_pacifism = u.uconduct.killer;

                    Your("leash chokes %s to death!", mon_nam(mtmp));
                    /* hero might not have intended to kill pet, but
                       that's the result of his actions; gain experience,
                       lose pacifism, take alignment and luck hit, make
                       corpse less likely to remain tame after revival */
                    xkilled(mtmp, XKILL_NOMSG);
                    /* life-saving doesn't ordinarily reset this */
                    if (!DEADMONSTER(mtmp))
                        u.uconduct.killer = save_pacifism;
                } else {
                    pline("%s is choked by the leash!", Monnam(mtmp));
                    /* tameness eventually drops to 1 here (never 0) */
                    if (mtmp->mtame && rn2(mtmp->mtame))
                        mtmp->mtame--;
                }
            } else {
                if (um_dist(mtmp->mx, mtmp->my, 5)) {
                    pline("%s leash snaps loose!", s_suffix(Monnam(mtmp)));
                    m_unleash(mtmp, FALSE);
                } else {
                    You("pull on the leash.");
                    if (mtmp->data->msound != MS_SILENT)
                        switch (rn2(3)) {
                        case 0:
                            growl(mtmp);
                            break;
                        case 1:
                            yelp(mtmp);
                            break;
                        default:
                            whimper(mtmp);
                            break;
                        }
                }
            }
        }
    }
}

/* charisma is supposed to include qualities like leadership and personal
   magnetism rather than just appearance, but it has devolved to this... */
const char *
beautiful(void)
{
    const char *res;
    int cha = ACURR(A_CHA);

    /* don't bother complaining about the sexism; nethack is not real life */
    res = ((cha >= 25) ? "sublime" /* 25 is the maximum possible */
           : (cha >= 19) ? "splendorous" /* note: not "splendiferous" */
             : (cha >= 16) ? ((poly_gender() == 1) ? "beautiful" : "handsome")
               : (cha >= 14) ? ((poly_gender() == 1) ? "winsome" : "amiable")
                 : (cha >= 11) ? "cute"
                   : (cha >= 9) ? "plain"
                     : (cha >= 6) ? "homely"
                       : (cha >= 4) ? "ugly"
                         : "hideous"); /* 3 is the minimum possible */
    return res;
}

static const char look_str[] = "look %s.";

static int
use_mirror(struct obj *obj)
{
    const char *mirror, *uvisage;
    struct monst *mtmp;
    unsigned how_seen;
    char mlet;
    boolean vis, invis_mirror, useeit, monable;

    if (!getdir((char *) 0))
        return ECMD_CANCEL;
    invis_mirror = Invis;
    useeit = !Blind && (!invis_mirror || See_invisible);
    uvisage = beautiful();
    mirror = simpleonames(obj); /* "mirror" or "looking glass" */
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
            pline_The("%s fogs up and doesn't reflect!", mirror);
        else
            pline("%s", Nothing_seems_to_happen);
        return ECMD_TIME;
    }
    if (!u.dx && !u.dy && !u.dz) {
        if (!useeit) {
            You_cant("see your %s %s.", uvisage, body_part(FACE));
        } else {
            if (u.umonnum == PM_FLOATING_EYE) {
                if (Free_action) {
                    You("stiffen momentarily under your gaze.");
                } else {
                    if (Hallucination)
                        pline("Yow!  The %s stares back!", mirror);
                    else
                        pline("Yikes!  You've frozen yourself!");
                    if (!Hallucination || !rn2(4)) {
                        nomul(-rnd(MAXULEV + 6 - u.ulevel));
                        gm.multi_reason = "gazing into a mirror";
                    }
                    gn.nomovemsg = 0; /* default, "you can move again" */
                }
            } else if (is_vampire(gy.youmonst.data)
                       || is_vampshifter(&gy.youmonst)) {
                You("don't have a reflection.");
            } else if (u.umonnum == PM_UMBER_HULK) {
                pline("Huh?  That doesn't look like you!");
                make_confused(HConfusion + d(3, 4), FALSE);
            } else if (Hallucination) {
                You(look_str, hcolor((char *) 0));
            } else if (Sick) {
                You(look_str, "peaked");
            } else if (u.uhs >= WEAK) {
                You(look_str, "undernourished");
            } else if (Upolyd) {
                You("look like %s.", an(pmname(&mons[u.umonnum], Ugender)));
            } else {
                You("look as %s as ever.", uvisage);
            }
        }
        return ECMD_TIME;
    }
    if (u.uswallow) {
        if (useeit)
            You("reflect %s %s.", s_suffix(mon_nam(u.ustuck)),
                mbodypart(u.ustuck, STOMACH));
        return ECMD_TIME;
    }
    if (Underwater) {
        if (useeit)
            You("%s.",
                Hallucination ? "give the fish a chance to fix their makeup"
                              : "reflect the murky water");
        return ECMD_TIME;
    }
    if (u.dz) {
        if (useeit)
            You("reflect the %s.",
                (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
        return ECMD_TIME;
    }
    mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
                (int (*) (MONST_P, OBJ_P)) 0,
                (int (*) (OBJ_P, OBJ_P)) 0, &obj);
    if (!mtmp || !haseyes(mtmp->data) || gn.notonhead)
        return ECMD_TIME;

    /* couldsee(mtmp->mx, mtmp->my) is implied by the fact that bhit()
       targeted it, so we can ignore possibility of X-ray vision */
    vis = canseemon(mtmp);
    /* ways to directly see monster (excludes X-ray vision, telepathy,
       extended detection, type-specific warning) */
#define SEENMON (MONSEEN_NORMAL | MONSEEN_SEEINVIS | MONSEEN_INFRAVIS)
    how_seen = vis ? howmonseen(mtmp) : 0;
    /* whether monster is able to use its vision-based capabilities */
    monable = !mtmp->mcan && (!mtmp->minvis || perceives(mtmp->data));
    mlet = mtmp->data->mlet;
    if (mtmp->msleeping) {
        if (vis)
            pline("%s is too tired to look at your %s.", Monnam(mtmp),
                  mirror);
    } else if (!mtmp->mcansee) {
        if (vis)
            pline("%s can't see anything right now.", Monnam(mtmp));
    } else if (invis_mirror && !perceives(mtmp->data)) {
        if (vis)
            pline("%s fails to notice your %s.", Monnam(mtmp), mirror);
        /* infravision doesn't produce an image in the mirror */
    } else if ((how_seen & SEENMON) == MONSEEN_INFRAVIS) {
        if (vis) /* (redundant) */
            pline("%s in the dark.",
                  monverbself(mtmp, Monnam(mtmp), "are",
                              "too far away to see"));
        /* some monsters do special things */
    } else if (mlet == S_VAMPIRE || mlet == S_GHOST || is_vampshifter(mtmp)) {
        if (vis)
            pline("%s doesn't have a reflection.", Monnam(mtmp));
    } else if (monable && mtmp->data == &mons[PM_MEDUSA]) {
        if (mon_reflects(mtmp, "The gaze is reflected away by %s %s!"))
            return ECMD_TIME;
        if (vis)
            pline("%s is turned to stone!", Monnam(mtmp));
        gs.stoned = TRUE;
        killed(mtmp);
    } else if (monable && mtmp->data == &mons[PM_FLOATING_EYE]) {
        int tmp = d((int) mtmp->m_lev, (int) mtmp->data->mattk[0].damd);
        if (!rn2(4))
            tmp = 120;
        if (vis)
            pline("%s is frozen by its reflection.", Monnam(mtmp));
        else
            You_hear("%s stop moving.", something);
        paralyze_monst(mtmp, (int) mtmp->mfrozen + tmp);
    } else if (monable && mtmp->data == &mons[PM_UMBER_HULK]) {
        if (vis)
            pline("%s confuses itself!", Monnam(mtmp));
        mtmp->mconf = 1;
    } else if (monable && (mlet == S_NYMPH
                           || mtmp->data == &mons[PM_AMOROUS_DEMON])) {
        if (vis) {
            char buf[BUFSZ]; /* "She" or "He" */

            pline("%s in your %s.", /* "<mon> admires self in your mirror " */
                  monverbself(mtmp, Monnam(mtmp), "admire", (char *) 0),
                  mirror);
            pline("%s takes it!", upstart(strcpy(buf, mhe(mtmp))));
        } else
            pline("It steals your %s!", mirror);
        setnotworn(obj); /* in case mirror was wielded */
        freeinv(obj);
        (void) mpickobj(mtmp, obj);
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, RLOC_MSG);
    } else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data)
               && (!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
        boolean do_react = TRUE;

        if (mtmp->mfrozen) {
            if (vis)
                You("discern no obvious reaction from %s.", mon_nam(mtmp));
            else
                You_feel(
                       "a bit silly gesturing the mirror in that direction.");
            do_react = FALSE;
        }
        if (do_react) {
            if (vis)
                pline("%s is frightened by its reflection.", Monnam(mtmp));
            monflee(mtmp, d(2, 4), FALSE, FALSE);
        }
    } else if (!Blind) {
        if (mtmp->minvis && !See_invisible)
            ;
        else if ((mtmp->minvis && !perceives(mtmp->data))
                 /* redundant: can't get here if these are true */
                 || !haseyes(mtmp->data) || gn.notonhead || !mtmp->mcansee)
            pline("%s doesn't seem to notice %s reflection.", Monnam(mtmp),
                  mhis(mtmp));
        else
            pline("%s ignores %s reflection.", Monnam(mtmp), mhis(mtmp));
    }
    return ECMD_TIME;
}

static void
use_bell(struct obj **optr)
{
    register struct obj *obj = *optr;
    struct monst *mtmp;
    boolean wakem = FALSE, learno = FALSE,
            ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
            invoking = (obj->otyp == BELL_OF_OPENING
                        && invocation_pos(u.ux, u.uy)
                        && !On_stairs(u.ux, u.uy));

    Hero_playnotes(obj_to_instr(obj), "C", 100);
    You("ring %s.", the(xname(obj)));

    if (Underwater || (u.uswallow && ordinary)) {
        pline("But the sound is muffled.");

    } else if (invoking && ordinary) {
        /* needs to be recharged... */
        pline("But it makes no sound.");
        learno = TRUE; /* help player figure out why */

    } else if (ordinary) {
        if (obj->cursed && !rn2(4)
            /* note: once any of them are gone, we stop all of them */
            && !(gm.mvitals[PM_WOOD_NYMPH].mvflags & G_GONE)
            && !(gm.mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
            && !(gm.mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE)
            && (mtmp = makemon(mkclass(S_NYMPH, 0), u.ux, u.uy,
                               NO_MINVENT | MM_NOMSG)) != 0) {
            You("summon %s!", a_monnam(mtmp));
            if (!obj_resists(obj, 93, 100)) {
                pline("%s shattered!", Tobjnam(obj, "have"));
                useup(obj);
                *optr = 0;
            } else
                switch (rn2(3)) {
                default:
                    break;
                case 1:
                    mon_adjust_speed(mtmp, 2, (struct obj *) 0);
                    break;
                case 2: /* no explanation; it just happens... */
                    gn.nomovemsg = "";
                    gm.multi_reason = NULL;
                    nomul(-rnd(2));
                    break;
                }
        }
        wakem = TRUE;

    } else {
        /* charged Bell of Opening */
        consume_obj_charge(obj, TRUE);

        if (u.uswallow) {
            if (!obj->cursed)
                (void) openit();
            else
                pline1(nothing_happens);

        } else if (obj->cursed) {
            coord mm;

            mm.x = u.ux;
            mm.y = u.uy;
            mkundead(&mm, FALSE, NO_MINVENT);
            wakem = TRUE;

        } else if (invoking) {
            pline("%s an unsettling shrill sound...", Tobjnam(obj, "issue"));
            obj->age = gm.moves;
            learno = TRUE;
            wakem = TRUE;

        } else if (obj->blessed) {
            int res = 0;

            if (uchain) {
                unpunish();
                res = 1;
            } else if (u.utrap && u.utraptype == TT_BURIEDBALL) {
                buried_ball_to_freedom();
                res = 1;
            }
            res += openit();
            switch (res) {
            case 0:
                pline1(nothing_happens);
                break;
            case 1:
                pline("%s opens...", Something);
                learno = TRUE;
                break;
            default:
                pline("Things open around you...");
                learno = TRUE;
                break;
            }

        } else { /* uncursed */
            if (findit() != 0)
                learno = TRUE;
            else
                pline1(nothing_happens);
        }

    } /* charged BofO */

    if (learno) {
        makeknown(BELL_OF_OPENING);
        obj->known = 1;
    }
    if (wakem)
        wake_nearby();
}

static void
use_candelabrum(struct obj *obj)
{
    const char *s = (obj->spe != 1) ? "candles" : "candle";

    if (obj->lamplit) {
        You("snuff the %s.", s);
        end_burn(obj, TRUE);
        return;
    }
    if (obj->spe <= 0) {
        struct obj *otmp;

        pline("This %s has no %s.", xname(obj), s);
        /* only output tip if candles are in inventory */
        for (otmp = gi.invent; otmp; otmp = otmp->nobj)
            if (Is_candle(otmp))
                break;
        if (otmp)
            pline("To attach candles, apply them instead of the %s.",
                  xname(obj));
        return;
    }
    if (Underwater) {
        You("cannot make fire under water.");
        return;
    }
    if (u.uswallow || obj->cursed) {
        if (!Blind)
            pline_The("%s %s for a moment, then %s.", s, vtense(s, "flicker"),
                      vtense(s, "die"));
        return;
    }
    if (obj->spe < 7) {
        There("%s only %d %s in %s.", vtense(s, "are"), obj->spe, s,
              the(xname(obj)));
        if (!Blind)
            pline("%s lit.  %s dimly.", obj->spe == 1 ? "It is" : "They are",
                  Tobjnam(obj, "shine"));
    } else {
        pline("%s's %s burn%s", The(xname(obj)), s,
              (Blind ? "." : " brightly!"));
    }
    if (!invocation_pos(u.ux, u.uy) || On_stairs(u.ux, u.uy)) {
        pline_The("%s %s being rapidly consumed!", s, vtense(s, "are"));
        /* this used to be obj->age /= 2, rounding down; an age of
           1 would yield 0, confusing begin_burn() and producing an
           unlightable, unrefillable candelabrum; round up instead */
        obj->age = (obj->age + 1L) / 2L;

        /* to make absolutely sure the game doesn't become unwinnable as
           a consequence of a broken candelabrum */
        if (obj->age == 0) {
            impossible("Candelabrum with candles but no fuel?");
            obj->age = 1;
        }
    } else {
        if (obj->spe == 7) {
            if (Blind)
                pline("%s a strange warmth!", Tobjnam(obj, "radiate"));
            else
                pline("%s with a strange light!", Tobjnam(obj, "glow"));
        }
        obj->known = 1;
    }
    begin_burn(obj, FALSE);
}

static void
use_candle(struct obj **optr)
{
    register struct obj *obj = *optr;
    register struct obj *otmp;
    const char *s = (obj->quan != 1) ? "candles" : "candle";
    char qbuf[QBUFSZ], qsfx[QBUFSZ], *q;
    boolean was_lamplit;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    /* obj is the candle; otmp is the candelabrum */
    otmp = carrying(CANDELABRUM_OF_INVOCATION);
    if (!otmp || otmp->spe == 7) {
        use_lamp(obj);
        return;
    }

    /* first, minimal candelabrum suffix for formatting candles */
    Sprintf(qsfx, " to\033%s?", thesimpleoname(otmp));
    /* next, format the candles as a prefix for the candelabrum */
    (void) safe_qbuf(qbuf, "Attach ", qsfx, obj, yname, thesimpleoname, s);
    /* strip temporary candelabrum suffix */
    if ((q = strstri(qbuf, " to\033")) != 0)
        Strcpy(q, " to ");
    /* last, format final "attach candles to candelabrum?" query */
    if (y_n(safe_qbuf(qbuf, qbuf, "?", otmp, yname, thesimpleoname, "it"))
        == 'n') {
        use_lamp(obj);
        return;
    } else {
        if ((long) otmp->spe + obj->quan > 7L) {
            obj = splitobj(obj, 7L - (long) otmp->spe);
            /* avoid a grammatical error if obj->quan gets
               reduced to 1 candle from more than one */
            s = (obj->quan != 1) ? "candles" : "candle";
        } else
            *optr = 0;

        /* The candle's age field doesn't correctly reflect the amount
           of fuel in it while it's lit, because the fuel is measured
           by the timer. So to get accurate age updating, we need to
           end the burn temporarily while attaching the candle. */
        was_lamplit = obj->lamplit;
        if (was_lamplit)
            end_burn(obj, TRUE);

        You("attach %ld%s %s to %s.", obj->quan, !otmp->spe ? "" : " more", s,
            the(xname(otmp)));
        if (!otmp->spe || otmp->age > obj->age)
            otmp->age = obj->age;
        otmp->spe += (int) obj->quan;
        if (otmp->lamplit && !was_lamplit)
            pline_The("new %s magically %s!", s, vtense(s, "ignite"));
        else if (!otmp->lamplit && was_lamplit)
            pline("%s out.", (obj->quan > 1L) ? "They go" : "It goes");
        if (obj->unpaid) {
            struct monst *shkp VOICEONLY = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

            SetVoice(shkp, 0, 80, 0);
            verbalize("You %s %s, you bought %s!",
                      otmp->lamplit ? "burn" : "use",
                      (obj->quan > 1L) ? "them" : "it",
                      (obj->quan > 1L) ? "them" : "it");
        }
        if (obj->quan < 7L && otmp->spe == 7)
            pline("%s now has seven%s candles attached.", The(xname(otmp)),
                  otmp->lamplit ? " lit" : "");
        /* candelabrum's light range might increase */
        if (otmp->lamplit)
            obj_merge_light_sources(otmp, otmp);
        /* candles are no longer a separate light source */
        /* candles are now gone */
        useupall(obj);
        /* candelabrum's weight is changing */
        otmp->owt = weight(otmp);
        update_inventory();
    }
}

/* call in drop, throw, and put in box, etc. */
boolean
snuff_candle(struct obj *otmp)
{
    boolean candle = Is_candle(otmp);

    if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION)
        && otmp->lamplit) {
        char buf[BUFSZ];
        coordxy x, y;
        boolean many = candle ? (otmp->quan > 1L) : (otmp->spe > 1);

        (void) get_obj_location(otmp, &x, &y, 0);
        if (otmp->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
            pline("%s%scandle%s flame%s extinguished.", Shk_Your(buf, otmp),
                  (candle ? "" : "candelabrum's "), (many ? "s'" : "'s"),
                  (many ? "s are" : " is"));
        end_burn(otmp, TRUE);
        return TRUE;
    }
    return FALSE;
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(struct obj *obj)
{
    coordxy x, y;

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN || obj->otyp == POT_OIL) {
            (void) get_obj_location(obj, &x, &y, 0);
            if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
                pline("%s %s out!", Yname2(obj), otense(obj, "go"));
            end_burn(obj, TRUE);
            return TRUE;
        }
        if (snuff_candle(obj))
            return TRUE;
    }
    return FALSE;
}

/* called when lit object is hit by water */
boolean
splash_lit(struct obj *obj)
{
    boolean result, dunk = FALSE;

    /* lantern won't be extinguished by a rust trap or rust monster attack
       but will be if submerged or placed into a container or swallowed by
       a monster (for mobile light source handling, not because it ought
       to stop being lit in all those situations...) */
    if (obj->lamplit && obj->otyp == BRASS_LANTERN) {
        struct monst *mtmp;
        boolean useeit = FALSE, uhearit = FALSE, snuff = TRUE;

        if (obj->where == OBJ_INVENT) {
            useeit = !Blind;
            uhearit = !Deaf;
            /* underwater light sources aren't allowed but if hero
               is just entering water, Underwater won't be set yet */
            dunk = (is_pool(u.ux, u.uy)
                    && ((!Levitation && !Flying && !Wwalking)
                        || Is_waterlevel(&u.uz)));
            snuff = FALSE;
        } else if (obj->where == OBJ_MINVENT
                   /* don't assume that lit lantern has been swallowed;
                      a nymph might have stolen it or picked it up */
                   && ((mtmp = obj->ocarry), humanoid(mtmp->data))) {
            coordxy x, y;

            useeit = get_obj_location(obj, &x, &y, 0) && cansee(x, y);
            uhearit = couldsee(x, y) && distu(x, y) < 5 * 5;
            dunk = (is_pool(mtmp->mx, mtmp->my)
                    && ((!is_flyer(mtmp->data) && !is_floater(mtmp->data))
                        || Is_waterlevel(&u.uz)));
            snuff = FALSE;
        }

        if (useeit || uhearit)
            pline("%s %s%s%s.", Yname2(obj),
                  uhearit ? "crackles" : "",
                  (uhearit && useeit) ? " and " : "",
                  useeit ? "flickers" : "");
        if (!dunk && !snuff)
            return FALSE;
    }

    result = snuff_lit(obj);

    /* this is simpler when we wait until after lantern has been snuffed */
    if (dunk) {
        /* drain some of the battery but don't short it out entirely */
        obj->age -= (obj->age > 200L) ? 100L : (obj->age / 2L);
    }
    return result;
}

/* Called when potentially lightable object is affected by fire_damage().
   Return TRUE if object becomes lit and FALSE otherwise --ALI */
boolean
catch_lit(struct obj *obj)
{
    coordxy x, y;

    if (!obj->lamplit && ignitable(obj) && get_obj_location(obj, &x, &y, 0)) {
        if (((obj->otyp == MAGIC_LAMP /* spe==0 => no djinni inside */
              /* spe==0 => no candles attached */
              || obj->otyp == CANDELABRUM_OF_INVOCATION) && obj->spe == 0)
            /* age_is_relative && age==0 && still-exists means out of fuel */
            || (age_is_relative(obj) && obj->age == 0)
            /* lantern is classified as ignitable() but not by fire */
            || obj->otyp == BRASS_LANTERN)
            return FALSE;
        if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->cursed)
            return FALSE;
        if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP)
            /* once lit, cursed lamp is as good as non-cursed one, so failure
               to light is a minor inconvenience to make cursed be worse */
            && obj->cursed && !rn2(2))
            return FALSE;

        if (obj->where == OBJ_INVENT || cansee(x, y))
            pline("%s %s %s", Yname2(obj),
                  /* "catches light!" or "feels warm." */
                  otense(obj, Blind ? "feel" : "catch"),
                  Blind ? "warm." : "light!");
        if (obj->otyp == POT_OIL)
            makeknown(obj->otyp);
        if (carried(obj) && obj->unpaid && costly_spot(u.ux, u.uy)) {
            struct monst *shkp VOICEONLY = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

            /* if it catches while you have it, then it's your tough luck */
            check_unpaid(obj);
            SetVoice(shkp, 0, 80, 0);
            verbalize("That's in addition to the cost of %s %s, of course.",
                      yname(obj),
                      (obj->quan == 1L) ? "itself" : "themselves");
            bill_dummy_object(obj);
        }
        begin_burn(obj, FALSE);
        return TRUE;
    }
    return FALSE;
}

/* light a lamp or candle */
static void
use_lamp(struct obj *obj)
{
    char buf[BUFSZ];
    const char *lamp = (obj->otyp == OIL_LAMP
                        || obj->otyp == MAGIC_LAMP) ? "lamp"
                       : (obj->otyp == BRASS_LANTERN) ? "lantern"
                         : NULL;

    /*
     * When blind, lamps' and candles' on/off state can be distinguished
     * by heat.  For brass lantern assume that there is an on/off switch
     * that can be felt.
     */

    if (obj->lamplit) {
        if (lamp) /* lamp or lantern */
            pline("%s%s is now off.", Shk_Your(buf, obj), lamp);
        else
            You("snuff out %s.", yname(obj));
        end_burn(obj, TRUE);
        return;
    }
    if (Underwater) {
        pline("%s.",
              !Is_candle(obj) ? "This is not a diving lamp"
                              : "Sorry, fire and water don't mix");
        return;
    }
    /* magic lamps with an spe == 0 (wished for) cannot be lit */
    if ((!Is_candle(obj) && obj->age == 0)
        || (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
        if (obj->otyp == BRASS_LANTERN) {
            if (!Blind)
                Your("lantern is out of power.");
            else
                pline("%s", Nothing_seems_to_happen);
        } else {
            pline("This %s has no oil.", xname(obj));
        }
        return;
    }
    if (obj->cursed && !rn2(2)) {
        if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP) && !rn2(3)) {
            pline_The("lamp spills and covers your %s with oil.",
                      fingers_or_gloves(TRUE));
            make_glib((int) (Glib & TIMEOUT) + d(2, 10));
        } else if (!Blind) {
            pline("%s for a moment, then %s.", Tobjnam(obj, "flicker"),
                  otense(obj, "die"));
        } else {
            pline("%s", Nothing_seems_to_happen);
        }
    } else {
        if (lamp) { /* lamp or lantern */
            check_unpaid(obj);
            pline("%s%s is now on.", Shk_Your(buf, obj), lamp);
        } else { /* candle(s) */
            pline("%s flame%s %s%s", s_suffix(Yname2(obj)), plur(obj->quan),
                  otense(obj, "burn"), Blind ? "." : " brightly!");
            if (obj->unpaid && costly_spot(u.ux, u.uy)
                && obj->age == 20L * (long) objects[obj->otyp].oc_cost) {
                const char *ithem = (obj->quan > 1L) ? "them" : "it";
                struct monst *shkp VOICEONLY = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

                SetVoice(shkp, 0, 80, 0);
                verbalize("You burn %s, you bought %s!", ithem, ithem);
                bill_dummy_object(obj);
            }
        }
        begin_burn(obj, FALSE);
    }
}

static void
light_cocktail(struct obj **optr)
{
    struct obj *obj = *optr; /* obj is a potion of oil */
    char buf[BUFSZ];
    boolean split1off;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    if (obj->lamplit) {
        You("snuff the lit potion.");
        end_burn(obj, TRUE);
        /*
         * Free & add to re-merge potion.  This will average the
         * age of the potions.  Not exactly the best solution,
         * but its easy.
         */
        freeinv(obj);
        *optr = addinv(obj);
        return;
    } else if (Underwater) {
        There("is not enough oxygen to sustain a fire.");
        return;
    }

    split1off = (obj->quan > 1L);
    if (split1off)
        obj = splitobj(obj, 1L);

    You("light %spotion.%s", shk_your(buf, obj),
        Blind ? "" : "  It gives off a dim light.");

    if (obj->unpaid && costly_spot(u.ux, u.uy)) {
        struct monst *shkp VOICEONLY = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

        /* Normally, we shouldn't both partially and fully charge
         * for an item, but (Yendorian Fuel) Taxes are inevitable...
         */
        check_unpaid(obj);
        SetVoice(shkp, 0, 80, 0);
        verbalize("That's in addition to the cost of the potion, of course.");
        bill_dummy_object(obj);
    }
    makeknown(obj->otyp);

    begin_burn(obj, FALSE); /* after shop billing */
    if (split1off) {
        obj_extract_self(obj); /* free from inv */
        obj->nomerge = 1;
        obj = hold_another_object(obj, "You drop %s!", doname(obj),
                                  (const char *) 0);
        if (obj)
            obj->nomerge = 0;
    }
    *optr = obj;
}

/* getobj callback for object to be rubbed - not selecting a secondary object
   to rub on a gray stone or rub jelly on */
static int
rub_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
        || obj->otyp == BRASS_LANTERN || is_graystone(obj)
        || obj->otyp == LUMP_OF_ROYAL_JELLY)
        return GETOBJ_SUGGEST;

    return GETOBJ_EXCLUDE;
}

/* the #rub command */
int
dorub(void)
{
    struct obj *obj;

    if (nohands(gy.youmonst.data)) {
        You("aren't able to rub anything without hands.");
        return ECMD_OK;
    }
    obj = getobj("rub", rub_ok, GETOBJ_NOFLAGS);
    if (!obj)
        return ECMD_CANCEL;
    if (obj->oclass == GEM_CLASS || obj->oclass == FOOD_CLASS) {
        if (is_graystone(obj)) {
            return use_stone(obj);
        } else if (obj->otyp == LUMP_OF_ROYAL_JELLY) {
            return use_royal_jelly(obj);
        } else {
            pline("Sorry, I don't know how to use that.");
            return ECMD_OK;
        }
    }
    if (obj != uwep) {
        if (wield_tool(obj, "rub")) {
            cmdq_add_ec(CQ_CANNED, dorub);
            cmdq_add_key(CQ_CANNED, obj->invlet);
            return ECMD_TIME;
        }
        return ECMD_OK;
    }

    /* now uwep is obj */
    if (uwep->otyp == MAGIC_LAMP) {
        if (uwep->spe > 0 && !rn2(3)) {
            check_unpaid_usage(uwep, TRUE); /* unusual item use */
            /* bones preparation:  perform the lamp transformation
               before releasing the djinni in case the latter turns out
               to be fatal (a hostile djinni has no chance to attack yet,
               but an indebted one who grants a wish might bestow an
               artifact which blasts the hero with lethal results) */
            uwep->otyp = OIL_LAMP;
            uwep->spe = 0; /* for safety */
            uwep->age = rn1(500, 1000);
            if (uwep->lamplit)
                begin_burn(uwep, TRUE);
            djinni_from_bottle(uwep);
            makeknown(MAGIC_LAMP);
            update_inventory();
        } else if (rn2(2)) {
            You("%s smoke.", !Blind ? "see a puff of" : "smell");
        } else
            pline1(nothing_happens);
    } else if (obj->otyp == BRASS_LANTERN) {
        /* message from Adventure */
        pline("Rubbing the electric lamp is not particularly rewarding.");
        pline("Anyway, nothing exciting happens.");
    } else
        pline1(nothing_happens);
    return ECMD_TIME;
}

/* the #jump command */
int
dojump(void)
{
    /* Physical jump */
    return jump(0);
}

enum jump_trajectory {
    jAny  = 0, /* any direction => magical jump */
    jHorz = 1,
    jVert = 2,
    jDiag = 3  /* jHorz|jVert */
};

/* callback routine for walk_path() */
static boolean
check_jump(genericptr arg, coordxy x, coordxy y)
{
    int traj = *(int *) arg;
    struct rm *lev = &levl[x][y];

    if (Passes_walls)
        return TRUE;
    if (IS_STWALL(lev->typ))
        return FALSE;
    if (IS_DOOR(lev->typ)) {
        if (closed_door(x, y))
            return FALSE;
        if ((lev->doormask & D_ISOPEN) != 0 && traj != jAny
            /* reject diagonal jump into or out-of or through open door */
            && (traj == jDiag
                /* reject horizontal jump through horizontal open door
                   and non-horizontal (ie, vertical) jump through
                   non-horizontal (vertical) open door */
                || ((traj & jHorz) != 0) == (lev->horizontal != 0)))
            return FALSE;
        /* empty doorways aren't restricted */
    }
    /* let giants jump over boulders (what about Flying?
       and is there really enough head room for giants to jump
       at all, let alone over something tall?) */
    if (sobj_at(BOULDER, x, y) && !throws_rocks(gy.youmonst.data))
        return FALSE;
    return TRUE;
}

static boolean
is_valid_jump_pos(coordxy x, coordxy y, int magic, boolean showmsg)
{
    if (!magic && !(HJumping & ~INTRINSIC) && !EJumping && distu(x, y) != 5) {
        /* The Knight jumping restriction still applies when riding a
         * horse.  After all, what shape is the knight piece in chess?
         */
        if (showmsg)
            pline("Illegal move!");
        return FALSE;
    } else if (distu(x, y) > (magic ? 6 + magic * 3 : 9)) {
        if (showmsg)
            pline("Too far!");
        return FALSE;
    } else if (!isok(x, y)) {
        if (showmsg)
            You("cannot jump there!");
        return FALSE;
    } else if (!cansee(x, y)) {
        if (showmsg)
            You("cannot see where to land!");
        return FALSE;
    } else {
        coord uc, tc;
        struct rm *lev = &levl[u.ux][u.uy];
        /* we want to categorize trajectory for use in determining
           passage through doorways: horizonal, vertical, or diagonal;
           since knight's jump and other irregular directions are
           possible, we flatten those out to simplify door checks */
        int diag, traj;
        coordxy dx = x - u.ux, dy = y - u.uy,
                ax = abs(dx), ay = abs(dy);

        /* diag: any non-orthogonal destination classified as diagonal */
        diag = (magic || Passes_walls || (!dx && !dy)) ? jAny
               : !dy ? jHorz : !dx ? jVert : jDiag;
        /* traj: flatten out the trajectory => some diagonals re-classified */
        if (ax >= 2 * ay)
            ay = 0;
        else if (ay >= 2 * ax)
            ax = 0;
        traj = (magic || Passes_walls || (!ax && !ay)) ? jAny
               : !ay ? jHorz : !ax ? jVert : jDiag;
        /* walk_path doesn't process the starting spot;
           this is iffy:  if you're starting on a closed door spot,
           you _can_ jump diagonally from doorway (without needing
           Passes_walls); that's intentional but is it correct? */
        if (diag == jDiag && IS_DOOR(lev->typ)
            && (lev->doormask & D_ISOPEN) != 0
            && (traj == jDiag
                || ((traj & jHorz) != 0) == (lev->horizontal != 0))) {
            if (showmsg)
                You_cant("jump diagonally out of a doorway.");
            return FALSE;
        }
        uc.x = u.ux, uc.y = u.uy;
        tc.x = x, tc.y = y; /* target */
        if (!walk_path(&uc, &tc, check_jump, (genericptr_t) &traj)) {
            if (showmsg)
                There("is an obstacle preventing that jump.");
            return FALSE;
        }
    }
    return TRUE;
}

static boolean
get_valid_jump_position(coordxy x, coordxy y)
{
    return (isok(x, y)
            && (ACCESSIBLE(levl[x][y].typ) || Passes_walls)
            && is_valid_jump_pos(x, y, gj.jumping_is_magic, FALSE));
}

static void
display_jump_positions(int state)
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        coordxy x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (coordxy) u.ux;
                y = dy + (coordxy) u.uy;
                if (get_valid_jump_position(x, y))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

int
jump(int magic) /* 0=Physical, otherwise skill level */
{
    coord cc;

    /* attempt "jumping" spell if hero has no innate jumping ability */
    if (!magic && !Jumping && known_spell(SPE_JUMPING) >= spe_Fresh)
        return spelleffects(SPE_JUMPING, FALSE, FALSE);

    if (!magic && (nolimbs(gy.youmonst.data) || slithy(gy.youmonst.data))) {
        /* normally (nolimbs || slithy) implies !Jumping,
           but that isn't necessarily the case for knights */
        You_cant("jump; you have no legs!");
        return ECMD_OK;
    } else if (!magic && !Jumping) {
        You_cant("jump very far.");
        return ECMD_OK;

    /* if steed is immobile, can't do physical jump but can do spell one */
    } else if (!magic && u.usteed && stucksteed(FALSE)) {
        /* stucksteed gave "<steed> won't move" message */
        return ECMD_OK;
    } else if (u.uswallow) {
        if (magic) {
            You("bounce around a little.");
            return ECMD_TIME;
        }
        pline("You've got to be kidding!");
        return ECMD_OK;
    } else if (u.uinwater) {
        if (magic) {
            You("swish around a little.");
            return ECMD_TIME;
        }
        pline("This calls for swimming, not jumping!");
        return ECMD_OK;
    } else if (u.ustuck) {
        if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf) {
            struct monst *mtmp = u.ustuck;

            set_ustuck((struct monst *) 0);
            You("pull free from %s.", mon_nam(mtmp));
            return ECMD_TIME;
        }
        if (magic) {
            You("writhe a little in the grasp of %s!", mon_nam(u.ustuck));
            return ECMD_TIME;
        }
        You("cannot escape from %s!", mon_nam(u.ustuck));
        return ECMD_OK;
    } else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        if (magic) {
            You("flail around a little.");
            return ECMD_TIME;
        }
        You("don't have enough traction to jump.");
        return ECMD_OK;
    } else if (!magic && near_capacity() > UNENCUMBERED) {
        You("are carrying too much to jump!");
        return ECMD_OK;
    } else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
        You("lack the strength to jump!");
        return ECMD_OK;
    } else if (!magic && Wounded_legs) {
        legs_in_no_shape("jumping", u.usteed != 0);
        return ECMD_OK;
    } else if (u.usteed && u.utrap) {
        pline("%s is stuck in a trap.", Monnam(u.usteed));
        return ECMD_OK;
    }

    pline("Where do you want to jump?");
    cc.x = u.ux;
    cc.y = u.uy;
    gj.jumping_is_magic = magic;
    getpos_sethilite(display_jump_positions, get_valid_jump_position);
    if (getpos(&cc, TRUE, "the desired position") < 0)
        return ECMD_CANCEL; /* user pressed ESC */
    if (!is_valid_jump_pos(cc.x, cc.y, magic, TRUE)) {
        return ECMD_FAIL;
    } else {
        coord uc;
        int range, temp;

        if (u.utrap)
            switch (u.utraptype) {
            case TT_BEARTRAP: {
                long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;

                You("rip yourself free of the bear trap!  Ouch!");
                losehp(Maybe_Half_Phys(rnd(10)), "jumping out of a bear trap",
                       KILLED_BY);
                set_wounded_legs(side, rn1(1000, 500));
                break;
            }
            case TT_PIT:
                You("leap from the pit!");
                break;
            case TT_WEB:
                You("tear the web apart as you pull yourself free!");
                deltrap(t_at(u.ux, u.uy));
                break;
            case TT_LAVA:
                You("pull yourself above the %s!", hliquid("lava"));
                reset_utrap(TRUE);
                return ECMD_TIME;
            case TT_BURIEDBALL:
            case TT_INFLOOR:
                You("strain your %s, but you're still %s.",
                    makeplural(body_part(LEG)),
                    (u.utraptype == TT_INFLOOR)
                        ? "stuck in the floor"
                        : "attached to the buried ball");
                set_wounded_legs(LEFT_SIDE, rn1(10, 11));
                set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
                return ECMD_TIME;
            }

        /*
         * Check the path from uc to cc, calling hurtle_step at each
         * location.  The final position actually reached will be
         * in cc.
         */
        uc.x = u.ux;
        uc.y = u.uy;
        /* calculate max(abs(dx), abs(dy)) as the range */
        range = cc.x - uc.x;
        if (range < 0)
            range = -range;
        temp = cc.y - uc.y;
        if (temp < 0)
            temp = -temp;
        if (range < temp)
            range = temp;
        (void) walk_path(&uc, &cc, hurtle_jump, (genericptr_t) &range);
        /* hurtle_jump -> hurtle_step results in <u.ux,u.uy> == <cc.x,cc.y>
         * and usually moves the ball if punished, but does not handle all
         * the effects of landing on the final position.
         */
        teleds(cc.x, cc.y, TELEDS_NO_FLAGS);
        nomul(-1);
        gm.multi_reason = "jumping around";
        gn.nomovemsg = "";
        morehungry(rnd(25));
        return ECMD_TIME;
    }
}

boolean
tinnable(struct obj *corpse)
{
    if (corpse->oeaten)
        return 0;
    if (!mons[corpse->corpsenm].cnutrit)
        return 0;
    return 1;
}

static void
use_tinning_kit(struct obj *obj)
{
    struct obj *corpse, *can;
    struct permonst *mptr;

    /* This takes only 1 move.  If this is to be changed to take many
     * moves, we've got to deal with decaying corpses...
     */
    if (obj->spe <= 0) {
        You("seem to be out of tins.");
        return;
    }
    if (!(corpse = floorfood("tin", 2)))
        return;
    if (corpse->oeaten) {
        You("cannot tin %s which is partly eaten.", something);
        return;
    }
    mptr = &mons[corpse->corpsenm];
    if (touch_petrifies(mptr) && !Stone_resistance && !uarmg) {
        char kbuf[BUFSZ];
        const char *corpse_name = an(cxname(corpse));

        if (poly_when_stoned(gy.youmonst.data)) {
            You("tin %s without wearing gloves.", corpse_name);
            kbuf[0] = '\0';
        } else {
            pline("Tinning %s without wearing gloves is a fatal mistake...",
                  corpse_name);
            Sprintf(kbuf, "trying to tin %s without gloves", corpse_name);
        }
        instapetrify(kbuf);
    }
    if (is_rider(mptr)) {
        if (revive_corpse(corpse))
            verbalize("Yes...  But War does not preserve its enemies...");
        else
            pline_The("corpse evades your grasp.");
        return;
    }
    if (mptr->cnutrit == 0) {
        pline("That's too insubstantial to tin.");
        return;
    }
    consume_obj_charge(obj, TRUE);

    if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
        static const char you_buy_it[] = "You tin it, you bought it!";

        can->corpsenm = corpse->corpsenm;
        can->cursed = obj->cursed;
        can->blessed = obj->blessed;
        can->owt = weight(can);
        can->known = 1;
        /* Mark tinned tins. No spinach allowed... */
        set_tin_variety(can, HOMEMADE_TIN);
        if (carried(corpse)) {
            if (corpse->unpaid) {
                struct monst *shkp VOICEONLY = shop_keeper(*in_rooms(
                                                       u.ux, u.uy, SHOPBASE));

                SetVoice(shkp, 0, 80, 0);
                verbalize(you_buy_it);
            }
            useup(corpse);
        } else {
            if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge) {
                struct monst *shkp VOICEONLY = shop_keeper(*in_rooms(corpse->ox,
                                                 corpse->oy, SHOPBASE));
                SetVoice(shkp, 0, 80, 0);
                verbalize(you_buy_it);
            }
            useupf(corpse, 1L);
        }
        (void) hold_another_object(can, "You make, but cannot pick up, %s.",
                                   doname(can), (const char *) 0);
    } else
        impossible("Tinning failed.");
}

void
use_unicorn_horn(struct obj **optr)
{
#define PROP_COUNT 7           /* number of properties we're dealing with */
    int idx, val, val_limit, trouble_count, unfixable_trbl, did_prop;
    int trouble_list[PROP_COUNT];
    struct obj *obj = (optr ? *optr : (struct obj *) 0);

    if (obj && obj->cursed) {
        long lcount = (long) rn1(90, 10);

        switch (rn2(13) / 2) { /* case 6 is half as likely as the others */
        case 0:
            make_sick((Sick & TIMEOUT) ? (Sick & TIMEOUT) / 3L + 1L
                                       : (long) rn1(ACURR(A_CON), 20),
                      xname(obj), TRUE, SICK_NONVOMITABLE);
            break;
        case 1:
            make_blinded((Blinded & TIMEOUT) + lcount, TRUE);
            break;
        case 2:
            if (!Confusion)
                You("suddenly feel %s.",
                    Hallucination ? "trippy" : "confused");
            make_confused((HConfusion & TIMEOUT) + lcount, TRUE);
            break;
        case 3:
            make_stunned((HStun & TIMEOUT) + lcount, TRUE);
            break;
        case 4:
            if (Vomiting)
                vomit();
            else
                make_vomiting(14L, FALSE);
            break;
        case 5:
            (void) make_hallucinated((HHallucination & TIMEOUT) + lcount,
                                     TRUE, 0L);
            break;
        case 6:
            if (Deaf) /* make_deaf() won't give feedback when already deaf */
                pline("%s", Nothing_seems_to_happen);
            make_deaf((HDeaf & TIMEOUT) + lcount, TRUE);
            break;
        }
        return;
    }

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop_trouble(X) trouble_list[trouble_count++] = (X)
#define TimedTrouble(P) (((P) && !((P) & ~TIMEOUT)) ? ((P) & TIMEOUT) : 0L)

    trouble_count = unfixable_trbl = did_prop = 0;

    /* collect property troubles */
    if (TimedTrouble(Sick))
        prop_trouble(SICK);
    if (TimedTrouble(Blinded) > (long) u.ucreamed
        && !(u.uswallow
             && attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        prop_trouble(BLINDED);
    if (TimedTrouble(HHallucination))
        prop_trouble(HALLUC);
    if (TimedTrouble(Vomiting))
        prop_trouble(VOMITING);
    if (TimedTrouble(HConfusion))
        prop_trouble(CONFUSION);
    if (TimedTrouble(HStun))
        prop_trouble(STUNNED);
    if (TimedTrouble(HDeaf))
        prop_trouble(DEAF);

    if (trouble_count == 0) {
        pline1(nothing_happens);
        return;
    } else if (trouble_count > 1)
        shuffle_int_array(trouble_list, trouble_count);

    /*
     *  Chances for number of troubles to be fixed
     *               0      1      2      3      4      5      6      7
     *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%   0.8%
     *  uncursed:  35.4%  35.4%  22.9%   6.3%    0      0      0      0
     */
    val_limit = rn2(d(2, (obj && obj->blessed) ? 4 : 2));
    if (val_limit > trouble_count)
        val_limit = trouble_count;

    /* fix [some of] the troubles */
    for (val = 0; val < val_limit; val++) {
        idx = trouble_list[val];

        switch (idx) {
        case SICK:
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
            did_prop++;
            break;
        case BLINDED:
            make_blinded((long) u.ucreamed, TRUE);
            did_prop++;
            break;
        case HALLUC:
            (void) make_hallucinated(0L, TRUE, 0L);
            did_prop++;
            break;
        case VOMITING:
            make_vomiting(0L, TRUE);
            did_prop++;
            break;
        case CONFUSION:
            make_confused(0L, TRUE);
            did_prop++;
            break;
        case STUNNED:
            make_stunned(0L, TRUE);
            did_prop++;
            break;
        case DEAF:
            make_deaf(0L, TRUE);
            did_prop++;
            break;
        default:
            impossible("use_unicorn_horn: bad trouble? (%d)", idx);
            break;
        }
    }

    if (did_prop)
        gc.context.botl = TRUE;
    else
        pline("%s", Nothing_seems_to_happen);

#undef PROP_COUNT
#undef prop_trouble
#undef TimedTrouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(anything *arg, long timeout)
{
    struct obj *figurine = arg->a_obj;
    struct monst *mtmp;
    coord cc;
    boolean cansee_spot, silent, okay_spot;
    boolean redraw = FALSE;
    boolean suppress_see = FALSE;
    char monnambuf[BUFSZ], carriedby[BUFSZ];

    if (!figurine) {
        impossible("null figurine in fig_transform()");
        return;
    }
    silent = (timeout != gm.moves); /* happened while away */
    okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
    if (figurine->where == OBJ_INVENT || figurine->where == OBJ_MINVENT)
        okay_spot = enexto(&cc, cc.x, cc.y, &mons[figurine->corpsenm]);
    if (!okay_spot || !figurine_location_checks(figurine, &cc, TRUE)) {
        /* reset the timer to try again later */
        (void) start_timer((long) rnd(5000), TIMER_OBJECT, FIG_TRANSFORM,
                           obj_to_any(figurine));
        return;
    }

    cansee_spot = cansee(cc.x, cc.y);
    mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
    if (mtmp) {
        char and_vanish[BUFSZ];
        struct obj *mshelter = gl.level.objects[mtmp->mx][mtmp->my];

        /* [m_monnam() yields accurate mon type, overriding hallucination] */
        Sprintf(monnambuf, "%s", an(m_monnam(mtmp)));
        and_vanish[0] = '\0';
        if ((mtmp->minvis && !See_invisible)
            || (mtmp->data->mlet == S_MIMIC
                && M_AP_TYPE(mtmp) != M_AP_NOTHING))
            suppress_see = TRUE;

        if (mtmp->mundetected) {
            if (hides_under(mtmp->data) && mshelter) {
                Sprintf(and_vanish, " and %s under %s",
                        locomotion(mtmp->data, "crawl"), doname(mshelter));
            } else if (mtmp->data->mlet == S_MIMIC
                       || mtmp->data->mlet == S_EEL) {
                suppress_see = TRUE;
            } else
                Strcpy(and_vanish, " and vanish");
        }

        switch (figurine->where) {
        case OBJ_INVENT:
            if (Blind || suppress_see)
                You_feel("%s %s from your pack!", something,
                         locomotion(mtmp->data, "drop"));
            else
                You_see("%s %s out of your pack%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), and_vanish);
            break;

        case OBJ_FLOOR:
            if (cansee_spot && !silent) {
                if (suppress_see)
                    pline("%s suddenly vanishes!", an(xname(figurine)));
                else
                    You_see("a figurine transform into %s%s!", monnambuf,
                            and_vanish);
                redraw = TRUE; /* update figurine's map location */
            }
            break;

        case OBJ_MINVENT:
            if (cansee_spot && !silent && !suppress_see) {
                struct monst *mon;

                mon = figurine->ocarry;
                /* figurine carrying monster might be invisible */
                if (canseemon(figurine->ocarry)
                    && (!mon->wormno || cansee(mon->mx, mon->my)))
                    Sprintf(carriedby, "%s pack", s_suffix(a_monnam(mon)));
                else if (is_pool(mon->mx, mon->my))
                    Strcpy(carriedby, "empty water");
                else
                    Strcpy(carriedby, "thin air");
                You_see("%s %s out of %s%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), carriedby,
                        and_vanish);
            }
            break;
#if 0
        case OBJ_MIGRATING:
            break;
#endif

        default:
            impossible("figurine came to life where? (%d)",
                       (int) figurine->where);
            break;
        }
    }
    /* free figurine now */
    if (carried(figurine)) {
        useup(figurine);
    } else {
        obj_extract_self(figurine);
        obfree(figurine, (struct obj *) 0);
    }
    if (redraw)
        newsym(cc.x, cc.y);
}

static boolean
figurine_location_checks(struct obj *obj, coord *cc, boolean quietly)
{
    coordxy x, y;

    if (carried(obj) && u.uswallow) {
        if (!quietly)
            You("don't have enough room in here.");
        return FALSE;
    }
    x = cc ? cc->x : u.ux;
    y = cc ? cc->y : u.uy;
    if (!isok(x, y)) {
        if (!quietly)
            You("cannot put the figurine there.");
        return FALSE;
    }
    if (IS_ROCK(levl[x][y].typ)
        && !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x, y))) {
        if (!quietly)
            You("cannot place a figurine in %s!",
                IS_TREE(levl[x][y].typ) ? "a tree" : "solid rock");
        return FALSE;
    }
    if (sobj_at(BOULDER, x, y) && !passes_walls(&mons[obj->corpsenm])
        && !throws_rocks(&mons[obj->corpsenm])) {
        if (!quietly)
            You("cannot fit the figurine on the boulder.");
        return FALSE;
    }
    return TRUE;
}

static int
use_figurine(struct obj **optr)
{
    register struct obj *obj = *optr;
    coordxy x, y;
    coord cc;

    if (u.uswallow) {
        /* can't activate a figurine while swallowed */
        if (!figurine_location_checks(obj, (coord *) 0, FALSE))
            return ECMD_OK;
    }
    if (!getdir((char *) 0)) {
        gc.context.move = gm.multi = 0;
        return ECMD_CANCEL;
    }
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    cc.x = x;
    cc.y = y;
    /* Passing FALSE arg here will result in messages displayed */
    if (!figurine_location_checks(obj, &cc, FALSE))
        return ECMD_TIME;
    You("%s and it %stransforms.",
        (u.dx || u.dy) ? "set the figurine beside you"
                       : (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                          || is_pool(cc.x, cc.y))
                             ? "release the figurine"
                             : (u.dz < 0 ? "toss the figurine into the air"
                                         : "set the figurine on the ground"),
        Blind ? "supposedly " : "");
    (void) make_familiar(obj, cc.x, cc.y, FALSE);
    (void) stop_timer(FIG_TRANSFORM, obj_to_any(obj));
    useup(obj);
    if (Blind)
        map_invisible(cc.x, cc.y);
    *optr = 0;
    return ECMD_TIME;
}

/* getobj callback for object to apply grease to */
static int
grease_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_SUGGEST;

    if (obj->oclass == COIN_CLASS)
        return GETOBJ_EXCLUDE;

    if (inaccessible_equipment(obj, (const char *) 0, FALSE))
        return GETOBJ_EXCLUDE_INACCESS;

    /* Possible extension: don't suggest greasing objects which are already
     * greased. */
    return GETOBJ_SUGGEST;
}

static int
use_grease(struct obj *obj)
{
    struct obj *otmp;

    if (Glib) {
        pline("%s from your %s.", Tobjnam(obj, "slip"),
              fingers_or_gloves(FALSE));
        dropx(obj);
        return ECMD_TIME;
    }

    if (obj->spe > 0) {
        int oldglib;

        if ((obj->cursed || Fumbling) && !rn2(2)) {
            consume_obj_charge(obj, TRUE);

            pline("%s from your %s.", Tobjnam(obj, "slip"),
                  fingers_or_gloves(FALSE));
            dropx(obj);
            return ECMD_TIME;
        }
        otmp = getobj("grease", grease_ok, GETOBJ_PROMPT);
        if (!otmp)
            return ECMD_CANCEL;
        if (inaccessible_equipment(otmp, "grease", FALSE))
            return ECMD_OK;
        consume_obj_charge(obj, TRUE);

        oldglib = (int) (Glib & TIMEOUT);
        if (otmp != &cg.zeroobj) {
            You("cover %s with a thick layer of grease.", yname(otmp));
            otmp->greased = 1;
            if (obj->cursed && !nohands(gy.youmonst.data)) {
                make_glib(oldglib + rn1(6, 10)); /* + 10..15 */
                pline("Some of the grease gets all over your %s.",
                      fingers_or_gloves(TRUE));
            }
        } else {
            make_glib(oldglib + rn1(11, 5)); /* + 5..15 */
            You("coat your %s with grease.", fingers_or_gloves(TRUE));
        }
    } else {
        if (obj->known)
            pline("%s empty.", Tobjnam(obj, "are"));
        else
            pline("%s to be empty.", Tobjnam(obj, "seem"));
    }
    update_inventory();
    return ECMD_TIME;
}

/* getobj callback for object to rub on a known touchstone */
static int
touchstone_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    /* Gold being suggested as a rub target is questionable - it fits the
     * real-world historic use of touchstones, but doesn't do anything
     * significant in the game. */
    if (obj->oclass == COIN_CLASS)
        return GETOBJ_SUGGEST;

    /* don't suggest identified gems */
    if (obj->oclass == GEM_CLASS
        && !(obj->dknown && objects[obj->otyp].oc_name_known))
        return GETOBJ_SUGGEST;

    return GETOBJ_DOWNPLAY;
}


/* touchstones - by Ken Arnold */
static int
use_stone(struct obj *tstone)
{
    static const char scritch[] = "\"scritch, scritch\"";
    struct obj *obj;
    boolean do_scratch;
    const char *streak_color;
    char stonebuf[QBUFSZ];
    int oclass;
    boolean known;

    /* in case it was acquired while blinded */
    if (!Blind)
        tstone->dknown = 1;
    known = (tstone->otyp == TOUCHSTONE && tstone->dknown
              && objects[TOUCHSTONE].oc_name_known);
    Sprintf(stonebuf, "rub on the stone%s", plur(tstone->quan));
    /* when the touchstone is fully known, don't bother listing extra
       junk as likely candidates for rubbing */
    if ((obj = getobj(stonebuf, known ? touchstone_ok : any_obj_ok,
                      GETOBJ_PROMPT)) == 0)
        return ECMD_CANCEL;

    if (obj == tstone && obj->quan == 1L) {
        You_cant("rub %s on itself.", the(xname(obj)));
        return ECMD_OK;
    }

    if (tstone->otyp == TOUCHSTONE && tstone->cursed
        && obj->oclass == GEM_CLASS && !is_graystone(obj)
        && !obj_resists(obj, 80, 100)) {
        if (Blind)
            You_feel("something shatter.");
        else if (Hallucination)
            pline("Oh, wow, look at the pretty shards.");
        else
            pline("A sharp crack shatters %s%s.",
                  (obj->quan > 1L) ? "one of " : "", the(xname(obj)));
        useup(obj);
        return ECMD_TIME;
    }

    if (Blind) {
        pline(scritch);
        return ECMD_TIME;
    } else if (Hallucination) {
        pline("Oh wow, man: Fractals!");
        return ECMD_TIME;
    }

    do_scratch = FALSE;
    streak_color = 0;

    oclass = obj->oclass;
    /* prevent non-gemstone rings from being treated like gems */
    if (oclass == RING_CLASS
        && objects[obj->otyp].oc_material != GEMSTONE
        && objects[obj->otyp].oc_material != MINERAL)
        oclass = RANDOM_CLASS; /* something that's neither gem nor ring */

    switch (oclass) {
    case GEM_CLASS: /* these have class-specific handling below */
    case RING_CLASS:
        if (tstone->otyp != TOUCHSTONE) {
            do_scratch = TRUE;
        } else if (obj->oclass == GEM_CLASS
                   && (tstone->blessed
                       || (!tstone->cursed && (Role_if(PM_ARCHEOLOGIST)
                                               || Race_if(PM_GNOME))))) {
            makeknown(TOUCHSTONE);
            makeknown(obj->otyp);
            prinv((char *) 0, obj, 0L);
            return ECMD_TIME;
        } else {
            /* either a ring or the touchstone was not effective */
            if (objects[obj->otyp].oc_material == GLASS) {
                do_scratch = TRUE;
                break;
            }
        }
        streak_color = c_obj_colors[objects[obj->otyp].oc_color];
        break; /* gem or ring */

    default:
        switch (objects[obj->otyp].oc_material) {
        case CLOTH:
            pline("%s a little more polished now.", Tobjnam(tstone, "look"));
            return ECMD_TIME;
        case LIQUID:
            if (!obj->known) /* note: not "whetstone" */
                You("must think this is a wetstone, do you?");
            else
                pline("%s a little wetter now.", Tobjnam(tstone, "are"));
            return ECMD_TIME;
        case WAX:
            streak_color = "waxy";
            break; /* okay even if not touchstone */
        case WOOD:
            streak_color = "wooden";
            break; /* okay even if not touchstone */
        case GOLD:
            do_scratch = TRUE; /* scratching and streaks */
            streak_color = "golden";
            break;
        case SILVER:
            do_scratch = TRUE; /* scratching and streaks */
            streak_color = "silvery";
            break;
        default:
            /* Objects passing the is_flimsy() test will not
               scratch a stone.  They will leave streaks on
               non-touchstones and touchstones alike. */
            if (is_flimsy(obj))
                streak_color = c_obj_colors[objects[obj->otyp].oc_color];
            else
                do_scratch = (tstone->otyp != TOUCHSTONE);
            break;
        }
        break; /* default oclass */
    }

    Sprintf(stonebuf, "stone%s", plur(tstone->quan));
    if (do_scratch)
        You("make %s%sscratch marks on the %s.",
            streak_color ? streak_color : (const char *) "",
            streak_color ? " " : "", stonebuf);
    else if (streak_color)
        You_see("%s streaks on the %s.", streak_color, stonebuf);
    else
        pline(scritch);
    return ECMD_TIME;
}

void
reset_trapset(void)
{
    gt.trapinfo.tobj = 0;
    gt.trapinfo.force_bungle = 0;
}

/* Place a landmine/bear trap.  Helge Hafting */
static void
use_trap(struct obj *otmp)
{
    int ttyp, tmp;
    const char *what = (char *) 0;
    char buf[BUFSZ];
    int levtyp = levl[u.ux][u.uy].typ;
    const char *occutext = "setting the trap";

    if (nohands(gy.youmonst.data))
        what = "without hands";
    else if (Stunned)
        what = "while stunned";
    else if (u.uswallow)
        what = digests(u.ustuck->data) ? "while swallowed" : "while engulfed";
    else if (Underwater)
        what = "underwater";
    else if (Levitation)
        what = "while levitating";
    else if (is_pool(u.ux, u.uy))
        what = "in water";
    else if (is_lava(u.ux, u.uy))
        what = "in lava";
    else if (On_stairs(u.ux, u.uy)) {
        stairway *stway = stairway_at(u.ux, u.uy);
        what = stway->isladder ? "on the ladder" : "on the stairs";
    } else if (IS_FURNITURE(levtyp) || IS_ROCK(levtyp)
             || closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
        what = "here";
    else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
        what = (levtyp == AIR)
                   ? "in midair"
                   : (levtyp == CLOUD)
                         ? "in a cloud"
                         : "in this place"; /* Air/Water Plane catch-all */
    if (what) {
        You_cant("set a trap %s!", what);
        reset_trapset();
        return;
    }
    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    if (otmp == gt.trapinfo.tobj && u_at(gt.trapinfo.tx, gt.trapinfo.ty)) {
        You("resume setting %s%s.", shk_your(buf, otmp),
            trapname(ttyp, FALSE));
        set_occupation(set_trap, occutext, 0);
        return;
    }
    gt.trapinfo.tobj = otmp;
    gt.trapinfo.tx = u.ux, gt.trapinfo.ty = u.uy;
    tmp = ACURR(A_DEX);
    gt.trapinfo.time_needed =
        (tmp > 17) ? 2 : (tmp > 12) ? 3 : (tmp > 7) ? 4 : 5;
    if (Blind)
        gt.trapinfo.time_needed *= 2;
    tmp = ACURR(A_STR);
    if (ttyp == BEAR_TRAP && tmp < 18)
        gt.trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
    /*[fumbling and/or confusion and/or cursed object check(s)
       should be incorporated here instead of in set_trap]*/
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
        boolean chance;

        if (Fumbling || otmp->cursed)
            chance = (rnl(10) > 3);
        else
            chance = (rnl(10) > 5);
        You("aren't very skilled at reaching from %s.", mon_nam(u.usteed));
        Sprintf(buf, "Continue your attempt to set %s?",
                the(trapname(ttyp, FALSE)));
        if (y_n(buf) == 'y') {
            if (chance) {
                switch (ttyp) {
                case LANDMINE: /* set it off */
                    gt.trapinfo.time_needed = 0;
                    gt.trapinfo.force_bungle = TRUE;
                    break;
                case BEAR_TRAP: /* drop it without arming it */
                    reset_trapset();
                    You("drop %s!", the(trapname(ttyp, FALSE)));
                    dropx(otmp);
                    return;
                }
            }
        } else {
            reset_trapset();
            return;
        }
    }
    You("begin setting %s%s.", shk_your(buf, otmp), trapname(ttyp, FALSE));
    set_occupation(set_trap, occutext, 0);
    return;
}

static int
set_trap(void)
{
    struct obj *otmp = gt.trapinfo.tobj;
    struct trap *ttmp;
    int ttyp;

    if (!otmp || !carried(otmp) || u.ux != gt.trapinfo.tx
        || u.uy != gt.trapinfo.ty) {
        /* ?? */
        reset_trapset();
        return 0;
    }

    if (--gt.trapinfo.time_needed > 0)
        return 1; /* still busy */

    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    ttmp = maketrap(u.ux, u.uy, ttyp);
    if (ttmp) {
        ttmp->madeby_u = 1;
        feeltrap(ttmp);
        if (*in_rooms(u.ux, u.uy, SHOPBASE)) {
            add_damage(u.ux, u.uy, 0L); /* schedule removal */
        }
        if (!gt.trapinfo.force_bungle)
            You("finish arming %s.", the(trapname(ttyp, FALSE)));
        if (((otmp->cursed || Fumbling) && (rnl(10) > 5))
            || gt.trapinfo.force_bungle)
            dotrap(ttmp,
                   (unsigned) (gt.trapinfo.force_bungle ? FORCEBUNGLE : 0));
    } else {
        /* this shouldn't happen */
        Your("trap setting attempt fails.");
    }
    useup(otmp);
    reset_trapset();
    return 0;
}

int
use_whip(struct obj *obj)
{
    char buf[BUFSZ];
    struct monst *mtmp;
    struct obj *otmp;
    int rx, ry, proficient, res = ECMD_OK;
    const char *msg_slipsfree = "The bullwhip slips free.";
    const char *msg_snap = "Snap!";

    if (obj != uwep) {
        if (wield_tool(obj, "lash")) {
            cmdq_add_ec(CQ_CANNED, doapply);
            cmdq_add_key(CQ_CANNED, obj->invlet);
            return ECMD_TIME;
        }
        return ECMD_OK;
    }
    if (!getdir((char *) 0))
        return (res|ECMD_CANCEL);

    if (u.uswallow) {
        mtmp = u.ustuck;
        rx = mtmp->mx;
        ry = mtmp->my;
    } else {
        confdir(FALSE);
        rx = u.ux + u.dx;
        ry = u.uy + u.dy;
        if (!isok(rx, ry)) {
            You("miss.");
            return res;
        }
        mtmp = m_at(rx, ry);
    }

    /* fake some proficiency checks */
    proficient = 0;
    if (Role_if(PM_ARCHEOLOGIST))
        ++proficient;
    if (ACURR(A_DEX) < 6)
        proficient--;
    else if (ACURR(A_DEX) >= 14)
        proficient += (ACURR(A_DEX) - 14);
    if (Fumbling)
        --proficient;
    if (proficient > 3)
        proficient = 3;
    if (proficient < 0)
        proficient = 0;

    if (u.uswallow) {
        There("is not enough room to flick your bullwhip.");

    } else if (Underwater) {
        There("is too much resistance to flick your bullwhip.");

    } else if (u.dz < 0) {
        You("flick a bug off of the %s.", ceiling(u.ux, u.uy));

    } else if (!u.dz && (IS_WATERWALL(levl[rx][ry].typ)
                         || levl[rx][ry].typ == LAVAWALL)) {
        You("cause a small splash.");
        if (levl[rx][ry].typ == LAVAWALL)
            (void) fire_damage(uwep, FALSE, rx, ry);
        return ECMD_TIME;
    } else if ((!u.dx && !u.dy) || (u.dz > 0)) {
        int dam;

        /* Sometimes you hit your steed by mistake */
        if (u.usteed && !rn2(proficient + 2)) {
            You("whip %s!", mon_nam(u.usteed));
            kick_steed();
            return ECMD_TIME;
        }
        if (is_pool_or_lava(u.ux, u.uy)
            || IS_WATERWALL(levl[rx][ry].typ)
            || levl[rx][ry].typ == LAVAWALL) {
            You("cause a small splash.");
            if (is_lava(u.ux, u.uy))
                (void) fire_damage(uwep, FALSE, u.ux, u.uy);
            return ECMD_TIME;
        }
        if (Levitation || u.usteed || Flying) {
            /* Have a shot at snaring something on the floor.  A flyer
               can reach the floor so could just pick an item up, but
               allow snagging by whip too. */
            otmp = gl.level.objects[u.ux][u.uy];
            if (otmp && otmp->otyp == CORPSE && otmp->corpsenm == PM_HORSE) {
                pline("Why beat a dead horse?");
                return ECMD_TIME;
            }
            if (otmp && proficient) {
                You("wrap your bullwhip around %s on the %s.",
                    an(singular(otmp, xname)), surface(u.ux, u.uy));
                if (rnl(6) || pickup_object(otmp, 1L, TRUE) < 1)
                    pline1(msg_slipsfree);
                return ECMD_TIME;
            }
        }
        dam = rnd(2) + dbon() + obj->spe;
        if (dam <= 0)
            dam = 1;
        You("hit your %s with your bullwhip.", body_part(FOOT));
        Sprintf(buf, "killed %sself with %s bullwhip", uhim(), uhis());
        losehp(Maybe_Half_Phys(dam), buf, NO_KILLER_PREFIX);
        return ECMD_TIME;

    } else if ((Fumbling || Glib) && !rn2(5)) {
        pline_The("bullwhip slips out of your %s.", body_part(HAND));
        dropx(obj);

    } else if (u.utrap && u.utraptype == TT_PIT) {
        /*
         * Assumptions:
         *
         * if you're in a pit
         *    - you are attempting to get out of the pit
         *    - if there is no suitable boulder or furniture to target,
         *      target a big monster for that, or if a small or medium
         *      monster is present, attack it
         *      [if both boulder and furniture are present, target the
         *      former because it is on top of the latter]
         * else if you are applying it towards a monster
         *    - if monster is concealed, reveal it and proceed;
         *    - if it was not concealed and is wielding a weapon, attempt
         *      to disarm it;
         *    - otherwise attack it.
         *
         * if you're confused (and thus off the mark)
         *    - you only end up hitting.
         */
        const char *wrapped_what = sobj_at(BOULDER, rx, ry) ? "a boulder"
                                   : IS_FURNITURE(levl[rx][ry].typ)
                                     ? something : (char *) 0;

        if (mtmp) {
            /* if a big monster is known to be present, target it in
               preference to boulder or furniture; if any small or medium
               monster is present, or an unseen big one, use the boulder
               or furniture if available, otherwise attack */
            if (bigmonst(mtmp->data) && canspotmon(mtmp))
                wrapped_what = strcpy(buf, mon_nam(mtmp));

            if (!wrapped_what)
                goto whipattack;
        }
        if (wrapped_what) {
            coord cc;

            cc.x = rx;
            cc.y = ry;
            You("wrap your bullwhip around %s.", wrapped_what);
            if (proficient && rn2(proficient + 2)) {
                if (!mtmp || enexto(&cc, rx, ry, gy.youmonst.data)) {
                    You("yank yourself out of the pit!");
                    reset_utrap(TRUE); /* [was after teleds(); do this before
                                        * in case it has no alternative other
                                        * than to put hero in another trap] */
                    teleds(cc.x, cc.y, TELEDS_ALLOW_DRAG);
                    gv.vision_full_recalc = 1;
                }
            } else {
                pline1(msg_slipsfree);
            }
            if (mtmp)
                wakeup(mtmp, TRUE);
        } else
            pline1(msg_snap);

    } else if (mtmp) {
 whipattack:
        otmp = 0; /* if monster is unseen, can't attempt to disarm it */
        if (!canspotmon(mtmp)) {
            boolean spotitnow;

            mtmp->mundetected = 0; /* bring non-mimic hider out of hiding */
            /* check visibility again after mundetected=0 in case being
               brought out of hiding has exposed it (might not if hero is
               blind or formerly hidden monster is also invisible) */
            spotitnow = canspotmon(mtmp);
            if (spotitnow || !glyph_is_invisible(levl[rx][ry].glyph)) {
                pline("%s is there that you %s.",
                      !spotitnow ? "A monster" : Amonnam(mtmp),
                      !Blind ? "couldn't see" : "hadn't noticed");
                if (!spotitnow)
                    map_invisible(rx, ry);
                else
                    newsym(rx, ry);
            }
        } else {
            /* monster is known so if it is wielding something, try to
               disarm it rather than make a direct attack */
            otmp = MON_WEP(mtmp);
        }

        if (otmp) {
            char onambuf[BUFSZ];
            const char *mon_hand;
            boolean gotit = proficient && (!Fumbling || !rn2(10));

            Strcpy(onambuf, cxname(otmp));
            if (gotit) {
                mon_hand = mbodypart(mtmp, HAND);
                if (bimanual(otmp))
                    mon_hand = makeplural(mon_hand);
            } else
                mon_hand = 0; /* lint suppression */

            You("wrap your bullwhip around %s.", yname(otmp));
            if (gotit && mwelded(otmp)) {
                pline("%s welded to %s %s%c",
                      (otmp->quan == 1L) ? "It is" : "They are", mhis(mtmp),
                      mon_hand, !otmp->bknown ? '!' : '.');
                set_bknown(otmp, 1);
                gotit = FALSE; /* can't pull it free */
            }
            if (gotit) {
                obj_extract_self(otmp);
                possibly_unwield(mtmp, FALSE);
                setmnotwielded(mtmp, otmp);

                switch (rn2(proficient + 1)) {
                case 2:
                    /* to floor near you */
                    You("yank %s to the %s!", yname(otmp),
                        surface(u.ux, u.uy));
                    place_object(otmp, u.ux, u.uy);
                    stackobj(otmp);
                    break;
                case 3:
#if 0
                    /* right to you */
                    if (!rn2(25)) {
                        /* proficient with whip, but maybe not
                           so proficient at catching weapons */
                        int hitu, hitvalu;

                        hitvalu = 8 + otmp->spe;
                        hitu = thitu(hitvalu, dmgval(otmp, &gy.youmonst),
                                     &otmp, (char *)0);
                        if (hitu) {
                            pline_The("%s hits you as you try to snatch it!",
                                      the(onambuf));
                        }
                        place_object(otmp, u.ux, u.uy);
                        stackobj(otmp);
                        break;
                    }
#endif /* 0 */
                    /* right into your inventory */
                    You("snatch %s!", yname(otmp));
                    if (otmp->otyp == CORPSE
                        && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg
                        && !Stone_resistance
                        && !(poly_when_stoned(gy.youmonst.data)
                             && polymon(PM_STONE_GOLEM))) {
                        char kbuf[BUFSZ];

                        Strcpy(kbuf, (otmp->quan == 1L) ? an(onambuf)
                                                        : onambuf);
                        pline("Snatching %s is a fatal mistake.", kbuf);
                        /* corpse probably has a rot timer but is now
                           OBJ_FREE; end of game cleanup will panic if
                           it isn't part of current level; plus it would
                           be missing from bones, so put it on the floor */
                        place_object(otmp, u.ux, u.uy); /* but don't stack */

                        instapetrify(kbuf);
                        /* life-saved; free the corpse again */
                        obj_extract_self(otmp);
                    }
                    (void) hold_another_object(otmp, "You drop %s!",
                                               doname(otmp), (const char *) 0);
                    break;
                default:
                    /* to floor beneath mon */
                    You("yank %s from %s %s!", the(onambuf),
                        s_suffix(mon_nam(mtmp)), mon_hand);
                    obj_no_longer_held(otmp);
                    place_object(otmp, mtmp->mx, mtmp->my);
                    stackobj(otmp);
                    break;
                }
            } else {
                pline1(msg_slipsfree);
            }
        } else { /* mtmp isn't wielding a weapon; attack it */
            boolean do_snap = TRUE;

            if (M_AP_TYPE(mtmp) && !Protection_from_shape_changers
                && !sensemon(mtmp)) {
                stumble_onto_mimic(mtmp);
                do_snap = FALSE;
            } else {
                You("flick your bullwhip towards %s.", mon_nam(mtmp));
            }
            if (proficient && force_attack(mtmp, FALSE))
                return ECMD_TIME;
            if (do_snap)
                pline1(msg_snap);
        }
        /* regardless of mtmp's weapon or hero's proficiency */
        wakeup(mtmp, TRUE);

    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* it must be air -- water checked above */
        You("snap your whip through thin air.");

    } else {
        pline1(msg_snap);
    }
    return ECMD_TIME;
}

static const char
    not_enough_room[] = "There's not enough room here to use that.",
    where_to_hit[] = "Where do you want to hit?",
    cant_see_spot[] = "won't hit anything if you can't see that spot.",
    cant_reach[] = "can't reach that spot from here.";

#define glyph_is_poleable(G) \
    (glyph_is_monster(G) || glyph_is_invisible(G) || glyph_is_statue(G))

/* find pos of monster in range, if only one monster */
static boolean
find_poleable_mon(coord *pos, int min_range, int max_range)
{
    struct monst *mtmp;
    coord mpos = { 0, 0 }; /* no candidate location yet */
    boolean impaired;
    coordxy x, y, lo_x, hi_x, lo_y, hi_y, rt;
    int glyph;

    impaired = (Confusion || Stunned || Hallucination);
    rt = isqrt(max_range);
    lo_x = max(u.ux - rt, 1), hi_x = min(u.ux + rt, COLNO - 1);
    lo_y = max(u.uy - rt, 0), hi_y = min(u.uy + rt, ROWNO - 1);
    for (x = lo_x; x <= hi_x; ++x) {
        for (y = lo_y; y <= hi_y; ++y) {
            if (distu(x, y) < min_range || distu(x, y) > max_range
                || !isok(x, y) || !cansee(x, y))
                continue;
            glyph = glyph_at(x, y);
            if (!impaired
                && glyph_is_monster(glyph)
                && (mtmp = m_at(x, y)) != 0
                && (mtmp->mtame || (mtmp->mpeaceful && flags.confirm)))
                continue;
            if (glyph_is_poleable(glyph)
                && (!glyph_is_statue(glyph) || impaired)) {
                if (mpos.x)
                    return FALSE; /* more than one candidate location */
                mpos.x = x, mpos.y = y;
            }
        }
    }
    if (!mpos.x)
        return FALSE; /* no candidate location */
    *pos = mpos;
    return TRUE;
}

static boolean
get_valid_polearm_position(coordxy x, coordxy y)
{
    int glyph;

    glyph = glyph_at(x, y);

    return (isok(x, y) && distu(x, y) >= gp.polearm_range_min
            && distu(x, y) <= gp.polearm_range_max
            && (cansee(x, y) || (couldsee(x, y)
                                 && glyph_is_poleable(glyph))));
}

static void
display_polearm_positions(int state)
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        coordxy x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (get_valid_polearm_position(x, y)) {
                    tmp_at(x, y);
                }
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

/* Distance attacks by pole-weapons */
int
use_pole(struct obj *obj, boolean autohit)
{
    const char thump[] = "Thump!  Your blow bounces harmlessly off the %s.";
    int res = ECMD_OK, typ, max_range, min_range, glyph;
    coord cc;
    struct monst *mtmp;
    struct monst *hitm = gc.context.polearm.hitmon;

    /* Are you allowed to use the pole? */
    if (u.uswallow) {
        pline(not_enough_room);
        return ECMD_OK;
    }
    if (obj != uwep) {
        if (wield_tool(obj, "swing")) {
            cmdq_add_ec(CQ_CANNED, doapply);
            cmdq_add_key(CQ_CANNED, obj->invlet);
            return ECMD_TIME;
        }
        return ECMD_OK;
    }
    /* assert(obj == uwep); */

    /*
     * Calculate allowable range (pole's reach is always 2 steps):
     *  unskilled and basic: orthogonal direction, 4..4;
     *  skilled: as basic, plus knight's jump position, 4..5;
     *  expert: as skilled, plus diagonal, 4..8.
     *      ...9...
     *      .85458.
     *      .52125.
     *      9410149
     *      .52125.
     *      .85458.
     *      ...9...
     *  (Note: no roles in nethack can become expert or better
     *  for polearm skill; Yeoman in slash'em can become expert.)
     */
    min_range = 4;
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8; /* (P_SKILL(typ) >= P_EXPERT) */

    gp.polearm_range_min = min_range;
    gp.polearm_range_max = max_range;

    /* Prompt for a location */
    if (!autohit)
        pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    if (!find_poleable_mon(&cc, min_range, max_range) && hitm
        && !DEADMONSTER(hitm) && sensemon(hitm)
        && mdistu(hitm) <= max_range && mdistu(hitm) >= min_range) {
        cc.x = hitm->mx;
        cc.y = hitm->my;
    }
    if (!autohit) {
        getpos_sethilite(display_polearm_positions,
                         get_valid_polearm_position);
        if (getpos(&cc, TRUE, "the spot to hit") < 0)
            /* ESC; uses turn iff polearm became wielded */
            return (res | ECMD_CANCEL);
    }

    glyph = glyph_at(cc.x, cc.y);
    if (distu(cc.x, cc.y) > max_range) {
        pline("Too far!");
        return ECMD_FAIL;
    } else if (distu(cc.x, cc.y) < min_range) {
        if (autohit && u_at(cc.x, cc.y))
            pline("Don't know what to hit.");
        else
            pline("Too close!");
        return ECMD_FAIL;
    } else if (!cansee(cc.x, cc.y) && !glyph_is_poleable(glyph)) {
        You(cant_see_spot);
        return ECMD_FAIL;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return ECMD_FAIL;
    }

    gc.context.polearm.hitmon = (struct monst *) 0;
    /* Attack the monster there */
    gb.bhitpos = cc;
    if ((mtmp = m_at(gb.bhitpos.x, gb.bhitpos.y)) != (struct monst *) 0) {
        if (attack_checks(mtmp, uwep)) /* can attack proceed? */
            /* no, abort the attack attempt; result depends on
               res: 1 => polearm became wielded, 0 => already wielded;
               gc.context.move: 1 => discovered hidden monster at target spot,
               0 => answered 'n' to "Really attack?" prompt */
            return res | (gc.context.move ? ECMD_TIME : ECMD_OK);
        if (overexertion())
            return ECMD_TIME; /* burn nutrition; maybe pass out */
        gc.context.polearm.hitmon = mtmp;
        check_caitiff(mtmp);
        gn.notonhead = (gb.bhitpos.x != mtmp->mx || gb.bhitpos.y != mtmp->my);
        (void) thitmonst(mtmp, uwep);
    } else if (glyph_is_statue(glyph) /* might be hallucinatory */
               && sobj_at(STATUE, gb.bhitpos.x, gb.bhitpos.y)) {
        struct trap *t = t_at(gb.bhitpos.x, gb.bhitpos.y);

        if (t && t->ttyp == STATUE_TRAP
            && activate_statue_trap(t, t->tx, t->ty, FALSE)) {
            ; /* feedback has been give by animate_statue() */
        } else {
            /* Since statues look like monsters now, we say something
               different from "you miss" or "there's nobody there".
               Note:  we only do this when a statue is displayed here,
               because the player is probably attempting to attack it;
               other statues obscured by anything are just ignored. */
            pline(thump, "statue");
            wake_nearto(gb.bhitpos.x, gb.bhitpos.y, 25);
        }
    } else {
        /* no monster here and no statue seen or remembered here */
        (void) unmap_invisible(gb.bhitpos.x, gb.bhitpos.y);

        if (glyph_to_obj(glyph) == BOULDER
            && sobj_at(BOULDER, gb.bhitpos.x, gb.bhitpos.y)) {
            pline(thump, "boulder");
            wake_nearto(gb.bhitpos.x, gb.bhitpos.y, 25);
        } else if (!accessible(gb.bhitpos.x, gb.bhitpos.y)
                   || IS_FURNITURE(levl[gb.bhitpos.x][gb.bhitpos.y].typ)) {
            /* similar to 'F'orcefight with a melee weapon; we know that
               the spot can be seen or we wouldn't have gotten this far */
            You("uselessly attack %s.",
                (levl[gb.bhitpos.x][gb.bhitpos.y].typ == STONE
                 || levl[gb.bhitpos.x][gb.bhitpos.y].typ == SCORR)
                ? "stone"
                : glyph_is_cmap(glyph)
                  ? the(defsyms[glyph_to_cmap(glyph)].explanation)
                  : (const char *) "an unknown obstacle");
        } else {
            You("miss; there is no one there to hit.");
        }
    }
    u_wipe_engr(2); /* same as for melee or throwing */
    return ECMD_TIME;
}

static int
use_cream_pie(struct obj *obj)
{
    boolean wasblind = Blind;
    boolean wascreamed = u.ucreamed;
    boolean several = FALSE;

    if (obj->quan > 1L) {
        several = TRUE;
        obj = splitobj(obj, 1L);
    }
    if (Hallucination)
        You("give yourself a facial.");
    else
        You("immerse your %s in %s%s.", body_part(FACE),
              several ? "one of " : "",
              several ? makeplural(the(xname(obj))) : the(xname(obj)));
    if (can_blnd((struct monst *) 0, &gy.youmonst, AT_WEAP, obj)) {
        int blindinc = rnd(25);
        u.ucreamed += blindinc;
        make_blinded(Blinded + (long) blindinc, FALSE);
        if (!Blind || (Blind && wasblind))
            pline("There's %ssticky goop all over your %s.",
                  wascreamed ? "more " : "", body_part(FACE));
        else /* Blind  && !wasblind */
            You_cant("see through all the sticky goop on your %s.",
                     body_part(FACE));
    }

    setnotworn(obj);
    /* useup() is appropriate, but we want costly_alteration()'s message */
    costly_alteration(obj, COST_SPLAT);
    obj_extract_self(obj);
    delobj(obj);
    return ECMD_OK;
}

/* getobj callback for object to rub royal jelly on */
static int
jelly_ok(struct obj *obj)
{
    if (obj && obj->otyp == EGG)
        return GETOBJ_SUGGEST;

    return GETOBJ_EXCLUDE;
}

static int
use_royal_jelly(struct obj *obj)
{
    int oldcorpsenm;
    unsigned was_timed;
    struct obj *eobj;

    if (obj->quan > 1L)
        obj = splitobj(obj, 1L);
    /* remove from inventory so that it won't be offered as a choice
       to rub on itself */
    freeinv(obj);

    /* right now you can rub one royal jelly on an entire stack of eggs */
    eobj = getobj("rub the royal jelly on", jelly_ok, GETOBJ_PROMPT);
    if (!eobj) {
        addinv(obj); /* put the unused lump back; if it came from
                      * a split, it should merge back */
        return ECMD_CANCEL;
    }

    You("smear royal jelly all over %s.", yname(eobj));
    if (eobj->otyp != EGG) {
        pline1(nothing_happens);
        goto useup_jelly;
    }

    oldcorpsenm = eobj->corpsenm;
    if (eobj->corpsenm == PM_KILLER_BEE)
        eobj->corpsenm = PM_QUEEN_BEE;

    if (obj->cursed) {
        if (eobj->timed || eobj->corpsenm != oldcorpsenm)
            pline("The %s %s feebly.", xname(eobj), otense(eobj, "quiver"));
        else
            pline("%s", Nothing_seems_to_happen);
        kill_egg(eobj);
        goto useup_jelly;
    }

    was_timed = eobj->timed;
    if (eobj->corpsenm != NON_PM) {
        if (!eobj->timed)
            attach_egg_hatch_timeout(eobj, 0L);
        /* blessed royal jelly will make the hatched creature think
           you're the parent - but has no effect if you laid the egg */
        if (obj->blessed && !eobj->spe)
            eobj->spe = 2;
    }

    if ((eobj->timed && !was_timed) || eobj->spe == 2
        || eobj->corpsenm != oldcorpsenm)
        pline("The %s %s briefly.", xname(eobj), otense(eobj, "quiver"));
    else
        pline("%s", Nothing_seems_to_happen);

 useup_jelly:
    /* not useup() because we've already done freeinv() */
    setnotworn(obj);
    obfree(obj, (struct obj *) 0);
    return ECMD_TIME;
}

static int
grapple_range(void)
{
    int typ = uwep_skill_type();
    int max_range = 4;

    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8;
    return max_range;
}

static boolean
can_grapple_location(coordxy x, coordxy y)
{
    return (isok(x, y) && cansee(x, y) && distu(x, y) <= grapple_range());
}

static int
use_grapple(struct obj *obj)
{
    int res = ECMD_OK, typ, tohit;
    boolean save_confirm;
    coord cc;
    struct monst *mtmp;
    struct obj *otmp;

    /* Are you allowed to use the hook? */
    if (u.uswallow) {
        pline(not_enough_room);
        return ECMD_OK;
    }
    if (obj != uwep) {
        if (wield_tool(obj, "cast")) {
            cmdq_add_ec(CQ_CANNED, doapply);
            cmdq_add_key(CQ_CANNED, obj->invlet);
            return ECMD_TIME;
        }
        return ECMD_OK;
    }
    /* assert(obj == uwep); */

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    getpos_sethilite(NULL, can_grapple_location);
    if (getpos(&cc, TRUE, "the spot to hit") < 0)
        /* ESC; uses turn iff grapnel became wielded */
        return (res | ECMD_CANCEL);

    /* Calculate range; unlike use_pole(), there's no minimum for range */
    typ = uwep_skill_type();
    if (distu(cc.x, cc.y) > grapple_range()) {
        pline("Too far!");
        return res;
    } else if (!cansee(cc.x, cc.y)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    /* What do you want to hit? */
    tohit = rn2(5);
    if (typ != P_NONE && P_SKILL(typ) >= P_SKILLED) {
        winid tmpwin = create_nhwindow(NHW_MENU);
        anything any;
        char buf[BUFSZ];
        menu_item *selected;
        int clr = 0;

        any = cg.zeroany; /* set all bits to zero */
        any.a_int = 1; /* use index+1 (cant use 0) as identifier */
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any.a_int++;
        Sprintf(buf, "an object on the %s", surface(cc.x, cc.y));
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, buf, MENU_ITEMFLAGS_NONE);
        any.a_int++;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, "a monster", MENU_ITEMFLAGS_NONE);
        any.a_int++;
        Sprintf(buf, "the %s", surface(cc.x, cc.y));
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
                 buf, MENU_ITEMFLAGS_NONE);
        end_menu(tmpwin, "Aim for what?");
        tohit = rn2(4);
        if (select_menu(tmpwin, PICK_ONE, &selected) > 0
            && rn2(P_SKILL(typ) > P_SKILLED ? 20 : 2))
            tohit = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        destroy_nhwindow(tmpwin);
    }

    /* possibly scuff engraving at your feet;
       any engraving at the target location is unaffected */
    if (tohit == 2 || !rn2(2))
        u_wipe_engr(rnd(2));

    /* What did you hit? */
    switch (tohit) {
    case 0: /* Trap */
        /* FIXME -- untrap needs to deal with non-adjacent traps */
        break;
    case 1: /* Object */
        if ((otmp = gl.level.objects[cc.x][cc.y]) != 0) {
            You("snag an object from the %s!", surface(cc.x, cc.y));
            (void) pickup_object(otmp, 1L, FALSE);
            /* If pickup fails, leave it alone */
            newsym(cc.x, cc.y);
            return ECMD_TIME;
        }
        break;
    case 2: /* Monster */
        gb.bhitpos = cc;
        if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *) 0)
            break;
        gn.notonhead = (gb.bhitpos.x != mtmp->mx || gb.bhitpos.y != mtmp->my);
        save_confirm = flags.confirm;
        if (verysmall(mtmp->data) && !rn2(4)
            && enexto(&cc, u.ux, u.uy, (struct permonst *) 0)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp); /* despite fact there's no damage */
            You("pull in %s!", mon_nam(mtmp));
            mtmp->mundetected = 0;
            rloc_to(mtmp, cc.x, cc.y);
            return ECMD_TIME;
        } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data))
                   || rn2(4)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp);
            (void) thitmonst(mtmp, uwep);
            return ECMD_TIME;
        }
    /*FALLTHRU*/
    case 3: /* Surface */
        if (IS_AIR(levl[cc.x][cc.y].typ) || is_pool(cc.x, cc.y))
            pline_The("hook slices through the %s.", surface(cc.x, cc.y));
        else {
            You("are yanked toward the %s!", surface(cc.x, cc.y));
            hurtle(sgn(cc.x - u.ux), sgn(cc.y - u.uy), 1, FALSE);
            spoteffects(TRUE);
        }
        return ECMD_TIME;
    default: /* Yourself (oops!) */
        if (P_SKILL(typ) <= P_BASIC) {
            You("hook yourself!");
            losehp(Maybe_Half_Phys(rn1(10, 10)), "a grappling hook",
                   KILLED_BY);
            return ECMD_TIME;
        }
        break;
    }
    pline1(nothing_happens);
    return ECMD_TIME;
}

#define BY_OBJECT ((struct monst *) 0)

/* return 1 if the wand is broken, hence some time elapsed */
static int
do_break_wand(struct obj *obj)
{
    static const char nothing_else_happens[] = "But nothing else happens...";
    register int i;
    coordxy x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    boolean shop_damage = FALSE;
    boolean fillmsg = FALSE;
    int expltype = EXPL_MAGICAL;
    char confirm[QBUFSZ], buf[BUFSZ];
    boolean is_fragile = (objdescr_is(obj, "balsa")
                          || objdescr_is(obj, "glass"));

    if (nohands(gy.youmonst.data)) {
        You_cant("break %s without hands!", yname(obj));
        return ECMD_OK;
    } else if (!freehand()) {
        Your("%s are occupied!", makeplural(body_part(HAND)));
        return ECMD_OK;
    } else if (ACURR(A_STR) < (is_fragile ? 5 : 10)) {
        You("don't have the strength to break %s!", yname(obj));
        return ECMD_OK;
    }
    if (!paranoid_query(ParanoidBreakwand,
                        safe_qbuf(confirm,
                                  "Are you really sure you want to break ",
                                  "?", obj, yname, ysimple_name, "the wand")))
        return ECMD_OK;
    pline("Raising %s high above your %s, you %s it in two!", yname(obj),
          body_part(HEAD), is_fragile ? "snap" : "break");

    /* [ALI] Do this first so that wand is removed from bill. Otherwise,
     * the freeinv() below also hides it from setpaid() which causes problems.
     */
    if (obj->unpaid) {
        check_unpaid(obj); /* Extra charge for use */
        costly_alteration(obj, COST_DSTROY);
    }

    gc.current_wand = obj; /* destroy_item might reset this */
    freeinv(obj);       /* hide it from destroy_item instead... */
    setnotworn(obj);    /* so we need to do this ourselves */

    if (!zappable(obj)) {
        pline(nothing_else_happens);
        goto discard_broken_wand;
    }
    /* successful call to zappable() consumes a charge; put it back */
    obj->spe++;
    /* might have "wrested" a final charge, taking it from 0 to -1;
       if so, we just brought it back up to 0, which wouldn't do much
       below so give it 1..3 charges now, usually making it stronger
       than an ordinary last charge (the wand is already gone from
       inventory, so perm_invent can't accidentally reveal this) */
    if (!obj->spe)
        obj->spe = rnd(3);

    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_OPENING:
        if (u.ustuck) {
            release_hold();
            if (obj->dknown)
                makeknown(WAN_OPENING);
            goto discard_broken_wand;
        }
        /*FALLTHRU*/
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_SECRET_DOOR_DETECTION:
        pline(nothing_else_happens);
        goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
        dmg *= 4;
        goto wanexpl;
    case WAN_FIRE:
        expltype = EXPL_FIERY;
        /*FALLTHRU*/
    case WAN_COLD:
        if (expltype == EXPL_MAGICAL)
            expltype = EXPL_FROSTY;
        dmg *= 2;
        /*FALLTHRU*/
    case WAN_MAGIC_MISSILE:
 wanexpl:
        explode(u.ux, u.uy, -(obj->otyp), dmg, WAND_CLASS, expltype);
        makeknown(obj->otyp); /* explode describes the effect */
        goto discard_broken_wand;
    case WAN_STRIKING:
        /* we want this before the explosion instead of at the very end */
        Soundeffect(se_wall_of_force, 65);
        pline("A wall of force smashes down around you!");
        dmg = d(1 + obj->spe, 6); /* normally 2d12 */
        /*FALLTHRU*/
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_TELEPORTATION:
    case WAN_UNDEAD_TURNING:
        affects_objects = TRUE;
        break;
    default:
        break;
    }

    /* magical explosion and its visual effect occur before specific effects
     */
    /* [TODO?  This really ought to prevent the explosion from being
       fatal so that we never leave a bones file where none of the
       surrounding targets (or underlying objects) got affected yet.] */
    explode(obj->ox, obj->oy, -(obj->otyp), rnd(dmg), WAND_CLASS,
            EXPL_MAGICAL);

    /* prepare for potential feedback from polymorph... */
    zapsetup();

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= N_DIRS; i++) {
        gb.bhitpos.x = x = obj->ox + xdir[i];
        gb.bhitpos.y = y = obj->oy + ydir[i];
        if (!isok(x, y))
            continue;

        if (obj->otyp == WAN_DIGGING) {
            schar typ;

            if (dig_check(BY_OBJECT, FALSE, x, y)) {
                if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ)) {
                    /* normally, pits and holes don't anger guards, but they
                     * do if it's a wall or door that's being dug */
                    watch_dig((struct monst *) 0, x, y, TRUE);
                    if (*in_rooms(x, y, SHOPBASE))
                        shop_damage = TRUE;
                }
                /*
                 * Let liquid flow into the newly created pits.
                 * Adjust corresponding code in music.c for
                 * drum of earthquake if you alter this sequence.
                 */
                typ = fillholetyp(x, y, FALSE);
                if (typ != ROOM) {
                    levl[x][y].typ = typ, levl[x][y].flags = 0;
                    liquid_flow(x, y, typ, t_at(x, y),
                                fillmsg
                                  ? (char *) 0
                                  : "Some holes are quickly filled with %s!");
                    fillmsg = TRUE;
                } else {
                    digactualhole(x, y, BY_OBJECT,
                                  (rn2(obj->spe) < 3
                                   || (!Can_dig_down(&u.uz)
                                       && !levl[x][y].candig)) ? PIT : HOLE);
                }
            }
            continue;
        } else if (obj->otyp == WAN_CREATE_MONSTER) {
            /* u.ux,u.uy creates it near you--x,y might create it in rock */
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
            continue;
        } else if (x != u.ux || y != u.uy) {
            /*
             * Wand breakage is targeting a square adjacent to the hero,
             * which might contain a monster or a pile of objects or both.
             * Handle objects last; avoids having undead turning raise an
             * undead's corpse and then attack resulting undead monster.
             * obj->bypass in bhitm() prevents the polymorphing of items
             * dropped due to monster's polymorph and prevents undead
             * turning that kills an undead from raising resulting corpse.
             */
            if ((mon = m_at(x, y)) != 0) {
                (void) bhitm(mon, obj);
                /* if (gc.context.botl) bot(); */
            }
            if (affects_objects && gl.level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (gc.context.botl)
                    bot(); /* potion effects */
            }
        } else {
            /*
             * Wand breakage is targeting the hero.  Using xdir[]+ydir[]
             * deltas for location selection causes this case to happen
             * after all the surrounding squares have been handled.
             * Process objects first, in case damage is fatal and leaves
             * bones, or teleportation sends one or more of the objects to
             * same destination as hero (lookhere/autopickup); also avoids
             * the polymorphing of gear dropped due to hero's transformation.
             * (Unlike with monsters being hit by zaps, we can't rely on use
             * of obj->bypass in the zap code to accomplish that last case
             * since it's also used by retouch_equipment() for polyself.)
             */
            if (affects_objects && gl.level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (gc.context.botl)
                    bot(); /* potion effects */
            }
            damage = zapyourself(obj, FALSE);
            if (damage) {
                Sprintf(buf, "killed %sself by breaking a wand", uhim());
                losehp(Maybe_Half_Phys(damage), buf, NO_KILLER_PREFIX);
            }
            if (gc.context.botl)
                bot(); /* blindness */
        }
    }

    /* potentially give post zap/break feedback */
    zapwrapup();

    /* Note: if player fell thru, this call is a no-op.
       Damage is handled in digactualhole in that case */
    if (shop_damage)
        pay_for_damage("dig into", FALSE);

    if (obj->otyp == WAN_LIGHT)
        litroom(TRUE, obj); /* only needs to be done once */

 discard_broken_wand:
    obj = gc.current_wand; /* [see dozap() and destroy_item()] */
    gc.current_wand = 0;
    if (obj)
        delobj(obj);
    nomul(0);
    return ECMD_TIME;
}

/* getobj callback for object to apply - this is more complex than most other
 * callbacks because there are a lot of appliables */
static int
apply_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    /* all tools, all wands (breaking), all spellbooks (flipping through -
       including blank/novel/Book of the Dead) */
    if (obj->oclass == TOOL_CLASS || obj->oclass == WAND_CLASS
        || obj->oclass == SPBOOK_CLASS)
        return GETOBJ_SUGGEST;

    /* certain weapons */
    if (obj->oclass == WEAPON_CLASS
        && (is_pick(obj) || is_axe(obj) || is_pole(obj)
            || obj->otyp == BULLWHIP))
        return GETOBJ_SUGGEST;

    if (obj->oclass == POTION_CLASS) {
        /* permit applying unknown potions, but don't suggest them */
        if (!obj->dknown || !objects[obj->otyp].oc_name_known)
            return GETOBJ_DOWNPLAY;

        /* only applicable potion is oil, and it will only be suggested as a
           choice when already discovered */
        if (obj->otyp == POT_OIL)
            return GETOBJ_SUGGEST;
    }

    /* certain foods */
    if (obj->otyp == CREAM_PIE || obj->otyp == EUCALYPTUS_LEAF
        || obj->otyp == LUMP_OF_ROYAL_JELLY)
        return GETOBJ_SUGGEST;

    if (is_graystone(obj)) {
        /* The only case where we don't suggest a gray stone is if we KNOW it
           isn't a touchstone. */
        if (!obj->dknown)
            return GETOBJ_SUGGEST;

        if (obj->otyp != TOUCHSTONE
            && (objects[TOUCHSTONE].oc_name_known
                || objects[obj->otyp].oc_name_known))
            return GETOBJ_EXCLUDE_SELECTABLE;

        return GETOBJ_SUGGEST;
    }

    /* item can't be applied; if picked anyway,
       _EXCLUDE would yield "That is a silly thing to apply.",
       _EXCLUDE_SELECTABLE yields "Sorry, I don't know how to use that." */
    return GETOBJ_EXCLUDE_SELECTABLE;
}

/* the #apply command, 'a' */
int
doapply(void)
{
    struct obj *obj;
    register int res = ECMD_TIME;

    if (nohands(gy.youmonst.data)) {
        You("aren't able to use or apply tools in your current form.");
        return ECMD_OK;
    }
    if (check_capacity((char *) 0))
        return ECMD_OK;

    obj = getobj("use or apply", apply_ok, GETOBJ_NOFLAGS);
    if (!obj)
        return ECMD_CANCEL;

    if (!retouch_object(&obj, FALSE))
        return ECMD_TIME; /* evading your grasp costs a turn; just be
                             grateful that you don't drop it as well */

    if (obj->oclass == WAND_CLASS)
        return do_break_wand(obj);

    if (obj->oclass == SPBOOK_CLASS)
        return flip_through_book(obj);

    switch (obj->otyp) {
    case BLINDFOLD:
    case LENSES:
        if (obj == ublindf) {
            if (!cursed(obj))
                Blindf_off(obj);
        } else if (!ublindf) {
            Blindf_on(obj);
        } else {
            You("are already %s.",
                (ublindf->otyp == TOWEL) ? "covered by a towel"
                : (ublindf->otyp == BLINDFOLD) ? "wearing a blindfold"
                  : "wearing lenses");
        }
        break;
    case CREAM_PIE:
        res = use_cream_pie(obj);
        break;
    case LUMP_OF_ROYAL_JELLY:
        res = use_royal_jelly(obj);
        break;
    case BULLWHIP:
        res = use_whip(obj);
        break;
    case GRAPPLING_HOOK:
        res = use_grapple(obj);
        break;
    case LARGE_BOX:
    case CHEST:
    case ICE_BOX:
    case SACK:
    case BAG_OF_HOLDING:
    case OILSKIN_SACK:
        res = use_container(&obj, TRUE, FALSE);
        break;
    case BAG_OF_TRICKS:
        (void) bagotricks(obj, FALSE, (int *) 0);
        break;
    case CAN_OF_GREASE:
        res = use_grease(obj);
        break;
    case LOCK_PICK:
    case CREDIT_CARD:
    case SKELETON_KEY:
        res = (pick_lock(obj, 0, 0, NULL) != 0) ? ECMD_TIME : ECMD_OK;
        break;
    case PICK_AXE:
    case DWARVISH_MATTOCK:
        res = use_pick_axe(obj);
        break;
    case TINNING_KIT:
        use_tinning_kit(obj);
        break;
    case LEASH:
        res = use_leash(obj);
        break;
    case SADDLE:
        res = use_saddle(obj);
        break;
    case MAGIC_WHISTLE:
        use_magic_whistle(obj);
        break;
    case TIN_WHISTLE:
        use_whistle(obj);
        break;
    case EUCALYPTUS_LEAF:
        /* MRKR: Every Australian knows that a gum leaf makes an excellent
         * whistle, especially if your pet is a tame kangaroo named Skippy.
         */
        if (obj->blessed) {
            use_magic_whistle(obj);
            /* sometimes the blessing will be worn off */
            if (!rn2(49)) {
                if (!Blind) {
                    pline("%s %s.", Yobjnam2(obj, "glow"), hcolor("brown"));
                    set_bknown(obj, 1);
                }
                unbless(obj);
            }
        } else {
            use_whistle(obj);
        }
        break;
    case STETHOSCOPE:
        res = use_stethoscope(obj);
        break;
    case MIRROR:
        res = use_mirror(obj);
        break;
    case BELL:
    case BELL_OF_OPENING:
        use_bell(&obj);
        break;
    case CANDELABRUM_OF_INVOCATION:
        use_candelabrum(obj);
        break;
    case WAX_CANDLE:
    case TALLOW_CANDLE:
        use_candle(&obj);
        break;
    case OIL_LAMP:
    case MAGIC_LAMP:
    case BRASS_LANTERN:
        use_lamp(obj);
        break;
    case POT_OIL:
        light_cocktail(&obj);
        break;
    case EXPENSIVE_CAMERA:
        res = use_camera(obj);
        break;
    case TOWEL:
        res = use_towel(obj);
        break;
    case CRYSTAL_BALL:
        use_crystal_ball(&obj);
        break;
    case MAGIC_MARKER:
        res = dowrite(obj);
        break;
    case TIN_OPENER:
        res = use_tin_opener(obj);
        break;
    case FIGURINE:
        res = use_figurine(&obj);
        break;
    case UNICORN_HORN:
        use_unicorn_horn(&obj);
        break;
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
        res = do_play_instrument(obj);
        break;
    case HORN_OF_PLENTY: /* not a musical instrument */
        (void) hornoplenty(obj, FALSE, (struct obj *) 0);
        break;
    case LAND_MINE:
    case BEARTRAP:
        use_trap(obj);
        break;
    case FLINT:
    case LUCKSTONE:
    case LOADSTONE:
    case TOUCHSTONE:
        res = use_stone(obj);
        break;
    default:
        /* Pole-weapons can strike at a distance */
        if (is_pole(obj)) {
            res = use_pole(obj, FALSE);
            break;
        } else if (is_pick(obj) || is_axe(obj)) {
            res = use_pick_axe(obj);
            break;
        }
        pline("Sorry, I don't know how to use that.");
        return ECMD_FAIL;
    }
    if (obj && obj->oartifact) {
        res |= arti_speak(obj); /* sets ECMD_TIME bit if artifact speaks */
    }
    return res;
}

/* Keep track of unfixable troubles for purposes of messages saying you feel
 * great.
 */
int
unfixable_trouble_count(boolean is_horn)
{
    int unfixable_trbl = 0;

    if (Stoned)
        unfixable_trbl++;
    if (Slimed)
        unfixable_trbl++;
    if (Strangled)
        unfixable_trbl++;
    if (ATEMP(A_DEX) < 0 && Wounded_legs)
        unfixable_trbl++;
    if (ATEMP(A_STR) < 0 && u.uhs >= WEAK)
        unfixable_trbl++;
    /* lycanthropy is undesirable, but it doesn't actually make you feel bad
       so don't count it as a trouble which can't be fixed */

    /*
     * Unicorn horn can fix these when they're timed but not when
     * they aren't.  Potion of restore ability doesn't touch them,
     * so they're always unfixable for the not-unihorn case.
     * [Most of these are timed only, so always curable via horn.
     * An exception is Stunned, which can be forced On by certain
     * polymorph forms (stalker, bats).]
     */
    if (Sick && (!is_horn || (Sick & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Stunned && (!is_horn || (HStun & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Confusion && (!is_horn || (HConfusion & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Hallucination && (!is_horn || (HHallucination & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Vomiting && (!is_horn || (Vomiting & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Deaf && (!is_horn || (HDeaf & ~TIMEOUT) != 0L))
        unfixable_trbl++;

    return unfixable_trbl;
}

static int
flip_through_book(struct obj *obj)
{
    if (Underwater) {
        You("don't want to get the pages even more soggy, do you?");
        return ECMD_OK;
    }

    You("flip through the pages of %s.", thesimpleoname(obj));

    if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        if (!Deaf) {
            if (!Hallucination) {
                Soundeffect(se_rustling_paper, 50);
            }
            You_hear("the pages make an unpleasant %s sound.",
                     Hallucination ? "chuckling"
                                   : "rustling");
        } else if (!Blind) {
            You_see("the pages glow faintly %s.", hcolor(NH_RED));
        } else {
            You_feel("the pages tremble.");
        }
    } else if (Blind) {
        pline("The pages feel %s.",
              Hallucination ? "freshly picked"
                            : "rough and dry");
    } else if (obj->otyp == SPE_BLANK_PAPER) {
        pline("This spellbook %s.",
              Hallucination ? "doesn't have much of a plot"
                            : "has nothing written in it");
        makeknown(obj->otyp);
    } else if (Hallucination) {
        You("enjoy the animated initials.");
    } else if (obj->otyp == SPE_NOVEL) {
        pline("This looks like it might be interesting to read.");
    } else {
        static const char *const fadeness[] = {
            "fresh",
            "slightly faded",
            "very faded",
            "extremely faded",
            "barely visible"
        };
        int findx = min(obj->spestudied, MAX_SPELL_STUDY);

        pline("The%s ink in this spellbook is %s.",
              objects[obj->otyp].oc_magic ? " magical" : "",
              fadeness[findx]);
    }

    return ECMD_TIME;
}

/*apply.c*/

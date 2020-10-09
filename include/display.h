/* NetHack 3.7	display.h	$NHDT-Date: 1597700875 2020/08/17 21:47:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) Dean Luick, with acknowledgements to Kevin Darcy */
/* and Dave Cohrs, 1990.                                          */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef VISION_H
#include "vision.h"
#endif
#ifndef MONDATA_H
#include "mondata.h" /* for mindless() */
#endif

/* types of explosions */
enum explosion_types {
    EXPL_DARK    = 0,
    EXPL_NOXIOUS = 1,
    EXPL_MUDDY   = 2,
    EXPL_WET     = 3,
    EXPL_MAGICAL = 4,
    EXPL_FIERY   = 5,
    EXPL_FROSTY  = 6,
    EXPL_MAX     = 7
};

/*
 * vobj_at()
 *
 * Returns the head of the list of objects that the player can see
 * at location (x,y).
 */
#define vobj_at(x, y) (g.level.objects[x][y])

/*
 * sensemon()
 *
 * Returns true if the hero can sense the given monster.  This includes
 * monsters that are hiding or mimicing other monsters.
 */
#define tp_sensemon(mon) \
    (/* The hero can always sense a monster IF:        */  \
     /* 1. the monster has a brain to sense            */  \
     (!mindless(mon->data))                                \
     /* AND     2a. hero is blind and telepathic       */  \
      && ((Blind && Blind_telepat)                         \
          /* OR 2b. hero is using a telepathy inducing */  \
          /*        object and in range                */  \
          || (Unblind_telepat                              \
              && (distu(mon->mx, mon->my) <= (BOLT_LIM * BOLT_LIM)))))

#define sensemon(mon) \
    (tp_sensemon(mon) || Detect_monsters || MATCH_WARN_OF_MON(mon))

/*
 * mon_warning() is used to warn of any dangerous monsters in your
 * vicinity, and a glyph representing the warning level is displayed.
 */

#define mon_warning(mon)                                                 \
    (Warning && !(mon)->mpeaceful && (distu((mon)->mx, (mon)->my) < 100) \
     && (((int) ((mon)->m_lev / 4)) >= g.context.warnlevel))

/*
 * mon_visible()
 *
 * Returns true if the hero can see the monster.  It is assumed that the
 * hero can physically see the location of the monster.  The function
 * vobj_at() returns a pointer to an object that the hero can see there.
 * Infravision is not taken into account.
 */
#if 0
#define mon_visible(mon) \
    (/* The hero can see the monster IF the monster                     */ \
     (!mon->minvis || See_invisible)  /*     1. is not invisible        */ \
     && !mon->mundetected             /* AND 2. not an undetected hider */ \
     && !(mon->mburied || u.uburied)) /* AND 3. neither you nor it is buried */
#else   /* without 'mburied' and 'uburied' */
#define mon_visible(mon) \
    (/* The hero can see the monster IF the monster                     */ \
     (!mon->minvis || See_invisible)  /*     1. is not invisible        */ \
     && !mon->mundetected)            /* AND 2. not an undetected hider */
#endif

/*
 * see_with_infrared()
 *
 * This function is true if the player can see a monster using infravision.
 * The caller must check for invisibility (invisible monsters are also
 * invisible to infravision), because this is usually called from within
 * canseemon() or canspotmon() which already check that.
 */
#define see_with_infrared(mon)                        \
    (!Blind && Infravision && mon && infravisible(mon->data) \
     && couldsee(mon->mx, mon->my))

/*
 * canseemon()
 *
 * This is the globally used canseemon().  It is not called within the display
 * routines.  Like mon_visible(), but it checks to see if the hero sees the
 * location instead of assuming it.  (And also considers worms.)
 */
#define canseemon(mon)                                                    \
    ((mon->wormno ? worm_known(mon)                                       \
                  : (cansee(mon->mx, mon->my) || see_with_infrared(mon))) \
     && mon_visible(mon))

/*
 * canspotmon(mon)
 *
 * This function checks whether you can either see a monster or sense it by
 * telepathy, and is what you usually call for monsters about which nothing is
 * known.
 */
#define canspotmon(mon) (canseemon(mon) || sensemon(mon))

/* knowninvisible(mon)
 * This one checks to see if you know a monster is both there and invisible.
 * 1) If you can see the monster and have see invisible, it is assumed the
 * monster is transparent, but visible in some manner.  (Earlier versions of
 * Nethack were really inconsistent on this.)
 * 2) If you can't see the monster, but can see its location and you have
 * telepathy that works when you can see, you can tell that there is a
 * creature in an apparently empty spot.
 * Infravision is not relevant; we assume that invisible monsters are also
 * invisible to infravision.
 */
#define knowninvisible(mon)                                               \
    (mtmp->minvis                                                         \
     && ((cansee(mon->mx, mon->my) && (See_invisible || Detect_monsters)) \
         || (!Blind && (HTelepat & ~INTRINSIC)                            \
             && distu(mon->mx, mon->my) <= (BOLT_LIM * BOLT_LIM))))

/*
 * is_safemon(mon)
 *
 * A special case check used in attack() and domove().  Placing the
 * definition here is convenient.  No longer limited to pets.
 */
#define is_safemon(mon) \
    (flags.safe_dog && (mon) && (mon)->mpeaceful && canspotmon(mon)     \
     && !Confusion && !Hallucination && !Stunned)

/*
 * canseeself()
 * senseself()
 * canspotself()
 *
 * This returns true if the hero can see her/himself.
 *
 * Sensing yourself by touch is treated as seeing yourself, even if
 * unable to see.  So when blind, being invisible won't affect your
 * self-perception, and when swallowed, the enclosing monster touches.
 */
#define canseeself() (Blind || u.uswallow || (!Invisible && !u.uundetected))
#define senseself() (Unblind_telepat || Detect_monsters)
#define canspotself() (canseeself() || senseself())

/*
 * random_monster()
 * random_object()
 *
 * Respectively return a random monster or object.
 */
#define random_monster(rng) rng(NUMMONS)
#define random_object(rng) (rng(NUM_OBJECTS - 1) + 1)

/*
 * what_obj()
 * what_mon()
 *
 * If hallucinating, choose a random object/monster, otherwise, use the one
 * given. Use the given rng to handle hallucination.
 */
#define what_obj(obj, rng) (Hallucination ? random_object(rng) : obj)
#define what_mon(mon, rng) (Hallucination ? random_monster(rng) : mon)

/*
 * newsym_rn2
 *
 * An appropriate random number generator for use with newsym(), when
 * randomness is needed there. This is currently hardcoded as
 * rn2_on_display_rng, but is futureproofed for cases where we might
 * want to prevent display-random objects entering the character's
 * memory (this isn't important at present but may be if we need
 * reproducible gameplay for some reason).
 */
#define newsym_rn2 rn2_on_display_rng

/*
 * covers_objects()
 * covers_traps()
 *
 * These routines are true if what is really at the given location will
 * "cover" any objects or traps that might be there.
 */
#define covers_objects(xx, yy) \
    ((is_pool(xx, yy) && !Underwater) || (levl[xx][yy].typ == LAVAPOOL))

#define covers_traps(xx, yy) covers_objects(xx, yy)

/*
 * tmp_at() control calls.
 */
#define DISP_BEAM    (-1) /* Keep all glyphs showing & clean up at end. */
#define DISP_ALL     (-2) /* Like beam, but still displayed if not visible. */
#define DISP_TETHER  (-3) /* Like beam, but tether glyph differs from final */
#define DISP_FLASH   (-4) /* Clean up each glyph before displaying new one. */
#define DISP_ALWAYS  (-5) /* Like flash, but still displayed if not visible. */
#define DISP_CHANGE  (-6) /* Change glyph. */
#define DISP_END     (-7) /* Clean up. */
#define DISP_FREEMEM (-8) /* Free all memory during exit only. */

/* Total number of cmap indices in the shield_static[] array. */
#define SHIELD_COUNT 21
#define BACKTRACK (-1)    /* flag for DISP_END to display each prior location */

/*
 * display_self()
 *
 * Display the hero.  It is assumed that all checks necessary to determine
 * _if_ the hero can be seen have already been done.
 */
#define maybe_display_usteed(otherwise_self)                            \
    ((u.usteed && mon_visible(u.usteed))                                \
     ? ridden_mon_to_glyph(u.usteed, rn2_on_display_rng)                \
     : (otherwise_self))

#define display_self() \
    show_glyph(u.ux, u.uy,                                                  \
           maybe_display_usteed((U_AP_TYPE == M_AP_NOTHING)                 \
                                ? hero_glyph                                \
                                : (U_AP_TYPE == M_AP_FURNITURE)             \
                                  ? cmap_to_glyph(g.youmonst.mappearance)     \
                                  : (U_AP_TYPE == M_AP_OBJECT)              \
                                    ? objnum_to_glyph(g.youmonst.mappearance) \
                                    /* else U_AP_TYPE == M_AP_MONSTER */    \
                                    : monnum_to_glyph(g.youmonst.mappearance)))

/*
 * A glyph is an abstraction that represents a _unique_ monster, object,
 * dungeon part, or effect.  The uniqueness is important.  For example,
 * It is not enough to have four (one for each "direction") zap beam glyphs,
 * we need a set of four for each beam type.  Why go to so much trouble?
 * Because it is possible that any given window dependent display driver
 * [print_glyph()] can produce something different for each type of glyph.
 * That is, a beam of cold and a beam of fire would not only be different
 * colors, but would also be represented by different symbols.
 *
 * Glyphs are grouped for easy accessibility:
 *
 * monster      Represents all the wild (not tame) monsters.  Count: NUMMONS.
 *
 * pet          Represents all of the tame monsters.  Count: NUMMONS
 *
 * invisible    Invisible monster placeholder.  Count: 1
 *
 * detect       Represents all detected monsters.  Count: NUMMONS
 *
 * corpse       One for each monster.  Count: NUMMONS
 *
 * ridden       Represents all monsters being ridden.  Count: NUMMONS
 *
 * object       One for each object.  Count: NUM_OBJECTS
 *
 * cmap         One for each entry in the character map.  The character map
 *              is the dungeon features and other miscellaneous things.
 *              Count: MAXPCHARS
 *
 * explosions   A set of nine for each of the following seven explosion types:
 *                   dark, noxious, muddy, wet, magical, fiery, frosty.
 *              The nine positions represent those surrounding the hero.
 *              Count: MAXEXPCHARS * EXPL_MAX
 *
 * zap beam     A set of four (there are four directions) for each beam type.
 *              The beam type is shifted over 2 positions and the direction
 *              is stored in the lower 2 bits.  Count: NUM_ZAP << 2
 *
 * swallow      A set of eight for each monster.  The eight positions rep-
 *              resent those surrounding the hero.  The monster number is
 *              shifted over 3 positions and the swallow position is stored
 *              in the lower three bits.  Count: NUMMONS << 3
 *
 * warning      A set of six representing the different warning levels.
 *
 * statue       One for each monster.  Count: NUMMONS
 *
 * unexplored   One for unexplored areas of the map
 * nothing      Nothing but background
 *
 * The following are offsets used to convert to and from a glyph.
 */
#define NUM_ZAP 8 /* number of zap beam types */

#define GLYPH_MON_OFF     0
#define GLYPH_PET_OFF     (NUMMONS + GLYPH_MON_OFF)
#define GLYPH_INVIS_OFF   (NUMMONS + GLYPH_PET_OFF)
#define GLYPH_DETECT_OFF  (1 + GLYPH_INVIS_OFF)
#define GLYPH_BODY_OFF    (NUMMONS + GLYPH_DETECT_OFF)
#define GLYPH_RIDDEN_OFF  (NUMMONS + GLYPH_BODY_OFF)
#define GLYPH_OBJ_OFF     (NUMMONS + GLYPH_RIDDEN_OFF)
#define GLYPH_CMAP_OFF    (NUM_OBJECTS + GLYPH_OBJ_OFF)
#define GLYPH_EXPLODE_OFF ((MAXPCHARS - MAXEXPCHARS) + GLYPH_CMAP_OFF)
#define GLYPH_ZAP_OFF     ((MAXEXPCHARS * EXPL_MAX) + GLYPH_EXPLODE_OFF)
#define GLYPH_SWALLOW_OFF ((NUM_ZAP << 2) + GLYPH_ZAP_OFF)
#define GLYPH_WARNING_OFF ((NUMMONS << 3) + GLYPH_SWALLOW_OFF)
#define GLYPH_STATUE_OFF  (WARNCOUNT + GLYPH_WARNING_OFF)
#define GLYPH_UNEXPLORED_OFF (NUMMONS + GLYPH_STATUE_OFF)
#define GLYPH_NOTHING_OFF (GLYPH_UNEXPLORED_OFF + 1)
#define MAX_GLYPH         (GLYPH_NOTHING_OFF + 1)

#define NO_GLYPH          MAX_GLYPH
#define GLYPH_INVISIBLE   GLYPH_INVIS_OFF
#define GLYPH_UNEXPLORED  GLYPH_UNEXPLORED_OFF
#define GLYPH_NOTHING     GLYPH_NOTHING_OFF

#define warning_to_glyph(mwarnlev) ((mwarnlev) + GLYPH_WARNING_OFF)
#define mon_to_glyph(mon, rng)                                      \
    ((int) what_mon(monsndx((mon)->data), rng) + GLYPH_MON_OFF)
#define detected_mon_to_glyph(mon, rng)                             \
    ((int) what_mon(monsndx((mon)->data), rng) + GLYPH_DETECT_OFF)
#define ridden_mon_to_glyph(mon, rng)                               \
    ((int) what_mon(monsndx((mon)->data), rng) + GLYPH_RIDDEN_OFF)
#define pet_to_glyph(mon, rng)                                      \
    ((int) what_mon(monsndx((mon)->data), rng) + GLYPH_PET_OFF)

/* This has the unfortunate side effect of needing a global variable    */
/* to store a result. 'otg_temp' is defined and declared in decl.{ch}.  */
#define random_obj_to_glyph(rng) \
    ((g.otg_temp = random_object(rng)) == CORPSE                \
         ? random_monster(rng) + GLYPH_BODY_OFF                 \
         : g.otg_temp + GLYPH_OBJ_OFF)

#define obj_to_glyph(obj, rng) \
    (((obj)->otyp == STATUE)                                    \
         ? statue_to_glyph(obj, rng)                            \
         : Hallucination                                        \
               ? random_obj_to_glyph(rng)                       \
               : ((obj)->otyp == CORPSE)                        \
                     ? (int) (obj)->corpsenm + GLYPH_BODY_OFF   \
                     : (int) (obj)->otyp + GLYPH_OBJ_OFF)

/* MRKR: Statues now have glyphs corresponding to the monster they    */
/*       represent and look like monsters when you are hallucinating. */

#define statue_to_glyph(obj, rng)                              \
    (Hallucination ? random_monster(rng) + GLYPH_MON_OFF       \
                   : (int) (obj)->corpsenm + GLYPH_STATUE_OFF)

/* briefly used for Qt's "paper doll" inventory which shows map tiles for
   equipped objects; those vary like floor items during hallucination now
   so this isn't used anywhere */
#define obj_to_true_glyph(obj) \
    (((obj)->otyp == STATUE)                            \
     ? ((int) (obj)->corpsenm + GLYPH_STATUE_OFF)       \
       : ((obj)->otyp == CORPSE)                        \
         ? ((int) (obj)->corpsenm + GLYPH_BODY_OFF)     \
           : ((int) (obj)->otyp + GLYPH_OBJ_OFF))

#define cmap_to_glyph(cmap_idx) ((int) (cmap_idx) + GLYPH_CMAP_OFF)
#define explosion_to_glyph(expltype, idx) \
    ((((expltype) * MAXEXPCHARS) + ((idx) - S_explode1)) + GLYPH_EXPLODE_OFF)

#define trap_to_glyph(trap)                                \
    cmap_to_glyph(trap_to_defsym((trap)->ttyp))

/* Not affected by hallucination.  Gives a generic body for CORPSE */
/* MRKR: ...and the generic statue */
#define objnum_to_glyph(onum) ((int) (onum) + GLYPH_OBJ_OFF)
#define monnum_to_glyph(mnum) ((int) (mnum) + GLYPH_MON_OFF)
#define detected_monnum_to_glyph(mnum) ((int) (mnum) + GLYPH_DETECT_OFF)
#define ridden_monnum_to_glyph(mnum) ((int) (mnum) + GLYPH_RIDDEN_OFF)
#define petnum_to_glyph(mnum) ((int) (mnum) + GLYPH_PET_OFF)

/* The hero's glyph when seen as a monster.
 */
#define hero_glyph                                                    \
    monnum_to_glyph((Upolyd || !flags.showrace)                       \
                        ? u.umonnum                                   \
                        : (flags.female && g.urace.femalenum != NON_PM) \
                              ? g.urace.femalenum                       \
                              : g.urace.malenum)

/*
 * Change the given glyph into it's given type.  Note:
 *      1) Pets, detected, and ridden monsters are animals and are converted
 *         to the proper monster number.
 *      2) Bodies are all mapped into the generic CORPSE object
 *      3) If handed a glyph out of range for the type, these functions
 *         will return NO_GLYPH (see exception below)
 *      4) glyph_to_swallow() does not return a showsyms[] index, but an
 *         offset from the first swallow symbol.  If handed something
 *         out of range, it will return zero (for lack of anything better
 *         to return).
 */
#define glyph_to_mon(glyph) \
    (glyph_is_normal_monster(glyph)                             \
         ? ((glyph) - GLYPH_MON_OFF)                            \
         : glyph_is_pet(glyph)                                  \
               ? ((glyph) - GLYPH_PET_OFF)                      \
               : glyph_is_detected_monster(glyph)               \
                     ? ((glyph) - GLYPH_DETECT_OFF)             \
                     : glyph_is_ridden_monster(glyph)           \
                           ? ((glyph) - GLYPH_RIDDEN_OFF)       \
                           : glyph_is_statue(glyph)             \
                                 ? ((glyph) - GLYPH_STATUE_OFF) \
                                 : NO_GLYPH)
#define glyph_to_obj(glyph) \
    (glyph_is_body(glyph)                        \
         ? CORPSE                                \
         : glyph_is_statue(glyph)                \
               ? STATUE                          \
               : glyph_is_normal_object(glyph)   \
                     ? ((glyph) - GLYPH_OBJ_OFF) \
                     : NO_GLYPH)
#define glyph_to_trap(glyph) \
    (glyph_is_trap(glyph) ? ((int) defsym_to_trap((glyph) - GLYPH_CMAP_OFF)) \
                          : NO_GLYPH)
#define glyph_to_cmap(glyph) \
    (glyph_is_cmap(glyph) ? ((glyph) - GLYPH_CMAP_OFF) : NO_GLYPH)
#define glyph_to_swallow(glyph) \
    (glyph_is_swallow(glyph) ? (((glyph) - GLYPH_SWALLOW_OFF) & 0x7) : 0)
#define glyph_to_warning(glyph) \
    (glyph_is_warning(glyph) ? ((glyph) - GLYPH_WARNING_OFF) : NO_GLYPH);

/*
 * Return true if the given glyph is what we want.  Note that bodies are
 * considered objects.
 */
#define glyph_is_monster(glyph)                            \
    (glyph_is_normal_monster(glyph) || glyph_is_pet(glyph) \
     || glyph_is_ridden_monster(glyph) || glyph_is_detected_monster(glyph))
#define glyph_is_normal_monster(glyph) \
    ((glyph) >= GLYPH_MON_OFF && (glyph) < (GLYPH_MON_OFF + NUMMONS))
#define glyph_is_pet(glyph) \
    ((glyph) >= GLYPH_PET_OFF && (glyph) < (GLYPH_PET_OFF + NUMMONS))
#define glyph_is_body(glyph) \
    ((glyph) >= GLYPH_BODY_OFF && (glyph) < (GLYPH_BODY_OFF + NUMMONS))

#define glyph_is_statue(glyph) \
    ((glyph) >= GLYPH_STATUE_OFF && (glyph) < (GLYPH_STATUE_OFF + NUMMONS))

#define glyph_is_ridden_monster(glyph) \
    ((glyph) >= GLYPH_RIDDEN_OFF && (glyph) < (GLYPH_RIDDEN_OFF + NUMMONS))
#define glyph_is_detected_monster(glyph) \
    ((glyph) >= GLYPH_DETECT_OFF && (glyph) < (GLYPH_DETECT_OFF + NUMMONS))
#define glyph_is_invisible(glyph) ((glyph) == GLYPH_INVISIBLE)
#define glyph_is_normal_object(glyph) \
    ((glyph) >= GLYPH_OBJ_OFF && (glyph) < (GLYPH_OBJ_OFF + NUM_OBJECTS))
#define glyph_is_object(glyph)                               \
    (glyph_is_normal_object(glyph) || glyph_is_statue(glyph) \
     || glyph_is_body(glyph))
#define glyph_is_trap(glyph)                         \
    ((glyph) >= (GLYPH_CMAP_OFF + trap_to_defsym(1)) \
     && (glyph) < (GLYPH_CMAP_OFF + trap_to_defsym(1) + TRAPNUM))
#define glyph_is_cmap(glyph) \
    ((glyph) >= GLYPH_CMAP_OFF && (glyph) < (GLYPH_CMAP_OFF + MAXPCHARS))
#define glyph_is_swallow(glyph)   \
    ((glyph) >= GLYPH_SWALLOW_OFF \
     && (glyph) < (GLYPH_SWALLOW_OFF + (NUMMONS << 3)))
#define glyph_is_warning(glyph)   \
    ((glyph) >= GLYPH_WARNING_OFF \
     && (glyph) < (GLYPH_WARNING_OFF + WARNCOUNT))
#define glyph_is_unexplored(glyph) ((glyph) == GLYPH_UNEXPLORED)
#define glyph_is_nothing(glyph) ((glyph) == GLYPH_NOTHING)

#endif /* DISPLAY_H */

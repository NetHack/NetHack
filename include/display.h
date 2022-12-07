/* NetHack 3.7	display.h	$NHDT-Date: 1661295667 2022/08/23 23:01:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.77 $ */
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

/*
 * vobj_at()
 *
 * Returns the head of the list of objects that the player can see
 * at location (x,y).  [Vestige of unimplemented invisible objects.]
 */
#define vobj_at(x, y) (gl.level.objects[x][y])

/*
 * sensemon()
 *
 * Returns true if the hero can sense the given monster.  This includes
 * monsters that are hiding or mimicing other monsters.
 *
 * [3.7] Note: the map doesn't display any monsters when hero is swallowed
 * (or display non-adjacent, non-submerged ones when hero is underwater),
 * so treat those situations as blocking telepathy, detection, and warning
 * even though conceptually they shouldn't do so.
 *
 * [3.7 also] The macros whose name begins with an understore have been
 * converted to functions in order to have compilers generate smaller code.
 * The retained underscore versions are still used in display.c but should
 * only be used in other situations if the function calls actually produce
 * noticeably slower processing.
 */
#define _tp_sensemon(mon) \
    (/* The hero can always sense a monster IF:        */  \
     /* 1. the monster has a brain to sense            */  \
     (!mindless(mon->data))                                \
     /* AND     2a. hero is blind and telepathic       */  \
      && ((Blind && Blind_telepat)                         \
          /* OR 2b. hero is using a telepathy inducing */  \
          /*        object and in range                */  \
          || (Unblind_telepat                              \
              && (mdistu(mon) <= (BOLT_LIM * BOLT_LIM)))))

/* organized to perform cheaper tests first;
   is_pool() vs is_pool_or_lava(): hero who is underwater can see adjacent
   lava, but presumeably any monster there is on top so not sensed */
#define _sensemon(mon) \
    (   (!u.uswallow || (mon) == u.ustuck)                                   \
     && (!Underwater || (mdistu(mon) <= 2 && is_pool((mon)->mx, (mon)->my))) \
     && (Detect_monsters || tp_sensemon(mon) || MATCH_WARN_OF_MON(mon))   )

/*
 * mon_warning() is used to warn of any dangerous monsters in your
 * vicinity, and a glyph representing the warning level is displayed.
 */
#define _mon_warning(mon) \
    (Warning && !(mon)->mpeaceful && (mdistu(mon) < 100)     \
     && (((int) ((mon)->m_lev / 4)) >= gc.context.warnlevel))

/*
 * mon_visible()
 *
 * Returns true if the hero can see the monster.  It is assumed that the
 * hero can physically see the location of the monster.  The function
 * vobj_at() returns a pointer to an object that the hero can see there.
 * Infravision is not taken into account.
 *
 * Note:  not reliable for concealed mimics.  They don't have
 * 'mon->mundetected' set even when mimicking objects or furniture.
 * [Fixing this with a pair of mon->m_ap_type checks here (via either
 * 'typ!=object && typ!=furniture' or 'typ==nothing || typ==monster')
 * will require reviewing every instance of mon_visible(), canseemon(),
 * canspotmon(), is_safemon() and perhaps others.  Fixing it by setting
 * mon->mundetected when concealed would be better but also require
 * reviewing all those instances and also existing mundetected instances.]
 */
#if 0
#define _mon_visible(mon) \
    (/* The hero can see the monster IF the monster                     */ \
     (!mon->minvis || See_invisible)  /*     1. is not invisible        */ \
     && !mon->mundetected             /* AND 2. not an undetected hider */ \
     && !(mon->mburied || u.uburied)) /* AND 3. neither you nor it is buried */
#else   /* without 'mburied' and 'uburied' */
#define _mon_visible(mon) \
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
#define _see_with_infrared(mon) \
    (!Blind && Infravision && mon && infravisible(mon->data) \
     && couldsee(mon->mx, mon->my))

/*
 * canseemon()
 *
 * This is the globally used canseemon().  It is not called within the display
 * routines.  Like mon_visible(), but it checks to see if the hero sees the
 * location instead of assuming it.  (And also considers worms.)
 */
#define _canseemon(mon) \
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
 * [3.7: the macro definition erroneously started with 'mtmp->minvis' for
 * over 20 years.  The one place it's used called it as knowninvisible(mtmp)
 * so worked by coincidence when there was no argument expansion involved
 * on the first line.]
 */
#define _knowninvisible(mon) \
    ((mon)->minvis                                                      \
     && ((cansee((mon)->mx, (mon)->my)                                  \
          && (See_invisible || Detect_monsters))                        \
         || (!Blind && (HTelepat & ~INTRINSIC)                          \
             && mdistu(mon) <= (BOLT_LIM * BOLT_LIM))))

/*
 * is_safemon(mon)
 *
 * A special case check used in attack() and domove().  Placing the
 * definition here is convenient.  No longer limited to pets.
 */
#define _is_safemon(mon) \
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
#define maybe_display_usteed(otherwise_self)                 \
    ((u.usteed && mon_visible(u.usteed))                     \
         ? ridden_mon_to_glyph(u.usteed, rn2_on_display_rng) \
         : (otherwise_self))

#define display_self() \
    show_glyph(u.ux, u.uy, maybe_display_usteed(                        \
        ((int) U_AP_TYPE == M_AP_NOTHING)                               \
        ? hero_glyph                                                    \
        : ((int) U_AP_TYPE == M_AP_FURNITURE)                           \
          ? cmap_to_glyph((int) gy.youmonst.mappearance)                \
          : ((int) U_AP_TYPE == M_AP_OBJECT)                            \
            ? objnum_to_glyph((int) gy.youmonst.mappearance)            \
            /* else U_AP_TYPE == M_AP_MONSTER */                        \
            : monnum_to_glyph((int) gy.youmonst.mappearance, Ugender)))

/*
 * NetHack glyphs
 *
 * A glyph is an abstraction that represents a _unique_ monster, object,
 * dungeon part, or effect.  The uniqueness is important.  For example,
 * It is not enough to have four (one for each "direction") zap beam glyphs,
 * we need a set of four for each beam type.  Why go to so much trouble?
 * Because it is possible that any given window dependent display driver
 * [print_glyph()] can produce something different for each type of glyph.
 * That is, a beam of cold and a beam of fire would not only be different
 * colors, but would also be represented by different symbols.
 */


#include "color.h"
/* 3.6.3: poison gas zap used to be yellow and acid zap was green,
   which conflicted with the corresponding dragon colors */
enum zap_colors {
    zap_color_missile    = HI_ZAP,
    zap_color_fire       = CLR_ORANGE,
    zap_color_frost      = CLR_WHITE,
    zap_color_sleep      = HI_ZAP,
    zap_color_death      = CLR_BLACK,
    zap_color_lightning  = CLR_WHITE,
    zap_color_poison_gas = CLR_GREEN,
    zap_color_acid       = CLR_YELLOW
};

enum altar_colors {
    altar_color_unaligned = CLR_RED,
#if defined(USE_GENERAL_ALTAR_COLORS)
        /* On OSX with TERM=xterm-color256 these render as
         *  white -> tty: gray, curses: ok
         *  gray  -> both tty and curses: black
         *  black -> both tty and curses: blue
         *  red   -> both tty and curses: ok.
         * Since the colors have specific associations (with the
         * unicorns matched with each alignment), we shouldn't use
         * scrambled colors and we don't have sufficient information
         * to handle platform-specific color variations.
         */
    altar_color_chaotic = CLR_BLACK,
    altar_color_neutral = CLR_GRAY,
    altar_color_lawful  = CLR_WHITE,
#else
    altar_color_chaotic = CLR_GRAY,
    altar_color_neutral = CLR_GRAY,
    altar_color_lawful  = CLR_GRAY,
#endif
    altar_color_other = CLR_BRIGHT_MAGENTA,
};

/* types of explosions */
enum explosion_types {
    EXPL_DARK = 0,
    EXPL_NOXIOUS = 1,
    EXPL_MUDDY = 2,
    EXPL_WET = 3,
    EXPL_MAGICAL = 4,
    EXPL_FIERY = 5,
    EXPL_FROSTY = 6,
    EXPL_MAX = 7
};

/* above plus this redundant? */
enum expl_types {
    expl_dark,
    expl_noxious,
    expl_muddy,
    expl_wet,
    expl_magical,
    expl_fiery,
    expl_frosty,
};

enum explode_colors {
    explode_color_dark = CLR_BLACK,
    explode_color_noxious = CLR_GREEN,
    explode_color_muddy = CLR_BROWN,
    explode_color_wet = CLR_BLUE,
    explode_color_magical = CLR_MAGENTA,
    explode_color_fiery = CLR_ORANGE,
    explode_color_frosty = CLR_WHITE
};
enum altar_types {
    altar_unaligned,
    altar_chaotic,
    altar_neutral,
    altar_lawful,
    altar_other
};
enum level_walls  { main_walls, mines_walls, gehennom_walls,
                    knox_walls, sokoban_walls };
enum { GM_FLAGS, GM_TTYCHAR, GM_COLOR, NUM_GLYPHMOD }; /* glyphmod entries */
enum glyphmap_change_triggers { gm_nochange, gm_newgame, gm_levelchange,
                                gm_optionchange, gm_symchange,
                                gm_accessibility_change };
#define NUM_ZAP 8 /* number of zap beam types */

/*
 * Glyphs are grouped for easy accessibility:
 *
 * male monsters    Represents all the wild (not tame) male monsters.
 *                  Count: NUMMONS.
 *
 * female monsters  Represents all the wild (not tame) female monsters.
 *                  Count: NUMMONS.
 *
 * male pets        Represents all of the male tame monsters.
 *                  Count: NUMMONS.
 *
 * female pets      Represents all of the female tame monsters.
 *                  Count: NUMMONS.
 *
 * invisible        Invisible monster placeholder.
 *                  Count: 1.
 *
 * detect (male)    Represents all detected male monsters.
 *                  Count: NUMMONS.
 *
 * detect (female)  Represents all detected female monsters.
 *                  Count: NUMMONS.
 *
 * corpse           One for each monster (male/female not differentiated).
 *                  Count: NUMMONS.
 *
 * ridden (male)    Represents all male monsters being ridden.
 *                  Count: NUMMONS
 *
 * ridden (female)  Represents all female monsters being ridden.
 *                  Count: NUMMONS
 *
 * object           One for each object.
 *                  Count: NUM_OBJECTS
 *
 * Stone            Stone
 *                  Count: 1
 *
 * main walls       level walls (main)
 *                  Count: (S_trwall - S_vwall) + 1 = 11
 *
 * mines walls      level walls (mines)
 *                  Count: (S_trwall - S_vwall) + 1 = 11
 *
 * gehennom walls   level walls (gehennom)
 *                  Count: (S_trwall - S_vwall) + 1 = 11
 *
 * knox walls       level walls (knox)
 *                  Count: (S_trwall - S_vwall) + 1 = 11
 *
 * sokoban walls    level walls (sokoban)
 *                  Count: (S_trwall - S_vwall) + 1 = 11
 *
 * cmap A           S_ndoor through S_brdnladder
 *                  Count: (S_brdnladder - S_ndoor) + 1 = 19
 *
 * Altars           Altar (unaligned, chaotic, neutral, lawful, other)
 *                  Count: 5
 *
 * cmap B           S_grave through S_arrow_trap + TRAPNUM - 1
 *                  Count: (S_arrow_trap + (TRAPNUM - 1) - S_grave) = 39
 *
 * zap beams        set of four (there are four directions) HI_ZAP.
 *                  Count: 4 * NUM_ZAP
 *
 * cmap C           S_digbeam through S_goodpos
 *                  Count: (S_goodpos - S_digbeam) + 1 = 10
 *
 * swallow          A set of eight for each monster.  The eight positions
 *                  represent those surrounding the hero.  The monster
 *                  number is shifted over 3 positions and the swallow
 *                  position is stored in the lower three bits.
 *                  Count: NUMMONS << 3
 *
 * dark explosions        A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * noxious explosions     A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * muddy explosions       A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * wet explosions         A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * magical explosions     A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * fiery explosions       A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * frosty explosions      A set of nine.
 *                        Count: MAXEXPCHAR
 *
 * warning                A set of six representing the different warning levels.
 *                        Count: 6
 *
 * statues (male)         One for each male monster.
 *                        Count: NUMMONS
 *
 * statues (female)       One for each female mo nster.
 *                        Count: NUMMONS
 *
 * objects piletop        Represents the top of a pile as well as
 *                        the object.
 *                        Count: NUM_OBJECTS
 *
 * bodies piletop         Represents the top of a pile as well as
 *                        the object, corpse in this case.
 *                        Count: NUMMONS
 *
 * male statues piletop   Represents the top of a pile as well as
 *                        the statue of a male monster.
 *                        Count: NUMMONS
 *
 * female statues piletop Represents the top of a pile as well as
 *                        the statue of a female monster.
 *                        Count: NUMMONS
 *
 * unexplored             One for unexplored areas of the map
 * nothing                Nothing but background
 *
 * The following are offsets used to convert to and from a glyph.
 */

enum glyph_offsets {
    GLYPH_MON_OFF = 0,
    GLYPH_MON_MALE_OFF = (GLYPH_MON_OFF),
    GLYPH_MON_FEM_OFF = (NUMMONS + GLYPH_MON_MALE_OFF),
    GLYPH_PET_OFF = (NUMMONS + GLYPH_MON_FEM_OFF),
    GLYPH_PET_MALE_OFF = (GLYPH_PET_OFF),
    GLYPH_PET_FEM_OFF = (NUMMONS + GLYPH_PET_MALE_OFF),
    GLYPH_INVIS_OFF = (NUMMONS + GLYPH_PET_FEM_OFF),
    GLYPH_DETECT_OFF = (1 + GLYPH_INVIS_OFF),
    GLYPH_DETECT_MALE_OFF = (GLYPH_DETECT_OFF),
    GLYPH_DETECT_FEM_OFF = (NUMMONS + GLYPH_DETECT_MALE_OFF),
    GLYPH_BODY_OFF = (NUMMONS + GLYPH_DETECT_FEM_OFF),
    GLYPH_RIDDEN_OFF = (NUMMONS + GLYPH_BODY_OFF),
    GLYPH_RIDDEN_MALE_OFF = (GLYPH_RIDDEN_OFF),
    GLYPH_RIDDEN_FEM_OFF = (NUMMONS + GLYPH_RIDDEN_MALE_OFF),
    GLYPH_OBJ_OFF = (NUMMONS + GLYPH_RIDDEN_FEM_OFF),
    GLYPH_CMAP_OFF = (NUM_OBJECTS + GLYPH_OBJ_OFF),
    GLYPH_CMAP_STONE_OFF = (GLYPH_CMAP_OFF),
    GLYPH_CMAP_MAIN_OFF = (1 + GLYPH_CMAP_STONE_OFF),
    GLYPH_CMAP_MINES_OFF = (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_MAIN_OFF),
    GLYPH_CMAP_GEH_OFF = (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_MINES_OFF),
    GLYPH_CMAP_KNOX_OFF = (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_GEH_OFF),
    GLYPH_CMAP_SOKO_OFF = (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_KNOX_OFF),
    GLYPH_CMAP_A_OFF = (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_SOKO_OFF),
    GLYPH_ALTAR_OFF = (((S_brdnladder - S_ndoor) + 1) + GLYPH_CMAP_A_OFF),
    GLYPH_CMAP_B_OFF = (5 + GLYPH_ALTAR_OFF),
    GLYPH_ZAP_OFF = ((S_arrow_trap + MAXTCHARS - S_grave) + GLYPH_CMAP_B_OFF),
    GLYPH_CMAP_C_OFF = ((NUM_ZAP << 2) + GLYPH_ZAP_OFF),
    GLYPH_SWALLOW_OFF = (((S_goodpos - S_digbeam) + 1) + GLYPH_CMAP_C_OFF),
    GLYPH_EXPLODE_OFF = ((NUMMONS << 3) + GLYPH_SWALLOW_OFF),
    GLYPH_EXPLODE_DARK_OFF = (GLYPH_EXPLODE_OFF),
    GLYPH_EXPLODE_NOXIOUS_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_DARK_OFF),
    GLYPH_EXPLODE_MUDDY_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_NOXIOUS_OFF),
    GLYPH_EXPLODE_WET_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_MUDDY_OFF),
    GLYPH_EXPLODE_MAGICAL_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_WET_OFF),
    GLYPH_EXPLODE_FIERY_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_MAGICAL_OFF),
    GLYPH_EXPLODE_FROSTY_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_FIERY_OFF),
    GLYPH_WARNING_OFF = (MAXEXPCHARS + GLYPH_EXPLODE_FROSTY_OFF),
    GLYPH_STATUE_OFF = (WARNCOUNT + GLYPH_WARNING_OFF),
    GLYPH_STATUE_MALE_OFF = (GLYPH_STATUE_OFF),
    GLYPH_STATUE_FEM_OFF = (NUMMONS + GLYPH_STATUE_MALE_OFF),
    GLYPH_PILETOP_OFF = (NUMMONS + GLYPH_STATUE_FEM_OFF),
    GLYPH_OBJ_PILETOP_OFF = (GLYPH_PILETOP_OFF),
    GLYPH_BODY_PILETOP_OFF = (NUM_OBJECTS + GLYPH_OBJ_PILETOP_OFF),
    GLYPH_STATUE_MALE_PILETOP_OFF = (NUMMONS + GLYPH_BODY_PILETOP_OFF),
    GLYPH_STATUE_FEM_PILETOP_OFF = (NUMMONS + GLYPH_STATUE_MALE_PILETOP_OFF),
    GLYPH_UNEXPLORED_OFF = (NUMMONS + GLYPH_STATUE_FEM_PILETOP_OFF),
    GLYPH_NOTHING_OFF = (GLYPH_UNEXPLORED_OFF + 1),
    MAX_GLYPH
};

#define NO_GLYPH          MAX_GLYPH
#define GLYPH_INVISIBLE   GLYPH_INVIS_OFF
#define GLYPH_UNEXPLORED  GLYPH_UNEXPLORED_OFF
#define GLYPH_NOTHING     GLYPH_NOTHING_OFF

#define warning_to_glyph(mwarnlev) ((mwarnlev) + GLYPH_WARNING_OFF)
#define mon_to_glyph(mon, rng) \
    ((int) what_mon(monsndx((mon)->data), rng)                          \
     + (((mon)->female == 0) ?  GLYPH_MON_MALE_OFF : GLYPH_MON_FEM_OFF))
#define detected_mon_to_glyph(mon, rng) \
    ((int) what_mon(monsndx((mon)->data), rng)                          \
     + (((mon)->female == 0) ?  GLYPH_DETECT_MALE_OFF : GLYPH_DETECT_FEM_OFF))
#define ridden_mon_to_glyph(mon, rng) \
    ((int) what_mon(monsndx((mon)->data), rng)                          \
     + (((mon)->female == 0) ?  GLYPH_RIDDEN_MALE_OFF : GLYPH_RIDDEN_FEM_OFF))
#define pet_to_glyph(mon, rng) \
    ((int) what_mon(monsndx((mon)->data), rng)                          \
     + (((mon)->female == 0) ? GLYPH_PET_MALE_OFF : GLYPH_PET_FEM_OFF))

/* treat unaligned as the default instead of explicitly checking for it;
   altar alignment uses 3 bits with 4 defined values and 4 unused ones */
#define altar_to_glyph(amsk) \
    ((((amsk) & AM_SANCTUM) == AM_SANCTUM)                \
      ? (GLYPH_ALTAR_OFF + altar_other)                   \
      : (((amsk) & AM_MASK) == AM_LAWFUL)                 \
         ? (GLYPH_ALTAR_OFF + altar_lawful)               \
         : (((amsk) & AM_MASK) == AM_NEUTRAL)             \
            ? (GLYPH_ALTAR_OFF + altar_neutral)           \
            : (((amsk) & AM_MASK) == AM_CHAOTIC)          \
               ? (GLYPH_ALTAR_OFF + altar_chaotic)        \
               /* (((amsk) & AM_MASK) == AM_UNALIGNED) */ \
               : (GLYPH_ALTAR_OFF + altar_unaligned))

/* not used, nor is it correct
#define zap_to_glyph(zaptype, cmap_idx) \
    ((((cmap_idx) - S_vbeam) + 1) + GLYPH_ZAP_OFF)
*/

/* EXPL_FIERY is the default explosion type */
#define explosion_to_glyph(expltyp, idx) \
    ((idx) - S_expl_tl                                                  \
     + (((expltyp) == EXPL_FROSTY) ? GLYPH_EXPLODE_FROSTY_OFF           \
        : ((expltyp) == EXPL_MAGICAL) ? GLYPH_EXPLODE_MAGICAL_OFF       \
          : ((expltyp) == EXPL_WET) ? GLYPH_EXPLODE_WET_OFF             \
           : ((expltyp) == EXPL_MUDDY) ? GLYPH_EXPLODE_MUDDY_OFF        \
             : ((expltyp) == EXPL_NOXIOUS) ? GLYPH_EXPLODE_NOXIOUS_OFF  \
               : GLYPH_EXPLODE_FIERY_OFF))

/* cmap_walls_to_glyph(): return the glyph number for specified wall
   symbol; result varies by dungeon branch */
#define cmap_walls_to_glyph(cmap_idx) \
    ((cmap_idx) - S_vwall                               \
     + (In_mines(&u.uz) ? GLYPH_CMAP_MINES_OFF          \
        : In_hell(&u.uz) ? GLYPH_CMAP_GEH_OFF           \
          : Is_knox(&u.uz) ? GLYPH_CMAP_KNOX_OFF        \
            : In_sokoban(&u.uz) ? GLYPH_CMAP_SOKO_OFF   \
              : GLYPH_CMAP_MAIN_OFF))

/* cmap_D0walls_to_glyph(): simpler version of cmap_walls_to_glyph()
   which returns the glyph that would be used in the main dungeon,
   regardless of hero's current location */
#define cmap_D0walls_to_glyph(cmap_idx) \
    ((cmap_idx) - S_vwall + GLYPH_CMAP_MAIN_OFF)

#define cmap_a_to_glyph(cmap_idx) \
    (((cmap_idx) - S_ndoor) + GLYPH_CMAP_A_OFF)

#define cmap_b_to_glyph(cmap_idx) \
    (((cmap_idx) - S_grave) + GLYPH_CMAP_B_OFF)

#define cmap_c_to_glyph(cmap_idx) \
    (((cmap_idx) - S_digbeam) + GLYPH_CMAP_C_OFF)

#define cmap_to_glyph(cmap_idx) \
    ( ((cmap_idx) == S_stone)   ? GLYPH_CMAP_STONE_OFF                      \
    : ((cmap_idx) <= S_trwall)  ? cmap_walls_to_glyph(cmap_idx)             \
    : ((cmap_idx) <  S_altar)   ? cmap_a_to_glyph(cmap_idx)                 \
    : ((cmap_idx) == S_altar)   ? altar_to_glyph(AM_NEUTRAL)                \
    : ((cmap_idx) <  S_arrow_trap + MAXTCHARS) ? cmap_b_to_glyph(cmap_idx)  \
    : ((cmap_idx) <= S_goodpos) ? cmap_c_to_glyph(cmap_idx)                 \
      : NO_GLYPH )

#define trap_to_glyph(trap)                                \
    cmap_to_glyph(trap_to_defsym(((int) (trap)->ttyp)))

/* Not affected by hallucination.  Gives a generic body for CORPSE */
/* MRKR: ...and the generic statue */
#define objnum_to_glyph(onum) ((int) (onum) + GLYPH_OBJ_OFF)
#define monnum_to_glyph(mnum,gnd) \
    ((int) (mnum) + (((gnd) == MALE) ? GLYPH_MON_MALE_OFF       \
                                     : GLYPH_MON_FEM_OFF))
#define detected_monnum_to_glyph(mnum,gnd) \
    ((int) (mnum) + (((gnd) == MALE) ? GLYPH_DETECT_MALE_OFF    \
                                     : GLYPH_DETECT_FEM_OFF))
#define ridden_monnum_to_glyph(mnum,gnd) \
    ((int) (mnum) + (((gnd) == MALE) ? GLYPH_RIDDEN_MALE_OFF    \
                                     : GLYPH_RIDDEN_FEM_OFF))
#define petnum_to_glyph(mnum,gnd) \
    ((int) (mnum) + (((gnd) == MALE) ? GLYPH_PET_MALE_OFF       \
                                     : GLYPH_PET_FEM_OFF))

/* The hero's glyph when seen as a monster.
 */
#define hero_glyph \
    monnum_to_glyph((Upolyd || !flags.showrace) ? u.umonnum : gu.urace.mnum, \
                    (Ugender))

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

#define glyph_to_trap(glyph) \
    (glyph_is_trap(glyph)                                                   \
        ? ((int) defsym_to_trap(((glyph) - GLYPH_TRAP_OFF) + S_arrow_trap)) \
        : NO_GLYPH)

#define glyph_is_cmap_main(glyph) \
    ((glyph) >= GLYPH_CMAP_MAIN_OFF                                     \
     && (glyph) < (((S_trwall - S_vwall) +1) + GLYPH_CMAP_MAIN_OFF))
#define glyph_is_cmap_mines(glyph) \
    ((glyph) >= GLYPH_CMAP_MINES_OFF                                    \
     && (glyph) < (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_MINES_OFF))
#define glyph_is_cmap_gehennom(glyph) \
    ((glyph) >= GLYPH_CMAP_GEH_OFF                                      \
     && (glyph) < (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_GEH_OFF))
#define glyph_is_cmap_knox(glyph) \
    ((glyph) >= GLYPH_CMAP_KNOX_OFF                                     \
     && (glyph) < (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_KNOX_OFF))
#define glyph_is_cmap_sokoban(glyph) \
    ((glyph) >= GLYPH_CMAP_SOKO_OFF                                     \
     && (glyph) < (((S_trwall - S_vwall) + 1) + GLYPH_CMAP_SOKO_OFF))
#define glyph_is_cmap_a(glyph) \
    ((glyph) >= GLYPH_CMAP_A_OFF                                        \
     && (glyph) < (((S_brdnladder - S_ndoor) + 1) + GLYPH_CMAP_A_OFF))
#define glyph_is_cmap_altar(glyph) \
    ((glyph) >= GLYPH_ALTAR_OFF && (glyph) < (5 + GLYPH_ALTAR_OFF))
#define glyph_is_cmap_b(glyph) \
    ((glyph) >= GLYPH_CMAP_B_OFF                                        \
     && ((glyph) < ((S_arrow_trap + MAXTCHARS - S_grave) + GLYPH_CMAP_B_OFF)))
#define glyph_is_cmap_zap(glyph) \
    ((glyph) >= GLYPH_ZAP_OFF && (glyph) < ((NUM_ZAP << 2) + GLYPH_ZAP_OFF))
#define glyph_is_cmap_c(glyph) \
    ((glyph) >= GLYPH_CMAP_C_OFF                                        \
     && (glyph) < (((S_goodpos - S_digbeam) + 1) + GLYPH_CMAP_C_OFF))
#define glyph_is_swallow(glyph) \
    ((glyph) >= GLYPH_SWALLOW_OFF                                       \
     && (glyph) < (((NUMMONS << 3) + GLYPH_SWALLOW_OFF)))
#define glyph_is_explosion(glyph) \
    ((glyph) >= GLYPH_EXPLODE_OFF                                       \
     && (glyph) < (MAXEXPCHARS + GLYPH_EXPLODE_FROSTY_OFF))
#if 0   /* this is more precise but expands to a lot of unnecessary code */
#define glyph_is_cmap(glyph) \
    (((glyph) == GLYPH_CMAP_STONE_OFF) \
     || glyph_is_cmap_main(glyph)      \
     || glyph_is_cmap_mines(glyph)     \
     || glyph_is_cmap_gehennom(glyph)  \
     || glyph_is_cmap_knox(glyph)      \
     || glyph_is_cmap_sokoban(glyph)   \
     || glyph_is_cmap_a(glyph)         \
     || glyph_is_cmap_altar(glyph)     \
     || glyph_is_cmap_b(glyph)         \
     || glyph_is_cmap_c(glyph))
#endif
#define glyph_is_cmap(glyph) \
    ((glyph) >= GLYPH_CMAP_STONE_OFF \
     && (glyph) < (GLYPH_CMAP_C_OFF + ((S_goodpos - S_digbeam) + 1)))

/* final MAXPCHARS is legal array index because of trailing fencepost entry */
#define glyph_to_cmap(glyph) \
    (((glyph) == GLYPH_CMAP_STONE_OFF)                                  \
      ? S_stone                                                         \
      : glyph_is_cmap_main(glyph)                                       \
        ? (((glyph) - GLYPH_CMAP_MAIN_OFF) + S_vwall)                   \
        : glyph_is_cmap_mines(glyph)                                    \
          ? (((glyph) - GLYPH_CMAP_MINES_OFF) + S_vwall)                \
          : glyph_is_cmap_gehennom(glyph)                               \
            ? (((glyph) - GLYPH_CMAP_GEH_OFF) + S_vwall)                \
            : glyph_is_cmap_knox(glyph)                                 \
              ? (((glyph) - GLYPH_CMAP_KNOX_OFF) + S_vwall)             \
              : glyph_is_cmap_sokoban(glyph)                            \
                ? (((glyph) - GLYPH_CMAP_SOKO_OFF) + S_vwall)           \
                : glyph_is_cmap_a(glyph)                                \
                  ? (((glyph) - GLYPH_CMAP_A_OFF) + S_ndoor)            \
                  : glyph_is_cmap_altar(glyph)                          \
                    ? (S_altar)                                         \
                    : glyph_is_cmap_b(glyph)                            \
                      ? (((glyph) - GLYPH_CMAP_B_OFF) + S_grave)        \
                      : glyph_is_cmap_c(glyph)                          \
                        ? (((glyph) - GLYPH_CMAP_C_OFF) + S_digbeam)    \
                        : glyph_is_cmap_zap(glyph)                      \
                          ? ((((glyph) - GLYPH_ZAP_OFF) % 4) + S_vbeam) \
                          : MAXPCHARS)

#define glyph_to_swallow(glyph) \
    (glyph_is_swallow(glyph) ? (((glyph) - GLYPH_SWALLOW_OFF) & 0x7) : 0)
#define glyph_to_warning(glyph) \
    (glyph_is_warning(glyph) ? ((glyph) - GLYPH_WARNING_OFF) : NO_GLYPH)

/*
 * Return true if the given glyph is what we want.  Note that bodies are
 * considered objects.
 */
#define glyph_is_normal_male_monster(glyph) \
    ((glyph) >= GLYPH_MON_MALE_OFF && (glyph) < (GLYPH_MON_MALE_OFF + NUMMONS))
#define glyph_is_normal_female_monster(glyph) \
    ((glyph) >= GLYPH_MON_FEM_OFF && (glyph) < (GLYPH_MON_FEM_OFF + NUMMONS))
#define glyph_is_normal_monster(glyph) \
    (glyph_is_normal_male_monster(glyph) || glyph_is_normal_female_monster(glyph))
#define glyph_is_female_pet(glyph) \
    ((glyph) >= GLYPH_PET_FEM_OFF && (glyph) < (GLYPH_PET_FEM_OFF + NUMMONS))
#define glyph_is_male_pet(glyph) \
    ((glyph) >= GLYPH_PET_MALE_OFF && (glyph) < (GLYPH_PET_MALE_OFF + NUMMONS))
#define glyph_is_pet(glyph) \
    (glyph_is_male_pet(glyph) || glyph_is_female_pet(glyph))
#define glyph_is_ridden_female_monster(glyph) \
    ((glyph) >= GLYPH_RIDDEN_FEM_OFF                    \
     && (glyph) < (GLYPH_RIDDEN_FEM_OFF + NUMMONS))
#define glyph_is_ridden_male_monster(glyph) \
    ((glyph) >= GLYPH_RIDDEN_MALE_OFF                   \
     && (glyph) < (GLYPH_RIDDEN_MALE_OFF + NUMMONS))
#define glyph_is_ridden_monster(glyph) \
    (glyph_is_ridden_male_monster(glyph) \
        || glyph_is_ridden_female_monster(glyph))
#define glyph_is_detected_female_monster(glyph) \
    ((glyph) >= GLYPH_DETECT_FEM_OFF                    \
     && (glyph) < (GLYPH_DETECT_FEM_OFF + NUMMONS))
#define glyph_is_detected_male_monster(glyph) \
    ((glyph) >= GLYPH_DETECT_MALE_OFF                   \
     && (glyph) < (GLYPH_DETECT_MALE_OFF + NUMMONS))
#define glyph_is_detected_monster(glyph) \
    (glyph_is_detected_male_monster(glyph) \
        || glyph_is_detected_female_monster(glyph))
#define glyph_is_monster(glyph)                            \
    (glyph_is_normal_monster(glyph) || glyph_is_pet(glyph) \
     || glyph_is_ridden_monster(glyph) || glyph_is_detected_monster(glyph))
#define glyph_is_invisible(glyph) ((glyph) == GLYPH_INVISIBLE)

/* final NUMMONS is legal array index because of trailing fencepost entry */
#define glyph_to_mon(glyph) \
       (glyph_is_normal_female_monster(glyph)                  \
         ? ((glyph) - GLYPH_MON_FEM_OFF)                       \
         : glyph_is_normal_male_monster(glyph)                 \
           ? ((glyph) - GLYPH_MON_MALE_OFF)                    \
           : glyph_is_female_pet(glyph)                        \
             ? ((glyph) - GLYPH_PET_FEM_OFF)                   \
             : glyph_is_male_pet(glyph)                        \
               ? ((glyph) - GLYPH_PET_MALE_OFF)                \
               : glyph_is_detected_female_monster(glyph)       \
                 ? ((glyph) - GLYPH_DETECT_FEM_OFF)            \
                 : glyph_is_detected_male_monster(glyph)       \
                   ? ((glyph) - GLYPH_DETECT_MALE_OFF)         \
                   : glyph_is_ridden_female_monster(glyph)     \
                     ? ((glyph) - GLYPH_RIDDEN_FEM_OFF)        \
                     : glyph_is_ridden_male_monster(glyph)     \
                       ? ((glyph) - GLYPH_RIDDEN_MALE_OFF)     \
                       : NUMMONS)

#define obj_is_piletop(obj) \
    ((obj)->where == OBJ_FLOOR                                  \
        /*&& gl.level.objects[(obj)->ox][(obj)->oy]*/            \
        && gl.level.objects[(obj)->ox][(obj)->oy]->nexthere)

#define glyph_is_body_piletop(glyph) \
    (((glyph) >= GLYPH_BODY_PILETOP_OFF)                        \
     && ((glyph) < (GLYPH_BODY_PILETOP_OFF + NUMMONS)))
#define glyph_is_body(glyph) \
    ((((glyph) >= GLYPH_BODY_OFF) && ((glyph) < (GLYPH_BODY_OFF + NUMMONS))) \
     || glyph_is_body_piletop(glyph))

#define glyph_is_fem_statue_piletop(glyph) \
    (((glyph) >= GLYPH_STATUE_FEM_PILETOP_OFF)                  \
      && ((glyph) < (GLYPH_STATUE_FEM_PILETOP_OFF + NUMMONS)))
#define glyph_is_male_statue_piletop(glyph) \
    (((glyph) >= GLYPH_STATUE_MALE_PILETOP_OFF)                 \
         && ((glyph) < (GLYPH_STATUE_MALE_PILETOP_OFF + NUMMONS)))
#define glyph_is_fem_statue(glyph) \
    ((((glyph) >= GLYPH_STATUE_FEM_OFF)                         \
      && ((glyph) < (GLYPH_STATUE_FEM_OFF + NUMMONS)))          \
     || glyph_is_fem_statue_piletop(glyph))
#define glyph_is_male_statue(glyph) \
    ((((glyph) >= GLYPH_STATUE_MALE_OFF)                        \
      && ((glyph) < (GLYPH_STATUE_MALE_OFF + NUMMONS)))         \
     || glyph_is_male_statue_piletop(glyph))
#define glyph_is_statue(glyph) \
    (glyph_is_male_statue(glyph) || glyph_is_fem_statue(glyph))
#define glyph_is_normal_piletop_obj(glyph) \
    (((glyph) >= GLYPH_OBJ_PILETOP_OFF)                         \
     && ((glyph) < (GLYPH_OBJ_PILETOP_OFF + NUM_OBJECTS)))
#define glyph_is_normal_object(glyph) \
    ((((glyph) >= GLYPH_OBJ_OFF)                                \
      && ((glyph) < (GLYPH_OBJ_OFF + NUM_OBJECTS)))             \
     || glyph_is_normal_piletop_obj(glyph))

#if 0
#define glyph_is_object(glyph) \
  ((((glyph) >= GLYPH_OBJ_OFF) && ((glyph) < (GLYPH_OBJ_OFF + NUM_OBJECTS))) \
   || (((glyph) >= GLYPH_OBJ_PILETOP_OFF)                               \
       && ((glyph) < (GLYPH_OBJ_PILETOP_OFF + NUM_OBJECTS)))            \
   || (((glyph) >= GLYPH_STATUE_MALE_OFF)                               \
       && ((glyph) < (GLYPH_STATUE_MALE_OFF + NUMMONS)))                \
   || (((glyph) >= GLYPH_STATUE_MALE_PILETOP_OFF)                       \
       && ((glyph) < (GLYPH_STATUE_MALE_PILETOP_OFF + NUMMONS)))        \
   || (((glyph) >= GLYPH_STATUE_FEM_OFF)                                \
       && ((glyph) < (GLYPH_STATUE_FEM_OFF + NUMMONS)))                 \
   || (((glyph) >= GLYPH_STATUE_FEM_PILETOP_OFF)                        \
       && ((glyph) < (GLYPH_STATUE_FEM_PILETOP_OFF + NUMMONS)))         \
   || (((glyph) >= GLYPH_BODY_OFF)                                      \
       && ((glyph) < (GLYPH_BODY_OFF + NUMMONS)))                       \
   || (((glyph) >= GLYPH_BODY_PILETOP_OFF)                              \
       && ((glyph) < (GLYPH_BODY_PILETOP_OFF + NUMMONS))))
#endif
#define glyph_is_object(glyph) \
    (glyph_is_normal_object(glyph) || glyph_is_statue(glyph)    \
     || glyph_is_body(glyph))

/* briefly used for Qt's "paper doll" inventory which shows map tiles for
   equipped objects; those vary like floor items during hallucination now
   so this isn't used anywhere */
#define obj_to_true_glyph(obj)                                  \
    (((obj)->otyp == STATUE)                                    \
     ? ((int) (obj)->corpsenm                                   \
        + (((obj)->spe & CORPSTAT_GENDER) == CORPSTAT_FEMALE)   \
             ? (obj_is_piletop(obj)                             \
                ? (GLYPH_STATUE_FEM_PILETOP_OFF)                \
                : (GLYPH_STATUE_FEM_OFF))                       \
             : (obj_is_piletop(obj)                             \
                ? (GLYPH_STATUE_MALE_PILETOP_OFF)               \
                : (GLYPH_STATUE_MALE_OFF))                      \
     : (((obj)->otyp == CORPSE)                                 \
         ? ((int) (obj)->corpsenm                               \
            + (obj_is_piletop(obj)                              \
                ? (GLYPH_BODY_PILETOP_OFF)                      \
                ? (GLYPH_BODY_OFF)))                            \
         : ((int) (obj)->otyp + GLYPH_OBJ_OFF))))

/* final NUM_OBJECTS is legal array idx because of trailing fencepost entry */
#define glyph_to_obj(glyph) \
    (glyph_is_body(glyph) ? CORPSE                              \
     : glyph_is_statue(glyph) ? STATUE                          \
       : glyph_is_normal_object(glyph)                          \
           ? ((glyph) - (glyph_is_normal_piletop_obj(glyph)     \
                           ? GLYPH_OBJ_PILETOP_OFF              \
                           : GLYPH_OBJ_OFF))                    \
           : NUM_OBJECTS)

#define glyph_to_body_corpsenm(glyph) \
    (glyph_is_body_piletop(glyph)                       \
       ? ((glyph) - GLYPH_BODY_PILETOP_OFF)             \
       : ((glyph) - GLYPH_BODY_OFF))

#define glyph_to_statue_corpsenm(glyph) \
    (glyph_is_fem_statue_piletop(glyph)                 \
       ? ((glyph) - GLYPH_STATUE_FEM_PILETOP_OFF)       \
       : glyph_is_male_statue_piletop(glyph)            \
           ? ((glyph) - GLYPH_STATUE_MALE_PILETOP_OFF)  \
           : glyph_is_fem_statue(glyph)                 \
               ? ((glyph) - GLYPH_STATUE_FEM_OFF)       \
               : glyph_is_male_statue(glyph)            \
                   ? ((glyph) - GLYPH_STATUE_MALE_OFF)  \
                   : NO_GLYPH)

/* These have the unfortunate side effect of needing a global variable  */
/* to store a result. 'otg_temp' is defined and declared in decl.{ch}.  */
#define random_obj_to_glyph(rng) \
    ((go.otg_temp = random_object(rng)) == CORPSE        \
         ? (random_monster(rng) + GLYPH_BODY_OFF)       \
         : (go.otg_temp + GLYPH_OBJ_OFF))
#define corpse_to_glyph(obj) \
    ((int) ((obj)->corpsenm + (obj_is_piletop(obj)        \
                                ? GLYPH_BODY_PILETOP_OFF  \
                                : GLYPH_BODY_OFF)))

/* Paraphrased rationale from the original commit for the boulder
   exception: If a boulder is the topmost item on a pile, then it is
   not mapped to a piletop glyph; mainly because boulders are "solid";
   boulders dropped by monsters are nearly always over other objects;
   also done so that special levels such a Sokoban can "hide" items
   under the boulders. */
#define normal_obj_to_glyph(obj) \
    ((int) ((obj)->otyp + ((obj_is_piletop(obj) && ((obj)->otyp != BOULDER)) \
                           ? GLYPH_OBJ_PILETOP_OFF                           \
                           : GLYPH_OBJ_OFF)))

/* MRKR: Statues now have glyphs corresponding to the monster they    */
/*       represent and look like monsters when you are hallucinating. */

#define statue_to_glyph(obj, rng) \
    ((Hallucination)                                                \
     ? ((random_monster(rng))                                       \
        + ((!(rng)(2)) ? GLYPH_MON_MALE_OFF : GLYPH_MON_FEM_OFF))   \
     : ((int) (obj)->corpsenm                                       \
        + ((((obj)->spe & CORPSTAT_GENDER) == CORPSTAT_FEMALE)      \
           ? (obj_is_piletop(obj)                                   \
              ? GLYPH_STATUE_FEM_PILETOP_OFF                        \
              : GLYPH_STATUE_FEM_OFF)                               \
           : (obj_is_piletop(obj)                                   \
              ? GLYPH_STATUE_MALE_PILETOP_OFF                       \
              : GLYPH_STATUE_MALE_OFF))))

#define obj_to_glyph(obj, rng) \
    (((obj)->otyp == STATUE)                             \
        ? statue_to_glyph(obj, rng)                      \
        : ((Hallucination)                               \
             ? random_obj_to_glyph(rng)                  \
             : (((obj)->otyp == CORPSE)                  \
                  ? corpse_to_glyph(obj)                 \
                  : normal_obj_to_glyph(obj))))

#define GLYPH_TRAP_OFF \
    (GLYPH_CMAP_B_OFF + (S_arrow_trap - S_grave))
#define glyph_is_trap(glyph) \
    ((glyph) >= (GLYPH_TRAP_OFF)                        \
     && (glyph) < ((GLYPH_TRAP_OFF) + MAXTCHARS))
#define glyph_is_warning(glyph) \
    ((glyph) >= GLYPH_WARNING_OFF                       \
     && (glyph) < (GLYPH_WARNING_OFF + WARNCOUNT))
#define glyph_is_unexplored(glyph) ((glyph) == GLYPH_UNEXPLORED)
#define glyph_is_nothing(glyph) ((glyph) == GLYPH_NOTHING)

#if 0
#define glyph_is_piletop(glyph) \
    (glyph_is_body_piletop(glyph) || glyph_is_statue_piletop(glyph) \
        || glyph_is_obj_piletop(glyph))
#endif

/* mgflags for altering map_glyphinfo() internal behavior */
#define MG_FLAG_NORMAL     0x00
#define MG_FLAG_NOOVERRIDE 0x01 /* disregard accessibility override values */

/* Special mapped glyphflags encoded by reset_glyphmap() + map_glyphinfo() */
#define MG_HERO    0x00001  /* represents the hero */
#define MG_CORPSE  0x00002  /* represents a body */
#define MG_INVIS   0x00004  /* represents invisible monster */
#define MG_DETECT  0x00008  /* represents a detected monster */
#define MG_PET     0x00010  /* represents a pet */
#define MG_RIDDEN  0x00020  /* represents a ridden monster */
#define MG_STATUE  0x00040  /* represents a statue */
#define MG_OBJPILE 0x00080  /* more than one stack of objects */
#define MG_BW_LAVA 0x00100  /* 'black & white lava': highlight lava if it
                             * can't be distinguished from water by color */
#define MG_BW_ICE  0x00200  /* similar for ice vs floor */
#define MG_BW_SINK 0x00200  /* identical for sink vs fountain [note: someday
                             * this may become a distinct flag */
#define MG_NOTHING 0x00400  /* char represents GLYPH_NOTHING */
#define MG_UNEXPL  0x00800  /* char represents GLYPH_UNEXPLORED */
#define MG_MALE    0x01000  /* represents a male mon or statue of one */
#define MG_FEMALE  0x02000  /* represents a female mon or statue of one */
#define MG_BADXY   0x04000  /* bad coordinates were passed */

typedef struct {
    xint8 gnew; /* perhaps move this bit into the rm structure. */
    glyph_info glyphinfo;
} gbuf_entry;

#ifdef TEXTCOLOR
extern const int altarcolors[];
extern const int zapcolors[];
extern const int explodecolors[];
extern int wallcolors[];
#endif

/* If USE_TILES is defined during build, this comes from the generated
 * tile.c, complete with appropriate tile references in the initialization.
 * Otherwise, it comes from display.c.
 */
extern const glyph_info nul_glyphinfo;

#endif /* DISPLAY_H */

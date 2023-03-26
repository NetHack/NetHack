/* NetHack 3.7	display.c	$NHDT-Date: 1672561294 2023/01/01 08:21:34 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.200 $ */
/* Copyright (c) Dean Luick, with acknowledgements to Kevin Darcy */
/* and Dave Cohrs, 1990.                                          */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *                      THE NEW DISPLAY CODE
 *
 * The old display code has been broken up into three parts: vision, display,
 * and drawing.  Vision decides what locations can and cannot be physically
 * seen by the hero.  Display decides _what_ is displayed at a given location.
 * Drawing decides _how_ to draw a monster, fountain, sword, etc.
 *
 * The display system uses information from the vision system to decide
 * what to draw at a given location.  The routines for the vision system
 * can be found in vision.c and vision.h.  The routines for display can
 * be found in this file (display.c) and display.h.  The drawing routines
 * are part of the window port.  See doc/window.txt for the drawing
 * interface.
 *
 * The display system deals with an abstraction called a glyph.  Anything
 * that could possibly be displayed has a unique glyph identifier.
 *
 * What is seen on the screen is a combination of what the hero remembers
 * and what the hero currently sees.  Objects and dungeon features (walls
 * doors, etc) are remembered when out of sight.  Monsters and temporary
 * effects are not remembered.  Each location on the level has an
 * associated glyph.  This is the hero's _memory_ of what he or she has
 * seen there before.
 *
 * Display rules:
 *
 *      If the location is in sight, display in order:
 *              visible (or sensed) monsters
 *              visible objects
 *              known traps
 *              background
 *
 *      If the location is out of sight, display in order:
 *              sensed monsters (via telepathy or persistent detection)
 *              warning (partly-sensed monster shown as an abstraction)
 *              memory
 *
 *      "Remembered, unseen monster" is handled like an object rather
 *      than a monster, and stays displayed whether or not it is in sight.
 *      It is removed when a visible or sensed or warned-of monster gets
 *      shown at its location or when searching or fighting reveals that
 *      no monster is there.
 *
 *
 * Here is a list of the major routines in this file to be used externally:
 *
 * newsym
 *
 * Possibly update the screen location (x,y).  This is the workhorse routine.
 * It is always correct --- where correct means following the in-sight/out-
 * of-sight rules.  **Most of the code should use this routine.**  This
 * routine updates the map and displays monsters.
 *
 *
 * map_background
 * map_object
 * map_trap or map_engraving
 * map_invisible
 * unmap_object
 *
 * If you absolutely must override the in-sight/out-of-sight rules, there
 * are two possibilities.  First, you can mess with vision to force the
 * location in sight then use newsym(), or you can  use the map_* routines.
 * The first has not been tried [no need] and the second is used in the
 * detect routines --- detect object, magic mapping, etc.  The map_*
 * routines *change* what the hero remembers.  All changes made by these
 * routines will be sticky --- they will survive screen redraws.  Do *not*
 * use these for things that only temporarily change the screen.  These
 * routines are also used directly by newsym().  unmap_object is used to
 * clear a remembered object when/if detection reveals it isn't there.
 *
 *
 * show_glyph
 *
 * This is direct (no processing in between) buffered access to the screen.
 * Temporary screen effects are run through this and its companion,
 * flush_screen().  There is yet a lower level routine, print_glyph(),
 * but this is unbuffered and graphic dependent (i.e. it must be surrounded
 * by graphic set-up and tear-down routines).  Do not use print_glyph().
 *
 *
 * see_monsters
 * see_objects
 * see_traps
 *
 * These are only used when something affects all of the monsters or
 * objects or traps.  For objects and traps, the only thing is hallucination.
 * For monsters, there are hallucination and changing from/to blindness, etc.
 *
 *
 * tmp_at
 *
 * This is a useful interface for displaying temporary items on the screen.
 * Its interface is different than previously, so look at it carefully.
 *
 *
 *
 * Parts of the rm structure that are used:
 *
 *      typ     - What is really there.
 *      glyph   - What the hero remembers.  This will never be a monster.
 *                Monsters "float" above this.
 *      lit     - True if the position is lit.  An optimization for
 *                lit/unlit rooms.
 *      waslit  - True if the position was *remembered* as lit.
 *      seenv   - A vector of bits representing the directions from which the
 *                hero has seen this position.  The vector's primary use is
 *                determining how walls are seen.  E.g. a wall sometimes looks
 *                like stone on one side, but is seen as wall from the other.
 *                Other uses are for unmapping detected objects and felt
 *                locations, where we need to know if the hero has ever
 *                seen the location.
 *      flags   - Additional information for the typ field.  Different for
 *                each typ.
 *      horizontal - Indicates whether the wall or door is horizontal or
 *                vertical.
 */
#include "hack.h"

static void show_mon_or_warn(coordxy, coordxy, int);
static void display_monster(coordxy, coordxy, struct monst *, int, boolean);
static int swallow_to_glyph(int, int);
static void display_warning(struct monst *);

static int check_pos(coordxy, coordxy, int);
static void get_bkglyph_and_framecolor(coordxy x, coordxy y, int *, uint32 *);
static int tether_glyph(coordxy, coordxy);
static void mimic_light_blocking(struct monst *);
#ifdef UNBUFFERED_GLYPHINFO
static glyph_info *glyphinfo_at(coordxy, coordxy, int);
#endif

/*#define WA_VERBOSE*/ /* give (x,y) locations for all "bad" spots */
#ifdef WA_VERBOSE
static boolean more_than_one(coordxy, coordxy, coordxy, coordxy, coordxy);
#endif

static int set_twall(coordxy, coordxy, coordxy, coordxy,
                     coordxy, coordxy, coordxy, coordxy);
static int set_wall(coordxy, coordxy, int);
static int set_corn(coordxy, coordxy, coordxy, coordxy,
                    coordxy, coordxy, coordxy, coordxy);
static int set_crosswall(coordxy, coordxy);
static void set_seenv(struct rm *, coordxy, coordxy, coordxy, coordxy);
static void t_warn(struct rm *);
static int wall_angle(struct rm *);

#define remember_topology(x, y) (gl.lastseentyp[x][y] = levl[x][y].typ)

/*
 *      See display.h for descriptions of tp_sensemon() through
 *      is_safemon().  Some of these were generating an awful lot of
 *      code "behind the curtain", particularly canspotmon() (which is
 *      still a macro but one that now expands to a pair of function
 *      calls rather than to a ton of special case checks).  Return
 *      values are all int 0 or 1, not boolean.
 *
 *      They're still implemented as macros within this file.
 */
int
tp_sensemon(struct monst *mon)
{
    return _tp_sensemon(mon);
}
#define tp_sensemon(mon) _tp_sensemon(mon)

int
sensemon(struct monst *mon)
{
    return _sensemon(mon);
}
#define sensemon(mon) _sensemon(mon)

int
mon_warning(struct monst *mon)
{
    return _mon_warning(mon);
}
#define mon_warning(mon) _mon_warning(mon)

int
mon_visible(struct monst *mon)
{
    return _mon_visible(mon);
}
#define mon_visible(mon) _mon_visible(mon)

int
see_with_infrared(struct monst *mon)
{
    return _see_with_infrared(mon);
}
#define see_with_infrared(mon) _see_with_infrared(mon)

int
canseemon(struct monst *mon)
{
    return _canseemon(mon);
}
#define canseemon(mon) _canseemon(mon)

int
knowninvisible(struct monst *mon)
{
    return _knowninvisible(mon);
}
/* #define knowninvisible() isn't useful here */

int
is_safemon(struct monst *mon)
{
    return _is_safemon(mon);
}
/* #define is_safemon() isn't useful here */

/*
 *      End of former macro-only vision related (mostly) routines
 *      converted to functions.
 */

/*
 * magic_map_background()
 *
 * This function is similar to map_background (see below) except we pay
 * attention to and correct unexplored, lit ROOM and CORR spots.
 */
void
magic_map_background(coordxy x, coordxy y, int show)
{
    int glyph = back_to_glyph(x, y); /* assumes hero can see x,y */
    struct rm *lev = &levl[x][y];

    /*
     * Correct for out of sight lit corridors and rooms that the hero
     * doesn't remember as lit.
     */
    if (!cansee(x, y) && !lev->waslit) {
        /* Floor spaces are dark if unlit.  Corridors are dark if unlit. */
        if (lev->typ == ROOM && glyph == cmap_to_glyph(S_room))
            glyph = (flags.dark_room && iflags.use_color)
                        ? cmap_to_glyph(DARKROOMSYM)
                        : GLYPH_NOTHING;
        else if (lev->typ == CORR && glyph == cmap_to_glyph(S_litcorr))
            glyph = cmap_to_glyph(S_corr);
    }
    if (gl.level.flags.hero_memory)
        lev->glyph = glyph;
    if (show)
        show_glyph(x, y, glyph);

    remember_topology(x, y);
}

/*
 * The routines map_background(), map_object(), and map_trap() could just
 * as easily be:
 *
 *      map_glyph(x,y,glyph,show)
 *
 * Which is called with the xx_to_glyph() in the call.  Then I can get
 * rid of 3 routines that don't do very much anyway.  And then stop
 * having to create fake objects and traps.  However, I am reluctant to
 * make this change.
 */

/*
 * map_background()
 *
 * Make the real background part of our map.  This routine assumes that
 * the hero can physically see the location.  Update the screen if directed.
 */
void
map_background(register coordxy x, register coordxy y, register int show)
{
    register int glyph = back_to_glyph(x, y);

    if (gl.level.flags.hero_memory)
        levl[x][y].glyph = glyph;
    if (show)
        show_glyph(x, y, glyph);
}

/*
 * map_trap()
 *
 * Map the trap and print it out if directed.  This routine assumes that the
 * hero can physically see the location.
 */
void
map_trap(register struct trap *trap, register int show)
{
    register coordxy x = trap->tx, y = trap->ty;
    register int glyph = trap_to_glyph(trap);

    if (gl.level.flags.hero_memory)
        levl[x][y].glyph = glyph;
    if (show)
        show_glyph(x, y, glyph);
}

/*
 * map_engraving()
 *
 * Map the engraving and print it out if directed.
 */
void
map_engraving(struct engr *ep, register int show)
{
    coordxy x = ep->engr_x, y = ep->engr_y;
    int glyph = engraving_to_glyph(ep);

    if (gl.level.flags.hero_memory)
        levl[x][y].glyph = glyph;
    if (show)
        show_glyph(x, y, glyph);
}

/*
 * map_object()
 *
 * Map the given object.  This routine assumes that the hero can physically
 * see the location of the object.  Update the screen if directed.
 * [Note: feel_location() -> map_location() -> map_object() contradicts
 * the claim here that the hero can see obj's <ox,oy>.]
 */
void
map_object(register struct obj *obj, int show)
{
    register coordxy x = obj->ox, y = obj->oy;
    register int glyph = obj_to_glyph(obj, newsym_rn2);

    /* if this object is already displayed as a generic object, it might
       become a specific one now */
    if (glyph_is_generic_object(glyph) && cansee(x, y) && !Hallucination) {
        /* these 'r' and 'neardist' calculations match distant_name(objnam.c)
           and see_nearby_objects(below); we assume that this is a lone
           object or a pile-top, not something below the top of a pile */
        int r = (u.xray_range > 2) ? u.xray_range : 2,
            /* neardist produces a small square with rounded corners */
            neardist = (r * r) * 2 - r; /* same as r*r + r*(r-1) */

        if (distu(x, y) <= neardist) {
            obj->dknown = 1;
            glyph = obj_to_glyph(obj, newsym_rn2);
        }
    }

    if (gl.level.flags.hero_memory) {
        /* MRKR: While hallucinating, statues are seen as random monsters */
        /*       but remembered as random objects.                        */

        if (Hallucination && obj->otyp == STATUE) {
            levl[x][y].glyph = random_obj_to_glyph(newsym_rn2);
        } else {
            levl[x][y].glyph = glyph;
        }
    }
    if (show)
        show_glyph(x, y, glyph);
}

/*
 * map_invisible()
 *
 * Make the hero remember that a square contains an invisible monster.
 * This is a special case in that the square will continue to be displayed
 * this way even when the hero is close enough to see it.  To get rid of
 * this and display the square's actual contents, use unmap_object() followed
 * by newsym() if necessary.
 */
void
map_invisible(register coordxy x, register coordxy y)
{
    if (x != u.ux || y != u.uy) { /* don't display I at hero's location */
        if (gl.level.flags.hero_memory)
            levl[x][y].glyph = GLYPH_INVISIBLE;
        show_glyph(x, y, GLYPH_INVISIBLE);
    }
}

boolean
unmap_invisible(coordxy x, coordxy y)
{
    if (isok(x,y) && glyph_is_invisible(levl[x][y].glyph)) {
        unmap_object(x, y);
        newsym(x, y);
        return TRUE;
    }
    return FALSE;
}


/*
 * unmap_object()
 *
 * Remove something from the map when the hero realizes it's not there
 * anymore.  Replace it with background or known trap, but not with
 * any other remembered object.  If this is used for detection, a full
 * screen update is imminent anyway; if this is used to get rid of an
 * invisible monster notation, we might have to call newsym().
 */
void
unmap_object(register coordxy x, register coordxy y)
{
    register struct trap *trap;
    struct engr *ep;

    if (!gl.level.flags.hero_memory)
        return;

    if ((trap = t_at(x, y)) != 0 && trap->tseen && !covers_traps(x, y)) {
        map_trap(trap, 0);
    } else if (levl[x][y].seenv) {
        struct rm *lev = &levl[x][y];

        if (spot_shows_engravings(x, y)
               && (ep = engr_at(x, y)) != 0 && !covers_traps(x, y))
            map_engraving(ep, 0);
        else
            map_background(x, y, 0);

        /* turn remembered dark room squares dark */
        if (!lev->waslit && lev->glyph == cmap_to_glyph(S_room)
            && lev->typ == ROOM)
            lev->glyph = cmap_to_glyph(S_stone);
    } else {
        levl[x][y].glyph = cmap_to_glyph(S_stone); /* default val */
    }
}

/*
 * map_location()
 *
 * Make whatever at this location show up.  This is only for non-living
 * things.  This will not handle feeling invisible objects correctly.
 *
 * Internal to display.c, this is a #define for speed.
 */
#define _map_location(x, y, show) \
    {                                                                       \
        register struct obj *obj;                                           \
        register struct trap *trap;                                         \
        struct engr *ep;                                                    \
                                                                            \
        if ((obj = vobj_at(x, y)) && !covers_objects(x, y))                 \
            map_object(obj, show);                                          \
        else if ((trap = t_at(x, y)) && trap->tseen && !covers_traps(x, y)) \
            map_trap(trap, show);                                           \
        else if (spot_shows_engravings(x, y)                                \
                 && (ep = engr_at(x, y)) != 0                               \
                 && !covers_traps(x, y))                                    \
            map_engraving(ep, show);                                        \
        else                                                                \
            map_background(x, y, show);                                     \
                                                                            \
        remember_topology(x, y);                                            \
    }

void
map_location(coordxy x, coordxy y, int show)
{
    _map_location(x, y, show);
}

/* display something on monster layer; may need to fixup object layer */
static void
show_mon_or_warn(coordxy x, coordxy y, int monglyph)
{
    struct obj *o;

    /* "remembered, unseen monster" is tracked by object layer so if we're
       putting something on monster layer at same spot, stop remembering
       that; if an object is in view there, start remembering it instead */
    if (glyph_is_invisible(levl[x][y].glyph)) {
        unmap_object(x, y);
        if (cansee(x, y) && (o = vobj_at(x, y)) != 0)
            map_object(o, FALSE);
    }

    show_glyph(x, y, monglyph);
}

#define DETECTED 2
#define PHYSICALLY_SEEN 1
#define is_worm_tail(mon) ((mon) && ((x != (mon)->mx) || (y != (mon)->my)))

/*
 * display_monster()
 *
 * Note that this is *not* a map_XXXX() function!  Monsters sort of float
 * above everything.
 *
 * Yuck.  Display body parts by recognizing that the display position is
 * not the same as the monster position.  Currently the only body part is
 * a worm tail.
 *
 */
static void
display_monster(coordxy x, coordxy y,    /* display position */
                struct monst *mon,   /* monster to display */
                int sightflags,      /* 1 if the monster is physically seen;
                                        2 if detected using Detect_monsters */
                boolean worm_tail)   /* mon is actually a worm tail */
{
    boolean mon_mimic = (M_AP_TYPE(mon) != M_AP_NOTHING);
    int sensed = (mon_mimic && (Protection_from_shape_changers
                                || sensemon(mon)));
    /*
     * We must do the mimic check first.  If the mimic is mimicing something,
     * and the location is in sight, we have to change the hero's memory
     * so that when the position is out of sight, the hero remembers what
     * the mimic was mimicing.
     */

    if (mon_mimic && (sightflags == PHYSICALLY_SEEN)) {
        switch (M_AP_TYPE(mon)) {
        default:
            impossible("display_monster:  bad m_ap_type value [ = %d ]",
                       (int) mon->m_ap_type);
            /*FALLTHRU*/
        case M_AP_NOTHING:
            show_glyph(x, y, mon_to_glyph(mon, newsym_rn2));
            break;

        case M_AP_FURNITURE: {
            /*
             * This is a poor man's version of map_background().  I can't
             * use map_background() because we are overriding what is in
             * the 'typ' field.  Maybe have map_background()'s parameters
             * be (x,y,glyph) instead of just (x,y).
             *
             * mappearance is currently set to an S_ index value in
             * makemon.c.
             */
            int sym = mon->mappearance, glyph = cmap_to_glyph(sym);

            levl[x][y].glyph = glyph;
            if (!sensed) {
                show_glyph(x, y, glyph);
                /* override real topology with mimic's fake one */
                gl.lastseentyp[x][y] = cmap_to_type(sym);
            }
            break;
        }

        case M_AP_OBJECT: {
            /* Make a fake object to send to map_object(). */
            struct obj obj;

            obj = cg.zeroobj;
            obj.ox = x;
            obj.oy = y;
            obj.otyp = mon->mappearance;
            /* might be mimicing a corpse or statue */
            obj.corpsenm = has_mcorpsenm(mon) ? MCORPSENM(mon) : PM_TENGU;
            map_object(&obj, !sensed);
            break;
        }

        case M_AP_MONSTER:
            show_glyph(x, y,
                       monnum_to_glyph(what_mon((int) mon->mappearance,
                                                rn2_on_display_rng),
                                       mon->female ? FEMALE : MALE));
            break;
        }
    }

    /* If mimic is unsuccessfully mimicing something, display the monster. */
    if (!mon_mimic || sensed) {
        int num;

        /* [ALI] Only use detected glyphs when monster wouldn't be
         * visible by any other means.
         *
         * There are no glyphs for "detected pets" so we have to
         * decide whether to display such things as detected or as tame.
         * If both are being highlighted in the same way, it doesn't
         * matter, but if not, showing them as pets is preferrable.
         */
        if (mon->mtame && !Hallucination) {
            if (worm_tail)
                num = petnum_to_glyph(PM_LONG_WORM_TAIL,
                        mon->female ? FEMALE : MALE);
            else
                num = pet_to_glyph(mon, rn2_on_display_rng);
        } else if (sightflags == DETECTED) {
            if (worm_tail)
                num = detected_monnum_to_glyph(
                             what_mon(PM_LONG_WORM_TAIL, rn2_on_display_rng),
                            mon->female ? FEMALE : MALE);
            else
                num = detected_mon_to_glyph(mon, rn2_on_display_rng);
        } else {
            if (worm_tail)
                num = monnum_to_glyph(
                             what_mon(PM_LONG_WORM_TAIL, rn2_on_display_rng),
                             mon->female ? FEMALE : MALE);
            else
                num = mon_to_glyph(mon, rn2_on_display_rng);
        }
        show_mon_or_warn(x, y, num);
    }
}

/*
 * display_warning()
 *
 * This is also *not* a map_XXXX() function!  Monster warnings float
 * above everything just like monsters do, but only if the monster
 * is not showing.
 *
 * Do not call for worm tails.
 */
static void
display_warning(struct monst *mon)
{
    coordxy x = mon->mx, y = mon->my;
    int glyph;

    if (mon_warning(mon)) {
        int wl = Hallucination ?
            rn2_on_display_rng(WARNCOUNT - 1) + 1 : warning_of(mon);
        glyph = warning_to_glyph(wl);
    } else if (MATCH_WARN_OF_MON(mon)) {
        glyph = mon_to_glyph(mon, rn2_on_display_rng);
    } else {
        impossible("display_warning did not match warning type?");
        return;
    }
    show_mon_or_warn(x, y, glyph);
}

int
warning_of(struct monst *mon)
{
    int wl = 0, tmp = 0;

    if (mon_warning(mon)) {
        tmp = (int) (mon->m_lev / 4);    /* match display.h */
        wl = (tmp > WARNCOUNT - 1) ? WARNCOUNT - 1 : tmp;
    }
    return wl;
}

/* map or status window might not be ready for output during level creation
   or game restoration (something like u.usteed which affects display of
   the hero and also a status condition might not be set up yet) */
boolean
suppress_map_output(void)
{
    if (gi.in_mklev || gp.program_state.saving || gp.program_state.restoring)
        return TRUE;
#ifdef HANGUPHANDLING
    if (gp.program_state.done_hup)
        return TRUE;
#endif
    return FALSE;
}

/*
 * feel_newsym()
 *
 * When hero knows what happened to location, even when blind.
 */
void
feel_newsym(coordxy x, coordxy y)
{
    if (Blind)
        feel_location(x, y);
    else
        newsym(x, y);
}


/*
 * feel_location()
 *
 * Feel the given location.  This assumes that the hero is blind and that
 * the given position is either the hero's or one of the eight squares
 * adjacent to the hero (except for a boulder push).
 * If an invisible monster has gone away, that will be discovered.  If an
 * invisible monster has appeared, this will _not_ be discovered since
 * searching only finds one monster per turn so we must check that separately.
 */
void
feel_location(coordxy x, coordxy y)
{
    struct rm *lev;
    struct obj *boulder;
    register struct monst *mon;

    /* replicate safeguards used by newsym(); might not be required here */
    if (suppress_map_output())
        return;

    if (!isok(x, y))
        return;
    lev = &(levl[x][y]);
    /* If hero's memory of an invisible monster is accurate, we want to keep
     * him from detecting the same monster over and over again on each turn.
     * We must return (so we don't erase the monster).  (We must also, in the
     * search function, be sure to skip over previously detected 'I's.)
     */
    if (glyph_is_invisible(lev->glyph) && m_at(x, y))
        return;

    /* The hero can't feel non pool locations while under water
       except for lava and ice. */
    if (Underwater && !Is_waterlevel(&u.uz)
        && !is_pool_or_lava(x, y) && !is_ice(x, y))
        return;

    /* Set the seen vector as if the hero had seen it.
       It doesn't matter if the hero is levitating or not. */
    set_seenv(lev, u.ux, u.uy, x, y);

    if (!can_reach_floor(FALSE)) {
        /*
         * Levitation Rules.  It is assumed that the hero can feel the state
         * of the walls around herself and can tell if she is in a corridor,
         * room, or doorway.  Boulders are felt because they are large enough.
         * Anything else is unknown because the hero can't reach the ground.
         * This makes things difficult.
         *
         * Check (and display) in order:
         *
         *      + Stone, walls, and closed doors.
         *      + Boulders.  [see a boulder before a doorway]
         *      + doors.
         *      + Room/water positions
         *      + Everything else (hallways!)
         */
        if (IS_ROCK(lev->typ)
            || (IS_DOOR(lev->typ)
                && (lev->doormask & (D_LOCKED | D_CLOSED)))) {
            map_background(x, y, 1);
        } else if ((boulder = sobj_at(BOULDER, x, y)) != 0) {
            map_object(boulder, 1);
        } else if (IS_DOOR(lev->typ)) {
            map_background(x, y, 1);
        } else if (IS_ROOM(lev->typ) || IS_POOL(lev->typ)) {
            boolean do_room_glyph;

            /*
             * An open room or water location.  Normally we wouldn't touch
             * this, but we have to get rid of remembered boulder symbols.
             * This will only occur in rare occasions when the hero goes
             * blind and doesn't find a boulder where expected (something
             * came along and picked it up).  We know that there is not a
             * boulder at this location.  Show fountains, pools, etc.
             * underneath if already seen.  Otherwise, show the appropriate
             * floor symbol.
             *
             * Similarly, if the hero digs a hole in a wall or feels a
             * location that used to contain an unseen monster.  In these
             * cases, there's no reason to assume anything was underneath,
             * so just show the appropriate floor symbol.  If something was
             * embedded in the wall, the glyph will probably already
             * reflect that.  Don't change the symbol in this case.
             *
             * This isn't quite correct.  If the boulder was on top of some
             * other objects they should be seen once the boulder is removed.
             * However, we have no way of knowing that what is there now
             * was there then.  So we let the hero have a lapse of memory.
             * We could also just display what is currently on the top of the
             * object stack (if anything).
             */
            do_room_glyph = FALSE;
            if (lev->glyph == objnum_to_glyph(BOULDER)
                || glyph_is_invisible(lev->glyph)) {
                if (lev->typ != ROOM && lev->seenv)
                    map_background(x, y, 1);
                else
                    do_room_glyph = TRUE;
            } else if (lev->glyph >= cmap_to_glyph(S_stone)
                       && lev->glyph < cmap_to_glyph(S_darkroom)) {
                do_room_glyph = TRUE;
            }
            if (do_room_glyph) {
                lev->glyph = (flags.dark_room && iflags.use_color
                              && !Is_rogue_level(&u.uz))
                                 ? cmap_to_glyph(S_darkroom)
                                 : (lev->waslit ? cmap_to_glyph(S_room)
                                                : cmap_to_glyph(S_stone));
                show_glyph(x, y, lev->glyph);
            }
        } else {
            /* We feel it (I think hallways are the only things left). */
            map_background(x, y, 1);
            /* Corridors are never felt as lit (unless remembered that way) */
            /* (lit_corridor only).                                         */
            if (lev->typ == CORR && lev->glyph == cmap_to_glyph(S_litcorr)
                && !lev->waslit)
                show_glyph(x, y, lev->glyph = cmap_to_glyph(S_corr));
            else if (lev->typ == ROOM && flags.dark_room && iflags.use_color
                     && lev->glyph == cmap_to_glyph(S_room))
                show_glyph(x, y, lev->glyph = cmap_to_glyph(S_darkroom));
        }
    } else {
        _map_location(x, y, 1);

        if (Punished) {
            /*
             * A ball or chain is only felt if it is first on the object
             * location list.  Otherwise, we need to clear the felt bit ---
             * something has been dropped on the ball/chain.  If the bit is
             * not cleared, then when the ball/chain is moved it will drop
             * the wrong glyph.
             *
             * Note: during unpunish() we can be called by delobj() when
             * destroying uchain while uball hasn't been cleared yet (so
             * Punished will still yield True but uchain might not be part
             * of the floor list anymore).
             */
            if (uchain && uchain->where == OBJ_FLOOR
                && uchain->ox == x && uchain->oy == y
                && gl.level.objects[x][y] == uchain)
                u.bc_felt |= BC_CHAIN;
            else
                u.bc_felt &= ~BC_CHAIN; /* do not feel the chain */

            if (uball && uball->where == OBJ_FLOOR
                && uball->ox == x && uball->oy == y
                && gl.level.objects[x][y] == uball)
                u.bc_felt |= BC_BALL;
            else
                u.bc_felt &= ~BC_BALL; /* do not feel the ball */
        }

        /* Floor spaces are dark if unlit.  Corridors are dark if unlit. */
        if (lev->typ == ROOM && lev->glyph == cmap_to_glyph(S_room)
            && (!lev->waslit || (flags.dark_room && iflags.use_color)))
            show_glyph(x, y, lev->glyph = cmap_to_glyph(
                                 flags.dark_room ? S_darkroom : S_stone));
        else if (lev->typ == CORR && lev->glyph == cmap_to_glyph(S_litcorr)
                 && !lev->waslit)
            show_glyph(x, y, lev->glyph = cmap_to_glyph(S_corr));
    }
    /* draw monster on top if we can sense it */
    if ((x != u.ux || y != u.uy) && (mon = m_at(x, y)) != 0 && sensemon(mon))
        display_monster(x, y, mon,
                        (tp_sensemon(mon) || MATCH_WARN_OF_MON(mon))
                            ? PHYSICALLY_SEEN
                            : DETECTED,
                        is_worm_tail(mon));
}

/*
 * newsym()
 *
 * Possibly put a new glyph at the given location.
 */
void
newsym(coordxy x, coordxy y)
{
    struct monst *mon;
    int see_it;
    boolean worm_tail;
    register struct rm *lev = &(levl[x][y]);

    /* don't try to produce map output when level is in a state of flux */
    if (suppress_map_output())
        return;

    /* only permit updating the hero when swallowed */
    if (u.uswallow) {
        if (u_at(x, y))
            display_self();
        return;
    }
    if (Underwater && !Is_waterlevel(&u.uz)) {
        /* when underwater, don't do anything unless <x,y> is an
           adjacent water or lava or ice position */
        if (!(is_pool_or_lava(x, y) || is_ice(x, y)) || !next2u(x, y))
            return;
    }

    /* Can physically see the location. */
    if (cansee(x, y)) {
        NhRegion *reg = visible_region_at(x, y);
        /*
         * Don't use templit here:  E.g.
         *
         *      lev->waslit = !!(lev->lit || templit(x,y));
         *
         * Otherwise we have the "light pool" problem, where non-permanently
         * lit areas just out of sight stay remembered as lit.  They should
         * re-darken.
         *
         * Perhaps ALL areas should revert to their "unlit" look when
         * out of sight.
         */
        lev->waslit = (lev->lit != 0); /* remember lit condition */

        mon = m_at(x, y);
        worm_tail = is_worm_tail(mon);

        /*
         * Normal region shown only on accessible positions, but
         * poison clouds and steam clouds also shown above lava,
         * pools and moats.
         * However, sensed monsters take precedence over all regions.
         */
        if (reg
            && (ACCESSIBLE(lev->typ)
                || (reg->visible && is_pool_or_lava(x, y)))
            && (!mon || worm_tail || !sensemon(mon))) {
            show_region(reg, x, y);
            return;
        }

        if (u_at(x, y)) {
            int see_self = canspotself();

            /* update map information for <u.ux,u.uy> (remembered topology
               and object/known trap/terrain glyph) but only display it if
               hero can't see him/herself, then show self if appropriate */
            _map_location(x, y, !see_self);
            if (see_self)
                display_self();
        } else {
            see_it = mon && (mon_visible(mon)
                             || (!worm_tail && (tp_sensemon(mon)
                                                || MATCH_WARN_OF_MON(mon))));
            if (mon && (see_it || (!worm_tail && Detect_monsters))) {
                if (mon->mtrapped) {
                    struct trap *trap = t_at(x, y);
                    int tt = trap ? trap->ttyp : NO_TRAP;

                    /* if monster is in a physical trap, you see trap too */
                    if (tt == BEAR_TRAP || is_pit(tt) || tt == WEB)
                        trap->tseen = 1;
                }
                _map_location(x, y, 0); /* map under the monster */
                /* also gets rid of any invisibility glyph */
                display_monster(x, y, mon,
                                see_it ? PHYSICALLY_SEEN : DETECTED,
                                worm_tail);
            } else if (mon && mon_warning(mon) && !is_worm_tail(mon)) {
                display_warning(mon);
            } else if (glyph_is_invisible(lev->glyph)) {
                map_invisible(x, y);
            } else
                _map_location(x, y, 1); /* map the location */
        }

    /* Can't see the location. */
    } else {
        if (u_at(x, y)) {
            feel_location(u.ux, u.uy); /* forces an update */

            if (canspotself())
                display_self();
        } else if ((mon = m_at(x, y)) != 0
                   && ((see_it = (tp_sensemon(mon) || MATCH_WARN_OF_MON(mon)
                                  || (see_with_infrared(mon)
                                      && mon_visible(mon)))) != 0
                       || Detect_monsters)) {
            /* Seen or sensed monsters are printed every time.
               This also gets rid of any invisibility glyph. */
            display_monster(x, y, mon, see_it ? 0 : DETECTED,
                            is_worm_tail(mon) ? TRUE : FALSE);
        } else if (mon && mon_warning(mon) && !is_worm_tail(mon)) {
            display_warning(mon);

        /*
         * If the location is remembered as being both dark (waslit is false)
         * and lit (glyph is a lit room or lit corridor) then it was either:
         *
         *      (1) A dark location that the hero could see through night
         *          vision.
         *      (2) Darkened while out of the hero's sight.  This can happen
         *          when cursed scroll of light is read.
         *
         * In either case, we have to manually correct the hero's memory to
         * match waslit.  Deciding when to change waslit is non-trivial.
         *
         *  Note:  If flags.lit_corridor is set, then corridors act like room
         *         squares.  That is, they light up if in night vision range.
         *         If flags.lit_corridor is not set, then corridors will
         *         remain dark unless lit by a light spell and may darken
         *         again, as discussed above.
         *
         * These checks and changes must be here and not in back_to_glyph().
         * They are dependent on the position being out of sight.
         */
        } else if (Is_rogue_level(&u.uz)) {
            if (lev->glyph == cmap_to_glyph(S_litcorr) && lev->typ == CORR)
                show_glyph(x, y, lev->glyph = cmap_to_glyph(S_corr));
            else if (lev->glyph == cmap_to_glyph(S_room) && lev->typ == ROOM
                     && !lev->waslit)
                show_glyph(x, y, lev->glyph = cmap_to_glyph(S_stone));
            else
                goto show_mem;
        } else if (!lev->waslit || (flags.dark_room && iflags.use_color)) {
            if (lev->glyph == cmap_to_glyph(S_litcorr) && lev->typ == CORR)
                show_glyph(x, y, lev->glyph = cmap_to_glyph(S_corr));
            else if (lev->glyph == cmap_to_glyph(S_room) && lev->typ == ROOM)
                show_glyph(x, y, lev->glyph = cmap_to_glyph(DARKROOMSYM));
            else
                goto show_mem;
        } else {
 show_mem:
            show_glyph(x, y, lev->glyph);
        }
    }
}

#undef is_worm_tail

/*
 * shieldeff()
 *
 * Put magic shield pyrotechnics at the given location.  This *could* be
 * pulled into a platform dependent routine for fancier graphics if desired.
 */
void
shieldeff(coordxy x, coordxy y)
{
    register int i;

    if (!flags.sparkle)
        return;
    if (cansee(x, y)) { /* Don't see anything if can't see the location */
        for (i = 0; i < SHIELD_COUNT; i++) {
            show_glyph(x, y, cmap_to_glyph(shield_static[i]));
            flush_screen(1); /* make sure the glyph shows up */
            delay_output();
        }
        newsym(x, y); /* restore the old information */
    }
}

static int
tether_glyph(coordxy x, coordxy y)
{
    int tdx, tdy;
    tdx = u.ux - x;
    tdy = u.uy - y;
    return zapdir_to_glyph(sgn(tdx),sgn(tdy), 2);
}

/*
 * tmp_at()
 *
 * Temporarily place glyphs on the screen.  Do not call delay_output().  It
 * is up to the caller to decide if it wants to wait [presently, everyone
 * but explode() wants to delay].
 *
 * Call:
 *      (DISP_BEAM,    glyph)   open, initialize glyph
 *      (DISP_FLASH,   glyph)   open, initialize glyph
 *      (DISP_ALWAYS,  glyph)   open, initialize glyph
 *      (DISP_CHANGE,  glyph)   change glyph
 *      (DISP_END,     0)       close & clean up (2nd argument doesn't matter)
 *      (DISP_FREEMEM, 0)       only used to prevent memory leak during exit)
 *      (x, y)                  display the glyph at the location
 *
 * DISP_BEAM   - Display the given glyph at each location, but do not erase
 *               any until the close call.
 * DISP_TETHER - Display a tether glyph at each location, and the tethered
 *               object at the farthest location, but do not erase any
 *               until the return trip or close.
 * DISP_FLASH  - Display the given glyph at each location, but erase the
 *               previous location's glyph.
 * DISP_ALWAYS - Like DISP_FLASH, but vision is not taken into account.
 */

#define TMP_AT_MAX_GLYPHS (COLNO * 2)

static struct tmp_glyph {
    coord saved[TMP_AT_MAX_GLYPHS]; /* previously updated positions */
    int sidx;                       /* index of next unused slot in saved[] */
    int style; /* either DISP_BEAM or DISP_FLASH or DISP_ALWAYS */
    int glyph; /* glyph to use when printing */
    struct tmp_glyph *prev;
} tgfirst;

void
tmp_at(coordxy x, coordxy y)
{
    static struct tmp_glyph *tglyph = (struct tmp_glyph *) 0;
    struct tmp_glyph *tmp;

    switch (x) {
    case DISP_BEAM:
    case DISP_ALL:
    case DISP_TETHER:
    case DISP_FLASH:
    case DISP_ALWAYS:
        if (!tglyph)
            tmp = &tgfirst;
        else /* nested effect; we need dynamic memory */
            tmp = (struct tmp_glyph *) alloc(sizeof *tmp);
        tmp->prev = tglyph;
        tglyph = tmp;
        tglyph->sidx = 0;
        tglyph->style = x;
        tglyph->glyph = y;
        flush_screen(0); /* flush buffered glyphs */
        return;

    case DISP_FREEMEM: /* in case game ends with tmp_at() in progress */
        while (tglyph) {
            tmp = tglyph->prev;
            if (tglyph != &tgfirst)
                free((genericptr_t) tglyph);
            tglyph = tmp;
        }
        return;

    default:
        break;
    }

    if (!tglyph) {
        panic("tmp_at: tglyph not initialized");
    } else {
        switch (x) {
        case DISP_CHANGE:
            tglyph->glyph = y;
            break;

        case DISP_END:
            if (tglyph->style == DISP_BEAM || tglyph->style == DISP_ALL) {
                register int i;

                /* Erase (reset) from source to end */
                for (i = 0; i < tglyph->sidx; i++)
                    newsym(tglyph->saved[i].x, tglyph->saved[i].y);
            } else if (tglyph->style == DISP_TETHER) {
                int i;

                if (y == BACKTRACK && tglyph->sidx > 1) {
                    /* backtrack */
                    for (i = tglyph->sidx - 1; i > 0; i--) {
                        newsym(tglyph->saved[i].x, tglyph->saved[i].y);
                        show_glyph(tglyph->saved[i - 1].x,
                                   tglyph->saved[i - 1].y, tglyph->glyph);
                        flush_screen(0); /* make sure it shows up */
                        delay_output();
                    }
                    tglyph->sidx = 1;
                }
                for (i = 0; i < tglyph->sidx; i++)
                    newsym(tglyph->saved[i].x, tglyph->saved[i].y);
            } else {              /* DISP_FLASH or DISP_ALWAYS */
                if (tglyph->sidx) /* been called at least once */
                    newsym(tglyph->saved[0].x, tglyph->saved[0].y);
            }
            /* tglyph->sidx = 0; -- about to be freed, so not necessary */
            tmp = tglyph->prev;
            if (tglyph != &tgfirst)
                free((genericptr_t) tglyph);
            tglyph = tmp;
            break;

        default: /* do it */
            if (!isok(x, y))
                break;
            if (tglyph->style == DISP_BEAM || tglyph->style == DISP_ALL) {
                if (tglyph->style != DISP_ALL && !cansee(x, y))
                    break;
                if (tglyph->sidx >= TMP_AT_MAX_GLYPHS)
                    break; /* too many locations */
                /* save pos for later erasing */
                tglyph->saved[tglyph->sidx].x = x;
                tglyph->saved[tglyph->sidx].y = y;
                tglyph->sidx += 1;
            } else if (tglyph->style == DISP_TETHER) {
                if (tglyph->sidx >= TMP_AT_MAX_GLYPHS)
                    break; /* too many locations */
                if (tglyph->sidx) {
                    int px, py;

                    px = tglyph->saved[tglyph->sidx - 1].x;
                    py = tglyph->saved[tglyph->sidx - 1].y;
                    show_glyph(px, py, tether_glyph(px, py));
                }
                /* save pos for later use or erasure */
                tglyph->saved[tglyph->sidx].x = x;
                tglyph->saved[tglyph->sidx].y = y;
                tglyph->sidx += 1;
            } else { /* DISP_FLASH/ALWAYS */
                if (tglyph
                        ->sidx) { /* not first call, so reset previous pos */
                    newsym(tglyph->saved[0].x, tglyph->saved[0].y);
                    tglyph->sidx = 0; /* display is presently up to date */
                }
                if (!cansee(x, y) && tglyph->style != DISP_ALWAYS)
                    break;
                tglyph->saved[0].x = x;
                tglyph->saved[0].y = y;
                tglyph->sidx = 1;
            }

            show_glyph(x, y, tglyph->glyph); /* show it */
            flush_screen(0);                 /* make sure it shows up */
            break;
        } /* end switch */
    }
}

/*
 * flash_glyph_at(x, y, glyph, repeatcount)
 *
 * Briefly flash between the passed glyph and the glyph that's
 * meant to be at the location.
 */
void
flash_glyph_at(coordxy x, coordxy y, int tg, int rpt)
{
    int i, glyph[2];

    rpt *= 2; /* two loop iterations per 'count' */
    glyph[0] = tg;
    glyph[1] = (gl.level.flags.hero_memory) ? levl[x][y].glyph
                                         : back_to_glyph(x, y);
    /* even iteration count (guaranteed) ends with glyph[1] showing;
       caller might want to override that, but no newsym() calls here
       in case caller has tinkered with location visibility */
    for (i = 0; i < rpt; i++) {
        show_glyph(x, y, glyph[i % 2]);
        flush_screen(1);
        delay_output();
    }
}

/*
 * swallowed()
 *
 * The hero is swallowed.  Show a special graphics sequence for this.  This
 * bypasses all of the display routines and messes with buffered screen
 * directly.  This method works because both vision and display check for
 * being swallowed.
 */
void
swallowed(int first)
{
    static coordxy lastx, lasty; /* last swallowed position */
    int swallower, left_ok, rght_ok;

    if (first) {
        cls();
        bot();
    } else {
        coordxy x, y;

        /* Clear old location */
        for (y = lasty - 1; y <= lasty + 1; y++)
            for (x = lastx - 1; x <= lastx + 1; x++)
                if (isok(x, y))
                    show_glyph(x, y, GLYPH_UNEXPLORED);
    }

    swallower = monsndx(u.ustuck->data);
    /* assume isok(u.ux,u.uy) */
    left_ok = isok(u.ux - 1, u.uy);
    rght_ok = isok(u.ux + 1, u.uy);
    /*
     *  Display the hero surrounded by the monster's stomach.
     */
    if (isok(u.ux, u.uy - 1)) {
        if (left_ok)
            show_glyph(u.ux - 1, u.uy - 1,
                       swallow_to_glyph(swallower, S_sw_tl));
        show_glyph(u.ux, u.uy - 1, swallow_to_glyph(swallower, S_sw_tc));
        if (rght_ok)
            show_glyph(u.ux + 1, u.uy - 1,
                       swallow_to_glyph(swallower, S_sw_tr));
    }

    if (left_ok)
        show_glyph(u.ux - 1, u.uy, swallow_to_glyph(swallower, S_sw_ml));
    display_self();
    if (rght_ok)
        show_glyph(u.ux + 1, u.uy, swallow_to_glyph(swallower, S_sw_mr));

    if (isok(u.ux, u.uy + 1)) {
        if (left_ok)
            show_glyph(u.ux - 1, u.uy + 1,
                       swallow_to_glyph(swallower, S_sw_bl));
        show_glyph(u.ux, u.uy + 1, swallow_to_glyph(swallower, S_sw_bc));
        if (rght_ok)
            show_glyph(u.ux + 1, u.uy + 1,
                       swallow_to_glyph(swallower, S_sw_br));
    }

    /* Update the swallowed position. */
    lastx = u.ux;
    lasty = u.uy;
}

/*
 * under_water()
 *
 * Similar to swallowed() in operation.  Shows hero when underwater
 * except when in water level.  Special routines exist for that.
 */
void
under_water(int mode)
{
    static coordxy lastx, lasty;
    static boolean dela;
    coordxy x, y;

    /* swallowing has a higher precedence than under water */
    if (Is_waterlevel(&u.uz) || u.uswallow)
        return;

    /* full update */
    if (mode == 1 || dela) {
        cls();
        dela = FALSE;

    /* delayed full update */
    } else if (mode == 2) {
        dela = TRUE;
        return;

    /* limited update */
    } else {
        for (y = lasty - 1; y <= lasty + 1; y++)
            for (x = lastx - 1; x <= lastx + 1; x++)
                if (isok(x, y))
                    show_glyph(x, y, GLYPH_UNEXPLORED);
    }

    /*
     * TODO?  Should this honor Xray radius rather than force radius 1?
     */

    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++)
            if (isok(x, y) && (is_pool_or_lava(x, y) || is_ice(x, y))) {
                if (Blind && !u_at(x, y))
                    show_glyph(x, y, GLYPH_UNEXPLORED);
                else
                    newsym(x, y);
            }
    lastx = u.ux;
    lasty = u.uy;
}

/*
 *      under_ground()
 *
 *      Very restricted display.  You can only see yourself.
 */
void
under_ground(int mode)
{
    static boolean dela;

    /* swallowing has a higher precedence than under ground */
    if (u.uswallow)
        return;

    /* full update */
    if (mode == 1 || dela) {
        cls();
        dela = FALSE;

    /* delayed full update */
    } else if (mode == 2) {
        dela = TRUE;
        return;

    /* limited update */
    } else {
        newsym(u.ux, u.uy);
    }
}

/* ======================================================================== */

/*
 * Loop through all of the monsters and update them.  Called when:
 *      + going blind & telepathic
 *      + regaining sight & telepathic
 *      + getting and losing infravision
 *      + hallucinating
 *      + doing a full screen redraw
 *      + see invisible times out or a ring of see invisible is taken off
 *        or intrinsic see invisible is stolen by a gremlin
 *      + when a potion of see invisible is quaffed or a ring of see
 *        invisible is put on
 *      + gaining telepathy when blind [givit() in eat.c, pleased() in pray.c]
 *      + losing telepathy while blind [xkilled() in mon.c, attrcurse() in
 *        sit.c]
 */
void
see_monsters(void)
{
    register struct monst *mon;
    int new_warn_obj_cnt = 0;

    if (gd.defer_see_monsters)
        return;

    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        newsym(mon->mx, mon->my);
        if (mon->wormno)
            see_wsegs(mon);
        if (Warn_of_mon && (gc.context.warntype.obj & mon->data->mflags2) != 0L)
            new_warn_obj_cnt++;
    }

    /*
     * Make Sting glow blue or stop glowing if required.
     */
    if (new_warn_obj_cnt != gw.warn_obj_cnt) {
        Sting_effects(new_warn_obj_cnt);
        gw.warn_obj_cnt = new_warn_obj_cnt;
    }

    /* when mounted, hero's location gets caught by monster loop */
    if (!u.usteed)
        newsym(u.ux, u.uy);
}

static void
mimic_light_blocking(struct monst *mtmp)
{
    if (mtmp->minvis && is_lightblocker_mappear(mtmp)) {
        if (See_invisible)
            block_point(mtmp->mx, mtmp->my);
        else
            unblock_point(mtmp->mx, mtmp->my);
    }
}

/*
 * Block/unblock light depending on what a mimic is mimicing and if it's
 * invisible or not.  Should be called only when the state of See_invisible
 * changes.
 */
void
set_mimic_blocking(void)
{
    iter_mons(mimic_light_blocking);
}

/*
 * Loop through all of the object *locations* and update them.  Called when
 *      + hallucinating.
 */
void
see_objects(void)
{
    register struct obj *obj;

    for (obj = fobj; obj; obj = obj->nobj)
        if (vobj_at(obj->ox, obj->oy) == obj)
            newsym(obj->ox, obj->oy);

    /* Qt's "paper doll" subset of persistent inventory shows map tiles
       for objects which aren't on the floor so not handled by above loop;
       inventory which includes glyphs should also be affected, so do this
       for all interfaces in case any feature that for persistent inventory */
    update_inventory();
}

/* mark the top object of nearby stacks as having been seen, and if
   that object was being displayed as generic, redisplay it as specific */
void
see_nearby_objects(void)
{
    struct obj *obj;
    int glyph;
    coordxy ix, iy, x = u.ux, y = u.uy;
    /* these 'r' and 'neardist' calculations match distant_name(objnam.c) */
    int r = (u.xray_range > 2) ? u.xray_range : 2,
        /* neardist produces a small square with rounded corners */
        neardist = (r * r) * 2 - r; /* same as r*r + r*(r-1) */

    /* [note: this could be optimized to avoid the distu() calculations] */
    for (iy = y - r; iy <= y + r; ++iy)
        for (ix = x - r; ix <= x + r; ++ix) {
            if (!isok(ix, iy))
                continue;
            /* skip if no object or the object has already been marked as
               having been seen up close */
            if ((obj = vobj_at(ix, iy)) == 0 || obj->dknown)
                continue;
            /* skip if the spot can't be seen or is too far (diagonal) */
            if (!cansee(ix, iy) || distu(ix, iy) > neardist)
                continue;

            obj->dknown = 1; /* near enough to see it */
            /* operate on remembered glyph rather than current one */
            glyph = levl[ix][iy].glyph;
            if (glyph_is_generic_object(glyph))
                newsym_force(ix, iy);
        }
}

/*
 * Update hallucinated traps.
 */
void
see_traps(void)
{
    struct trap *trap;
    int glyph;

    for (trap = gf.ftrap; trap; trap = trap->ntrap) {
        glyph = glyph_at(trap->tx, trap->ty);
        if (glyph_is_trap(glyph))
            newsym(trap->tx, trap->ty);
    }
}

/*  glyph, ttychar, { glyphflags, { sym.color, sym.symidx },
                      tileidx, u } */
static glyph_info no_ginfo = {
    NO_GLYPH, ' ', NO_COLOR, { MG_BADXY, { NO_COLOR, 0 }, 0
#ifdef ENHANCED_SYMBOLS
                                                 , 0
#endif
    }
};
#ifndef UNBUFFERED_GLYPHINFO
#define Glyphinfo_at(x, y, glyph) \
    (((x) < 0 || (y) < 0 || (x) >= COLNO || (y) >= ROWNO) ? &no_ginfo   \
     : &gg.gbuf[(y)][(x)].glyphinfo)
#else
static glyph_info ginfo;
#define Glyphinfo_at(x, y, glyph) glyphinfo_at(x, y, glyph)
#endif

#ifdef TILES_IN_GLYPHMAP
extern const glyph_info nul_glyphinfo; /* tile.c */
#else
/* glyph, ttychar, { glyphflags, { sym.color, sym.symidx },
                     tileidx, 0} */
const glyph_info nul_glyphinfo = {
    NO_GLYPH, ' ', NO_COLOR,
        {  /* glyph_map */
            MG_UNEXPL,
            { NO_COLOR, SYM_UNEXPLORED + SYM_OFF_X },
            0
#ifdef ENHANCED_SYMBOLS
             , 0
#endif
        }
};
#endif

#ifdef TILES_IN_GLYPHMAP
extern glyph_map glyphmap[MAX_GLYPH]; /* from tile.c */
#else
glyph_map glyphmap[MAX_GLYPH] = {
    { 0U, { 0, 0}, 0
#ifdef ENHANCED_SYMBOLS
            , 0
#endif
    }
};
#endif

/*
 * Put the cursor on the hero.  Flush all accumulated glyphs before doing it.
 */
void
curs_on_u(void)
{
    flush_screen(1); /* Flush waiting glyphs & put cursor on hero */
}

/* the #redraw command */
int
doredraw(void)
{
    docrt();
    return ECMD_OK;
}

void
docrt(void)
{
    coordxy x, y;
    register struct rm *lev;

    if (!u.ux || gp.program_state.in_docrt)
        return; /* display isn't ready yet */

    gp.program_state.in_docrt = TRUE;

    if (u.uswallow) {
        swallowed(1);
        goto post_map;
    }
    if (Underwater && !Is_waterlevel(&u.uz)) {
        under_water(1);
        goto post_map;
    }
    if (u.uburied) {
        under_ground(1);
        goto post_map;
    }

    /* shut down vision */
    vision_recalc(2);

    /*
     * This routine assumes that cls() does the following:
     *      + fills the physical screen with the symbol for rock
     *      + clears the glyph buffer
     */
    cls();

    /* display memory */
    for (x = 1; x < COLNO; x++) {
        lev = &levl[x][0];
        for (y = 0; y < ROWNO; y++, lev++)
            show_glyph(x, y, lev->glyph);
    }

    /* see what is to be seen */
    vision_recalc(0);

    /* overlay with monsters */
    see_monsters();

 post_map:

    /* perm_invent */
    update_inventory();

    gc.context.botlx = 1; /* force a redraw of the bottom line */
    gp.program_state.in_docrt = FALSE;
}

/* for panning beyond a clipped region; resend the current map data to
   the interface rather than use docrt()'s regeneration of that data */
void
redraw_map(void)
{
    coordxy x, y;
    int glyph;
    glyph_info bkglyphinfo = nul_glyphinfo;

    /*
     * Not sure whether this is actually necessary; save and restore did
     * used to get much too involved with each dungeon level as it was
     * read and written.
     *
     * !u.ux: display isn't ready yet; (restoring || !on_level()): was part
     * of cliparound() but interface shouldn't access this much internals
     */
    if (!u.ux || suppress_map_output() || !on_level(&u.uz0, &u.uz))
        return;

    /*
     * This yields sensible clipping when #terrain+getpos is in
     * progress and the screen displays something other than what
     * the map would currently be showing.
     */
    for (y = 0; y < ROWNO; ++y)
        for (x = 1; x < COLNO; ++x) {
            glyph = glyph_at(x, y); /* not levl[x][y].glyph */
            get_bkglyph_and_framecolor(x, y, &bkglyphinfo.glyph, &bkglyphinfo.framecolor);
            print_glyph(WIN_MAP, x, y,
                        Glyphinfo_at(x, y, glyph), &bkglyphinfo);
        }
    flush_screen(1);
#ifndef UNBUFFERED_GLYPHINFO
    nhUse(glyph);
#endif
}

/*
 * =======================================================
 */
void
reglyph_darkroom(void)
{
    coordxy x, y;

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            struct rm *lev = &levl[x][y];

            if (!flags.dark_room) {
                if (lev->glyph == cmap_to_glyph(S_corr)
                    && lev->waslit)
                    lev->glyph = cmap_to_glyph(S_litcorr);
            } else {
                if (lev->glyph == cmap_to_glyph(S_litcorr)
                    && !cansee(x, y))
                    lev->glyph = cmap_to_glyph(S_corr);
            }

            if (!flags.dark_room || !iflags.use_color
                || Is_rogue_level(&u.uz)) {
                if (lev->glyph == cmap_to_glyph(S_darkroom))
                    lev->glyph = lev->waslit ? cmap_to_glyph(S_room)
                                             : GLYPH_NOTHING;
            } else {
                if (lev->glyph == cmap_to_glyph(S_room) && lev->seenv
                    && lev->waslit && !cansee(x, y))
                    lev->glyph = cmap_to_glyph(S_darkroom);
                else if (lev->glyph == GLYPH_NOTHING
                         && lev->typ == ROOM && lev->seenv && !cansee(x, y))
                    lev->glyph = cmap_to_glyph(S_darkroom);
            }
        }
    if (flags.dark_room && iflags.use_color)
        gs.showsyms[S_darkroom] = gs.showsyms[S_room];
    else
        gs.showsyms[S_darkroom] = gs.showsyms[SYM_NOTHING + SYM_OFF_X];
}

/* ======================================================================== */
/* Glyph Buffering (3rd screen) =========================================== */

/* FIXME: This is a dirty hack, because newsym() doesn't distinguish
 * between object piles and single objects, it doesn't mark the location
 * for update. */
void
newsym_force(coordxy x, coordxy y)
{
    newsym(x, y);
    gg.gbuf[y][x].gnew = 1;
    if (gg.gbuf_start[y] > x)
        gg.gbuf_start[y] = x;
    if (gg.gbuf_stop[y] < x)
        gg.gbuf_stop[y] = x;
}

/*
 * Store the glyph in the 3rd screen for later flushing.
 */
void
show_glyph(coordxy x, coordxy y, int glyph)
{
#ifndef UNBUFFERED_GLYPHINFO
    glyph_info glyphinfo;
#endif

    /*
     * Check for bad positions and glyphs.
     */
    if (!isok(x, y)) {
        const char *text = "";
        int offset = -1;

        /* column 0 is invalid, but it's often used as a flag, so ignore it */
        if (x == 0)
            return;

        /*
         *  This assumes an ordering of the offsets.  See display.h for
         *  the definition.
         */
        if (glyph < 0 || glyph >= MAX_GLYPH) {
            /* invalid location and also invalid glyph */
            text = "invalid";
        } else if ((offset = (glyph - GLYPH_NOTHING_OFF)) >= 0) {
            text = "nothing";
        } else if ((offset = (glyph - GLYPH_UNEXPLORED_OFF)) >= 0) {
            text = "unexplored";
        } else if ((offset = (glyph - GLYPH_STATUE_FEM_PILETOP_OFF)) >= 0) {
            text = "statue of a female monster at top of a pile";
        } else if ((offset = (glyph - GLYPH_STATUE_MALE_PILETOP_OFF)) >= 0) {
            text = "statue of a male monster at top of a pile";
        } else if ((offset = (glyph - GLYPH_BODY_PILETOP_OFF)) >= 0) {
            text = "body at top of a pile";
        } else if ((offset = (glyph - GLYPH_OBJ_PILETOP_OFF)) >= 0) {
            text = (glyph_is_piletop_generic_obj(glyph)
                    ? "generic object at top of a pile"
                    : "object at top of a pile");
        } else if ((offset = (glyph - GLYPH_STATUE_FEM_OFF)) >= 0) {
            text = "statue of female monster";
        } else if ((offset = (glyph - GLYPH_STATUE_MALE_OFF)) >= 0) {
            text = "statue of male monster";
        } else if ((offset = (glyph - GLYPH_WARNING_OFF)) >= 0) {
            /* warn flash */
            text = "warning explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_FROSTY_OFF)) >= 0) {
            text = "frosty explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_FIERY_OFF)) >= 0) {
            text = "fiery explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_MAGICAL_OFF)) >= 0) {
            text = "magical explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_WET_OFF)) >= 0) {
            text = "wet explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_MUDDY_OFF)) >= 0) {
            text = "muddy explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_NOXIOUS_OFF)) >= 0) {
            text = "noxious explosion";
        } else if ((offset = (glyph - GLYPH_EXPLODE_DARK_OFF)) >= 0) {
            text = "dark explosion";
        } else if ((offset = (glyph - GLYPH_SWALLOW_OFF)) >= 0) {
            text = "swallow";
        } else if ((offset = (glyph - GLYPH_CMAP_C_OFF)) >= 0) {
            text = "cmap C";
        } else if ((offset = (glyph - GLYPH_ZAP_OFF)) >= 0) {
            text = "zap";
        } else if ((offset = (glyph - GLYPH_CMAP_B_OFF)) >= 0) {
            text = "cmap B";
        } else if ((offset = (glyph - GLYPH_ALTAR_OFF)) >= 0) {
            text = "altar";
        } else if ((offset = (glyph - GLYPH_CMAP_A_OFF)) >= 0) {
            text = "cmap A";
        } else if ((offset = (glyph - GLYPH_CMAP_SOKO_OFF)) >= 0) {
            text = "sokoban dungeon walls";
        } else if ((offset = (glyph - GLYPH_CMAP_KNOX_OFF)) >= 0) {
            text = "knox dungeon walls";
        } else if ((offset = (glyph - GLYPH_CMAP_GEH_OFF)) >= 0) {
            text = "gehennom dungeon walls";
        } else if ((offset = (glyph - GLYPH_CMAP_MINES_OFF)) >= 0) {
            text = "gnomish mines dungeon walls";
        } else if ((offset = (glyph - GLYPH_CMAP_MAIN_OFF)) >= 0) {
            text = "main dungeon walls";
        } else if ((offset = (glyph - GLYPH_CMAP_STONE_OFF)) >= 0) {
            text = "stone";
        } else if ((offset = (glyph - GLYPH_OBJ_OFF)) >= 0) {
            text = (glyph_is_normal_generic_obj(glyph)
                    ? "generic object"
                    : "object");
        } else if ((offset = (glyph - GLYPH_RIDDEN_FEM_OFF)) >= 0) {
            text = "ridden female monster";
        } else if ((offset = (glyph - GLYPH_RIDDEN_MALE_OFF)) >= 0) {
            text = "ridden male monster";
        } else if ((offset = (glyph - GLYPH_BODY_OFF)) >= 0) {
            text = "body";
        } else if ((offset = (glyph - GLYPH_DETECT_FEM_OFF)) >= 0) {
            text = "detected female monster";
        } else if ((offset = (glyph - GLYPH_DETECT_MALE_OFF)) >= 0) {
            text = "detected male monster";
        } else if ((offset = (glyph - GLYPH_INVIS_OFF)) >= 0) {
            text = "invisible monster";
        } else if ((offset = (glyph - GLYPH_PET_FEM_OFF)) >= 0) {
            text = "female pet";
        } else if ((offset = (glyph - GLYPH_PET_MALE_OFF)) >= 0) {
            text = "male pet";
        } else if ((offset = (glyph - GLYPH_MON_FEM_OFF)) >= 0) {
            text = "female monster";
        } else if ((offset = (glyph - GLYPH_MON_MALE_OFF)) >= 0) {
            text = "male monster";
        }
        impossible("show_glyph:  bad pos <%d,%d> with glyph %d [%s %d].",
                   x, y, glyph, text, offset);
        return;
    } else if (glyph < 0 || glyph >= MAX_GLYPH) {
        /* valid location but invalid glyph */
        impossible("show_glyph:  bad glyph %d [max %d] at <%d,%d>.",
                   glyph, MAX_GLYPH, x, y);
        return;
    }
#ifndef UNBUFFERED_GLYPHINFO
    /* without UNBUFFERED_GLYPHINFO defined the glyphinfo values are buffered
       alongside the glyphs themselves for better performance, increased
       buffer size */
    map_glyphinfo(x, y, glyph, 0, &glyphinfo);
#endif

    if (gg.gbuf[y][x].glyphinfo.glyph != glyph
#ifndef UNBUFFERED_GLYPHINFO
        /* flags might change (single object vs pile, monster tamed or pet
           gone feral), color might change (altar's alignment converted by
           invisible hero), but ttychar normally won't change unless the
           glyph does too (changing boulder symbol would be an exception,
           but that triggers full redraw so doesn't matter here); still,
           be thorough and check everything */
        || gg.gbuf[y][x].glyphinfo.ttychar != glyphinfo.ttychar
        || gg.gbuf[y][x].glyphinfo.gm.glyphflags != glyphinfo.gm.glyphflags
        || gg.gbuf[y][x].glyphinfo.gm.sym.color != glyphinfo.gm.sym.color
        || gg.gbuf[y][x].glyphinfo.gm.tileidx != glyphinfo.gm.tileidx
#endif
        || iflags.use_background_glyph) {
        gg.gbuf[y][x].glyphinfo.glyph = glyph;
        gg.gbuf[y][x].gnew = 1;
#ifndef UNBUFFERED_GLYPHINFO
        gg.gbuf[y][x].glyphinfo.glyph = glyphinfo.glyph;
        gg.gbuf[y][x].glyphinfo.ttychar = glyphinfo.ttychar;
        gg.gbuf[y][x].glyphinfo.gm = glyphinfo.gm;
#endif
        if (gg.gbuf_start[y] > x)
            gg.gbuf_start[y] = x;
        if (gg.gbuf_stop[y] < x)
            gg.gbuf_stop[y] = x;
    }
}

/*
 * Reset the changed glyph borders so that none of the 3rd screen has
 * changed.
 */
#define reset_glyph_bbox()                \
    {                                     \
        int i;                            \
                                          \
        for (i = 0; i < ROWNO; i++) {     \
            gg.gbuf_start[i] = COLNO - 1; \
            gg.gbuf_stop[i] = 0;          \
        }                                 \
    }

static gbuf_entry nul_gbuf = {
    0,                                 /* gnew */
    { GLYPH_UNEXPLORED, (unsigned) ' ', NO_COLOR, /* glyphinfo.glyph */
        /* glyphinfo.gm */
        { MG_UNEXPL, { (unsigned) NO_COLOR, 0 }, 0
#ifdef ENHANCED_SYMBOLS
                                                  , 0
#endif
        }
    }
};

/*
 * Turn the 3rd screen into UNEXPLORED that needs to be refreshed.
 */
void
clear_glyph_buffer(void)
{
    register coordxy x, y;
    gbuf_entry *gptr = &gg.gbuf[0][0];
    glyph_info *giptr =
#ifndef UNBUFFERED_GLYPHINFO
                        &gptr->glyphinfo;
#else
                        &ginfo;

    map_glyphinfo(0, 0, GLYPH_UNEXPLORED, 0, giptr);
#endif
#ifndef UNBUFFERED_GLYPHINFO
    nul_gbuf.gnew = (giptr->ttychar != nul_gbuf.glyphinfo.ttychar
                     || giptr->gm.sym.color != nul_gbuf.glyphinfo.gm.sym.color
                     || giptr->gm.glyphflags
                        != nul_gbuf.glyphinfo.gm.glyphflags
                     || giptr->gm.tileidx != nul_gbuf.glyphinfo.gm.tileidx)
#else
    nul_gbuf.gnew = (giptr->glyphinfo.ttychar != ' '
                     || giptr->gm.sym.color != NO_COLOR
                     || (giptr->gm.glyphflags & ~MG_UNEXPL) != 0)
#endif
                         ? 1 : 0;
    for (y = 0; y < ROWNO; y++) {
        gptr = &gg.gbuf[y][0];
        for (x = COLNO; x; x--) {
            *gptr++ = nul_gbuf;
        }
        gg.gbuf_start[y] = 1;
        gg.gbuf_stop[y] = COLNO - 1;
    }
}

/* used by tty after menu or text popup has temporarily overwritten the map
   and it has been erased so shows spaces, not necessarily S_unexplored */
void
row_refresh(coordxy start, coordxy stop, coordxy y)
{
    register coordxy x;
    int glyph;
    register boolean force;
    gbuf_entry *gptr = &gg.gbuf[0][0];
    glyph_info bkglyphinfo = nul_glyphinfo;
    glyph_info *giptr =
#ifndef UNBUFFERED_GLYPHINFO
                        &gptr->glyphinfo;
#else
                        &ginfo;

    map_glyphinfo(0, 0, GLYPH_UNEXPLORED, 0U, giptr);
#endif
#ifndef UNBUFFERED_GLYPHINFO
    force = (giptr->ttychar != nul_gbuf.glyphinfo.ttychar
                 || giptr->gm.sym.color != nul_gbuf.glyphinfo.gm.sym.color
                 || giptr->gm.glyphflags != nul_gbuf.glyphinfo.gm.glyphflags
                 || giptr->gm.tileidx != nul_gbuf.glyphinfo.gm.tileidx)
#else
    force = (giptr->ttychar != ' '
                 || giptr->gm.sym.color != NO_COLOR
                 || (giptr->gm.glyphflags & ~MG_UNEXPL) != 0)
#endif
                 ? 1 : 0;
    for (x = start; x <= stop; x++) {
        gptr = &gg.gbuf[y][x];
        glyph = gptr->glyphinfo.glyph;
        get_bkglyph_and_framecolor(x, y, &bkglyphinfo.glyph,
                                   &bkglyphinfo.framecolor);
        if (force || glyph != GLYPH_UNEXPLORED
            || bkglyphinfo.framecolor != NO_COLOR) {
            print_glyph(WIN_MAP, x, y,
                        Glyphinfo_at(x, y, glyph), &bkglyphinfo);
        }
    }
}

void
cls(void)
{
    static boolean in_cls = 0;

    if (in_cls)
        return;
    in_cls = TRUE;
    display_nhwindow(WIN_MESSAGE, FALSE); /* flush messages */
    gc.context.botlx = 1;                    /* force update of botl window */
    clear_nhwindow(WIN_MAP);              /* clear physical screen */

    clear_glyph_buffer(); /* force gbuf[][].glyph to unexplored */
    in_cls = FALSE;
}

/*
 * Synch the third screen with the display.
 */
void
flush_screen(int cursor_on_u)
{
    /* Prevent infinite loops on errors:
     *      flush_screen->print_glyph->impossible->pline->flush_screen
     */
    static int flushing = 0;
    static int delay_flushing = 0;
    register coordxy x, y;
    glyph_info bkglyphinfo = nul_glyphinfo;
    int bkglyph;

    /* 3.7: don't update map, status, or perm_invent during save/restore */
    if (suppress_map_output())
        return;

    if (cursor_on_u == -1)
        delay_flushing = !delay_flushing;
    if (delay_flushing)
        return;
    if (flushing)
        return; /* if already flushing then return */
    flushing = 1;
#ifdef HANGUPHANDLING
    if (gp.program_state.done_hup)
        return;
#endif

    /* get this done now, before we place the cursor on the hero */
    if (gc.context.botl || gc.context.botlx)
        bot();
    else if (iflags.time_botl)
        timebot();

    for (y = 0; y < ROWNO; y++) {
        register gbuf_entry *gptr = &gg.gbuf[y][x = gg.gbuf_start[y]];

        for (; x <= gg.gbuf_stop[y]; gptr++, x++) {
            get_bkglyph_and_framecolor(x, y, &bkglyph, &bkglyphinfo.framecolor);
            if (gptr->gnew
                || (gw.wsettings.map_frame_color != NO_COLOR && bkglyphinfo.framecolor != NO_COLOR)) {
                map_glyphinfo(x, y, bkglyph, 0, &bkglyphinfo); /* won't touch framecolor */
                print_glyph(WIN_MAP, x, y,
                            Glyphinfo_at(x, y, gptr->glyph), &bkglyphinfo);
                gptr->gnew = 0;
            }
        }
    }
    reset_glyph_bbox();

    /* after map update, before display_nhwindow(WIN_MAP) */
    if (cursor_on_u)
        curs(WIN_MAP, u.ux, u.uy); /* move cursor to the hero */

    display_nhwindow(WIN_MAP, FALSE);
    flushing = 0;
}

/* ======================================================================== */

/*
 * back_to_glyph()
 *
 * Use the information in the rm structure at the given position to create
 * a glyph of a background.
 *
 * I had to add a field in the rm structure (horizontal) so that we knew
 * if open doors and secret doors were horizontal or vertical.  Previously,
 * the screen symbol had the horizontal/vertical information set at
 * level generation time.
 *
 * I used the 'ladder' field (really doormask) for deciding if stairwells
 * were up or down.  I didn't want to check the upstairs and dnstairs
 * variables.
 */
int
back_to_glyph(coordxy x, coordxy y)
{
    int idx, bypass_glyph = NO_GLYPH;
    struct rm *ptr = &(levl[x][y]);
    struct stairway *sway;

    switch (ptr->typ) {
    case SCORR:
    case STONE:
        idx = gl.level.flags.arboreal ? S_tree : S_stone;
        break;
    case ROOM:
        idx = S_room;
        break;
    case CORR:
        idx = (ptr->waslit || flags.lit_corridor) ? S_litcorr : S_corr;
        break;
    case HWALL:
    case VWALL:
    case TLCORNER:
    case TRCORNER:
    case BLCORNER:
    case BRCORNER:
    case CROSSWALL:
    case TUWALL:
    case TDWALL:
    case TLWALL:
    case TRWALL:
    case SDOOR:
        idx = ptr->seenv ? wall_angle(ptr) : S_stone;
        break;
    case DOOR:
        if (ptr->doormask) {
            if (ptr->doormask & D_BROKEN)
                idx = S_ndoor;
            else if (ptr->doormask & D_ISOPEN)
                idx = (ptr->horizontal) ? S_hodoor : S_vodoor;
            else /* else is closed */
                idx = (ptr->horizontal) ? S_hcdoor : S_vcdoor;
        } else
            idx = S_ndoor;
        break;
    case IRONBARS:
        idx = S_bars;
        break;
    case TREE:
        idx = S_tree;
        break;
    case POOL:
    case MOAT:
        idx = S_pool;
        break;
    case STAIRS:
        sway = stairway_at(x, y);
        if (known_branch_stairs(sway))
            idx = (ptr->ladder & LA_DOWN) ? S_brdnstair : S_brupstair;
        else
            idx = (ptr->ladder & LA_DOWN) ? S_dnstair : S_upstair;
        break;
    case LADDER:
        sway = stairway_at(x, y);
        if (known_branch_stairs(sway))
            idx = (ptr->ladder & LA_DOWN) ? S_brdnladder : S_brupladder;
        else
            idx = (ptr->ladder & LA_DOWN) ? S_dnladder : S_upladder;
        break;
    case FOUNTAIN:
        idx = S_fountain;
        break;
    case SINK:
        idx = S_sink;
        break;
    case ALTAR:
        idx = S_altar;  /* not really used */
        bypass_glyph = altar_to_glyph(ptr->altarmask);
        break;
    case GRAVE:
        idx = S_grave;
        break;
    case THRONE:
        idx = S_throne;
        break;
    case LAVAPOOL:
        idx = S_lava;
        break;
    case LAVAWALL:
        idx = S_lavawall;
        break;
    case ICE:
        idx = S_ice;
        break;
    case AIR:
        idx = S_air;
        break;
    case CLOUD:
        idx = S_cloud;
        break;
    case WATER:
        idx = S_water;
        break;
    case DBWALL:
        idx = (ptr->horizontal) ? S_hcdbridge : S_vcdbridge;
        break;
    case DRAWBRIDGE_UP:
        switch (ptr->drawbridgemask & DB_UNDER) {
        case DB_MOAT:
            idx = S_pool;
            break;
        case DB_LAVA:
            idx = S_lava;
            break;
        case DB_ICE:
            idx = S_ice;
            break;
        case DB_FLOOR:
            idx = S_room;
            break;
        default:
            impossible("Strange db-under: %d",
                       ptr->drawbridgemask & DB_UNDER);
            idx = S_room; /* something is better than nothing */
            break;
        }
        break;
    case DRAWBRIDGE_DOWN:
        idx = (ptr->horizontal) ? S_hodbridge : S_vodbridge;
        break;
    default:
        impossible("back_to_glyph:  unknown level type [ = %d ]", ptr->typ);
        idx = S_room;
        break;
    }

    return (bypass_glyph != NO_GLYPH) ? bypass_glyph : cmap_to_glyph(idx);
}

/*
 * swallow_to_glyph()
 *
 * Convert a monster number and a swallow location into the correct glyph.
 * If you don't want a patchwork monster while hallucinating, decide on
 * a random monster in swallowed() and don't use what_mon() here.
 */
static int
swallow_to_glyph(int mnum, int loc)
{
    int m_3 = what_mon(mnum, rn2_on_display_rng) << 3;

    if (loc < S_sw_tl || S_sw_br < loc) {
        impossible("swallow_to_glyph: bad swallow location");
        loc = S_sw_br;
    }
    return (m_3 | (loc - S_sw_tl)) + GLYPH_SWALLOW_OFF;
}

/*
 * zapdir_to_glyph()
 *
 * Change the given zap direction and beam type into a glyph.  Each beam
 * type has four glyphs, one for each of the symbols below.  The order of
 * the zap symbols [0-3] as defined in rm.h are:
 *
 *      |  S_vbeam      ( 0, 1) or ( 0,-1)
 *      -  S_hbeam      ( 1, 0) or (-1, 0)
 *      \  S_lslant     ( 1, 1) or (-1,-1)
 *      /  S_rslant     (-1, 1) or ( 1,-1)
 */
int
zapdir_to_glyph(int dx, int dy, int beam_type)
{
    if (beam_type >= NUM_ZAP) {
        impossible("zapdir_to_glyph:  illegal beam type");
        beam_type = 0;
    }
    dx = (dx == dy) ? 2 : (dx && dy) ? 3 : dx ? 1 : 0;

    return ((int) ((beam_type << 2) | dx)) + GLYPH_ZAP_OFF;
}

/*
 * Utility routine for dowhatis() used to find out the glyph displayed at
 * the location.  This isn't necessarily the same as the glyph in the levl
 * structure, so we must check the "third screen".
 */
int
glyph_at(coordxy x, coordxy y)
{
    if (x < 0 || y < 0 || x >= COLNO || y >= ROWNO)
        return cmap_to_glyph(S_room); /* XXX */
    return gg.gbuf[y][x].glyphinfo.glyph;
}

#ifdef UNBUFFERED_GLYPHINFO
glyph_info *
glyphinfo_at(coordxy x, coordxy y, int glyph)
{
    map_glyphinfo(x, y, glyph, 0, &ginfo);
    return &ginfo;
}
#endif

/*
 * Get the glyph for the background so that it can potentially
 * be merged into graphical window ports to improve the appearance
 * of stuff on dark room squares and the plane of air etc.
 * [This should be using background as recorded for #overview rather
 * than current data from the map.]
 *
 * Also get the frame color to use for highlighting the map location
 * for some purpose.
 *
 */

static void
get_bkglyph_and_framecolor(coordxy x, coordxy y, int *bkglyph, uint32 *framecolor)
{
    int idx, tmp_bkglyph = GLYPH_UNEXPLORED;
    struct rm *lev = &levl[x][y];

    if (iflags.use_background_glyph && lev->seenv != 0
        && (gg.gbuf[y][x].glyphinfo.glyph != GLYPH_UNEXPLORED)) {
        switch (lev->typ) {
        case SCORR:
        case STONE:
            idx = gl.level.flags.arboreal ? S_tree : S_stone;
            break;
        case ROOM:
           idx = S_room;
           break;
        case CORR:
           idx = (lev->waslit || flags.lit_corridor) ? S_litcorr : S_corr;
           break;
        case ICE:
           idx = S_ice;
           break;
        case AIR:
           idx = S_air;
           break;
        case CLOUD:
           idx = S_cloud;
           break;
        case POOL:
        case MOAT:
           idx = S_pool;
           break;
        case WATER:
           idx = S_water;
           break;
        case LAVAPOOL:
           idx = S_lava;
           break;
        case LAVAWALL:
            idx = S_lavawall;
            break;
        default:
           idx = S_room;
           break;
        }

        if (!cansee(x, y) && (!lev->waslit || flags.dark_room)) {
            /* Floor spaces are dark if unlit.  Corridors are dark if unlit. */
            if (lev->typ == CORR && idx == S_litcorr)
                idx = S_corr;
            else if (idx == S_room)
                idx = (flags.dark_room && iflags.use_color)
                         ? DARKROOMSYM : S_stone;
        }
        if (idx != S_room)
            tmp_bkglyph = cmap_to_glyph(idx);
    }
    *bkglyph = tmp_bkglyph;
#if 0
    /* this guard should be unnecessary */
    if (!framecolor) {
        impossible("null framecolor passed to get_bkglyph_and_framecolor");
        return;
    }
#endif
    if (iflags.bgcolors
        && gw.wsettings.map_frame_color != NO_COLOR && mapxy_valid(x, y))
        *framecolor = gw.wsettings.map_frame_color;
    else
        *framecolor = NO_COLOR;
}

#if defined(TILES_IN_GLYPHMAP) && defined(MSDOS)
#define HAS_ROGUE_IBM_GRAPHICS \
    (gc.currentgraphics == ROGUESET && SYMHANDLING(H_IBM) && !iflags.grmode)
#else
#define HAS_ROGUE_IBM_GRAPHICS \
    (gc.currentgraphics == ROGUESET && SYMHANDLING(H_IBM))
#endif
#define HI_DOMESTIC CLR_WHITE /* monst.c */

/* masks for per-level variances kept in gg.glyphmap_perlevel_flags */
#define GMAP_SET            0x0001
#define GMAP_ROGUELEVEL     0x0002

void
map_glyphinfo(
    coordxy x, coordxy y,
    int glyph,
    unsigned mgflags,
    glyph_info *glyphinfo)
{
    int offset;
    boolean is_you = (u_at(x, y)
                      /* verify hero or steed (not something underneath
                         when hero is invisible without see invisible,
                         or a temporary display effect like an explosion);
                         this is only approximate, because hero might be
                         mimicking an object (after eating mimic corpse or
                         if polyd into mimic) or furniture (only if polyd) */
                      && glyph_is_monster(glyph));
    const glyph_map *gmap = &glyphmap[glyph];

    glyphinfo->gm = *gmap; /* glyphflags, sym.symidx, sym.color, tileidx */
    /*
     * Only hero tinkering permitted on-the-fly (who).
     * Unique glyphs in glyphmap[] determine everything else (what).
     *
     * (Note: if hero is invisible without see invisible, he/she usually
     * can't see himself/herself.  This applies to accessibility hacks
     * as well as to regular display.)
     */
    if (is_you) {
#ifdef TEXTCOLOR
        if (!iflags.use_color || Upolyd || glyph != hero_glyph) {
            ; /* color tweak not needed (!use_color) or not wanted (poly'd
                 or riding--which uses steed's color, not hero's) */
        } else if (HAS_ROGUE_IBM_GRAPHICS
                   && gs.symset[gc.currentgraphics].nocolor == 0) {
            /* actually player should be yellow-on-gray if in corridor */
            glyphinfo->gm.sym.color = CLR_YELLOW;
        } else if (flags.showrace) {
            /* for showrace, non-human hero is displayed by the symbol of
               corresponding type of monster rather than by '@' (handled
               by newsym()); we change the color to same as human hero */
            glyphinfo->gm.sym.color = HI_DOMESTIC;
        }
#endif
        /* accessibility
          This unchanging display character for hero was requested by
          a blind player to enhance screen reader use.
          Turn on override symbol if caller hasn't specified NOOVERRIDE. */
        if (sysopt.accessibility == 1 && !(mgflags & MG_FLAG_NOOVERRIDE)) {
            offset = SYM_HERO_OVERRIDE + SYM_OFF_X;
            if ((gg.glyphmap_perlevel_flags & GMAP_ROGUELEVEL)
                    ? go.ov_rogue_syms[offset]
                    : go.ov_primary_syms[offset])
                glyphinfo->gm.sym.symidx = offset;
        }
        glyphinfo->gm.glyphflags |= MG_HERO;
    }
    if (sysopt.accessibility == 1
        && (mgflags & MG_FLAG_NOOVERRIDE) && glyph_is_pet(glyph)) {
        /* one more accessibility kludge;
           turn off override symbol if caller has specified NOOVERRIDE */
        glyphinfo->gm.sym.symidx = mons[glyph_to_mon(glyph)].mlet + SYM_OFF_M;
    }
    glyphinfo->ttychar = gs.showsyms[glyphinfo->gm.sym.symidx];
    glyphinfo->glyph = glyph;
}

#ifdef TEXTCOLOR
/*
 *  This must be the same order as used for buzz() in zap.c.
 *  The zap_color_ and altar_color_ enums are in decl.h.
 */
const int zapcolors[NUM_ZAP] = {
    zap_color_missile,    zap_color_fire,  zap_color_frost,
    zap_color_sleep,      zap_color_death, zap_color_lightning,
    zap_color_poison_gas, zap_color_acid,
};
const int altarcolors[] = {
    altar_color_unaligned, altar_color_chaotic, altar_color_neutral,
    altar_color_lawful, altar_color_other
};
const int explodecolors[7] = {
    explode_color_dark,   explode_color_noxious, explode_color_muddy,
    explode_color_wet,    explode_color_magical, explode_color_fiery,
    explode_color_frosty,
};

/* main_walls, mines_walls, gehennom_walls, knox_walls, sokoban_walls */
int wallcolors[sokoban_walls + 1] = {
    /* default init value is to match defsym[S_vwall + n].color (CLR_GRAY) */
    CLR_GRAY, CLR_GRAY, CLR_GRAY, CLR_GRAY, CLR_GRAY,
    /* CLR_GRAY, CLR_BROWN, CLR_RED, CLR_GRAY, CLR_BRIGHT_BLUE, */
};

#endif /* text color */

#ifdef TEXTCOLOR
#define zap_color(n) color = iflags.use_color ? zapcolors[n] : NO_COLOR
#define cmap_color(n) color = iflags.use_color ? defsyms[n].color : NO_COLOR
#define obj_color(n) color = iflags.use_color ? objects[n].oc_color : NO_COLOR
#define mon_color(n) color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define invis_color(n) color = NO_COLOR
#define pet_color(n) color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define warn_color(n) \
    color = iflags.use_color ? def_warnsyms[n].color : NO_COLOR
#define explode_color(n) \
    color = iflags.use_color ? explodecolors[n] : NO_COLOR
#define wall_color(n) color = iflags.use_color ? wallcolors[n] : NO_COLOR
#define altar_color(n) color = iflags.use_color ? altarcolors[n] : NO_COLOR
#else /* no text color */

#define zap_color(n)
#define cmap_color(n)
#define obj_color(n)
#define mon_color(n)
#define invis_color(n)
#define pet_color(c)
#define warn_color(n)
#define explode_color(n)
#define wall_color(n)
#define altar_color(n)
#endif

#if 0
#define is_objpile(x, y)                          \
    (!Hallucination && gl.level.objects[(x)][(y)] \
     && gl.level.objects[(x)][(y)]->nexthere)
#endif

static int cmap_to_roguecolor(int);

static int
cmap_to_roguecolor(int cmap)
{
    int color = NO_COLOR;

    if (gs.symset[gc.currentgraphics].nocolor)
        return NO_COLOR;

    if (cmap >= S_vwall && cmap <= S_hcdoor)
        color = CLR_BROWN;
    else if (cmap >= S_arrow_trap && cmap <= S_polymorph_trap)
        color = CLR_MAGENTA;
    else if (cmap == S_corr || cmap == S_litcorr)
        color = CLR_GRAY;
    else if (cmap >= S_room && cmap <= S_water
                && cmap != S_darkroom)
        color = CLR_GREEN;
    else
        color = NO_COLOR;

    return color;
}

/*
   reset_glyphmap(trigger)

   The glyphmap likely needs to be re-calculated for the following triggers:

   gm_levelchange    called when the player has gone to a new level.

   gm_symchange      called if someone has interactively altered the symbols
                     for the game, most likely at the options menu.

   gm_optionchange   The game settings have been toggled and some of them
                     are configured to trigger a reset_glyphmap()

   gm_accessibility_change  One of the accessibility flags has changed.

*/

void
reset_glyphmap(enum glyphmap_change_triggers trigger)
{
    int glyph;
    register int offset;
    int color = NO_COLOR;

    /* condense multiple tests in macro version down to single */
    boolean has_rogue_ibm_graphics = HAS_ROGUE_IBM_GRAPHICS,
            has_rogue_color = (has_rogue_ibm_graphics
                               && gs.symset[gc.currentgraphics].nocolor == 0);
    if (trigger == gm_levelchange)
        gg.glyphmap_perlevel_flags = 0;

    if (!gg.glyphmap_perlevel_flags) {
        /*
         *    GMAP_SET                0x00000001
         *    GMAP_ROGUELEVEL         0x00000002
         */
        gg.glyphmap_perlevel_flags |= GMAP_SET;

        if (Is_rogue_level(&u.uz)) {
            gg.glyphmap_perlevel_flags |= GMAP_ROGUELEVEL;
        }
    }

    for (glyph = 0; glyph < MAX_GLYPH; ++glyph) {
        glyph_map *gmap = &glyphmap[glyph];

        gmap->glyphflags = 0U;
        /*
         *  Map the glyph to a character and color.
         *
         *  Warning:  For speed, this makes an assumption on the order of
         *            offsets.  The order is set in display.h.
         */
        if ((offset = (glyph - GLYPH_NOTHING_OFF)) >= 0) {
            gmap->sym.symidx = SYM_NOTHING + SYM_OFF_X;
            color = NO_COLOR;
            gmap->glyphflags |= MG_NOTHING;
        } else if ((offset = (glyph - GLYPH_UNEXPLORED_OFF)) >= 0) {
            gmap->sym.symidx = SYM_UNEXPLORED + SYM_OFF_X;
            color = NO_COLOR;
            gmap->glyphflags |= MG_UNEXPL;
        } else if ((offset = (glyph - GLYPH_STATUE_FEM_PILETOP_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = CLR_RED;
            else
                obj_color(STATUE);
            gmap->glyphflags |= (MG_STATUE | MG_FEMALE | MG_OBJPILE);
        } else if ((offset = (glyph - GLYPH_STATUE_MALE_PILETOP_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = CLR_RED;
            else
                obj_color(STATUE);
            gmap->glyphflags |= (MG_STATUE | MG_MALE | MG_OBJPILE);
        } else if ((offset = (glyph - GLYPH_BODY_PILETOP_OFF)) >= 0) {
            gmap->sym.symidx = objects[CORPSE].oc_class + SYM_OFF_O;
            if (has_rogue_color)
                color = CLR_RED;
            else
                mon_color(offset);
            gmap->glyphflags |= (MG_CORPSE | MG_OBJPILE);
        } else if ((offset = (glyph - GLYPH_OBJ_PILETOP_OFF)) >= 0) {
            gmap->sym.symidx = objects[offset].oc_class + SYM_OFF_O;
            if (offset == BOULDER)
                gmap->sym.symidx = SYM_BOULDER + SYM_OFF_X;
            if (has_rogue_color) {
                switch (objects[offset].oc_class) {
                case COIN_CLASS:
                    color = CLR_YELLOW;
                    break;
                case FOOD_CLASS:
                    color = CLR_RED;
                    break;
                default:
                    color = CLR_BRIGHT_BLUE;
                    break;
                }
            } else
                obj_color(offset);
            gmap->glyphflags |= MG_OBJPILE;
        } else if ((offset = (glyph - GLYPH_STATUE_FEM_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = CLR_RED;
            else
                obj_color(STATUE);
            gmap->glyphflags |= (MG_STATUE | MG_FEMALE);
        } else if ((offset = (glyph - GLYPH_STATUE_MALE_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = CLR_RED;
            else
                obj_color(STATUE);
            gmap->glyphflags |= (MG_STATUE | MG_MALE);
        } else if ((offset = (glyph - GLYPH_WARNING_OFF))
                   >= 0) { /* warn flash */
            gmap->sym.symidx = offset + SYM_OFF_W;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                warn_color(offset);
        } else if ((offset = (glyph - GLYPH_EXPLODE_FROSTY_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_frosty);
        } else if ((offset = (glyph - GLYPH_EXPLODE_FIERY_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_fiery);
        } else if ((offset = (glyph - GLYPH_EXPLODE_MAGICAL_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_magical);
        } else if ((offset = (glyph - GLYPH_EXPLODE_WET_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_wet);
        } else if ((offset = (glyph - GLYPH_EXPLODE_MUDDY_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_muddy);
        } else if ((offset = (glyph - GLYPH_EXPLODE_NOXIOUS_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_noxious);
        } else if ((offset = (glyph - GLYPH_EXPLODE_DARK_OFF)) >= 0) {
            gmap->sym.symidx = S_expl_tl + offset + SYM_OFF_P;
            explode_color(expl_dark);
        } else if ((offset = (glyph - GLYPH_SWALLOW_OFF)) >= 0) {
            /* see swallow_to_glyph() in display.c */
            gmap->sym.symidx = (S_sw_tl + (offset & 0x7)) + SYM_OFF_P;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                mon_color(offset >> 3);
        } else if ((offset = (glyph - GLYPH_CMAP_C_OFF)) >= 0) {
            gmap->sym.symidx = S_digbeam + offset + SYM_OFF_P;
            if (has_rogue_color)
                color = cmap_to_roguecolor(S_digbeam + offset);
            else
                cmap_color(S_digbeam + offset);
        } else if ((offset = (glyph - GLYPH_ZAP_OFF)) >= 0) {
            /* see zapdir_to_glyph() in display.c */
            gmap->sym.symidx = (S_vbeam + (offset & 0x3)) + SYM_OFF_P;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                zap_color((offset >> 2));
        } else if ((offset = (glyph - GLYPH_CMAP_B_OFF)) >= 0) {
            int cmap = S_grave + offset;

            gmap->sym.symidx = cmap + SYM_OFF_P;
            cmap_color(cmap);
            if (!iflags.use_color) {
                /* try to provide a visible difference between water and lava
                   if they use the same symbol and color is disabled */
                if (cmap == S_lava
                    && (gs.showsyms[gmap->sym.symidx]
                            == gs.showsyms[S_pool + SYM_OFF_P]
                        || gs.showsyms[gmap->sym.symidx]
                               == gs.showsyms[S_water + SYM_OFF_P])) {
                    gmap->glyphflags |= MG_BW_LAVA;

                /* similar for floor [what about empty doorway?] and ice */
                } else if (cmap == S_ice
                           && (gs.showsyms[gmap->sym.symidx]
                                   == gs.showsyms[S_room + SYM_OFF_P]
                               || gs.showsyms[gmap->sym.symidx]
                                      == gs.showsyms[S_darkroom
                                                    + SYM_OFF_P])) {
                    gmap->glyphflags |= MG_BW_ICE;

                /* and for fountain vs sink */
                } else if (cmap == S_sink
                           && (gs.showsyms[gmap->sym.symidx]
                               == gs.showsyms[S_fountain + SYM_OFF_P])) {
                    gmap->glyphflags |= MG_BW_SINK;
                }
            } else if (has_rogue_color) {
                color = cmap_to_roguecolor(cmap);
            }
        } else if ((offset = (glyph - GLYPH_ALTAR_OFF)) >= 0) {
            /* unaligned, chaotic, neutral, lawful, other altar */
            gmap->sym.symidx = S_altar + SYM_OFF_P;
            if (has_rogue_color)
                color = cmap_to_roguecolor(S_altar);
            else
                altar_color(offset);
        } else if ((offset = (glyph - GLYPH_CMAP_A_OFF)) >= 0) {
            int cmap = S_ndoor + offset;
            gmap->sym.symidx = cmap + SYM_OFF_P;
            cmap_color(cmap);
            /*
             *   Some specialty color mappings not hardcoded in data init
             */
            if (has_rogue_color) {
                color = cmap_to_roguecolor(cmap);
#ifdef TEXTCOLOR
            /* provide a visible difference if normal and lit corridor
               use the same symbol */
            } else if ((cmap == S_litcorr)
                       && gs.showsyms[gmap->sym.symidx]
                              == gs.showsyms[S_corr + SYM_OFF_P]) {
                color = CLR_WHITE;
#endif
            }
        } else if ((offset = (glyph - GLYPH_CMAP_SOKO_OFF)) >= 0) {
            gmap->sym.symidx = S_vwall + offset + SYM_OFF_P;
            wall_color(sokoban_walls);
        } else if ((offset = (glyph - GLYPH_CMAP_KNOX_OFF)) >= 0) {
            gmap->sym.symidx = S_vwall + offset + SYM_OFF_P;
            wall_color(knox_walls);
        } else if ((offset = (glyph - GLYPH_CMAP_GEH_OFF)) >= 0) {
            gmap->sym.symidx = S_vwall + offset + SYM_OFF_P;
            wall_color(gehennom_walls);
        } else if ((offset = (glyph - GLYPH_CMAP_MINES_OFF)) >= 0) {
            gmap->sym.symidx = S_vwall + offset + SYM_OFF_P;
            wall_color(mines_walls);
        } else if ((offset = (glyph - GLYPH_CMAP_MAIN_OFF)) >= 0) {
            gmap->sym.symidx = S_vwall + offset + SYM_OFF_P;
            if (has_rogue_color)
                color = cmap_to_roguecolor(S_vwall + offset);
            else
                wall_color(main_walls);
        } else if ((offset = (glyph - GLYPH_CMAP_STONE_OFF)) >= 0) {
            gmap->sym.symidx = SYM_OFF_P;
            cmap_color(S_stone);
        } else if ((offset = (glyph - GLYPH_OBJ_OFF)) >= 0) {
            gmap->sym.symidx = objects[offset].oc_class + SYM_OFF_O;
            if (offset == BOULDER)
                gmap->sym.symidx = SYM_BOULDER + SYM_OFF_X;
            if (has_rogue_color) {
                switch (objects[offset].oc_class) {
                case COIN_CLASS:
                    color = CLR_YELLOW;
                    break;
                case FOOD_CLASS:
                    color = CLR_RED;
                    break;
                default:
                    color = CLR_BRIGHT_BLUE;
                    break;
                }
            } else
                obj_color(offset);
        } else if ((offset = (glyph - GLYPH_RIDDEN_FEM_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                /* This currently implies that the hero is here -- monsters */
                /* don't ride (yet...).  Should we set it to yellow like in */
                /* the monster case below?  There is no equivalent in rogue.
                 */
                color = NO_COLOR;
            else
                mon_color(offset);
            gmap->glyphflags |= (MG_RIDDEN | MG_FEMALE);
        } else if ((offset = (glyph - GLYPH_RIDDEN_MALE_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                mon_color(offset);
            gmap->glyphflags |= (MG_RIDDEN | MG_MALE);
        } else if ((offset = (glyph - GLYPH_BODY_OFF)) >= 0) {
            gmap->sym.symidx = objects[CORPSE].oc_class + SYM_OFF_O;
            if (has_rogue_color)
                color = CLR_RED;
            else
                mon_color(offset);
            gmap->glyphflags |= MG_CORPSE;
        } else if ((offset = (glyph - GLYPH_DETECT_FEM_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                mon_color(offset);
            /* Disabled for now; anyone want to get reverse video to work? */
            /* is_reverse = TRUE; */
            gmap->glyphflags |= (MG_DETECT | MG_FEMALE);
        } else if ((offset = (glyph - GLYPH_DETECT_MALE_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                mon_color(offset);
            /* Disabled for now; anyone want to get reverse video to work? */
            /* is_reverse = TRUE; */
            gmap->glyphflags |= (MG_DETECT | MG_MALE);
        } else if ((offset = (glyph - GLYPH_INVIS_OFF)) >= 0) {
            gmap->sym.symidx = SYM_INVISIBLE + SYM_OFF_X;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                invis_color(offset);
            gmap->glyphflags |= MG_INVIS;
        } else if ((offset = (glyph - GLYPH_PET_FEM_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                pet_color(offset);
            gmap->glyphflags |= (MG_PET | MG_FEMALE);
        } else if ((offset = (glyph - GLYPH_PET_MALE_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color)
                color = NO_COLOR;
            else
                pet_color(offset);
            gmap->glyphflags |= (MG_PET | MG_MALE);
        } else if ((offset = (glyph - GLYPH_MON_FEM_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color) {
                color = NO_COLOR;
            } else {
                mon_color(offset);
            }
            gmap->glyphflags |= MG_FEMALE;
        } else if ((offset = (glyph - GLYPH_MON_MALE_OFF)) >= 0) {
            gmap->sym.symidx = mons[offset].mlet + SYM_OFF_M;
            if (has_rogue_color) {
                color = CLR_YELLOW;
            } else {
                mon_color(offset);
            }
            gmap->glyphflags |= MG_MALE;
        }
        /* This was requested by a blind player to enhance screen reader use
         */
        if (sysopt.accessibility == 1 && (gmap->glyphflags & MG_PET) != 0) {
            int pet_override = ((gg.glyphmap_perlevel_flags & GMAP_ROGUELEVEL)
                         ? go.ov_rogue_syms[SYM_PET_OVERRIDE + SYM_OFF_X]
                         : go.ov_primary_syms[SYM_PET_OVERRIDE + SYM_OFF_X]);

            if (gs.showsyms[pet_override] != ' ')
                gmap->sym.symidx = SYM_PET_OVERRIDE + SYM_OFF_X;
        }
#ifdef TEXTCOLOR
        /* Turn off color if no color defined, or rogue level w/o PC graphics.
         */
        if ((!has_color(color)
             || ((gg.glyphmap_perlevel_flags & GMAP_ROGUELEVEL)
                 && !has_rogue_color)) || !iflags.use_color)
#endif
            color = NO_COLOR;
        gmap->sym.color = color;
    }
    gg.glyph_reset_timestamp = gm.moves;
}

/* ------------------------------------------------------------------------ */
/* Wall Angle ------------------------------------------------------------- */

#ifdef WA_VERBOSE

static const char *type_to_name(int);
static void error4(coordxy, coordxy, int, int, int, int);

static int bad_count[MAX_TYPE]; /* count of positions flagged as bad */
static const char *const type_names[MAX_TYPE] = {
    "STONE", "VWALL", "HWALL", "TLCORNER", "TRCORNER", "BLCORNER", "BRCORNER",
    "CROSSWALL", "TUWALL", "TDWALL", "TLWALL", "TRWALL", "DBWALL", "TREE",
    "SDOOR", "SCORR", "POOL", "MOAT", "WATER", "DRAWBRIDGE_UP", "LAVAPOOL",
    "LAVAWALL",
    "IRON_BARS", "DOOR", "CORR", "ROOM", "STAIRS", "LADDER", "FOUNTAIN",
    "THRONE", "SINK", "GRAVE", "ALTAR", "ICE", "DRAWBRIDGE_DOWN", "AIR",
    "CLOUD"
};

static const char *
type_to_name(int type)
{
    return (type < 0 || type >= MAX_TYPE) ? "unknown" : type_names[type];
}

static void
error4(coordxy x, coordxy y, int a, int b, int c, int dd)
{
    pline("set_wall_state: %s @ (%d,%d) %s%s%s%s",
          type_to_name(levl[x][y].typ), x, y,
          a ? "1" : "", b ? "2" : "", c ? "3" : "", dd ? "4" : "");
    bad_count[levl[x][y].typ]++;
}
#endif /* WA_VERBOSE */

/*
 * Return 'which' if position is implies an unfinished exterior.  Return
 * zero otherwise.  Unfinished implies outer area is rock or a corridor.
 *
 * Things that are ambiguous: lava
 */
static int
check_pos(coordxy x, coordxy y, int which)
{
    int type;

    if (!isok(x, y))
        return which;
    type = levl[x][y].typ;
    if (IS_ROCK(type) || type == CORR || type == SCORR)
        return which;
    return 0;
}

/* Return TRUE if more than one is non-zero. */
/*ARGSUSED*/
#ifdef WA_VERBOSE
static boolean
more_than_one(coordxy x, coordxy y, coordxy a, coordxy b, coordxy c)
{
    if ((a && (b | c)) || (b && (a | c)) || (c && (a | b))) {
        error4(x, y, a, b, c, 0);
        return TRUE;
    }
    return FALSE;
}
#else
#define more_than_one(x, y, a, b, c) \
    (((a) && ((b) | (c))) || ((b) && ((a) | (c))) || ((c) && ((a) | (b))))
#endif

/* Return the wall mode for a T wall. */
static int
set_twall(
#ifdef WA_VERBOSE
          coordxy x0, coordxy y0, /* used #if WA_VERBOSE */
#else
          coordxy x0 UNUSED, coordxy y0 UNUSED,
#endif
          coordxy x1, coordxy y1, coordxy x2, coordxy y2, coordxy x3, coordxy y3)
{
    int wmode, is_1, is_2, is_3;

    is_1 = check_pos(x1, y1, WM_T_LONG);
    is_2 = check_pos(x2, y2, WM_T_BL);
    is_3 = check_pos(x3, y3, WM_T_BR);
    if (more_than_one(x0, y0, is_1, is_2, is_3)) {
        wmode = 0;
    } else {
        wmode = is_1 + is_2 + is_3;
    }
    return wmode;
}

/* Return wall mode for a horizontal or vertical wall. */
static int
set_wall(coordxy x, coordxy y, int horiz)
{
    int wmode, is_1, is_2;

    if (horiz) {
        is_1 = check_pos(x, y - 1, WM_W_TOP);
        is_2 = check_pos(x, y + 1, WM_W_BOTTOM);
    } else {
        is_1 = check_pos(x - 1, y, WM_W_LEFT);
        is_2 = check_pos(x + 1, y, WM_W_RIGHT);
    }
    if (more_than_one(x, y, is_1, is_2, 0)) {
        wmode = 0;
    } else {
        wmode = is_1 + is_2;
    }
    return wmode;
}

/* Return a wall mode for a corner wall. (x4,y4) is the "inner" position. */
static int
set_corn(coordxy x1, coordxy y1, coordxy x2, coordxy y2, coordxy x3, coordxy y3, coordxy x4, coordxy y4)
{
    coordxy wmode, is_1, is_2, is_3, is_4;

    is_1 = check_pos(x1, y1, 1);
    is_2 = check_pos(x2, y2, 1);
    is_3 = check_pos(x3, y3, 1);
    is_4 = check_pos(x4, y4, 1); /* inner location */

    /*
     * All 4 should not be true.  So if the inner location is rock,
     * use it.  If all of the outer 3 are true, use outer.  We currently
     * can't cover the case where only part of the outer is rock, so
     * we just say that all the walls are finished (if not overridden
     * by the inner section).
     */
    if (is_4) {
        wmode = WM_C_INNER;
    } else if (is_1 && is_2 && is_3)
        wmode = WM_C_OUTER;
    else
        wmode = 0; /* finished walls on all sides */

    return wmode;
}

/* Return mode for a crosswall. */
static int
set_crosswall(coordxy x, coordxy y)
{
    coordxy wmode, is_1, is_2, is_3, is_4;

    is_1 = check_pos(x - 1, y - 1, 1);
    is_2 = check_pos(x + 1, y - 1, 1);
    is_3 = check_pos(x + 1, y + 1, 1);
    is_4 = check_pos(x - 1, y + 1, 1);

    wmode = is_1 + is_2 + is_3 + is_4;
    if (wmode > 1) {
        if (is_1 && is_3 && (is_2 + is_4 == 0)) {
            wmode = WM_X_TLBR;
        } else if (is_2 && is_4 && (is_1 + is_3 == 0)) {
            wmode = WM_X_BLTR;
        } else {
#ifdef WA_VERBOSE
            error4(x, y, is_1, is_2, is_3, is_4);
#endif
            wmode = 0;
        }
    } else if (is_1)
        wmode = WM_X_TL;
    else if (is_2)
        wmode = WM_X_TR;
    else if (is_3)
        wmode = WM_X_BR;
    else if (is_4)
        wmode = WM_X_BL;

    return wmode;
}

/* called for every <x,y> by set_wall_state() and for specific <x,y> during
   vault wall repair */
void
xy_set_wall_state(coordxy x, coordxy y)
{
    coordxy wmode;
    struct rm *lev = &levl[x][y];

    switch (lev->typ) {
    case SDOOR:
        wmode = set_wall(x, y, (coordxy) lev->horizontal);
        break;
    case VWALL:
        wmode = set_wall(x, y, 0);
        break;
    case HWALL:
        wmode = set_wall(x, y, 1);
        break;
    case TDWALL:
        wmode = set_twall(x, y, x, y - 1, x - 1, y + 1, x + 1, y + 1);
        break;
    case TUWALL:
        wmode = set_twall(x, y, x, y + 1, x + 1, y - 1, x - 1, y - 1);
        break;
    case TLWALL:
        wmode = set_twall(x, y, x + 1, y, x - 1, y - 1, x - 1, y + 1);
        break;
    case TRWALL:
        wmode = set_twall(x, y, x - 1, y, x + 1, y + 1, x + 1, y - 1);
        break;
    case TLCORNER:
        wmode = set_corn(x - 1, y - 1, x, y - 1, x - 1, y, x + 1, y + 1);
        break;
    case TRCORNER:
        wmode = set_corn(x, y - 1, x + 1, y - 1, x + 1, y, x - 1, y + 1);
        break;
    case BLCORNER:
        wmode = set_corn(x, y + 1, x - 1, y + 1, x - 1, y, x + 1, y - 1);
        break;
    case BRCORNER:
        wmode = set_corn(x + 1, y, x + 1, y + 1, x, y + 1, x - 1, y - 1);
        break;
    case CROSSWALL:
        wmode = set_crosswall(x, y);
        break;

    default:
        wmode = -1; /* don't set wall info */
        break;
    }

    if (wmode >= 0)
        lev->wall_info = (lev->wall_info & ~WM_MASK) | wmode;
}

/* Called from mklev.  Scan the level and set the wall modes. */
void
set_wall_state(void)
{
    coordxy x, y;

#ifdef WA_VERBOSE
    for (x = 0; x < MAX_TYPE; x++)
        bad_count[x] = 0;
#endif

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            xy_set_wall_state(x, y);

#ifdef WA_VERBOSE
    /* check if any bad positions found */
    for (x = y = 0; x < MAX_TYPE; x++)
        if (bad_count[x]) {
            if (y == 0) {
                y = 1; /* only prcoordxy once */
                pline("set_wall_type: wall mode problems with: ");
            }
            pline("%s %d;", type_names[x], bad_count[x]);
        }
#endif /* WA_VERBOSE */
}

/* ------------------------------------------------------------------------ */
/* This matrix is used here and in vision.c. */
const seenV seenv_matrix[3][3] = {
    { SV2, SV1,   SV0 },
    { SV3, SVALL, SV7 },
    { SV4, SV5,   SV6 }
};

#define sign(z) ((z) < 0 ? -1 : ((z) > 0 ? 1 : 0))

/* Set the seen vector of lev as if seen from (x0,y0) to (x,y). */
static void
set_seenv(
    struct rm *lev,
    coordxy x0, coordxy y0, /* from */
    coordxy x, coordxy y)   /*  to  */
{
    coordxy dx = x - x0, dy = y0 - y;

    lev->seenv |= seenv_matrix[sign(dy) + 1][sign(dx) + 1];
}

/* Called by blackout(vault.c) when vault guard removes temporary corridor,
   turning spot <x0,y0> back into stone; <x1,y1> is an adjacent spot. */
void
unset_seenv(
    struct rm *lev,         /* &levl[x1][y1] */
    coordxy x0, coordxy y0, /* from */
    coordxy x1, coordxy y1) /*  to; abs(x1-x0)==1 && abs(y0-y1)==1 */

{
    coordxy dx = x1 - x0, dy = y0 - y1;

    lev->seenv &= ~seenv_matrix[dy + 1][dx + 1];
}

/* ------------------------------------------------------------------------ */

/* T wall types, one for each row in wall_matrix[][]. */
#define T_d 0
#define T_l 1
#define T_u 2
#define T_r 3

/*
 * These are the column names of wall_matrix[][].  They are the "results"
 * of a tdwall pattern match.  All T walls are rotated so they become
 * a tdwall.  Then we do a single pattern match, but return the
 * correct result for the original wall by using different rows for
 * each of the wall types.
 */
#define T_stone 0
#define T_tlcorn 1
#define T_trcorn 2
#define T_hwall 3
#define T_tdwall 4

static const int wall_matrix[4][5] = {
    { S_stone, S_tlcorn, S_trcorn, S_hwall, S_tdwall }, /* tdwall */
    { S_stone, S_trcorn, S_brcorn, S_vwall, S_tlwall }, /* tlwall */
    { S_stone, S_brcorn, S_blcorn, S_hwall, S_tuwall }, /* tuwall */
    { S_stone, S_blcorn, S_tlcorn, S_vwall, S_trwall }, /* trwall */
};

/* Cross wall types, one for each "solid" quarter.  Rows of cross_matrix[][].
 */
#define C_bl 0
#define C_tl 1
#define C_tr 2
#define C_br 3

/*
 * These are the column names for cross_matrix[][].  They express results
 * in C_br (bottom right) terms.  All crosswalls with a single solid
 * quarter are rotated so the solid section is at the bottom right.
 * We pattern match on that, but return the correct result depending
 * on which row we're looking at.
 */
#define C_trcorn 0
#define C_brcorn 1
#define C_blcorn 2
#define C_tlwall 3
#define C_tuwall 4
#define C_crwall 5

static const int cross_matrix[4][6] = {
    { S_brcorn, S_blcorn, S_tlcorn, S_tuwall, S_trwall, S_crwall },
    { S_blcorn, S_tlcorn, S_trcorn, S_trwall, S_tdwall, S_crwall },
    { S_tlcorn, S_trcorn, S_brcorn, S_tdwall, S_tlwall, S_crwall },
    { S_trcorn, S_brcorn, S_blcorn, S_tlwall, S_tuwall, S_crwall },
};

/* Print out a T wall warning and all interesting info. */
static void
t_warn(struct rm *lev)
{
    static const char warn_str[] = "wall_angle: %s: case %d: seenv = 0x%x";
    const char *wname;

    /* 3.7: non-T_wall cases added after shop repair (via breaching a wall,
       using locking magic to put a door there, then unlocking the door;
       D_CLOSED carried over to the wall) triggered warning for "unknown" */
    switch (lev->typ) {
    case TUWALL:
        wname = "tuwall";
        break;
    case TLWALL:
        wname = "tlwall";
        break;
    case TRWALL:
        wname = "trwall";
        break;
    case TDWALL:
        wname = "tdwall";
        break;
    case VWALL:
        wname = "vwall";
        break;
    case HWALL:
        wname = "hwall";
        break;
    case TLCORNER:
        wname = "tlcorner";
        break;
    case TRCORNER:
        wname = "trcorner";
        break;
    case BLCORNER:
        wname = "blcorner";
        break;
    case BRCORNER:
        wname = "brcorner";
        break;
    default:
        wname = "unknown";
        break;
    }
    impossible(warn_str, wname, lev->wall_info & WM_MASK,
               (unsigned int) lev->seenv);
}

/*
 * Return the correct graphics character index using wall type, wall mode,
 * and the seen vector.  It is expected that seenv is non zero.
 *
 * All T-wall vectors are rotated to be TDWALL.  All single crosswall
 * blocks are rotated to bottom right.  All double crosswall are rotated
 * to W_X_BLTR.  All results are converted back.
 *
 * The only way to understand this is to take out pen and paper and
 * draw diagrams.  See rm.h for more details on the wall modes and
 * seen vector (SV).
 */
static int
wall_angle(struct rm *lev)
{
    register unsigned int seenv = lev->seenv & 0xff;
    const int *row;
    int col, idx;

#define only(sv, bits) (((sv) & (bits)) && !((sv) & ~(bits)))
    switch (lev->typ) {
    case TUWALL:
        row = wall_matrix[T_u];
        seenv = (seenv >> 4 | seenv << 4) & 0xff; /* rotate to tdwall */
        goto do_twall;
    case TLWALL:
        row = wall_matrix[T_l];
        seenv = (seenv >> 2 | seenv << 6) & 0xff; /* rotate to tdwall */
        goto do_twall;
    case TRWALL:
        row = wall_matrix[T_r];
        seenv = (seenv >> 6 | seenv << 2) & 0xff; /* rotate to tdwall */
        goto do_twall;
    case TDWALL:
        row = wall_matrix[T_d];
 do_twall:
        switch (lev->wall_info & WM_MASK) {
        case 0:
            if (seenv == SV4) {
                col = T_tlcorn;
            } else if (seenv == SV6) {
                col = T_trcorn;
            } else if (seenv & (SV3 | SV5 | SV7)
                       || ((seenv & SV4) && (seenv & SV6))) {
                col = T_tdwall;
            } else if (seenv & (SV0 | SV1 | SV2)) {
                col = (seenv & (SV4 | SV6) ? T_tdwall : T_hwall);
            } else {
                t_warn(lev);
                col = T_stone;
            }
            break;
        case WM_T_LONG:
            if (seenv & (SV3 | SV4) && !(seenv & (SV5 | SV6 | SV7))) {
                col = T_tlcorn;
            } else if (seenv & (SV6 | SV7) && !(seenv & (SV3 | SV4 | SV5))) {
                col = T_trcorn;
            } else if ((seenv & SV5)
                       || ((seenv & (SV3 | SV4)) && (seenv & (SV6 | SV7)))) {
                col = T_tdwall;
            } else {
                /* only SV0|SV1|SV2 */
                if (!only(seenv, SV0 | SV1 | SV2))
                    t_warn(lev);
                col = T_stone;
            }
            break;
        case WM_T_BL:
#if 0  /* older method, fixed */
            if (only(seenv, SV4 | SV5)) {
                col = T_tlcorn;
            } else if ((seenv & (SV0 | SV1 | SV2))
                       && only(seenv, SV0 | SV1 | SV2 | SV6 | SV7)) {
                col = T_hwall;
            } else if ((seenv & SV3)
                       || ((seenv & (SV0 | SV1 | SV2))
                           && (seenv & (SV4 | SV5)))) {
                col = T_tdwall;
            } else {
                if (seenv != SV6)
                    t_warn(lev);
                col = T_stone;
            }
#endif /* 0 */
            if (only(seenv, SV4 | SV5))
                col = T_tlcorn;
            else if ((seenv & (SV0 | SV1 | SV2 | SV7))
                     && !(seenv & (SV3 | SV4 | SV5)))
                col = T_hwall;
            else if (only(seenv, SV6))
                col = T_stone;
            else
                col = T_tdwall;
            break;
        case WM_T_BR:
#if 0  /* older method, fixed */
            if (only(seenv, SV5 | SV6)) {
                col = T_trcorn;
            } else if ((seenv & (SV0 | SV1 | SV2))
                       && only(seenv, SV0 | SV1 | SV2 | SV3 | SV4)) {
                col = T_hwall;
            } else if ((seenv & SV7)
                       || ((seenv & (SV0 | SV1 | SV2))
                           && (seenv & (SV5 | SV6)))) {
                col = T_tdwall;
            } else {
                if (seenv != SV4)
                    t_warn(lev);
                col = T_stone;
            }
#endif /* 0 */
            if (only(seenv, SV5 | SV6))
                col = T_trcorn;
            else if ((seenv & (SV0 | SV1 | SV2 | SV3))
                     && !(seenv & (SV5 | SV6 | SV7)))
                col = T_hwall;
            else if (only(seenv, SV4))
                col = T_stone;
            else
                col = T_tdwall;

            break;
        default:
            impossible("wall_angle: unknown T wall mode %d",
                       lev->wall_info & WM_MASK);
            col = T_stone;
            break;
        }
        idx = row[col];
        break;

    case SDOOR:
        if (lev->horizontal)
            goto horiz;
        /*FALLTHRU*/
    case VWALL:
        switch (lev->wall_info & WM_MASK) {
        case 0:
            idx = seenv ? S_vwall : S_stone;
            break;
        case 1:
            idx = seenv & (SV1 | SV2 | SV3 | SV4 | SV5) ? S_vwall : S_stone;
            break;
        case 2:
            idx = seenv & (SV0 | SV1 | SV5 | SV6 | SV7) ? S_vwall : S_stone;
            break;
        default:
            impossible("wall_angle: unknown vwall mode %d",
                       lev->wall_info & WM_MASK);
            idx = S_stone;
            break;
        }
        break;

    case HWALL:
 horiz:
        switch (lev->wall_info & WM_MASK) {
        case 0:
            idx = seenv ? S_hwall : S_stone;
            break;
        case 1:
            idx = seenv & (SV3 | SV4 | SV5 | SV6 | SV7) ? S_hwall : S_stone;
            break;
        case 2:
            idx = seenv & (SV0 | SV1 | SV2 | SV3 | SV7) ? S_hwall : S_stone;
            break;
        default:
            impossible("wall_angle: unknown hwall mode %d",
                       lev->wall_info & WM_MASK);
            idx = S_stone;
            break;
        }
        break;

#define set_corner(idx, lev, which, outer, inner, name)    \
    switch ((lev)->wall_info & WM_MASK) {                  \
    case 0:                                                \
        idx = which;                                       \
        break;                                             \
    case WM_C_OUTER:                                       \
        idx = seenv & (outer) ? which : S_stone;           \
        break;                                             \
    case WM_C_INNER:                                       \
        idx = seenv & ~(inner) ? which : S_stone;          \
        break;                                             \
    default:                                               \
        impossible("wall_angle: unknown %s mode %d", name, \
                   (lev)->wall_info &WM_MASK);             \
        idx = S_stone;                                     \
        break;                                             \
    }

    case TLCORNER:
        set_corner(idx, lev, S_tlcorn, (SV3 | SV4 | SV5), SV4, "tlcorn");
        break;
    case TRCORNER:
        set_corner(idx, lev, S_trcorn, (SV5 | SV6 | SV7), SV6, "trcorn");
        break;
    case BLCORNER:
        set_corner(idx, lev, S_blcorn, (SV1 | SV2 | SV3), SV2, "blcorn");
        break;
    case BRCORNER:
        set_corner(idx, lev, S_brcorn, (SV7 | SV0 | SV1), SV0, "brcorn");
        break;

    case CROSSWALL:
        switch (lev->wall_info & WM_MASK) {
        case 0:
            if (seenv == SV0)
                idx = S_brcorn;
            else if (seenv == SV2)
                idx = S_blcorn;
            else if (seenv == SV4)
                idx = S_tlcorn;
            else if (seenv == SV6)
                idx = S_trcorn;
            else if (!(seenv & ~(SV0 | SV1 | SV2))
                     && (seenv & SV1 || seenv == (SV0 | SV2)))
                idx = S_tuwall;
            else if (!(seenv & ~(SV2 | SV3 | SV4))
                     && (seenv & SV3 || seenv == (SV2 | SV4)))
                idx = S_trwall;
            else if (!(seenv & ~(SV4 | SV5 | SV6))
                     && (seenv & SV5 || seenv == (SV4 | SV6)))
                idx = S_tdwall;
            else if (!(seenv & ~(SV0 | SV6 | SV7))
                     && (seenv & SV7 || seenv == (SV0 | SV6)))
                idx = S_tlwall;
            else
                idx = S_crwall;
            break;

        case WM_X_TL:
            row = cross_matrix[C_tl];
            seenv = (seenv >> 4 | seenv << 4) & 0xff;
            goto do_crwall;
        case WM_X_TR:
            row = cross_matrix[C_tr];
            seenv = (seenv >> 6 | seenv << 2) & 0xff;
            goto do_crwall;
        case WM_X_BL:
            row = cross_matrix[C_bl];
            seenv = (seenv >> 2 | seenv << 6) & 0xff;
            goto do_crwall;
        case WM_X_BR:
            row = cross_matrix[C_br];
 do_crwall:
            if (seenv == SV4) {
                idx = S_stone;
            } else {
                seenv = seenv & ~SV4; /* strip SV4 */
                if (seenv == SV0) {
                    col = C_brcorn;
                } else if (seenv & (SV2 | SV3)) {
                    if (seenv & (SV5 | SV6 | SV7))
                        col = C_crwall;
                    else if (seenv & (SV0 | SV1))
                        col = C_tuwall;
                    else
                        col = C_blcorn;
                } else if (seenv & (SV5 | SV6)) {
                    if (seenv & (SV1 | SV2 | SV3))
                        col = C_crwall;
                    else if (seenv & (SV0 | SV7))
                        col = C_tlwall;
                    else
                        col = C_trcorn;
                } else if (seenv & SV1) {
                    col = seenv & SV7 ? C_crwall : C_tuwall;
                } else if (seenv & SV7) {
                    col = seenv & SV1 ? C_crwall : C_tlwall;
                } else {
                    impossible("wall_angle: bottom of crwall check");
                    col = C_crwall;
                }

                idx = row[col];
            }
            break;

        case WM_X_TLBR:
            if (only(seenv, SV1 | SV2 | SV3))
                idx = S_blcorn;
            else if (only(seenv, SV5 | SV6 | SV7))
                idx = S_trcorn;
            else if (only(seenv, SV0 | SV4))
                idx = S_stone;
            else
                idx = S_crwall;
            break;

        case WM_X_BLTR:
            if (only(seenv, SV0 | SV1 | SV7))
                idx = S_brcorn;
            else if (only(seenv, SV3 | SV4 | SV5))
                idx = S_tlcorn;
            else if (only(seenv, SV2 | SV6))
                idx = S_stone;
            else
                idx = S_crwall;
            break;

        default:
            impossible("wall_angle: unknown crosswall mode");
            idx = S_stone;
            break;
        }
        break;

    default:
        impossible("wall_angle: unexpected wall type %d", lev->typ);
        idx = S_stone;
    }
    return idx;
}

/*
 * c++ 20 has problems with some of the display.h macros because
 * comparisons and bit-fiddling and math between different enums
 * is deprecated.
 * Create function versions of some of the macros used in some
 * NetHack c++ source files (Qt) for use there.
 */
int
fn_cmap_to_glyph(int cmap)
{
    return cmap_to_glyph(cmap);
}
/*display.c*/

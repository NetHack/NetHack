/* NetHack 3.6	vision.c	$NHDT-Date: 1448013598 2015/11/20 09:59:58 $  $NHDT-Branch: master $:$NHDT-Revision: 1.27 $ */
/* Copyright (c) Dean Luick, with acknowledgements to Dave Cohrs, 1990. */
/* NetHack may be freely redistributed.  See license for details.       */

#include "hack.h"

/* Circles
 * ==================================================================*/

/*
 * These numbers are limit offsets for one quadrant of a circle of a given
 * radius (the first number of each line) from the source.  The number in
 * the comment is the element number (so pointers can be set up).  Each
 * "circle" has as many elements as its radius+1.  The radius is the number
 * of points away from the source that the limit exists.  The radius of the
 * offset on the same row as the source *is* included so we don't have to
 * make an extra check.  For example, a circle of radius 4 has offsets:
 *
 *              XXX     +2
 *              ...X    +3
 *              ....X   +4
 *              ....X   +4
 *              @...X   +4
 *
 */
char circle_data[] = {
    /*  0*/ 1,  1,
    /*  2*/ 2,  2,  1,
    /*  5*/ 3,  3,  2,  1,
    /*  9*/ 4,  4,  4,  3,  2,
    /* 14*/ 5,  5,  5,  4,  3,  2,
    /* 20*/ 6,  6,  6,  5,  5,  4,  2,
    /* 27*/ 7,  7,  7,  6,  6,  5,  4,  2,
    /* 35*/ 8,  8,  8,  7,  7,  6,  6,  4,  2,
    /* 44*/ 9,  9,  9,  9,  8,  8,  7,  6,  5,  3,
    /* 54*/ 10, 10, 10, 10, 9,  9,  8,  7,  6,  5,  3,
    /* 65*/ 11, 11, 11, 11, 10, 10, 9,  9,  8,  7,  5,  3,
    /* 77*/ 12, 12, 12, 12, 11, 11, 10, 10, 9,  8,  7,  5,  3,
    /* 90*/ 13, 13, 13, 13, 12, 12, 12, 11, 10, 10, 9,  7,  6, 3,
    /*104*/ 14, 14, 14, 14, 13, 13, 13, 12, 12, 11, 10, 9,  8, 6, 3,
    /*119*/ 15, 15, 15, 15, 14, 14, 14, 13, 13, 12, 11, 10, 9, 8, 6, 3,
    /*135*/ 16 /* MAX_RADIUS+1; used to terminate range loops -dlc */
};

/*
 * These are the starting indexes into the circle_data[] array for a
 * circle of a given radius.
 */
char circle_start[] = {
    /*  */ 0, /* circles of radius zero are not used */
    /* 1*/ 0,
    /* 2*/ 2,
    /* 3*/ 5,
    /* 4*/ 9,
    /* 5*/ 14,
    /* 6*/ 20,
    /* 7*/ 27,
    /* 8*/ 35,
    /* 9*/ 44,
    /*10*/ 54,
    /*11*/ 65,
    /*12*/ 77,
    /*13*/ 90,
    /*14*/ 104,
    /*15*/ 119,
};

/*==========================================================================*/
/* Vision (arbitrary line of sight)
 * =========================================*/

/*------ global variables ------*/

#if 0 /* (moved to decl.c) */
/* True if we need to run a full vision recalculation. */
boolean vision_full_recalc = 0;

/* Pointers to the current vision array. */
char    **viz_array;
#endif
char *viz_rmin, *viz_rmax; /* current vision cs bounds */

/*------ local variables ------*/

static char could_see[2][ROWNO][COLNO]; /* vision work space */
static char *cs_rows0[ROWNO], *cs_rows1[ROWNO];
static char cs_rmin0[ROWNO], cs_rmax0[ROWNO];
static char cs_rmin1[ROWNO], cs_rmax1[ROWNO];

static char viz_clear[ROWNO][COLNO]; /* vision clear/blocked map */
static char *viz_clear_rows[ROWNO];

static char left_ptrs[ROWNO][COLNO]; /* LOS algorithm helpers */
static char right_ptrs[ROWNO][COLNO];

/* Forward declarations. */
STATIC_DCL void FDECL(fill_point, (int, int));
STATIC_DCL void FDECL(dig_point, (int, int));
STATIC_DCL void NDECL(view_init);
STATIC_DCL void FDECL(view_from, (int, int, char **, char *, char *, int,
                                  void (*)(int, int, genericptr_t),
                                  genericptr_t));
STATIC_DCL void FDECL(get_unused_cs, (char ***, char **, char **));
STATIC_DCL void FDECL(rogue_vision, (char **, char *, char *));

/* Macro definitions that I can't find anywhere. */
#define sign(z) ((z) < 0 ? -1 : ((z) ? 1 : 0))
#define v_abs(z) ((z) < 0 ? -(z) : (z)) /* don't use abs -- it may exist */

/*
 * vision_init()
 *
 * The one-time vision initialization routine.
 *
 * This must be called before mklev() is called in newgame() [allmain.c],
 * or before a game restore.   Else we die a horrible death.
 */
void
vision_init()
{
    int i;

    /* Set up the pointers. */
    for (i = 0; i < ROWNO; i++) {
        cs_rows0[i] = could_see[0][i];
        cs_rows1[i] = could_see[1][i];
        viz_clear_rows[i] = viz_clear[i];
    }

    /* Start out with cs0 as our current array */
    viz_array = cs_rows0;
    viz_rmin = cs_rmin0;
    viz_rmax = cs_rmax0;

    vision_full_recalc = 0;
    (void) memset((genericptr_t) could_see, 0, sizeof(could_see));

    /* Initialize the vision algorithm (currently C or D). */
    view_init();

#ifdef VISION_TABLES
    /* Note:  this initializer doesn't do anything except guarantee that
     *        we're linked properly.
     */
    vis_tab_init();
#endif
}

/*
 * does_block()
 *
 * Returns true if the level feature, object, or monster at (x,y) blocks
 * sight.
 */
int
does_block(x, y, lev)
int x, y;
register struct rm *lev;
{
    struct obj *obj;
    struct monst *mon;

    /* Features that block . . */
    if (IS_ROCK(lev->typ) || lev->typ == TREE
        || (IS_DOOR(lev->typ)
            && (lev->doormask & (D_CLOSED | D_LOCKED | D_TRAPPED))))
        return 1;

    if (lev->typ == CLOUD || lev->typ == WATER
        || (lev->typ == MOAT && Underwater))
        return 1;

    /* Boulders block light. */
    for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
        if (obj->otyp == BOULDER)
            return 1;

    /* Mimics mimicing a door or boulder or ... block light. */
    if ((mon = m_at(x, y)) && (!mon->minvis || See_invisible)
        && is_lightblocker_mappear(mon))
        return 1;

    return 0;
}

/*
 * vision_reset()
 *
 * This must be called *after* the levl[][] structure is set with the new
 * level and the level monsters and objects are in place.
 */
void
vision_reset()
{
    int y;
    register int x, i, dig_left, block;
    register struct rm *lev;

    /* Start out with cs0 as our current array */
    viz_array = cs_rows0;
    viz_rmin = cs_rmin0;
    viz_rmax = cs_rmax0;

    (void) memset((genericptr_t) could_see, 0, sizeof(could_see));

    /* Reset the pointers and clear so that we have a "full" dungeon. */
    (void) memset((genericptr_t) viz_clear, 0, sizeof(viz_clear));

    /* Dig the level */
    for (y = 0; y < ROWNO; y++) {
        dig_left = 0;
        block = TRUE; /* location (0,y) is always stone; it's !isok() */
        lev = &levl[1][y];
        for (x = 1; x < COLNO; x++, lev += ROWNO)
            if (block != (IS_ROCK(lev->typ) || does_block(x, y, lev))) {
                if (block) {
                    for (i = dig_left; i < x; i++) {
                        left_ptrs[y][i] = dig_left;
                        right_ptrs[y][i] = x - 1;
                    }
                } else {
                    i = dig_left;
                    if (dig_left)
                        dig_left--; /* point at first blocked point */
                    for (; i < x; i++) {
                        left_ptrs[y][i] = dig_left;
                        right_ptrs[y][i] = x;
                        viz_clear[y][i] = 1;
                    }
                }
                dig_left = x;
                block = !block;
            }
        /* handle right boundary; almost identical for blocked/unblocked */
        i = dig_left;
        if (!block && dig_left)
            dig_left--; /* point at first blocked point */
        for (; i < COLNO; i++) {
            left_ptrs[y][i] = dig_left;
            right_ptrs[y][i] = (COLNO - 1);
            viz_clear[y][i] = !block;
        }
    }

    iflags.vision_inited = 1; /* vision is ready */
    vision_full_recalc = 1;   /* we want to run vision_recalc() */
}

/*
 * get_unused_cs()
 *
 * Called from vision_recalc() and at least one light routine.  Get pointers
 * to the unused vision work area.
 */
STATIC_OVL void
get_unused_cs(rows, rmin, rmax)
char ***rows;
char **rmin, **rmax;
{
    register int row;
    register char *nrmin, *nrmax;

    if (viz_array == cs_rows0) {
        *rows = cs_rows1;
        *rmin = cs_rmin1;
        *rmax = cs_rmax1;
    } else {
        *rows = cs_rows0;
        *rmin = cs_rmin0;
        *rmax = cs_rmax0;
    }

    /* return an initialized, unused work area */
    nrmin = *rmin;
    nrmax = *rmax;

    (void) memset((genericptr_t) * *rows, 0,
                  ROWNO * COLNO);       /* we see nothing */
    for (row = 0; row < ROWNO; row++) { /* set row min & max */
        *nrmin++ = COLNO - 1;
        *nrmax++ = 0;
    }
}

/*
 * rogue_vision()
 *
 * Set the "could see" and in sight bits so vision acts just like the old
 * rogue game:
 *
 *      + If in a room, the hero can see to the room boundaries.
 *      + The hero can always see adjacent squares.
 *
 * We set the in_sight bit here as well to escape a bug that shows up
 * due to the one-sided lit wall hack.
 */
STATIC_OVL void
rogue_vision(next, rmin, rmax)
char **next; /* could_see array pointers */
char *rmin, *rmax;
{
    int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET; /* no SHARED... */
    int start, stop, in_door, xhi, xlo, yhi, ylo;
    register int zx, zy;

    /* If in a lit room, we are able to see to its boundaries. */
    /* If dark, set COULD_SEE so various spells work -dlc */
    if (rnum >= 0) {
        for (zy = rooms[rnum].ly - 1; zy <= rooms[rnum].hy + 1; zy++) {
            rmin[zy] = start = rooms[rnum].lx - 1;
            rmax[zy] = stop = rooms[rnum].hx + 1;

            for (zx = start; zx <= stop; zx++) {
                if (rooms[rnum].rlit) {
                    next[zy][zx] = COULD_SEE | IN_SIGHT;
                    levl[zx][zy].seenv = SVALL; /* see the walls */
                } else
                    next[zy][zx] = COULD_SEE;
            }
        }
    }

    in_door = levl[u.ux][u.uy].typ == DOOR;

    /* Can always see adjacent. */
    ylo = max(u.uy - 1, 0);
    yhi = min(u.uy + 1, ROWNO - 1);
    xlo = max(u.ux - 1, 1);
    xhi = min(u.ux + 1, COLNO - 1);
    for (zy = ylo; zy <= yhi; zy++) {
        if (xlo < rmin[zy])
            rmin[zy] = xlo;
        if (xhi > rmax[zy])
            rmax[zy] = xhi;

        for (zx = xlo; zx <= xhi; zx++) {
            next[zy][zx] = COULD_SEE | IN_SIGHT;
            /*
             * Yuck, update adjacent non-diagonal positions when in a doorway.
             * We need to do this to catch the case when we first step into
             * a room.  The room's walls were not seen from the outside, but
             * now are seen (the seen bits are set just above).  However, the
             * positions are not updated because they were already in sight.
             * So, we have to do it here.
             */
            if (in_door && (zx == u.ux || zy == u.uy))
                newsym(zx, zy);
        }
    }
}

/*#define EXTEND_SPINE*/ /* possibly better looking wall-angle */

#ifdef EXTEND_SPINE

STATIC_DCL int FDECL(new_angle, (struct rm *, unsigned char *, int, int));
/*
 * new_angle()
 *
 * Return the new angle seen by the hero for this location.  The angle
 * bit is given in the value pointed at by sv.
 *
 * For T walls and crosswall, just setting the angle bit, even though
 * it is technically correct, doesn't look good.  If we can see the
 * next position beyond the current one and it is a wall that we can
 * see, then we want to extend a spine of the T to connect with the wall
 * that is beyond.  Example:
 *
 *       Correct, but ugly                         Extend T spine
 *
 *              | ...                                   | ...
 *              | ...   <-- wall beyond & floor -->     | ...
 *              | ...                                   | ...
 * Unseen   -->   ...                                   | ...
 * spine        +-...   <-- trwall & doorway    -->     +-...
 *              | ...                                   | ...
 *
 *
 *                 @    <-- hero                -->        @
 *
 *
 * We fake the above check by only checking if the horizontal
 * & vertical positions adjacent to the crosswall and T wall are
 * unblocked.  Then, _in general_ we can see beyond.  Generally,
 * this is good enough.
 *
 *      + When this function is called we don't have all of the seen
 *        information (we're doing a top down scan in vision_recalc).
 *        We would need to scan once to set all IN_SIGHT and COULD_SEE
 *        bits, then again to correctly set the seenv bits.
 *      + I'm trying to make this as cheap as possible.  The display
 *        & vision eat up too much CPU time.
 *
 *
 * Note:  Even as I write this, I'm still not convinced.  There are too
 *        many exceptions.  I may have to bite the bullet and do more
 *        checks.       - Dean 2/11/93
 */
STATIC_OVL int
new_angle(lev, sv, row, col)
struct rm *lev;
unsigned char *sv;
int row, col;
{
    register int res = *sv;

    /*
     * Do extra checks for crosswalls and T walls if we see them from
     * an angle.
     */
    if (lev->typ >= CROSSWALL && lev->typ <= TRWALL) {
        switch (res) {
        case SV0:
            if (col > 0 && viz_clear[row][col - 1])
                res |= SV7;
            if (row > 0 && viz_clear[row - 1][col])
                res |= SV1;
            break;
        case SV2:
            if (row > 0 && viz_clear[row - 1][col])
                res |= SV1;
            if (col < COLNO - 1 && viz_clear[row][col + 1])
                res |= SV3;
            break;
        case SV4:
            if (col < COLNO - 1 && viz_clear[row][col + 1])
                res |= SV3;
            if (row < ROWNO - 1 && viz_clear[row + 1][col])
                res |= SV5;
            break;
        case SV6:
            if (row < ROWNO - 1 && viz_clear[row + 1][col])
                res |= SV5;
            if (col > 0 && viz_clear[row][col - 1])
                res |= SV7;
            break;
        }
    }
    return res;
}
#else
/*
 * new_angle()
 *
 * Return the new angle seen by the hero for this location.  The angle
 * bit is given in the value pointed at by sv.
 *
 * The other parameters are not used.
 */
#define new_angle(lev, sv, row, col) (*sv)

#endif

/*
 * vision_recalc()
 *
 * Do all of the heavy vision work.  Recalculate all locations that could
 * possibly be seen by the hero --- if the location were lit, etc.  Note
 * which locations are actually seen because of lighting.  Then add to
 * this all locations that be seen by hero due to night vision and x-ray
 * vision.  Finally, compare with what the hero was able to see previously.
 * Update the difference.
 *
 * This function is usually called only when the variable 'vision_full_recalc'
 * is set.  The following is a list of places where this function is called,
 * with three valid values for the control flag parameter:
 *
 * Control flag = 0.  A complete vision recalculation.  Generate the vision
 * tables from scratch.  This is necessary to correctly set what the hero
 * can see.  (1) and (2) call this routine for synchronization purposes, (3)
 * calls this routine so it can operate correctly.
 *
 *      + After the monster move, before input from the player. [moveloop()]
 *      + At end of moveloop. [moveloop() ??? not sure why this is here]
 *      + Right before something is printed. [pline()]
 *      + Right before we do a vision based operation. [do_clear_area()]
 *      + screen redraw, so we can renew all positions in sight. [docrt()]
 *      + When toggling temporary blindness, in case additional events
 *        impacted by vision occur during the same move [make_blinded()]
 *
 * Control flag = 1.  An adjacent vision recalculation.  The hero has moved
 * one square.  Knowing this, it might be possible to optimize the vision
 * recalculation using the current knowledge.  This is presently unimplemented
 * and is treated as a control = 0 call.
 *
 *      + Right after the hero moves. [domove()]
 *
 * Control flag = 2.  Turn off the vision system.  Nothing new will be
 * displayed, since nothing is seen.  This is usually done when you need
 * a newsym() run on all locations in sight, or on some locations but you
 * don't know which ones.
 *
 *      + Before a screen redraw, so all positions are renewed. [docrt()]
 *      + Right before the hero arrives on a new level. [goto_level()]
 *      + Right after a scroll of light is read. [litroom()]
 *      + After an option has changed that affects vision [parseoptions()]
 *      + Right after the hero is swallowed. [gulpmu()]
 *      + Just before bubbles are moved. [movebubbles()]
 */
void
vision_recalc(control)
int control;
{
    char **temp_array; /* points to the old vision array */
    char **next_array; /* points to the new vision array */
    char *next_row;    /* row pointer for the new array */
    char *old_row;     /* row pointer for the old array */
    char *next_rmin;   /* min pointer for the new array */
    char *next_rmax;   /* max pointer for the new array */
    char *ranges;      /* circle ranges -- used for xray & night vision */
    int row = 0;       /* row counter (outer loop)  */
    int start, stop;   /* inner loop starting/stopping index */
    int dx, dy;        /* one step from a lit door or lit wall (see below) */
    register int col;  /* inner loop counter */
    register struct rm *lev; /* pointer to current pos */
    struct rm *flev; /* pointer to position in "front" of current pos */
    extern unsigned char seenv_matrix[3][3]; /* from display.c */
    static unsigned char colbump[COLNO + 1]; /* cols to bump sv */
    unsigned char *sv;                       /* ptr to seen angle bits */
    int oldseenv;                            /* previous seenv value */

    vision_full_recalc = 0; /* reset flag */
    if (in_mklev || !iflags.vision_inited)
        return;

    /*
     * Either the light sources have been taken care of, or we must
     * recalculate them here.
     */

    /* Get the unused could see, row min, and row max arrays. */
    get_unused_cs(&next_array, &next_rmin, &next_rmax);

    /* You see nothing, nothing can see you --- if swallowed or refreshing. */
    if (u.uswallow || control == 2) {
        /* do nothing -- get_unused_cs() nulls out the new work area */
        ;
    } else if (Blind) {
        /*
         * Calculate the could_see array even when blind so that monsters
         * can see you, even if you can't see them.  Note that the current
         * setup allows:
         *
         *      + Monsters to see with the "new" vision, even on the rogue
         *        level.
         *
         *      + Monsters can see you even when you're in a pit.
         */
        view_from(u.uy, u.ux, next_array, next_rmin, next_rmax, 0,
                  (void FDECL((*), (int, int, genericptr_t))) 0,
                  (genericptr_t) 0);

        /*
         * Our own version of the update loop below.  We know we can't see
         * anything, so we only need update positions we used to be able
         * to see.
         */
        temp_array = viz_array; /* set viz_array so newsym() will work */
        viz_array = next_array;

        for (row = 0; row < ROWNO; row++) {
            old_row = temp_array[row];

            /* Find the min and max positions on the row. */
            start = min(viz_rmin[row], next_rmin[row]);
            stop = max(viz_rmax[row], next_rmax[row]);

            for (col = start; col <= stop; col++)
                if (old_row[col] & IN_SIGHT)
                    newsym(col, row);
        }

        /* skip the normal update loop */
        goto skip;
    } else if (Is_rogue_level(&u.uz)) {
        rogue_vision(next_array, next_rmin, next_rmax);
    } else {
        int has_night_vision = 1; /* hero has night vision */

        if (Underwater && !Is_waterlevel(&u.uz)) {
            /*
             * The hero is under water.  Only see surrounding locations if
             * they are also underwater.  This overrides night vision but
             * does not override x-ray vision.
             */
            has_night_vision = 0;

            for (row = u.uy - 1; row <= u.uy + 1; row++)
                for (col = u.ux - 1; col <= u.ux + 1; col++) {
                    if (!isok(col, row) || !is_pool(col, row))
                        continue;

                    next_rmin[row] = min(next_rmin[row], col);
                    next_rmax[row] = max(next_rmax[row], col);
                    next_array[row][col] = IN_SIGHT | COULD_SEE;
                }

        /* if in a pit, just update for immediate locations */
        } else if (u.utrap && u.utraptype == TT_PIT) {
            for (row = u.uy - 1; row <= u.uy + 1; row++) {
                if (row < 0)
                    continue;
                if (row >= ROWNO)
                    break;

                next_rmin[row] = max(0, u.ux - 1);
                next_rmax[row] = min(COLNO - 1, u.ux + 1);
                next_row = next_array[row];

                for (col = next_rmin[row]; col <= next_rmax[row]; col++)
                    next_row[col] = IN_SIGHT | COULD_SEE;
            }
        } else
            view_from(u.uy, u.ux, next_array, next_rmin, next_rmax, 0,
                      (void FDECL((*), (int, int, genericptr_t))) 0,
                      (genericptr_t) 0);

        /*
         * Set the IN_SIGHT bit for xray and night vision.
         */
        if (u.xray_range >= 0) {
            if (u.xray_range) {
                ranges = circle_ptr(u.xray_range);

                for (row = u.uy - u.xray_range; row <= u.uy + u.xray_range;
                     row++) {
                    if (row < 0)
                        continue;
                    if (row >= ROWNO)
                        break;
                    dy = v_abs(u.uy - row);
                    next_row = next_array[row];

                    start = max(0, u.ux - ranges[dy]);
                    stop = min(COLNO - 1, u.ux + ranges[dy]);

                    for (col = start; col <= stop; col++) {
                        char old_row_val = next_row[col];
                        next_row[col] |= IN_SIGHT;
                        oldseenv = levl[col][row].seenv;
                        levl[col][row].seenv = SVALL; /* see all! */
                        /* Update if previously not in sight or new angle. */
                        if (!(old_row_val & IN_SIGHT) || oldseenv != SVALL)
                            newsym(col, row);
                    }

                    next_rmin[row] = min(start, next_rmin[row]);
                    next_rmax[row] = max(stop, next_rmax[row]);
                }

            } else { /* range is 0 */
                next_array[u.uy][u.ux] |= IN_SIGHT;
                levl[u.ux][u.uy].seenv = SVALL;
                next_rmin[u.uy] = min(u.ux, next_rmin[u.uy]);
                next_rmax[u.uy] = max(u.ux, next_rmax[u.uy]);
            }
        }

        if (has_night_vision && u.xray_range < u.nv_range) {
            if (!u.nv_range) { /* range is 0 */
                next_array[u.uy][u.ux] |= IN_SIGHT;
                levl[u.ux][u.uy].seenv = SVALL;
                next_rmin[u.uy] = min(u.ux, next_rmin[u.uy]);
                next_rmax[u.uy] = max(u.ux, next_rmax[u.uy]);
            } else if (u.nv_range > 0) {
                ranges = circle_ptr(u.nv_range);

                for (row = u.uy - u.nv_range; row <= u.uy + u.nv_range;
                     row++) {
                    if (row < 0)
                        continue;
                    if (row >= ROWNO)
                        break;
                    dy = v_abs(u.uy - row);
                    next_row = next_array[row];

                    start = max(0, u.ux - ranges[dy]);
                    stop = min(COLNO - 1, u.ux + ranges[dy]);

                    for (col = start; col <= stop; col++)
                        if (next_row[col])
                            next_row[col] |= IN_SIGHT;

                    next_rmin[row] = min(start, next_rmin[row]);
                    next_rmax[row] = max(stop, next_rmax[row]);
                }
            }
        }
    }

    /* Set the correct bits for all light sources. */
    do_light_sources(next_array);

    /*
     * Make the viz_array the new array so that cansee() will work correctly.
     */
    temp_array = viz_array;
    viz_array = next_array;

    /*
     * The main update loop.  Here we do two things:
     *
     *      + Set the IN_SIGHT bit for places that we could see and are lit.
     *      + Reset changed places.
     *
     * There is one thing that make deciding what the hero can see
     * difficult:
     *
     *  1.  Directional lighting.  Items that block light create problems.
     *      The worst offenders are doors.  Suppose a door to a lit room
     *      is closed.  It is lit on one side, but not on the other.  How
     *      do you know?  You have to check the closest adjacent position.
     *      Even so, that is not entirely correct.  But it seems close
     *      enough for now.
     */
    colbump[u.ux] = colbump[u.ux + 1] = 1;
    for (row = 0; row < ROWNO; row++) {
        dy = u.uy - row;
        dy = sign(dy);
        next_row = next_array[row];
        old_row = temp_array[row];

        /* Find the min and max positions on the row. */
        start = min(viz_rmin[row], next_rmin[row]);
        stop = max(viz_rmax[row], next_rmax[row]);
        lev = &levl[start][row];

        sv = &seenv_matrix[dy + 1][start < u.ux ? 0 : (start > u.ux ? 2 : 1)];

        for (col = start; col <= stop;
             lev += ROWNO, sv += (int) colbump[++col]) {
            if (next_row[col] & IN_SIGHT) {
                /*
                 * We see this position because of night- or xray-vision.
                 */
                oldseenv = lev->seenv;
                lev->seenv |=
                    new_angle(lev, sv, row, col); /* update seen angle */

                /* Update pos if previously not in sight or new angle. */
                if (!(old_row[col] & IN_SIGHT) || oldseenv != lev->seenv)
                    newsym(col, row);

            } else if ((next_row[col] & COULD_SEE)
                     && (lev->lit || (next_row[col] & TEMP_LIT))) {
                /*
                 * We see this position because it is lit.
                 */
                if ((IS_DOOR(lev->typ) || lev->typ == SDOOR
                     || IS_WALL(lev->typ)) && !viz_clear[row][col]) {
                    /*
                     * Make sure doors, walls, boulders or mimics don't show
                     * up
                     * at the end of dark hallways.  We do this by checking
                     * the adjacent position.  If it is lit, then we can see
                     * the door or wall, otherwise we can't.
                     */
                    dx = u.ux - col;
                    dx = sign(dx);
                    flev = &(levl[col + dx][row + dy]);
                    if (flev->lit
                        || next_array[row + dy][col + dx] & TEMP_LIT) {
                        next_row[col] |= IN_SIGHT; /* we see it */

                        oldseenv = lev->seenv;
                        lev->seenv |= new_angle(lev, sv, row, col);

                        /* Update pos if previously not in sight or new
                         * angle.*/
                        if (!(old_row[col] & IN_SIGHT)
                            || oldseenv != lev->seenv)
                            newsym(col, row);
                    } else
                        goto not_in_sight; /* we don't see it */

                } else {
                    next_row[col] |= IN_SIGHT; /* we see it */

                    oldseenv = lev->seenv;
                    lev->seenv |= new_angle(lev, sv, row, col);

                    /* Update pos if previously not in sight or new angle. */
                    if (!(old_row[col] & IN_SIGHT) || oldseenv != lev->seenv)
                        newsym(col, row);
                }
            } else if ((next_row[col] & COULD_SEE) && lev->waslit) {
                /*
                 * If we make it here, the hero _could see_ the location,
                 * but doesn't see it (location is not lit).
                 * However, the hero _remembers_ it as lit (waslit is true).
                 * The hero can now see that it is not lit, so change waslit
                 * and update the location.
                 */
                lev->waslit = 0; /* remember lit condition */
                newsym(col, row);

            /*
             * At this point we know that the row position is *not* in normal
             * sight.  That is, the position is could be seen, but is dark
             * or LOS is just plain blocked.
             *
             * Update the position if:
             * o If the old one *was* in sight.  We may need to clean up
             *   the glyph -- E.g. darken room spot, etc.
             * o If we now could see the location (yet the location is not
             *   lit), but previously we couldn't see the location, or vice
             *   versa.  Update the spot because there there may be an
             *   infrared monster there.
             */
            } else {
            not_in_sight:
                if ((old_row[col] & IN_SIGHT)
                    || ((next_row[col] & COULD_SEE)
                        ^ (old_row[col] & COULD_SEE)))
                    newsym(col, row);
            }

        } /* end for col . . */
    }     /* end for row . .  */
    colbump[u.ux] = colbump[u.ux + 1] = 0;

skip:
    /* This newsym() caused a crash delivering msg about failure to open
     * dungeon file init_dungeons() -> panic() -> done(11) ->
     * vision_recalc(2) -> newsym() -> crash!  u.ux and u.uy are 0 and
     * program_state.panicking == 1 under those circumstances
     */
    if (!program_state.panicking)
        newsym(u.ux, u.uy); /* Make sure the hero shows up! */

    /* Set the new min and max pointers. */
    viz_rmin = next_rmin;
    viz_rmax = next_rmax;

    recalc_mapseen();
}

/*
 * block_point()
 *
 * Make the location opaque to light.
 */
void
block_point(x, y)
int x, y;
{
    fill_point(y, x);

    /* recalc light sources here? */

    /*
     * We have to do a full vision recalculation if we "could see" the
     * location.  Why? Suppose some monster opened a way so that the
     * hero could see a lit room.  However, the position of the opening
     * was out of night-vision range of the hero.  Suddenly the hero should
     * see the lit room.
     */
    if (viz_array[y][x])
        vision_full_recalc = 1;
}

/*
 * unblock_point()
 *
 * Make the location transparent to light.
 */
void
unblock_point(x, y)
int x, y;
{
    dig_point(y, x);

    /* recalc light sources here? */

    if (viz_array[y][x])
        vision_full_recalc = 1;
}

/*==========================================================================*\
 :                                                                          :
 :      Everything below this line uses (y,x) instead of (x,y) --- the      :
 :      algorithms are faster if they are less recursive and can scan       :
 :      on a row longer.                                                    :
 :                                                                          :
\*==========================================================================*/

/* ======================================================================= *\
                        Left and Right Pointer Updates
\* ======================================================================= */

/*
 *              LEFT and RIGHT pointer rules
 *
 *
 * **NOTE**  The rules changed on 4/4/90.  This comment reflects the
 * new rules.  The change was so that the stone-wall optimization
 * would work.
 *
 * OK, now the tough stuff.  We must maintain our left and right
 * row pointers.  The rules are as follows:
 *
 * Left Pointers:
 * ______________
 *
 * + If you are a clear spot, your left will point to the first
 *   stone to your left.  If there is none, then point the first
 *   legal position in the row (0).
 *
 * + If you are a blocked spot, then your left will point to the
 *   left-most blocked spot to your left that is connected to you.
 *   This means that a left-edge (a blocked spot that has an open
 *   spot on its left) will point to itself.
 *
 *
 * Right Pointers:
 * ---------------
 * + If you are a clear spot, your right will point to the first
 *   stone to your right.  If there is none, then point the last
 *   legal position in the row (COLNO-1).
 *
 * + If you are a blocked spot, then your right will point to the
 *   right-most blocked spot to your right that is connected to you.
 *   This means that a right-edge (a blocked spot that has an open
 *    spot on its right) will point to itself.
 */
STATIC_OVL void
dig_point(row, col)
int row, col;
{
    int i;

    if (viz_clear[row][col])
        return; /* already done */

    viz_clear[row][col] = 1;

    /*
     * Boundary cases first.
     */
    if (col == 0) { /* left edge */
        if (viz_clear[row][1]) {
            right_ptrs[row][0] = right_ptrs[row][1];
        } else {
            right_ptrs[row][0] = 1;
            for (i = 1; i <= right_ptrs[row][1]; i++)
                left_ptrs[row][i] = 1;
        }
    } else if (col == (COLNO - 1)) { /* right edge */

        if (viz_clear[row][COLNO - 2]) {
            left_ptrs[row][COLNO - 1] = left_ptrs[row][COLNO - 2];
        } else {
            left_ptrs[row][COLNO - 1] = COLNO - 2;
            for (i = left_ptrs[row][COLNO - 2]; i < COLNO - 1; i++)
                right_ptrs[row][i] = COLNO - 2;
        }

    /*
     * At this point, we know we aren't on the boundaries.
     */
    } else if (viz_clear[row][col - 1] && viz_clear[row][col + 1]) {
        /* Both sides clear */
        for (i = left_ptrs[row][col - 1]; i <= col; i++) {
            if (!viz_clear[row][i])
                continue; /* catch non-end case */
            right_ptrs[row][i] = right_ptrs[row][col + 1];
        }
        for (i = col; i <= right_ptrs[row][col + 1]; i++) {
            if (!viz_clear[row][i])
                continue; /* catch non-end case */
            left_ptrs[row][i] = left_ptrs[row][col - 1];
        }

    } else if (viz_clear[row][col - 1]) {
        /* Left side clear, right side blocked. */
        for (i = col + 1; i <= right_ptrs[row][col + 1]; i++)
            left_ptrs[row][i] = col + 1;

        for (i = left_ptrs[row][col - 1]; i <= col; i++) {
            if (!viz_clear[row][i])
                continue; /* catch non-end case */
            right_ptrs[row][i] = col + 1;
        }
        left_ptrs[row][col] = left_ptrs[row][col - 1];

    } else if (viz_clear[row][col + 1]) {
        /* Right side clear, left side blocked. */
        for (i = left_ptrs[row][col - 1]; i < col; i++)
            right_ptrs[row][i] = col - 1;

        for (i = col; i <= right_ptrs[row][col + 1]; i++) {
            if (!viz_clear[row][i])
                continue; /* catch non-end case */
            left_ptrs[row][i] = col - 1;
        }
        right_ptrs[row][col] = right_ptrs[row][col + 1];

    } else {
        /* Both sides blocked */
        for (i = left_ptrs[row][col - 1]; i < col; i++)
            right_ptrs[row][i] = col - 1;

        for (i = col + 1; i <= right_ptrs[row][col + 1]; i++)
            left_ptrs[row][i] = col + 1;

        left_ptrs[row][col] = col - 1;
        right_ptrs[row][col] = col + 1;
    }
}

STATIC_OVL void
fill_point(row, col)
int row, col;
{
    int i;

    if (!viz_clear[row][col])
        return;

    viz_clear[row][col] = 0;

    if (col == 0) {
        if (viz_clear[row][1]) { /* adjacent is clear */
            right_ptrs[row][0] = 0;
        } else {
            right_ptrs[row][0] = right_ptrs[row][1];
            for (i = 1; i <= right_ptrs[row][1]; i++)
                left_ptrs[row][i] = 0;
        }
    } else if (col == COLNO - 1) {
        if (viz_clear[row][COLNO - 2]) { /* adjacent is clear */
            left_ptrs[row][COLNO - 1] = COLNO - 1;
        } else {
            left_ptrs[row][COLNO - 1] = left_ptrs[row][COLNO - 2];
            for (i = left_ptrs[row][COLNO - 2]; i < COLNO - 1; i++)
                right_ptrs[row][i] = COLNO - 1;
        }

    /*
     * Else we know that we are not on an edge.
     */
    } else if (viz_clear[row][col - 1] && viz_clear[row][col + 1]) {
        /* Both sides clear */
        for (i = left_ptrs[row][col - 1] + 1; i <= col; i++)
            right_ptrs[row][i] = col;

        if (!left_ptrs[row][col - 1]) /* catch the end case */
            right_ptrs[row][0] = col;

        for (i = col; i < right_ptrs[row][col + 1]; i++)
            left_ptrs[row][i] = col;

        if (right_ptrs[row][col + 1] == COLNO - 1) /* catch the end case */
            left_ptrs[row][COLNO - 1] = col;

    } else if (viz_clear[row][col - 1]) {
        /* Left side clear, right side blocked. */
        for (i = col; i <= right_ptrs[row][col + 1]; i++)
            left_ptrs[row][i] = col;

        for (i = left_ptrs[row][col - 1] + 1; i < col; i++)
            right_ptrs[row][i] = col;

        if (!left_ptrs[row][col - 1]) /* catch the end case */
            right_ptrs[row][i] = col;

        right_ptrs[row][col] = right_ptrs[row][col + 1];

    } else if (viz_clear[row][col + 1]) {
        /* Right side clear, left side blocked. */
        for (i = left_ptrs[row][col - 1]; i <= col; i++)
            right_ptrs[row][i] = col;

        for (i = col + 1; i < right_ptrs[row][col + 1]; i++)
            left_ptrs[row][i] = col;

        if (right_ptrs[row][col + 1] == COLNO - 1) /* catch the end case */
            left_ptrs[row][i] = col;

        left_ptrs[row][col] = left_ptrs[row][col - 1];

    } else {
        /* Both sides blocked */
        for (i = left_ptrs[row][col - 1]; i <= col; i++)
            right_ptrs[row][i] = right_ptrs[row][col + 1];

        for (i = col; i <= right_ptrs[row][col + 1]; i++)
            left_ptrs[row][i] = left_ptrs[row][col - 1];
    }
}

/*==========================================================================*/
/*==========================================================================*/
/* Use either algorithm C or D.  See the config.h for more details.
 * =========*/

/*
 * Variables local to both Algorithms C and D.
 */
static int start_row;
static int start_col;
static int step;
static char **cs_rows;
static char *cs_left;
static char *cs_right;

static void FDECL((*vis_func), (int, int, genericptr_t));
static genericptr_t varg;

/*
 * Both Algorithms C and D use the following macros.
 *
 *      good_row(z)       - Return TRUE if the argument is a legal row.
 *      set_cs(rowp,col)  - Set the local could see array.
 *      set_min(z)        - Save the min value of the argument and the current
 *                            row minimum.
 *      set_max(z)        - Save the max value of the argument and the current
 *                            row maximum.
 *
 * The last three macros depend on having local pointers row_min, row_max,
 * and rowp being set correctly.
 */
#define set_cs(rowp, col) (rowp[col] = COULD_SEE)
#define good_row(z) ((z) >= 0 && (z) < ROWNO)
#define set_min(z)      \
    if (*row_min > (z)) \
        *row_min = (z)
#define set_max(z)      \
    if (*row_max < (z)) \
        *row_max = (z)
#define is_clear(row, col) viz_clear_rows[row][col]

/*
 * clear_path()         expanded into 4 macros/functions:
 *
 *      q1_path()
 *      q2_path()
 *      q3_path()
 *      q4_path()
 *
 * "Draw" a line from the start to the given location.  Stop if we hit
 * something that blocks light.  The start and finish points themselves are
 * not checked, just the points between them.  These routines do _not_
 * expect to be called with the same starting and stopping point.
 *
 * These routines use the generalized integer Bresenham's algorithm (fast
 * line drawing) for all quadrants.  The algorithm was taken from _Procedural
 * Elements for Computer Graphics_, by David F. Rogers.  McGraw-Hill, 1985.
 */
#ifdef MACRO_CPATH /* quadrant calls are macros */

/*
 * When called, the result is in "result".
 * The first two arguments (srow,scol) are one end of the path.  The next
 * two arguments (row,col) are the destination.  The last argument is
 * used as a C language label.  This means that it must be different
 * in each pair of calls.
 */

/*
 *  Quadrant I (step < 0).
 */
#define q1_path(srow, scol, y2, x2, label)           \
    {                                                \
        int dx, dy;                                  \
        register int k, err, x, y, dxs, dys;         \
                                                     \
        x = (scol);                                  \
        y = (srow);                                  \
        dx = (x2) -x;                                \
        dy = y - (y2);                               \
                                                     \
        result = 0; /* default to a blocked path */  \
                                                     \
        dxs = dx << 1; /* save the shifted values */ \
        dys = dy << 1;                               \
        if (dy > dx) {                               \
            err = dxs - dy;                          \
                                                     \
            for (k = dy - 1; k; k--) {               \
                if (err >= 0) {                      \
                    x++;                             \
                    err -= dys;                      \
                }                                    \
                y--;                                 \
                err += dxs;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
        } else {                                     \
            err = dys - dx;                          \
                                                     \
            for (k = dx - 1; k; k--) {               \
                if (err >= 0) {                      \
                    y--;                             \
                    err -= dxs;                      \
                }                                    \
                x++;                                 \
                err += dys;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
        }                                            \
                                                     \
        result = 1;                                  \
    }

/*
 * Quadrant IV (step > 0).
 */
#define q4_path(srow, scol, y2, x2, label)           \
    {                                                \
        int dx, dy;                                  \
        register int k, err, x, y, dxs, dys;         \
                                                     \
        x = (scol);                                  \
        y = (srow);                                  \
        dx = (x2) -x;                                \
        dy = (y2) -y;                                \
                                                     \
        result = 0; /* default to a blocked path */  \
                                                     \
        dxs = dx << 1; /* save the shifted values */ \
        dys = dy << 1;                               \
        if (dy > dx) {                               \
            err = dxs - dy;                          \
                                                     \
            for (k = dy - 1; k; k--) {               \
                if (err >= 0) {                      \
                    x++;                             \
                    err -= dys;                      \
                }                                    \
                y++;                                 \
                err += dxs;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
                                                     \
        } else {                                     \
            err = dys - dx;                          \
                                                     \
            for (k = dx - 1; k; k--) {               \
                if (err >= 0) {                      \
                    y++;                             \
                    err -= dxs;                      \
                }                                    \
                x++;                                 \
                err += dys;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
        }                                            \
                                                     \
        result = 1;                                  \
    }

/*
 * Quadrant II (step < 0).
 */
#define q2_path(srow, scol, y2, x2, label)           \
    {                                                \
        int dx, dy;                                  \
        register int k, err, x, y, dxs, dys;         \
                                                     \
        x = (scol);                                  \
        y = (srow);                                  \
        dx = x - (x2);                               \
        dy = y - (y2);                               \
                                                     \
        result = 0; /* default to a blocked path */  \
                                                     \
        dxs = dx << 1; /* save the shifted values */ \
        dys = dy << 1;                               \
        if (dy > dx) {                               \
            err = dxs - dy;                          \
                                                     \
            for (k = dy - 1; k; k--) {               \
                if (err >= 0) {                      \
                    x--;                             \
                    err -= dys;                      \
                }                                    \
                y--;                                 \
                err += dxs;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
        } else {                                     \
            err = dys - dx;                          \
                                                     \
            for (k = dx - 1; k; k--) {               \
                if (err >= 0) {                      \
                    y--;                             \
                    err -= dxs;                      \
                }                                    \
                x--;                                 \
                err += dys;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
        }                                            \
                                                     \
        result = 1;                                  \
    }

/*
 * Quadrant III (step > 0).
 */
#define q3_path(srow, scol, y2, x2, label)           \
    {                                                \
        int dx, dy;                                  \
        register int k, err, x, y, dxs, dys;         \
                                                     \
        x = (scol);                                  \
        y = (srow);                                  \
        dx = x - (x2);                               \
        dy = (y2) -y;                                \
                                                     \
        result = 0; /* default to a blocked path */  \
                                                     \
        dxs = dx << 1; /* save the shifted values */ \
        dys = dy << 1;                               \
        if (dy > dx) {                               \
            err = dxs - dy;                          \
                                                     \
            for (k = dy - 1; k; k--) {               \
                if (err >= 0) {                      \
                    x--;                             \
                    err -= dys;                      \
                }                                    \
                y++;                                 \
                err += dxs;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
                                                     \
        } else {                                     \
            err = dys - dx;                          \
                                                     \
            for (k = dx - 1; k; k--) {               \
                if (err >= 0) {                      \
                    y++;                             \
                    err -= dxs;                      \
                }                                    \
                x--;                                 \
                err += dys;                          \
                if (!is_clear(y, x))                 \
                    goto label; /* blocked */        \
            }                                        \
        }                                            \
                                                     \
        result = 1;                                  \
    }

#else /* !MACRO_CPATH -- quadrants are really functions */

STATIC_DCL int FDECL(_q1_path, (int, int, int, int));
STATIC_DCL int FDECL(_q2_path, (int, int, int, int));
STATIC_DCL int FDECL(_q3_path, (int, int, int, int));
STATIC_DCL int FDECL(_q4_path, (int, int, int, int));

#define q1_path(sy, sx, y, x, dummy) result = _q1_path(sy, sx, y, x)
#define q2_path(sy, sx, y, x, dummy) result = _q2_path(sy, sx, y, x)
#define q3_path(sy, sx, y, x, dummy) result = _q3_path(sy, sx, y, x)
#define q4_path(sy, sx, y, x, dummy) result = _q4_path(sy, sx, y, x)

/*
 * Quadrant I (step < 0).
 */
STATIC_OVL int
_q1_path(srow, scol, y2, x2)
int scol, srow, y2, x2;
{
    int dx, dy;
    register int k, err, x, y, dxs, dys;

    x = scol;
    y = srow;
    dx = x2 - x;
    dy = y - y2;

    dxs = dx << 1; /* save the shifted values */
    dys = dy << 1;
    if (dy > dx) {
        err = dxs - dy;

        for (k = dy - 1; k; k--) {
            if (err >= 0) {
                x++;
                err -= dys;
            }
            y--;
            err += dxs;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    } else {
        err = dys - dx;

        for (k = dx - 1; k; k--) {
            if (err >= 0) {
                y--;
                err -= dxs;
            }
            x++;
            err += dys;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    }

    return 1;
}

/*
 * Quadrant IV (step > 0).
 */
STATIC_OVL int
_q4_path(srow, scol, y2, x2)
int scol, srow, y2, x2;
{
    int dx, dy;
    register int k, err, x, y, dxs, dys;

    x = scol;
    y = srow;
    dx = x2 - x;
    dy = y2 - y;

    dxs = dx << 1; /* save the shifted values */
    dys = dy << 1;
    if (dy > dx) {
        err = dxs - dy;

        for (k = dy - 1; k; k--) {
            if (err >= 0) {
                x++;
                err -= dys;
            }
            y++;
            err += dxs;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    } else {
        err = dys - dx;

        for (k = dx - 1; k; k--) {
            if (err >= 0) {
                y++;
                err -= dxs;
            }
            x++;
            err += dys;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    }

    return 1;
}

/*
 * Quadrant II (step < 0).
 */
STATIC_OVL int
_q2_path(srow, scol, y2, x2)
int scol, srow, y2, x2;
{
    int dx, dy;
    register int k, err, x, y, dxs, dys;

    x = scol;
    y = srow;
    dx = x - x2;
    dy = y - y2;

    dxs = dx << 1; /* save the shifted values */
    dys = dy << 1;
    if (dy > dx) {
        err = dxs - dy;

        for (k = dy - 1; k; k--) {
            if (err >= 0) {
                x--;
                err -= dys;
            }
            y--;
            err += dxs;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    } else {
        err = dys - dx;

        for (k = dx - 1; k; k--) {
            if (err >= 0) {
                y--;
                err -= dxs;
            }
            x--;
            err += dys;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    }

    return 1;
}

/*
 * Quadrant III (step > 0).
 */
STATIC_OVL int
_q3_path(srow, scol, y2, x2)
int scol, srow, y2, x2;
{
    int dx, dy;
    register int k, err, x, y, dxs, dys;

    x = scol;
    y = srow;
    dx = x - x2;
    dy = y2 - y;

    dxs = dx << 1; /* save the shifted values */
    dys = dy << 1;
    if (dy > dx) {
        err = dxs - dy;

        for (k = dy - 1; k; k--) {
            if (err >= 0) {
                x--;
                err -= dys;
            }
            y++;
            err += dxs;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    } else {
        err = dys - dx;

        for (k = dx - 1; k; k--) {
            if (err >= 0) {
                y++;
                err -= dxs;
            }
            x--;
            err += dys;
            if (!is_clear(y, x))
                return 0; /* blocked */
        }
    }

    return 1;
}

#endif /* ?MACRO_CPATH */

/*
 * Use vision tables to determine if there is a clear path from
 * (col1,row1) to (col2,row2).  This is used by:
 *      m_cansee()
 *      m_canseeu()
 *      do_light_sources()
 */
boolean
clear_path(col1, row1, col2, row2)
int col1, row1, col2, row2;
{
    int result;

    if (col1 < col2) {
        if (row1 > row2) {
            q1_path(row1, col1, row2, col2, cleardone);
        } else {
            q4_path(row1, col1, row2, col2, cleardone);
        }
    } else {
        if (row1 > row2) {
            q2_path(row1, col1, row2, col2, cleardone);
        } else if (row1 == row2 && col1 == col2) {
            result = 1;
        } else {
            q3_path(row1, col1, row2, col2, cleardone);
        }
    }
#ifdef MACRO_CPATH
cleardone:
#endif
    return (boolean) result;
}

#ifdef VISION_TABLES
/*==========================================================================*\
                            GENERAL LINE OF SIGHT
                                Algorithm D
\*==========================================================================*/

/*
 * Indicate caller for the shadow routines.
 */
#define FROM_RIGHT 0
#define FROM_LEFT 1

/*
 * Include the table definitions.
 */
#include "vis_tab.h"

/* 3D table pointers. */
static close2d *close_dy[CLOSE_MAX_BC_DY];
static far2d *far_dy[FAR_MAX_BC_DY];

STATIC_DCL void FDECL(right_side,  (int, int, int, int, int,
                                    int, int, char *));
STATIC_DCL void FDECL(left_side, (int, int, int, int, int, int, int, char *));
STATIC_DCL int FDECL(close_shadow, (int, int, int, int));
STATIC_DCL int FDECL(far_shadow, (int, int, int, int));

/*
 * Initialize algorithm D's table pointers.  If we don't have these,
 * then we do 3D table lookups.  Verrrry slow.
 */
STATIC_OVL void
view_init()
{
    int i;

    for (i = 0; i < CLOSE_MAX_BC_DY; i++)
        close_dy[i] = &close_table[i];

    for (i = 0; i < FAR_MAX_BC_DY; i++)
        far_dy[i] = &far_table[i];
}

/*
 * If the far table has an entry of OFF_TABLE, then the far block prevents
 * us from seeing the location just above/below it.  I.e. the first visible
 * location is one *before* the block.
 */
#define OFF_TABLE 0xff

STATIC_OVL int
close_shadow(side, this_row, block_row, block_col)
int side, this_row, block_row, block_col;
{
    register int sdy, sdx, pdy, offset;

    /*
     * If on the same column (block_row = -1), then we can see it.
     */
    if (block_row < 0)
        return block_col;

    /* Take explicit absolute values.  Adjust. */
    if ((sdy = (start_row - block_row)) < 0)
        sdy = -sdy;
    --sdy; /* src   dy */
    if ((sdx = (start_col - block_col)) < 0)
        sdx = -sdx; /* src   dx */
    if ((pdy = (block_row - this_row)) < 0)
        pdy = -pdy; /* point dy */

    if (sdy < 0 || sdy >= CLOSE_MAX_SB_DY || sdx >= CLOSE_MAX_SB_DX
        || pdy >= CLOSE_MAX_BC_DY) {
        impossible("close_shadow:  bad value");
        return block_col;
    }
    offset = close_dy[sdy]->close[sdx][pdy];
    if (side == FROM_RIGHT)
        return block_col + offset;

    return block_col - offset;
}

STATIC_OVL int
far_shadow(side, this_row, block_row, block_col)
int side, this_row, block_row, block_col;
{
    register int sdy, sdx, pdy, offset;

    /*
     * Take care of a bug that shows up only on the borders.
     *
     * If the block is beyond the border, then the row is negative.  Return
     * the block's column number (should be 0 or COLNO-1).
     *
     * Could easily have the column be -1, but then wouldn't know if it was
     * the left or right border.
     */
    if (block_row < 0)
        return block_col;

    /* Take explicit absolute values.  Adjust. */
    if ((sdy = (start_row - block_row)) < 0)
        sdy = -sdy; /* src   dy */
    if ((sdx = (start_col - block_col)) < 0)
        sdx = -sdx;
    --sdx; /* src   dx */
    if ((pdy = (block_row - this_row)) < 0)
        pdy = -pdy;
    --pdy; /* point dy */

    if (sdy >= FAR_MAX_SB_DY || sdx < 0 || sdx >= FAR_MAX_SB_DX || pdy < 0
        || pdy >= FAR_MAX_BC_DY) {
        impossible("far_shadow:  bad value");
        return block_col;
    }
    if ((offset = far_dy[sdy]->far_q[sdx][pdy]) == OFF_TABLE)
        offset = -1;
    if (side == FROM_RIGHT)
        return block_col + offset;

    return block_col - offset;
}

/*
 * right_side()
 *
 * Figure out what could be seen on the right side of the source.
 */
STATIC_OVL void
right_side(row, cb_row, cb_col, fb_row, fb_col, left, right_mark, limits)
int row;            /* current row */
int cb_row, cb_col; /* close block row and col */
int fb_row, fb_col; /* far block row and col */
int left;           /* left mark of the previous row */
int right_mark;     /* right mark of previous row */
char *limits;       /* points at range limit for current row, or NULL */
{
    register int i;
    register char *rowp = NULL;
    int hit_stone = 0;
    int left_shadow, right_shadow, loc_right;
    int lblock_col; /* local block column (current row) */
    int nrow, deeper;
    char *row_min = NULL; /* left most */
    char *row_max = NULL; /* right most */
    int lim_max;          /* right most limit of circle */

    nrow = row + step;
    deeper = good_row(nrow) && (!limits || (*limits >= *(limits + 1)));
    if (!vis_func) {
        rowp = cs_rows[row];
        row_min = &cs_left[row];
        row_max = &cs_right[row];
    }
    if (limits) {
        lim_max = start_col + *limits;
        if (lim_max > COLNO - 1)
            lim_max = COLNO - 1;
        if (right_mark > lim_max)
            right_mark = lim_max;
        limits++; /* prepare for next row */
    } else
        lim_max = COLNO - 1;

    /*
     * Get the left shadow from the close block.  This value could be
     * illegal.
     */
    left_shadow = close_shadow(FROM_RIGHT, row, cb_row, cb_col);

    /*
     * Mark all stone walls as seen before the left shadow.  All this work
     * for a special case.
     *
     * NOTE.  With the addition of this code in here, it is now *required*
     * for the algorithm to work correctly.  If this is commented out,
     * change the above assignment so that left and not left_shadow is the
     * variable that gets the shadow.
     */
    while (left <= right_mark) {
        loc_right = right_ptrs[row][left];
        if (loc_right > lim_max)
            loc_right = lim_max;
        if (viz_clear_rows[row][left]) {
            if (loc_right >= left_shadow) {
                left = left_shadow; /* opening ends beyond shadow */
                break;
            }
            left = loc_right;
            loc_right = right_ptrs[row][left];
            if (loc_right > lim_max)
                loc_right = lim_max;
            if (left == loc_right)
                return; /* boundary */

            /* Shadow covers opening, beyond right mark */
            if (left == right_mark && left_shadow > right_mark)
                return;
        }

        if (loc_right > right_mark) /* can't see stone beyond the mark */
            loc_right = right_mark;

        if (vis_func) {
            for (i = left; i <= loc_right; i++)
                (*vis_func)(i, row, varg);
        } else {
            for (i = left; i <= loc_right; i++)
                set_cs(rowp, i);
            set_min(left);
            set_max(loc_right);
        }

        if (loc_right == right_mark)
            return; /* all stone */
        if (loc_right >= left_shadow)
            hit_stone = 1;
        left = loc_right + 1;
    }

    /*
     * At this point we are at the first visible clear spot on or beyond
     * the left shadow, unless the left shadow is an illegal value.  If we
     * have "hit stone" then we have a stone wall just to our left.
     */

    /*
     * Get the right shadow.  Make sure that it is a legal value.
     */
    if ((right_shadow = far_shadow(FROM_RIGHT, row, fb_row, fb_col)) >= COLNO)
        right_shadow = COLNO - 1;
    /*
     * Make vertical walls work the way we want them.  In this case, we
     * note when the close block blocks the column just above/beneath
     * it (right_shadow < fb_col [actually right_shadow == fb_col-1]).  If
     * the location is filled, then we want to see it, so we put the
     * right shadow back (same as fb_col).
     */
    if (right_shadow < fb_col && !viz_clear_rows[row][fb_col])
        right_shadow = fb_col;
    if (right_shadow > lim_max)
        right_shadow = lim_max;

    /*
     * Main loop.  Within the range of sight of the previous row, mark all
     * stone walls as seen.  Follow open areas recursively.
     */
    while (left <= right_mark) {
        /* Get the far right of the opening or wall */
        loc_right = right_ptrs[row][left];
        if (loc_right > lim_max)
            loc_right = lim_max;

        if (!viz_clear_rows[row][left]) {
            hit_stone = 1; /* use stone on this row as close block */
            /*
             * We can see all of the wall until the next open spot or the
             * start of the shadow caused by the far block (right).
             *
             * Can't see stone beyond the right mark.
             */
            if (loc_right > right_mark)
                loc_right = right_mark;

            if (vis_func) {
                for (i = left; i <= loc_right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left; i <= loc_right; i++)
                    set_cs(rowp, i);
                set_min(left);
                set_max(loc_right);
            }

            if (loc_right == right_mark)
                return; /* hit the end */
            left = loc_right + 1;
            loc_right = right_ptrs[row][left];
            if (loc_right > lim_max)
                loc_right = lim_max;
            /* fall through... we know at least one position is visible */
        }

        /*
         * We are in an opening.
         *
         * If this is the first open spot since the could see area  (this is
         * true if we have hit stone), get the shadow generated by the wall
         * just to our left.
         */
        if (hit_stone) {
            lblock_col = left - 1; /* local block column */
            left = close_shadow(FROM_RIGHT, row, row, lblock_col);
            if (left > lim_max)
                break; /* off the end */
        }

        /*
         * Check if the shadow covers the opening.  If it does, then
         * move to end of the opening.  A shadow generated on from a
         * wall on this row does *not* cover the wall on the right
         * of the opening.
         */
        if (left >= loc_right) {
            if (loc_right == lim_max) { /* boundary */
                if (left == lim_max) {
                    if (vis_func)
                        (*vis_func)(lim_max, row, varg);
                    else {
                        set_cs(rowp, lim_max); /* last pos */
                        set_max(lim_max);
                    }
                }
                return; /* done */
            }
            left = loc_right;
            continue;
        }

        /*
         * If the far wall of the opening (loc_right) is closer than the
         * shadow limit imposed by the far block (right) then use the far
         * wall as our new far block when we recurse.
         *
         * If the limits are the same, and the far block really exists
         * (fb_row >= 0) then do the same as above.
         *
         * Normally, the check would be for the far wall being closer OR EQUAL
         * to the shadow limit.  However, there is a bug that arises from the
         * fact that the clear area pointers end in an open space (if it
         * exists) on a boundary.  This then makes a far block exist where it
         * shouldn't --- on a boundary.  To get around that, I had to
         * introduce the concept of a non-existent far block (when the
         * row < 0).  Next I have to check for it.  Here is where that check
         * exists.
         */
        if ((loc_right < right_shadow)
            || (fb_row >= 0 && loc_right == right_shadow)) {
            if (vis_func) {
                for (i = left; i <= loc_right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left; i <= loc_right; i++)
                    set_cs(rowp, i);
                set_min(left);
                set_max(loc_right);
            }

            if (deeper) {
                if (hit_stone)
                    right_side(nrow, row, lblock_col, row, loc_right, left,
                               loc_right, limits);
                else
                    right_side(nrow, cb_row, cb_col, row, loc_right, left,
                               loc_right, limits);
            }

            /*
             * The following line, setting hit_stone, is needed for those
             * walls that are only 1 wide.  If hit stone is *not* set and
             * the stone is only one wide, then the close block is the old
             * one instead one on the current row.  A way around having to
             * set it here is to make left = loc_right (not loc_right+1) and
             * let the outer loop take care of it.  However, if we do that
             * then we then have to check for boundary conditions here as
             * well.
             */
            hit_stone = 1;

            left = loc_right + 1;

        /*
         * The opening extends beyond the right mark.  This means that
         * the next far block is the current far block.
         */
        } else {
            if (vis_func) {
                for (i = left; i <= right_shadow; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left; i <= right_shadow; i++)
                    set_cs(rowp, i);
                set_min(left);
                set_max(right_shadow);
            }

            if (deeper) {
                if (hit_stone)
                    right_side(nrow, row, lblock_col, fb_row, fb_col, left,
                               right_shadow, limits);
                else
                    right_side(nrow, cb_row, cb_col, fb_row, fb_col, left,
                               right_shadow, limits);
            }

            return; /* we're outta here */
        }
    }
}

/*
 * left_side()
 *
 * This routine is the mirror image of right_side().  Please see right_side()
 * for blow by blow comments.
 */
STATIC_OVL void
left_side(row, cb_row, cb_col, fb_row, fb_col, left_mark, right, limits)
int row;            /* the current row */
int cb_row, cb_col; /* close block row and col */
int fb_row, fb_col; /* far block row and col */
int left_mark;      /* left mark of previous row */
int right;          /* right mark of the previous row */
char *limits;
{
    register int i;
    register char *rowp = NULL;
    int hit_stone = 0;
    int left_shadow, right_shadow, loc_left;
    int lblock_col; /* local block column (current row) */
    int nrow, deeper;
    char *row_min = NULL; /* left most */
    char *row_max = NULL; /* right most */
    int lim_min;

    nrow = row + step;
    deeper = good_row(nrow) && (!limits || (*limits >= *(limits + 1)));
    if (!vis_func) {
        rowp = cs_rows[row];
        row_min = &cs_left[row];
        row_max = &cs_right[row];
    }
    if (limits) {
        lim_min = start_col - *limits;
        if (lim_min < 0)
            lim_min = 0;
        if (left_mark < lim_min)
            left_mark = lim_min;
        limits++; /* prepare for next row */
    } else
        lim_min = 0;

    /* This value could be illegal. */
    right_shadow = close_shadow(FROM_LEFT, row, cb_row, cb_col);

    while (right >= left_mark) {
        loc_left = left_ptrs[row][right];
        if (loc_left < lim_min)
            loc_left = lim_min;
        if (viz_clear_rows[row][right]) {
            if (loc_left <= right_shadow) {
                right = right_shadow; /* opening ends beyond shadow */
                break;
            }
            right = loc_left;
            loc_left = left_ptrs[row][right];
            if (loc_left < lim_min)
                loc_left = lim_min;
            if (right == loc_left)
                return; /* boundary */
        }

        if (loc_left < left_mark) /* can't see beyond the left mark */
            loc_left = left_mark;

        if (vis_func) {
            for (i = loc_left; i <= right; i++)
                (*vis_func)(i, row, varg);
        } else {
            for (i = loc_left; i <= right; i++)
                set_cs(rowp, i);
            set_min(loc_left);
            set_max(right);
        }

        if (loc_left == left_mark)
            return; /* all stone */
        if (loc_left <= right_shadow)
            hit_stone = 1;
        right = loc_left - 1;
    }

    /* At first visible clear spot on or beyond the right shadow. */

    if ((left_shadow = far_shadow(FROM_LEFT, row, fb_row, fb_col)) < 0)
        left_shadow = 0;

    /* Do vertical walls as we want. */
    if (left_shadow > fb_col && !viz_clear_rows[row][fb_col])
        left_shadow = fb_col;
    if (left_shadow < lim_min)
        left_shadow = lim_min;

    while (right >= left_mark) {
        loc_left = left_ptrs[row][right];

        if (!viz_clear_rows[row][right]) {
            hit_stone = 1; /* use stone on this row as close block */

            /* We can only see walls until the left mark */
            if (loc_left < left_mark)
                loc_left = left_mark;

            if (vis_func) {
                for (i = loc_left; i <= right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = loc_left; i <= right; i++)
                    set_cs(rowp, i);
                set_min(loc_left);
                set_max(right);
            }

            if (loc_left == left_mark)
                return; /* hit end */
            right = loc_left - 1;
            loc_left = left_ptrs[row][right];
            if (loc_left < lim_min)
                loc_left = lim_min;
            /* fall through...*/
        }

        /* We are in an opening. */
        if (hit_stone) {
            lblock_col = right + 1; /* stone block (local) */
            right = close_shadow(FROM_LEFT, row, row, lblock_col);
            if (right < lim_min)
                return; /* off the end */
        }

        /*  Check if the shadow covers the opening. */
        if (right <= loc_left) {
            /*  Make a boundary condition work. */
            if (loc_left == lim_min) { /* at boundary */
                if (right == lim_min) {
                    if (vis_func)
                        (*vis_func)(lim_min, row, varg);
                    else {
                        set_cs(rowp, lim_min); /* caught the last pos */
                        set_min(lim_min);
                    }
                }
                return; /* and break out the loop */
            }

            right = loc_left;
            continue;
        }

        /* If the far wall of the opening is closer than the shadow limit. */
        if ((loc_left > left_shadow)
            || (fb_row >= 0 && loc_left == left_shadow)) {
            if (vis_func) {
                for (i = loc_left; i <= right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = loc_left; i <= right; i++)
                    set_cs(rowp, i);
                set_min(loc_left);
                set_max(right);
            }

            if (deeper) {
                if (hit_stone)
                    left_side(nrow, row, lblock_col, row, loc_left, loc_left,
                              right, limits);
                else
                    left_side(nrow, cb_row, cb_col, row, loc_left, loc_left,
                              right, limits);
            }

            hit_stone = 1; /* needed for walls of width 1 */
            right = loc_left - 1;

        /*  The opening extends beyond the left mark. */
        } else {
            if (vis_func) {
                for (i = left_shadow; i <= right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left_shadow; i <= right; i++)
                    set_cs(rowp, i);
                set_min(left_shadow);
                set_max(right);
            }

            if (deeper) {
                if (hit_stone)
                    left_side(nrow, row, lblock_col, fb_row, fb_col,
                              left_shadow, right, limits);
                else
                    left_side(nrow, cb_row, cb_col, fb_row, fb_col,
                              left_shadow, right, limits);
            }

            return; /* we're outta here */
        }
    }
}

/*
 * view_from
 *
 * Calculate a view from the given location.  Initialize and fill a
 * ROWNOxCOLNO array (could_see) with all the locations that could be
 * seen from the source location.  Initialize and fill the left most
 * and right most boundaries of what could be seen.
 */
STATIC_OVL void
view_from(srow, scol, loc_cs_rows, left_most, right_most, range, func, arg)
int srow, scol;               /* source row and column */
char **loc_cs_rows;           /* could_see array (row pointers) */
char *left_most, *right_most; /* limits of what could be seen */
int range;                    /* 0 if unlimited */
void FDECL((*func), (int, int, genericptr_t));
genericptr_t arg;
{
    register int i;
    char *rowp;
    int nrow, left, right, left_row, right_row;
    char *limits;

    /* Set globals for near_shadow(), far_shadow(), etc. to use. */
    start_col = scol;
    start_row = srow;
    cs_rows = loc_cs_rows;
    cs_left = left_most;
    cs_right = right_most;
    vis_func = func;
    varg = arg;

    /*  Find the left and right limits of sight on the starting row. */
    if (viz_clear_rows[srow][scol]) {
        left = left_ptrs[srow][scol];
        right = right_ptrs[srow][scol];
    } else {
        left = (!scol) ? 0 : (viz_clear_rows[srow][scol - 1]
                                  ? left_ptrs[srow][scol - 1]
                                  : scol - 1);
        right = (scol == COLNO - 1) ? COLNO - 1
                                    : (viz_clear_rows[srow][scol + 1]
                                           ? right_ptrs[srow][scol + 1]
                                           : scol + 1);
    }

    if (range) {
        if (range > MAX_RADIUS || range < 1)
            panic("view_from called with range %d", range);
        limits = circle_ptr(range) + 1; /* start at next row */
        if (left < scol - range)
            left = scol - range;
        if (right > scol + range)
            right = scol + range;
    } else
        limits = (char *) 0;

    if (func) {
        for (i = left; i <= right; i++)
            (*func)(i, srow, arg);
    } else {
        /* Row optimization */
        rowp = cs_rows[srow];

        /* We know that we can see our row. */
        for (i = left; i <= right; i++)
            set_cs(rowp, i);
        cs_left[srow] = left;
        cs_right[srow] = right;
    }

    /* The far block has a row number of -1 if we are on an edge. */
    right_row = (right == COLNO - 1) ? -1 : srow;
    left_row = (!left) ? -1 : srow;

    /*
     *  Check what could be seen in quadrants.
     */
    if ((nrow = srow + 1) < ROWNO) {
        step = 1; /* move down */
        if (scol < COLNO - 1)
            right_side(nrow, -1, scol, right_row, right, scol, right, limits);
        if (scol)
            left_side(nrow, -1, scol, left_row, left, left, scol, limits);
    }

    if ((nrow = srow - 1) >= 0) {
        step = -1; /* move up */
        if (scol < COLNO - 1)
            right_side(nrow, -1, scol, right_row, right, scol, right, limits);
        if (scol)
            left_side(nrow, -1, scol, left_row, left, left, scol, limits);
    }
}

#else /*===== End of algorithm D =====*/

/*==========================================================================*\
                            GENERAL LINE OF SIGHT
                                Algorithm C
\*==========================================================================*/

/*
 * Defines local to Algorithm C.
 */
STATIC_DCL void FDECL(right_side, (int, int, int, char *));
STATIC_DCL void FDECL(left_side, (int, int, int, char *));

/* Initialize algorithm C (nothing). */
STATIC_OVL void
view_init()
{
}

/*
 * Mark positions as visible on one quadrant of the right side.  The
 * quadrant is determined by the value of the global variable step.
 */
STATIC_OVL void
right_side(row, left, right_mark, limits)
int row;        /* current row */
int left;       /* first (left side) visible spot on prev row */
int right_mark; /* last (right side) visible spot on prev row */
char *limits;   /* points at range limit for current row, or NULL */
{
    int right;                  /* right limit of "could see" */
    int right_edge;             /* right edge of an opening */
    int nrow;                   /* new row (calculate once) */
    int deeper;                 /* if TRUE, call self as needed */
    int result;                 /* set by q?_path() */
    register int i;             /* loop counter */
    register char *rowp = NULL; /* row optimization */
    char *row_min = NULL;       /* left most  [used by macro set_min()] */
    char *row_max = NULL;       /* right most [used by macro set_max()] */
    int lim_max;                /* right most limit of circle */

    nrow = row + step;
    /*
     * Can go deeper if the row is in bounds and the next row is within
     * the circle's limit.  We tell the latter by checking to see if the next
     * limit value is the start of a new circle radius (meaning we depend
     * on the structure of circle_data[]).
     */
    deeper = good_row(nrow) && (!limits || (*limits >= *(limits + 1)));
    if (!vis_func) {
        rowp = cs_rows[row]; /* optimization */
        row_min = &cs_left[row];
        row_max = &cs_right[row];
    }
    if (limits) {
        lim_max = start_col + *limits;
        if (lim_max > COLNO - 1)
            lim_max = COLNO - 1;
        if (right_mark > lim_max)
            right_mark = lim_max;
        limits++; /* prepare for next row */
    } else
        lim_max = COLNO - 1;

    while (left <= right_mark) {
        right_edge = right_ptrs[row][left];
        if (right_edge > lim_max)
            right_edge = lim_max;

        if (!is_clear(row, left)) {
            /*
             * Jump to the far side of a stone wall.  We can set all
             * the points in between as seen.
             *
             * If the right edge goes beyond the right mark, check to see
             * how much we can see.
             */
            if (right_edge > right_mark) {
                /*
                 * If the mark on the previous row was a clear position,
                 * the odds are that we can actually see part of the wall
                 * beyond the mark on this row.  If so, then see one beyond
                 * the mark.  Otherwise don't.  This is a kludge so corners
                 * with an adjacent doorway show up in nethack.
                 */
                right_edge = is_clear(row - step, right_mark) ? right_mark + 1
                                                              : right_mark;
            }
            if (vis_func) {
                for (i = left; i <= right_edge; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left; i <= right_edge; i++)
                    set_cs(rowp, i);
                set_min(left);
                set_max(right_edge);
            }
            left = right_edge + 1; /* no limit check necessary */
            continue;
        }

        /* No checking needed if our left side is the start column. */
        if (left != start_col) {
            /*
             * Find the left side.  Move right until we can see it or we run
             * into a wall.
             */
            for (; left <= right_edge; left++) {
                if (step < 0) {
                    q1_path(start_row, start_col, row, left, rside1);
                } else {
                    q4_path(start_row, start_col, row, left, rside1);
                }
            rside1: /* used if q?_path() is a macro */
                if (result)
                    break;
            }

            /*
             * Check for boundary conditions.  We *need* check (2) to break
             * an infinite loop where:
             *
             *           left == right_edge == right_mark == lim_max.
             *
             */
            if (left > lim_max)
                return;            /* check (1) */
            if (left == lim_max) { /* check (2) */
                if (vis_func) {
                    (*vis_func)(lim_max, row, varg);
                } else {
                    set_cs(rowp, lim_max);
                    set_max(lim_max);
                }
                return;
            }
            /*
             * Check if we can see any spots in the opening.  We might
             * (left == right_edge) or might not (left == right_edge+1) have
             * been able to see the far wall.  Make sure we *can* see the
             * wall (remember, we can see the spot above/below this one)
             * by backing up.
             */
            if (left >= right_edge) {
                left = right_edge; /* for the case left == right_edge+1 */
                continue;
            }
        }

        /*
         * Find the right side.  If the marker from the previous row is
         * closer than the edge on this row, then we have to check
         * how far we can see around the corner (under the overhang).  Stop
         * at the first non-visible spot or we actually hit the far wall.
         *
         * Otherwise, we know we can see the right edge of the current row.
         *
         * This must be a strict less than so that we can always see a
         * horizontal wall, even if it is adjacent to us.
         */
        if (right_mark < right_edge) {
            for (right = right_mark; right <= right_edge; right++) {
                if (step < 0) {
                    q1_path(start_row, start_col, row, right, rside2);
                } else {
                    q4_path(start_row, start_col, row, right, rside2);
                }
            rside2: /* used if q?_path() is a macro */
                if (!result)
                    break;
            }
            --right; /* get rid of the last increment */
        } else
            right = right_edge;

        /*
         * We have the range that we want.  Set the bits.  Note that
         * there is no else --- we no longer handle splinters.
         */
        if (left <= right) {
            /*
             * An ugly special case.  If you are adjacent to a vertical wall
             * and it has a break in it, then the right mark is set to be
             * start_col.  We *want* to be able to see adjacent vertical
             * walls, so we have to set it back.
             */
            if (left == right && left == start_col && start_col < (COLNO - 1)
                && !is_clear(row, start_col + 1))
                right = start_col + 1;

            if (right > lim_max)
                right = lim_max;
            /* set the bits */
            if (vis_func) {
                for (i = left; i <= right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left; i <= right; i++)
                    set_cs(rowp, i);
                set_min(left);
                set_max(right);
            }

            /* recursive call for next finger of light */
            if (deeper)
                right_side(nrow, left, right, limits);
            left = right + 1; /* no limit check necessary */
        }
    }
}

/*
 * This routine is the mirror image of right_side().  See right_side() for
 * extensive comments.
 */
STATIC_OVL void
left_side(row, left_mark, right, limits)
int row, left_mark, right;
char *limits;
{
    int left, left_edge, nrow, deeper, result;
    register int i;
    register char *rowp = NULL;
    char *row_min = NULL;
    char *row_max = NULL;
    int lim_min;

#ifdef GCC_WARN
    rowp = row_min = row_max = 0;
#endif
    nrow = row + step;
    deeper = good_row(nrow) && (!limits || (*limits >= *(limits + 1)));
    if (!vis_func) {
        rowp = cs_rows[row];
        row_min = &cs_left[row];
        row_max = &cs_right[row];
    }
    if (limits) {
        lim_min = start_col - *limits;
        if (lim_min < 0)
            lim_min = 0;
        if (left_mark < lim_min)
            left_mark = lim_min;
        limits++; /* prepare for next row */
    } else
        lim_min = 0;

    while (right >= left_mark) {
        left_edge = left_ptrs[row][right];
        if (left_edge < lim_min)
            left_edge = lim_min;

        if (!is_clear(row, right)) {
            /* Jump to the far side of a stone wall. */
            if (left_edge < left_mark) {
                /* Maybe see more (kludge). */
                left_edge = is_clear(row - step, left_mark) ? left_mark - 1
                                                            : left_mark;
            }
            if (vis_func) {
                for (i = left_edge; i <= right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left_edge; i <= right; i++)
                    set_cs(rowp, i);
                set_min(left_edge);
                set_max(right);
            }
            right = left_edge - 1; /* no limit check necessary */
            continue;
        }

        if (right != start_col) {
            /* Find the right side. */
            for (; right >= left_edge; right--) {
                if (step < 0) {
                    q2_path(start_row, start_col, row, right, lside1);
                } else {
                    q3_path(start_row, start_col, row, right, lside1);
                }
            lside1: /* used if q?_path() is a macro */
                if (result)
                    break;
            }

            /* Check for boundary conditions. */
            if (right < lim_min)
                return;
            if (right == lim_min) {
                if (vis_func) {
                    (*vis_func)(lim_min, row, varg);
                } else {
                    set_cs(rowp, lim_min);
                    set_min(lim_min);
                }
                return;
            }
            /* Check if we can see any spots in the opening. */
            if (right <= left_edge) {
                right = left_edge;
                continue;
            }
        }

        /* Find the left side. */
        if (left_mark > left_edge) {
            for (left = left_mark; left >= left_edge; --left) {
                if (step < 0) {
                    q2_path(start_row, start_col, row, left, lside2);
                } else {
                    q3_path(start_row, start_col, row, left, lside2);
                }
            lside2: /* used if q?_path() is a macro */
                if (!result)
                    break;
            }
            left++; /* get rid of the last decrement */
        } else
            left = left_edge;

        if (left <= right) {
            /* An ugly special case. */
            if (left == right && right == start_col && start_col > 0
                && !is_clear(row, start_col - 1))
                left = start_col - 1;

            if (left < lim_min)
                left = lim_min;
            if (vis_func) {
                for (i = left; i <= right; i++)
                    (*vis_func)(i, row, varg);
            } else {
                for (i = left; i <= right; i++)
                    set_cs(rowp, i);
                set_min(left);
                set_max(right);
            }

            /* Recurse */
            if (deeper)
                left_side(nrow, left, right, limits);
            right = left - 1; /* no limit check necessary */
        }
    }
}

/*
 * Calculate all possible visible locations from the given location
 * (srow,scol).  NOTE this is (y,x)!  Mark the visible locations in the
 * array provided.
 */
STATIC_OVL void
view_from(srow, scol, loc_cs_rows, left_most, right_most, range, func, arg)
int srow, scol;     /* starting row and column */
char **loc_cs_rows; /* pointers to the rows of the could_see array */
char *left_most;    /* min mark on each row */
char *right_most;   /* max mark on each row */
int range;          /* 0 if unlimited */
void FDECL((*func), (int, int, genericptr_t));
genericptr_t arg;
{
    register int i; /* loop counter */
    char *rowp;     /* optimization for setting could_see */
    int nrow;       /* the next row */
    int left;       /* the left-most visible column */
    int right;      /* the right-most visible column */
    char *limits;   /* range limit for next row */

    /* Set globals for q?_path(), left_side(), and right_side() to use. */
    start_col = scol;
    start_row = srow;
    cs_rows = loc_cs_rows; /* 'could see' rows */
    cs_left = left_most;
    cs_right = right_most;
    vis_func = func;
    varg = arg;

    /*
     * Determine extent of sight on the starting row.
     */
    if (is_clear(srow, scol)) {
        left = left_ptrs[srow][scol];
        right = right_ptrs[srow][scol];
    } else {
        /*
         * When in stone, you can only see your adjacent squares, unless
         * you are on an array boundary or a stone/clear boundary.
         */
        left = (!scol) ? 0
                       : (is_clear(srow, scol - 1) ? left_ptrs[srow][scol - 1]
                                                   : scol - 1);
        right = (scol == COLNO - 1)
                    ? COLNO - 1
                    : (is_clear(srow, scol + 1) ? right_ptrs[srow][scol + 1]
                                                : scol + 1);
    }

    if (range) {
        if (range > MAX_RADIUS || range < 1)
            panic("view_from called with range %d", range);
        limits = circle_ptr(range) + 1; /* start at next row */
        if (left < scol - range)
            left = scol - range;
        if (right > scol + range)
            right = scol + range;
    } else
        limits = (char *) 0;

    if (func) {
        for (i = left; i <= right; i++)
            (*func)(i, srow, arg);
    } else {
        /* Row pointer optimization. */
        rowp = cs_rows[srow];

        /* We know that we can see our row. */
        for (i = left; i <= right; i++)
            set_cs(rowp, i);
        cs_left[srow] = left;
        cs_right[srow] = right;
    }

    /*
     * Check what could be seen in quadrants.  We need to check for valid
     * rows here, since we don't do it in the routines right_side() and
     * left_side() [ugliness to remove extra routine calls].
     */
    if ((nrow = srow + 1) < ROWNO) { /* move down */
        step = 1;
        if (scol < COLNO - 1)
            right_side(nrow, scol, right, limits);
        if (scol)
            left_side(nrow, left, scol, limits);
    }

    if ((nrow = srow - 1) >= 0) { /* move up */
        step = -1;
        if (scol < COLNO - 1)
            right_side(nrow, scol, right, limits);
        if (scol)
            left_side(nrow, left, scol, limits);
    }
}

#endif /*===== End of algorithm C =====*/

/*
 * AREA OF EFFECT "ENGINE"
 *
 * Calculate all possible visible locations as viewed from the given location
 * (srow,scol) within the range specified. Perform "func" with (x, y) args and
 * additional argument "arg" for each square.
 *
 * If not centered on the hero, just forward arguments to view_from(); it
 * will call "func" when necessary.  If the hero is the center, use the
 * vision matrix and reduce extra work.
 */
void
do_clear_area(scol, srow, range, func, arg)
int scol, srow, range;
void FDECL((*func), (int, int, genericptr_t));
genericptr_t arg;
{
    /* If not centered on hero, do the hard work of figuring the area */
    if (scol != u.ux || srow != u.uy) {
        view_from(srow, scol, (char **) 0, (char *) 0, (char *) 0, range,
                  func, arg);
    } else {
        register int x;
        int y, min_x, max_x, max_y, offset;
        char *limits;
        boolean override_vision;

        /* vision doesn't pass through water or clouds, detection should
           [this probably ought to be an arg supplied by our caller...] */
        override_vision =
            (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz)) && detecting(func);

        if (range > MAX_RADIUS || range < 1)
            panic("do_clear_area:  illegal range %d", range);
        if (vision_full_recalc)
            vision_recalc(0); /* recalc vision if dirty */
        limits = circle_ptr(range);
        if ((max_y = (srow + range)) >= ROWNO)
            max_y = ROWNO - 1;
        if ((y = (srow - range)) < 0)
            y = 0;
        for (; y <= max_y; y++) {
            offset = limits[v_abs(y - srow)];
            if ((min_x = (scol - offset)) < 0)
                min_x = 0;
            if ((max_x = (scol + offset)) >= COLNO)
                max_x = COLNO - 1;
            for (x = min_x; x <= max_x; x++)
                if (couldsee(x, y) || override_vision)
                    (*func)(x, y, arg);
        }
    }
}

/* bitmask indicating ways mon is seen; extracted from lookat(pager.c) */
unsigned
howmonseen(mon)
struct monst *mon;
{
    boolean useemon = (boolean) canseemon(mon);
    int xraydist = (u.xray_range < 0) ? -1 : (u.xray_range * u.xray_range);
    unsigned how_seen = 0; /* result */

    /* normal vision;
       cansee is true for both normal and astral vision,
       but couldsee it not true for astral vision */
    if ((mon->wormno ? worm_known(mon) : (cansee(mon->mx, mon->my)
                                          && couldsee(mon->mx, mon->my)))
        && mon_visible(mon) && !mon->minvis)
        how_seen |= MONSEEN_NORMAL;
    /* see invisible */
    if (useemon && mon->minvis)
        how_seen |= MONSEEN_SEEINVIS;
    /* infravision */
    if ((!mon->minvis || See_invisible) && see_with_infrared(mon))
        how_seen |= MONSEEN_INFRAVIS;
    /* telepathy */
    if (tp_sensemon(mon))
        how_seen |= MONSEEN_TELEPAT;
    /* xray */
    if (useemon && xraydist > 0 && distu(mon->mx, mon->my) <= xraydist)
        how_seen |= MONSEEN_XRAYVIS;
    /* extended detection */
    if (Detect_monsters)
        how_seen |= MONSEEN_DETECT;
    /* class-/type-specific warning */
    if (MATCH_WARN_OF_MON(mon))
        how_seen |= MONSEEN_WARNMON;

    return how_seen;
}

/*vision.c*/

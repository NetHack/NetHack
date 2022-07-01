/* NetHack 3.7	mkmap.c	$NHDT-Date: 1596498181 2020/08/03 23:43:01 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.28 $ */
/* Copyright (c) J. C. Collet, M. Stephenson and D. Cohrs, 1992   */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sp_lev.h"

#define HEIGHT (ROWNO - 1)
#define WIDTH (COLNO - 2)

static void init_map(schar);
static void init_fill(schar, schar);
static schar get_map(int, int, schar);
static void pass_one(schar, schar);
static void pass_two(schar, schar);
static void pass_three(schar, schar);
static void join_map(schar, schar);
static void finish_map(schar, schar, boolean, boolean, boolean);
static void remove_room(unsigned);
void mkmap(lev_init *);

static void
init_map(schar bg_typ)
{
    register int i, j;

    for (i = 1; i < COLNO; i++)
        for (j = 0; j < ROWNO; j++) {
            levl[i][j].typ = bg_typ;
            levl[i][j].lit = FALSE;
        }
}

static void
init_fill(schar bg_typ, schar fg_typ)
{
    register int i, j;
    long limit, count;

    limit = (WIDTH * HEIGHT * 2) / 5;
    count = 0;
    while (count < limit) {
        i = rn1(WIDTH - 1, 2);
        j = rnd(HEIGHT - 1);
        if (levl[i][j].typ == bg_typ) {
            levl[i][j].typ = fg_typ;
            count++;
        }
    }
}

static schar
get_map(int col, int row, schar bg_typ)
{
    if (col <= 0 || row < 0 || col > WIDTH || row >= HEIGHT)
        return bg_typ;
    return levl[col][row].typ;
}

static const int dirs[16] = {
    -1, -1 /**/, -1,  0 /**/, -1, 1 /**/, 0, -1 /**/,
     0,  1 /**/,  1, -1 /**/,  1, 0 /**/, 1,  1
};

static void
pass_one(schar bg_typ, schar fg_typ)
{
    register int i, j;
    short count, dr;

    for (i = 2; i <= WIDTH; i++)
        for (j = 1; j < HEIGHT; j++) {
            for (count = 0, dr = 0; dr < 8; dr++)
                if (get_map(i + dirs[dr * 2], j + dirs[(dr * 2) + 1], bg_typ)
                    == fg_typ)
                    count++;

            switch (count) {
            case 0: /* death */
            case 1:
            case 2:
                levl[i][j].typ = bg_typ;
                break;
            case 5:
            case 6:
            case 7:
            case 8:
                levl[i][j].typ = fg_typ;
                break;
            default:
                break;
            }
        }
}

#define new_loc(i, j) *(g.new_locations + ((j) * (WIDTH + 1)) + (i))

static void
pass_two(schar bg_typ, schar fg_typ)
{
    register int i, j;
    short count, dr;

    for (i = 2; i <= WIDTH; i++)
        for (j = 1; j < HEIGHT; j++) {
            for (count = 0, dr = 0; dr < 8; dr++)
                if (get_map(i + dirs[dr * 2], j + dirs[(dr * 2) + 1], bg_typ)
                    == fg_typ)
                    count++;
            if (count == 5)
                new_loc(i, j) = bg_typ;
            else
                new_loc(i, j) = get_map(i, j, bg_typ);
        }

    for (i = 2; i <= WIDTH; i++)
        for (j = 1; j < HEIGHT; j++)
            levl[i][j].typ = new_loc(i, j);
}

static void
pass_three(schar bg_typ, schar fg_typ)
{
    register int i, j;
    short count, dr;

    for (i = 2; i <= WIDTH; i++)
        for (j = 1; j < HEIGHT; j++) {
            for (count = 0, dr = 0; dr < 8; dr++)
                if (get_map(i + dirs[dr * 2], j + dirs[(dr * 2) + 1], bg_typ)
                    == fg_typ)
                    count++;
            if (count < 3)
                new_loc(i, j) = bg_typ;
            else
                new_loc(i, j) = get_map(i, j, bg_typ);
        }

    for (i = 2; i <= WIDTH; i++)
        for (j = 1; j < HEIGHT; j++)
            levl[i][j].typ = new_loc(i, j);
}

/*
 * use a flooding algorithm to find all locations that should
 * have the same rm number as the current location.
 * if anyroom is TRUE, use IS_ROOM to check room membership instead of
 * exactly matching levl[sx][sy].typ and walls are included as well.
 */
void
flood_fill_rm(
    int sx,
    register int sy,
    register int rmno,
    boolean lit,
    boolean anyroom)
{
    register int i;
    int nx;
    schar fg_typ = levl[sx][sy].typ;

    /* back up to find leftmost uninitialized location */
    while (sx > 0 && (anyroom ? IS_ROOM(levl[sx][sy].typ)
                              : levl[sx][sy].typ == fg_typ)
           && (int) levl[sx][sy].roomno != rmno)
        sx--;
    sx++; /* compensate for extra decrement */

    /* assume sx,sy is valid */
    if (sx < g.min_rx)
        g.min_rx = sx;
    if (sy < g.min_ry)
        g.min_ry = sy;

    for (i = sx; i <= WIDTH && levl[i][sy].typ == fg_typ; i++) {
        levl[i][sy].roomno = rmno;
        levl[i][sy].lit = lit;
        if (anyroom) {
            /* add walls to room as well */
            register int ii, jj;
            for (ii = (i == sx ? i - 1 : i); ii <= i + 1; ii++)
                for (jj = sy - 1; jj <= sy + 1; jj++)
                    if (isok(ii, jj) && (IS_WALL(levl[ii][jj].typ)
                                         || IS_DOOR(levl[ii][jj].typ)
                                         || levl[ii][jj].typ == SDOOR)) {
                        levl[ii][jj].edge = 1;
                        if (lit)
                            levl[ii][jj].lit = lit;

                        if (levl[ii][jj].roomno == NO_ROOM)
                            levl[ii][jj].roomno = rmno;
                        else if ((int) levl[ii][jj].roomno != rmno)
                            levl[ii][jj].roomno = SHARED;
                    }
        }
        g.n_loc_filled++;
    }
    nx = i;

    if (isok(sx, sy - 1)) {
        for (i = sx; i < nx; i++)
            if (levl[i][sy - 1].typ == fg_typ) {
                if ((int) levl[i][sy - 1].roomno != rmno)
                    flood_fill_rm(i, sy - 1, rmno, lit, anyroom);
            } else {
                if ((i > sx || isok(i - 1, sy - 1))
                    && levl[i - 1][sy - 1].typ == fg_typ) {
                    if ((int) levl[i - 1][sy - 1].roomno != rmno)
                        flood_fill_rm(i - 1, sy - 1, rmno, lit, anyroom);
                }
                if ((i < nx - 1 || isok(i + 1, sy - 1))
                    && levl[i + 1][sy - 1].typ == fg_typ) {
                    if ((int) levl[i + 1][sy - 1].roomno != rmno)
                        flood_fill_rm(i + 1, sy - 1, rmno, lit, anyroom);
                }
            }
    }
    if (isok(sx, sy + 1)) {
        for (i = sx; i < nx; i++)
            if (levl[i][sy + 1].typ == fg_typ) {
                if ((int) levl[i][sy + 1].roomno != rmno)
                    flood_fill_rm(i, sy + 1, rmno, lit, anyroom);
            } else {
                if ((i > sx || isok(i - 1, sy + 1))
                    && levl[i - 1][sy + 1].typ == fg_typ) {
                    if ((int) levl[i - 1][sy + 1].roomno != rmno)
                        flood_fill_rm(i - 1, sy + 1, rmno, lit, anyroom);
                }
                if ((i < nx - 1 || isok(i + 1, sy + 1))
                    && levl[i + 1][sy + 1].typ == fg_typ) {
                    if ((int) levl[i + 1][sy + 1].roomno != rmno)
                        flood_fill_rm(i + 1, sy + 1, rmno, lit, anyroom);
                }
            }
    }

    if (nx > g.max_rx)
        g.max_rx = nx - 1; /* nx is just past valid region */
    if (sy > g.max_ry)
        g.max_ry = sy;
}

static void
join_map(schar bg_typ, schar fg_typ)
{
    register struct mkroom *croom, *croom2;

    register int i, j;
    int sx, sy;
    coord sm, em;

    /* first, use flood filling to find all of the regions that need joining
     */
    for (i = 2; i <= WIDTH; i++)
        for (j = 1; j < HEIGHT; j++) {
            if (levl[i][j].typ == fg_typ && levl[i][j].roomno == NO_ROOM) {
                g.min_rx = g.max_rx = i;
                g.min_ry = g.max_ry = j;
                g.n_loc_filled = 0;
                flood_fill_rm(i, j, g.nroom + ROOMOFFSET, FALSE, FALSE);
                if (g.n_loc_filled > 3) {
                    add_room(g.min_rx, g.min_ry, g.max_rx, g.max_ry,
                             FALSE, OROOM, TRUE);
                    g.rooms[g.nroom - 1].irregular = TRUE;
                    if (g.nroom >= (MAXNROFROOMS * 2))
                        goto joinm;
                } else {
                    /*
                     * it's a tiny hole; erase it from the map to avoid
                     * having the player end up here with no way out.
                     */
                    for (sx = g.min_rx; sx <= g.max_rx; sx++)
                        for (sy = g.min_ry; sy <= g.max_ry; sy++)
                            if ((int) levl[sx][sy].roomno
                                == g.nroom + ROOMOFFSET) {
                                levl[sx][sy].typ = bg_typ;
                                levl[sx][sy].roomno = NO_ROOM;
                            }
                }
            }
        }

 joinm:
    /*
     * Ok, now we can actually join the regions with fg_typ's.
     * The rooms are already sorted due to the previous loop,
     * so don't call sort_rooms(), which can screw up the roomno's
     * validity in the levl structure.
     */
    for (croom = &g.rooms[0], croom2 = croom + 1;
         croom2 < &g.rooms[g.nroom]; ) {
        /* pick random starting and end locations for "corridor" */
        if (!somexy(croom, &sm) || !somexy(croom2, &em)) {
            /* ack! -- the level is going to be busted */
            /* arbitrarily pick centers of both rooms and hope for the best */
            impossible("No start/end room loc in join_map.");
            sm.x = croom->lx + ((croom->hx - croom->lx) / 2);
            sm.y = croom->ly + ((croom->hy - croom->ly) / 2);
            em.x = croom2->lx + ((croom2->hx - croom2->lx) / 2);
            em.y = croom2->ly + ((croom2->hy - croom2->ly) / 2);
        }

        (void) dig_corridor(&sm, &em, FALSE, fg_typ, bg_typ);

        /* choose next region to join */
        /* only increment croom if croom and croom2 are non-overlapping */
        if (croom2->lx > croom->hx
            || ((croom2->ly > croom->hy || croom2->hy < croom->ly)
                && rn2(3))) {
            croom = croom2;
        }
        croom2++; /* always increment the next room */
    }
}

static void
finish_map(
    schar fg_typ,
    schar bg_typ,
    boolean lit,
    boolean walled,
    boolean icedpools)
{
    int i, j;

    if (walled)
        wallify_map(1, 0, COLNO-1, ROWNO-1);

    if (lit) {
        for (i = 1; i < COLNO; i++)
            for (j = 0; j < ROWNO; j++)
                if ((!IS_ROCK(fg_typ) && levl[i][j].typ == fg_typ)
                    || (!IS_ROCK(bg_typ) && levl[i][j].typ == bg_typ)
                    || (bg_typ == TREE && levl[i][j].typ == bg_typ)
                    || (walled && IS_WALL(levl[i][j].typ)))
                    levl[i][j].lit = TRUE;
        for (i = 0; i < g.nroom; i++)
            g.rooms[i].rlit = 1;
    }
    /* light lava even if everything's otherwise unlit;
       ice might be frozen pool rather than frozen moat */
    for (i = 1; i < COLNO; i++)
        for (j = 0; j < ROWNO; j++) {
            if (levl[i][j].typ == LAVAPOOL)
                levl[i][j].lit = TRUE;
            else if (levl[i][j].typ == ICE)
                levl[i][j].icedpool = icedpools ? ICED_POOL : ICED_MOAT;
        }
}

/*
 * When level processed by join_map is overlaid by a MAP, some rooms may no
 * longer be valid.  All rooms in the region lx <= x < hx, ly <= y < hy are
 * removed.  Rooms partially in the region are truncated.  This function
 * must be called before the REGIONs or ROOMs of the map are processed, or
 * those rooms will be removed as well.  Assumes roomno fields in the
 * region are already cleared, and roomno and irregular fields outside the
 * region are all set.
 */
void
remove_rooms(int lx, int ly, int hx, int hy)
{
    int i;
    struct mkroom *croom;

    for (i = g.nroom - 1; i >= 0; --i) {
        croom = &g.rooms[i];
        if (croom->hx < lx || croom->lx >= hx || croom->hy < ly
            || croom->ly >= hy)
            continue; /* no overlap */

        if (croom->lx < lx || croom->hx >= hx || croom->ly < ly
            || croom->hy >= hy) { /* partial overlap */
            /* TODO: ensure remaining parts of room are still joined */

            if (!croom->irregular)
                impossible("regular room in joined map");
        } else {
            /* total overlap, remove the room */
            remove_room((unsigned) i);
        }
    }
}

/*
 * Remove roomno from the rooms array, decrementing nroom.  Also updates
 * all level roomno values of affected higher numbered rooms.  Assumes
 * level structure contents corresponding to roomno have already been reset.
 * Currently handles only the removal of rooms that have no subrooms.
 */
static void
remove_room(unsigned int roomno)
{
    struct mkroom *croom = &g.rooms[roomno];
    struct mkroom *maxroom = &g.rooms[--g.nroom];
    int i, j;
    unsigned oroomno;

    if (croom != maxroom) {
        /* since the order in the array only matters for making corridors,
         * copy the last room over the one being removed on the assumption
         * that corridors have already been dug. */
        (void) memcpy((genericptr_t) croom, (genericptr_t) maxroom,
                      sizeof(struct mkroom));

        /* since maxroom moved, update affected level roomno values */
        oroomno = g.nroom + ROOMOFFSET;
        roomno += ROOMOFFSET;
        for (i = croom->lx; i <= croom->hx; ++i)
            for (j = croom->ly; j <= croom->hy; ++j) {
                if (levl[i][j].roomno == oroomno)
                    levl[i][j].roomno = roomno;
            }
    }

    maxroom->hx = -1; /* just like add_room */
}

#define N_P1_ITER 1 /* tune map generation via this value */
#define N_P2_ITER 1 /* tune map generation via this value */
#define N_P3_ITER 2 /* tune map smoothing via this value */

boolean
litstate_rnd(int litstate)
{
    if (litstate < 0)
        return (rnd(1 + abs(depth(&u.uz))) < 11 && rn2(77)) ? TRUE : FALSE;
    return (boolean) litstate;
}

void
mkmap(lev_init* init_lev)
{
    schar bg_typ = init_lev->bg, fg_typ = init_lev->fg;
    boolean smooth = init_lev->smoothed, join = init_lev->joined;
    xint16 lit = init_lev->lit, walled = init_lev->walled;
    int i;

    lit = litstate_rnd(lit);

    g.new_locations = (char *) alloc((WIDTH + 1) * HEIGHT);

    init_map(bg_typ);
    init_fill(bg_typ, fg_typ);

    for (i = 0; i < N_P1_ITER; i++)
        pass_one(bg_typ, fg_typ);

    for (i = 0; i < N_P2_ITER; i++)
        pass_two(bg_typ, fg_typ);

    if (smooth)
        for (i = 0; i < N_P3_ITER; i++)
            pass_three(bg_typ, fg_typ);

    if (join)
        join_map(bg_typ, fg_typ);

    finish_map(fg_typ, bg_typ, (boolean) lit, (boolean) walled,
               init_lev->icedpools);
    /* a walled, joined level is cavernous, not mazelike -dlc */
    if (walled && join) {
        g.level.flags.is_maze_lev = FALSE;
        g.level.flags.is_cavernous_lev = TRUE;
    }
    free(g.new_locations);
}

/*mkmap.c*/

/* NetHack 3.6	extralev.c	$NHDT-Date: 1446975468 2015/11/08 09:37:48 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/*      Copyright 1988, 1989 by Ken Arromdee                      */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Support code for "rogue"-style level.
 */

#include "hack.h"

struct rogueroom {
    xchar rlx, rly;
    xchar dx, dy;
    boolean real;
    uchar doortable;
    int nroom; /* Only meaningful for "real" rooms */
};
#define UP 1
#define DOWN 2
#define LEFT 4
#define RIGHT 8

static NEARDATA struct rogueroom r[3][3];
STATIC_DCL void FDECL(roguejoin, (int, int, int, int, int));
STATIC_DCL void FDECL(roguecorr, (int, int, int));
STATIC_DCL void FDECL(miniwalk, (int, int));

STATIC_OVL
void
roguejoin(x1, y1, x2, y2, horiz)
int x1, y1, x2, y2;
int horiz;
{
    register int x, y, middle;
    if (horiz) {
        middle = x1 + rn2(x2 - x1 + 1);
        for (x = min(x1, middle); x <= max(x1, middle); x++)
            corr(x, y1);
        for (y = min(y1, y2); y <= max(y1, y2); y++)
            corr(middle, y);
        for (x = min(middle, x2); x <= max(middle, x2); x++)
            corr(x, y2);
    } else {
        middle = y1 + rn2(y2 - y1 + 1);
        for (y = min(y1, middle); y <= max(y1, middle); y++)
            corr(x1, y);
        for (x = min(x1, x2); x <= max(x1, x2); x++)
            corr(x, middle);
        for (y = min(middle, y2); y <= max(middle, y2); y++)
            corr(x2, y);
    }
}

STATIC_OVL
void
roguecorr(x, y, dir)
int x, y, dir;
{
    register int fromx, fromy, tox, toy;

    if (dir == DOWN) {
        r[x][y].doortable &= ~DOWN;
        if (!r[x][y].real) {
            fromx = r[x][y].rlx;
            fromy = r[x][y].rly;
            fromx += 1 + 26 * x;
            fromy += 7 * y;
        } else {
            fromx = r[x][y].rlx + rn2(r[x][y].dx);
            fromy = r[x][y].rly + r[x][y].dy;
            fromx += 1 + 26 * x;
            fromy += 7 * y;
            if (!IS_WALL(levl[fromx][fromy].typ))
                impossible("down: no wall at %d,%d?", fromx, fromy);
            dodoor(fromx, fromy, &rooms[r[x][y].nroom]);
            levl[fromx][fromy].doormask = D_NODOOR;
            fromy++;
        }
        if (y >= 2) {
            impossible("down door from %d,%d going nowhere?", x, y);
            return;
        }
        y++;
        r[x][y].doortable &= ~UP;
        if (!r[x][y].real) {
            tox = r[x][y].rlx;
            toy = r[x][y].rly;
            tox += 1 + 26 * x;
            toy += 7 * y;
        } else {
            tox = r[x][y].rlx + rn2(r[x][y].dx);
            toy = r[x][y].rly - 1;
            tox += 1 + 26 * x;
            toy += 7 * y;
            if (!IS_WALL(levl[tox][toy].typ))
                impossible("up: no wall at %d,%d?", tox, toy);
            dodoor(tox, toy, &rooms[r[x][y].nroom]);
            levl[tox][toy].doormask = D_NODOOR;
            toy--;
        }
        roguejoin(fromx, fromy, tox, toy, FALSE);
        return;
    } else if (dir == RIGHT) {
        r[x][y].doortable &= ~RIGHT;
        if (!r[x][y].real) {
            fromx = r[x][y].rlx;
            fromy = r[x][y].rly;
            fromx += 1 + 26 * x;
            fromy += 7 * y;
        } else {
            fromx = r[x][y].rlx + r[x][y].dx;
            fromy = r[x][y].rly + rn2(r[x][y].dy);
            fromx += 1 + 26 * x;
            fromy += 7 * y;
            if (!IS_WALL(levl[fromx][fromy].typ))
                impossible("down: no wall at %d,%d?", fromx, fromy);
            dodoor(fromx, fromy, &rooms[r[x][y].nroom]);
            levl[fromx][fromy].doormask = D_NODOOR;
            fromx++;
        }
        if (x >= 2) {
            impossible("right door from %d,%d going nowhere?", x, y);
            return;
        }
        x++;
        r[x][y].doortable &= ~LEFT;
        if (!r[x][y].real) {
            tox = r[x][y].rlx;
            toy = r[x][y].rly;
            tox += 1 + 26 * x;
            toy += 7 * y;
        } else {
            tox = r[x][y].rlx - 1;
            toy = r[x][y].rly + rn2(r[x][y].dy);
            tox += 1 + 26 * x;
            toy += 7 * y;
            if (!IS_WALL(levl[tox][toy].typ))
                impossible("left: no wall at %d,%d?", tox, toy);
            dodoor(tox, toy, &rooms[r[x][y].nroom]);
            levl[tox][toy].doormask = D_NODOOR;
            tox--;
        }
        roguejoin(fromx, fromy, tox, toy, TRUE);
        return;
    } else
        impossible("corridor in direction %d?", dir);
}

/* Modified walkfrom() from mkmaze.c */
STATIC_OVL
void
miniwalk(x, y)
int x, y;
{
    register int q, dir;
    int dirs[4];

    while (1) {
        q = 0;
#define doorhere (r[x][y].doortable)
        if (x > 0 && (!(doorhere & LEFT))
            && (!r[x - 1][y].doortable || !rn2(10)))
            dirs[q++] = 0;
        if (x < 2 && (!(doorhere & RIGHT))
            && (!r[x + 1][y].doortable || !rn2(10)))
            dirs[q++] = 1;
        if (y > 0 && (!(doorhere & UP))
            && (!r[x][y - 1].doortable || !rn2(10)))
            dirs[q++] = 2;
        if (y < 2 && (!(doorhere & DOWN))
            && (!r[x][y + 1].doortable || !rn2(10)))
            dirs[q++] = 3;
        /* Rogue levels aren't just 3 by 3 mazes; they have some extra
         * connections, thus that 1/10 chance
         */
        if (!q)
            return;
        dir = dirs[rn2(q)];
        switch (dir) { /* Move in direction */
        case 0:
            doorhere |= LEFT;
            x--;
            doorhere |= RIGHT;
            break;
        case 1:
            doorhere |= RIGHT;
            x++;
            doorhere |= LEFT;
            break;
        case 2:
            doorhere |= UP;
            y--;
            doorhere |= DOWN;
            break;
        case 3:
            doorhere |= DOWN;
            y++;
            doorhere |= UP;
            break;
        }
        miniwalk(x, y);
    }
}

void
makeroguerooms()
{
    register int x, y;
    /* Rogue levels are structured 3 by 3, with each section containing
     * a room or an intersection.  The minimum width is 2 each way.
     * One difference between these and "real" Rogue levels: real Rogue
     * uses 24 rows and NetHack only 23.  So we cheat a bit by making the
     * second row of rooms not as deep.
     *
     * Each normal space has 6/7 rows and 25 columns in which a room may
     * actually be placed.  Walls go from rows 0-5/6 and columns 0-24.
     * Not counting walls, the room may go in
     * rows 1-5 and columns 1-23 (numbering starting at 0).  A room
     * coordinate of this type may be converted to a level coordinate
     * by adding 1+28*x to the column, and 7*y to the row.  (The 1
     * is because column 0 isn't used [we only use 1-78]).
     * Room height may be 2-4 (2-5 on last row), length 2-23 (not
     * counting walls).
     */
#define here r[x][y]

    nroom = 0;
    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++) {
            /* Note: we want to insure at least 1 room.  So, if the
             * first 8 are all dummies, force the last to be a room.
             */
            if (!rn2(5) && (nroom || (x < 2 && y < 2))) {
                /* Arbitrary: dummy rooms may only go where real
                 * ones do.
                 */
                here.real = FALSE;
                here.rlx = rn1(22, 2);
                here.rly = rn1((y == 2) ? 4 : 3, 2);
            } else {
                here.real = TRUE;
                here.dx = rn1(22, 2); /* 2-23 long, plus walls */
                here.dy = rn1((y == 2) ? 4 : 3, 2); /* 2-5 high, plus walls */

                /* boundaries of room floor */
                here.rlx = rnd(23 - here.dx + 1);
                here.rly = rnd(((y == 2) ? 5 : 4) - here.dy + 1);
                nroom++;
            }
            here.doortable = 0;
        }
    miniwalk(rn2(3), rn2(3));
    nroom = 0;
    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++) {
            if (here.real) { /* Make a room */
                int lowx, lowy, hix, hiy;

                r[x][y].nroom = nroom;
                smeq[nroom] = nroom;

                lowx = 1 + 26 * x + here.rlx;
                lowy = 7 * y + here.rly;
                hix = 1 + 26 * x + here.rlx + here.dx - 1;
                hiy = 7 * y + here.rly + here.dy - 1;
                /* Strictly speaking, it should be lit only if above
                 * level 10, but since Rogue rooms are only
                 * encountered below level 10, use !rn2(7).
                 */
                add_room(lowx, lowy, hix, hiy, (boolean) !rn2(7), OROOM,
                         FALSE);
            }
        }

    /* Now, add connecting corridors. */
    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++) {
            if (here.doortable & DOWN)
                roguecorr(x, y, DOWN);
            if (here.doortable & RIGHT)
                roguecorr(x, y, RIGHT);
            if (here.doortable & LEFT)
                impossible("left end of %d, %d never connected?", x, y);
            if (here.doortable & UP)
                impossible("up end of %d, %d never connected?", x, y);
        }
}

void
corr(x, y)
int x, y;
{
    if (rn2(50)) {
        levl[x][y].typ = CORR;
    } else {
        levl[x][y].typ = SCORR;
    }
}

void
makerogueghost()
{
    register struct monst *ghost;
    struct obj *ghostobj;
    struct mkroom *croom;
    int x, y;

    if (!nroom)
        return; /* Should never happen */
    croom = &rooms[rn2(nroom)];
    x = somex(croom);
    y = somey(croom);
    if (!(ghost = makemon(&mons[PM_GHOST], x, y, NO_MM_FLAGS)))
        return;
    ghost->msleeping = 1;
    ghost = christen_monst(ghost, roguename());

    if (rn2(4)) {
        ghostobj = mksobj_at(FOOD_RATION, x, y, FALSE, FALSE);
        ghostobj->quan = (long) rnd(7);
        ghostobj->owt = weight(ghostobj);
    }
    if (rn2(2)) {
        ghostobj = mksobj_at(MACE, x, y, FALSE, FALSE);
        ghostobj->spe = rnd(3);
        if (rn2(4))
            curse(ghostobj);
    } else {
        ghostobj = mksobj_at(TWO_HANDED_SWORD, x, y, FALSE, FALSE);
        ghostobj->spe = rnd(5) - 2;
        if (rn2(4))
            curse(ghostobj);
    }
    ghostobj = mksobj_at(BOW, x, y, FALSE, FALSE);
    ghostobj->spe = 1;
    if (rn2(4))
        curse(ghostobj);

    ghostobj = mksobj_at(ARROW, x, y, FALSE, FALSE);
    ghostobj->spe = 0;
    ghostobj->quan = (long) rn1(10, 25);
    ghostobj->owt = weight(ghostobj);
    if (rn2(4))
        curse(ghostobj);

    if (rn2(2)) {
        ghostobj = mksobj_at(RING_MAIL, x, y, FALSE, FALSE);
        ghostobj->spe = rn2(3);
        if (!rn2(3))
            ghostobj->oerodeproof = TRUE;
        if (rn2(4))
            curse(ghostobj);
    } else {
        ghostobj = mksobj_at(PLATE_MAIL, x, y, FALSE, FALSE);
        ghostobj->spe = rnd(5) - 2;
        if (!rn2(3))
            ghostobj->oerodeproof = TRUE;
        if (rn2(4))
            curse(ghostobj);
    }
    if (rn2(2)) {
        ghostobj = mksobj_at(FAKE_AMULET_OF_YENDOR, x, y, TRUE, FALSE);
        ghostobj->known = TRUE;
    }
}

/*extralev.c*/

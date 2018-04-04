/* NetHack 3.6	rect.c	$NHDT-Date: 1432512774 2015/05/25 00:12:54 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) 1990 by Jean-Christophe Collet                   */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

int FDECL(get_rect_ind, (NhRect *));

STATIC_DCL boolean FDECL(intersect, (NhRect *, NhRect *, NhRect *));

/*
 * In this file, we will handle the various rectangle functions we
 * need for room generation.
 */

#define MAXRECT 50

/* According to 1.3d and 3.0.0 source, these represent the minimum amount of
 * required space around a room in any direction, counting the walls as part of
 * the room.
 * Rooms' XLIM and YLIM buffers can overlap with other rooms. */
#define XLIM 4
#define YLIM 3

static NhRect rect[MAXRECT + 1];
static int rect_cnt;

/*
 * Initialisation of internal structures. Should be called for every
 * new level to be built.
 * Specifically, this creates one giant rectangle spanning the entire level.
 * Note: if levels can never go below x=3 or y=2, why does this code allow
 * otherwise?
 */
void
init_rect()
{
    rect_cnt = 1;
    rect[0].lx = rect[0].ly = 0;
    rect[0].hx = COLNO - 1;
    rect[0].hy = ROWNO - 1;
}

/* Find and return the index of one precise NhRect, or -1 if it doesn't exist
 * in the rect array. */
int
get_rect_ind(r)
NhRect *r;
{
    register NhRect *rectp;
    register int lx, ly, hx, hy;
    register int i;

    lx = r->lx;
    ly = r->ly;
    hx = r->hx;
    hy = r->hy;
    for (i = 0, rectp = &rect[0]; i < rect_cnt; i++, rectp++)
        if (lx == rectp->lx && ly == rectp->ly && hx == rectp->hx
            && hy == rectp->hy)
            return i;
    return -1;
}

/* Look through the rect array for a free rectangle that completely contains
 * the given rectangle, and return it, or NULL if no such rectangle exists.
 */
NhRect *
get_rect(r)
NhRect *r;
{
    register NhRect *rectp;
    register int lx, ly, hx, hy;
    register int i;

    lx = r->lx;
    ly = r->ly;
    hx = r->hx;
    hy = r->hy;
    for (i = 0, rectp = &rect[0]; i < rect_cnt; i++, rectp++)
        if (lx >= rectp->lx && ly >= rectp->ly && hx <= rectp->hx
            && hy <= rectp->hy)
            return rectp;
    return 0;
}

/* Pick and return a random NhRect. */
NhRect *
rnd_rect()
{
    return rect_cnt > 0 ? &rect[rn2(rect_cnt)] : 0;
}

/*
 * Compute the intersection between the rectangles r1 and r2.
 * If they don't intersect at all, return FALSE.
 * If they do, set r3 to be the intersection, and return TRUE.
 */
STATIC_OVL boolean
intersect(r1, r2, r3)
NhRect *r1, *r2, *r3;
{
    if (r2->lx > r1->hx || r2->ly > r1->hy || r2->hx < r1->lx
        || r2->hy < r1->ly)
        return FALSE;

    r3->lx = (r2->lx > r1->lx ? r2->lx : r1->lx);
    r3->ly = (r2->ly > r1->ly ? r2->ly : r1->ly);
    r3->hx = (r2->hx > r1->hx ? r1->hx : r2->hx);
    r3->hy = (r2->hy > r1->hy ? r1->hy : r2->hy);

    if (r3->lx > r3->hx || r3->ly > r3->hy)
        return FALSE;
    return TRUE;
}

/* Remove the given rectangle from the rect array. */
void
remove_rect(r)
NhRect *r;
{
    int ind;

    ind = get_rect_ind(r);
    if (ind >= 0)
        rect[ind] = rect[--rect_cnt];
}

/* Add the given rectangle to the rect array. */
void
add_rect(r)
NhRect *r;
{
    if (rect_cnt >= MAXRECT) {
        if (wizard)
            pline("MAXRECT may be too small.");
        return;
    }
    /* Check that this NhRect is not included in another one */
    if (get_rect(r))
        return;
    rect[rect_cnt] = *r;
    rect_cnt++;
}

/* Split up r1 into multiple smaller rectangles because of r2 being placed.
 * Assumes that r2 is completely contained within r1, and that r1 exists in
 * the rect[] array.
 * Specifically, this will try to make up to four new rectangles out of r1
 * r1 was already in the list and r2 is included in r1.
 * The code that adds the new rectangles appears to add them only if they could
 * feasibly hold another room.
 * Note that the smaller rectangles can and do intersect! They'll intersect
 * anywhere that isn't directly in line with r2.
 */
void
split_rects(r1, r2)
NhRect *r1, *r2;
{
    NhRect r, old_r;
    int i;

    old_r = *r1;
    remove_rect(r1);

    /* Recurse this function on any other rectangles in rect[] that happen to
     * intersect.
     * Under the assumptions of this function, that r1 did in fact completely
     * contain r2, and that r1 was in the list, shouldn't this loop not
     * actually do anything? */
    for (i = rect_cnt - 1; i >= 0; i--)
        if (intersect(&rect[i], r2, &r))
            split_rects(&rect[i], &r);

    /* If r2's left edge is at least 2*YLIM + 6 spaces to the right of old_r's
     * left edge, add a new rectangle with the same coordinates as old_r except
     * that its right edge is set to near r2's left edge, with one unoccupied
     * buffer space in between.
     * This guarantees that the new shrunken rectangle will be at least
     * 2*YLIM + 4 spaces wide (4 being the minimum width/height for a room
     * counting walls).
     * Special case if old_r was on the right edge of the map:
     * r2's left edge only needs to be at least YLIM + 7 spaces to the right of
     * old_r's left edge, and the new shrunken rectangle will be at least
     * YLIM + 5 spaces wide.
     *
     * Possible bug here? This is for when old_r is touching the bottom of
     * the map, and it's considering the case where r2 is comparatively closer
     * to the bottom than old_r.ly.
     * We'd end up creating a shrunken rectangle that's only YLIM+5 spaces high
     * to the *top* of r2.
     * It seems like the not-multiplying-YLIM-by-2 code is intended to address
     * the fact that new rectangles on the bottom of the map only need a buffer
     * of YLIM in one direction. But the rectangle being created here isn't on
     * the bottom of the map at all. */
    if (r2->ly - old_r.ly - 1
        > (old_r.hy < ROWNO - 1 ? 2 * YLIM : YLIM + 1) + 4) {
        r = old_r;
        r.hy = r2->ly - 2;
        add_rect(&r);
    }
    /* Do this exact same process for the other three directions. */
    if (r2->lx - old_r.lx - 1
        > (old_r.hx < COLNO - 1 ? 2 * XLIM : XLIM + 1) + 4) {
        r = old_r;
        r.hx = r2->lx - 2;
        add_rect(&r);
    }
    if (old_r.hy - r2->hy - 1 > (old_r.ly > 0 ? 2 * YLIM : YLIM + 1) + 4) {
        r = old_r;
        r.ly = r2->hy + 2;
        add_rect(&r);
    }
    if (old_r.hx - r2->hx - 1 > (old_r.lx > 0 ? 2 * XLIM : XLIM + 1) + 4) {
        r = old_r;
        r.lx = r2->hx + 2;
        add_rect(&r);
    }
}

/*rect.c*/

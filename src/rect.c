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
#define XLIM 4
#define YLIM 3

static NhRect rect[MAXRECT + 1];
static int rect_cnt;

/*
 * Initialisation of internal structures. Should be called for every
 * new level to be build...
 */

void
init_rect()
{
    rect_cnt = 1;
    rect[0].lx = rect[0].ly = 0;
    rect[0].hx = COLNO - 1;
    rect[0].hy = ROWNO - 1;
}

/*
 * Search Index of one precise NhRect.
 *
 */

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

/*
 * Search a free rectangle that include the one given in arg
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

/*
 * Get some random NhRect from the list.
 */

NhRect *
rnd_rect()
{
    return rect_cnt > 0 ? &rect[rn2(rect_cnt)] : 0;
}

/*
 * Search intersection between two rectangles (r1 & r2).
 * return TRUE if intersection exist and put it in r3.
 * otherwise returns FALSE
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

/*
 * Remove a rectangle from the list of free NhRect.
 */

void
remove_rect(r)
NhRect *r;
{
    int ind;

    ind = get_rect_ind(r);
    if (ind >= 0)
        rect[ind] = rect[--rect_cnt];
}

/*
 * Add a NhRect to the list.
 */

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

/*
 * Okay, here we have two rectangles (r1 & r2).
 * r1 was already in the list and r2 is included in r1.
 * What we want is to allocate r2, that is split r1 into smaller rectangles
 * then remove it.
 */

void
split_rects(r1, r2)
NhRect *r1, *r2;
{
    NhRect r, old_r;
    int i;

    old_r = *r1;
    remove_rect(r1);

    /* Walk down since rect_cnt & rect[] will change... */
    for (i = rect_cnt - 1; i >= 0; i--)
        if (intersect(&rect[i], r2, &r))
            split_rects(&rect[i], &r);

    if (r2->ly - old_r.ly - 1
        > (old_r.hy < ROWNO - 1 ? 2 * YLIM : YLIM + 1) + 4) {
        r = old_r;
        r.hy = r2->ly - 2;
        add_rect(&r);
    }
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

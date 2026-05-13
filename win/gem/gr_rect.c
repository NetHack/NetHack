/* NetHack 3.6	gr_rect.c	$NHDT-Date: 1432512810 2015/05/25 00:13:30 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
 */
/* Copyright (c) Christian Bressler, 2001 */
/* NetHack may be freely redistributed.  See license for details. */
/* This is an almost exact copy of qt_clust.cpp */
/* gr_rect.c */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "gr_rect.h"
dirty_rect *
new_dirty_rect(int size)
{
    dirty_rect *new = NULL;
    if (size > 0) {
        new = (dirty_rect *) calloc(1L, sizeof(dirty_rect));
        if (new) {
            new->rects = (GRECT *) calloc((long) size, sizeof(GRECT));
            if (new->rects == NULL) {
                free(new);
                return (NULL);
            }
            new->max = size;
        }
    }
    return (new);
}
void
delete_dirty_rect(dirty_rect *this)
{
    if (this == NULL)
        return;
    if (this->rects)
        free(this->rects);
    /* In case the Pointer is reused wrongly */
    this->rects = NULL;
    this->max = 0;
    this->used = 0;
    free(this);
}
static int gc_inside(GRECT *frame, GRECT *test);
static int gc_touch(GRECT *frame, GRECT *test);
static void gc_combine(GRECT *frame, GRECT *test);
static long gc_area(GRECT *area);
int
add_dirty_rect(dirty_rect *dr, GRECT *area)
{
    int cursor;
    long lowestcost = 9999999L;
    int cheapest = -1;
    int cheapestmerge1 = -1;
    int cheapestmerge2 = -1;
    int merge1;
    int merge2;
    for (cursor = 0; cursor < dr->used; cursor++) {
        if (gc_inside(&dr->rects[cursor], area)) {
            /* Wholly contained already. */
            return (TRUE);
        }
    }
    for (cursor = 0; cursor < dr->used; cursor++) {
        if (gc_touch(&dr->rects[cursor], area)) {
            GRECT larger = dr->rects[cursor];
            long cost;
            gc_combine(&larger, area);
            cost = gc_area(&larger) - gc_area(&dr->rects[cursor]);
            if (cost < lowestcost) {
                int bad = FALSE, c;
                for (c = 0; c < dr->used && !bad; c++) {
                    bad = gc_touch(&dr->rects[c], &larger) && c != cursor;
                }
                if (!bad) {
                    cheapest = cursor;
                    lowestcost = cost;
                }
            }
        }
    }
    if (cheapest >= 0) {
        gc_combine(&dr->rects[cheapest], area);
        return (TRUE);
    }
    if (dr->used < dr->max) {
        dr->rects[dr->used++] = *area;
        return (TRUE);
    }
    // Do cheapest of:
    // 	add to closest cluster
    // 	do cheapest cluster merge, add to new cluster
    lowestcost = 9999999L;
    cheapest = -1;
    for (cursor = 0; cursor < dr->used; cursor++) {
        GRECT larger = dr->rects[cursor];
        long cost;
        gc_combine(&larger, area);
        cost = gc_area(&larger) - gc_area(&dr->rects[cursor]);
        if (cost < lowestcost) {
            int bad = FALSE, c;
            for (c = 0; c < dr->used && !bad; c++) {
                bad = gc_touch(&dr->rects[c], &larger) && c != cursor;
            }
            if (!bad) {
                cheapest = cursor;
                lowestcost = cost;
            }
        }
    }
    // XXX could make an heuristic guess as to whether we
    // XXX need to bother looking for a cheap merge.
    for (merge1 = 0; merge1 < dr->used; merge1++) {
        for (merge2 = 0; merge2 < dr->used; merge2++) {
            if (merge1 != merge2) {
                GRECT larger = dr->rects[merge1];
                long cost;
                gc_combine(&larger, &dr->rects[merge2]);
                cost = gc_area(&larger) - gc_area(&dr->rects[merge1])
                       - gc_area(&dr->rects[merge2]);
                if (cost < lowestcost) {
                    int bad = FALSE, c;
                    for (c = 0; c < dr->used && !bad; c++) {
                        bad = gc_touch(&dr->rects[c], &larger) && c != cursor;
                    }
                    if (!bad) {
                        cheapestmerge1 = merge1;
                        cheapestmerge2 = merge2;
                        lowestcost = cost;
                    }
                }
            }
        }
    }
    if (cheapestmerge1 >= 0) {
        gc_combine(&dr->rects[cheapestmerge1], &dr->rects[cheapestmerge2]);
        dr->rects[cheapestmerge2] = dr->rects[dr->used - 1];
        dr->rects[dr->used - 1] = *area;
    } else {
        gc_combine(&dr->rects[cheapest], area);
    }
    // NB: clusters do not intersect (or intersection will
    //	 overwrite).  This is a result of the above algorithm,
    //	 given the assumption that (x,y) are ordered topleft
    //	 to bottomright.
    return (TRUE);
}
int
get_dirty_rect(dirty_rect *dr, GRECT *area)
{
    if (dr == NULL || area == NULL || dr->rects == NULL || dr->used <= 0
        || dr->max <= 0)
        return (FALSE);
    *area = dr->rects[--dr->used];
    return (TRUE);
}
int
clear_dirty_rect(dirty_rect *dr)
{
    if (dr)
        dr->used = 0;
    return (TRUE);
}
int
resize_dirty_rect(dirty_rect *dr, int new_size)
{
    return (FALSE);
}
static int
gc_inside(GRECT *frame, GRECT *test)
{
    if (frame && test && frame->g_x <= test->g_x && frame->g_y <= test->g_y
        && frame->g_x + frame->g_w >= test->g_x + test->g_w
        && frame->g_y + frame->g_h >= test->g_y + test->g_h)
        return (TRUE);
    return (FALSE);
}
static int
gc_touch(GRECT *frame, GRECT *test)
{
    GRECT tmp = { test->g_x - 1, test->g_y - 1, test->g_w + 2,
                  test->g_h + 2 };
    return (rc_intersect(frame, &tmp));
}
static void
gc_combine(GRECT *frame, GRECT *test)
{
    if (!frame || !test)
        return;
    if (frame->g_x > test->g_x) {
        frame->g_w += frame->g_x - test->g_x;
        frame->g_x = test->g_x;
    }
    if (frame->g_y > test->g_y) {
        frame->g_h += frame->g_y - test->g_y;
        frame->g_y = test->g_y;
    }
    if (frame->g_x + frame->g_w < test->g_x + test->g_w)
        frame->g_w = test->g_x + test->g_w - frame->g_x;
    if (frame->g_y + frame->g_h < test->g_y + test->g_h)
        frame->g_h = test->g_y + test->g_h - frame->g_y;
}
static long
gc_area(GRECT *area)
{
    return ((long) area->g_h * (long) area->g_w);
}

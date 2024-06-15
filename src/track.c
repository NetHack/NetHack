/* NetHack 3.7	track.c	$NHDT-Date: 1596498219 2020/08/03 23:43:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */
/* track.c - version 1.0.2 */

#include "hack.h"

#define UTSZ 100

static NEARDATA int utcnt, utpnt;
static NEARDATA coord utrack[UTSZ];

void
initrack(void)
{
    utcnt = utpnt = 0;
    (void) memset((genericptr_t) &utrack, 0, sizeof(utrack));
}

/* add to track */
void
settrack(void)
{
    if (utcnt < UTSZ)
        utcnt++;
    if (utpnt == UTSZ)
        utpnt = 0;
    utrack[utpnt].x = u.ux;
    utrack[utpnt].y = u.uy;
    utpnt++;
}

/* get a track coord on or next to x,y and last tracked by hero,
   returns null if no such track */
coord *
gettrack(coordxy x, coordxy y)
{
    int cnt, ndist;
    coord *tc;
    cnt = utcnt;
    for (tc = &utrack[utpnt]; cnt--;) {
        if (tc == utrack)
            tc = &utrack[UTSZ - 1];
        else
            tc--;
        ndist = distmin(x, y, tc->x, tc->y);

        if (ndist <= 1)
            return (ndist ? tc : 0);
    }
    return (coord *) 0;
}

/* return TRUE if x,y has hero tracks on it */
boolean
hastrack(coordxy x, coordxy y)
{
    int i;

    for (i = 0; i < utcnt; i++)
        if (utrack[i].x == x && utrack[i].y == y)
            return TRUE;

    return FALSE;
}

/* save the hero tracking info */
void
save_track(NHFILE *nhfp)
{
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel) {
            int i;

            bwrite(nhfp->fd, (genericptr_t) &utcnt, sizeof utcnt);
            bwrite(nhfp->fd, (genericptr_t) &utpnt, sizeof utpnt);
            for (i = 0; i < utcnt; i++) {
                bwrite(nhfp->fd, (genericptr_t) &utrack[i].x, sizeof utrack[i].x);
                bwrite(nhfp->fd, (genericptr_t) &utrack[i].y, sizeof utrack[i].y);
            }
        }
    }
    if (release_data(nhfp))
        initrack();
}

/* restore the hero tracking info */
void
rest_track(NHFILE *nhfp)
{
    if (nhfp->structlevel) {
        int i;

        mread(nhfp->fd, (genericptr_t) &utcnt, sizeof utcnt);
        mread(nhfp->fd, (genericptr_t) &utpnt, sizeof utpnt);
        if (utcnt > UTSZ || utpnt > UTSZ)
            panic("rest_track: impossible pt counts");
        for (i = 0; i < utcnt; i++) {
            mread(nhfp->fd, (genericptr_t) &utrack[i].x, sizeof utrack[i].x);
            mread(nhfp->fd, (genericptr_t) &utrack[i].y, sizeof utrack[i].y);
        }
    }
}

/*track.c*/

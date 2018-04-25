/* NetHack 3.6	track.c	$NHDT-Date: 1432512769 2015/05/25 00:12:49 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */
/* track.c - version 1.0.2 */

#include "hack.h"

#define UTSZ 50

STATIC_VAR NEARDATA int utcnt, utpnt;
STATIC_VAR NEARDATA coord utrack[UTSZ];

void
initrack()
{
    utcnt = utpnt = 0;
}

/* add to track */
void
settrack()
{
    if (utcnt < UTSZ)
        utcnt++;
    if (utpnt == UTSZ)
        utpnt = 0;
    utrack[utpnt].x = u.ux;
    utrack[utpnt].y = u.uy;
    utpnt++;
}

coord *
gettrack(x, y)
register int x, y;
{
    register int cnt, ndist;
    register coord *tc;
    cnt = utcnt;
    for (tc = &utrack[utpnt]; cnt--;) {
        if (tc == utrack)
            tc = &utrack[UTSZ - 1];
        else
            tc--;
        ndist = distmin(x, y, tc->x, tc->y);

        /* if far away, skip track entries til we're closer */
        if (ndist > 2) {
            ndist -= 2; /* be careful due to extra decrement at top of loop */
            cnt -= ndist;
            if (cnt <= 0)
                return (coord *) 0; /* too far away, no matches possible */
            if (tc < &utrack[ndist])
                tc += (UTSZ - ndist);
            else
                tc -= ndist;
        } else if (ndist <= 1)
            return (ndist ? tc : 0);
    }
    return (coord *) 0;
}

/*track.c*/

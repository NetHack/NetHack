/* NetHack 5.0	track.c	$NHDT-Date: 1596498219 2020/08/03 23:43:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
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

#ifndef SFCTOOL
/* add to track */
void
settrack(void)
{
    if ((uleft && uleft->otyp == RIN_STEALTH)
        || (uright && uright->otyp == RIN_STEALTH))
        return;

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
#endif /* !SFCTOOL */

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
    if (update_file(nhfp)) {
        int i;

        Sfo_int(nhfp, &utcnt, "track-utcnt");
        Sfo_int(nhfp, &utpnt, "track-utpnt");
        for (i = 0; i < utcnt; i++) {
            Sfo_nhcoord(nhfp, &utrack[i], "utrack");
        }
    }
    if (release_data(nhfp))
        initrack();
}

/* restore the hero tracking info */
void
rest_track(NHFILE *nhfp)
{
    int i;

    Sfi_int(nhfp, &utcnt, "track-utcnt");
    Sfi_int(nhfp, &utpnt, "track-utpnt");

    if (utcnt > UTSZ || utpnt > UTSZ)
        panic("rest_track: impossible pt counts");
    for (i = 0; i < utcnt; i++) {
        Sfi_nhcoord(nhfp, &utrack[i], "utrack");
    }
}

/*track.c*/

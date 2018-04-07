/* NetHack 3.6	mklev.c	$NHDT-Date: 1511681724 2017/11/26 07:35:24 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* for UNIX, Rand #def'd to (long)lrand48() or (long)random() */
/* croom->lx etc are schar (width <= int), so % arith ensures that */
/* conversion of result to int is reasonable */

STATIC_DCL void FDECL(mkfount, (int, struct mkroom *));
STATIC_DCL void FDECL(mksink, (struct mkroom *));
STATIC_DCL void FDECL(mkaltar, (struct mkroom *));
STATIC_DCL void FDECL(mkgrave, (struct mkroom *));
STATIC_DCL void NDECL(makevtele);
STATIC_DCL void NDECL(clear_level_structures);
STATIC_DCL void NDECL(makelevel);
STATIC_DCL boolean FDECL(bydoor, (XCHAR_P, XCHAR_P));
STATIC_DCL struct mkroom *FDECL(find_branch_room, (coord *));
STATIC_DCL struct mkroom *FDECL(pos_to_room, (XCHAR_P, XCHAR_P));
STATIC_DCL boolean FDECL(place_niche, (struct mkroom *, int *, int *, int *));
STATIC_DCL void FDECL(makeniche, (int));
STATIC_DCL void NDECL(make_niches);
STATIC_PTR int FDECL(CFDECLSPEC do_comp, (const genericptr,
                                          const genericptr));
STATIC_DCL void FDECL(dosdoor, (XCHAR_P, XCHAR_P, struct mkroom *, int));
STATIC_DCL void FDECL(join, (int, int, BOOLEAN_P));
STATIC_DCL void FDECL(do_room_or_subroom, (struct mkroom *, int, int,
                                           int, int, BOOLEAN_P,
                                           SCHAR_P, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void NDECL(makerooms);
STATIC_DCL void FDECL(finddpos, (coord *, XCHAR_P, XCHAR_P,
                                 XCHAR_P, XCHAR_P));
STATIC_DCL void FDECL(mkinvpos, (XCHAR_P, XCHAR_P, int));
STATIC_DCL void FDECL(mk_knox_portal, (XCHAR_P, XCHAR_P));

#define create_vault() create_room(-1, -1, 2, 2, -1, -1, VAULT, TRUE)
#define init_vault() vault_x = -1
#define do_vault() (vault_x != -1)
static xchar vault_x, vault_y;
static boolean made_branch; /* used only during level creation */

/* Compare two room pointers by their x-coordinate. Used as a callback to
 * qsort.
 * Args must be (const genericptr) so that qsort will always be happy. */
STATIC_PTR int CFDECLSPEC
do_comp(vx, vy)
const genericptr vx;
const genericptr vy;
{
#ifdef LINT
    /* lint complains about possible pointer alignment problems, but we know
       that vx and vy are always properly aligned. Hence, the following
       bogus definition:
    */
    return (vx == vy) ? 0 : -1;
#else
    register const struct mkroom *x, *y;

    x = (const struct mkroom *) vx;
    y = (const struct mkroom *) vy;
    if (x->lx < y->lx)
        return -1;
    return (x->lx > y->lx);
#endif /* LINT */
}

/* Find a valid position to place a door within the rectangle bounded by
 * (xl, yl, xh, yh), as defined by okdoor(). First, try to pick a single random
 * spot, then iterate over the entire area.
 * If it can't find any valid places it'll just default to an
 * existing door.
 */
STATIC_OVL void
finddpos(cc, xl, yl, xh, yh)
coord *cc;
xchar xl, yl, xh, yh;
{
    register xchar x, y;

    x = rn1(xh - xl + 1, xl);
    y = rn1(yh - yl + 1, yl);
    if (okdoor(x, y))
        goto gotit;

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (okdoor(x, y))
                goto gotit;

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR)
                goto gotit;
    /* cannot find something reasonable -- strange.
     * should this be impossible()? */
    x = xl;
    y = yh;
gotit:
    cc->x = x;
    cc->y = y;
    return;
}

/* Sort the rooms array, using do_comp as the comparison function. */
void
sort_rooms()
{
#if defined(SYSV) || defined(DGUX)
    qsort((genericptr_t) rooms, (unsigned) nroom, sizeof(struct mkroom),
          do_comp);
#else
    qsort((genericptr_t) rooms, nroom, sizeof(struct mkroom), do_comp);
#endif
}

/* Initialize the croom struct and the portion of the level it sits on. This
 * must be a regular (rectangular) room.
 * lowx, lowy, hix, hiy: the bounding box of the room floor, NOT including its
 *   walls.
 * lit: Whether to light the whole room area.
 * rtype: The room type. This directly sets croom->rtype without calling mkroom
 *   even for special rooms. All randomly generated rooms currently specify
 *   OROOM, but special levels may want to specify a rtype and leave the room
 *   unfilled (e.g. an abandoned temple).
 * special: If FALSE, this function will initialize the room terrain to be a
 *   rectangle of floor surrounded by the appropriate walls. If TRUE, it will
 *   skip this step.
 * is_room: Whether this room is a full room. FALSE if it's a subroom.
 *   Only relevant to wallification and if special = FALSE. */
STATIC_OVL void
do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special, is_room)
register struct mkroom *croom;
int lowx, lowy;
register int hix, hiy;
boolean lit;
schar rtype;
boolean special;
boolean is_room;
{
    register int x, y;
    struct rm *lev;

    /* locations might bump level edges in wall-less rooms */
    /* add/subtract 1 to allow for edge locations */
    if (!lowx)
        lowx++;
    if (!lowy)
        lowy++;
    if (hix >= COLNO - 1)
        hix = COLNO - 2;
    if (hiy >= ROWNO - 1)
        hiy = ROWNO - 2;

    if (lit) {
        for (x = lowx - 1; x <= hix + 1; x++) {
            lev = &levl[x][max(lowy - 1, 0)];
            for (y = lowy - 1; y <= hiy + 1; y++)
                lev++->lit = 1;
        }
        croom->rlit = 1;
    } else
        croom->rlit = 0;

    croom->lx = lowx;
    croom->hx = hix;
    croom->ly = lowy;
    croom->hy = hiy;
    croom->rtype = rtype;
    croom->doorct = 0;
    /* if we're not making a vault, doorindex will still be 0
     * if we are, we'll have problems adding niches to the previous room
     * unless fdoor is at least doorindex
     */
    croom->fdoor = doorindex;
    croom->irregular = FALSE;

    croom->nsubrooms = 0;
    croom->sbrooms[0] = (struct mkroom *) 0;
    if (!special) {
        for (x = lowx - 1; x <= hix + 1; x++)
            for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
                levl[x][y].typ = HWALL;
                levl[x][y].horizontal = 1; /* For open/secret doors. */
            }
        for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
            for (y = lowy; y <= hiy; y++) {
                levl[x][y].typ = VWALL;
                levl[x][y].horizontal = 0; /* For open/secret doors. */
            }
        for (x = lowx; x <= hix; x++) {
            lev = &levl[x][lowy];
            for (y = lowy; y <= hiy; y++)
                lev++->typ = ROOM;
        }
        if (is_room) {
            levl[lowx - 1][lowy - 1].typ = TLCORNER;
            levl[hix + 1][lowy - 1].typ = TRCORNER;
            levl[lowx - 1][hiy + 1].typ = BLCORNER;
            levl[hix + 1][hiy + 1].typ = BRCORNER;
        } else { /* a subroom */
            wallification(lowx - 1, lowy - 1, hix + 1, hiy + 1);
        }
    }
}

/* Adds a new room to the map.
 * Arguments are the same as do_room_or_subroom(), except is_room is hardcoded
 * to TRUE.
 */
void
add_room(lowx, lowy, hix, hiy, lit, rtype, special)
int lowx, lowy, hix, hiy;
boolean lit;
schar rtype;
boolean special;
{
    register struct mkroom *croom;

    croom = &rooms[nroom];
    do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special,
                       (boolean) TRUE);
    croom++;
    croom->hx = -1;
    nroom++;
}

/* Adds a new subroom to the map as part of the given room.
 * Arguments are again the same as those passed to do_room_or_subroom() with
 * is_room hardcoded to FALSE.
 */
void
add_subroom(proom, lowx, lowy, hix, hiy, lit, rtype, special)
struct mkroom *proom;
int lowx, lowy, hix, hiy;
boolean lit;
schar rtype;
boolean special;
{
    register struct mkroom *croom;

    croom = &subrooms[nsubroom];
    do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special,
                       (boolean) FALSE);
    proom->sbrooms[proom->nsubrooms++] = croom;
    croom++;
    croom->hx = -1;
    nsubroom++;
}

/* Repeatedly create rooms and place them on the map until we can't create any
 * more. */
STATIC_OVL void
makerooms()
{
    boolean tried_vault = FALSE;

    /* rnd_rect() will return 0 if no more rects are available... */
    while (nroom < MAXNROFROOMS && rnd_rect()) {
        /* If a certain number of rooms have already been created, and we have
         * not yet tried to make a vault, with 50% probability, try to create
         * one. */
        if (nroom >= (MAXNROFROOMS / 6) && rn2(2) && !tried_vault) {
            tried_vault = TRUE;
            if (create_vault()) {
                /* This won't actually create the room and edit the terrain
                 * with add_room. It'll just set the lx and ly of rooms[nroom]
                 * to represent its location. */
                vault_x = rooms[nroom].lx;
                vault_y = rooms[nroom].ly;
                rooms[nroom].hx = -1;
            }
        } else {
            /* Try to create another random room. If it can't find anywhere for
             * one to go, stop making rooms.
             * Use the parameters for a totally random ordinary room. */
            if (!create_room(-1, -1, -1, -1, -1, -1, OROOM, -1))
                return;
        }
    }
    return;
}

/* Join rooms a and b together by drawing a corridor and placing doors.
 * If nxcor is TRUE, it will be pickier about whether to draw the corridor at
 * all, and will not create doors in !okdoor() locations.
 * The corridor will be made of CORR terrain unless this is an arboreal level
 * in which case it will use ROOM.
 * Afterwards, the smeq values of a and b will be set equal to each other.
 * Should this return boolean (success or failure)? */
STATIC_OVL void
join(a, b, nxcor)
register int a, b;
boolean nxcor;
{
    coord cc, tt, org, dest;
    register xchar tx, ty, xx, yy;
    register struct mkroom *croom, *troom;
    register int dx, dy;

    croom = &rooms[a];
    troom = &rooms[b];

    /* find positions cc and tt for doors in croom and troom
       and direction for a corridor between them */

    /* if either room is not an actual room (hx = -1), or if too many doors
     * exist already, abort */
    if (troom->hx < 0 || croom->hx < 0 || doorindex >= DOORMAX)
        return;

    /* Determine how croom and troom are positioned relative to each other,
     * then pick random positions on their walls that face each other where
     * doors will be created.
     * Note: This has a horizontal bias; if troom is, for instance, both to the
     * right of and below croom, the ordering of the if clauses here will
     * always place the doors on croom's right wall and troom's left wall.
     * This may be intentional, since the playing field is much wider than it
     * is tall. */
    if (troom->lx > croom->hx) {
        /* troom to the right of croom */
        dx = 1;
        dy = 0;
        xx = croom->hx + 1;
        tx = troom->lx - 1;
        finddpos(&cc, xx, croom->ly, xx, croom->hy);
        finddpos(&tt, tx, troom->ly, tx, troom->hy);
    } else if (troom->hy < croom->ly) {
        /* troom below croom */
        dy = -1;
        dx = 0;
        yy = croom->ly - 1;
        finddpos(&cc, croom->lx, yy, croom->hx, yy);
        ty = troom->hy + 1;
        finddpos(&tt, troom->lx, ty, troom->hx, ty);
    } else if (troom->hx < croom->lx) {
        /* troom to the left of croom */
        dx = -1;
        dy = 0;
        xx = croom->lx - 1;
        tx = troom->hx + 1;
        finddpos(&cc, xx, croom->ly, xx, croom->hy);
        finddpos(&tt, tx, troom->ly, tx, troom->hy);
    } else {
        /* otherwise troom must be below croom */
        dy = 1;
        dx = 0;
        yy = croom->hy + 1;
        ty = troom->ly - 1;
        finddpos(&cc, croom->lx, yy, croom->hx, yy);
        finddpos(&tt, troom->lx, ty, troom->hx, ty);
    }
    xx = cc.x;
    yy = cc.y;
    tx = tt.x - dx;
    ty = tt.y - dy;

    /* If nxcor is TRUE and the space outside croom's door isn't stone (maybe
     * some previous corridor has already been drawn here?), abort.
     * TODO: this check should also be converted to != STONE */
    if (nxcor && levl[xx + dx][yy + dy].typ)
        return;

    /* If we can put a door in croom's wall or nxcor is FALSE, do so. */
    if (okdoor(xx, yy) || !nxcor)
        dodoor(xx, yy, croom);

    /* Attempt to dig the corridor. If it fails for some reason, abort. */
    org.x = xx + dx;
    org.y = yy + dy;
    dest.x = tx;
    dest.y = ty;
    if (!dig_corridor(&org, &dest, nxcor, level.flags.arboreal ? ROOM : CORR,
                      STONE))
        return;

    /* We succeeded in digging the corridor.
     * If we can put the door in troom's wall or nxcor is FALSE, do so. */
    if (okdoor(tt.x, tt.y) || !nxcor)
        dodoor(tt.x, tt.y, troom);

    /* Set the smeq values for these rooms to be equal to each other, denoting
     * that these two rooms are now part of the same reachable section of the
     * level.
     * Importantly, this does NOT propagate the smeq value to any other rooms
     * with the to-be-overwritten smeq value! */
    if (smeq[a] < smeq[b])
        smeq[b] = smeq[a];
    else
        smeq[a] = smeq[b];
}

/* Generate corridors connecting all the rooms on the level. */
void
makecorridors()
{
    int a, b, i;
    boolean any = TRUE;

    /* Connect each room to the next room in rooms.
     *
     * Since during normal random level generation, rooms is sorted by order of
     * x-coordinate prior to calling this function, this first step will,
     * unless it hits the !rn2(50), connect each room to the next room to its
     * right, which will set everyone's smeq value to the same number. This
     * will deny the next two loops in this function from getting to connect
     * anything. Occasionally a level will be created by this having a series
     * of up-and-down switchbacks, and no other corridors.
     *
     * It's rather easy to see all the rooms joined in order from left to right
     * across the level if you know what you're looking for. */
    for (a = 0; a < nroom - 1; a++) {
        join(a, a + 1, FALSE);
        if (!rn2(50))
            break; /* allow some randomness */
    }

    /* Connect each room to the room two rooms after it in rooms, if and only
     * if they do not have the same smeq already. */
    for (a = 0; a < nroom - 2; a++)
        if (smeq[a] != smeq[a + 2])
            join(a, a + 2, FALSE);

    /* Connect any remaining rooms with different smeqs.
     * The "any" variable is an optimization; if on a given loop no different
     * smeqs were found from the current room, there's nothing more to be done.
     * */
    for (a = 0; any && a < nroom; a++) {
        any = FALSE;
        for (b = 0; b < nroom; b++)
            if (smeq[a] != smeq[b]) {
                join(a, b, FALSE);
                any = TRUE;
            }
    }
    /* By now, all rooms should be guaranteed to be connected. */

    /* Attempt to draw a few more corridors between rooms, but don't draw the
     * corridor if it starts on an already carved out corridor space. Possibly
     * also don't create the doors.*/
    if (nroom > 2)
        for (i = rn2(nroom) + 4; i; i--) {
            a = rn2(nroom);
            b = rn2(nroom - 2);
            if (b >= a)
                b += 2;
            join(a, b, TRUE);
        }
}

/* Adds a door, not to the level itself, but to the doors array, and updates
 * other mkroom structs as necessary.
 * x and y are the coordinates of the door, and aroom is the room which is
 * getting the door. */
void
add_door(x, y, aroom)
register int x, y;
register struct mkroom *aroom;
{
    register struct mkroom *broom;
    register int tmp;
    int i;

    /* If this room doesn't have any doors yet, it becomes the last room on the
     * doors array. */
    if (aroom->doorct == 0)
        aroom->fdoor = doorindex;

    aroom->doorct++;

    /* If this room did already have doors, move all the other doors up in
     * position by 1. */
    for (tmp = doorindex; tmp > aroom->fdoor; tmp--)
        doors[tmp] = doors[tmp - 1];

    /* If this room was not the last room on the doors array, increment fdoor
     * for any rooms after it (because aroom's will be eating up another index)
     */
    for (i = 0; i < nroom; i++) {
        broom = &rooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }
    /* ditto for subrooms */
    for (i = 0; i < nsubroom; i++) {
        broom = &subrooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }

    /* finally, increment doorindex because we have one more door now, and
     * aroom's first door becomes this one. */
    doorindex++;
    doors[aroom->fdoor].x = x;
    doors[aroom->fdoor].y = y;
}

/* Create a door or a secret door (using type) in aroom at location (x,y).
 * Sets the doormask randomly. Contains the guts of the random probabilities
 * that determine what doorstate the door gets, and whether it becomes trapped.
 *
 * Doors are never generated broken. Secret doors always generate closed or
 * locked.
 *
 * Shop doors are a strange case: if this function is aware that it's adding a
 * secret door to a shop, it will take pains to guarantee that the door will
 * never generate trapped.
 * However, at the time that we're joining up rooms and calling this function,
 * we haven't yet established whether any of them are special rooms or shops.
 * (You can't tell whether it's going to be a shop unless you can guarantee it
 * has one door, and at room-joining time we can't guarantee that.)
 * It turns out that what _actually_ happens with shops, after verifying they
 * have one doorway after everything is joined up, is that they will fix their
 * door themselves in stock_room().
 * The shdoor code in this function will only actually run if we had already
 * generated a shop and then decided to create another door on its wall. It's
 * unlikely that this would ever happen.
 */
STATIC_OVL void
dosdoor(x, y, aroom, type)
register xchar x, y;
struct mkroom *aroom;
int type;
{
    boolean shdoor = *in_rooms(x, y, SHOPBASE) ? TRUE : FALSE;

    if (!IS_WALL(levl[x][y].typ)) /* avoid SDOORs on already made doors */
        type = DOOR;
    levl[x][y].typ = type;
    if (type == DOOR) {
        if (!rn2(3)) { /* is it a locked door, closed, or a doorway? */
            if (!rn2(5))
                levl[x][y].doormask = D_ISOPEN;
            else if (!rn2(6))
                levl[x][y].doormask = D_LOCKED;
            else
                levl[x][y].doormask = D_CLOSED;

            if (levl[x][y].doormask != D_ISOPEN && !shdoor
                && level_difficulty() >= 5 && !rn2(25))
                levl[x][y].doormask |= D_TRAPPED;
        } else {
            /* FIXME: A simple ternary statement probably doesn't warrant its
             * own #ifdef... */
#ifdef STUPID
            if (shdoor)
                levl[x][y].doormask = D_ISOPEN;
            else
                levl[x][y].doormask = D_NODOOR;
#else
            levl[x][y].doormask = (shdoor ? D_ISOPEN : D_NODOOR);
#endif
        }

        /* also done in roguecorr(); doing it here first prevents
           making mimics in place of trapped doors on rogue level */
        if (Is_rogue_level(&u.uz))
            levl[x][y].doormask = D_NODOOR;

        if (levl[x][y].doormask & D_TRAPPED) {
            struct monst *mtmp;

            if (level_difficulty() >= 9 && !rn2(5)
                && !((mvitals[PM_SMALL_MIMIC].mvflags & G_GONE)
                     && (mvitals[PM_LARGE_MIMIC].mvflags & G_GONE)
                     && (mvitals[PM_GIANT_MIMIC].mvflags & G_GONE))) {
                /* make a mimic instead */
                levl[x][y].doormask = D_NODOOR;
                mtmp = makemon(mkclass(S_MIMIC, 0), x, y, NO_MM_FLAGS);
                if (mtmp)
                    set_mimic_sym(mtmp);
            }
        }
        /* newsym(x,y); */
    } else { /* SDOOR */
        if (shdoor || !rn2(5))
            levl[x][y].doormask = D_LOCKED;
        else
            levl[x][y].doormask = D_CLOSED;

        if (!shdoor && level_difficulty() >= 4 && !rn2(20))
            levl[x][y].doormask |= D_TRAPPED;
    }

    add_door(x, y, aroom);
}

/* Determine whether a niche (closet) can be placed on one edge of a room.
 * Contrary to the name, this does not actually place a niche; perhaps it
 * should be renamed to something more straightforward.
 * If the niche can be placed, xx and yy will then contain the coordinate
 * for the door, and dy will contain the direction it's supposed to go in (that
 * is, the actual niche square is (xx, yy+dy)).
 */
STATIC_OVL boolean
place_niche(aroom, dy, xx, yy)
register struct mkroom *aroom;
int *dy, *xx, *yy;
{
    coord dd;

    /* Niches only ever generate on the top and bottom walls of rooms, for some
     * reason. Probably because it looks better.
     * Horizontal "niches" might still appear from time to time as a result of
     * dig_corridor shenanigans, but they're failed corridors, not real niches.
     * Look for a suitable spot on one of these walls to place a niche. */
    if (rn2(2)) {
        *dy = 1;
        finddpos(&dd, aroom->lx, aroom->hy + 1, aroom->hx, aroom->hy + 1);
    } else {
        *dy = -1;
        finddpos(&dd, aroom->lx, aroom->ly - 1, aroom->hx, aroom->ly - 1);
    }
    *xx = dd.x;
    *yy = dd.y;
    /* Spot for the niche must be stone; other spot just inside the room must
     * not be water or another dungeon feature.
     * Note that there's no checking that the area surrounding the niche is
     * also stone; niches can generate touching one or more corridor spaces. */
    return (boolean) ((isok(*xx, *yy + *dy)
                       && levl[*xx][*yy + *dy].typ == STONE)
                      && (isok(*xx, *yy - *dy)
                          && !IS_POOL(levl[*xx][*yy - *dy].typ)
                          && !IS_FURNITURE(levl[*xx][*yy - *dy].typ)));
}

/* there should be one of these per trap, in the same order as trap.h */
static NEARDATA const char *trap_engravings[TRAPNUM] = {
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0,
    /* 14..16: trap door, teleport, level-teleport */
    "Vlad was here", "ad aerarium", "ad aerarium", (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
};

/* Actually create a niche/closet, on a random room. Place a trap on it if
 * trap_type != NO_TRAP.
 */
STATIC_OVL void
makeniche(trap_type)
int trap_type;
{
    register struct mkroom *aroom;
    struct rm *rm;
    int vct = 8; /* number of attempts */
    int dy, xx, yy;
    struct trap *ttmp;

    if (doorindex < DOORMAX) {
        while (vct--) {
            aroom = &rooms[rn2(nroom)];
            if (aroom->rtype != OROOM)
                /* don't place niches in special rooms */
                continue;
            if (aroom->doorct == 1 && rn2(5))
                /* usually don't place in rooms with 1 door */
                continue;
            if (!place_niche(aroom, &dy, &xx, &yy))
                /* didn't find a suitable spot */
                continue;

            rm = &levl[xx][yy + dy];
            if (trap_type || !rn2(4)) {
                /* all closets with traps and 25% of other closets require some
                 * searching */
                rm->typ = SCORR;
                if (trap_type) {
                    /* don't place fallthru traps on undiggable levels */
                    if ((trap_type == HOLE || trap_type == TRAPDOOR)
                        && !Can_fall_thru(&u.uz))
                        trap_type = ROCKTRAP;
                    ttmp = maketrap(xx, yy + dy, trap_type);
                    if (ttmp) {
                        if (trap_type != ROCKTRAP)
                            ttmp->once = 1;
                        /* make the specified engraving in front of the door */
                        if (trap_engravings[trap_type]) {
                            make_engr_at(xx, yy - dy,
                                         trap_engravings[trap_type], 0L,
                                         DUST);
                            wipe_engr_at(xx, yy - dy, 5,
                                         FALSE); /* age it a little */
                        }
                    }
                }
                /* place the door */
                dosdoor(xx, yy, aroom, SDOOR);
            } else {
                rm->typ = CORR;
                /* 1/7 of these niches are generated inaccessible - no actual
                 * connection to their corresponding room */
                if (rn2(7))
                    dosdoor(xx, yy, aroom, rn2(5) ? SDOOR : DOOR);
                else {
                    /* inaccessible niches occasionally have iron bars */
                    if (!rn2(5) && IS_WALL(levl[xx][yy].typ)) {
                        levl[xx][yy].typ = IRONBARS;
                        if (rn2(3))
                            (void) mkcorpstat(CORPSE, (struct monst *) 0,
                                              mkclass(S_HUMAN, 0), xx,
                                              yy + dy, TRUE);
                    }
                    /* Place a teleport scroll here so the player can escape.
                     * If an inaccessible niche is generated on a no-tele
                     * level, the player shouldn't be able to get into it
                     * without some way of getting back out... */
                    if (!level.flags.noteleport)
                        (void) mksobj_at(SCR_TELEPORTATION, xx, yy + dy, TRUE,
                                         FALSE);
                    if (!rn2(3))
                        (void) mkobj_at(0, xx, yy + dy, TRUE);
                }
            }
            return;
        }
    }
}

/* Try to create several random niches across an entire level.
 * Does NOT include the niche for a vault teleporter, if one exists. */
STATIC_OVL void
make_niches()
{
    /* This should really be nroom / 2... */
    int ct = rnd((nroom >> 1) + 1), dep = depth(&u.uz);
    boolean ltptr = (!level.flags.noteleport && dep > 15),
            vamp = (dep > 5 && dep < 25);

    while (ct--) {
        if (ltptr && !rn2(6)) {
            /* occasional fake vault teleporter */
            ltptr = FALSE;
            makeniche(LEVEL_TELEP);
        } else if (vamp && !rn2(6)) {
            /* "Vlad was here" trapdoor */
            vamp = FALSE;
            makeniche(TRAPDOOR);
        } else
            /* regular untrapped niche */
            makeniche(NO_TRAP);
    }
}

/* Create a vault teleporter niche.
 * The code seems to assume that any teleport trap inside a niche should always
 * go to a vault; this may become problematic if the player ever gains the
 * ability to make teleport traps...
 */
STATIC_OVL void
makevtele()
{
    makeniche(TELEP_TRAP);
}

/* clear out various globals that keep information on the current level.
 * some of this is only necessary for some types of levels (maze, normal,
 * special) but it's easier to put it all in one place than make sure
 * each type initializes what it needs to separately.
 */
STATIC_OVL void
clear_level_structures()
{
    static struct rm zerorm = { cmap_to_glyph(S_stone),
                                0, 0, 0, 0, 0, 0, 0, 0, 0 };
    register int x, y;
    register struct rm *lev;

    for (x = 0; x < COLNO; x++) {
        lev = &levl[x][0];
        for (y = 0; y < ROWNO; y++) {
            *lev++ = zerorm;
            /*
             * These used to be '#if MICROPORT_BUG',
             * with use of memset(0) for '#if !MICROPORT_BUG' below,
             * but memset is not appropriate for initializing pointers,
             * so do these level.objects[][] and level.monsters[][]
             * initializations unconditionally.
             */
            level.objects[x][y] = (struct obj *) 0;
            level.monsters[x][y] = (struct monst *) 0;
        }
    }
    level.objlist = (struct obj *) 0;
    level.buriedobjlist = (struct obj *) 0;
    level.monlist = (struct monst *) 0;
    level.damagelist = (struct damage *) 0;
    level.bonesinfo = (struct cemetery *) 0;

    level.flags.nfountains = 0;
    level.flags.nsinks = 0;
    level.flags.has_shop = 0;
    level.flags.has_vault = 0;
    level.flags.has_zoo = 0;
    level.flags.has_court = 0;
    level.flags.has_morgue = level.flags.graveyard = 0;
    level.flags.has_beehive = 0;
    level.flags.has_barracks = 0;
    level.flags.has_temple = 0;
    level.flags.has_swamp = 0;
    level.flags.noteleport = 0;
    level.flags.hardfloor = 0;
    level.flags.nommap = 0;
    level.flags.hero_memory = 1;
    level.flags.shortsighted = 0;
    level.flags.sokoban_rules = 0;
    level.flags.is_maze_lev = 0;
    level.flags.is_cavernous_lev = 0;
    level.flags.arboreal = 0;
    level.flags.wizard_bones = 0;
    level.flags.corrmaze = 0;

    nroom = 0;
    rooms[0].hx = -1;
    nsubroom = 0;
    subrooms[0].hx = -1;
    doorindex = 0;
    init_rect();
    init_vault();
    xdnstair = ydnstair = xupstair = yupstair = 0;
    sstairs.sx = sstairs.sy = 0;
    xdnladder = ydnladder = xupladder = yupladder = 0;
    made_branch = FALSE;
    clear_regions();
}

/* Full initialization of all level structures, map, objects, etc.
 * Handles any level - special levels will load that special level, Gehennom
 * will create mazes, and so on.
 * Called only from mklev(). */
STATIC_OVL void
makelevel()
{
    register struct mkroom *croom, *troom;
    register int tryct;
    register int x, y;
    struct monst *tmonst; /* always put a web with a spider */
    branch *branchp;
    int room_threshold;

    /* this is apparently used to denote that a lot of program state is
     * uninitialized */
    if (wiz1_level.dlevel == 0)
        init_dungeons();
    oinit(); /* assign level dependent obj probabilities */
    clear_level_structures(); /* full level reset */

    /* FIXME: pointless braces? */
    {
        register s_level *slev = Is_special(&u.uz);

        /* check for special levels */
        if (slev && !Is_rogue_level(&u.uz)) {
            /* special non-Rogue level */
            makemaz(slev->proto);
            return;
        } else if (dungeons[u.uz.dnum].proto[0]) {
            /* named prototype file */
            makemaz("");
            return;
        } else if (In_mines(&u.uz)) {
            /* mines filler */
            makemaz("minefill");
            return;
        } else if (In_quest(&u.uz)) {
            /* quest filler */
            char fillname[9];
            s_level *loc_lev;

            Sprintf(fillname, "%s-loca", urole.filecode);
            loc_lev = find_level(fillname);

            Sprintf(fillname, "%s-fil", urole.filecode);
            Strcat(fillname,
                   (u.uz.dlevel < loc_lev->dlevel.dlevel) ? "a" : "b");
            makemaz(fillname);
            return;
        } else if (In_hell(&u.uz)
                   || (rn2(5) && u.uz.dnum == medusa_level.dnum
                       && depth(&u.uz) > depth(&medusa_level))) {
            /* Gehennom, or 80% of levels below Medusa - maze filler */
            makemaz("");
            return;
        }
    }

    /* otherwise, fall through - it's a "regular" level. */
    if (Is_rogue_level(&u.uz)) {
        /* place rooms and fake bones pile */
        makeroguerooms();
        makerogueghost();
    } else {
        /* regular dungeon fill level */
        makerooms();
    }

    /* order rooms[] by x-coordinate */
    sort_rooms();

    /* construct stairs (up and down in different rooms if possible) */
    croom = &rooms[rn2(nroom)];
    if (!Is_botlevel(&u.uz))
        mkstairs(somex(croom), somey(croom), 0, croom); /* down */
    if (nroom > 1) {
        troom = croom;
        croom = &rooms[rn2(nroom - 1)];
        /* slight bias here for upstairs to be 1 room to the right of the
         * downstairs room */
        if (croom == troom)
            croom++;
    }

    /* now do the upstairs */
    if (u.uz.dlevel != 1) {
        xchar sx, sy;
        do {
            sx = somex(croom);
            sy = somey(croom);
        } while (occupied(sx, sy));
        mkstairs(sx, sy, 1, croom); /* up */
    }

    branchp = Is_branchlev(&u.uz);    /* possible dungeon branch */
    room_threshold = branchp ? 4 : 3; /* minimum number of rooms needed
                                         to allow a random special room */
    if (Is_rogue_level(&u.uz))
        goto skip0;
    makecorridors();
    make_niches();

    /* Did makerooms place a 2x2 unconnected room to be a vault? If so, fill
     * it.
     * Is there really a reason for do_vault() to be a macro? All it does is
     * test whether vault_x is a real coordinate. It's only used here. */
    if (do_vault()) {
        xchar w, h;
        debugpline0("trying to make a vault...");
        w = 1;
        h = 1;
        /* make sure vault can actually be placed */
        if (check_room(&vault_x, &w, &vault_y, &h, TRUE)) {
        fill_vault:
            add_room(vault_x, vault_y, vault_x + w, vault_y + h, TRUE, VAULT,
                     FALSE);
            level.flags.has_vault = 1;
            ++room_threshold;
            fill_room(&rooms[nroom - 1], FALSE);
            mk_knox_portal(vault_x + w, vault_y + h);
            /* Only put a vault teleporter with 1/3 chance;
             * a teleportation trap in a closet is a sure sign that a vault is
             * on the level, but a vault is not a sure sign of a vault
             * teleporter. */
            if (!level.flags.noteleport && !rn2(3))
                makevtele();
        } else if (rnd_rect() && create_vault()) {
            /* If we didn't create a vault already, try once more. */
            vault_x = rooms[nroom].lx;
            vault_y = rooms[nroom].ly;
            if (check_room(&vault_x, &w, &vault_y, &h, TRUE))
                goto fill_vault;
            else
                rooms[nroom].hx = -1;
        }
    }

    {
        /* Try to create one special room on the level.
         * The available special rooms depend on how deep you are.
         * If a special room is selected and fails to be created (e.g. it tried
         * to make a shop and failed because no room had exactly 1 door), it
         * won't try to create the other types of available special rooms. */
        register int u_depth = depth(&u.uz);

        if (wizard && nh_getenv("SHOPTYPE"))
            /* special case that overrides everything else for wizard mode */
            mkroom(SHOPBASE);
        else if (u_depth > 1 && u_depth < depth(&medusa_level)
                 && nroom >= room_threshold && rn2(u_depth) < 3)
            mkroom(SHOPBASE);
        else if (u_depth > 4 && !rn2(6))
            mkroom(COURT);
        else if (u_depth > 5 && !rn2(8)
                 && !(mvitals[PM_LEPRECHAUN].mvflags & G_GONE))
            mkroom(LEPREHALL);
        else if (u_depth > 6 && !rn2(7))
            mkroom(ZOO);
        else if (u_depth > 8 && !rn2(5))
            mkroom(TEMPLE);
        else if (u_depth > 9 && !rn2(5)
                 && !(mvitals[PM_KILLER_BEE].mvflags & G_GONE))
            mkroom(BEEHIVE);
        else if (u_depth > 11 && !rn2(6))
            mkroom(MORGUE);
        else if (u_depth > 12 && !rn2(8) && antholemon())
            mkroom(ANTHOLE);
        else if (u_depth > 14 && !rn2(4)
                 && !(mvitals[PM_SOLDIER].mvflags & G_GONE))
            mkroom(BARRACKS);
        else if (u_depth > 15 && !rn2(6))
            mkroom(SWAMP);
        else if (u_depth > 16 && !rn2(8)
                 && !(mvitals[PM_COCKATRICE].mvflags & G_GONE))
            mkroom(COCKNEST);
    }

skip0:
    /* Place multi-dungeon branch. */
    place_branch(branchp, 0, 0);

    /* for each room: put things inside */
    for (croom = rooms; croom->hx > 0; croom++) {
        if (croom->rtype != OROOM)
            continue;

        /* put a sleeping monster inside */
        /* Note: monster may be on the stairs. This cannot be
           avoided: maybe the player fell through a trap door
           while a monster was on the stairs. Conclusion:
           we have to check for monsters on the stairs anyway. */

        if (u.uhave.amulet || !rn2(3)) {
            x = somex(croom);
            y = somey(croom);
            tmonst = makemon((struct permonst *) 0, x, y, NO_MM_FLAGS);
            if (tmonst && tmonst->data == &mons[PM_GIANT_SPIDER]
                && !occupied(x, y))
                (void) maketrap(x, y, WEB);
        }
        /* put traps and mimics inside */
        x = 8 - (level_difficulty() / 6);
        if (x <= 1)
            /* maxes out at level_difficulty() == 36 */
            x = 2;
        while (!rn2(x))
            mktrap(0, 0, croom, (coord *) 0);

        /* maybe put some gold inside */
        if (!rn2(3))
            (void) mkgold(0L, somex(croom), somey(croom));
        if (Is_rogue_level(&u.uz))
            goto skip_nonrogue;

        /* maybe place some dungeon features inside */
        if (!rn2(10))
            mkfount(0, croom);
        if (!rn2(60))
            mksink(croom);
        if (!rn2(60))
            mkaltar(croom);
        x = 80 - (depth(&u.uz) * 2);
        if (x < 2)
            x = 2;
        if (!rn2(x))
            mkgrave(croom);

        /* put statues inside */
        if (!rn2(20))
            (void) mkcorpstat(STATUE, (struct monst *) 0,
                              (struct permonst *) 0, somex(croom),
                              somey(croom), CORPSTAT_INIT);
        /* put box/chest inside;
         *  40% chance for at least 1 box, regardless of number
         *  of rooms; about 5 - 7.5% for 2 boxes, least likely
         *  when few rooms; chance for 3 or more is negligible.
         */
        if (!rn2(nroom * 5 / 2))
            (void) mksobj_at((rn2(3)) ? LARGE_BOX : CHEST, somex(croom),
                             somey(croom), TRUE, FALSE);

        /* maybe make some graffiti
         * chance decreases the lower you get in the dungeon */
        if (!rn2(27 + 3 * abs(depth(&u.uz)))) {
            char buf[BUFSZ];
            const char *mesg = random_engraving(buf);
            if (mesg) {
                do {
                    x = somex(croom);
                    y = somey(croom);
                } while (levl[x][y].typ != ROOM && !rn2(40));
                if (!(IS_POOL(levl[x][y].typ)
                      || IS_FURNITURE(levl[x][y].typ)))
                    make_engr_at(x, y, mesg, 0L, MARK);
            }
        }

    skip_nonrogue:
        /* place a random object in the room, with a recursive 20% chance of
         * placing another */
        if (!rn2(3)) {
            (void) mkobj_at(0, somex(croom), somey(croom), TRUE);
            tryct = 0;
            while (!rn2(5)) {
                if (++tryct > 100) {
                    impossible("tryct overflow4");
                    break;
                }
                (void) mkobj_at(0, somex(croom), somey(croom), TRUE);
            }
        }
    }
}

/*
 * Place deposits of minerals (gold and misc gems) in the stone
 * surrounding the rooms on the map.
 * Also place kelp in water.
 * mineralize(-1, -1, -1, -1, FALSE); => "default" behaviour
 * The four probability arguments aren't percentages; assuming the spot to
 * place the item is suitable, kelp will be placed with 1/prob chance;
 * whereas gold and gems will be placed with prob/1000 chance.
 * skip_lvl_checks will ignore any checks that items don't get mineralized in
 * the wrong levels. This is currently only TRUE if a special level forces it
 * to be.
 */
void
mineralize(kelp_pool, kelp_moat, goldprob, gemprob, skip_lvl_checks)
int kelp_pool, kelp_moat, goldprob, gemprob;
boolean skip_lvl_checks;
{
    s_level *sp;
    struct obj *otmp;
    int x, y, cnt;

    if (kelp_pool < 0)
        kelp_pool = 10;
    if (kelp_moat < 0)
        kelp_moat = 30;

    /* Place kelp, except on the plane of water */
    if (!skip_lvl_checks && In_endgame(&u.uz))
        return;
    for (x = 2; x < (COLNO - 2); x++)
        for (y = 1; y < (ROWNO - 1); y++)
            if ((kelp_pool && levl[x][y].typ == POOL && !rn2(kelp_pool))
                || (kelp_moat && levl[x][y].typ == MOAT && !rn2(kelp_moat)))
                (void) mksobj_at(KELP_FROND, x, y, TRUE, FALSE);

    /* determine if it is even allowed;
       almost all special levels are excluded */
    if (!skip_lvl_checks
        && (In_hell(&u.uz) || In_V_tower(&u.uz) || Is_rogue_level(&u.uz)
            || level.flags.arboreal
            || ((sp = Is_special(&u.uz)) != 0 && !Is_oracle_level(&u.uz)
                && (!In_mines(&u.uz) || sp->flags.town))))
        return;

    /* basic level-related probabilities */
    if (goldprob < 0)
        goldprob = 20 + depth(&u.uz) / 3;
    if (gemprob < 0)
        gemprob = goldprob / 4;

    /* mines have ***MORE*** goodies - otherwise why mine? */
    if (!skip_lvl_checks) {
        if (In_mines(&u.uz)) {
            goldprob *= 2;
            gemprob *= 3;
        } else if (In_quest(&u.uz)) {
            goldprob /= 4;
            gemprob /= 6;
        }
    }

    /*
     * Seed rock areas with gold and/or gems.
     * We use fairly low level object handling to avoid unnecessary
     * overhead from placing things in the floor chain prior to burial.
     */
    for (x = 2; x < (COLNO - 2); x++)
        for (y = 1; y < (ROWNO - 1); y++)
            if (levl[x][y + 1].typ != STONE) { /* <x,y> spot not eligible */
                y += 2; /* next two spots aren't eligible either */
            } else if (levl[x][y].typ != STONE) { /* this spot not eligible */
                y += 1; /* next spot isn't eligible either */
            } else if (!(levl[x][y].wall_info & W_NONDIGGABLE)
                       && levl[x][y - 1].typ == STONE
                       && levl[x + 1][y - 1].typ == STONE
                       && levl[x - 1][y - 1].typ == STONE
                       && levl[x + 1][y].typ == STONE
                       && levl[x - 1][y].typ == STONE
                       && levl[x + 1][y + 1].typ == STONE
                       && levl[x - 1][y + 1].typ == STONE) {
                if (rn2(1000) < goldprob) {
                    if ((otmp = mksobj(GOLD_PIECE, FALSE, FALSE)) != 0) {
                        otmp->ox = x, otmp->oy = y;
                        otmp->quan = 1L + rnd(goldprob * 3);
                        otmp->owt = weight(otmp);
                        if (!rn2(3))
                            add_to_buried(otmp);
                        else
                            place_object(otmp, x, y);
                    }
                }
                if (rn2(1000) < gemprob) {
                    for (cnt = rnd(2 + dunlev(&u.uz) / 3); cnt > 0; cnt--)
                        if ((otmp = mkobj(GEM_CLASS, FALSE)) != 0) {
                            if (otmp->otyp == ROCK) {
                                dealloc_obj(otmp); /* discard it */
                            } else {
                                otmp->ox = x, otmp->oy = y;
                                if (!rn2(3))
                                    add_to_buried(otmp);
                                else
                                    place_object(otmp, x, y);
                            }
                        }
                }
            }
}

/* Topmost level creation routine.
 * Mainly just wraps around makelevel(), but also handles loading bones files,
 * mineralizing after the level is created, blocking digging, setting roomnos
 * via topologize, and a couple other things.
 * Called from a few places: newgame() (to generate level 1), goto_level (any
 * other levels), and wiz_makemap (wizard mode regenerating the level).
 */
void
mklev()
{
    struct mkroom *croom;
    int ridx;

    init_mapseen(&u.uz);
    if (getbones())
        return;

    in_mklev = TRUE;
    makelevel();
    bound_digging();
    mineralize(-1, -1, -1, -1, FALSE);
    in_mklev = FALSE;
    /* has_morgue gets cleared once morgue is entered; graveyard stays
       set (graveyard might already be set even when has_morgue is clear
       [see fixup_special()], so don't update it unconditionally) */
    if (level.flags.has_morgue)
        level.flags.graveyard = 1;
    if (!level.flags.is_maze_lev) {
        for (croom = &rooms[0]; croom != &rooms[nroom]; croom++)
#ifdef SPECIALIZATION
            topologize(croom, FALSE);
#else
            topologize(croom);
#endif
    }
    set_wall_state();
    /* for many room types, rooms[].rtype is zeroed once the room has been
       entered; rooms[].orig_rtype always retains original rtype value */
    for (ridx = 0; ridx < SIZE(rooms); ridx++)
        rooms[ridx].orig_rtype = rooms[ridx].rtype;
}

/* Set the roomno correctly for all squares of the given room.
 * Mostly this sets them to the roomno from croom, but if there are any walls
 * that already have a roomno defined, it changes them to SHARED.
 * Then it recurses on subrooms.
 *
 * If SPECIALIZATION is defined and croom->rtype = OROOM, it will set the
 * roomno to NO_ROOM, but only if do_ordinary is TRUE.
 */
void
#ifdef SPECIALIZATION
topologize(croom, do_ordinary)
struct mkroom *croom;
boolean do_ordinary;
#else
topologize(croom)
struct mkroom *croom;
#endif
{
    register int x, y, roomno = (int) ((croom - rooms) + ROOMOFFSET);
    int lowx = croom->lx, lowy = croom->ly;
    int hix = croom->hx, hiy = croom->hy;
#ifdef SPECIALIZATION
    schar rtype = croom->rtype;
#endif
    int subindex, nsubrooms = croom->nsubrooms;

    /* skip the room if already done; i.e. a shop handled out of order */
    /* also skip if this is non-rectangular (it _must_ be done already) */
    if ((int) levl[lowx][lowy].roomno == roomno || croom->irregular)
        return;
#ifdef SPECIALIZATION
    if (Is_rogue_level(&u.uz))
        do_ordinary = TRUE; /* vision routine helper */
    if ((rtype != OROOM) || do_ordinary)
#endif
        {
        /* do innards first */
        for (x = lowx; x <= hix; x++)
            for (y = lowy; y <= hiy; y++)
#ifdef SPECIALIZATION
                if (rtype == OROOM)
                    levl[x][y].roomno = NO_ROOM;
                else
#endif
                    levl[x][y].roomno = roomno;
        /* top and bottom edges */
        for (x = lowx - 1; x <= hix + 1; x++)
            for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
                levl[x][y].edge = 1;
                if (levl[x][y].roomno)
                    levl[x][y].roomno = SHARED;
                else
                    levl[x][y].roomno = roomno;
            }
        /* sides */
        for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
            for (y = lowy; y <= hiy; y++) {
                levl[x][y].edge = 1;
                if (levl[x][y].roomno)
                    levl[x][y].roomno = SHARED;
                else
                    levl[x][y].roomno = roomno;
            }
    }
    /* subrooms */
    for (subindex = 0; subindex < nsubrooms; subindex++)
#ifdef SPECIALIZATION
        topologize(croom->sbrooms[subindex], (boolean) (rtype != OROOM));
#else
        topologize(croom->sbrooms[subindex]);
#endif
}

/* Find an unused room for a branch location. */
STATIC_OVL struct mkroom *
find_branch_room(mp)
coord *mp;
{
    struct mkroom *croom = 0;

    if (nroom == 0) {
        mazexy(mp); /* already verifies location */
    } else {
        /* not perfect - there may be only one stairway */
        if (nroom > 2) {
            int tryct = 0;

            do
                croom = &rooms[rn2(nroom)];
            while ((croom == dnstairs_room || croom == upstairs_room
                    || croom->rtype != OROOM) && (++tryct < 100));
        } else
            croom = &rooms[rn2(nroom)];

        do {
            if (!somexy(croom, mp))
                impossible("Can't place branch!");
        } while (occupied(mp->x, mp->y)
                 || (levl[mp->x][mp->y].typ != CORR
                     && levl[mp->x][mp->y].typ != ROOM));
    }
    return croom;
}

/* Find the room for (x,y).  Return null if not in a room. */
STATIC_OVL struct mkroom *
pos_to_room(x, y)
xchar x, y;
{
    int i;
    struct mkroom *curr;

    for (curr = rooms, i = 0; i < nroom; curr++, i++)
        if (inside_room(curr, x, y))
            return curr;
    ;
    return (struct mkroom *) 0;
}

/* Place a branch staircase or ladder for branch br at the coordinates (x,y).
 * If x is zero, pick the branch room and coordinates within it randomly.
 * If br is null, or the global made_branch is TRUE, do nothing. */
void
place_branch(br, x, y)
branch *br; /* branch to place */
xchar x, y; /* location */
{
    coord m;
    d_level *dest;
    boolean make_stairs;
    struct mkroom *br_room;

    /*
     * Return immediately if there is no branch to make or we have
     * already made one.  This routine can be called twice when
     * a special level is loaded that specifies an SSTAIR location
     * as a favored spot for a branch.
     */
    if (!br || made_branch)
        return;

    if (!x) { /* find random coordinates for branch */
        br_room = find_branch_room(&m);
        x = m.x;
        y = m.y;
    } else {
        br_room = pos_to_room(x, y);
    }

    if (on_level(&br->end1, &u.uz)) {
        /* we're on end1 */
        make_stairs = br->type != BR_NO_END1;
        dest = &br->end2;
    } else {
        /* we're on end2 */
        make_stairs = br->type != BR_NO_END2;
        dest = &br->end1;
    }

    if (br->type == BR_PORTAL) {
        mkportal(x, y, dest->dnum, dest->dlevel);
    } else if (make_stairs) {
        sstairs.sx = x;
        sstairs.sy = y;
        sstairs.up =
            (char) on_level(&br->end1, &u.uz) ? br->end1_up : !br->end1_up;
        assign_level(&sstairs.tolev, dest);
        sstairs_room = br_room;

        levl[x][y].ladder = sstairs.up ? LA_UP : LA_DOWN;
        levl[x][y].typ = STAIRS;
    }
    /*
     * Set made_branch to TRUE even if we didn't make a stairwell (i.e.
     * make_stairs is false) since there is currently only one branch
     * per level, if we failed once, we're going to fail again on the
     * next call.
     */
    made_branch = TRUE;
}

/* Return TRUE if the given location is directly adjacent to a door or secret
 * door in any direction. */
STATIC_OVL boolean
bydoor(x, y)
register xchar x, y;
{
    register int typ;

    if (isok(x + 1, y)) {
        typ = levl[x + 1][y].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x - 1, y)) {
        typ = levl[x - 1][y].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x, y + 1)) {
        typ = levl[x][y + 1].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x, y - 1)) {
        typ = levl[x][y - 1].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    return FALSE;
}

/* Return TRUE if it is allowable to create a door at (x,y).
 * The given coordinate must be a wall and not be adjacent to a door, and we
 * can't be at the max number of doors.
 * FIXME: This should return boolean. */
int
okdoor(x, y)
xchar x, y;
{
    boolean near_door = bydoor(x, y);

    return ((levl[x][y].typ == HWALL || levl[x][y].typ == VWALL)
            && doorindex < DOORMAX && !near_door);
}

/* Wrapper for dosdoor. Create a door randomly at location (x,y) in aroom.
 * For some reason, the logic of whether or not to make the door secret is
 * here, while all the other logic of determining the door state is in dosdoor.
 */
void
dodoor(x, y, aroom)
int x, y;
struct mkroom *aroom;
{
    if (doorindex >= DOORMAX) {
        impossible("DOORMAX exceeded?");
        return;
    }

    dosdoor(x, y, aroom, rn2(8) ? DOOR : SDOOR);
}

/* Return TRUE if the given location contains a trap, dungeon furniture, liquid
 * terrain, or the vibrating square.
 * Generally used for determining if a space is unsuitable for placing
 * something.
 */
boolean
occupied(x, y)
register xchar x, y;
{
    return (boolean) (t_at(x, y) || IS_FURNITURE(levl[x][y].typ)
                      || is_lava(x, y) || is_pool(x, y)
                      || invocation_pos(x, y));
}

/* Create a trap.
 * If num is a valid trap index, create that specific trap.
 * If tm is non-NULL, create the trap at tm's coordinates. Otherwise, if
 * mazeflag is TRUE, choose a random maze position; if FALSE, assume that croom
 * is non-NULL and pick a random location inside croom.
 *
 * If num is invalid as a trap index, it will create a random trap. In
 * Gehennom, there is a 20% chance it will just pick fire trap. If various
 * factors mean that the trap is unsuitable (usually because of difficulty), it
 * will keep trying until it picks something valid.
 *
 * If a fallthru trap is created on a undiggable-floor level, it defaults to
 * ROCKTRAP. If a WEB is created, a giant spider is created on top of it.
 * Finally, if it is very early in the dungeon, and the trap is potentially
 *
 * lethal, create a minimal fake bones pile on the trap.
 */
void
mktrap(num, mazeflag, croom, tm)
int num, mazeflag;
struct mkroom *croom;
coord *tm;
{
    register int kind;
    unsigned lvl = level_difficulty();
    coord m;

    /* no traps in pools */
    if (tm && is_pool(tm->x, tm->y))
        return;

    if (num > 0 && num < TRAPNUM) {
        kind = num;
    } else if (Is_rogue_level(&u.uz)) {
        /* presumably Rogue-specific traps */
        switch (rn2(7)) {
        default:
            kind = BEAR_TRAP;
            break; /* 0 */
        case 1:
            kind = ARROW_TRAP;
            break;
        case 2:
            kind = DART_TRAP;
            break;
        case 3:
            kind = TRAPDOOR;
            break;
        case 4:
            kind = PIT;
            break;
        case 5:
            kind = SLP_GAS_TRAP;
            break;
        case 6:
            kind = RUST_TRAP;
            break;
        }
    } else if (Inhell && !rn2(5)) {
        /* bias the frequency of fire traps in Gehennom */
        kind = FIRE_TRAP;
    } else {
        do {
            kind = rnd(TRAPNUM - 1);
            /* reject "too hard" traps */
            switch (kind) {
            case MAGIC_PORTAL:
            case VIBRATING_SQUARE:
                kind = NO_TRAP;
                break;
            case ROLLING_BOULDER_TRAP:
            case SLP_GAS_TRAP:
                if (lvl < 2)
                    kind = NO_TRAP;
                break;
            case LEVEL_TELEP:
                if (lvl < 5 || level.flags.noteleport)
                    kind = NO_TRAP;
                break;
            case SPIKED_PIT:
                if (lvl < 5)
                    kind = NO_TRAP;
                break;
            case LANDMINE:
                if (lvl < 6)
                    kind = NO_TRAP;
                break;
            case WEB:
                if (lvl < 7)
                    kind = NO_TRAP;
                break;
            case STATUE_TRAP:
            case POLY_TRAP:
                if (lvl < 8)
                    kind = NO_TRAP;
                break;
            case FIRE_TRAP:
                if (!Inhell)
                    kind = NO_TRAP;
                break;
            case TELEP_TRAP:
                if (level.flags.noteleport)
                    kind = NO_TRAP;
                break;
            case HOLE:
                /* make these much less often than other traps */
                if (rn2(7))
                    kind = NO_TRAP;
                break;
            }
        } while (kind == NO_TRAP);
    }

    if ((kind == TRAPDOOR || kind == HOLE) && !Can_fall_thru(&u.uz))
        kind = ROCKTRAP;

    if (tm) {
        m = *tm;
    } else {
        register int tryct = 0;
        boolean avoid_boulder = (kind == PIT || kind == SPIKED_PIT
                                 || kind == TRAPDOOR || kind == HOLE);

        /* Try up to 200 times to find a random coordinate for the trap. */
        do {
            if (++tryct > 200)
                return;
            if (mazeflag)
                mazexy(&m);
            else if (!somexy(croom, &m))
                return;
        } while (occupied(m.x, m.y)
                 || (avoid_boulder && sobj_at(BOULDER, m.x, m.y)));
    }

    (void) maketrap(m.x, m.y, kind);
    if (kind == WEB)
        (void) makemon(&mons[PM_GIANT_SPIDER], m.x, m.y, NO_MM_FLAGS);

    /* The hero isn't the only person who's entered the dungeon in
       search of treasure. On the very shallowest levels, there's a
       chance that a created trap will have killed something already
       (and this is guaranteed on the first level).

       This isn't meant to give any meaningful treasure (in fact, any
       items we drop here are typically cursed, other than ammo fired
       by the trap). Rather, it's mostly just for flavour and to give
       players on very early levels a sufficient chance to avoid traps
       that may end up killing them before they have a fair chance to
       build max HP. Including cursed items gives the same fair chance
       to the starting pet, and fits the rule that possessions of the
       dead are normally cursed.

       Some types of traps are excluded because they're entirely
       nonlethal, even indirectly. We also exclude all of the
       later/fancier traps because they tend to have special
       considerations (e.g. webs, portals), often are indirectly
       lethal, and tend not to generate on shallower levels anyway.
       Finally, pits are excluded because it's weird to see an item
       in a pit and yet not be able to identify that the pit is there. */
    if (lvl <= (unsigned) rnd(4)
        && kind != SQKY_BOARD && kind != RUST_TRAP
        && kind != PIT && kind != SPIKED_PIT && kind < HOLE) {
        /* Object generated by the trap; initially NULL, stays NULL if
           we fail to generate an object or if the trap doesn't
           generate objects. */
        struct obj *otmp = NULL;
        int victim_mnum; /* race of the victim */

        /* Not all trap types have special handling here; only the ones
           that kill in a specific way that's obvious after the fact. */
        switch (kind) {
        case ARROW_TRAP:
            otmp = mksobj(ARROW, TRUE, FALSE);
            otmp->opoisoned = 0;
            /* don't adjust the quantity; maybe the trap shot multiple
               times, there was an untrapping attempt, etc... */
            break;
        case DART_TRAP:
            otmp = mksobj(DART, TRUE, FALSE);
            break;
        case ROCKTRAP:
            otmp = mksobj(ROCK, TRUE, FALSE);
            break;
        default:
            /* no item dropped by the trap */
            break;
        }
        if (otmp) {
            place_object(otmp, m.x, m.y);
        }

        /* now otmp is reused for other items we're placing */

        /* Place a random possession. This could be a weapon, tool,
           food, or gem, i.e. the item classes that are typically
           nonmagical and not worthless. */
        do {
            int poss_class = RANDOM_CLASS; /* init => lint suppression */

            switch (rn2(4)) {
            case 0:
                poss_class = WEAPON_CLASS;
                break;
            case 1:
                poss_class = TOOL_CLASS;
                break;
            case 2:
                poss_class = FOOD_CLASS;
                break;
            case 3:
                poss_class = GEM_CLASS;
                break;
            }

            otmp = mkobj(poss_class, FALSE);
            /* these items are always cursed, both for flavour (owned
               by a dead adventurer, bones-pile-style) and for balance
               (less useful to use, and encourage pets to avoid the trap) */
            if (otmp) {
                otmp->blessed = 0;
                otmp->cursed = 1;
                otmp->owt = weight(otmp);
                place_object(otmp, m.x, m.y);
            }

            /* 20% chance of placing an additional item, recursively */
        } while (!rn2(5));

        /* Place a corpse. */
        switch (rn2(15)) {
        case 0:
            /* elf corpses are the rarest as they're the most useful */
            victim_mnum = PM_ELF;
            /* elven adventurers get sleep resistance early; so don't
               generate elf corpses on sleeping gas traps unless a)
               we're on dlvl 2 (1 is impossible) and b) we pass a coin
               flip */
            if (kind == SLP_GAS_TRAP && !(lvl <= 2 && rn2(2)))
                victim_mnum = PM_HUMAN;
            break;
        case 1: case 2:
            victim_mnum = PM_DWARF;
            break;
        case 3: case 4: case 5:
            victim_mnum = PM_ORC;
            break;
        case 6: case 7: case 8: case 9:
            /* more common as they could have come from the Mines */
            victim_mnum = PM_GNOME;
            /* 10% chance of a candle too */
            if (!rn2(10)) {
                otmp = mksobj(rn2(4) ? TALLOW_CANDLE : WAX_CANDLE,
                              TRUE, FALSE);
                otmp->quan = 1;
                otmp->blessed = 0;
                otmp->cursed = 1;
                otmp->owt = weight(otmp);
                place_object(otmp, m.x, m.y);
            }
            break;
        default:
            /* the most common race */
            victim_mnum = PM_HUMAN;
            break;
        }
        otmp = mkcorpstat(CORPSE, NULL, &mons[victim_mnum], m.x, m.y,
                          CORPSTAT_INIT);
        if (otmp)
            otmp->age -= 51; /* died too long ago to eat */
    }
}

/* Create some (non-branch) stairs at (x,y) (absolute coords) inside croom.
 * up is whether or not it's upstairs.
 */
void
mkstairs(x, y, up, croom)
xchar x, y;
char up; /* should probably be boolean... */
struct mkroom *croom;
{
    if (!x) {
        impossible("mkstairs:  bogus stair attempt at <%d,%d>", x, y);
        return;
    }

    /*
     * We can't make a regular stair off an end of the dungeon.  This
     * attempt can happen when a special level is placed at an end and
     * has an up or down stair specified in its description file.
     */
    if ((dunlev(&u.uz) == 1 && up)
        || (dunlev(&u.uz) == dunlevs_in_dungeon(&u.uz) && !up))
        return;

    if (up) {
        xupstair = x;
        yupstair = y;
        upstairs_room = croom;
    } else {
        xdnstair = x;
        ydnstair = y;
        dnstairs_room = croom;
    }

    levl[x][y].typ = STAIRS;
    levl[x][y].ladder = up ? LA_UP : LA_DOWN;
}

/* Place a fountain.
 * If mazeflag is TRUE, it will pick a random maze position; otherwise it will
 * assume croom is non-null and will pick a random position inside it.
 * May become a magic fountain with 1/7 chance. */
STATIC_OVL void
mkfount(mazeflag, croom)
int mazeflag;
struct mkroom *croom;
{
    coord m;
    register int tryct = 0;

    /* This code is repeated across several functions and should probably be
     * extracted into its own function... */
    do {
        if (++tryct > 200)
            return;
        if (mazeflag)
            mazexy(&m);
        else if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    levl[m.x][m.y].typ = FOUNTAIN;
    /* Is it a "blessed" fountain? (affects drinking from fountain) */
    if (!rn2(7))
        levl[m.x][m.y].blessedftn = 1;

    level.flags.nfountains++;
}

/* Place a sink somewhere in croom. */
STATIC_OVL void
mksink(croom)
struct mkroom *croom;
{
    coord m;
    register int tryct = 0;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    /* Put a sink at m.x, m.y */
    levl[m.x][m.y].typ = SINK;

    level.flags.nsinks++;
}

/* Place an altar somewhere in croom.
 * Set its alignment randomly with uniform probability. */
STATIC_OVL void
mkaltar(croom)
struct mkroom *croom;
{
    coord m;
    register int tryct = 0;
    aligntyp al;

    if (croom->rtype != OROOM)
        return;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    /* Put an altar at m.x, m.y */
    levl[m.x][m.y].typ = ALTAR;

    /* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
    al = rn2((int) A_LAWFUL + 2) - 1;
    levl[m.x][m.y].altarmask = Align2amask(al);
}

/* Create a grave (headstone) somewhere in croom. Special rules:
 * 1/10 of graves get a bell placed on them and a special inscription. The
 *   inscription is otherwise pulled from epitaph.
 * 1/3 of graves get gold placed on them.
 * 0-4 random cursed objects may be buried under the grave.
 */
static void
mkgrave(croom)
struct mkroom *croom;
{
    coord m;
    register int tryct = 0;
    register struct obj *otmp;
    boolean dobell = !rn2(10);

    if (croom->rtype != OROOM)
        return;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    /* Put a grave at m.x, m.y */
    make_grave(m.x, m.y, dobell ? "Saved by the bell!" : (char *) 0);

    /* Possibly fill it with objects */
    if (!rn2(3))
        (void) mkgold(0L, m.x, m.y);
    for (tryct = rn2(5); tryct; tryct--) {
        otmp = mkobj(RANDOM_CLASS, TRUE);
        if (!otmp)
            return;
        curse(otmp);
        otmp->ox = m.x;
        otmp->oy = m.y;
        add_to_buried(otmp);
    }

    /* Leave a bell, in case we accidentally buried someone alive */
    if (dobell)
        (void) mksobj_at(BELL, m.x, m.y, TRUE, FALSE);
    return;
}

/* maze levels have slightly different constraints from normal levels */
#define x_maze_min 2
#define y_maze_min 2

/*
 * Major level transmutation: add a set of stairs (to the Sanctum) after
 * an earthquake that leaves behind a a new topology, centered at inv_pos.
 * Assumes there are no rooms within the invocation area and that inv_pos
 * is not too close to the edge of the map.  Also assume the hero can see,
 * which is guaranteed for normal play due to the fact that sight is needed
 * to read the Book of the Dead.
 * FIXME: Can't assume that the hero can see, they could be roleplaying blind.
 */
void
mkinvokearea()
{
    int dist;
    xchar xmin = inv_pos.x, xmax = inv_pos.x;
    xchar ymin = inv_pos.y, ymax = inv_pos.y;
    register xchar i;

    pline_The("floor shakes violently under you!");
    pline_The("walls around you begin to bend and crumble!");
    display_nhwindow(WIN_MESSAGE, TRUE);

    /* any trap hero is stuck in will be going away now */
    if (u.utrap) {
        u.utrap = 0;
        if (u.utraptype == TT_BURIEDBALL)
            buried_ball_to_punishment();
    }
    mkinvpos(xmin, ymin, 0); /* middle, before placing stairs */

    for (dist = 1; dist < 7; dist++) {
        xmin--;
        xmax++;

        /* top and bottom */
        if (dist != 3) { /* the area is wider that it is high */
            ymin--;
            ymax++;
            for (i = xmin + 1; i < xmax; i++) {
                mkinvpos(i, ymin, dist);
                mkinvpos(i, ymax, dist);
            }
        }

        /* left and right */
        for (i = ymin; i <= ymax; i++) {
            mkinvpos(xmin, i, dist);
            mkinvpos(xmax, i, dist);
        }

        flush_screen(1); /* make sure the new glyphs shows up */
        delay_output();
    }

    You("are standing at the top of a stairwell leading down!");
    mkstairs(u.ux, u.uy, 0, (struct mkroom *) 0); /* down */
    newsym(u.ux, u.uy);
    vision_full_recalc = 1; /* everything changed */
}

/* Change level topology.  Boulders in the vicinity are eliminated.
 * Temporarily overrides vision in the name of a nice effect.
 */
STATIC_OVL void
mkinvpos(x, y, dist)
xchar x, y;
int dist;
{
    struct trap *ttmp;
    struct obj *otmp;
    boolean make_rocks;
    register struct rm *lev = &levl[x][y];

    /* clip at existing map borders if necessary */
    if (!within_bounded_area(x, y, x_maze_min + 1, y_maze_min + 1,
                             x_maze_max - 1, y_maze_max - 1)) {
        /* only outermost 2 columns and/or rows may be truncated due to edge
         */
        if (dist < (7 - 2))
            panic("mkinvpos: <%d,%d> (%d) off map edge!", x, y, dist);
        return;
    }

    /* clear traps */
    if ((ttmp = t_at(x, y)) != 0)
        deltrap(ttmp);

    /* clear boulders; leave some rocks for non-{moat|trap} locations */
    make_rocks = (dist != 1 && dist != 4 && dist != 5) ? TRUE : FALSE;
    while ((otmp = sobj_at(BOULDER, x, y)) != 0) {
        if (make_rocks) {
            fracture_rock(otmp);
            make_rocks = FALSE; /* don't bother with more rocks */
        } else {
            obj_extract_self(otmp);
            obfree(otmp, (struct obj *) 0);
        }
    }
    unblock_point(x, y); /* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seenv = 0;
    lev->doormask = 0;
    if (dist < 6)
        lev->lit = TRUE;
    lev->waslit = TRUE;
    lev->horizontal = FALSE;
    /* short-circuit vision recalc */
    viz_array[y][x] = (dist < 6) ? (IN_SIGHT | COULD_SEE) : COULD_SEE;

    switch (dist) {
    case 1: /* fire traps */
        if (is_pool(x, y))
            break;
        lev->typ = ROOM;
        ttmp = maketrap(x, y, FIRE_TRAP);
        if (ttmp)
            ttmp->tseen = TRUE;
        break;
    case 0: /* lit room locations */
    case 2:
    case 3:
    case 6: /* unlit room locations */
        lev->typ = ROOM;
        break;
    case 4: /* pools (aka a wide moat) */
    case 5:
        lev->typ = MOAT;
        /* No kelp! */
        break;
    default:
        impossible("mkinvpos called with dist %d", dist);
        break;
    }

    /* display new value of position; could have a monster/object on it */
    newsym(x, y);
}

/*
 * The portal to Ludios is special.  The entrance can only occur within a
 * vault in the main dungeon at a depth greater than 10.  The Ludios branch
 * structure reflects this by having a bogus "source" dungeon:  the value
 * of n_dgns (thus, Is_branchlev() will never find it).
 *
 * Ludios will remain isolated until the branch is corrected by this function.
 */
STATIC_OVL void
mk_knox_portal(x, y)
xchar x, y;
{
    extern int n_dgns; /* from dungeon.c */
    d_level *source;
    branch *br;
    schar u_depth;

    br = dungeon_branch("Fort Ludios");
    if (on_level(&knox_level, &br->end1)) {
        source = &br->end2;
    } else {
        /* disallow Knox branch on a level with one branch already */
        if (Is_branchlev(&u.uz))
            return;
        source = &br->end1;
    }

    /* Already set or 2/3 chance of deferring until a later level. */
    if (source->dnum < n_dgns || (rn2(3) && !wizard))
        return;

    if (!(u.uz.dnum == oracle_level.dnum      /* in main dungeon */
          && !at_dgn_entrance("The Quest")    /* but not Quest's entry */
          && (u_depth = depth(&u.uz)) > 10    /* beneath 10 */
          && u_depth < depth(&medusa_level))) /* and above Medusa */
        return;

    /* Adjust source to be current level and re-insert branch. */
    *source = u.uz;
    insert_branch(br, TRUE);

    debugpline0("Made knox portal.");
    place_branch(br, x, y);
}

/*mklev.c*/

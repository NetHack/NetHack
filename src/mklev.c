/* NetHack 3.6	mklev.c	$NHDT-Date: 1511681724 2017/11/26 07:35:24 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Alex Smith, 2017. */
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

/* Args must be (const genericptr) so that qsort will always be happy. */

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
    /* cannot find something reasonable -- strange */
    x = xl;
    y = yh;
gotit:
    cc->x = x;
    cc->y = y;
    return;
}

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

STATIC_OVL void
makerooms()
{
    boolean tried_vault = FALSE;

    /* make rooms until satisfied */
    /* rnd_rect() will returns 0 if no more rects are available... */
    while (nroom < MAXNROFROOMS && rnd_rect()) {
        if (nroom >= (MAXNROFROOMS / 6) && rn2(2) && !tried_vault) {
            tried_vault = TRUE;
            if (create_vault()) {
                vault_x = rooms[nroom].lx;
                vault_y = rooms[nroom].ly;
                rooms[nroom].hx = -1;
            }
        } else if (!create_room(-1, -1, -1, -1, -1, -1, OROOM, -1))
            return;
    }
    return;
}

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

    if (troom->hx < 0 || croom->hx < 0 || doorindex >= DOORMAX)
        return;
    if (troom->lx > croom->hx) {
        dx = 1;
        dy = 0;
        xx = croom->hx + 1;
        tx = troom->lx - 1;
        finddpos(&cc, xx, croom->ly, xx, croom->hy);
        finddpos(&tt, tx, troom->ly, tx, troom->hy);
    } else if (troom->hy < croom->ly) {
        dy = -1;
        dx = 0;
        yy = croom->ly - 1;
        finddpos(&cc, croom->lx, yy, croom->hx, yy);
        ty = troom->hy + 1;
        finddpos(&tt, troom->lx, ty, troom->hx, ty);
    } else if (troom->hx < croom->lx) {
        dx = -1;
        dy = 0;
        xx = croom->lx - 1;
        tx = troom->hx + 1;
        finddpos(&cc, xx, croom->ly, xx, croom->hy);
        finddpos(&tt, tx, troom->ly, tx, troom->hy);
    } else {
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
    if (nxcor && levl[xx + dx][yy + dy].typ)
        return;
    if (okdoor(xx, yy) || !nxcor)
        dodoor(xx, yy, croom);

    org.x = xx + dx;
    org.y = yy + dy;
    dest.x = tx;
    dest.y = ty;

    if (!dig_corridor(&org, &dest, nxcor, level.flags.arboreal ? ROOM : CORR,
                      STONE))
        return;

    /* we succeeded in digging the corridor */
    if (okdoor(tt.x, tt.y) || !nxcor)
        dodoor(tt.x, tt.y, troom);

    if (smeq[a] < smeq[b])
        smeq[b] = smeq[a];
    else
        smeq[a] = smeq[b];
}

void
makecorridors()
{
    int a, b, i;
    boolean any = TRUE;

    for (a = 0; a < nroom - 1; a++) {
        join(a, a + 1, FALSE);
        if (!rn2(50))
            break; /* allow some randomness */
    }
    for (a = 0; a < nroom - 2; a++)
        if (smeq[a] != smeq[a + 2])
            join(a, a + 2, FALSE);
    for (a = 0; any && a < nroom; a++) {
        any = FALSE;
        for (b = 0; b < nroom; b++)
            if (smeq[a] != smeq[b]) {
                join(a, b, FALSE);
                any = TRUE;
            }
    }
    if (nroom > 2)
        for (i = rn2(nroom) + 4; i; i--) {
            a = rn2(nroom);
            b = rn2(nroom - 2);
            if (b >= a)
                b += 2;
            join(a, b, TRUE);
        }
}

void
add_door(x, y, aroom)
register int x, y;
register struct mkroom *aroom;
{
    register struct mkroom *broom;
    register int tmp;
    int i;

    if (aroom->doorct == 0)
        aroom->fdoor = doorindex;

    aroom->doorct++;

    for (tmp = doorindex; tmp > aroom->fdoor; tmp--)
        doors[tmp] = doors[tmp - 1];

    for (i = 0; i < nroom; i++) {
        broom = &rooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }
    for (i = 0; i < nsubroom; i++) {
        broom = &subrooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }

    doorindex++;
    doors[aroom->fdoor].x = x;
    doors[aroom->fdoor].y = y;
}

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

STATIC_OVL boolean
place_niche(aroom, dy, xx, yy)
register struct mkroom *aroom;
int *dy, *xx, *yy;
{
    coord dd;

    if (rn2(2)) {
        *dy = 1;
        finddpos(&dd, aroom->lx, aroom->hy + 1, aroom->hx, aroom->hy + 1);
    } else {
        *dy = -1;
        finddpos(&dd, aroom->lx, aroom->ly - 1, aroom->hx, aroom->ly - 1);
    }
    *xx = dd.x;
    *yy = dd.y;
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

STATIC_OVL void
makeniche(trap_type)
int trap_type;
{
    register struct mkroom *aroom;
    struct rm *rm;
    int vct = 8;
    int dy, xx, yy;
    struct trap *ttmp;

    if (doorindex < DOORMAX) {
        while (vct--) {
            aroom = &rooms[rn2(nroom)];
            if (aroom->rtype != OROOM)
                continue; /* not an ordinary room */
            if (aroom->doorct == 1 && rn2(5))
                continue;
            if (!place_niche(aroom, &dy, &xx, &yy))
                continue;

            rm = &levl[xx][yy + dy];
            if (trap_type || !rn2(4)) {
                rm->typ = SCORR;
                if (trap_type) {
                    if (is_hole(trap_type) && !Can_fall_thru(&u.uz))
                        trap_type = ROCKTRAP;
                    ttmp = maketrap(xx, yy + dy, trap_type);
                    if (ttmp) {
                        if (trap_type != ROCKTRAP)
                            ttmp->once = 1;
                        if (trap_engravings[trap_type]) {
                            make_engr_at(xx, yy - dy,
                                         trap_engravings[trap_type], 0L,
                                         DUST);
                            wipe_engr_at(xx, yy - dy, 5,
                                         FALSE); /* age it a little */
                        }
                    }
                }
                dosdoor(xx, yy, aroom, SDOOR);
            } else {
                rm->typ = CORR;
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

STATIC_OVL void
make_niches()
{
    int ct = rnd((nroom >> 1) + 1), dep = depth(&u.uz);
    boolean ltptr = (!level.flags.noteleport && dep > 15),
            vamp = (dep > 5 && dep < 25);

    while (ct--) {
        if (ltptr && !rn2(6)) {
            ltptr = FALSE;
            makeniche(LEVEL_TELEP);
        } else if (vamp && !rn2(6)) {
            vamp = FALSE;
            makeniche(TRAPDOOR);
        } else
            makeniche(NO_TRAP);
    }
}

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

STATIC_OVL void
makelevel()
{
    register struct mkroom *croom, *troom;
    register int tryct;
    register int x, y;
    struct monst *tmonst; /* always put a web with a spider */
    branch *branchp;
    int room_threshold;

    if (wiz1_level.dlevel == 0)
        init_dungeons();
    oinit(); /* assign level dependent obj probabilities */
    clear_level_structures();

    {
        register s_level *slev = Is_special(&u.uz);

        /* check for special levels */
        if (slev && !Is_rogue_level(&u.uz)) {
            makemaz(slev->proto);
            return;
        } else if (dungeons[u.uz.dnum].proto[0]) {
            makemaz("");
            return;
        } else if (In_mines(&u.uz)) {
            makemaz("minefill");
            return;
        } else if (In_quest(&u.uz)) {
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
            makemaz("");
            return;
        }
    }

    /* otherwise, fall through - it's a "regular" level. */

    if (Is_rogue_level(&u.uz)) {
        makeroguerooms();
        makerogueghost();
    } else
        makerooms();
    sort_rooms();

    /* construct stairs (up and down in different rooms if possible) */
    croom = &rooms[rn2(nroom)];
    if (!Is_botlevel(&u.uz))
        mkstairs(somex(croom), somey(croom), 0, croom); /* down */
    if (nroom > 1) {
        troom = croom;
        croom = &rooms[rn2(nroom - 1)];
        if (croom == troom)
            croom++;
    }

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

    /* make a secret treasure vault, not connected to the rest */
    if (do_vault()) {
        xchar w, h;
        debugpline0("trying to make a vault...");
        w = 1;
        h = 1;
        if (check_room(&vault_x, &w, &vault_y, &h, TRUE)) {
        fill_vault:
            add_room(vault_x, vault_y, vault_x + w, vault_y + h, TRUE, VAULT,
                     FALSE);
            level.flags.has_vault = 1;
            ++room_threshold;
            fill_room(&rooms[nroom - 1], FALSE);
            mk_knox_portal(vault_x + w, vault_y + h);
            if (!level.flags.noteleport && !rn2(3))
                makevtele();
        } else if (rnd_rect() && create_vault()) {
            vault_x = rooms[nroom].lx;
            vault_y = rooms[nroom].ly;
            if (check_room(&vault_x, &w, &vault_y, &h, TRUE))
                goto fill_vault;
            else
                rooms[nroom].hx = -1;
        }
    }

    {
        register int u_depth = depth(&u.uz);

        if (wizard && nh_getenv("SHOPTYPE"))
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
            x = 2;
        while (!rn2(x))
            mktrap(0, 0, croom, (coord *) 0);
        if (!rn2(3))
            (void) mkgold(0L, somex(croom), somey(croom));
        if (Is_rogue_level(&u.uz))
            goto skip_nonrogue;
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

        /* maybe make some graffiti */
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
 *      Place deposits of minerals (gold and misc gems) in the stone
 *      surrounding the rooms on the map.
 *      Also place kelp in water.
 *      mineralize(-1, -1, -1, -1, FALSE); => "default" behaviour
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

/* If given a branch, randomly place a special stair or portal. */
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

/* see whether it is allowable to create a door at [x,y] */
int
okdoor(x, y)
xchar x, y;
{
    boolean near_door = bydoor(x, y);

    return ((levl[x][y].typ == HWALL || levl[x][y].typ == VWALL)
            && doorindex < DOORMAX && !near_door);
}

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

boolean
occupied(x, y)
register xchar x, y;
{
    return (boolean) (t_at(x, y) || IS_FURNITURE(levl[x][y].typ)
                      || is_lava(x, y) || is_pool(x, y)
                      || invocation_pos(x, y));
}

/* make a trap somewhere (in croom if mazeflag = 0 && !tm) */
/* if tm != null, make trap at that location */
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

    if (is_hole(kind) && !Can_fall_thru(&u.uz))
        kind = ROCKTRAP;

    if (tm) {
        m = *tm;
    } else {
        register int tryct = 0;
        boolean avoid_boulder = (is_pit(kind) || is_hole(kind));

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
        && !is_pit(kind) && kind < HOLE) {
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

void
mkstairs(x, y, up, croom)
xchar x, y;
char up;
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

STATIC_OVL void
mkfount(mazeflag, croom)
int mazeflag;
struct mkroom *croom;
{
    coord m;
    register int tryct = 0;

    do {
        if (++tryct > 200)
            return;
        if (mazeflag)
            mazexy(&m);
        else if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    /* Put a fountain at m.x, m.y */
    levl[m.x][m.y].typ = FOUNTAIN;
    /* Is it a "blessed" fountain? (affects drinking from fountain) */
    if (!rn2(7))
        levl[m.x][m.y].blessedftn = 1;

    level.flags.nfountains++;
}

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

    /* Put a grave at <m.x,m.y> */
    make_grave(m.x, m.y, dobell ? "Saved by the bell!" : (char *) 0);

    /* Possibly fill it with objects */
    if (!rn2(3)) {
        /* this used to use mkgold(), which puts a stack of gold on
           the ground (or merges it with an existing one there if
           present), and didn't bother burying it; now we create a
           loose, easily buriable, stack but we make no attempt to
           replicate mkgold()'s level-based formula for the amount */
        struct obj *gold = mksobj(GOLD_PIECE, TRUE, FALSE);

        gold->quan = (long) (rnd(20) + level_difficulty() * rnd(5));
        gold->owt = weight(gold);
        gold->ox = m.x, gold->oy = m.y;
        add_to_buried(gold);
    }
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
        if (u.utraptype == TT_BURIEDBALL)
            buried_ball_to_punishment();
        reset_utrap(FALSE);
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
    livelog_write_string(LL_ACHIEVE, "performed the invocation");
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
    struct monst *mon;

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

    if ((mon = m_at(x, y)) != 0) {
        /* wake up mimics, don't want to deal with them blocking vision */
        if (mon->m_ap_type)
            seemimic(mon);

        if ((ttmp = t_at(x, y)) != 0)
            (void) mintrap(mon);
        else
            (void) minliquid(mon);
    }

    if (!does_block(x, y, lev))
        unblock_point(x, y); /* make sure vision knows this location is open */

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

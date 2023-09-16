/* NetHack 3.7	mklev.c	$NHDT-Date: 1691877661 2023/08/12 22:01:01 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.155 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Alex Smith, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* for UNIX, Rand #def'd to (long)lrand48() or (long)random() */
/* croom->lx etc are schar (width <= int), so % arith ensures that */
/* conversion of result to int is reasonable */

static boolean generate_stairs_room_good(struct mkroom *, int);
static struct mkroom *generate_stairs_find_room(void);
static void generate_stairs(void);
static void mkfount(struct mkroom *);
static boolean find_okay_roompos(struct mkroom *, coord *);
static void mksink(struct mkroom *);
static void mkaltar(struct mkroom *);
static void mkgrave(struct mkroom *);
static void makevtele(void);
void clear_level_structures(void);
static void fill_ordinary_room(struct mkroom *);
static void makelevel(void);
static boolean bydoor(coordxy, coordxy);
static void mktrap_victim(struct trap *);
static struct mkroom *find_branch_room(coord *);
static struct mkroom *pos_to_room(coordxy, coordxy);
static boolean cardinal_nextto_room(struct mkroom *, coordxy, coordxy);
static boolean place_niche(struct mkroom *, int *, coordxy *, coordxy *);
static void makeniche(int);
static void make_niches(void);
static int QSORTCALLBACK mkroom_cmp(const genericptr, const genericptr);
static void dosdoor(coordxy, coordxy, struct mkroom *, int);
static void join(int, int, boolean);
static void alloc_doors(void);
static void do_room_or_subroom(struct mkroom *,
                               coordxy, coordxy, coordxy, coordxy,
                               boolean, schar, boolean, boolean);
static void makerooms(void);
static boolean door_into_nonjoined(coordxy, coordxy);
static boolean finddpos(coord *, coordxy, coordxy, coordxy, coordxy);
static void mkinvpos(coordxy, coordxy, int);
static void mk_knox_portal(coordxy, coordxy);

#define create_vault() create_room(-1, -1, 2, 2, -1, -1, VAULT, TRUE)
#define init_vault() gv.vault_x = -1
#define do_vault() (gv.vault_x != -1)

/* Args must be (const genericptr) so that qsort will always be happy. */

static int QSORTCALLBACK
mkroom_cmp(const genericptr vx, const genericptr vy)
{
    register const struct mkroom *x, *y;

    x = (const struct mkroom *) vx;
    y = (const struct mkroom *) vy;
    if (x->lx < y->lx)
        return -1;
    return (x->lx > y->lx);
}

/* Return TRUE if a door placed at (x, y) which otherwise passes okdoor()
 * checks would be connecting into an area that was declared as joined = false.
 * Checking for this in finddpos() enables us to have rooms with sub-areas
 * (such as shops) that will never randomly generate unwanted doors in order
 * to connect them up to other areas.
 */
static boolean
door_into_nonjoined(coordxy x, coordxy y)
{
    coordxy tx, ty, i;

    for (i = 0; i < 4; i++) {
        tx = x + xdir[dirs_ord[i]];
        ty = y + ydir[dirs_ord[i]];
        if (!isok(tx, ty) || IS_ROCK(levl[tx][ty].typ))
            continue;

        /* Is this connecting to a room that doesn't want joining? */
        if (levl[tx][ty].roomno >= ROOMOFFSET
            && !gr.rooms[levl[tx][ty].roomno - ROOMOFFSET].needjoining) {
            return TRUE;
        }
    }
    return FALSE;
}

static boolean
finddpos(coord *cc, coordxy xl, coordxy yl, coordxy xh, coordxy yh)
{
    register coordxy x, y;

    x = rn1(xh - xl + 1, xl);
    y = rn1(yh - yl + 1, yl);
    if (okdoor(x, y) && !door_into_nonjoined(x, y))
        goto gotit;

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (okdoor(x, y) && !door_into_nonjoined(x, y))
                goto gotit;

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR)
                goto gotit;
    /* cannot find something reasonable -- strange */
    x = xl;
    y = yh;
    return FALSE;
 gotit:
    cc->x = x;
    cc->y = y;
    return TRUE;
}

/* Sort rooms on the level so they're ordered from left to right on the map.
   makecorridors() by default links rooms N and N+1 */
void
sort_rooms(void)
{
    coordxy x, y;
    unsigned i, ri[MAXNROFROOMS + 1], n = (unsigned) gn.nroom;

    qsort((genericptr_t) gr.rooms, n, sizeof (struct mkroom), mkroom_cmp);

    /* Update the roomnos on the map */
    for (i = 0; i < n; i++)
        ri[gr.rooms[i].roomnoidx] = i;

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            unsigned rno = levl[x][y].roomno;

            if (rno >= ROOMOFFSET && rno < MAXNROFROOMS + 1)
                levl[x][y].roomno = ri[rno - ROOMOFFSET] + ROOMOFFSET;
        }
}

static void
do_room_or_subroom(register struct mkroom *croom,
                   coordxy lowx, coordxy lowy, coordxy hix, coordxy hiy,
                   boolean lit, schar rtype, boolean special, boolean is_room)
{
    coordxy x, y;
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

    croom->roomnoidx = (croom - gr.rooms);
    croom->lx = lowx;
    croom->hx = hix;
    croom->ly = lowy;
    croom->hy = hiy;
    croom->rtype = rtype;
    croom->doorct = 0;
    /* if we're not making a vault, gd.doorindex will still be 0
     * if we are, we'll have problems adding niches to the previous room
     * unless fdoor is at least gd.doorindex
     */
    croom->fdoor = gd.doorindex;
    croom->irregular = FALSE;

    croom->nsubrooms = 0;
    croom->sbrooms[0] = (struct mkroom *) 0;
    if (!special) {
        croom->needjoining = TRUE;
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
add_room(int lowx, int lowy, int hix, int hiy,
         boolean lit, schar rtype, boolean special)
{
    register struct mkroom *croom;

    croom = &gr.rooms[gn.nroom];
    do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special,
                       (boolean) TRUE);
    croom++;
    croom->hx = -1;
    gn.nroom++;
}

void
add_subroom(struct mkroom *proom, int lowx, int lowy, int hix, int hiy,
            boolean lit, schar rtype, boolean special)
{
    register struct mkroom *croom;

    croom = &gs.subrooms[gn.nsubroom];
    do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special,
                       (boolean) FALSE);
    proom->sbrooms[proom->nsubrooms++] = croom;
    croom++;
    croom->hx = -1;
    gn.nsubroom++;
}

void
free_luathemes(enum lua_theme_group theme_group)
{
    int i;

    /*
     * Release which group(s)?
     *  tut_themes  => leaving tutorial, free tutorial themes only;
     *  most_themes => entering endgame, free non-endgame themes;
     *  all_themes  => end of game, free all themes.
     */
    for (i = 0; i < gn.n_dgns; ++i) {
        if ((theme_group == tut_themes && i != tutorial_dnum)
            || (theme_group == most_themes && i == astral_level.dnum))
            continue;
        if (gl.luathemes[i]) {
            nhl_done((lua_State *) gl.luathemes[i]);
            gl.luathemes[i] = (lua_State *) 0;
        }
    }
}

static void
makerooms(void)
{
    boolean tried_vault = FALSE;
    int themeroom_tries = 0;
    char *fname;
    nhl_sandbox_info sbi = {NHL_SB_SAFE, 0, 0, 0};
    lua_State *themes = (lua_State *) gl.luathemes[u.uz.dnum];

    if (!themes && *(fname = gd.dungeons[u.uz.dnum].themerms)) {
        if ((themes = nhl_init(&sbi)) != 0) {
            if (!nhl_loadlua(themes, fname)) {
                /* loading lua failed, don't use themed rooms */
                nhl_done(themes);
                themes = (lua_State *) 0;
            } else {
                /* success; save state for this dungeon branch */
                gl.luathemes[u.uz.dnum] = (genericptr_t) themes;
                /* keep themes context, so not 'nhl_done(themes);' */
                iflags.in_lua = FALSE; /* can affect error messages */
            }
        }
        if (!themes) /* don't try again when making next level */
            *fname = '\0'; /* gd.dungeons[u.uz.dnum].themerms */
    }

    if (themes) {
        create_des_coder();
        iflags.in_lua = gi.in_mk_themerooms = TRUE;
        gt.themeroom_failed = FALSE;
        lua_getglobal(themes, "pre_themerooms_generate");
        if ( nhl_pcall(themes, 0, 0)){
            impossible("Lua error: %s", lua_tostring(themes, -1));
        }
        iflags.in_lua = gi.in_mk_themerooms = FALSE;
    }

    /* make rooms until satisfied */
    /* rnd_rect() will returns 0 if no more rects are available... */
    while (gn.nroom < (MAXNROFROOMS-1) && rnd_rect()) {
        if (gn.nroom >= (MAXNROFROOMS / 6) && rn2(2) && !tried_vault) {
            tried_vault = TRUE;
            if (create_vault()) {
                gv.vault_x = gr.rooms[gn.nroom].lx;
                gv.vault_y = gr.rooms[gn.nroom].ly;
                gr.rooms[gn.nroom].hx = -1;
            }
        } else {
            if (themes) {
                iflags.in_lua = gi.in_mk_themerooms = TRUE;
                gt.themeroom_failed = FALSE;
                lua_getglobal(themes, "themerooms_generate");
                if (nhl_pcall(themes, 0, 0)) {
                    impossible("Lua error: %s", lua_tostring(themes, -1));
                }
                iflags.in_lua = gi.in_mk_themerooms = FALSE;
                if (gt.themeroom_failed
                    && ((themeroom_tries++ > 10)
                        || (gn.nroom >= (MAXNROFROOMS / 6))))
                    break;
            } else {
                if (!create_room(-1, -1, -1, -1, -1, -1, OROOM, -1))
                    break;;
            }
        }
    }
    if (themes) {
        reset_xystart_size();
        iflags.in_lua = gi.in_mk_themerooms = TRUE;
        gt.themeroom_failed = FALSE;
        lua_getglobal(themes, "post_themerooms_generate");
        if ( nhl_pcall(themes, 0, 0)){
            impossible("Lua error: %s", lua_tostring(themes, -1));
        }
        iflags.in_lua = gi.in_mk_themerooms = FALSE;

        wallification(1, 0, COLNO - 1, ROWNO - 1);
        free(gc.coder);
        gc.coder = NULL;
    }
}

static void
join(register int a, register int b, boolean nxcor)
{
    coord cc, tt, org, dest;
    register coordxy tx, ty, xx, yy;
    register struct mkroom *croom, *troom;
    register int dx, dy;

    croom = &gr.rooms[a];
    troom = &gr.rooms[b];

    if (!croom->needjoining || !troom->needjoining)
        return;

    /* find positions cc and tt for doors in croom and troom
       and direction for a corridor between them */

    if (troom->hx < 0 || croom->hx < 0)
        return;
    if (troom->lx > croom->hx) {
        dx = 1;
        dy = 0;
        xx = croom->hx + 1;
        tx = troom->lx - 1;
        if (!finddpos(&cc, xx, croom->ly, xx, croom->hy))
            return;
        if (!finddpos(&tt, tx, troom->ly, tx, troom->hy))
            return;
    } else if (troom->hy < croom->ly) {
        dy = -1;
        dx = 0;
        yy = croom->ly - 1;
        ty = troom->hy + 1;
        if (!finddpos(&cc, croom->lx, yy, croom->hx, yy))
            return;
        if (!finddpos(&tt, troom->lx, ty, troom->hx, ty))
            return;
    } else if (troom->hx < croom->lx) {
        dx = -1;
        dy = 0;
        xx = croom->lx - 1;
        tx = troom->hx + 1;
        if (!finddpos(&cc, xx, croom->ly, xx, croom->hy))
            return;
        if (!finddpos(&tt, tx, troom->ly, tx, troom->hy))
            return;
    } else {
        dy = 1;
        dx = 0;
        yy = croom->hy + 1;
        ty = troom->ly - 1;
        if (!finddpos(&cc, croom->lx, yy, croom->hx, yy))
            return;
        if (!finddpos(&tt, troom->lx, ty, troom->hx, ty))
            return;
    }
    xx = cc.x;
    yy = cc.y;
    tx = tt.x - dx;
    ty = tt.y - dy;
    if (nxcor && levl[xx + dx][yy + dy].typ != STONE)
        return;
    if (okdoor(xx, yy) || !nxcor)
        dodoor(xx, yy, croom);

    org.x = xx + dx;
    org.y = yy + dy;
    dest.x = tx;
    dest.y = ty;

    if (!dig_corridor(&org, &dest, nxcor,
                      gl.level.flags.arboreal ? ROOM : CORR, STONE))
        return;

    /* we succeeded in digging the corridor */
    if (okdoor(tt.x, tt.y) || !nxcor)
        dodoor(tt.x, tt.y, troom);

    if (gs.smeq[a] < gs.smeq[b])
        gs.smeq[b] = gs.smeq[a];
    else
        gs.smeq[a] = gs.smeq[b];
}

void
makecorridors(void)
{
    int a, b, i;
    boolean any = TRUE;

    for (a = 0; a < gn.nroom - 1; a++) {
        join(a, a + 1, FALSE);
        if (!rn2(50))
            break; /* allow some randomness */
    }
    for (a = 0; a < gn.nroom - 2; a++)
        if (gs.smeq[a] != gs.smeq[a + 2])
            join(a, a + 2, FALSE);
    for (a = 0; any && a < gn.nroom; a++) {
        any = FALSE;
        for (b = 0; b < gn.nroom; b++)
            if (gs.smeq[a] != gs.smeq[b]) {
                join(a, b, FALSE);
                any = TRUE;
            }
    }
    if (gn.nroom > 2)
        for (i = rn2(gn.nroom) + 4; i; i--) {
            a = rn2(gn.nroom);
            b = rn2(gn.nroom - 2);
            if (b >= a)
                b += 2;
            join(a, b, TRUE);
        }
}

/* (re)allocate space for gd.doors array */
static void
alloc_doors(void)
{
    if (!gd.doors || gd.doorindex >= gd.doors_alloc) {
        int c = gd.doors_alloc + DOORINC;
        coord *doortmp = (coord *) alloc(c * sizeof(coord));

        (void) memset((genericptr_t) doortmp, 0, c * sizeof(coord));
        if (gd.doors) {
            (void) memcpy(doortmp, gd.doors, gd.doors_alloc * sizeof(coord));
            free(gd.doors);
        }
        gd.doors = doortmp;
        gd.doors_alloc = c;
    }
}

void
add_door(coordxy x, coordxy y, register struct mkroom *aroom)
{
    register struct mkroom *broom;
    register int tmp;
    int i;

    alloc_doors();

    if (aroom->doorct) {
        for (i = 0; i < aroom->doorct; i++) {
            tmp = aroom->fdoor + i;
            if (gd.doors[tmp].x == x && gd.doors[tmp].y == y)
                return;
        }
    }

    if (aroom->doorct == 0)
        aroom->fdoor = gd.doorindex;

    aroom->doorct++;

    for (tmp = gd.doorindex; tmp > aroom->fdoor; tmp--)
        gd.doors[tmp] = gd.doors[tmp - 1];

    for (i = 0; i < gn.nroom; i++) {
        broom = &gr.rooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }
    for (i = 0; i < gn.nsubroom; i++) {
        broom = &gs.subrooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }

    gd.doorindex++;
    gd.doors[aroom->fdoor].x = x;
    gd.doors[aroom->fdoor].y = y;
}

static void
dosdoor(register coordxy x, register coordxy y, struct mkroom *aroom, int type)
{
    boolean shdoor = *in_rooms(x, y, SHOPBASE) ? TRUE : FALSE;

    if (!IS_WALL(levl[x][y].typ)) /* avoid S.doors on already made doors */
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
           making mimics in place of trapped doors on rogue gl.level */
        if (Is_rogue_level(&u.uz))
            levl[x][y].doormask = D_NODOOR;

        if (levl[x][y].doormask & D_TRAPPED) {
            struct monst *mtmp;

            if (level_difficulty() >= 9 && !rn2(5)
                && !((gm.mvitals[PM_SMALL_MIMIC].mvflags & G_GONE)
                     && (gm.mvitals[PM_LARGE_MIMIC].mvflags & G_GONE)
                     && (gm.mvitals[PM_GIANT_MIMIC].mvflags & G_GONE))) {
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

/* is x,y location such that NEWS direction from it is inside aroom,
   excluding subrooms */
static boolean
cardinal_nextto_room(struct mkroom *aroom, coordxy x, coordxy y)
{
    int rmno = (int) ((aroom - gr.rooms) + ROOMOFFSET);

    if (isok(x - 1, y) && !levl[x - 1][y].edge
        && (int) levl[x - 1][y].roomno == rmno)
        return TRUE;
    if (isok(x + 1, y) && !levl[x + 1][y].edge
        && (int) levl[x + 1][y].roomno == rmno)
        return TRUE;
    if (isok(x, y - 1) && !levl[x][y - 1].edge
        && (int) levl[x][y - 1].roomno == rmno)
        return TRUE;
    if (isok(x, y + 1) && !levl[x][y + 1].edge
        && (int) levl[x][y + 1].roomno == rmno)
        return TRUE;
    return FALSE;
}

static boolean
place_niche(
    struct mkroom *aroom,
    int *dy,
    coordxy *xx, coordxy *yy)
{
    coord dd;

    if (rn2(2)) {
        *dy = 1;
        if (!finddpos(&dd, aroom->lx, aroom->hy + 1, aroom->hx, aroom->hy + 1))
            return FALSE;
    } else {
        *dy = -1;
        if (!finddpos(&dd, aroom->lx, aroom->ly - 1, aroom->hx, aroom->ly - 1))
            return FALSE;
    }
    *xx = dd.x;
    *yy = dd.y;
    return (boolean) ((isok(*xx, *yy + *dy)
                       && levl[*xx][*yy + *dy].typ == STONE)
                      && (isok(*xx, *yy - *dy)
                          && !IS_POOL(levl[*xx][*yy - *dy].typ)
                          && !IS_FURNITURE(levl[*xx][*yy - *dy].typ))
                      && cardinal_nextto_room(aroom, *xx, *yy));
}

/* there should be one of these per trap, in the same order as trap.h */
static NEARDATA const char *trap_engravings[TRAPNUM] = {
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0,
    /* 14..16: trap door, teleport, level-teleport */
    "Vlad was here", "ad aerarium", "ad aerarium", (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    /* 24..25 */
    (char *) 0, (char *) 0,
};

static void
makeniche(int trap_type)
{
    register struct mkroom *aroom;
    struct rm *rm;
    int dy, vct = 8;
    coordxy xx, yy;
    struct trap *ttmp;

    while (vct--) {
        aroom = &gr.rooms[rn2(gn.nroom)];
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
                    (void) set_levltyp(xx, yy, IRONBARS);
                    if (rn2(3))
                        (void) mkcorpstat(CORPSE, (struct monst *) 0,
                                          mkclass(S_HUMAN, 0), xx,
                                          yy + dy, TRUE);
                }
                if (!gl.level.flags.noteleport)
                    (void) mksobj_at(SCR_TELEPORTATION, xx, yy + dy, TRUE,
                                     FALSE);
                if (!rn2(3))
                    (void) mkobj_at(RANDOM_CLASS, xx, yy + dy, TRUE);
            }
        }
        return;
    }
}

static void
make_niches(void)
{
    int ct = rnd((gn.nroom >> 1) + 1), dep = depth(&u.uz);
    boolean ltptr = (!gl.level.flags.noteleport && dep > 15),
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

static void
makevtele(void)
{
    makeniche(TELEP_TRAP);
}

/* count the different features (sinks, fountains) in the level */
void
count_level_features(void)
{
    coordxy x, y;

    gl.level.flags.nfountains = gl.level.flags.nsinks = 0;
    for (y = 0; y < ROWNO; y++)
        for (x = 1; x < COLNO; x++) {
            int typ = levl[x][y].typ;

            if (typ == FOUNTAIN)
                gl.level.flags.nfountains++;
            else if (typ == SINK)
                gl.level.flags.nsinks++;
        }
}

/* clear out various globals that keep information on the current level.
 * some of this is only necessary for some types of levels (maze, normal,
 * special) but it's easier to put it all in one place than make sure
 * each type initializes what it needs to separately.
 */
void
clear_level_structures(void)
{
    static struct rm zerorm = { GLYPH_UNEXPLORED,
                                0, 0, 0, 0, 0, 0, 0, 0, 0 };
    coordxy x, y;
    register struct rm *lev;

    /* note:  normally we'd start at x=1 because map column #0 isn't used
       (except for placing vault guard at <0,0> when removed from the map
       but not from the level); explicitly reset column #0 along with the
       rest so that we start the new level with a completely clean slate */
    for (x = 0; x < COLNO; x++) {
        lev = &levl[x][0];
        for (y = 0; y < ROWNO; y++) {
            *lev++ = zerorm;
            gl.level.objects[x][y] = (struct obj *) 0;
            gl.level.monsters[x][y] = (struct monst *) 0;
        }
    }
    gl.level.objlist = (struct obj *) 0;
    gl.level.buriedobjlist = (struct obj *) 0;
    gl.level.monlist = (struct monst *) 0;
    gl.level.damagelist = (struct damage *) 0;
    gl.level.bonesinfo = (struct cemetery *) 0;

    gl.level.flags.nfountains = 0;
    gl.level.flags.nsinks = 0;
    gl.level.flags.has_shop = 0;
    gl.level.flags.has_vault = 0;
    gl.level.flags.has_zoo = 0;
    gl.level.flags.has_court = 0;
    gl.level.flags.has_morgue = gl.level.flags.graveyard = 0;
    gl.level.flags.has_beehive = 0;
    gl.level.flags.has_barracks = 0;
    gl.level.flags.has_temple = 0;
    gl.level.flags.has_swamp = 0;
    gl.level.flags.noteleport = 0;
    gl.level.flags.hardfloor = 0;
    gl.level.flags.nommap = 0;
    gl.level.flags.hero_memory = 1;
    gl.level.flags.shortsighted = 0;
    gl.level.flags.sokoban_rules = 0;
    gl.level.flags.is_maze_lev = 0;
    gl.level.flags.is_cavernous_lev = 0;
    gl.level.flags.arboreal = 0;
    gl.level.flags.has_town = 0;
    gl.level.flags.wizard_bones = 0;
    gl.level.flags.corrmaze = 0;
    gl.level.flags.rndmongen = 0;
    gl.level.flags.deathdrops = 0;
    gl.level.flags.noautosearch = 0;
    gl.level.flags.fumaroles = 0;
    gl.level.flags.stormy = 0;

    gn.nroom = 0;
    gr.rooms[0].hx = -1;
    gn.nsubroom = 0;
    gs.subrooms[0].hx = -1;
    gd.doorindex = 0;
    gd.doors_alloc = 0;
    free(gd.doors);
    gd.doors = (coord *) 0;
    init_rect();
    init_vault();
    stairway_free_all();
    gm.made_branch = FALSE;
    clear_regions();
    free_exclusions();
    reset_xystart_size();
    if (gl.lev_message) {
        free(gl.lev_message);
        gl.lev_message = (char *) 0;
    }
}

/* Fill a "random" room (i.e. a typical non-special room in the Dungeons of
 * Doom) with random monsters, objects, and dungeon features.
 */
static void
fill_ordinary_room(struct mkroom *croom)
{
    int trycnt = 0;
    coord pos;
    struct monst *tmonst; /* always put a web with a spider */
    coordxy x, y;

    if (croom->rtype != OROOM && croom->rtype != THEMEROOM)
        return;

    /* If there are subrooms, fill them now - we don't want an outer room
     * that's specified to be unfilled to block an inner subroom that's
     * specified to be filled. */
    for (x = 0; x < croom->nsubrooms; ++x) {
        fill_ordinary_room(croom->sbrooms[x]);
    }

    if (croom->needfill != FILL_NORMAL)
        return;

    /* put a sleeping monster inside */
    /* Note: monster may be on the stairs. This cannot be
       avoided: maybe the player fell through a trap door
       while a monster was on the stairs. Conclusion:
       we have to check for monsters on the stairs anyway. */

    if ((u.uhave.amulet || !rn2(3)) && somexyspace(croom, &pos)) {
        tmonst = makemon((struct permonst *) 0, pos.x, pos.y, MM_NOGRP);
        if (tmonst && tmonst->data == &mons[PM_GIANT_SPIDER]
            && !occupied(pos.x, pos.y))
            (void) maketrap(pos.x, pos.y, WEB);
    }
    /* put traps and mimics inside */
    x = 8 - (level_difficulty() / 6);
    if (x <= 1)
        x = 2;
    while (!rn2(x) && (++trycnt < 1000))
        mktrap(0, MKTRAP_NOFLAGS, croom, (coord *) 0);
    if (!rn2(3) && somexyspace(croom, &pos))
        (void) mkgold(0L, pos.x, pos.y);
    if (Is_rogue_level(&u.uz))
        goto skip_nonrogue;
    if (!rn2(10))
        mkfount(croom);
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
    if (!rn2(20) && somexyspace(croom, &pos))
        (void) mkcorpstat(STATUE, (struct monst *) 0,
                            (struct permonst *) 0, pos.x,
                            pos.y, CORPSTAT_INIT);
    /* put box/chest inside;
     *  40% chance for at least 1 box, regardless of number
     *  of rooms; about 5 - 7.5% for 2 boxes, least likely
     *  when few rooms; chance for 3 or more is negligible.
     */
    if (!rn2(gn.nroom * 5 / 2) && somexyspace(croom, &pos))
        (void) mksobj_at((rn2(3)) ? LARGE_BOX : CHEST,
                            pos.x, pos.y, TRUE, FALSE);

    /* maybe make some graffiti */
    if (!rn2(27 + 3 * abs(depth(&u.uz)))) {
        char buf[BUFSZ];
        const char *mesg = random_engraving(buf);

        if (mesg) {
            do {
                somexyspace(croom, &pos);
                x = pos.x;
                y = pos.y;
            } while (levl[x][y].typ != ROOM && !rn2(40));
            if (!(IS_POOL(levl[x][y].typ)
                    || IS_FURNITURE(levl[x][y].typ)))
                make_engr_at(x, y, mesg, 0L, MARK);
        }
    }

 skip_nonrogue:
    if (!rn2(3) && somexyspace(croom, &pos)) {
        (void) mkobj_at(RANDOM_CLASS, pos.x, pos.y, TRUE);
        trycnt = 0;
        while (!rn2(5)) {
            if (++trycnt > 100) {
                impossible("trycnt overflow4");
                break;
            }
            if (somexyspace(croom, &pos)) {
                (void) mkobj_at(RANDOM_CLASS, pos.x, pos.y, TRUE);
            }
        }
    }
}

static void
makelevel(void)
{
    register struct mkroom *croom;
    branch *branchp;
    stairway *prevstairs;
    int room_threshold;
    register s_level *slev = Is_special(&u.uz);
    int i;

    if (wiz1_level.dlevel == 0)
        init_dungeons();
    oinit(); /* assign level dependent obj probabilities */
    clear_level_structures();

    /* check for special levels */
    if (slev && !Is_rogue_level(&u.uz)) {
        makemaz(slev->proto);
    } else if (gd.dungeons[u.uz.dnum].proto[0]) {
        makemaz("");
    } else if (gd.dungeons[u.uz.dnum].fill_lvl[0]) {
        makemaz(gd.dungeons[u.uz.dnum].fill_lvl);
    } else if (In_quest(&u.uz)) {
        char fillname[9];
        s_level *loc_lev;

        Sprintf(fillname, "%s-loca", gu.urole.filecode);
        loc_lev = find_level(fillname);

        Sprintf(fillname, "%s-fil", gu.urole.filecode);
        Strcat(fillname,
                (u.uz.dlevel < loc_lev->dlevel.dlevel) ? "a" : "b");
        makemaz(fillname);
    } else if (In_hell(&u.uz)
                || (rn2(5) && u.uz.dnum == medusa_level.dnum
                    && depth(&u.uz) > depth(&medusa_level))) {
        makemaz("");
    } else {
        /* otherwise, fall through - it's a "regular" level. */
        register int u_depth = depth(&u.uz);

        if (Is_rogue_level(&u.uz)) {
            makeroguerooms();
            makerogueghost();
        } else
            makerooms();
        sort_rooms();

        generate_stairs(); /* up and down stairs */

        branchp = Is_branchlev(&u.uz);    /* possible dungeon branch */
        room_threshold = branchp ? 4 : 3; /* minimum number of rooms needed
                                            to allow a random special room */
        if (Is_rogue_level(&u.uz))
            goto skip0;
        makecorridors();
        make_niches();

        /* make a secret treasure vault, not connected to the rest */
        if (do_vault()) {
            coordxy w, h;

            debugpline0("trying to make a vault...");
            w = 1;
            h = 1;
            if (check_room(&gv.vault_x, &w, &gv.vault_y, &h, TRUE)) {
 fill_vault:
                add_room(gv.vault_x, gv.vault_y, gv.vault_x + w, gv.vault_y + h,
                         TRUE, VAULT, FALSE);
                gl.level.flags.has_vault = 1;
                ++room_threshold;
                gr.rooms[gn.nroom - 1].needfill = FILL_NORMAL;
                fill_special_room(&gr.rooms[gn.nroom - 1]);
                mk_knox_portal(gv.vault_x + w, gv.vault_y + h);
                if (!gl.level.flags.noteleport && !rn2(3))
                    makevtele();
            } else if (rnd_rect() && create_vault()) {
                gv.vault_x = gr.rooms[gn.nroom].lx;
                gv.vault_y = gr.rooms[gn.nroom].ly;
                if (check_room(&gv.vault_x, &w, &gv.vault_y, &h, TRUE))
                    goto fill_vault;
                else
                    gr.rooms[gn.nroom].hx = -1;
            }
        }

        /* make up to 1 special room, with type dependent on depth;
           note that mkroom doesn't guarantee a room gets created, and that
           this step only sets the room's rtype - it doesn't fill it yet. */
        if (wizard && nh_getenv("SHOPTYPE"))
            do_mkroom(SHOPBASE);
        else if (u_depth > 1 && u_depth < depth(&medusa_level)
                 && gn.nroom >= room_threshold && rn2(u_depth) < 3)
            do_mkroom(SHOPBASE);
        else if (u_depth > 4 && !rn2(6))
            do_mkroom(COURT);
        else if (u_depth > 5 && !rn2(8)
                 && !(gm.mvitals[PM_LEPRECHAUN].mvflags & G_GONE))
            do_mkroom(LEPREHALL);
        else if (u_depth > 6 && !rn2(7))
            do_mkroom(ZOO);
        else if (u_depth > 8 && !rn2(5))
            do_mkroom(TEMPLE);
        else if (u_depth > 9 && !rn2(5)
                 && !(gm.mvitals[PM_KILLER_BEE].mvflags & G_GONE))
            do_mkroom(BEEHIVE);
        else if (u_depth > 11 && !rn2(6))
            do_mkroom(MORGUE);
        else if (u_depth > 12 && !rn2(8) && antholemon())
            do_mkroom(ANTHOLE);
        else if (u_depth > 14 && !rn2(4)
                 && !(gm.mvitals[PM_SOLDIER].mvflags & G_GONE))
            do_mkroom(BARRACKS);
        else if (u_depth > 15 && !rn2(6))
            do_mkroom(SWAMP);
        else if (u_depth > 16 && !rn2(8)
                 && !(gm.mvitals[PM_COCKATRICE].mvflags & G_GONE))
            do_mkroom(COCKNEST);

 skip0:
        prevstairs = gs.stairs; /* used to test for place_branch() success */
        /* Place multi-dungeon branch. */
        place_branch(branchp, 0, 0);

        /* for main dungeon level 1, the stairs up where the hero starts
           are branch stairs; treat them as if hero had just come down
           them by marking them as having been traversed; most recently
           created stairway is held in 'gs.stairs' */
        if (u.uz.dnum == 0 && u.uz.dlevel == 1 && gs.stairs != prevstairs)
            gs.stairs->u_traversed = TRUE;

        /* for each room: put things inside */
        for (croom = gr.rooms; croom->hx > 0; croom++) {
            fill_ordinary_room(croom);
        }
    }
    /* Fill all special rooms now, regardless of whether this is a special
     * level, proto level, or ordinary level. */
    for (i = 0; i < gn.nroom; ++i) {
        fill_special_room(&gr.rooms[i]);
    }

    if (gl.luacore && nhcb_counts[NHCB_LVL_ENTER]) {
        lua_getglobal(gl.luacore, "nh_callback_run");
        lua_pushstring(gl.luacore, nhcb_name[NHCB_LVL_ENTER]);
        nhl_pcall(gl.luacore, 1, 0);
    }
}

/*
 *      Place deposits of minerals (gold and misc gems) in the stone
 *      surrounding the rooms on the map.
 *      Also place kelp in water.
 *      mineralize(-1, -1, -1, -1, FALSE); => "default" behavior
 */
void
mineralize(int kelp_pool, int kelp_moat, int goldprob, int gemprob,
           boolean skip_lvl_checks)
{
    s_level *sp;
    struct obj *otmp;
    coordxy x, y;
    int cnt;

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
            || gl.level.flags.arboreal
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
level_finalize_topology(void)
{
    struct mkroom *croom;
    int ridx;

    bound_digging();
    mineralize(-1, -1, -1, -1, FALSE);
    gi.in_mklev = FALSE;
    /* avoid coordinates in future lua-loads for this level being thrown off
     * because xstart and ystart aren't saved with the level and will be 0 after
     * leaving and returning */
    gx.xstart = gy.ystart = 0;
    /* has_morgue gets cleared once morgue is entered; graveyard stays
       set (graveyard might already be set even when has_morgue is clear
       [see fixup_special()], so don't update it unconditionally) */
    if (gl.level.flags.has_morgue)
        gl.level.flags.graveyard = 1;
    if (!gl.level.flags.is_maze_lev) {
        for (croom = &gr.rooms[0]; croom != &gr.rooms[gn.nroom]; croom++)
#ifdef SPECIALIZATION
            topologize(croom, FALSE);
#else
            topologize(croom);
#endif
    }
    set_wall_state();
    /* for many room types, gr.rooms[].rtype is zeroed once the room has been
       entered; gr.rooms[].orig_rtype always retains original rtype value */
    for (ridx = 0; ridx < SIZE(gr.rooms); ridx++)
        gr.rooms[ridx].orig_rtype = gr.rooms[ridx].rtype;
}

void
mklev(void)
{
    reseed_random(rn2);
    reseed_random(rn2_on_display_rng);

    init_mapseen(&u.uz);
    if (getbones())
        return;

    gi.in_mklev = TRUE;
    makelevel();

    level_finalize_topology();

    reseed_random(rn2);
    reseed_random(rn2_on_display_rng);
}

void
#ifdef SPECIALIZATION
topologize(struct mkroom *croom, boolean do_ordinary)
#else
topologize(struct mkroom *croom)
#endif
{
    coordxy x, y;
    register int roomno = (int) ((croom - gr.rooms) + ROOMOFFSET);
    coordxy lowx = croom->lx, lowy = croom->ly;
    coordxy hix = croom->hx, hiy = croom->hy;
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
    /* gs.subrooms */
    for (subindex = 0; subindex < nsubrooms; subindex++)
#ifdef SPECIALIZATION
        topologize(croom->sbrooms[subindex], (boolean) (rtype != OROOM));
#else
        topologize(croom->sbrooms[subindex]);
#endif
}

/* Find an unused room for a branch location. */
static struct mkroom *
find_branch_room(coord *mp)
{
    struct mkroom *croom = 0;

    if (gn.nroom == 0) {
        mazexy(mp); /* already verifies location */
    } else {
        croom = generate_stairs_find_room();

        if (!somexyspace(croom, mp))
            impossible("Can't place branch!");
    }
    return croom;
}

/* Find the room for (x,y).  Return null if not in a room. */
static struct mkroom *
pos_to_room(coordxy x, coordxy y)
{
    int i;
    struct mkroom *curr;

    for (curr = gr.rooms, i = 0; i < gn.nroom; curr++, i++)
        if (inside_room(curr, x, y))
            return curr;
    ;
    return (struct mkroom *) 0;
}

/* If given a branch, randomly place a special stair or portal. */
void
place_branch(
    branch *br,       /* branch to place */
    coordxy x, coordxy y) /* location */
{
    coord m = {0};
    d_level *dest;
    boolean make_stairs;
    struct mkroom *br_room;

    /*
     * Return immediately if there is no branch to make or we have
     * already made one.  This routine can be called twice when
     * a special level is loaded that specifies an SSTAIR location
     * as a favored spot for a branch.
     */
    if (!br || gm.made_branch)
        return;

    nhUse(br_room);
    if (!x) { /* find random coordinates for branch */
        /* br_room = find_branch_room(&m); */
        (void) find_branch_room(&m);  /* sets m via mazexy() or somexy() */
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
        if (iflags.debug_fuzzer && (u.ucamefrom.dnum || u.ucamefrom.dlevel))
            mkportal(x, y, u.ucamefrom.dnum, u.ucamefrom.dlevel);
        else
            mkportal(x, y, dest->dnum, dest->dlevel);
    } else if (make_stairs) {
        boolean goes_up = on_level(&br->end1, &u.uz) ? br->end1_up
                                                     : !br->end1_up;

        stairway_add(x, y, goes_up, FALSE, dest);
        (void) set_levltyp(x, y, STAIRS);
        levl[x][y].ladder = goes_up ? LA_UP : LA_DOWN;
    }
    /*
     * Set made_branch to TRUE even if we didn't make a stairwell (i.e.
     * make_stairs is false) since there is currently only one branch
     * per level, if we failed once, we're going to fail again on the
     * next call.
     */
    gm.made_branch = TRUE;
}

static boolean
bydoor(register coordxy x, register coordxy y)
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
okdoor(coordxy x, coordxy y)
{
    boolean near_door = bydoor(x, y);

    return ((levl[x][y].typ == HWALL || levl[x][y].typ == VWALL)
            && ((isok(x - 1, y) && !IS_ROCK(levl[x - 1][y].typ))
                || (isok(x + 1, y) && !IS_ROCK(levl[x + 1][y].typ))
                || (isok(x, y - 1) && !IS_ROCK(levl[x][y - 1].typ))
                || (isok(x, y + 1) && !IS_ROCK(levl[x][y + 1].typ)))
            && !near_door);
}

void
dodoor(coordxy x, coordxy y, struct mkroom *aroom)
{
    dosdoor(x, y, aroom, rn2(8) ? DOOR : SDOOR);
}

boolean
occupied(coordxy x, coordxy y)
{
    return (boolean) (t_at(x, y) || IS_FURNITURE(levl[x][y].typ)
                      || is_lava(x, y) || is_pool(x, y)
                      || invocation_pos(x, y));
}

/* generate a corpse and some items on top of a trap */
static void
mktrap_victim(struct trap *ttmp)
{
    /* Object generated by the trap; initially NULL, stays NULL if
       we fail to generate an object or if the trap doesn't
       generate objects. */
    struct obj *otmp = NULL;
    int victim_mnum; /* race of the victim */
    unsigned lvl = level_difficulty();
    int kind = ttmp->ttyp;
    coordxy x = ttmp->tx, y = ttmp->ty;

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
        place_object(otmp, x, y);
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
            place_object(otmp, x, y);
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
            place_object(otmp, x, y);
        }
        break;
    default:
        /* the most common race */
        victim_mnum = PM_HUMAN;
        break;
    }
    otmp = mkcorpstat(CORPSE, NULL, &mons[victim_mnum], x, y,
                      CORPSTAT_INIT);
    if (otmp)
        otmp->age -= (TAINT_AGE + 1); /* died too long ago to eat */
}

/* make a trap somewhere (in croom if mazeflag = 0 && !tm) */
/* if tm != null, make trap at that location */
void
mktrap(
    int num,
    int mktrapflags,
    struct mkroom *croom,
    coord *tm)
{
    register int kind;
    struct trap *t;
    unsigned lvl = level_difficulty();
    coord m;

    /* no traps in pools */
    if (tm && is_pool(tm->x, tm->y))
        return;

    if (num > NO_TRAP && num < TRAPNUM) {
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
            /* these are controlled by the feature or object they guard,
               not by the map so mustn't be created on it */
            case TRAPPED_DOOR:
            case TRAPPED_CHEST:
                kind = NO_TRAP;
                break;
            /* these can have a random location but can't be generated
               randomly */
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
                if (lvl < 5 || gl.level.flags.noteleport
                    || single_level_branch(&u.uz))
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
                if (lvl < 7 && !(mktrapflags & MKTRAP_NOSPIDERONWEB))
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
                if (gl.level.flags.noteleport)
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
            if (mktrapflags & MKTRAP_MAZEFLAG)
                mazexy(&m);
            else if (!somexyspace(croom, &m))
                return;
        } while (occupied(m.x, m.y)
                 || (avoid_boulder && sobj_at(BOULDER, m.x, m.y)));
    }

    t = maketrap(m.x, m.y, kind);
    /* we should always get type of trap we're asking for (occupied() test
       should prevent cases where that might not happen) but be paranoid */
    kind = t ? t->ttyp : NO_TRAP;

    if (kind == WEB && !(mktrapflags & MKTRAP_NOSPIDERONWEB))
        (void) makemon(&mons[PM_GIANT_SPIDER], m.x, m.y, NO_MM_FLAGS);
    if (t && (mktrapflags & MKTRAP_SEEN))
        t->tseen = TRUE;
    if (kind == MAGIC_PORTAL && (u.ucamefrom.dnum || u.ucamefrom.dlevel)) {
        assign_level(&t->dst, &u.ucamefrom);
    }

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
    if (kind != NO_TRAP && !(mktrapflags & MKTRAP_NOVICTIM)
        && lvl <= (unsigned) rnd(4)
        && kind != SQKY_BOARD && kind != RUST_TRAP
        /* rolling boulder trap might not have a boulder if there was no
           viable path (such as when placed in the corner of a room), in
           which case tx,ty==launch.x,y; no boulder => no dead predecessor */
        && !(kind == ROLLING_BOULDER_TRAP
             && t->launch.x == t->tx && t->launch.y == t->ty)
        && !is_pit(kind) && kind < HOLE) {
        mktrap_victim(t);
    }
}

/* Create stairs up or down at x,y.
   If force is TRUE, change the terrain to ROOM first */
void
mkstairs(
    coordxy x, coordxy y,
    char up,       /* [why 'char' when usage is boolean?] */
    struct mkroom *croom UNUSED,
    boolean force)
{
    int ltyp;
    d_level dest;

    if (!x || !isok(x, y)) {
        impossible("mkstairs:  bogus stair attempt at <%d,%d>", x, y);
        return;
    }
    if (force)
        levl[x][y].typ = ROOM;
    ltyp = levl[x][y].typ; /* somexyspace() allows ice */
    if (ltyp != ROOM && ltyp != CORR && ltyp != ICE) {
        int glyph = back_to_glyph(x, y),
            sidx = glyph_to_cmap(glyph);

        impossible("mkstairs:  placing stairs %s on %s at <%d,%d>",
                   up ? "up" : "down", defsyms[sidx].explanation, x, y);
    }

    /*
     * We can't make a regular stair off an end of the dungeon.  This
     * attempt can happen when a special level is placed at an end and
     * has an up or down stair specified in its description file.
     */
    if (dunlev(&u.uz) == (up ? 1 : dunlevs_in_dungeon(&u.uz)))
        return;

    dest.dnum = u.uz.dnum;
    dest.dlevel = u.uz.dlevel + (up ? -1 : 1);
    stairway_add(x, y, up ? TRUE : FALSE, FALSE, &dest);

    (void) set_levltyp(x, y, STAIRS);
    levl[x][y].ladder = up ? LA_UP : LA_DOWN;
}

/* is room a good one to generate up or down stairs in? */
static boolean
generate_stairs_room_good(struct mkroom *croom, int phase)
{
    /*
     * phase values, smaller allows for more relaxed criteria:
     *  2 == no relaxed criteria;
     *  1 == allow a themed room;
     *  0 == allow same room as existing up/downstairs;
     * -1 == allow an unjoined room.
     */
    return (croom && (croom->needjoining || (phase < 0))
            && ((!has_dnstairs(croom) && !has_upstairs(croom))
                || phase < 1)
            && (croom->rtype == OROOM
                || ((phase < 2) && croom->rtype == THEMEROOM)));
}

/* find a good room to generate an up or down stairs in */
static struct mkroom *
generate_stairs_find_room(void)
{
    struct mkroom *croom;
    int i, phase, ai;
    int *rmarr;

    if (!gn.nroom)
        return (struct mkroom *) 0;

    rmarr = (int *) alloc(sizeof(int) * gn.nroom);

    for (phase = 2; phase > -1; phase--) {
        ai = 0;
        for (i = 0; i < gn.nroom; i++)
            if (generate_stairs_room_good(&gr.rooms[i], phase))
                rmarr[ai++] = i;
        if (ai > 0) {
            i = rmarr[rn2(ai)];
            free(rmarr);
            return &gr.rooms[i];
        }
    }

    free(rmarr);
    croom = &gr.rooms[rn2(gn.nroom)];
    return croom;
}

/* construct stairs up and down within the same branch,
   up and down in different rooms if possible */
static void
generate_stairs(void)
{
    struct mkroom *croom = generate_stairs_find_room();
    coord pos;

    if (!Is_botlevel(&u.uz)) {
        if (!somexyspace(croom, &pos)) {
            pos.x = somex(croom);
            pos.y = somey(croom);
        }
        mkstairs(pos.x, pos.y, 0, croom, FALSE); /* down */
    }

    if (gn.nroom > 1)
        croom = generate_stairs_find_room();

    if (u.uz.dlevel != 1) {
        if (!somexyspace(croom, &pos)) {
            pos.x = somex(croom);
            pos.y = somey(croom);
        }
        mkstairs(pos.x, pos.y, 1, croom, FALSE); /* up */
    }
}

static void
mkfount(struct mkroom *croom)
{
    coord m;

    if (!find_okay_roompos(croom, &m))
        return;

    /* Put a fountain at m.x, m.y */
    if (!set_levltyp(m.x, m.y, FOUNTAIN))
        return;
    /* Is it a "blessed" fountain? (affects drinking from fountain) */
    if (!rn2(7))
        levl[m.x][m.y].blessedftn = 1;

    gl.level.flags.nfountains++;
}

static boolean
find_okay_roompos(struct mkroom *croom, coord *crd)
{
    int tryct = 0;

    do {
        if (++tryct > 200)
            return FALSE;
        if (!somexyspace(croom, crd))
            return FALSE;
    } while (occupied(crd->x, crd->y) || bydoor(crd->x, crd->y));
    return TRUE;
}

static void
mksink(struct mkroom *croom)
{
    coord m;

    if (!find_okay_roompos(croom, &m))
        return;

    /* Put a sink at m.x, m.y */
    if (!set_levltyp(m.x, m.y, SINK))
        return;

    gl.level.flags.nsinks++;
}

static void
mkaltar(struct mkroom *croom)
{
    coord m;
    aligntyp al;

    if (croom->rtype != OROOM)
        return;

    if (!find_okay_roompos(croom, &m))
        return;

    /* Put an altar at m.x, m.y */
    if (!set_levltyp(m.x, m.y, ALTAR))
        return;

    /* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
    al = rn2((int) A_LAWFUL + 2) - 1;
    levl[m.x][m.y].altarmask = Align2amask(al);
}

static void
mkgrave(struct mkroom *croom)
{
    coord m;
    register int tryct = 0;
    register struct obj *otmp;
    boolean dobell = !rn2(10);

    if (croom->rtype != OROOM)
        return;

    if (!find_okay_roompos(croom, &m))
        return;

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
 * Major level transmutation:  add a set of stairs (to the Sanctum) after
 * an earthquake that leaves behind a new topology, centered at inv_pos.
 * Assumes there are no rooms within the invocation area and that gi.inv_pos
 * is not too close to the edge of the map.  Also assume the hero can see,
 * which is guaranteed for normal play due to the fact that sight is needed
 * to read the Book of the Dead.  [That assumption is not valid; it is
 * possible that "the Book reads the hero" rather than vice versa if
 * attempted while blind (in order to make blind-from-birth conduct viable).]
 */
void
mkinvokearea(void)
{
    int dist;
    coordxy xmin = gi.inv_pos.x, xmax = gi.inv_pos.x,
          ymin = gi.inv_pos.y, ymax = gi.inv_pos.y;
    register coordxy i;

    /* slightly odd if levitating, but not wrong */
    pline_The("floor shakes violently under you!");
    /*
     * TODO:
     *  Suppress this message if player has dug out all the walls
     *  that would otherwise be affected.
     */
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
        nh_delay_output();
    }

    You("are standing at the top of a stairwell leading down!");
    mkstairs(u.ux, u.uy, 0, (struct mkroom *) 0, FALSE); /* down */
    newsym(u.ux, u.uy);
    gv.vision_full_recalc = 1; /* everything changed */
}

/* Change level topology.  Boulders in the vicinity are eliminated.
 * Temporarily overrides vision in the name of a nice effect.
 */
static void
mkinvpos(coordxy x, coordxy y, int dist)
{
    struct trap *ttmp;
    struct obj *otmp;
    boolean make_rocks;
    register struct rm *lev = &levl[x][y];
    struct monst *mon;

    /* clip at existing map borders if necessary */
    if (!within_bounded_area(x, y, x_maze_min + 1, y_maze_min + 1,
                             gx.x_maze_max - 1, gy.y_maze_max - 1)) {
        /* outermost 2 columns and/or rows may be truncated due to edge */
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
    gv.viz_array[y][x] = (dist < 6) ? (IN_SIGHT | COULD_SEE) : COULD_SEE;

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
            (void) mintrap(mon, NO_TRAP_FLAGS);
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
static void
mk_knox_portal(coordxy x, coordxy y)
{
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
    if (source->dnum < gn.n_dgns || (rn2(3) && !wizard))
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

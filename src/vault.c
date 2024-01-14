/* NetHack 3.7	vault.c	$NHDT-Date: 1657868307 2022/07/15 06:58:27 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.91 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static boolean clear_fcorr(struct monst *, boolean) NONNULLARG1;
static void blackout(coordxy, coordxy);
static void restfakecorr(struct monst *) NONNULLARG1;
static void parkguard(struct monst *) NONNULLARG1;
static boolean in_fcorridor(struct monst *, coordxy, coordxy) NONNULLARG1;
static boolean find_guard_dest(struct monst *, coordxy *, coordxy *) NONNULLARG23;
static void move_gold(struct obj *, int) NONNULLARG1;
static void wallify_vault(struct monst *) NONNULLARG1;
static void gd_mv_monaway(struct monst *, int, int) NONNULLARG1;
static void gd_pick_corridor_gold(struct monst *, int, int) NONNULLARG1;
static int gd_move_cleanup(struct monst *, boolean, boolean) NONNULLARG1;
static void gd_letknow(struct monst *) NONNULLARG1;

void
newegd(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EGD(mtmp)) {
        EGD(mtmp) = (struct egd *) alloc(sizeof (struct egd));
        (void) memset((genericptr_t) EGD(mtmp), 0, sizeof (struct egd));
    }
}

void
free_egd(struct monst *mtmp)
{
    if (mtmp->mextra && EGD(mtmp)) {
        free((genericptr_t) EGD(mtmp));
        EGD(mtmp) = (struct egd *) 0;
    }
    mtmp->isgd = 0;
}

/* try to remove the temporary corridor (from vault to rest of map) being
   maintained by guard 'grd'; if guard is still in it, removal will fail,
   to be tried again later */
static boolean
clear_fcorr(struct monst *grd, boolean forceshow)
{
    coordxy fcx, fcy, fcbeg;
    struct monst *mtmp;
    boolean sawcorridor = FALSE,
            silently = gp.program_state.stopprint ? TRUE : FALSE;
    struct egd *egrd = EGD(grd);
    struct trap *trap;
    struct rm *lev;

    if (!on_level(&egrd->gdlevel, &u.uz))
        return TRUE;

    /* note: guard remains on 'fmon' list (alive or dead, at off-map
       coordinate <0,0>), until temporary corridor from vault back to
       civilization has been removed */
    while ((fcbeg = egrd->fcbeg) < egrd->fcend) {
        fcx = egrd->fakecorr[fcbeg].fx;
        fcy = egrd->fakecorr[fcbeg].fy;
        if ((DEADMONSTER(grd) || !in_fcorridor(grd, u.ux, u.uy))
            && egrd->gddone)
            forceshow = TRUE;
        if ((u_at(fcx, fcy) && !DEADMONSTER(grd))
            || (!forceshow && couldsee(fcx, fcy))
            || (Punished && !carried(uball) && uball->ox == fcx
                && uball->oy == fcy))
            return FALSE;

        if ((mtmp = m_at(fcx, fcy)) != 0) {
            if (mtmp->isgd) {
                return FALSE;
            } else {
                if (mtmp->mtame)
                    yelp(mtmp);
                if (!rloc(mtmp, RLOC_MSG))
                    m_into_limbo(mtmp);
            }
        }
        lev = &levl[fcx][fcy];
        if (lev->typ == CORR && cansee(fcx, fcy))
            sawcorridor = TRUE;
        lev->typ = egrd->fakecorr[fcbeg].ftyp;
        lev->flags = egrd->fakecorr[fcbeg].flags;
        if (IS_STWALL(lev->typ)) {
            /* destroy any trap here (pit dug by you, hole dug via
               wand while levitating or by monster, bear trap or land
               mine via object, spun web) when spot reverts to stone */
            if ((trap = t_at(fcx, fcy)) != 0)
                deltrap(trap);
            /* undo scroll/wand/spell of light affecting this spot */
            if (lev->typ == STONE)
                blackout(fcx, fcy);
        }
        del_engr_at(fcx, fcy);
        map_location(fcx, fcy, 1); /* bypass vision */
        if (!ACCESSIBLE(lev->typ))
            block_point(fcx, fcy);
        gv.vision_full_recalc = 1;
        egrd->fcbeg++;
    }
    if (sawcorridor && !silently)
        pline_The("corridor disappears.");
    /* only give encased message if hero is still alive (might get here
       via paygd() -> mongone() -> grddead() when game is over;
       died: no message, quit: message) */
    if (IS_ROCK(levl[u.ux][u.uy].typ) && (Upolyd ? u.mh : u.uhp) > 0
        && !silently)
        You("are encased in rock.");
    return TRUE;
}

/* as a temporary corridor is removed, set stone locations and adjacent
   spots to unlit; if player used scroll/wand/spell of light while inside
   the corridor, we don't want the light to reappear if/when a new tunnel
   goes through the same area */
static void
blackout(coordxy x, coordxy y)
{
    struct rm *lev;
    int i, j;

    for (i = x - 1; i <= x + 1; ++i)
        for (j = y - 1; j <= y + 1; ++j) {
            if (!isok(i, j))
                continue;
            lev = &levl[i][j];
            /* [possible bug: when (i != x || j != y), perhaps we ought
               to check whether the spot on the far side is lit instead
               of doing a blanket blackout of adjacent locations] */
            if (lev->typ == STONE)
                lev->lit = lev->waslit = 0;
            /* mark <i,j> as not having been seen from <x,y> */
            unset_seenv(lev, x, y, i, j);
        }
}

static void
restfakecorr(struct monst *grd)
{
    /* it seems you left the corridor - let the guard disappear */
    if (clear_fcorr(grd, FALSE)) {
        grd->isgd = 0; /* dmonsfree() should delete this mon */
        mongone(grd);
    }
}

/* move guard--dead to alive--to <0,0> until temporary corridor is removed */
static void
parkguard(struct monst *grd)
{
    /* either guard is dead or will now be treated as if so;
       monster traversal loops should skip it */
    if (grd == gc.context.polearm.hitmon)
        gc.context.polearm.hitmon = 0;
    if (grd->mx) {
        remove_monster(grd->mx, grd->my);
        newsym(grd->mx, grd->my);
    }
    if (m_at(0, 0) != grd)
        place_monster(grd, 0, 0);
    /* [grd->mx,my just got set to 0,0 by place_monster(), so this
       just sets EGD(grd)->ogx,ogy to 0,0 too; is that what we want?] */
    EGD(grd)->ogx = grd->mx;
    EGD(grd)->ogy = grd->my;
}

/* called in mon.c */
boolean
grddead(struct monst *grd)
{
    boolean dispose = clear_fcorr(grd, TRUE);

    if (!dispose) {
        /* destroy guard's gold; drop any other inventory */
        relobj(grd, 0, FALSE);
        grd->mhp = 0;
        parkguard(grd);
        dispose = clear_fcorr(grd, TRUE);
    }
    if (dispose)
        grd->isgd = 0; /* for dmonsfree() */
    return dispose;
}

static boolean
in_fcorridor(struct monst *grd, coordxy x, coordxy y)
{
    register int fci;
    struct egd *egrd = EGD(grd);

    for (fci = egrd->fcbeg; fci < egrd->fcend; fci++)
        if (x == egrd->fakecorr[fci].fx && y == egrd->fakecorr[fci].fy)
            return TRUE;
    return FALSE;
}

struct monst *
findgd(void)
{
    struct monst *mtmp, **mprev;

    /* this might find a guard parked at <0,0> since it'll be on fmon list */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (mtmp->isgd && on_level(&EGD(mtmp)->gdlevel, &u.uz)) {
            if (!mtmp->mx && !EGD(mtmp)->gddone)
                mtmp->mhp = mtmp->mhpmax;
            return mtmp;
        }
    }
    /* if not on fmon, look for a guard waiting to migrate to this level */
    for (mprev = &gm.migrating_mons; (mtmp = *mprev) != 0;
         mprev = &mtmp->nmon) {
        if (mtmp->isgd && on_level(&EGD(mtmp)->gdlevel, &u.uz)) {
            /* take out of migrating_mons and place at <0,0>;
               simplified mon_arrive(); avoid that because it would
               send mtmp into limbo if no regular map spot is available */
            *mprev = mtmp->nmon;
            mtmp->nmon = fmon;
            fmon = mtmp;
            mon_track_clear(mtmp);
            mtmp->mux = u.ux, mtmp->muy = u.uy;
            mtmp->mx = mtmp->my = 0; /* not on map (note: mx is already 0) */
            parkguard(mtmp);
            return mtmp;
        }
    }
    return (struct monst *) 0;
}

void
vault_summon_gd(void)
{
    if (vault_occupied(u.urooms) && !findgd())
        u.uinvault = (VAULT_GUARD_TIME - 1);
}

char
vault_occupied(char *array)
{
    register char *ptr;

    for (ptr = array; *ptr; ptr++)
        if (gr.rooms[*ptr - ROOMOFFSET].rtype == VAULT)
            return *ptr;
    return '\0';
}

/* hero has teleported out of vault while a guard is active */
void
uleftvault(struct monst *grd)
{
    /* only called if caller has checked vault_occupied() and findgd() */
    if (!grd || !grd->isgd || DEADMONSTER(grd)) {
        impossible("escaping vault without guard?");
        return;
    }
    /* if carrying gold and arriving anywhere other than next to the guard,
       set the guard loose */
    if ((money_cnt(gi.invent) || hidden_gold(TRUE))
        && um_dist(grd->mx, grd->my, 1)) {
        if (grd->mpeaceful) {
            if (canspotmon(grd)) /* see or sense via telepathy */
                pline("%s becomes irate.", Monnam(grd));
            grd->mpeaceful = 0; /* bypass setmangry() */
        }
        /* if arriving outside guard's temporary corridor, give the
           guard an extra move to deliver message(s) and to teleport
           out of and remove that corridor */
        if (!in_fcorridor(grd, u.ux, u.uy))
            (void) gd_move(grd);
    }
}

static boolean
find_guard_dest(struct monst *guard, coordxy *rx, coordxy *ry)
{
    coordxy x, y, dd, lx, ly;

    for (dd = 2; (dd < ROWNO || dd < COLNO); dd++) {
        for (y = u.uy - dd; y <= u.uy + dd; y++) {
            if (y < 0 || y > ROWNO - 1)
                continue;
            for (x = u.ux - dd; x <= u.ux + dd; x++) {
                if (y != u.uy - dd && y != u.uy + dd && x != u.ux - dd)
                    x = u.ux + dd;
                if (x < 1 || x > COLNO - 1)
                    continue;
                if (guard && ((x == guard->mx && y == guard->my)
                              || (guard->isgd && in_fcorridor(guard, x, y))))
                    continue;
                if (levl[x][y].typ == CORR) {
                    lx = (x < u.ux) ? x + 1 : (x > u.ux) ? x - 1 : x;
                    ly = (y < u.uy) ? y + 1 : (y > u.uy) ? y - 1 : y;
                    if (levl[lx][ly].typ != STONE && levl[lx][ly].typ != CORR)
                        goto incr_radius;
                    *rx = x;
                    *ry = y;
                    return TRUE;
                }
            }
        }
 incr_radius:
        ;
    }
    impossible("Not a single corridor on this level?");
    tele();
    return FALSE;
}

void
invault(void)
{
    struct monst *guard;
    struct obj *otmp;
    boolean spotted;
    int trycount, vaultroom = (int) vault_occupied(u.urooms);

    if (!vaultroom) {
        u.uinvault = 0;
        return;
    }
    ++u.uinvault;
    if (u.uinvault < VAULT_GUARD_TIME
        || (u.uinvault % (VAULT_GUARD_TIME / 2)) != 0)
        return;

    guard = findgd();
    if (!guard) {
        /* if time ok and no guard now. */
        char buf[BUFSZ];
        int x, y, gdx, gdy, typ;
        coordxy rx, ry;
        long umoney;

        /* first find the goal for the guard */
        if (!find_guard_dest((struct monst *) 0, &rx, &ry))
            return;
        gdx = rx, gdy = ry;
        vaultroom -= ROOMOFFSET;

        /* next find a good place for a door in the wall */
        x = u.ux;
        y = u.uy;
        if (levl[x][y].typ != ROOM) { /* player dug a door and is in it */
            if (levl[x + 1][y].typ == ROOM) {
                x = x + 1;
            } else if (levl[x][y + 1].typ == ROOM) {
                y = y + 1;
            } else if (levl[x - 1][y].typ == ROOM) {
                x = x - 1;
            } else if (levl[x][y - 1].typ == ROOM) {
                y = y - 1;
            } else if (levl[x + 1][y + 1].typ == ROOM) {
                x = x + 1;
                y = y + 1;
            } else if (levl[x - 1][y - 1].typ == ROOM) {
                x = x - 1;
                y = y - 1;
            } else if (levl[x + 1][y - 1].typ == ROOM) {
                x = x + 1;
                y = y - 1;
            } else if (levl[x - 1][y + 1].typ == ROOM) {
                x = x - 1;
                y = y + 1;
            }
        }
        while (levl[x][y].typ == ROOM) {
            register int dx, dy;

            dx = (gdx > x) ? 1 : (gdx < x) ? -1 : 0;
            dy = (gdy > y) ? 1 : (gdy < y) ? -1 : 0;
            if (abs(gdx - x) >= abs(gdy - y))
                x += dx;
            else
                y += dy;
        }
        if (u_at(x, y)) {
            if (levl[x + 1][y].typ == HWALL || levl[x + 1][y].typ == DOOR)
                x = x + 1;
            else if (levl[x - 1][y].typ == HWALL
                     || levl[x - 1][y].typ == DOOR)
                x = x - 1;
            else if (levl[x][y + 1].typ == VWALL
                     || levl[x][y + 1].typ == DOOR)
                y = y + 1;
            else if (levl[x][y - 1].typ == VWALL
                     || levl[x][y - 1].typ == DOOR)
                y = y - 1;
            else
                return;
        }

        /* make something interesting happen */
        if (!(guard = makemon(&mons[PM_GUARD], x, y, MM_EGD | MM_NOMSG)))
            return;
        guard->isgd = 1;
        guard->mpeaceful = 1;
        set_malign(guard);
        EGD(guard)->gddone = 0;
        EGD(guard)->ogx = x;
        EGD(guard)->ogy = y;
        assign_level(&(EGD(guard)->gdlevel), &u.uz);
        EGD(guard)->vroom = vaultroom;
        EGD(guard)->warncnt = 0;

        reset_faint(); /* if fainted - wake up */
        /* if there are any boulders in the guard's way, destroy them;
           perhaps the guard knows a touch equivalent of force bolt;
           otherwise the hero wouldn't be able to push one to follow the
           guard out of the vault because that guard would be in its way */
        if ((otmp = sobj_at(BOULDER, guard->mx, guard->my)) != 0) {
            void (*func)(const char *, ...) PRINTF_F_PTR(1, 2);
            const char *bname = simpleonames(otmp);
            int bcnt = 0;

            do {
                ++bcnt;
                fracture_rock(otmp);
                otmp = sobj_at(BOULDER, guard->mx, guard->my);
            } while (otmp);
            /* You_hear() will handle Deaf/!Deaf */
            func = !Blind ? You_see : You_hear;
            (*func)("%s shatter.",
                    (bcnt == 1) ? an(bname) : makeplural(bname));
        }
        spotted = canspotmon(guard);
        if (spotted) {
            pline("Suddenly one of the Vault's %s enters!",
                  makeplural(pmname(guard->data, Mgender(guard))));
            newsym(guard->mx, guard->my);
        } else {
            pline("Someone else has entered the Vault.");
            /* make sure that hero who can't see the guard knows where the
               wall is breeched, otherwise we couldn't follow the guard out;
               the breech isn't necessarily adjacent to the hero */
            map_invisible(guard->mx, guard->my);
        }

        if (u.uswallow) {
            /* can't interrogate hero, don't interrogate engulfer */
            if (!Deaf) {
                SetVoice(guard, 0, 80, 0);
                verbalize("What's going on here?");
            }
            if (!spotted)
                pline_The("other presence vanishes.");
            mongone(guard);
            return;
        }
        if (U_AP_TYPE == M_AP_OBJECT || u.uundetected) {
            if (U_AP_TYPE == M_AP_OBJECT
                && gy.youmonst.mappearance != GOLD_PIECE)
                if (!Deaf) {
                    SetVoice(guard, 0, 80, 0);
                    verbalize("Hey!  Who left that %s in here?",
                              mimic_obj_name(&gy.youmonst));
                }
            /* You're mimicking some object or you're hidden. */
            pline("Puzzled, %s turns around and leaves.", mhe(guard));
            mongone(guard);
            return;
        }
        if (Strangled || is_silent(gy.youmonst.data) || gm.multi < 0) {
            /* [we ought to record whether this this message has already
               been given in order to vary it upon repeat visits, but
               discarding the monster and its egd data renders that hard] */
            if (Deaf) {
                pline("%s huffs and turns to leave.", noit_Monnam(guard));
            } else {
                SetVoice(guard, 0, 80, 0);
                verbalize("I'll be back when you're ready to speak to me!");
            }
            mongone(guard);
            return;
        }

        stop_occupation(); /* if occupied, stop it *now* */
        if (gm.multi > 0) {
            nomul(0);
            unmul((char *) 0);
        }
        buf[0] = '\0';
        trycount = 5;
        do {
            getlin(Deaf ? "You are required to supply your name. -"
                        : "\"Hello stranger, who are you?\" -", buf);
            (void) mungspaces(buf);
        } while (!buf[0] && --trycount > 0);

        if (u.ualign.type == A_LAWFUL
            /* ignore trailing text, in case player includes rank */
            && strncmpi(buf, gp.plname, (int) strlen(gp.plname)) != 0) {
            adjalign(-1); /* Liar! */
        }

        if (!strcmpi(buf, "Croesus") || !strcmpi(buf, "Kroisos")
            || !strcmpi(buf, "Creosote")) { /* Discworld */
            if (!gm.mvitals[PM_CROESUS].died) {
                if (Deaf) {
                    if (!Blind)
                        pline("%s waves goodbye.", noit_Monnam(guard));
                } else {
                    SetVoice(guard, 0, 80, 0);
                    verbalize(
                         "Oh, yes, of course.  Sorry to have disturbed you.");
                }
                mongone(guard);
            } else {
                setmangry(guard, FALSE);
                if (Deaf) {
                   if (!Blind)
                        pline("%s mouths something and looks very angry!",
                              noit_Monnam(guard));
                } else {
                   SetVoice(guard, 0, 80, 0);
                   verbalize(
                           "Back from the dead, are you?  I'll remedy that!");
                }
                /* don't want guard to waste next turn wielding a weapon */
                if (!MON_WEP(guard)) {
                    guard->weapon_check = NEED_HTH_WEAPON;
                    (void) mon_wield_item(guard);
                }
            }
            return;
        }
        if (Deaf) {
            pline("%s doesn't %srecognize you.", noit_Monnam(guard),
                    (Blind) ? "" : "appear to ");
        } else {
            SetVoice(guard, 0, 80, 0);
            verbalize("I don't know you.");
        }
        umoney = money_cnt(gi.invent);
        if (!umoney && !hidden_gold(TRUE)) {
            if (Deaf) {
                pline("%s stomps%s.", noit_Monnam(guard),
                      (Blind) ? "" : " and beckons");
            } else {
                SetVoice(guard, 0, 80, 0);
                verbalize("Please follow me.");
            }
        } else {
            if (!umoney) {
                if (Deaf) {
                    if (!Blind)
                        pline("%s glares at you%s.", noit_Monnam(guard),
                              gi.invent ? "r stuff" : "");
                } else {
                   SetVoice(guard, 0, 80, 0);
                   verbalize("You have hidden gold.");
                }
            }
            if (Deaf) {
                if (!Blind)
                    pline(
                       "%s holds out %s palm and beckons with %s other hand.",
                          noit_Monnam(guard), noit_mhis(guard),
                          noit_mhis(guard));
            } else {
                SetVoice(guard, 0, 80, 0);
                verbalize(
                    "Most likely all your gold was stolen from this vault.");
                SetVoice(guard, 0, 80, 0);
                verbalize("Please drop that gold and follow me.");
            }
            EGD(guard)->dropgoldcnt++;
        }
        EGD(guard)->gdx = gdx;
        EGD(guard)->gdy = gdy;
        EGD(guard)->fcbeg = 0;
        EGD(guard)->fakecorr[0].fx = x;
        EGD(guard)->fakecorr[0].fy = y;
        typ = levl[x][y].typ;
        if (!IS_WALL(typ)) {
            /* guard arriving at non-wall implies a door; vault wall was
               dug into an empty doorway (which could subsequently have
               been plugged with an intact door by use of locking magic) */
            int vlt = EGD(guard)->vroom;
            coordxy lowx = gr.rooms[vlt].lx, hix = gr.rooms[vlt].hx;
            coordxy lowy = gr.rooms[vlt].ly, hiy = gr.rooms[vlt].hy;

            if (x == lowx - 1 && y == lowy - 1)
                typ = TLCORNER;
            else if (x == hix + 1 && y == lowy - 1)
                typ = TRCORNER;
            else if (x == lowx - 1 && y == hiy + 1)
                typ = BLCORNER;
            else if (x == hix + 1 && y == hiy + 1)
                typ = BRCORNER;
            else if (y == lowy - 1 || y == hiy + 1)
                typ = HWALL;
            else if (x == lowx - 1 || x == hix + 1)
                typ = VWALL;

            /* we lack access to the original wall_info bit mask for this
               former wall location so recreate it */
            levl[x][y].typ = typ; /* wall; will be changed to door below */
            levl[x][y].wall_info = 0; /* will be reset too via doormask */
            xy_set_wall_state(x, y); /* set WA_MASK bits in .wall_info */
        }
        EGD(guard)->fakecorr[0].ftyp = typ;
        EGD(guard)->fakecorr[0].flags = levl[x][y].flags;
        /* guard's entry point where confrontation with hero takes place */
        levl[x][y].typ = DOOR;
        levl[x][y].doormask = D_NODOOR;
        unblock_point(x, y); /* empty doorway doesn't block light */
        EGD(guard)->fcend = 1;
        EGD(guard)->warncnt = 1;
    }
}

static void
move_gold(struct obj *gold, int vroom)
{
    coordxy nx, ny;

    remove_object(gold);
    newsym(gold->ox, gold->oy);
    nx = gr.rooms[vroom].lx + rn2(2);
    ny = gr.rooms[vroom].ly + rn2(2);
    place_object(gold, nx, ny);
    stackobj(gold);
    newsym(nx, ny);
}

static void
wallify_vault(struct monst *grd)
{
    int typ;
    coordxy x, y;
    int vlt = EGD(grd)->vroom;
    char tmp_viz;
    coordxy lox = gr.rooms[vlt].lx - 1, hix = gr.rooms[vlt].hx + 1,
          loy = gr.rooms[vlt].ly - 1, hiy = gr.rooms[vlt].hy + 1;
    struct monst *mon;
    struct obj *gold, *rocks;
    struct trap *trap;
    boolean fixed = FALSE;
    boolean movedgold = FALSE;

    for (x = lox; x <= hix; x++)
        for (y = loy; y <= hiy; y++) {
            /* if not on the room boundary, skip ahead */
            if (x != lox && x != hix && y != loy && y != hiy)
                continue;

            if ((!IS_WALL(levl[x][y].typ) || g_at(x, y)
                 || sobj_at(ROCK, x, y) || sobj_at(BOULDER, x, y))
                && !in_fcorridor(grd, x, y)) {
                if ((mon = m_at(x, y)) != 0 && mon != grd) {
                    if (mon->mtame)
                        yelp(mon);
                    (void) rloc(mon, RLOC_ERR);
                }
                /* move gold at wall locations into the vault */
                if ((gold = g_at(x, y)) != 0) {
                    move_gold(gold, EGD(grd)->vroom);
                    movedgold = TRUE;
                }
                /* destroy rocks and boulders (subsume them into the walls);
                   other objects present stay intact and become embedded */
                while ((rocks = sobj_at(ROCK, x, y)) != 0) {
                    obj_extract_self(rocks);
                    obfree(rocks, (struct obj *) 0);
                }
                while ((rocks = sobj_at(BOULDER, x, y)) != 0) {
                    obj_extract_self(rocks);
                    obfree(rocks, (struct obj *) 0);
                }
                if ((trap = t_at(x, y)) != 0)
                    deltrap(trap);

                if (x == lox)
                    typ = (y == loy) ? TLCORNER
                          : (y == hiy) ? BLCORNER
                            : VWALL;
                else if (x == hix)
                    typ = (y == loy) ? TRCORNER
                          : (y == hiy) ? BRCORNER
                            : VWALL;
                else /* not left or right side, must be top or bottom */
                    typ = HWALL;

                levl[x][y].typ = typ;
                levl[x][y].wall_info = 0;
                xy_set_wall_state(x, y); /* set WA_MASK bits in .wall_info */
                del_engr_at(x, y);
                /*
                 * hack: player knows walls are restored because of the
                 * message, below, so show this on the screen.
                 */
                tmp_viz = gv.viz_array[y][x];
                gv.viz_array[y][x] = IN_SIGHT | COULD_SEE;
                newsym(x, y);
                gv.viz_array[y][x] = tmp_viz;
                block_point(x, y);
                fixed = TRUE;
            }
        }

    if (movedgold || fixed) {
        if (in_fcorridor(grd, grd->mx, grd->my) || cansee(grd->mx, grd->my))
            pline("%s whispers an incantation.", noit_Monnam(grd));
        else
            You_hear("a distant chant.");
        if (movedgold)
            pline("A mysterious force moves the gold into the vault.");
        if (fixed)
            pline_The("damaged vault's walls are magically restored!");
    }
}

static void
gd_mv_monaway(struct monst *grd, int nx, int ny)
{
    struct monst *mtmp = m_at(nx, ny);

    if (mtmp && mtmp != grd) {
        if (!Deaf) {
            SetVoice(grd, 0, 80, 0);
            verbalize("Out of my way, scum!");
        }
        if (!rloc(mtmp, RLOC_ERR | RLOC_MSG) || MON_AT(nx, ny))
            m_into_limbo(mtmp);
    }
}

/* have guard pick gold off the floor, possibly moving to the gold's
   position before message and back to his current spot after */
static void
gd_pick_corridor_gold(struct monst *grd, int goldx, int goldy)
{
    struct obj *gold;
    coord newcc, bestcc;
    int gdelta, newdelta, bestdelta, tryct,
        guardx = grd->mx, guardy = grd->my;
    boolean under_u = u_at(goldx, goldy),
            see_it = cansee(goldx, goldy);

    if (under_u) {
        /* Grab the gold from between the hero's feet.
           If guard is two or more steps away; bring him closer first. */
        gold = g_at(goldx, goldy);
        if (!gold) {
            impossible("vault guard: no gold at hero's feet?");
            return;
        }
        gdelta = distu(guardx, guardy);
        if (gdelta > 2 && see_it) { /* skip if player won't see it */
            bestdelta = gdelta;
            bestcc.x = (coordxy) guardx, bestcc.y = (coordxy) guardy;
            tryct = 9;
            do {
                /* pick an available spot nearest the hero and also try
                   to find the one meeting that criterium which is nearest
                   the guard's current location */
                if (enexto(&newcc, goldx, goldy, grd->data)) {
                    if ((newdelta = distu(newcc.x, newcc.y)) < bestdelta
                        || (newdelta == bestdelta
                            && dist2(newcc.x, newcc.y, guardx, guardy)
                               < dist2(bestcc.x, bestcc.y, guardx, guardy))) {
                        bestdelta = newdelta;
                        bestcc = newcc;
                    }
                }
            } while (--tryct >= 0);

            if (bestdelta < gdelta) {
                remove_monster(guardx, guardy);
                newsym(guardx, guardy);
                place_monster(grd, (int) bestcc.x, (int) bestcc.y);
                newsym(grd->mx, grd->my);
            }
        }
        obj_extract_self(gold);
        add_to_minv(grd, gold);
        newsym(goldx, goldy);

    /* guard is already at gold's location */
    } else if (goldx == guardx && goldy == guardy) {
        mpickgold(grd); /* does a newsym */

    /* gold is at some third spot, neither guard's nor hero's */
    } else {
        /* just for insurance... */
        gd_mv_monaway(grd, goldx, goldy); /* make room for guard */
        if (see_it) { /* skip if player won't see the message */
            remove_monster(grd->mx, grd->my);
            newsym(grd->mx, grd->my);
            place_monster(grd, goldx, goldy); /* sets <grd->mx, grd->my> */
        }
        mpickgold(grd); /* does a newsym */
    }

    if (see_it) { /* cansee(goldx, goldy) */
        pline("%s%s picks up the gold%s.", Some_Monnam(grd),
              (grd->mpeaceful && EGD(grd)->warncnt > 5)
                 ? " calms down and" : "",
              under_u ? " from beneath you" : "");
    }

    /* if guard was moved to get the gold, move him back */
    if (grd->mx != guardx || grd->my != guardy) {
        remove_monster(grd->mx, grd->my);
        newsym(grd->mx, grd->my);
        place_monster(grd, guardx, guardy);
        newsym(guardx, guardy);
    }
    return;
}


/* return 1: guard moved, -2: died  */
static int
gd_move_cleanup(
    struct monst *grd,
    boolean semi_dead,
    boolean disappear_msg_seen)
{
    int x, y;
    boolean see_guard;

    /*
     * The following is a kludge.  We need to keep the guard around in
     * order to be able to make the fake corridor disappear as the
     * player moves out of it, but we also need the guard out of the
     * way.  We send the guard to never-never land.  We set ogx ogy to
     * mx my in order to avoid a check at the top of this function.
     * At the end of the process, the guard is killed in restfakecorr().
     */
    x = grd->mx, y = grd->my;
    see_guard = canspotmon(grd);
    parkguard(grd); /* move to <0,0> */
    wallify_vault(grd);
    restfakecorr(grd);
    debugpline2("gd_move_cleanup: %scleanup%s",
                grd->isgd ? "" : "final ",
                grd->isgd ? " attempt" : "");
    if (!semi_dead && (in_fcorridor(grd, u.ux, u.uy) || cansee(x, y))) {
        if (!disappear_msg_seen && see_guard)
            pline("Suddenly, %s disappears.", noit_mon_nam(grd));
        return 1;
    }
    return -2;
}

static void
gd_letknow(struct monst *grd)
{
    if (!cansee(grd->mx, grd->my) || !mon_visible(grd))
        You_hear("%s.",
                    m_carrying(grd, TIN_WHISTLE)
                        ? "the shrill sound of a guard's whistle"
                        : "angry shouting");
    else
        You(um_dist(grd->mx, grd->my, 2)
                ? "see %s approaching."
                : "are confronted by %s.",
            /* "an angry guard" */
            x_monnam(grd, ARTICLE_A, "angry", 0, FALSE));
}

/*
 * return  1: guard moved,  0: guard didn't,  -1: let m_move do it,  -2: died
 */
int
gd_move(struct monst *grd)
{
    coordxy x, y, nx, ny, m, n, ex, ey;
    coordxy dx, dy, ggx = 0, ggy = 0, fci;
    uchar typ;
    struct rm *crm;
    struct fakecorridor *fcp;
    struct egd *egrd = EGD(grd);
    long umoney = 0L;
    boolean goldincorridor = FALSE, u_in_vault = FALSE, grd_in_vault = FALSE,
            semi_dead = DEADMONSTER(grd),
            u_carry_gold = FALSE, newspot = FALSE;

    if (!on_level(&(egrd->gdlevel), &u.uz))
        return -1;
    nx = ny = m = n = 0;
    if (semi_dead || !grd->mx || egrd->gddone) {
        egrd->gddone = 1;
        return gd_move_cleanup(grd, semi_dead, FALSE);
    }
    debugpline1("gd_move: %s guard", grd->mpeaceful ? "peaceful" : "hostile");

    u_in_vault = vault_occupied(u.urooms) ? TRUE : FALSE;
    grd_in_vault = *in_rooms(grd->mx, grd->my, VAULT) ? TRUE : FALSE;
    if (!u_in_vault && !grd_in_vault)
        wallify_vault(grd);

    if (!grd->mpeaceful) {
        if (!u_in_vault
            && (grd_in_vault || (in_fcorridor(grd, grd->mx, grd->my)
                                 && !in_fcorridor(grd, u.ux, u.uy)))) {
            (void) rloc(grd, RLOC_MSG);
            wallify_vault(grd);
            if (!in_fcorridor(grd, grd->mx, grd->my))
                (void) clear_fcorr(grd, TRUE);
            gd_letknow(grd);
            return -1;
        }
        if (!in_fcorridor(grd, grd->mx, grd->my))
            (void) clear_fcorr(grd, TRUE);
        return -1;
    }
    if (abs(egrd->ogx - grd->mx) > 1 || abs(egrd->ogy - grd->my) > 1)
        return -1; /* teleported guard - treat as monster */

    if (egrd->witness) {
        if (!Deaf) {
            SetVoice(grd, 0, 80, 0);
            verbalize("How dare you %s that gold, scoundrel!",
                      (egrd->witness & GD_EATGOLD) ? "consume" : "destroy");
        }
        egrd->witness = 0;
        grd->mpeaceful = 0;
        return -1;
    }

    umoney = money_cnt(gi.invent);
    u_carry_gold = (umoney > 0L || hidden_gold(TRUE) > 0L);
    if (egrd->fcend == 1) {
        if (u_in_vault && (u_carry_gold || um_dist(grd->mx, grd->my, 1))) {
            if (egrd->warncnt == 3 && !Deaf) {
                char buf[BUFSZ];

                Sprintf(buf, "%sfollow me!",
                        u_carry_gold ? (!umoney ? "drop that hidden gold and "
                                                : "drop that gold and ")
                                     : "");
                SetVoice(grd, 0, 80, 0);
                if (egrd->dropgoldcnt || !u_carry_gold)
                    verbalize("I repeat, %s", buf);
                else
                    verbalize("%s", upstart(buf));
                if (u_carry_gold)
                    egrd->dropgoldcnt++;
            }
            if (egrd->warncnt == 7) {
                m = grd->mx;
                n = grd->my;
                if (!Deaf) {
                    SetVoice(grd, 0, 80, 0);
                    verbalize("You've been warned, knave!");
                }
                grd->mpeaceful = 0;
                mnexto(grd, RLOC_NOMSG);
                levl[m][n].typ = egrd->fakecorr[0].ftyp;
                levl[m][n].flags = egrd->fakecorr[0].flags;
                del_engr_at(m, n);
                newsym(m, n);
                return -1;
            }
            /* not fair to get mad when (s)he's fainted or paralyzed */
            if (!is_fainted() && gm.multi >= 0)
                egrd->warncnt++;
            return 0;
        }

        if (!u_in_vault) {
            if (u_carry_gold) { /* player teleported */
                m = grd->mx;
                n = grd->my;
                (void) rloc(grd, RLOC_MSG);
                levl[m][n].typ = egrd->fakecorr[0].ftyp;
                levl[m][n].flags = egrd->fakecorr[0].flags;
                del_engr_at(m, n);
                newsym(m, n);
                grd->mpeaceful = 0;
                gd_letknow(grd);
                return -1;
            } else {
                if (!Deaf) {
                    SetVoice(grd, 0, 80, 0);
                    verbalize("Well, begone.");
                }
                egrd->gddone = 1;
                return gd_move_cleanup(grd, semi_dead, FALSE);
            }
        }
    }

    if (egrd->fcend > 1) {
        if (egrd->fcend > 2 && in_fcorridor(grd, grd->mx, grd->my)
            && !egrd->gddone && !in_fcorridor(grd, u.ux, u.uy)
            && (levl[egrd->fakecorr[0].fx][egrd->fakecorr[0].fy].typ
                == egrd->fakecorr[0].ftyp)) {
            pline("%s, confused, disappears.", noit_Monnam(grd));
            return gd_move_cleanup(grd, semi_dead, TRUE);
        }
        if (u_carry_gold && (in_fcorridor(grd, u.ux, u.uy)
                             /* cover a 'blind' spot */
                             || (egrd->fcend > 1 && u_in_vault))) {
            if (!grd->mx) {
                restfakecorr(grd);
                return -2;
            }
            if (egrd->warncnt < 6) {
                egrd->warncnt = 6;
                if (Deaf) {
                    if (!Blind)
                        pline("%s holds out %s palm demandingly!",
                              noit_Monnam(grd), noit_mhis(grd));
                } else {
                    SetVoice(grd, 0, 80, 0);
                    verbalize("Drop all your gold, scoundrel!");
                }
                return 0;
            } else {
                if (Deaf) {
                    if (!Blind)
                        pline("%s rubs %s hands with enraged delight!",
                              noit_Monnam(grd), noit_mhis(grd));
                } else {
                    SetVoice(grd, 0, 80, 0);
                    verbalize("So be it, rogue!");
                }
                grd->mpeaceful = 0;
                return -1;
            }
        }
    }
    for (fci = egrd->fcbeg; fci < egrd->fcend; fci++)
        if (g_at(egrd->fakecorr[fci].fx, egrd->fakecorr[fci].fy)) {
            m = egrd->fakecorr[fci].fx;
            n = egrd->fakecorr[fci].fy;
            goldincorridor = TRUE;
            break;
        }
    /* new gold can appear if it was embedded in stone and hero kicks it
       (on even via wish and drop) so don't assume hero has been warned */
    if (goldincorridor && !egrd->gddone) {
        gd_pick_corridor_gold(grd, m, n);
        if (!grd->mpeaceful)
            return -1;
        egrd->warncnt = 5;
        return 0;
    }
    if (um_dist(grd->mx, grd->my, 1) || egrd->gddone) {
        if (!egrd->gddone && !rn2(10) && !Deaf && !u.uswallow
            && !(u.ustuck && !sticks(gy.youmonst.data))) {
            SetVoice(grd, 0, 80, 0);
            verbalize("Move along!");
        }
        restfakecorr(grd);
        return 0; /* didn't move */
    }
    x = grd->mx;
    y = grd->my;

    if (u_in_vault)
        goto nextpos;

    /* look around (hor & vert only) for accessible places */
    for (nx = x - 1; nx <= x + 1; nx++)
        for (ny = y - 1; ny <= y + 1; ny++) {
            if ((nx == x || ny == y) && (nx != x || ny != y)
                && isok(nx, ny)) {
                crm = &levl[nx][ny];
                typ = crm->typ;
                if (!IS_STWALL(typ) && !IS_POOL(typ)) {
                    if (in_fcorridor(grd, nx, ny))
                        goto nextnxy;

                    if (*in_rooms(nx, ny, VAULT))
                        continue;

                    /* seems we found a good place to leave him alone */
                    egrd->gddone = 1;
                    if (ACCESSIBLE(typ))
                        goto newpos;
                    crm->typ = (typ == SCORR) ? CORR : DOOR;
                    if (crm->typ == DOOR)
                        crm->doormask = D_NODOOR;
                    else
                        crm->flags = 0;
                    del_engr_at(nx, ny);
                    goto proceed;
                }
            }
 nextnxy:
            ;
        }
 nextpos:
    nx = x;
    ny = y;
    ggx = egrd->gdx;
    ggy = egrd->gdy;
    dx = (ggx > x) ? 1 : (ggx < x) ? -1 : 0;
    dy = (ggy > y) ? 1 : (ggy < y) ? -1 : 0;
    if (abs(ggx - x) >= abs(ggy - y))
        nx += dx;
    else
        ny += dy;

    while ((typ = (crm = &levl[nx][ny])->typ) != STONE) {
        ex = nx + nx - x;
        ey = ny + ny - y;
        /* in view of the above we must have IS_WALL(typ) or typ == POOL */
        /* must be a wall here */
        if (isok(ex, ey) && IS_ROOM(levl[ex][ey].typ)) {
            crm->typ = DOOR;
            crm->doormask = D_NODOOR;
            del_engr_at(ex, ey);
            goto proceed;
        }
        if (dy && nx != x) {
            nx = x;
            ny = y + dy;
            continue;
        }
        if (dx && ny != y) {
            ny = y;
            nx = x + dx;
            dy = 0;
            continue;
        }
        /* I don't like this, but ... */
        if (IS_ROOM(typ)) {
            crm->typ = DOOR;
            crm->doormask = D_NODOOR;
            del_engr_at(ex, ey);
            goto proceed;
        }
        break;
    }
    crm->typ = CORR;
    crm->flags = 0;
 proceed:
    newspot = TRUE;
    unblock_point(nx, ny); /* doesn't block light */
    if (cansee(nx, ny))
        newsym(nx, ny);

    if ((nx != ggx || ny != ggy) || (grd->mx != ggx || grd->my != ggy)) {
        fcp = &(egrd->fakecorr[egrd->fcend]);
        /* fakecorr overflow does not occur because egrd->fakecorr[]
           is too small, but it has occurred when the same <x,y> are
           put into it repeatedly for some as yet unexplained reason */
        if (egrd->fcend++ == FCSIZ)
            panic("fakecorr overflow");
        fcp->fx = nx;
        fcp->fy = ny;
        fcp->ftyp = typ;
        fcp->flags = crm->flags;
    } else if (!egrd->gddone) {
        /* We're stuck, so try to find a new destination. */
        if (!find_guard_dest(grd, &egrd->gdx, &egrd->gdy)
            || (egrd->gdx == ggx && egrd->gdy == ggy)) {
            pline("%s, confused, disappears.", Monnam(grd));
            return gd_move_cleanup(grd, semi_dead, TRUE);
        } else
            goto nextpos;
    }
 newpos:
    gd_mv_monaway(grd, nx, ny);
    if (egrd->gddone)
        return gd_move_cleanup(grd, semi_dead, FALSE);
    egrd->ogx = grd->mx; /* update old positions */
    egrd->ogy = grd->my;
    remove_monster(grd->mx, grd->my);
    place_monster(grd, nx, ny);
    if (newspot && g_at(nx, ny)) {
        /* if there's gold already here (most likely from mineralize()),
           pick it up now so that guard doesn't later think hero dropped
           it and give an inappropriate message */
        mpickgold(grd);
        if (canspotmon(grd))
            pline("%s picks up some gold.", Monnam(grd));
    } else
        newsym(grd->mx, grd->my);
    restfakecorr(grd);
    return 1;
}

/* Routine when dying or quitting with a vault guard around */
void
paygd(boolean silently)
{
    struct monst *grd = findgd();
    long umoney = money_cnt(gi.invent);
    struct obj *coins, *nextcoins;
    int gdx, gdy;
    char buf[BUFSZ];

    if (!umoney || !grd)
        return;

    if (u.uinvault) {
        if (!silently)
            Your("%ld %s goes into the Magic Memory Vault.",
                 umoney, currency(umoney));
        gdx = u.ux;
        gdy = u.uy;
    } else {
        if (grd->mpeaceful) /* peaceful guard has no "right" to your gold */
            goto remove_guard;

        mnexto(grd, RLOC_NOMSG);
        if (!silently)
            pline("%s remits your gold to the vault.", Monnam(grd));
        gdx = gr.rooms[EGD(grd)->vroom].lx + rn2(2);
        gdy = gr.rooms[EGD(grd)->vroom].ly + rn2(2);
        Sprintf(buf, "To Croesus: here's the gold recovered from %s the %s.",
                gp.plname,
                pmname(&mons[u.umonster], flags.female ? FEMALE : MALE));
        make_grave(gdx, gdy, buf);
    }
    for (coins = gi.invent; coins; coins = nextcoins) {
        nextcoins = coins->nobj;
        if (objects[coins->otyp].oc_class == COIN_CLASS) {
            freeinv(coins);
            place_object(coins, gdx, gdy);
            stackobj(coins);
        }
    }
 remove_guard:
    mongone(grd);
    return;
}

/*
 * amount of gold in carried containers
 *
 * even_if_unknown:
 *   True:  all gold
 *   False: limit to known contents
 */
long
hidden_gold(boolean even_if_unknown)
{
    long value = 0L;
    struct obj *obj;

    for (obj = gi.invent; obj; obj = obj->nobj)
        if (Has_contents(obj) && (obj->cknown || even_if_unknown))
            value += contained_gold(obj, even_if_unknown);
    /* unknown gold stuck inside statues may cause some consternation... */

    return value;
}

/* prevent "You hear footsteps.." when inappropriate */
boolean
gd_sound(void)
{
    return !(vault_occupied(u.urooms) || findgd());
}

void
vault_gd_watching(unsigned int activity)
{
    struct monst *guard = findgd();

    if (guard && guard->mx && guard->mcansee && m_canseeu(guard)) {
        if (activity == GD_EATGOLD || activity == GD_DESTROYGOLD)
            EGD(guard)->witness = activity;
    }
}

/*vault.c*/

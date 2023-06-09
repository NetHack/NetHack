/* NetHack 3.7	teleport.c	$NHDT-Date: 1685863331 2023/06/04 07:22:11 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.206 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#define NEW_ENEXTO

static boolean goodpos_onscary(coordxy, coordxy, struct permonst *);
static boolean tele_jump_ok(coordxy, coordxy, coordxy, coordxy);
static boolean teleok(coordxy, coordxy, boolean);
static void vault_tele(void);
static boolean rloc_pos_ok(coordxy, coordxy, struct monst *);
static void rloc_to_core(struct monst *, coordxy, coordxy, unsigned);
static void mvault_tele(struct monst *);
static boolean m_blocks_teleporting(struct monst *);

/* does monster block others from teleporting? */
static boolean
m_blocks_teleporting(struct monst *mtmp)
{
    if (is_dlord(mtmp->data) || is_dprince(mtmp->data))
        return TRUE;
    return FALSE;
}

/* teleporting is prevented on this level for this monster? */
boolean
noteleport_level(struct monst* mon)
{
    /* demon court in Gehennom prevent others from teleporting */
    if (In_hell(&u.uz) && !(is_dlord(mon->data) || is_dprince(mon->data)))
        if (get_iter_mons(m_blocks_teleporting))
            return TRUE;

    /* natural no-teleport level */
    if (gl.level.flags.noteleport)
        return TRUE;

    return FALSE;
}

/* this is an approximation of onscary() that doesn't use any 'struct monst'
   fields aside from 'monst->data'; used primarily for new monster creation
   and monster teleport destination, not for ordinary monster movement */
static boolean
goodpos_onscary(
    coordxy x, coordxy y,
    struct permonst *mptr)
{
    /* onscary() checks Angels and lawful minions; this oversimplifies */
    if (mptr->mlet == S_HUMAN || mptr->mlet == S_ANGEL
        || is_rider(mptr) || unique_corpstat(mptr))
        return FALSE;
    /* onscary() checks for vampshifted vampire bats/fog clouds/wolves too */
    if (IS_ALTAR(levl[x][y].typ) && mptr->mlet == S_VAMPIRE)
        return TRUE;
    /* scare monster scroll doesn't have any of the below restrictions,
       being its own source of power */
    if (sobj_at(SCR_SCARE_MONSTER, x, y))
        return TRUE;
    /* engraved Elbereth doesn't work in Gehennom or the end-game */
    if (Inhell || In_endgame(&u.uz))
        return FALSE;
    /* creatures who don't (or can't) fear a written Elbereth and weren't
       caught by the minions check */
    if (mptr == &mons[PM_MINOTAUR] || !haseyes(mptr))
        return FALSE;
    return sengr_at("Elbereth", x, y, TRUE) ? TRUE : FALSE;
}

/*
 * Is (x,y) a good position of mtmp?  If mtmp is NULL, then is (x,y) good
 * for an object?
 *
 * This function will only look at mtmp->mdat, so makemon, mplayer, etc can
 * call it to generate new monster positions with fake monster structures.
 */
boolean
goodpos(
    coordxy x, coordxy y,
    struct monst *mtmp,
    mmflags_nht gpflags)
{
    struct permonst *mdat = (struct permonst *) 0;
    boolean ignorewater = ((gpflags & MM_IGNOREWATER) != 0),
            ignorelava = ((gpflags & MM_IGNORELAVA) != 0),
            checkscary = ((gpflags & GP_CHECKSCARY) != 0),
            allow_u = ((gpflags & GP_ALLOW_U) != 0);

    if (!isok(x, y))
        return FALSE;

    /* in many cases, we're trying to create a new monster, which
     * can't go on top of the player or any existing monster.
     * however, occasionally we are relocating engravings or objects,
     * which could be co-located and thus get restricted a bit too much.
     * oh well.
     */
    if (!allow_u) {
        if (u_at(x, y) && mtmp != &gy.youmonst
            && (mtmp != u.ustuck || !u.uswallow)
            && (!u.usteed || mtmp != u.usteed))
            return FALSE;
    }

    if (mtmp) {
        struct monst *mtmp2 = m_at(x, y);

        /* Be careful with long worms.  A monster may be placed back in
         * its own location.  Normally, if m_at() returns the same monster
         * that we're trying to place, the monster is being placed in its
         * own location.  However, that is not correct for worm segments,
         * because all the segments of the worm return the same m_at().
         * Actually we overdo the check a little bit--a worm can't be placed
         * in its own location, period.  If we just checked for mtmp->mx
         * != x || mtmp->my != y, we'd miss the case where we're called
         * to place the worm segment and the worm's head is at x,y.
         */
        if (mtmp2 && (mtmp2 != mtmp || mtmp->wormno))
            return FALSE;

        mdat = mtmp->data;
        if (is_pool(x, y) && !ignorewater) {
            /* [what about Breathless?] */
            if (mtmp == &gy.youmonst)
                return (Swimming || Amphibious
                        || (!Is_waterlevel(&u.uz)
                            && !is_waterwall(x, y)
                            /* water on the Plane of Water has no surface
                               so there's no way to be on or above that */
                            && (Levitation || Flying || Wwalking)));
            else
                return (is_swimmer(mdat)
                        || (!Is_waterlevel(&u.uz)
                            && !is_waterwall(x, y)
                            && m_in_air(mtmp)));
        } else if (mdat->mlet == S_EEL && rn2(13) && !ignorewater) {
            return FALSE;
        } else if (is_lava(x, y) && !ignorelava) {
            /* 3.6.3: floating eye can levitate over lava but it avoids
               that due the effect of the heat causing it to dry out */
            if (mdat == &mons[PM_FLOATING_EYE])
                return FALSE;
            else if (mtmp == &gy.youmonst)
                return (Levitation || Flying
                        || (Fire_resistance && Wwalking && uarmf
                            && uarmf->oerodeproof)
                        || (Upolyd && likes_lava(gy.youmonst.data)));
            else
                return (m_in_air(mtmp) || likes_lava(mdat));
        }
        if (passes_walls(mdat) && may_passwall(x, y))
            return TRUE;
        if (amorphous(mdat) && closed_door(x, y))
            return TRUE;
        /* avoid onscary() if caller has specified that restriction */
        if (checkscary && (mtmp->m_id ? onscary(x, y, mtmp)
                                      : goodpos_onscary(x, y, mdat)))
            return FALSE;
    }
    if (!accessible(x, y)) {
        if (!(is_pool(x, y) && ignorewater)
            && !(is_lava(x, y) && ignorelava))
            return FALSE;
    }
    /* skip boulder locations for most creatures */
    if (sobj_at(BOULDER, x, y) && (!mdat || !throws_rocks(mdat)))
        return FALSE;

    return TRUE;
}

/*
 * "entity next to"
 *
 * Attempt to find a good place for the given monster type in the closest
 * position to (xx,yy).  Do so in successive square rings around (xx,yy).
 * If there is more than one valid position in the ring, choose one randomly.
 * Return TRUE and the position chosen when successful, FALSE otherwise.
 */
boolean
enexto(
    coord *cc,
    coordxy xx, coordxy yy,
    struct permonst *mdat)
{
    return (enexto_core(cc, xx, yy, mdat, GP_CHECKSCARY)
            || enexto_core(cc, xx, yy, mdat, NO_MM_FLAGS));
}

#ifdef NEW_ENEXTO

boolean
enexto_core(
    coord *cc, /* output; <cc.x,cc.y> as close as feasible to <xx,yy> */
    coordxy xx, coordxy yy, /* input coordinates */
    struct permonst *mdat,  /* type of monster; affects whether water or
                             * lava or boulder spots will be considered */
    mmflags_nht entflags)   /* flags for goodpos() */
{
    coord candy[ROWNO * (COLNO - 1)]; /* enough room for every location */
    int i, nearcandyct, allcandyct;
    struct monst fakemon; /* dummy monster */
    boolean allow_xx_yy = (boolean) ((entflags & GP_ALLOW_XY) != 0);
    /* note: GP_ALLOW_XY isn't used by goodpos(); old enext_core() used to
       mask it off to hide it from goodpos but that isn't required and we
       want to keep it in case the debugpline4() gets called */

    if (!mdat) {
        debugpline0("enexto() called with null mdat");
        /* default to player's original monster type */
        mdat = &mons[u.umonster];
    }
    fakemon = cg.zeromonst;
    set_mon_data(&fakemon, mdat); /* set up for goodpos() */

    /* gather candidate coordinates within 3 steps, those 1 step away in
       random order first, then those 2 steps away in random order, then 3;
       this will usually find a good spot without scanning the whole map */
    nearcandyct = collect_coords(candy, xx, yy, 3, CC_NO_FLAGS,
                                 (boolean (*)(coordxy, coordxy)) 0);
    for (i = 0; i < nearcandyct; ++i) {
        *cc = candy[i];
        if (goodpos(cc->x, cc->y, &fakemon, entflags))
            return TRUE;
    }

    /* didn't find a spot; gather coordinates for the whole map except
       for <xx,yy> itself, ordered in expanding distance from <xx,yy>
       (subsets of equal distance grouped together with order randomized) */
    allcandyct = collect_coords(candy, xx, yy, 0, CC_NO_FLAGS,
                                (boolean (*)(coordxy, coordxy)) 0);
    /* skip first 'nearcandyct' spots, they have already been rejected;
       they will occur in different random order but same overall total */
    for (i = nearcandyct; i < allcandyct; ++i) {
        *cc = candy[i];
        if (goodpos(cc->x, cc->y, &fakemon, entflags))
            return TRUE;
    }

    /* still didn't find a spot; maybe try <xx,yy> itself */
    cc->x = xx, cc->y = yy; /* final value for 'cc' in case we return False */
    if (allow_xx_yy && goodpos(cc->x, cc->y, &fakemon, entflags))
        return TRUE;

    /* failed to find any acceptable spot */
    debugpline4("enexto(\"%s\",%d,%d,0x%08lx) failed",
                mdat->pmnames[NEUTRAL], (int) xx, (int) yy,
                (unsigned long) entflags);
    return FALSE;
}

#else   /* !NEW_ENEXTO */

boolean
enexto_core(
    coord *cc,
    coordxy xx, coordxy yy,
    struct permonst *mdat,
    mmflags_nht entflags)
{
#define MAX_GOOD 15
    coord good[MAX_GOOD], *good_ptr;
    coordxy x, y, range, i;
    coordxy xmin, xmax, ymin, ymax, rangemax;
    struct monst fakemon; /* dummy monster */
    boolean allow_xx_yy = (boolean) ((entflags & GP_ALLOW_XY) != 0);

    entflags &= ~GP_ALLOW_XY;
    if (!mdat) {
        debugpline0("enexto() called with null mdat");
        /* default to player's original monster type */
        mdat = &mons[u.umonster];
    }
    fakemon = cg.zeromonst;
    set_mon_data(&fakemon, mdat); /* set up for goodpos */

    /* used to use 'if (range > ROWNO && range > COLNO) return FALSE' below,
       so effectively 'max(ROWNO, COLNO)' which performs useless iterations
       (possibly many iterations if <xx,yy> is in the center of the map) */
    xmax = max(xx - 1, (COLNO - 1) - xx);
    ymax = max(yy - 0, (ROWNO - 1) - yy);
    rangemax = max(xmax, ymax);
    /* setup: no suitable spots yet, first iteration checks adjacent spots */
    good_ptr = good;
    range = 1;
    /*
     * Walk around the border of the square with center (xx,yy) and
     * radius range.  Stop when we find at least one valid position.
     */
    do {
        xmin = max(1, xx - range);
        xmax = min(COLNO - 1, xx + range);
        ymin = max(0, yy - range);
        ymax = min(ROWNO - 1, yy + range);

        for (x = xmin; x <= xmax; x++) {
            if (goodpos(x, ymin, &fakemon, entflags)) {
                good_ptr->x = x;
                good_ptr->y = ymin;
                /* beware of accessing beyond segment boundaries.. */
                if (good_ptr++ == &good[MAX_GOOD - 1])
                    goto full;
            }
            if (goodpos(x, ymax, &fakemon, entflags)) {
                good_ptr->x = x;
                good_ptr->y = ymax;
                if (good_ptr++ == &good[MAX_GOOD - 1])
                    goto full;
            }
        }
        /* 3.6.3: this used to use 'ymin+1' which left top row unchecked */
        for (y = ymin; y < ymax; y++) {
            if (goodpos(xmin, y, &fakemon, entflags)) {
                good_ptr->x = xmin;
                good_ptr->y = y;
                if (good_ptr++ == &good[MAX_GOOD - 1])
                    goto full;
            }
            if (goodpos(xmax, y, &fakemon, entflags)) {
                good_ptr->x = xmax;
                good_ptr->y = y;
                if (good_ptr++ == &good[MAX_GOOD - 1])
                    goto full;
            }
        }
    } while (++range <= rangemax && good_ptr == good);

    /* return False if we exhausted 'range' without finding anything */
    if (good_ptr == good) {
        /* 3.6.3: earlier versions didn't have the option to try <xx,yy>,
           and left 'cc' uninitialized when returning False */
        cc->x = xx, cc->y = yy;
        /* if every spot other than <xx,yy> has failed, try <xx,yy> itself */
        if (allow_xx_yy && goodpos(xx, yy, &fakemon, entflags)) {
            return TRUE; /* 'cc' is set */
        } else {
            debugpline3("enexto(\"%s\",%d,%d) failed",
                        mdat->pmnames[NEUTRAL], (int) xx, (int) yy);
            return FALSE;
        }
    }

 full:
    /* we've got between 1 and SIZE(good) candidates; choose one */
    i = (coordxy) rn2((int) (good_ptr - good));
    cc->x = good[i].x;
    cc->y = good[i].y;
    return TRUE;
#undef MAX_GOOD
}

#endif /* ?NEW_ENEXTO */

/*
 * Check for restricted areas present in some special levels.  (This might
 * need to be augmented to allow deliberate passage in wizard mode, but
 * only for explicitly chosen destinations.)
 */
static boolean
tele_jump_ok(coordxy x1, coordxy y1, coordxy x2, coordxy y2)
{
    if (!isok(x2, y2))
        return FALSE;
    if (gd.dndest.nlx > 0) {
        /* if inside a restricted region, can't teleport outside */
        if (within_bounded_area(x1, y1, gd.dndest.nlx, gd.dndest.nly,
                                gd.dndest.nhx, gd.dndest.nhy)
            && !within_bounded_area(x2, y2, gd.dndest.nlx, gd.dndest.nly,
                                    gd.dndest.nhx, gd.dndest.nhy))
            return FALSE;
        /* and if outside, can't teleport inside */
        if (!within_bounded_area(x1, y1, gd.dndest.nlx, gd.dndest.nly,
                                 gd.dndest.nhx, gd.dndest.nhy)
            && within_bounded_area(x2, y2, gd.dndest.nlx, gd.dndest.nly,
                                   gd.dndest.nhx, gd.dndest.nhy))
            return FALSE;
    }
    if (gu.updest.nlx > 0) { /* ditto */
        if (within_bounded_area(x1, y1, gu.updest.nlx, gu.updest.nly,
                                gu.updest.nhx, gu.updest.nhy)
            && !within_bounded_area(x2, y2, gu.updest.nlx, gu.updest.nly,
                                    gu.updest.nhx, gu.updest.nhy))
            return FALSE;
        if (!within_bounded_area(x1, y1, gu.updest.nlx, gu.updest.nly,
                                 gu.updest.nhx, gu.updest.nhy)
            && within_bounded_area(x2, y2, gu.updest.nlx, gu.updest.nly,
                                   gu.updest.nhx, gu.updest.nhy))
            return FALSE;
    }
    return TRUE;
}

static boolean
teleok(coordxy x, coordxy y, boolean trapok)
{
    if (!trapok) {
        /* allow teleportation onto vibrating square, it's not a real trap;
           also allow pits and holes if levitating or flying */
        struct trap *trap = t_at(x, y);

        if (!trap)
            trapok = TRUE;
        else if (trap->ttyp == VIBRATING_SQUARE)
            trapok = TRUE;
        else if ((is_pit(trap->ttyp) || is_hole(trap->ttyp))
                 && (Levitation || Flying))
            trapok = TRUE;

        if (!trapok)
            return FALSE;
    }
    if (!goodpos(x, y, &gy.youmonst, 0))
        return FALSE;
    if (!tele_jump_ok(u.ux, u.uy, x, y))
        return FALSE;
    if (!in_out_region(x, y))
        return FALSE;
    return TRUE;
}

void
teleds(coordxy nux, coordxy nuy, int teleds_flags)
{
    unsigned was_swallowed;
    boolean ball_active, ball_still_in_range = FALSE,
            allow_drag = (teleds_flags & TELEDS_ALLOW_DRAG) != 0,
            is_teleport = (teleds_flags & TELEDS_TELEPORT) != 0;
    struct monst *vault_guard = vault_occupied(u.urooms) ? findgd() : 0;

    if (u.utraptype == TT_BURIEDBALL) {
        /* unearth it */
        buried_ball_to_punishment();
    }
    ball_active = (Punished && uball->where != OBJ_FREE);
    if (!ball_active
        || near_capacity() > SLT_ENCUMBER
        || distmin(u.ux, u.uy, nux, nuy) > 1)
        allow_drag = FALSE;

    /* If they have to move the ball, then drag if allow_drag is true;
     * otherwise they are teleporting, so unplacebc().
     * If they don't have to move the ball, then always "drag" whether or
     * not allow_drag is true, because we are calling that function, not
     * to drag, but to move the chain.  *However* there are some dumb
     * special cases:
     *    0                          0
     *   _X  move east       ----->  X_
     *    @                           @
     * These are permissible if teleporting, but not if dragging.  As a
     * result, drag_ball() needs to know about allow_drag and might end
     * up dragging the ball anyway.  Also, drag_ball() might find that
     * dragging the ball is completely impossible (ball in range but there's
     * rock in the way), in which case it teleports the ball on its own.
     */
    if (ball_active) {
        if (!carried(uball) && distmin(nux, nuy, uball->ox, uball->oy) <= 2)
            ball_still_in_range = TRUE; /* don't have to move the ball */
        else if (!allow_drag)
            unplacebc(); /* have to move the ball */
    }
    reset_utrap(FALSE);
    was_swallowed = u.uswallow; /* set_ustuck(Null) clears uswallow */
    set_ustuck((struct monst *) 0);
    u.ux0 = u.ux;
    u.uy0 = u.uy;

    if (!hideunder(&gy.youmonst) && gy.youmonst.data->mlet == S_MIMIC) {
        /* mimics stop being unnoticed */
        gy.youmonst.m_ap_type = M_AP_NOTHING;
    }

    if (was_swallowed) {
        if (Punished) { /* ball&chain are off map while swallowed */
            ball_active = TRUE; /* to put chain and non-carried ball on map */
            ball_still_in_range = allow_drag = FALSE; /* (redundant) */
        }
        docrt();
    }
    if (ball_active && (ball_still_in_range || allow_drag)) {
        int bc_control;
        coordxy ballx, bally, chainx, chainy;
        boolean cause_delay;

        if (drag_ball(nux, nuy, &bc_control, &ballx, &bally, &chainx,
                      &chainy, &cause_delay, allow_drag))
            move_bc(0, bc_control, ballx, bally, chainx, chainy);
        else {
            /* dragging fails if hero is encumbered beyond 'burdened' */
            /* uball might've been cleared via drag_ball -> spoteffects ->
               dotrap -> magic trap unpunishment */
            ball_active = (Punished && uball->where != OBJ_FREE);
            if (ball_active)
                unplacebc(); /* to match placebc() below */
        }
    }

    /* must set u.ux, u.uy after drag_ball(), which may need to know
       the old position if allow_drag is true... */
    u_on_newpos(nux, nuy); /* set u.<x,y>, usteed-><mx,my>; cliparound() */
    fill_pit(u.ux0, u.uy0);
    if (ball_active && uchain && uchain->where == OBJ_FREE)
        placebc(); /* put back the ball&chain if they were taken off map */
    initrack(); /* teleports mess up tracking monsters without this */
    update_player_regions();
    /*
     *  Make sure the hero disappears from the old location.  This will
     *  not happen if she is teleported within sight of her previous
     *  location.  Force a full vision recalculation because the hero
     *  is now in a new location.
     */
    newsym(u.ux0, u.uy0);
    see_monsters();
    gv.vision_full_recalc = 1;
    nomul(0);
    vision_recalc(0); /* vision before effects */

    /* this used to take place sooner, but if a --More-- prompt was issued
       then the old map display was shown instead of the new one */
    if (is_teleport && Verbose(2, teleds))
        You("materialize in %s location!",
            (nux == u.ux0 && nuy == u.uy0) ? "the same" : "a different");
    /* if terrain type changes, levitation or flying might become blocked
       or unblocked; might issue message, so do this after map+vision has
       been updated for new location instead of right after u_on_newpos() */
    if (levl[u.ux][u.uy].typ != levl[u.ux0][u.uy0].typ)
        switch_terrain();
    /* sequencing issue:  we want guard's alarm, if any, to occur before
       room entry message, if any, so need to check for vault exit prior
       to spoteffects; but spoteffects() sets up new value for u.urooms
       and vault code depends upon that value, so we need to fake it */
    if (vault_guard) {
        char save_urooms[5]; /* [sizeof u.urooms] */

        Strcpy(save_urooms, u.urooms);
        Strcpy(u.urooms, in_rooms(u.ux, u.uy, VAULT));
        /* if hero has left vault, make guard notice */
        if (!vault_occupied(u.urooms))
            uleftvault(vault_guard);
        Strcpy(u.urooms, save_urooms); /* reset prior to spoteffects() */
    }
    /* possible shop entry message comes after guard's shrill whistle */
    spoteffects(TRUE);
    invocation_message();
    return;
}

/* make a list of coordinates in expanding distance from <cx,cy>;
   return value is number of coordinates inserted into ccc[]  */
int
collect_coords(
    coord *ccc, /* pointer to array of at least size ROWNO*(COLNO-1) */
    coordxy cx, coordxy cy, /* center point, not necessarly <u.ux,u.uy> */
    int maxradius,          /* how far from center to go collecting spots;
                             * 0 means collect entire map */
    unsigned cc_flags,      /* incl_center: put <cx,cy> in output list
                             * (provided that it passes filter, if any);
                             * unshuffled: keep output in collection order;
                             * ring_pairs: shuffle pairs of rings together
                             * instead of keeping each ring distinct;
                             * skip_mons: reject occupied spots;
                             * skip_inaccs: reject !ZAP_POS() spots */
    boolean (*filter)(coordxy, coordxy)) /* if Null, no filtering */
{
    coordxy x, y, lox, hix, loy, hiy;
    int radius, rowrange, colrange, k, n = 0;
    coord cc, *passcc = NULL;
    boolean newpass, passend,
            /* is <cx,cy> a candidate? */
            include_cxcy = (cc_flags & CC_INCL_CENTER) != 0,
            /* flag is negative; turn local variable into positive */
            scramble = (cc_flags & CC_UNSHUFFLED) == 0,
            /* if scrambling, shuffle rings 1+2, 3+4, &c together */
            ring_pairs = (scramble && (cc_flags & CC_RING_PAIRS) != 0),
            /* exclude locations containing monsters from output */
            skip_mons = (cc_flags & CC_SKIP_MONS) != 0,
            /* exclude !ZAP_POS() locations from output; allows pools+lava */
            skip_inaccessible = (cc_flags & CC_SKIP_INACCS) != 0;
    int result = 0;

    rowrange = (cy < ROWNO / 2) ? (ROWNO - 1 - cy) : cy;
    colrange = (cx < COLNO / 2) ? (COLNO - 1 - cx) : cx;
    k = max(rowrange, colrange);
    /* if no radius limit has been specified, cover the whole map */
    if (!maxradius)
        maxradius = k;
    else
        maxradius = min(maxradius, k);
    /*
     * We operate on an expanding radius around the center, optionally
     * starting with the center spot itself, and shuffle the edges of
     * each expanding square or "ring".  (So all 1's are shuffled at
     * end of the pass for radius==1, then all 2's at end of radius==2,
     * and so on.  Shuffling of each ring doesn't encroach on any of
     * the others except when ring_pairs mode is specified.)
     *
     * Diagram of first three rings (four if 'include_cxcy' is specified)
     *   rings       unshuffled output      sample shuffled output (varies)
     *  3333333     25 26  .  .  .  . 31     33 29  .  .  .  . 44
     *  3222223     32  9 10 11 12 13 33     35 22 16 14 24 13 40
     *  3211123     34 14  1  2  3 15  .     38 20  2  8  3 15  .
     *  3210123     .  16  4  0  5 17  .     .  11  1  0  6  9  .
     *  3211123     .  18  6  7  8 19 39     .  19  5  7  4 12 25
     *  3222223     40 20 21 22 23 24 41     43 10 21 17 23 18 27
     *  3333333     42  .  .  .  . 47 48     37  .  .  .  . 30 36
     * . == entry not shown to reduce clutter when viewing inner portion.
     *
     * The digits under 'rings' are ring number, which is also distmin
     * from center, indicating the order in which sets of spots are
     * evaluated.  Output gets collected in expanding rings.  For the
     * two output grids, the number shown represents the position in the
     * returned list of coordinates.  When shuffling while ring_pairs is
     * specified, the 1's and 2's will be mixed together, the 3's and
     * (unshown) 4's will be mixed together, and so forth.
     *
     * If caller processes the output list in order, the closest viable
     * spot will be chosen.  If a compeletely random spot is preferred,
     * the list can be requested to be unscrambled and then the caller
     * can shuffle it, overriding the collection rings.  A filter function
     * could be used to skip everything after the first acceptable spot.
     * 'ring_pairs' mode allows for choosing a very close spot that isn't
     * immediately adjacent to the center point, useful for emergency
     * teleport to not always end up at the closest possible spot.
     *
     * TODO:
     *  Redo filter interface to have caller pass in a context cookie
     *  that can be passed through to filter so that it could have access
     *  to more info than just <x,y>.  Also add a way to stop collecting
     *  when an optimal value is found, without checking and skipping the
     *  rest of the map.
     */
    for (radius = include_cxcy ? 0 : 1; radius <= maxradius; ++radius) {
        if (!ring_pairs) {
            newpass = passend = TRUE;
        } else {
            /* 0 (if include_cxcy) and maxradius override odd/even */
            newpass = ((radius % 2) != 0 || radius == 0); /* odd */
            passend = ((radius % 2) == 0 || radius == maxradius); /* even */
        }
        if (newpass || !passcc) { /* !passcc is redundant but used to fend
                                   * off analyzers thinking use of passcc
                                   * below might occur while still Null */
            passcc = ccc; /* start of output entries for current radius (or
                           * first half of radius pair depending on flags) */
            n = 0; /* number of entries for passcc; used for shuffling */
        }
        lox = cx - radius, hix = cx + radius;
        loy = cy - radius, hiy = cy + radius;
        for (y = max(loy, 0); y <= hiy; ++y) {
            if (y > ROWNO - 1)
                break; /* done with collection for current radius */
            for (x = max(lox, 1); x <= hix; ++x) { /* column 0 is not used */
                if (x > COLNO - 1)
                    break; /* advance to next 'y' */
                if (x != lox && x != hix && y != loy && y != hiy)
                    continue; /* not any edge of ring/square */
                if ((skip_mons && m_at(x, y))
                    /* note: !ACCESSIBLE() would reject water and lava;
                       !ZAP_POS() accepts them; caller needs to handle such */
                    || (skip_inaccessible && !ZAP_POS(levl[x][y].typ)))
                    continue; /* quick filters */
                if (filter && !(*filter)(x, y))
                    continue;
                cc.x = x, cc.y = y;
                *ccc++ = cc;
                ++n;
                ++result;
            }
        }
        if (scramble && passend) {
            /* shuffle entries gathered for current radius (or pair) */
            while (n > 1) {
                k = rn2(n); /* 0..n-1 */
                if (k) { /* swap [k] with [0] when k is 1..n-1 */
                    cc = passcc[0];
                    passcc[0] = passcc[k];
                    passcc[k] = cc;
                }
                ++passcc; /* passcc[0] has reached its final place    */
                --n;      /* and become exempt from further shuffling */
            }
        }
    } /* radius loop */
    debugpline4("collect_coords(,%d,%d,%d,,)=%d", cx, cy, maxradius, result);
    return result;
}

/* [try to] teleport hero to a safe spot */
boolean
safe_teleds(int teleds_flags)
{
    coordxy nux, nuy;
    unsigned cc_flags;
    coord candy[ROWNO * (COLNO - 1)], backupspot;
    int tcnt, candycount;

    /*
     * This used to try random locations up to 400 times, with first 200
     * tries disallowing trap locations and remaining 200 accepting such.
     * On levels without many accessible locations (either due to being
     * mostly stone or high monster population) it could fail to find a
     * spot.
     *
     * Now it tries completely randomly only 40 times, all disallowing
     * traps, then resorts to checking the entire map, near hero's spot
     * first then expanding out from there.  If no non-trap spot is found,
     * first trap spot is used.
     */
    for (tcnt = 0; tcnt < 40; ++tcnt) {
        nux = rnd(COLNO - 1);
        nuy = rn2(ROWNO);
        if (teleok(nux, nuy, FALSE)) {
            teleds(nux, nuy, teleds_flags);
            return TRUE;
        }
    }

    /* get a shuffled list of candidate locations, starting with spots
       1 or 2 steps from hero, then 3 or 4 steps, then 5 or 6, on up */
    cc_flags = CC_RING_PAIRS | CC_SKIP_MONS;
    if (!Passes_walls)
        cc_flags |= CC_SKIP_INACCS;
    candycount = collect_coords(candy, u.ux, u.uy, 0, cc_flags,
                                (boolean (*)(coordxy, coordxy)) 0);
    backupspot.x = backupspot.y = 0;
    /* skip trap locations via teleok(,,FALSE) but remember first
       encountered trap spot that is acceptable to teleok(,,TRUE) */
    for (tcnt = 0; tcnt < candycount; ++tcnt) {
        nux = candy[tcnt].x, nuy = candy[tcnt].y;
        if (teleok(nux, nuy, FALSE)) {
            teleds(nux, nuy, teleds_flags);
            return TRUE;
        }
        if (!backupspot.x && t_at(nux, nuy) && teleok(nux, nuy, TRUE))
            backupspot.x = nux, backupspot.y = nuy;
    }
    /* no non-trap spot found; if we skipped a viable trap spot, use it */
    if (backupspot.x) {
        teleds(backupspot.x, backupspot.y, teleds_flags);
        return TRUE;
    }
    return FALSE;
}

static void
vault_tele(void)
{
    struct mkroom *croom = search_special(VAULT);
    coord c;

    if (croom && somexyspace(croom, &c) && teleok(c.x, c.y, FALSE)) {
        teleds(c.x, c.y, TELEDS_TELEPORT);
        return;
    }
    tele();
}

boolean
teleport_pet(register struct monst* mtmp, boolean force_it)
{
    struct obj *otmp;

    if (mtmp == u.usteed)
        return FALSE;

    if (mtmp->mleashed) {
        otmp = get_mleash(mtmp);
        if (!otmp) {
            impossible("%s is leashed, without a leash.", Monnam(mtmp));
            goto release_it;
        }
        if (otmp->cursed && !force_it) {
            yelp(mtmp);
            return FALSE;
        } else {
            Your("leash goes slack.");
 release_it:
            m_unleash(mtmp, FALSE);
            return TRUE;
        }
    }
    return TRUE;
}

/* teleport the hero via some method other than scroll of teleport */
void
tele(void)
{
    scrolltele((struct obj *) 0);
}

/* teleport the hero; usually discover scroll of teleportation if via scroll */
void
scrolltele(struct obj* scroll)
{
    coord cc;

    /* Disable teleportation in stronghold && Vlad's Tower */
    if (noteleport_level(&gy.youmonst) && !wizard) {
        pline("A mysterious force prevents you from teleporting!");
        if (scroll)
            learnscroll(scroll); /* this is obviously a teleport scroll */
        return;
    }

    /* don't show trap if "Sorry..." */
    if (!Blinded)
        make_blinded(0L, FALSE);

    if ((u.uhave.amulet || On_W_tower_level(&u.uz)) && !rn2(3)) {
        You_feel("disoriented for a moment.");
        /* don't discover the scroll [at least not yet for wizard override];
           disorientation doesn't reveal that this is a teleport attempt */
        if (!wizard || y_n("Override?") != 'y')
            return;
    }
    if (((Teleport_control || (scroll && scroll->blessed)) && !Stunned)
        || wizard) {
        if (unconscious()) {
            pline("Being unconscious, you cannot control your teleport.");
        } else {
            char whobuf[BUFSZ];

            Strcpy(whobuf, "you");
            if (u.usteed)
                Sprintf(eos(whobuf), " and %s", mon_nam(u.usteed));
            pline("Where do %s want to be teleported?", whobuf);
            if (scroll)
                learnscroll(scroll);
            cc.x = u.ux;
            cc.y = u.uy;
            if (isok(iflags.travelcc.x, iflags.travelcc.y)) {
                /* The player showed some interest in traveling here;
                 * pre-suggest this coordinate. */
                cc = iflags.travelcc;
            }
            if (getpos(&cc, TRUE, "the desired position") < 0)
                return; /* abort */
            /* possible extensions: introduce a small error if
               magic power is low; allow transfer to solid rock */
            if (teleok(cc.x, cc.y, FALSE)) {
                /* for scroll, discover it regardless of destination */
                teleds(cc.x, cc.y, TELEDS_TELEPORT);
                if (u_at(iflags.travelcc.x, iflags.travelcc.y))
                    iflags.travelcc.x = iflags.travelcc.y = 0;
                return;
            }
            pline("Sorry...");
        }
    }

    /* we used to suppress discovery if hero teleported to a nearby
       spot which was already within view, but now there is always a
       "materialize" message regardless of how far you teleported so
       discovery of scroll type is unconditional */
    if (scroll)
        learnscroll(scroll);

    (void) safe_teleds(TELEDS_TELEPORT);
}

/* the #teleport command; 'm ^T' == choose among several teleport modes */
int
dotelecmd(void)
{
    long save_HTele, save_ETele;
    int res, added, hidden;
    boolean ignore_restrictions = FALSE;
/* also defined in spell.c */
#define NOOP_SPELL  0
#define HIDE_SPELL  1
#define ADD_SPELL   2
#define UNHIDESPELL 3
#define REMOVESPELL 4

    /* normal mode; ignore 'm' prefix if it was given */
    if (!wizard)
        return dotele(FALSE) ? ECMD_TIME : ECMD_OK;

    added = hidden = NOOP_SPELL;
    save_HTele = HTeleportation, save_ETele = ETeleportation;
    if (!iflags.menu_requested) {
        ignore_restrictions = TRUE;
    } else {
        static const struct tporttypes {
            char menulet;
            const char *menudesc;
        } tports[] = {
            /*
             * Potential combinations:
             *  1) attempt ^T without intrinsic, not know spell;
             *  2) via intrinsic, not know spell, obey restrictions;
             *  3) via intrinsic, not know spell, ignore restrictions;
             *  4) via intrinsic, know spell, obey restrictions;
             *  5) via intrinsic, know spell, ignore restrictions;
             *  6) via spell, not have intrinsic, obey restrictions;
             *  7) via spell, not have intrinsic, ignore restrictions;
             *  8) force, obey other restrictions;
             *  9) force, ignore restrictions.
             * We only support the 1st (t), 2nd (n), 6th (s), and 9th (w).
             *
             * This ignores the fact that there is an experience level
             * (or poly-form) requirement which might make normal ^T fail.
             */
            { 'n', "normal ^T on demand; no spell, obey restrictions" },
            { 's', "via spellcast; no intrinsic teleport" },
            { 't', "try ^T without having it; no spell" },
            { 'w', "debug mode; ignore restrictions" }, /* trad wizard mode */
        };
        menu_item *picks = (menu_item *) 0;
        anything any;
        winid win;
        int i, tmode, clr = 0;

        win = create_nhwindow(NHW_MENU);
        start_menu(win, MENU_BEHAVE_STANDARD);
        any = cg.zeroany;
        for (i = 0; i < SIZE(tports); ++i) {
            any.a_int = (int) tports[i].menulet;
            add_menu(win, &nul_glyphinfo, &any, (char) any.a_int, 0,
                     ATR_NONE, clr, tports[i].menudesc,
                     (tports[i].menulet == 'w') ? MENU_ITEMFLAGS_SELECTED
                                                : MENU_ITEMFLAGS_NONE);
        }
        end_menu(win, "Which way do you want to teleport?");
        i = select_menu(win, PICK_ONE, &picks);
        destroy_nhwindow(win);
        if (i > 0) {
            tmode = picks[0].item.a_int;
            /* if we got 2, use the one which wasn't preselected */
            if (i > 1 && tmode == 'w')
                tmode = picks[1].item.a_int;
            free((genericptr_t) picks);
        } else if (i == 0) {
            /* preselected one was explicitly chosen and got toggled off */
            tmode = 'w';
        } else { /* ESC */
            return ECMD_OK;
        }
        switch (tmode) {
        case 'n':
            HTeleportation |= I_SPECIAL; /* confer intrinsic teleportation */
            hidden = tport_spell(HIDE_SPELL); /* hide teleport-away */
            break;
        case 's':
            HTeleportation = ETeleportation = 0L; /* suppress intrinsic */
            added = tport_spell(ADD_SPELL); /* add teleport-away */
            break;
        case 't':
            HTeleportation = ETeleportation = 0L; /* suppress intrinsic */
            hidden = tport_spell(HIDE_SPELL); /* hide teleport-away */
            break;
        case 'w':
            ignore_restrictions = TRUE;
            break;
        }
    }

    /* if dotele() can be fatal, final disclosure might lie about
       intrinsic teleportation; we should be able to live with that
       since the menu finagling is only applicable in wizard mode */
    res = dotele(ignore_restrictions);

    HTeleportation = save_HTele;
    ETeleportation = save_ETele;
    if (added != NOOP_SPELL || hidden != NOOP_SPELL)
        /* can't both be non-NOOP so addition will yield the non-NOOP one */
        (void) tport_spell(added + hidden - NOOP_SPELL);

    return res ? ECMD_TIME : ECMD_OK;
#undef NOOP_SPELL
#undef HIDE_SPELL
#undef ADD_SPELL
#undef UNHIDESPELL
#undef REMOVESPELL
}

int
dotele(
    boolean break_the_rules) /* True: wizard mode ^T */
{
    struct trap *trap;
    const char *cantdoit;
    boolean trap_once = FALSE;

    trap = t_at(u.ux, u.uy);
    if (trap && !trap->tseen)
        trap = 0;

    if (trap) {
        if (trap->ttyp == LEVEL_TELEP && trap->tseen) {
            if (y_n("There is a level teleporter here. Trigger it?") == 'y') {
                level_tele_trap(trap, FORCETRAP);
                /* deliberate jumping will always take time even if it doesn't
                 * work */
                return 1;
            } else
                trap = 0; /* continue with normal horizontal teleport */
        } else if (trap->ttyp == TELEP_TRAP) {
            trap_once = trap->once; /* trap may get deleted, save this */
            if (trap->once) {
                pline("This is a vault teleport, usable once only.");
                if (y_n("Jump in?") == 'n') {
                    trap = 0;
                } else {
                    deltrap(trap);
                    newsym(u.ux, u.uy);
                }
            }
            if (trap)
                You("%s onto the teleportation trap.", u_locomotion("jump"));
        } else
            trap = 0;
    }
    if (!trap && !break_the_rules) {
        boolean castit = FALSE;
        int energy = 0;

        if (!Teleportation || (u.ulevel < (Role_if(PM_WIZARD) ? 8 : 12)
                               && !can_teleport(gy.youmonst.data))) {
            /* Try to use teleport away spell. */
            int knownsp = known_spell(SPE_TELEPORT_AWAY);

            /* casting isn't inhibited by being Stunned (...it ought to be) */
            castit = (knownsp >= spe_Fresh && !Confusion);
            if (!castit && !break_the_rules) {
                You("%s.", (!Teleportation ? ((knownsp != spe_Unknown)
                                              ? "can't cast that spell"
                                              : "don't know that spell")
                            : "are not able to teleport at will"));
                return 0;
            }
        }

        cantdoit = 0;
        /* 3.6.2: the magic numbers for hunger, strength, and energy
           have been changed to match the ones used in spelleffects().
           Also, failing these tests used to return 1 and use a move
           even though casting failure due to these reasons doesn't.
           [Note: this spellev() is different from the one in spell.c
           but they both yield the same result.] */
#define spellev(spell_otyp) ((int) objects[spell_otyp].oc_level)
        energy = 5 * spellev(SPE_TELEPORT_AWAY);
#undef spelllev
#if 0
        /* the addition of !break_the_rules to the outer if-block in
           1ada454f rendered this dead code */
        if (break_the_rules) {
            if (!castit)
                energy = 0;
            /* spell will cost more if carrying the Amulet, but the
               amount is rnd(2 * energy) so we can't know by how much;
               average is twice the normal cost, but could be triple;
               the extra energy is spent even if that results in not
               having enough to cast (which also uses the move) */
            else if (u.uen < energy)
                energy = u.uen;
        } else
#endif
        if (u.uhunger <= 10) {
            cantdoit = "are too weak from hunger";
        } else if (ACURR(A_STR) < 4) {
            cantdoit = "lack the strength";
        } else if (energy > u.uen) {
            cantdoit = "lack the energy";
        }
        if (cantdoit) {
            You("%s %s.", cantdoit,
                castit ? "for a teleport spell" : "to teleport");
            return 0;
        } else if (check_capacity(
                       "Your concentration falters from carrying so much.")) {
            return 1; /* this failure in spelleffects() also uses the move */
        }

        if (castit) {
            /* energy cost is deducted in spelleffects() */
            exercise(A_WIS, TRUE);
            if ((spelleffects(SPE_TELEPORT_AWAY, TRUE, FALSE) & ECMD_TIME))
                return 1;
            else if (!break_the_rules)
                return 0;
        } else {
            /* bypassing spelleffects(); apply energy cost directly */
            u.uen -= energy;
            gc.context.botl = 1;
        }
    }

    if (next_to_u()) {
        if (trap && trap_once)
            vault_tele();
        else
            tele();
        (void) next_to_u();
    } else {
        You("%s", shudder_for_moment);
        return 0;
    }
    if (!trap)
        morehungry(100);
    return 1;
}

void
level_tele(void)
{
    static const char get_there_from[] = "get there from %s.";
    register int newlev;
    d_level newlevel;
    const char *escape_by_flying = 0; /* when surviving dest of -N */
    char buf[BUFSZ];
    boolean force_dest = FALSE;

    if (iflags.debug_fuzzer) {
        do {
            newlevel.dnum = rn2(gn.n_dgns);
        } while (newlevel.dnum == astral_level.dnum
                 || gd.dungeons[newlevel.dnum].flags.unconnected
                 || !gd.dungeons[newlevel.dnum].num_dunlevs);
        newlevel.dlevel = 1 + rn2(dunlevs_in_dungeon(&newlevel));
        assign_level(&u.ucamefrom, &u.uz);
        schedule_goto(&newlevel, UTOTYPE_NONE, (char *) 0, (char *) 0);
        return;
    }
    if ((u.uhave.amulet || In_endgame(&u.uz) || In_sokoban(&u.uz))
        && !wizard) {
        You_feel("very disoriented for a moment.");
        return;
    }
    if ((Teleport_control && !Stunned) || wizard) {
        char qbuf[BUFSZ];
        int trycnt = 0;

        Strcpy(qbuf, "To what level do you want to teleport?");
        do {
            if (iflags.menu_requested) {
                /* wizard mode 'm ^V' skips prompting on first pass
                   (note: level Tport via menu won't have any second pass) */
                iflags.menu_requested = FALSE;
                if (wizard)
                    goto levTport_menu;
            }
            if (++trycnt == 2) {
                if (wizard)
                    Strcat(qbuf, " [type a number, name, or ? for a menu]");
                else
                    Strcat(qbuf, " [type a number or name]");
            }
            *buf = '\0'; /* EDIT_GETLIN: if we're on second or later pass,
                            the previous input was invalid so don't use it
                            as getlin()'s preloaded default answer */
            getlin(qbuf, buf);
            if (!strcmp(buf, "\033")) { /* cancelled */
                if (Confusion && rnl(5)) {
                    pline("Oops...");
                    goto random_levtport;
                }
                return;
            } else if (!strcmp(buf, "*")) {
                goto random_levtport;
            } else if (Confusion && rnl(5)) {
                pline("Oops...");
                goto random_levtport;
            }
            if (wizard && !strcmp(buf, "?")) {
                schar destlev;
                xint16 destdnum;

 levTport_menu:
                destlev = 0;
                destdnum = 0;
                newlev = (int) print_dungeon(TRUE, &destlev, &destdnum);
                if (!newlev)
                    return;

                newlevel.dnum = destdnum;
                newlevel.dlevel = destlev;
                if (In_endgame(&newlevel) && !In_endgame(&u.uz)) {
                    struct obj *amu;

                    if (!u.uhave.amulet
                        && (amu = mksobj(AMULET_OF_YENDOR, TRUE, FALSE))
                               != 0) {
                        /* ordinarily we'd use hold_another_object()
                           for something like this, but we don't want
                           fumbling or already full pack to interfere */
                        amu = addinv(amu);
                        prinv("Endgame prerequisite:", amu, 0L);
                    }
                }
                force_dest = TRUE;
            } else if ((newlev = lev_by_name(buf)) == 0)
                newlev = atoi(buf);
        } while (!newlev && !digit(buf[0])
                 && (buf[0] != '-' || !digit(buf[1])) && trycnt < 10);

        /* no dungeon escape via this route */
        if (newlev == 0) {
            if (trycnt >= 10)
                goto random_levtport;
            if (ynq("Go to Nowhere.  Are you sure?") != 'y')
                return;
            You("%s in agony as your body begins to warp...",
                is_silent(gy.youmonst.data) ? "writhe" : "scream");
            display_nhwindow(WIN_MESSAGE, FALSE);
            You("cease to exist.");
            if (gi.invent)
                Your("possessions land on the %s with a thud.",
                     surface(u.ux, u.uy));
            gk.killer.format = NO_KILLER_PREFIX;
            Strcpy(gk.killer.name, "committed suicide");
            done(DIED);
            pline("An energized cloud of dust begins to coalesce.");
            Your("body rematerializes%s.",
                 gi.invent ? ", and you gather up all your possessions" : "");
            return;
        }

        /* if in Knox and the requested level > 0, stay put.
         * we let negative values requests fall into the "heaven" loop.
         */
        if (single_level_branch(&u.uz) && newlev > 0 && !force_dest) {
            You1(shudder_for_moment);
            return;
        }
        /* if in Quest, the player sees "Home 1", etc., on the status
         * line, instead of the logical depth of the level.  controlled
         * level teleport request is likely to be relativized to the
         * status line, and consequently it should be incremented to
         * the value of the logical depth of the target level.
         *
         * we let negative values requests fall into the "heaven" handling.
         */
        if (In_quest(&u.uz) && newlev > 0)
            newlev = newlev + gd.dungeons[u.uz.dnum].depth_start - 1;
    } else { /* involuntary level tele */
 random_levtport:
        newlev = random_teleport_level();
        if (newlev == depth(&u.uz)) {
            You1(shudder_for_moment);
            return;
        }
    }

    if (u.utrap && u.utraptype == TT_BURIEDBALL)
        buried_ball_to_punishment();

    if (!next_to_u() && !force_dest) {
        You1(shudder_for_moment);
        return;
    }
    if (In_endgame(&u.uz)) { /* must already be wizard */
        int llimit = dunlevs_in_dungeon(&u.uz);

        if (newlev >= 0 || newlev <= -llimit) {
            You_cant(get_there_from, "here");
            return;
        }
        newlevel.dnum = u.uz.dnum;
        newlevel.dlevel = llimit + newlev;
        schedule_goto(&newlevel, UTOTYPE_NONE, (char *) 0, (char *) 0);
        return;
    }

    gk.killer.name[0] = 0; /* still alive, so far... */

    if (iflags.debug_fuzzer && newlev < 0)
        goto random_levtport;
    if (newlev < 0 && !force_dest) {
        if (*u.ushops0) {
            /* take unpaid inventory items off of shop bills */
            gi.in_mklev = TRUE; /* suppress map update */
            u_left_shop(u.ushops0, TRUE);
            /* you're now effectively out of the shop */
            *u.ushops0 = *u.ushops = '\0';
            gi.in_mklev = FALSE;
        }
        if (newlev <= -10) {
            You("arrive in heaven.");
            SetVoice((struct monst *) 0, 0, 80, voice_deity);
            verbalize("Thou art early, but we'll admit thee.");
            gk.killer.format = NO_KILLER_PREFIX;
            Strcpy(gk.killer.name, "went to heaven prematurely");
        } else if (newlev == -9) {
            You_feel("deliriously happy.");
            pline("(In fact, you're on Cloud 9!)");
            display_nhwindow(WIN_MESSAGE, FALSE);
        } else
            You("are now high above the clouds...");

        if (gk.killer.name[0]) {
            ; /* arrival in heaven is pending */
        } else if (Levitation) {
            escape_by_flying = "float gently down to earth";
        } else if (Flying) {
            escape_by_flying = "fly down to the ground";
        } else {
            pline("Unfortunately, you don't know how to fly.");
            You("plummet a few thousand feet to your death.");
            Sprintf(gk.killer.name,
                    "teleported out of the dungeon and fell to %s death",
                    uhis());
            gk.killer.format = NO_KILLER_PREFIX;
        }
    }

    if (gk.killer.name[0]) { /* the chosen destination was not survivable */
        d_level lsav;

        /* set specific death location; this also suppresses bones */
        lsav = u.uz;   /* save current level, see below */
        u.uz.dnum = 0; /* main dungeon */
        u.uz.dlevel = (newlev <= -10) ? -10 : 0; /* heaven or surface */
        done(DIED);
        /* can only get here via life-saving (or declining to die in
           explore|debug mode); the hero has now left the dungeon... */
        escape_by_flying = "find yourself back on the surface";
        u.uz = lsav; /* restore u.uz so escape code works */
    }

    /* calls done(ESCAPED) if newlevel==0 */
    if (escape_by_flying) {
        You("%s.", escape_by_flying);
        /* [dlevel used to be set to 1, but it doesn't make sense to
            teleport out of the dungeon and float or fly down to the
            surface but then actually arrive back inside the dungeon] */
        newlevel.dnum = 0;   /* specify main dungeon */
        newlevel.dlevel = 0; /* escape the dungeon */
    } else if (force_dest) {
        /* wizard mode menu; no further validation needed */
        ;
    } else if (u.uz.dnum == medusa_level.dnum
               && newlev >= gd.dungeons[u.uz.dnum].depth_start
                                + dunlevs_in_dungeon(&u.uz)) {
        find_hell(&newlevel);
    } else {
        /* FIXME: we should avoid using hard-coded knowledge of
           which branches don't connect to anything deeper;
           mainly used to distinguish "can't get there from here"
           vs "from anywhere" rather than to control destination */
        d_level *qbranch = In_quest(&u.uz) ? &qstart_level
                          : In_mines(&u.uz) ? &mineend_level
                            : &sanctum_level;
        int deepest = gd.dungeons[qbranch->dnum].depth_start
                      + dunlevs_in_dungeon(qbranch) - 1;

        /* if invocation did not yet occur, teleporting into
         * the last level of Gehennom is forbidden.
         */
        if (!wizard && Inhell && !u.uevent.invoked && newlev >= deepest) {
            newlev = deepest - 1;
            pline("Sorry...");
        }
        /* no teleporting out of quest dungeon */
        if (In_quest(&u.uz) && newlev < depth(&qstart_level))
            newlev = depth(&qstart_level);
        /* the player thinks of levels purely in logical terms, so
         * we must translate newlev to a number relative to the
         * current dungeon.
         */
        get_level(&newlevel, newlev);

        if (on_level(&newlevel, &u.uz) && newlev != depth(&u.uz)) {
            You_cant(get_there_from,
                     (newlev > deepest) ? "anywhere" : "here");
            return;
        }
    }

    schedule_goto(&newlevel, UTOTYPE_NONE, (char *) 0,
                  Verbose(2, level_tele)
                      ? "You materialize on a different level!"
                      : (char *) 0);
#if 0   /* always wait until end of turn to change level, otherwise code
         * that references monsters as this call stack unwinds won't be
         * able to access them reliably; the do-the-change-now code here
         * dates from when reading a scroll of teleportation wouldn't
         * always make the scroll become discovered but that's no longer
         * the case so it shouldn't be needed anymore */

    /* in case player just read a scroll and is about to be asked to
       call it something, we can't defer until the end of the turn */
    if (u.utotype && !gc.context.mon_moving)
        deferred_goto();
#endif
}

void
domagicportal(struct trap *ttmp)
{
    struct d_level target_level;
    int totype;
    const char *stunmsg = (char *) 0;

    if (u.utrap && u.utraptype == TT_BURIEDBALL)
        buried_ball_to_punishment();

    if (!next_to_u()) {
        You1(shudder_for_moment);
        return;
    }

    /* if landed from another portal, do nothing */
    /* problem: level teleport landing escapes the check */
    if (!on_level(&u.uz, &u.uz0))
        return;

    You("activated a magic portal!");

    /* prevent the poor shnook, whose amulet was stolen while in
     * the endgame, from accidently triggering the portal to the
     * next level, and thus losing the game
     */
    if (In_endgame(&u.uz) && !u.uhave.amulet) {
        You_feel("dizzy for a moment, but nothing happens...");
        return;
    }

    target_level = ttmp->dst;

    /* coming back from tutorial doesn't trigger stunning */
    if (In_tutorial(&u.uz) && !In_tutorial(&target_level)) {
        /* returning to normal play => arrive on level 1 stairs */
        totype = UTOTYPE_ATSTAIRS;
        stunmsg = "Resuming regular play.";
    } else {
        totype = UTOTYPE_PORTAL;
        stunmsg = !Stunned ? "You feel slightly dizzy."
                            : "You feel dizzier.";
        make_stunned((HStun & TIMEOUT) + 3L, FALSE);
    }

    schedule_goto(&target_level, totype, stunmsg, (char *) 0);
}

void
tele_trap(struct trap *trap)
{
    if (In_endgame(&u.uz) || Antimagic) {
        if (Antimagic)
            shieldeff(u.ux, u.uy);
        You_feel("a wrenching sensation.");
    } else if (!next_to_u()) {
        You1(shudder_for_moment);
    } else if (trap->once) {
        deltrap(trap);
        newsym(u.ux, u.uy); /* get rid of trap symbol */
        vault_tele();
    } else
        tele();
}

void
level_tele_trap(struct trap *trap, unsigned int trflags)
{
    char verbbuf[BUFSZ];
    boolean intentional = FALSE;

    if ((trflags & (VIASITTING | FORCETRAP)) != 0) {
        Strcpy(verbbuf, "trigger"); /* follows "You sit down." */
        intentional = TRUE;
    } else
        Sprintf(verbbuf, "%s onto", u_locomotion("step"));
    You("%s a level teleport trap!", verbbuf);

    if (Antimagic && !intentional) {
        shieldeff(u.ux, u.uy);
    }
    if ((Antimagic && !intentional) || In_endgame(&u.uz)) {
        You_feel("a wrenching sensation.");
        return;
    }
    deltrap(trap);
    newsym(u.ux, u.uy); /* get rid of trap symbol */
    level_tele();

    if (Hallucination || Teleport_control)
        You("briefly feel %s.", Hallucination ? "oriented" : "centered");
    else
        You_feel("%sdisoriented.", Confusion ? "even more " : "");
    /* magic portal traversal causes brief Stun; for level teleport, use
       confusion instead, and only when hero lacks control; do this after
       processing the level teleportation attempt because being confused
       can affect the outcome ("Oops" result) */
    if (!Teleport_control)
        make_confused((HConfusion & TIMEOUT) + 3L, FALSE);
}

/* check whether monster can arrive at location <x,y> via Tport (or fall) */
static boolean
rloc_pos_ok(
    coordxy x, coordxy y, /* coordinates of candidate location */
    struct monst *mtmp)
{
    coordxy xx, yy;

    if (!goodpos(x, y, mtmp, GP_CHECKSCARY))
        return FALSE;
    /*
     * Check for restricted areas present in some special levels.
     *
     * `xx' is current column; if 0, then `yy' will contain flag bits
     * rather than row:  bit #0 set => moving upwards; bit #1 set =>
     * inside the Wizard's tower.
     */
    xx = mtmp->mx;
    yy = mtmp->my;
    if (!xx) {
        /* no current location (migrating monster arrival) */
        if (gd.dndest.nlx && On_W_tower_level(&u.uz))
            return (((yy & 2) != 0)
                    /* inside xor not within */
                    ^ !within_bounded_area(x, y, gd.dndest.nlx, gd.dndest.nly,
                                           gd.dndest.nhx, gd.dndest.nhy));
        if (gu.updest.lx && (yy & 1) != 0) /* moving up */
            return (within_bounded_area(x, y, gu.updest.lx, gu.updest.ly,
                                        gu.updest.hx, gu.updest.hy)
                    && (!gu.updest.nlx
                        || !within_bounded_area(x, y,
                                              gu.updest.nlx, gu.updest.nly,
                                              gu.updest.nhx, gu.updest.nhy)));
        if (gd.dndest.lx && (yy & 1) == 0) /* moving down */
            return (within_bounded_area(x, y, gd.dndest.lx, gd.dndest.ly,
                                        gd.dndest.hx, gd.dndest.hy)
                    && (!gd.dndest.nlx
                        || !within_bounded_area(x, y,
                                              gd.dndest.nlx, gd.dndest.nly,
                                              gd.dndest.nhx, gd.dndest.nhy)));
    } else {
        /* [try to] prevent a shopkeeper or temple priest from being
           sent out of his room (caller might resort to goodpos() if
           we report failure here, so this isn't full prevention) */
        if (mtmp->isshk && inhishop(mtmp)) {
            if (levl[x][y].roomno != (unsigned char) ESHK(mtmp)->shoproom)
                return FALSE;
        } else if (mtmp->ispriest && inhistemple(mtmp)) {
            if (levl[x][y].roomno !=  (unsigned char) EPRI(mtmp)->shroom)
                return FALSE;
        }
        /* current location is <xx,yy> */
        if (!tele_jump_ok(xx, yy, x, y))
            return FALSE;
    }
    /* <x,y> is ok */
    return TRUE;
}

/*
 * rloc_to()
 *
 * Pulls a monster from its current position and places a monster at
 * a new x and y.  If oldx is 0, then the monster was not in the
 * levels.monsters array.  However, if oldx is 0, oldy may still have
 * a value because mtmp is a migrating_mon.  Worm tails are always
 * placed randomly around the head of the worm.
 */
static void
rloc_to_core(
    struct monst *mtmp,
    coordxy x, coordxy y,
    unsigned rlocflags)
{
    coordxy oldx = mtmp->mx, oldy = mtmp->my;
    boolean resident_shk = mtmp->isshk && inhishop(mtmp);
    boolean preventmsg = (rlocflags & RLOC_NOMSG) != 0;
    boolean vanishmsg = (rlocflags & RLOC_MSG) != 0;
    boolean appearmsg = (mtmp->mstrategy & STRAT_APPEARMSG) != 0;
    boolean domsg = !gi.in_mklev && (vanishmsg || appearmsg) && !preventmsg;
    boolean telemsg = FALSE;

    if (x == mtmp->mx && y == mtmp->my && m_at(x, y) == mtmp)
        return; /* that was easy */

    if (oldx) { /* "pick up" monster */
        if (domsg && canspotmon(mtmp)) {
            if (couldsee(x, y) || sensemon(mtmp)) {
                telemsg = TRUE;
            } else {
                pline("%s vanishes!", Monnam(mtmp));
            }
            /* avoid "It suddenly appears!" for a STRAT_APPEARMSG monster
               that has just teleported away if we won't see it after this
               vanishing (the regular appears message will be given if we
               do see it) */
            appearmsg = FALSE;
        }

        if (mtmp->wormno) {
            remove_worm(mtmp);
        } else {
            remove_monster(oldx, oldy);
            newsym(oldx, oldy); /* update old location */
        }
    }

    mon_track_clear(mtmp);
    place_monster(mtmp, x, y); /* put monster down */
    update_monster_region(mtmp);

    if (mtmp->wormno) /* now put down tail */
        place_worm_tail_randomly(mtmp, x, y);

    if (u.ustuck == mtmp) {
        if (u.uswallow) {
            u_on_newpos(mtmp->mx, mtmp->my);
            docrt();
        } else if (!next2u(mtmp->mx, mtmp->my)) {
           unstuck(mtmp);
        }
    }

    maybe_unhide_at(x, y);
    newsym(x, y);      /* update new location */
    set_apparxy(mtmp); /* orient monster */
    if (domsg && (canspotmon(mtmp) || appearmsg)) {
        int du = distu(x, y), olddu;
        const char *next = (du <= 2) ? " next to you" : 0, /* next2u() */
                   *nearu = (du <= BOLT_LIM * BOLT_LIM) ? " close by" : 0;

        mtmp->mstrategy &= ~STRAT_APPEARMSG; /* one chance only */
        if (telemsg && (couldsee(x, y) || sensemon(mtmp))) {
            pline("%s vanishes and reappears%s.",
                  Monnam(mtmp),
                  next ? next
                  : nearu ? nearu
                    : ((olddu = distu(oldx, oldy)) == du) ? ""
                      : (du < olddu) ? " closer to you"
                        : " farther away");
        } else {
            pline("%s %s%s%s!",
                  appearmsg ? Amonnam(mtmp) : Monnam(mtmp),
                  appearmsg ? "suddenly " : "",
                  !Blind ? "appears" : "arrives",
                  next ? next : nearu ? nearu : "");
        }
    }

    /* shopkeepers will only teleport if you zap them with a wand of
       teleportation or if they've been transformed into a jumpy monster;
       the latter only happens if you've attacked them with polymorph */
    if (resident_shk && !inhishop(mtmp))
        make_angry_shk(mtmp, oldx, oldy);

    /* if a monster carrying shop goods teleports out of the shop, blame
       it on the hero; chance of an unpaid item is vanishingly small, but
       no_charge is easily possible and needs to be cleared if not in shop;
       a for-sale item is ordinary here--shk won't notice it leaving; if
       mtmp teleports from one shop into another, no_charge status sticks
       and an item on the first shk's bill stays there */
    if (mtmp->minvent && !costly_spot(x, y)) {
        struct obj *otmp;
        struct monst *shkp = find_objowner(mtmp->minvent, oldx, oldy);
        boolean peaceful = !shkp || shkp->mpeaceful;

        for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
            if (otmp->no_charge)
                otmp->no_charge = 0;
            else if (shkp && onshopbill(otmp, shkp, TRUE))
                stolen_value(otmp, oldx, oldy, peaceful, FALSE);
        }
    }

    /* if hero is busy, maybe stop occupation */
    if (go.occupation)
        (void) dochugw(mtmp, FALSE);

    /* trapped monster teleported away */
    if (mtmp->mtrapped && !mtmp->wormno)
        (void) mintrap(mtmp, NO_TRAP_FLAGS);
}

void
rloc_to(struct monst *mtmp, coordxy x, coordxy y)
{
    rloc_to_core(mtmp, x, y, RLOC_NOMSG);
}

void
rloc_to_flag(
    struct monst *mtmp,
    coordxy x, coordxy y,
    unsigned rlocflags)
{
    rloc_to_core(mtmp, x, y, rlocflags);
}

static stairway *
stairway_find_forwiz(boolean isladder, boolean up)
{
    stairway *stway = gs.stairs;

    while (stway && !(stway->isladder == isladder
                      && stway->up == up && stway->tolev.dnum == u.uz.dnum))
        stway = stway->next;
    return stway;
}

/* place a monster at a random location, typically due to teleport;
   return TRUE if successful, FALSE if not; rlocflags is RLOC_foo flags */
boolean
rloc(
    struct monst *mtmp, /* mtmp->mx==0 implies migrating monster arrival */
    unsigned rlocflags)
{
    coord cc, backupcc, candy[ROWNO * (COLNO - 1)]; /* room for entire map */
    unsigned cc_flags;
    coordxy x, y;
    int trycount, i, j, candycount;

    if (mtmp == u.usteed) {
        tele();
        return TRUE;
    }

    if (mtmp->iswiz && mtmp->mx) { /* Wizard, not just arriving */
        stairway *stway;

        if (!In_W_tower(u.ux, u.uy, &u.uz)) {
            stway = stairway_find_forwiz(FALSE, TRUE);
        } else if (!stairway_find_forwiz(TRUE, FALSE)) { /* bottom of tower */
            stway = stairway_find_forwiz(TRUE, TRUE);
        } else {
            stway = stairway_find_forwiz(TRUE, FALSE);
        }

        x = stway ? stway->sx : 0;
        y = stway ? stway->sy : 0;

        /* if the wiz teleports away to heal, try the up staircase,
           to block the player's escaping before he's healed
           (deliberately use `goodpos' rather than `rloc_pos_ok' here) */
        if (goodpos(x, y, mtmp, NO_MM_FLAGS))
            goto found_xy;
    }

    /* wizard-mode player can choose destination by setting 'montelecontrol'
       option; ignored if/when this is arrival of a migrating monster */
    if (iflags.mon_telecontrol && mtmp->mx) {
        cc.x = mtmp->mx, cc.y = mtmp->my;
        if (control_mon_tele(mtmp, &cc, rlocflags, TRUE)) {
            x = cc.x, y = cc.y;
            goto found_xy;
        }
    }

    /* this used to try randomly 1000 times, then fallback to left-to-right
       top-to-bottom exhaustive check; now that the exhaustive check uses
       randomized order, reduce the number of random attempts to 50;
       on levels with lots of available space, random can find a spot more
       quickly but might fail to find one no matter how many tries it makes */
    for (trycount = 0; trycount < 50; ++trycount) {
        x = rnd(COLNO - 1); /* 1..COLNO-1 */
        y = rn2(ROWNO); /* 0..ROWNO-1 */
        if (rloc_pos_ok(x, y, mtmp)) /* rejects 'onscary' */
            goto found_xy;
    }

    /* try harder to find a good place; gather a list of all candidate
       locations (every accessible unoccupied spot except for hero's;
       goodpos() will reject that), then shuffle them ourselves instead
       of having collect_coords() do it (which would be in rings centered
       around arbitrary <COLNO/2,ROWNO/2>) */
    cc_flags = CC_INCL_CENTER | CC_UNSHUFFLED | CC_SKIP_MONS;
    if (!passes_walls(mtmp->data))
        cc_flags |= CC_SKIP_INACCS;
    candycount = collect_coords(candy, COLNO / 2, ROWNO / 2, 0, cc_flags,
                                (boolean (*)(coordxy, coordxy)) 0);
    backupcc.x = backupcc.y = 0;
    for (i = 0; i < candycount; ++i) {
        if ((j = rn2(candycount - i)) > 0) {
            cc = candy[i];
            candy[i] = candy[i + j];
            candy[i + j] = cc;
        }
        x = candy[i].x, y = candy[i].y;
        if (rloc_pos_ok(x, y, mtmp))
            goto found_xy;
        if (!backupcc.x && goodpos(x, y, mtmp, NO_MM_FLAGS))
            backupcc.x = x, backupcc.y = y;
    }

    /* we didn't find any spot acceptable to rloc_pos_ok() which avoids
       'onscary' and honors teleport regions, but if we did find a spot
       that was acceptable to goodpos() (which ignores 'onscary' and
       teleport regions) we'll use that; otherwise give up */
    if (!backupcc.x) {
        /* level either full of monsters or somehow faulty */
        if ((rlocflags & RLOC_ERR) != 0)
            impossible("rloc(): couldn't relocate monster");
        return FALSE;
    }
    x = backupcc.x, y = backupcc.y;
    /*goto found_xy;*/
 found_xy:
    rloc_to_core(mtmp, x, y, rlocflags);
    return TRUE;
}

/* let wizard-mode player choose a teleporting monster's destination */
boolean
control_mon_tele(
    struct monst *mon,
    coord *cc_p, /* input: default spot; output: player selected spot */
    unsigned rlocflags,
    boolean via_rloc)
{
    char tcbuf[BUFSZ];

    if (!isok(cc_p->x, cc_p->y)) {
        cc_p->x = mon->mx, cc_p->y = mon->my;
        if (!isok(cc_p->x, cc_p->y))
            cc_p->x = u.ux, cc_p->y = u.uy;
    }

    if (!wizard || !iflags.mon_telecontrol)
        return FALSE;

    pline("Teleport %s @ <%d,%d> where?",
          noit_mon_nam(mon), mon->mx, mon->my);
    /* getpos '?' will show "Move the cursor to <where to teleport Foo>:" */
    Sprintf(tcbuf, "where to teleport %s", noit_mon_nam(mon));
    if (getpos(cc_p, FALSE, tcbuf) >= 0 && !u_at(cc_p->x, cc_p->y)) {
        if (via_rloc
              ? rloc_pos_ok(cc_p->x, cc_p->y, mon)
              : goodpos(cc_p->x, cc_p->y, mon, rlocflags))
            return TRUE;
        if (!iflags.debug_fuzzer) {
            Sprintf(tcbuf, "<%d,%d> is not considered viable; force anyway?",
                    mon->mx, mon->my);
            if (y_n(tcbuf) == 'y')
                return TRUE;
        }
    }
    pline("%s destination.", via_rloc ? "Picking random" : "Using derived");
    return FALSE;
}

static void
mvault_tele(struct monst* mtmp)
{
    struct mkroom *croom = search_special(VAULT);
    coord c;

    if (croom && somexyspace(croom, &c) && goodpos(c.x, c.y, mtmp, 0)) {
        rloc_to(mtmp, c.x, c.y);
        return;
    }
    (void) rloc(mtmp, RLOC_NONE);
}

boolean
tele_restrict(struct monst* mon)
{
    if (noteleport_level(mon)) {
        if (canseemon(mon))
            pline("A mysterious force prevents %s from teleporting!",
                  mon_nam(mon));
        return TRUE;
    }
    return FALSE;
}

void
mtele_trap(struct monst* mtmp, struct trap* trap, int in_sight)
{
    char *monname;

    if (tele_restrict(mtmp))
        return;
    if (teleport_pet(mtmp, FALSE)) {
        /* save name with pre-movement visibility */
        monname = Monnam(mtmp);

        /* Note: don't remove the trap if a vault.  Other-
         * wise the monster will be stuck there, since
         * the guard isn't going to come for it...
         */
        if (trap->once)
            mvault_tele(mtmp);
        else
            (void) rloc(mtmp, RLOC_NONE);

        if (in_sight) {
            if (canseemon(mtmp))
                pline("%s seems disoriented.", monname);
            else
                pline("%s suddenly disappears!", monname);
            seetrap(trap);
        }
    }
}

/* return Trap_Effect_Finished if still on level, Trap_Moved_Mon if not */
int
mlevel_tele_trap(
    struct monst *mtmp,
    struct trap *trap,
    boolean force_it,
    int in_sight)
{
    int tt = (trap ? trap->ttyp : NO_TRAP);

    if (mtmp == u.ustuck) /* probably a vortex */
        return Trap_Effect_Finished; /* temporary? kludge */
    if (teleport_pet(mtmp, force_it)) {
        d_level tolevel;
        int migrate_typ = MIGR_RANDOM;

        if (is_hole(tt)) {
            if (Is_stronghold(&u.uz)) {
                assign_level(&tolevel, &valley_level);
            } else if (Is_botlevel(&u.uz)) {
                if (in_sight && trap->tseen)
                    pline("%s avoids the %s.", Monnam(mtmp),
                          (tt == HOLE) ? "hole" : "trap");
                return Trap_Effect_Finished;
            } else {
                assign_level(&tolevel, &trap->dst);
                (void) clamp_hole_destination(&tolevel);
            }
        } else if (tt == MAGIC_PORTAL) {
            if (In_endgame(&u.uz) && (mon_has_amulet(mtmp)
                                      || is_home_elemental(mtmp->data)
                                      || rn2(7))) {
                if (in_sight && mtmp->data->mlet != S_ELEMENTAL) {
                    pline("%s seems to shimmer for a moment.", Monnam(mtmp));
                    seetrap(trap);
                }
                return Trap_Effect_Finished;
            } else {
                assign_level(&tolevel, &trap->dst);
                migrate_typ = MIGR_PORTAL;
            }
        } else if (tt == LEVEL_TELEP || tt == NO_TRAP) {
            int nlev;

            if (mon_has_amulet(mtmp) || In_endgame(&u.uz)
                /* NO_TRAP is used when forcing a monster off the level;
                   onscary(0,0,) is true for the Wizard, Riders, lawful
                   minions, Angels of any alignment, shopkeeper or priest
                   currently inside his or her own special room */
                || (tt == NO_TRAP && onscary(0, 0, mtmp))) {
                if (in_sight)
                    pline("%s seems very disoriented for a moment.",
                          Monnam(mtmp));
                return Trap_Effect_Finished;
            }
            if (tt == NO_TRAP) {
                /* creature is being forced off the level to make room;
                   it will try to return to this level (at a random spot
                   rather than its current one) if the level is left by
                   the hero and then revisited */
                assign_level(&tolevel, &u.uz);
            } else {
                nlev = random_teleport_level();
                if (nlev == depth(&u.uz)) {
                    if (in_sight)
                        pline("%s shudders for a moment.", Monnam(mtmp));
                    return Trap_Effect_Finished;
                }
                get_level(&tolevel, nlev);
            }
        } else {
            impossible("mlevel_tele_trap: unexpected trap type (%d)", tt);
            return Trap_Effect_Finished;
        }

        if (in_sight) {
            pline("Suddenly, %s %s.", mon_nam(mtmp),
                  (tt == HOLE) ? "falls into a hole"
                  : (tt == TRAPDOOR) ? "falls through a trap door"
                  : "disappears out of sight");
            if (trap)
                seetrap(trap);
        }
        if (is_xport(tt) && !control_teleport(mtmp->data))
            mtmp->mconf = 1;
        migrate_to_level(mtmp, ledger_no(&tolevel), migrate_typ, (coord *) 0);
        return Trap_Moved_Mon; /* no longer on this level */
    }
    return Trap_Effect_Finished;
}

/* place object randomly, returns False if it's gone (eg broken) */
boolean
rloco(struct obj *obj)
{
    coordxy tx, ty, otx, oty;
    boolean restricted_fall;
    int try_limit = 4000;

    if (obj->otyp == CORPSE && is_rider(&mons[obj->corpsenm])) {
        if (revive_corpse(obj))
            return FALSE;
    }

    obj_extract_self(obj);
    otx = obj->ox;
    oty = obj->oy;
    restricted_fall = (otx == 0 && gd.dndest.lx);
    do {
        tx = rn1(COLNO - 3, 2);
        ty = rn2(ROWNO);
        if (!--try_limit)
            break;
    } while (!goodpos(tx, ty, (struct monst *) 0, 0)
             || (restricted_fall
                 && (!within_bounded_area(tx, ty, gd.dndest.lx, gd.dndest.ly,
                                          gd.dndest.hx, gd.dndest.hy)
                     || (gd.dndest.nlx
                         && within_bounded_area(tx, ty,
                                              gd.dndest.nlx, gd.dndest.nly,
                                              gd.dndest.nhx, gd.dndest.nhy))))
             /* on the Wizard Tower levels, objects inside should
                stay inside and objects outside should stay outside */
             || (gd.dndest.nlx && On_W_tower_level(&u.uz)
                 && within_bounded_area(tx, ty, gd.dndest.nlx, gd.dndest.nly,
                                        gd.dndest.nhx, gd.dndest.nhy)
                    != within_bounded_area(otx, oty,
                                           gd.dndest.nlx, gd.dndest.nly,
                                           gd.dndest.nhx, gd.dndest.nhy)));

    if (flooreffects(obj, tx, ty, "fall")) {
        /* update old location since flooreffects() couldn't;
           unblock_point() for boulder handled by obj_extract_self() */
        newsym(otx, oty);
        return FALSE;
    } else if (otx == 0 && oty == 0) {
        ; /* fell through a trap door; no update of old loc needed */
    } else {
        struct monst *shkp = find_objowner(obj, otx, oty);
        boolean objinshop = shkp && costly_spot(otx, oty),
                onboundary = shkp && costly_adjacent(shkp, otx, oty);

        /*
         * If object starts inside shop or is unpaid and on shop boundary:
         * if hero is outside the shop, treat this as theft;
         * otherwise, if it arrives inside same shop, remove it from bill;
         * otherwise, if it arrives on the boundary, add it to bill;
         * if it arrives outside the shop, treat this as a theft.
         * Billing routines deal with obj->no_charge.
         */
        if (objinshop || (obj->unpaid && onboundary)) {
            char h = *in_rooms(u.ux, u.uy, SHOPBASE),
                 oo = *in_rooms(otx, oty, 0);
            boolean hinshop = h && strchr(in_rooms(shkp->mx, shkp->my, 0), h);

            if (hinshop && costly_spot(tx, ty)
                /* verify that it's the same shop */
                && oo && strchr(in_rooms(tx, ty, 0), oo)) {
                if (obj->unpaid)
                    subfrombill(obj, shkp);
            } else if (hinshop && costly_adjacent(shkp, tx, ty)
                       && oo && strchr(in_rooms(tx, ty, 0), oo)) {
                if (!obj->unpaid)
                    addtobill(obj, FALSE, FALSE, FALSE);
            } else {
                (void) stolen_value(obj, otx, oty, FALSE, FALSE);
            }
        }

        newsym(otx, oty); /* update old location */
    }
    place_object(obj, tx, ty);
    /* note: block_point() for boulder handled by place_object() */
    newsym(tx, ty);
    return TRUE;
}

/* Returns an absolute depth */
int
random_teleport_level(void)
{
    int nlev, max_depth, min_depth, cur_depth = (int) depth(&u.uz);

    /* [the endgame case can only occur in wizard mode] */
    if (!rn2(5) || single_level_branch(&u.uz) || In_endgame(&u.uz))
        return cur_depth;

    /* What I really want to do is as follows:
     * -- If in a dungeon that goes down, the new level is to be restricted
     *    to [top of parent, bottom of current dungeon]
     * -- If in a dungeon that goes up, the new level is to be restricted
     *    to [top of current dungeon, bottom of parent]
     * -- If in a quest dungeon or similar dungeon entered by portals,
     *    the new level is to be restricted to [top of current dungeon,
     *    bottom of current dungeon]
     * The current behavior is not as sophisticated as that ideal, but is
     * still better what we used to do, which was like this for players
     * but different for monsters for no obvious reason.  Currently, we
     * must explicitly check for special dungeons.  We check for Knox
     * above; endgame is handled in the caller due to its different
     * message ("disoriented").
     * --KAA
     * 3.4.2: explicitly handle quest here too, to fix the problem of
     * monsters sometimes level teleporting out of it into main dungeon.
     * Also prevent monsters reaching the Sanctum prior to invocation.
     */
    if (In_quest(&u.uz)) {
        int bottom = dunlevs_in_dungeon(&u.uz),
            qlocate_depth = qlocate_level.dlevel;

        /* if hero hasn't reached the middle locate level yet,
           no one can randomly teleport past it */
        if (dunlev_reached(&u.uz) < qlocate_depth)
            bottom = qlocate_depth;
        min_depth = gd.dungeons[u.uz.dnum].depth_start;
        max_depth = bottom + (gd.dungeons[u.uz.dnum].depth_start - 1);
    } else {
        min_depth = 1;
        max_depth = dunlevs_in_dungeon(&u.uz)
                    + (gd.dungeons[u.uz.dnum].depth_start - 1);
        /* can't reach Sanctum if the invocation hasn't been performed */
        if (Inhell && !u.uevent.invoked)
            max_depth -= 1;
    }

    /* Get a random value relative to the current dungeon */
    /* Range is 1 to current+3, current not counting */
    nlev = rn2(cur_depth + 3 - min_depth) + min_depth;
    if (nlev >= cur_depth)
        nlev++;

    if (nlev > max_depth) {
        nlev = max_depth;
        /* teleport up if already on bottom */
        if (Is_botlevel(&u.uz))
            nlev -= rnd(3);
    }
    if (nlev < min_depth) {
        nlev = min_depth;
        if (nlev == cur_depth) {
            nlev += rnd(3);
            if (nlev > max_depth)
                nlev = max_depth;
        }
    }
    return nlev;
}

/* you teleport a monster (via wand, spell, or poly'd q.mechanic attack);
   return false iff the attempt fails */
boolean
u_teleport_mon(struct monst* mtmp, boolean give_feedback)
{
    coord cc;

    if (mtmp->ispriest && *in_rooms(mtmp->mx, mtmp->my, TEMPLE)) {
        if (give_feedback)
            pline("%s resists your magic!", Monnam(mtmp));
        return FALSE;
    } else if (engulfing_u(mtmp) && noteleport_level(mtmp)) {
        if (give_feedback)
            You("are no longer inside %s!", mon_nam(mtmp));
        unstuck(mtmp);
        (void) rloc(mtmp, RLOC_MSG);
    } else if (is_rider(mtmp->data) && rn2(13)
               && enexto(&cc, u.ux, u.uy, mtmp->data))
        rloc_to(mtmp, cc.x, cc.y);
    else
        (void) rloc(mtmp, RLOC_MSG);
    return TRUE;
}

/*teleport.c*/

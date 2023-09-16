/* NetHack 3.7	worm.c	$NHDT-Date: 1652689653 2022/05/16 08:27:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.56 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define newseg() (struct wseg *) alloc(sizeof (struct wseg))
#define dealloc_seg(wseg) free((genericptr_t) (wseg))

/* worm segment structure */
struct wseg {
    struct wseg *nseg;
    coordxy wx, wy; /* the segment's position */
};

static void toss_wsegs(struct wseg *, boolean);
static void shrink_worm(int);
#if 0
static void random_dir(int, int, int *, int *);
#endif
static struct wseg *create_worm_tail(int);

/*  Description of long worm implementation.
 *
 *  Each monst struct of the head of a tailed worm has a wormno set to
 *                      1 <= wormno < MAX_NUM_WORMS
 *  If wormno == 0 this does not mean that the monster is not a worm,
 *  it just means that the monster does not have a long worm tail.
 *
 *  The actual segments of a worm are not full blown monst structs.
 *  They are small wseg structs, and their position in the levels.monsters[][]
 *  array is held by the monst struct of the head of the worm.  This makes
 *  things like probing and hit point bookkeeping much easier.
 *
 *  The segments of the long worms on a level are kept as an array of
 *  singly threaded linked lists.  The wormno variable is used as an index
 *  for these segment arrays.
 *
 *  wtails:     The first (starting struct) of a linked list.  This points
 *              to the tail (last) segment of the worm.
 *
 *  wheads:     The last (end) of a linked list of segments.  This points to
 *              the segment that is at the same position as the real monster
 *              (the head).  Note that the segment that wheads[wormno] points
 *              to, is not displayed.  It is simply there to keep track of
 *              where the head came from, so that worm movement and display
 *              are simplified later.
 *              Keeping the head segment of the worm at the end of the list
 *              of tail segments is an endless source of confusion, but it is
 *              necessary.
 *              From now on, we will use "start" and "end" to refer to the
 *              linked list and "head" and "tail" to refer to the worm.
 *
 *  One final worm array is:
 *
 *  wgrowtime:  This tells us when to add another segment to the worm.
 *
 *  When a worm is moved, we add a new segment at the head, and delete the
 *  segment at the tail (unless we want it to grow).  This new head segment is
 *  located in the same square as the actual head of the worm.  If we want
 *  to grow the worm, we don't delete the tail segment, and we give the worm
 *  extra hit points, which possibly go into its maximum.
 *
 *  Non-moving worms (worm_nomove) are assumed to be surrounded by their own
 *  tail, and, thus, shrink instead of grow (as their tails keep going while
 *  their heads are stopped short).  In this case, we delete the last tail
 *  segment, and remove hit points from the worm.
 */

/* restart: worm removal resets these so they don't need to be incorporated
   into 'struct instance_globals g' for potential reinitialization provided
   that old game disposes of monsters properly before starting a new one */
static struct wseg *wheads[MAX_NUM_WORMS] = DUMMY,
                   *wtails[MAX_NUM_WORMS] = DUMMY;
static long wgrowtime[MAX_NUM_WORMS] = DUMMY;

/*
 *  get_wormno()
 *
 *  Find an unused worm tail slot and return the index.  A zero means that
 *  there are no slots available.  This means that the worm head can exist,
 *  it just cannot ever grow a tail.
 *
 *  It, also, means that there is an optimization to made.  The [0] positions
 *  of the arrays are never used.  Meaning, we really *could* have one more
 *  tailed worm on the level, or use a smaller array (using wormno - 1).
 *
 *  Implementation is left to the interested hacker.
 */
int
get_wormno(void)
{
    int new_wormno = 1;

    while (new_wormno < MAX_NUM_WORMS) {
        if (!wheads[new_wormno])
            return new_wormno; /* found empty wtails[] slot at new_wormno */
        new_wormno++;
    }
    return 0; /* level infested with worms */
}

/*
 *  initworm()
 *
 *  Use if (mon->wormno = get_wormno()) before calling this function!
 *
 *  Initialize the worm entry.  This will set up the worm grow time, and
 *  create and initialize the dummy segment for wheads[] and wtails[].
 *
 *  If the worm has no tail (ie get_wormno() fails) then this function need
 *  not be called.
 */
void
initworm(struct monst *worm, int wseg_count)
{
    struct wseg *seg, *new_tail = create_worm_tail(wseg_count);
    int wnum = worm->wormno;

    if (new_tail) {
        wtails[wnum] = new_tail;
        for (seg = new_tail; seg->nseg; seg = seg->nseg)
            continue;
        wheads[wnum] = seg;
    } else {
        wtails[wnum] = wheads[wnum] = seg = newseg();
        seg->nseg = (struct wseg *) 0;
    }
    seg->wx = worm->mx;
    seg->wy = worm->my;
    wgrowtime[wnum] = 0L;
}

/*
 *  toss_wsegs()
 *
 *  Get rid of all worm segments on and following the given pointer curr.
 *  The display may or may not need to be updated as we free the segments.
 */
static
void
toss_wsegs(struct wseg *curr, boolean display_update)
{
    struct wseg *nxtseg;

    while (curr) {
        nxtseg = curr->nseg;

        /* remove from level.monsters[][];
           need to check curr->wx for genocided while migrating_mon */
        if (curr->wx) {
            remove_monster(curr->wx, curr->wy);

            /* update screen before deallocation */
            if (display_update)
                newsym(curr->wx, curr->wy);
        }

        /* free memory used by the segment */
        dealloc_seg(curr);
        curr = nxtseg;
    }
}

/*
 *  shrink_worm()
 *
 *  Remove the tail segment of the worm (the starting segment of the list).
 */
static
void
shrink_worm(int wnum) /* worm number */
{
    struct wseg *seg;

    if (wtails[wnum] == wheads[wnum])
        return; /* no tail */

    seg = wtails[wnum];
    wtails[wnum] = seg->nseg;
    seg->nseg = (struct wseg *) 0;
    toss_wsegs(seg, TRUE);
}

/*
 *  worm_move()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  Move the worm.  Maybe grow.
 */
void
worm_move(struct monst *worm)
{
    struct wseg *seg, *new_seg; /* new segment */
    int wnum = worm->wormno;    /* worm number */

    /*
     *  Place a segment at the old worm head.  The head has already moved.
     */
    seg = wheads[wnum];
    place_worm_seg(worm, seg->wx, seg->wy);
    newsym(seg->wx, seg->wy); /* display the new segment */

    /*
     *  Create a new dummy segment head and place it at the end of the list.
     */
    new_seg = newseg();
    new_seg->wx = worm->mx;
    new_seg->wy = worm->my;
    new_seg->nseg = (struct wseg *) 0;
    seg->nseg = new_seg;    /* attach it to the end of the list */
    wheads[wnum] = new_seg; /* move the end pointer */

    if (wgrowtime[wnum] <= gm.moves) {
        int whplimit, whpcap, prev_mhp, wsegs = count_wsegs(worm);

        /* first set up for the next time to grow */
        if (!wgrowtime[wnum]) {
            /* new worm; usually grow a tail segment on its next turn */
            wgrowtime[wnum] = gm.moves + rnd(5);
        } else {
            int mmove = mcalcmove(worm, FALSE),
                /* prior to 3.7.0, next-grow increment was 3..17 but since
                   it got checked every 4th turn when the speed 3 worm got
                   to move, it was effectively 0..5; also, its usage was
                   'wgrowtime += incr', so often 'wgrowtime' would be
                   exceeded by 'moves' on consecutive turns for the worm,
                   resulting in an excessively rapid growth cycle */
                incr = rn1(10, 2); /* 2..12; after adjusting for long worn
                                    * speed of 3, effective value is 8..48 */

            incr = (incr * NORMAL_SPEED) / max(mmove, 1);
            wgrowtime[wnum] = gm.moves + incr;
        }

        /* increase HP based on number of segments; if it has shrunk, it
           won't gain new HP until regaining previous peak segment count;
           when wounded (whether from damage or from shrinking), the HP
           which might have been 'new' will heal */
        whplimit = !worm->m_lev ? 4 : (8 * (int) worm->m_lev);
        /* note: wsegs includes the hidden segment co-located with the head */
        if (wsegs > 33)
            whplimit += 2 * (wsegs - 33), wsegs = 33;
        if (wsegs > 22)
            whplimit += 4 * (wsegs - 22), wsegs = 22;
        if (wsegs > 11)
            whplimit += 6 * (wsegs - 11), wsegs = 11;
        whplimit += 8 * wsegs;
        if (whplimit > MHPMAX)
            whplimit = MHPMAX;

        prev_mhp = worm->mhp;
        worm->mhp += d(2, 2); /* 2..4, average 3 */
        whpcap = max(whplimit, worm->mhpmax);
        if (worm->mhp < whpcap) {
            /* can't exceed segment-derived limit unless level increase after
               peak tail growth has already done so; when that isn't the case,
               if segment growth exceeds current max HP then increase it */
            if (worm->mhp > whplimit)
                worm->mhp = max(prev_mhp, whplimit);
            if (worm->mhp > worm->mhpmax)
                worm->mhpmax = worm->mhp;
        } else {
            if (worm->mhp > worm->mhpmax)
                worm->mhp = worm->mhpmax;
        }
    } else {
        /* The worm doesn't grow, so the last segment goes away.
           (Done after inserting an extra segment at the head, so it
           isn't getting smaller here, just changing location without
           having to move any of the intermediate segments.) */
        shrink_worm(wnum);
    }
}

/*
 *  worm_nomove()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  The worm don't move so it should shrink.
 */
void
worm_nomove(struct monst *worm)
{
    shrink_worm((int) worm->wormno); /* shrink */

    if (worm->mhp > count_wsegs(worm)) {
        worm->mhp -= d(2, 2); /* 2..4, average 3; note: mhpmax not changed! */
        if (worm->mhp < 1)
            worm->mhp = 1;
    }
}

/*
 *  wormgone()
 *
 *  Kill a worm tail.  Also takes the head off the map.  Caller needs to
 *  keep track of what its coordinates were if planning to put it back.
 *
 *  Should only be called when mon->wormno is non-zero.
 */
void
wormgone(struct monst *worm)
{
    int wnum = worm->wormno;

    if (!wnum) /* note: continuing with wnum==0 runs to completion */
        impossible("wormgone: wormno is 0");

    worm->wormno = 0; /* still a long worm but doesn't grow/shrink anymore */
    /*
     *  This will also remove the real monster (ie 'w') from the its
     *  position in level.monsters[][].  (That happens when removing
     *  the hidden tail segment which is co-located with the head.)
     */
    toss_wsegs(wtails[wnum], TRUE);

    wheads[wnum] = wtails[wnum] = (struct wseg *) 0;
    wgrowtime[wnum] = 0L;

    /* we don't expect to encounter this here but check for it anyway;
       when a long worm gets created by a polymorph zap, it gets flagged
       with MCORPSENM()==PM_LONG_WORM so that the same zap won't trigger
       another polymorph if it hits the new tail */
    if (worm->data == &mons[PM_LONG_WORM] && has_mcorpsenm(worm))
        MCORPSENM(worm) = NON_PM; /* no longer polymorph-proof */
}

/*
 *  wormhitu()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  If the hero is near any part of the worm, the worm will try to attack.
 *  Returns 1 if the worm dies (poly'd hero with passive counter-attack)
 *  or 0 if it doesn't.
 */
int
wormhitu(struct monst *worm)
{
    int wnum = worm->wormno;
    struct wseg *seg;

    /*  This does not work right now because mattacku() thinks that the head
     *  is out of range of the player.  We might try to kludge, and bring
     *  the head within range for a tiny moment, but this needs a bit more
     *  looking at before we decide to do this.
     *
     *  Head has already had a chance to attack, so the dummy tail segment
     *  sharing its location should be skipped.
     */
    for (seg = wtails[wnum]; seg != wheads[wnum]; seg = seg->nseg)
        if (distu(seg->wx, seg->wy) < 3)
            if (mattacku(worm))
                return 1; /* your passive ability killed the worm */
    return 0;
}

/*  cutworm()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  When hitting a worm (worm) at position x, y, with a weapon (weap),
 *  there is a chance that the worm will be cut in half, and a chance
 *  that both halves will survive.
 */
void
cutworm(struct monst *worm, coordxy x, coordxy y,
        boolean cuttier) /* hit is by wielded blade or axe or by thrown axe */
{
    struct wseg *curr, *new_tail;
    struct monst *new_worm;
    int wnum = worm->wormno;
    int cut_chance, new_wnum;

    if (!wnum)
        return; /* bullet proofing */

    if (x == worm->mx && y == worm->my)
        return; /* hit on head */

    /* cutting goes best with a cuttier weapon */
    cut_chance = rnd(20); /* Normally     1-16 does not cut, 17-20 does, */
    if (cuttier)
        cut_chance += 10; /* with a blade 1- 6 does not cut,  7-20 does. */

    if (cut_chance < 17)
        return; /* not good enough */

    /* Find the segment that was attacked. */
    curr = wtails[wnum];

    while ((curr->wx != x) || (curr->wy != y)) {
        curr = curr->nseg;
        if (!curr) {
            impossible("cutworm: no segment at (%d,%d)", (int) x, (int) y);
            return;
        }
    }

    /* If this is the tail segment, then the worm just loses it. */
    if (curr == wtails[wnum]) {
        shrink_worm(wnum);
        return;
    }

    /*
     *  Split the worm.  The tail for the new worm is the old worm's tail.
     *  The tail for the old worm is the segment that follows "curr",
     *  and "curr" becomes the dummy segment under the new head.
     */
    new_tail = wtails[wnum];
    wtails[wnum] = curr->nseg;
    curr->nseg = (struct wseg *) 0; /* split the worm */

    /*
     *  At this point, the old worm is correct.  Any new worm will have
     *  it's head at "curr" and its tail at "new_tail".  The old worm
     *  must be at least level 3 in order to produce a new worm.
     */
    new_worm = 0;
    new_wnum = (worm->m_lev >= 3 && !rn2(3)) ? get_wormno() : 0;
    if (new_wnum) {
        remove_monster(x, y); /* clone_mon puts new head here */
        /* clone_mon() will fail if enough long worms have been
           created to have them be marked as extinct or if the hit
           that cut the current one has dropped it down to 1 HP */
        new_worm = clone_mon(worm, x, y);
    }

    /* Sometimes the tail end dies. */
    if (!new_worm) {
        place_worm_seg(worm, x, y); /* place the "head" segment back */
        if (gc.context.mon_moving) {
            if (canspotmon(worm))
                pline("Part of %s tail has been cut off.",
                      s_suffix(mon_nam(worm)));
        } else
            You("cut part of the tail off of %s.", mon_nam(worm));
        toss_wsegs(new_tail, TRUE);
        if (worm->mhp > 1)
            worm->mhp /= 2;
        return;
    }

    new_worm->wormno = new_wnum; /* affix new worm number */
    new_worm->mcloned = 0;       /* treat second worm as a normal monster */

    /* Devalue the monster level of both halves of the worm.
       Note: m_lev is always at least 3 in order to get this far. */
    worm->m_lev = max((unsigned) worm->m_lev - 2, 3);
    new_worm->m_lev = worm->m_lev;

    /* Calculate the lower-level mhp; use <N>d8 for long worms.
       Can't use newmonhp() here because it would reset m_lev. */
    new_worm->mhpmax = new_worm->mhp = d((int) new_worm->m_lev, 8);
    worm->mhpmax = d((int) worm->m_lev, 8); /* new maxHP for old worm */
    if (worm->mhpmax < worm->mhp)
        worm->mhp = worm->mhpmax;

    wtails[new_wnum] = new_tail; /* We've got all the info right now */
    wheads[new_wnum] = curr;     /* so we can do this faster than    */
    wgrowtime[new_wnum] = 0L;    /* trying to call initworm().       */

    /* Place the new monster at all the segment locations. */
    place_wsegs(new_worm, worm);

    if (gc.context.mon_moving)
        pline("%s is cut in half.", Monnam(worm));
    else
        You("cut %s in half.", mon_nam(worm));
}

/*
 *  see_wsegs()
 *
 *  Refresh all of the segments of the given worm.  This is only called
 *  from see_monster() in display.c or when a monster goes minvis.  It
 *  is located here for modularity.
 */
void
see_wsegs(struct monst *worm)
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr != wheads[worm->wormno]) {
        newsym(curr->wx, curr->wy);
        curr = curr->nseg;
    }
}

/*
 *  detect_wsegs()
 *
 *  Display all of the segments of the given worm for detection.
 */
void
detect_wsegs(struct monst *worm, boolean use_detection_glyph)
{
    int num;
    struct wseg *curr = wtails[worm->wormno];
    int what_tail = what_mon(PM_LONG_WORM_TAIL, newsym_rn2);

    while (curr != wheads[worm->wormno]) {
        num = use_detection_glyph ? detected_monnum_to_glyph(what_tail,
                    worm->female ? FEMALE : MALE)
              : worm->mtame ? petnum_to_glyph(what_tail,
                    worm->female ? FEMALE : MALE)
                : monnum_to_glyph(what_tail,
                    worm->female ? FEMALE : MALE);
        show_glyph(curr->wx, curr->wy, num);
        curr = curr->nseg;
    }
}

/*
 *  save_worm()
 *
 *  Save the worm information for later use.  The count is the number
 *  of segments, including the dummy.  Called from save.c.
 */
void
save_worm(NHFILE *nhfp)
{
    int i;
    int count;
    struct wseg *curr, *temp;

    if (perform_bwrite(nhfp)) {
        for (i = 1; i < MAX_NUM_WORMS; i++) {
            for (count = 0, curr = wtails[i]; curr; curr = curr->nseg)
                count++;
            /* Save number of segments */
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) &count, sizeof count);
            /* Save segment locations of the monster. */
            if (count) {
                for (curr = wtails[i]; curr; curr = curr->nseg) {
                    if (nhfp->structlevel) {
                        bwrite(nhfp->fd, (genericptr_t) &(curr->wx),
                               sizeof curr->wx);
                        bwrite(nhfp->fd, (genericptr_t) &(curr->wy),
                               sizeof curr->wy);
                    }
                }
            }
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) wgrowtime, sizeof wgrowtime);
        }
    }

    if (release_data(nhfp)) {
        /* Free the segments only.  savemonchn() will take care of the
         * monsters. */
        for (i = 1; i < MAX_NUM_WORMS; i++) {
            if (!(curr = wtails[i]))
                continue;

            while (curr) {
                temp = curr->nseg;
                dealloc_seg(curr); /* free the segment */
                curr = temp;
            }
            wheads[i] = wtails[i] = (struct wseg *) 0;
            wgrowtime[i] = 0L;
        }
    }
}

/*
 *  rest_worm()
 *
 *  Restore the worm information from the save file.  Called from restore.c
 */
void
rest_worm(NHFILE *nhfp)
{
    int i, j, count = 0;
    struct wseg *curr, *temp;

    for (i = 1; i < MAX_NUM_WORMS; i++) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &count, sizeof count);

        /* Get the segments. */
        for (curr = (struct wseg *) 0, j = 0; j < count; j++) {
            temp = newseg();
            temp->nseg = (struct wseg *) 0;
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) &(temp->wx), sizeof temp->wx);
                mread(nhfp->fd, (genericptr_t) &(temp->wy), sizeof temp->wy);
            }
            if (curr)
                curr->nseg = temp;
            else
                wtails[i] = temp;
            curr = temp;
        }
        wheads[i] = curr;
    }
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) wgrowtime, sizeof wgrowtime);
    }
}

/*
 *  place_wsegs()
 *
 *  Place the segments of the given worm.  Called from restore.c
 *  and from replmon() in mon.c.
 *  If oldworm is not NULL, assumes the oldworm segments are on map
 *  in the same location as worm segments
 */
void
place_wsegs(struct monst *worm, struct monst *oldworm)
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr != wheads[worm->wormno]) {
        coordxy x = curr->wx, y = curr->wy;
        struct monst *mtmp = m_at(x, y);

        if (oldworm && mtmp == oldworm)
            remove_monster(x, y);
        else if (mtmp)
            impossible("placing worm seg <%d,%d> over another mon", x, y);
        else if (oldworm)
            impossible("replacing worm seg <%d,%d> on empty spot", x, y);

        place_worm_seg(worm, x, y);
        curr = curr->nseg;
    }
    /* head segment is co-located with worm itself so not placed on the map */
    curr->wx = worm->mx, curr->wy = worm->my;
}

/* called from mon_sanity_check(mon.c) */
void
sanity_check_worm(struct monst *worm)
{
    struct wseg *curr;
    int wnum, x, y;

    if (!worm) {
        impossible("worm_sanity: null monster!");
        return;
    }
    /* note: wormno can't be less than 0 (unsigned bit field) and can't
       be greater that MAX_NUM_WORMS - 1 (which uses all available bits)
       so checking for 0 is all we can manage for wormno validation;
       since caller has already done that, this is rather pointless... */
    if (!worm->wormno) {
        impossible("worm_sanity: not a worm!");
        return;
    }

    wnum = worm->wormno;
    if (!wtails[wnum] || !wheads[wnum]) {
        impossible("wormno %d is set without proper tail", wnum);
        return;
    }
    /* if worm is migrating, we can't check its segments against the map */
    if (!worm->mx)
        return;

    curr = wtails[wnum];
    while (curr != wheads[wnum]) {
        x = curr->wx, y = curr->wy;
        if (!isok(x, y))
            impossible("worm seg not isok <%d,%d>", x, y);
        else if (gl.level.monsters[x][y] != worm)
            impossible("mon (%s) at seg location is not worm (%s)",
                       fmt_ptr((genericptr_t) gl.level.monsters[x][y]),
                       fmt_ptr((genericptr_t) worm));

        curr = curr->nseg;
    }
}

/* called from mon_sanity_check(mon.c) */
void
wormno_sanity_check(void)
{
#ifdef EXTRA_SANITY_CHECKS
    struct wseg *seg;
    int wh = 0, wt = 0;

    /* checking tail management, not a particular monster; since wormno==0
       means 'not a worm', wheads[0] and wtails[0] should always be empty;
       note: if erroneously non-Null, tail segment count will include the
       extra segment for the worm's head that isn't shown on the map */
    for (seg = wheads[0]; seg; seg = seg->nseg)
        ++wh;
    for (seg = wtails[0]; seg; seg = seg->nseg)
        ++wt;
    if (wh || wt) {
        impossible(
        "phantom worm tail #0 [head=%s, %d segment%s; tail=%s, %d segment%s]",
                   fmt_ptr(wheads[0]), wh, plur(wh),
                   fmt_ptr(wtails[0]), wt, plur(wt));
    }
#endif /* EXTRA_SANITY_CHECKS */
}

/*
 *  remove_worm()
 *
 *  This function is equivalent to the remove_monster #define in
 *  rm.h, only it will take the worm *and* tail out of the levels array.
 *  It does not get rid of (dealloc) the worm tail structures, and it does
 *  not remove the mon from the fmon chain.
 */
void
remove_worm(struct monst *worm)
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr) {
        if (curr->wx) {
            remove_monster(curr->wx, curr->wy);
            newsym(curr->wx, curr->wy);
            curr->wx = 0;
        }
        curr = curr->nseg;
    }
}

/*
 *  place_worm_tail_randomly()
 *
 *  Place a worm tail somewhere on a level behind the head.
 *  This routine essentially reverses the order of the wsegs from head
 *  to tail while placing them.
 *  x, and y are most likely the worm->mx, and worm->my, but don't *need* to
 *  be, if somehow the head is disjoint from the tail.
 */
void
place_worm_tail_randomly(struct monst *worm, coordxy x, coordxy y)
{
    int wnum = worm->wormno;
    struct wseg *curr = wtails[wnum];
    struct wseg *new_tail;
    int ox = x, oy = y;

    if (wnum && (!wtails[wnum] || !wheads[wnum])) {
        impossible("place_worm_tail_randomly: wormno is set without a tail!");
        return;
    }
    if (wtails[wnum] == wheads[wnum]) {
        /* single segment, co-located with worm;
           should either have same coordinates or have seg->wx==0
           to indicate that it is not currently on the map */
        if (curr->wx && (curr->wx != worm->mx || curr->wy != worm->my)) {
            impossible(
         "place_worm_tail_randomly: tail segment at <%d,%d>, worm at <%d,%d>",
                       curr->wx, curr->wy, worm->mx, worm->my);
            if (m_at(curr->wx, curr->wy) == worm)
                remove_monster(curr->wx, curr->wy);
        }
        curr->wx = worm->mx, curr->wy = worm->my;
        return;
    }
    /* remove head segment from map in case we end up calling toss_wsegs();
       if it doesn't get tossed, it will become the final tail segment and
       get new coordinates */
    wheads[wnum]->wx = wheads[wnum]->wy = 0;

    wheads[wnum] = new_tail = curr;
    curr = curr->nseg;
    new_tail->nseg = (struct wseg *) 0;
    new_tail->wx = x;
    new_tail->wy = y;

    while (curr) {
        coordxy nx = ox, ny = oy;

        if (rnd_nextto_goodpos(&nx, &ny, worm)) {
            place_worm_seg(worm, nx, ny);
            curr->wx = (coordxy) (ox = nx);
            curr->wy = (coordxy) (oy = ny);
            wtails[wnum] = curr;
            curr = curr->nseg;
            wtails[wnum]->nseg = new_tail;
            new_tail = wtails[wnum];
            newsym(nx, ny);
        } else {
            /* Oops.  Truncate because there is no place for rest of it. */
            toss_wsegs(curr, FALSE);
            curr = (struct wseg *) 0;
        }
    }
}

#if 0
/*
 * Given a coordinate x, y.
 * return in *nx, *ny, the coordinates of one of the <= 8 squares adjoining.
 *
 * This function, and the loop it serves, could be eliminated by coding
 * enexto() with a search radius.
 */
static void
random_dir(int x, int y, int *nx, int *ny)
{
    *nx = x + (x > 1                /* extreme left ? */
               ? (x < COLNO - 1     /* extreme right ? */
                  ? (rn2(3) - 1)    /* neither so +1, 0, or -1 */
                  : -rn2(2))        /* right edge, use -1 or 0 */
               : rn2(2));           /* left edge, use 0 or 1 */
    if (*nx != x) /* if x has changed, do same thing with y */
        *ny = y + (y > 0             /* y==0 is ok (x==0 is not) */
                   ? (y < ROWNO - 1
                      ? (rn2(3) - 1)
                      : -rn2(2))
                   : rn2(2));
    else /* when x has remained the same, force y to change */
        *ny = y + (y > 0
                   ? (y < ROWNO - 1
                      ? (rn2(2) ? 1 : -1)   /* not at edge, so +1 or -1 */
                      : -1)                 /* bottom, use -1 */
                   : 1);                    /* top, use +1 */
}
#endif

/* for size_monst(cmd.c) to support #stats */
int
size_wseg(struct monst *worm)
{
    return (int) (count_wsegs(worm) * sizeof (struct wseg));
}

/*  count_wsegs()
 *  returns the number of segments that a worm has.
 */
int
count_wsegs(struct monst *mtmp)
{
    int i = 0;
    struct wseg *curr;

    if (mtmp->wormno) {
        for (curr = wtails[mtmp->wormno]->nseg; curr; curr = curr->nseg)
            i++;
    }
    return i;
}

/*  create_worm_tail()
 *  will create a worm tail chain of (num_segs + 1) and return pointer to it.
 */
static
struct wseg *
create_worm_tail(int num_segs)
{
    int i = 0;
    struct wseg *new_tail, *curr;

    if (!num_segs)
        return (struct wseg *) 0;

    new_tail = curr = newseg();
    curr->nseg = (struct wseg *) 0;
    curr->wx = 0;
    curr->wy = 0;

    while (i < num_segs) {
        curr->nseg = newseg();
        curr = curr->nseg;
        curr->nseg = (struct wseg *) 0;
        curr->wx = 0;
        curr->wy = 0;
        i++;
    }

    return new_tail;
}

/*  worm_known()
 *  Is any segment of this worm in viewing range?  Note: caller must check
 *  invisibility and telepathy (which should only show the head anyway).
 *  Mostly used in the canseemon() macro.
 */
boolean
worm_known(struct monst *worm)
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr) {
        if (cansee(curr->wx, curr->wy))
            return TRUE;
        curr = curr->nseg;
    }
    return FALSE;
}

/* would moving from <x1,y1> to <x2,y2> involve passing between two
   consecutive segments of the same worm? */
boolean
worm_cross(int x1, int y1, int x2, int y2)
{
    struct monst *worm;
    struct wseg *curr, *wnxt;

    /*
     * With digits representing relative sequence number of the segments,
     * returns true when testing between @ and ? (passes through worm's
     * body), false between @ and ! (stays on same side of worm).
     *  .w1?..
     *  ..@2..
     *  .65!3.
     *  ...4..
     */

    if (distmin(x1, y1, x2, y2) != 1) {
        impossible("worm_cross checking for non-adjacent location?");
        return FALSE;
    }
    /* attempting to pass between worm segs is only relevant for diagonal */
    if (x1 == x2 || y1 == y2)
        return FALSE;

    /* is the same monster at <x1,y2> and at <x2,y1>? */
    worm = m_at(x1, y2);
    if (!worm || m_at(x2, y1) != worm)
        return FALSE;

    /* same monster is at both adjacent spots, so must be a worm; we need
       to figure out if the two spots are occupied by consecutive segments */
    for (curr = wtails[worm->wormno]; curr; curr = wnxt) {
        wnxt = curr->nseg;
        if (!wnxt)
            break; /* no next segment; can't continue */

        /* we don't know which of <x1,y2> or <x2,y1> we'll hit first, but
           whichever it is, they're consecutive iff next seg is the other */
        if (curr->wx == x1 && curr->wy == y2)
            return (boolean) (wnxt->wx == x2 && wnxt->wy == y1);
        if (curr->wx == x2 && curr->wy == y1)
            return (boolean) (wnxt->wx == x1 && wnxt->wy == y2);
    }
    /* should never reach here... */
    return FALSE;
}

/* construct an index number for a worm tail segment */
int
wseg_at(struct monst *worm, int x, int y)
{
    int res = 0;

    if (worm && worm->wormno && m_at(x, y) == worm) {
        struct wseg *curr;
        int i, n;
        coordxy wx = (coordxy) x, wy = (coordxy) y;

        for (i = 0, curr = wtails[worm->wormno]; curr; curr = curr->nseg) {
            if (curr->wx == wx && curr->wy == wy)
                break;
            ++i;
        }
        for (n = i; curr; curr = curr->nseg)
            ++n;
        res = n - i;
    }
    return res;
}

void
flip_worm_segs_vertical(struct monst *worm, int miny, int maxy)
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr) {
        curr->wy = (maxy - curr->wy + miny);
        curr = curr->nseg;
    }
}

void
flip_worm_segs_horizontal(struct monst *worm, int minx, int maxx)
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr) {
        curr->wx = (maxx - curr->wx + minx);
        curr = curr->nseg;
    }
}

/*worm.c*/

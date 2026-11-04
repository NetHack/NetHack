/* The Battle of Hinnom and the Valley of Berachah. */

#include "hack.h"

static boolean swap_pending = FALSE;
static int raze_turns;
static coordxy raze_x, raze_y;

static const char *const lever_metal[5] = {
    "iron", "copper", "brass", "silver", "gold"
};

/* des.map({ halign = "left" }) puts map column 0 at x = 1.
   Lua coordinates are shifted by that. These literals are the
   map columns, so add the same shift before using them on levl[]. */
#define HINNOM_XO 1
#define HX(c) ((coordxy) ((c) + HINNOM_XO))

static const coord lever_at[5] = {
    { HX(55), 14 }, { HX(57), 14 }, { HX(61), 14 },
    { HX(56), 15 }, { HX(60), 15 }
};

boolean
altar_is_good(coordxy x, coordxy y)
{
    if (!isok(x, y) || !IS_ALTAR(levl[x][y].typ))
        return FALSE;
    return (boolean) (Amask2align(levl[x][y].altarmask & AM_MASK) == A_LAWFUL);
}

boolean
hinnom_portal_room(coordxy x, coordxy y)
{
    return (boolean) (x >= HX(1) && x <= HX(13) && y >= 1 && y <= 6);
}

/* Michael and the angels with him meditate until the force field
   drops, or until this one is no longer at peace. #chat clears the
   wait bit; this puts it back on the next turn. A hurt, hostile, or
   conflicted one is released for good, so healing does not plant
   him again. The Valley of Berachah is after the battle. */
void
hinnom_host_hold(struct monst *mtmp)
{
    if (!mtmp || !Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    if (mtmp->data != &mons[PM_MICHAEL] && mtmp->data != &mons[PM_ANGEL])
        return;
    /* Hurt or hostile stays released after the hit points return.
       Conflict lets him move only while it lasts. */
    if (!mtmp->mpeaceful || mtmp->mhp < mtmp->mhpmax)
        mtmp->mstrategy |= STRAT_HINNOM_RELEASED;
    if (!hinnom_field_up() || Conflict
        || (mtmp->mstrategy & STRAT_HINNOM_RELEASED)) {
        mtmp->mstrategy &= ~STRAT_WAITFORU;
        return;
    }
    mtmp->mstrategy |= STRAT_WAITFORU;
    mtmp->mpeaceful = 1;
    mtmp->msleeping = 0;
}

boolean
hinnom_inside_field(coordxy x, coordxy y)
{
    return (boolean) (x >= HX(42) && x <= HX(76) && y >= 2 && y <= 18);
}

boolean
hinnom_gate_square(coordxy x, coordxy y)
{
    return (boolean) (Is_hinnom_level(&u.uz) && !u.uevent.berachah
                      && x == HX(41) && y == 10);
}

boolean
hinnom_backroom_door(coordxy x, coordxy y)
{
    return (boolean) (Is_hinnom_level(&u.uz) && !u.uevent.berachah
                      && x == HX(68) && y == 13);
}

boolean
hinnom_field_up(void)
{
    coordxy x, y;

    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return FALSE;
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (levl[x][y].typ == IRONBARS)
                return TRUE;
    return FALSE;
}

/* A human who serves the Great Dragon: a temple priest, a roving
   aligned priest or high priest of that god, or the fortress staff.
   Children are not servants. */
boolean
serves_great_dragon(struct monst *mtmp)
{
    if (!mtmp || !is_human(mtmp->data))
        return FALSE;
    if (mtmp->data == &mons[PM_CHILD])
        return FALSE;
    if ((mtmp->ispriest || mtmp->isminion)
        && mon_aligntyp(mtmp) == A_NONE)
        return TRUE;
    if (mtmp->data == &mons[PM_FORTRESS_GUARD]
        || mtmp->data == &mons[PM_DOCTOR])
        return TRUE;
    return FALSE;
}

boolean
fortress_monster(struct monst *mtmp)
{
    if (!mtmp || !Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return FALSE;
    if (!hinnom_inside_field(mtmp->mx, mtmp->my))
        return FALSE;
    if (mtmp->data == &mons[PM_MICHAEL] || mtmp->data == &mons[PM_ANGEL]
        || mtmp->data == &mons[PM_CHILD])
        return FALSE;
    return TRUE;
}

boolean
moloch_disguise_holds(void)
{
    return (boolean) (Is_hinnom_level(&u.uz) && !u.uevent.berachah
                      && uarmc && uarmc->otyp == HIGH_PRIEST_CLOAK
                      && !u.uevent.hinnom_alarm);
}

void
hinnom_alarm_fortress(void)
{
    struct monst *mtmp;

    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    if (u.uevent.hinnom_alarm)
        return;
    u.uevent.hinnom_alarm = 1;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || !fortress_monster(mtmp))
            continue;
        mtmp->mpeaceful = 0;
        mtmp->msleeping = 0;
        mtmp->mstrategy &= ~STRAT_WAITMASK;
    }
}

static void
anger_gate_guards(void)
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || mtmp->data != &mons[PM_FORTRESS_GUARD])
            continue;
        if (hinnom_inside_field(mtmp->mx, mtmp->my))
            continue;
        mtmp->mpeaceful = 0;
        mtmp->msleeping = 0;
        mtmp->mstrategy &= ~STRAT_WAITMASK;
    }
}

/* Burn engraving on a floor square that shares a side with a wall.
   It does not fade, and the map shows the engraving once seen. */
static void
carve_wall_clue(coordxy x, coordxy y, const char *text)
{
    struct engr *ep;

    if (!isok(x, y))
        return;
    make_engr_at(x, y, text, (char *) 0, 0L, BURN);
    ep = engr_at(x, y);
    if (ep)
        ep->nowipeout = 1;
}

/* Floor objects inside the field, other than the lab supplies and
   the quest items, are generation clutter. Monster inventories stay. */
void
hinnom_strip_floor_clutter(void)
{
    coordxy x, y;
    struct obj *obj, *nobj;

    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            if (!hinnom_inside_field(x, y))
                continue;
            for (obj = svl.level.objects[x][y]; obj; obj = nobj) {
                nobj = obj->nexthere;
                if (obj->otyp == POT_BOOZE || obj->otyp == POT_FRUIT_JUICE
                    || obj->otyp == POT_OIL || obj->otyp == SACK
                    || obj->otyp == FLINT_AND_TINDER
                    || obj->otyp == KEY_TO_HINNOM_GATE
                    || obj->otyp == KEY_TO_SANCTUM
                    || obj->otyp == HIGH_PRIEST_CLOAK)
                    continue;
                obj_extract_self(obj);
                obfree(obj, (struct obj *) 0);
            }
        }
}

static void
lower_hinnom_bridge(void)
{
    coordxy x = HX(43), y = 10;

    /* Span is the lava square; the portcullis is one step east.
       Quiet: the placeholder line already says the bridge lowers. */
    if (isok(x, y) && levl[x][y].typ == DRAWBRIDGE_UP)
        open_drawbridge(x, y, TRUE);
}

void
hinnom_forced_entry(void)
{
    if (levl[HX(41)][10].typ == DOOR)
        levl[HX(41)][10].doormask = D_ISOPEN;
    newsym(HX(41), 10);
    unblock_point(HX(41), 10);
    lower_hinnom_bridge();
    hinnom_alarm_fortress();
    anger_gate_guards();
}

boolean
hinnom_impassable_door(coordxy x, coordxy y)
{
    if (hinnom_gate_square(x, y) && hinnom_field_up())
        return TRUE;
    if (hinnom_backroom_door(x, y) && !u.uevent.hinnom_razed)
        return TRUE;
    return FALSE;
}

boolean
hinnom_blocks(coordxy x, coordxy y)
{
    if (hinnom_gate_square(x, y) && hinnom_field_up()) {
        pline("The barrier hums. It will not let you pass."); /* PLACEHOLDER */
        return TRUE;
    }
    if (hinnom_backroom_door(x, y) && !u.uevent.hinnom_razed) {
        pline("The door will not yield while the altar of evil stands."); /* PLACEHOLDER */
        return TRUE;
    }
    return FALSE;
}

static struct trap *
lever_of_rank(int rank)
{
    struct trap *t;

    for (t = gf.ftrap; t; t = t->ntrap)
        if (t->ttyp == POWER_LEVER && t->launch.x == rank)
            return t;
    return (struct trap *) 0;
}

static const char *
metal_of_rank(int rank)
{
    struct trap *t = lever_of_rank(rank);

    if (!t || t->launch.y < 0 || t->launch.y > 4)
        return "iron";
    return lever_metal[t->launch.y];
}

static void
reset_levers(void)
{
    struct trap *t;

    for (t = gf.ftrap; t; t = t->ntrap)
        if (t->ttyp == POWER_LEVER)
            t->once = 0;
}

static int
levers_set(void)
{
    struct trap *t;
    int n = 0;

    for (t = gf.ftrap; t; t = t->ntrap)
        if (t->ttyp == POWER_LEVER && t->once)
            n++;
    return n;
}

static void
move_angels_to_gate(void)
{
    struct monst *mtmp;
    coordxy spots[5][2] = { { HX(46), 10 }, { HX(46), 9 }, { HX(46), 11 },
                            { HX(47), 9 }, { HX(47), 11 } };
    int i = 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->data != &mons[PM_MICHAEL] && mtmp->data != &mons[PM_ANGEL])
            continue;
        if (i < 5 && !m_at(spots[i][0], spots[i][1]))
            rloc_to(mtmp, spots[i][0], spots[i][1]);
        else
            mnexto(mtmp, RLOC_NOMSG);
        i++;
    }
    pline("%s and his angels charge the fortress!",
          pmname(&mons[PM_MICHAEL], MALE)); /* PLACEHOLDER */
}

static void
hinnom_field_down(void)
{
    coordxy x, y;

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (levl[x][y].typ == IRONBARS) {
                levl[x][y].typ = ROOM;
                newsym(x, y);
            }
    pline("The hum dies. The barrier falls."); /* PLACEHOLDER */
    hinnom_alarm_fortress();
    move_angels_to_gate();
}

int
hinnom_pull_lever(struct trap *trap)
{
    char qbuf[QBUFSZ];
    int progress, rank;
    const char *metal;

    if (!trap || trap->ttyp != POWER_LEVER)
        return Trap_Effect_Finished;
    if (Levitation || Flying) {
        You("cannot reach the floor.");
        return Trap_Effect_Finished;
    }
    metal = (trap->launch.y >= 0 && trap->launch.y <= 4)
                ? lever_metal[trap->launch.y] : "iron";
    if (!hinnom_field_up()) {
        pline("The lever is dead."); /* PLACEHOLDER */
        return Trap_Effect_Finished;
    }
    Sprintf(qbuf, "Throw the %s lever?", metal);
    if (y_n(qbuf) != 'y')
        return Trap_Effect_Finished;
    progress = levers_set();
    rank = trap->launch.x;
    if (rank == progress + 1) {
        trap->once = 1;
        pline("The lever locks into place with a heavy clunk."); /* PLACEHOLDER */
        if (levers_set() >= 5)
            hinnom_field_down();
    } else {
        if (Shock_resistance) {
            shieldeff(u.ux, u.uy);
            pline("Power crackles around you harmlessly. The levers snap back."); /* PLACEHOLDER */
        } else {
            pline("A surge of power jolts you! The levers snap back."); /* PLACEHOLDER */
            losehp(d(4, 6), "a power lever", KILLED_BY_AN);
        }
        reset_levers();
    }
    return Trap_Effect_Finished;
}

void
hinnom_setup(void)
{
    struct trap *t;
    struct monst *mtmp;
    int order[5], i, j, rank;
    char buf[BUFSZ];

    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    for (i = 0; i < 5; i++)
        order[i] = i;
    for (i = 4; i > 0; i--) {
        j = rn2(i + 1);
        rank = order[i];
        order[i] = order[j];
        order[j] = rank;
    }
    for (t = gf.ftrap; t; t = t->ntrap) {
        if (t->ttyp != POWER_LEVER)
            continue;
        t->tseen = 1;
        t->once = 0;
        t->launch.y = 0;
        for (i = 0; i < 5; i++)
            if (t->tx == lever_at[i].x && t->ty == lever_at[i].y)
                t->launch.y = i;
        t->launch.x = 0;
        for (i = 0; i < 5; i++)
            if (order[i] == t->launch.y)
                t->launch.x = i + 1;
    }
    /* Burned into the floor square against a wall, so the engraving
       glyph shows as soon as that square is seen. Fourth is the
       northwest corner of the lab, under the north wall. Fifth is
       the lever room, against the south wall between the levers. */
    Sprintf(buf, "Fourth: %s.", metal_of_rank(4));
    carve_wall_clue(HX(55), 5, buf);
    Sprintf(buf, "%s last, or the power bites.", metal_of_rank(5));
    buf[0] = highc(buf[0]);
    carve_wall_clue(HX(58), 15, buf);

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->data == &mons[PM_ANGEL]) {
            if (!mtmp->isminion)
                newemin(mtmp);
            mtmp->isminion = 1;
            if (EMIN(mtmp)) {
                EMIN(mtmp)->min_align = A_LAWFUL;
                EMIN(mtmp)->renegade = 0;
            }
            mtmp->mpeaceful = 1;
        }
        if (mtmp->data == &mons[PM_MICHAEL] || mtmp->data == &mons[PM_ANGEL]) {
            mtmp->mpeaceful = 1;
            mtmp->msleeping = 0;
            mtmp->mflee = 0;
            mtmp->mstrategy |= STRAT_WAITFORU;
        }
        if (fortress_monster(mtmp) || mtmp->data == &mons[PM_FORTRESS_GUARD]
            || mtmp->data == &mons[PM_CHILD]
            || mtmp->data == &mons[PM_ALIGNED_CLERIC]
            || mtmp->data == &mons[PM_DOCTOR]
            || mtmp->data == &mons[PM_MOLOCH]) {
            mtmp->mpeaceful = 1;
            mtmp->msleeping = 0;
            mtmp->mflee = 0;
            /* Meditating, not asleep, so #chat still works. */
            mtmp->mstrategy |= STRAT_WAITFORU;
        }
    }
    hinnom_strip_floor_clutter();
}

void
berachah_setup(void)
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        mtmp->mpeaceful = 1;
        if (mtmp->data == &mons[PM_ANGEL]) {
            if (!mtmp->isminion)
                newemin(mtmp);
            mtmp->isminion = 1;
            if (EMIN(mtmp)) {
                EMIN(mtmp)->min_align = A_LAWFUL;
                EMIN(mtmp)->renegade = 0;
            }
        }
    }
}

void
hinnom_altar_razed(void)
{
    struct monst *mtmp;

    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    if (u.uevent.hinnom_razed)
        return;
    u.uevent.hinnom_razed = 1;
    svl.level.flags.rndmongen = 0;
    if (isok(HX(68), 13) && levl[HX(68)][13].typ == DOOR) {
        levl[HX(68)][13].doormask = D_CLOSED;
        newsym(HX(68), 13);
    }
    pline("Somewhere nearby, a heavy bolt slides back."); /* PLACEHOLDER */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || mtmp->data != &mons[PM_MOLOCH])
            continue;
        mtmp->mhp = max(1, mtmp->mhp / 4);
        if (canseemon(mtmp))
            pline("Moloch staggers as his altar falls!"); /* PLACEHOLDER */
        break;
    }
}

static int
raze_occupy(void)
{
    if (!IS_ALTAR(levl[u.ux][u.uy].typ) || u.ux != raze_x || u.uy != raze_y)
        return 0;
    if (--raze_turns > 0)
        return 1;
    raze_altar(raze_x, raze_y, TRUE);
    return 0;
}

void
raze_altar(coordxy x, coordxy y, boolean by_hero)
{
    int n, i;

    if (!isok(x, y) || !IS_ALTAR(levl[x][y].typ))
        return;
    if (levl[x][y].altarmask & AM_SANCTUM)
        return;
    if (by_hero)
        angry_priest();
    levl[x][y].typ = ROOM;
    levl[x][y].altarmask = 0;
    newsym(x, y);
    n = rn1(3, 2);
    for (i = 0; i < n; i++)
        (void) mksobj_at(ROCK, x, y, TRUE, FALSE);
    if (by_hero) {
        adjalign(5);
        change_luck(1);
        livelog_printf(LL_ACHIEVE, "razed an altar of evil");
        pline("You raze the altar of evil to the ground!"); /* PLACEHOLDER */
        pline("But thus shall ye deal with them; ye shall destroy their altars, and break down their images, and cut down their groves, and burn their graven images with fire. (Deuteronomy 7:5)");
    }
    if (Is_hinnom_level(&u.uz))
        hinnom_altar_razed();
}

int
doraze(void)
{
    if (!IS_ALTAR(levl[u.ux][u.uy].typ) || u.uswallow) {
        You("are not %s an altar.",
            (Levitation || Flying) ? "over" : "on");
        return ECMD_OK;
    }
    if (Confusion || Stunned) {
        You("are too impaired to perform the rite.");
        return ECMD_OK;
    }
    if (altar_is_good(u.ux, u.uy)) {
        pline("You will not lift a hand against the altar of the Lord."); /* PLACEHOLDER */
        return ECMD_OK;
    }
    if (levl[u.ux][u.uy].altarmask & AM_SANCTUM) {
        pline("This altar is beyond your strength to raze."); /* PLACEHOLDER */
        return ECMD_OK;
    }
    raze_x = u.ux;
    raze_y = u.uy;
    raze_turns = 5;
    set_occupation(raze_occupy, "razing an altar", 0);
    return ECMD_TIME;
}

boolean
hinnom_dest_clamp(d_level *newlevel)
{
    if (wizard)
        return FALSE;
    if (!u.uevent.gehennom_entered && In_hell(newlevel)
        && !Is_valley(newlevel))
        assign_level(newlevel, &valley_level);
    if (!u.uevent.hinnom_razed && In_hell(newlevel)
        && newlevel->dnum == hinnom_level.dnum
        && dunlev(newlevel) > dunlev(&hinnom_level)) {
        boolean below = (u.uz.dnum == hinnom_level.dnum
                         && dunlev(&u.uz) > dunlev(&hinnom_level));

        if (!below) {
            if (Is_hinnom_level(&u.uz)) {
                You("feel very disoriented for a moment.");
                return TRUE;
            }
            assign_level(newlevel, &hinnom_level);
        }
    }
    return FALSE;
}

void
hinnom_begin_swap(void)
{
    struct monst *mtmp;
    struct obj *obj, *nobj;
    coordxy x, y;
    int kids = 0;

    swap_pending = FALSE;
    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    if (!u.uevent.hinnom_razed || svm.mvitals[PM_MOLOCH].died <= 0)
        return;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp) && mtmp->data == &mons[PM_CHILD])
            kids++;
    if (kids) {
        change_luck(kids);
        adjalign(kids);
    }
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            for (obj = svl.level.objects[x][y]; obj; obj = nobj) {
                nobj = obj->nexthere;
                if (obj->where != OBJ_FLOOR)
                    continue;
                obj_extract_self(obj);
                add_to_migration(obj);
                obj->ox = x;
                obj->oy = y;
                obj->owornmask = (long) MIGR_EXACT_XY;
            }
    swap_pending = TRUE;
}

boolean
hinnom_swap_pending(void)
{
    return swap_pending;
}

void
hinnom_finish_swap(void)
{
    xint16 ledger;

    if (!swap_pending)
        return;
    swap_pending = FALSE;
    ledger = ledger_no(&u.uz);
    delete_levelfile(ledger);
    if (ledger > 0 && ledger <= maxledgerno())
        svl.level_info[ledger].flags &= ~(LFILE_EXISTS | VISITED);
    u.uevent.berachah = 1;
    svm.mvitals[PM_MICHAEL].mvflags &= ~(G_EXTINCT | G_GENOD);
}

void
berachah_fix_objects(void)
{
    struct obj *obj, *nobj;
    coordxy x, y;

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            if (levl[x][y].typ != TREE && !IS_POOL(levl[x][y].typ)
                && !IS_WALL(levl[x][y].typ) && levl[x][y].typ != IRONBARS)
                continue;
            for (obj = svl.level.objects[x][y]; obj; obj = nobj) {
                nobj = obj->nexthere;
                if (obj->where != OBJ_FLOOR)
                    continue;
                obj_extract_self(obj);
                place_object(obj, 4, 17);
                stackobj(obj);
            }
        }
}

boolean
living_water_at(coordxy x, coordxy y)
{
    if (!isok(x, y) || !IS_FOUNTAIN(levl[x][y].typ))
        return FALSE;
    if (Is_berachah_level(&u.uz))
        return TRUE;
    if (in_wilderness() && dunlev(&u.uz) == 5)
        return TRUE;
    if (on_jacobs_ladder(&u.uz))
        return TRUE;
    return FALSE;
}

boolean
living_water_here(void)
{
    return living_water_at(u.ux, u.uy);
}

void
living_water_restore(void)
{
    int i;

    if (Upolyd)
        u.mh = u.mhmax;
    else
        u.uhp = u.uhpmax;
    u.uen = u.uenmax;
    u.uhunger = 1000;
    newuhs(FALSE);
    make_sick(0L, (char *) 0, TRUE, SICK_ALL);
    make_stoned(0L, (char *) 0, 0, (char *) 0);
    make_slimed(0L, (char *) 0);
    make_blinded(0L, TRUE);
    make_deaf(0L, TRUE);
    make_confused(0L, TRUE);
    make_stunned(0L, TRUE);
    make_hallucinated(0L, TRUE, 0L);
    make_vomiting(0L, TRUE);
    heal_legs(0);
    if (u.ulycn != NON_PM) {
        u.ulycn = NON_PM;
        retouch_equipment(1);
    }
    for (i = 0; i < A_MAX; i++) {
        if (ABASE(i) < AMAX(i))
            ABASE(i) = AMAX(i);
    }
    disp.botl = TRUE;
}

static void
give_cloak(void)
{
    struct obj *otmp;

    otmp = mksobj(HIGH_PRIEST_CLOAK, TRUE, FALSE);
    if (!otmp)
        return;
    uncurse(otmp);
    otmp->dknown = otmp->known = otmp->bknown = 1;
    (void) hold_another_object(otmp, "Oops!  %s to the floor!",
                               The(aobjnam(otmp, "slip")),
                               (const char *) 0);
    u.uevent.hinnom_cloak = 1;
}

static void
open_field_gate(void)
{
    if (levl[HX(41)][10].typ == DOOR) {
        levl[HX(41)][10].doormask = D_ISOPEN;
        newsym(HX(41), 10);
        unblock_point(HX(41), 10);
    }
    lower_hinnom_bridge();
    pline("The gate swings open and the drawbridge lowers."); /* PLACEHOLDER */
}

static void
give_tears(void)
{
    struct obj *otmp;

    otmp = mksobj(POT_TEARS_OF_CHILDREN, FALSE, FALSE);
    if (!otmp)
        return;
    bless(otmp);
    otmp->dknown = otmp->known = otmp->bknown = 1;
    (void) hold_another_object(otmp, "Oops!  %s to the floor!",
                               The(aobjnam(otmp, "slip")),
                               (const char *) 0);
    u.uevent.berachah_tears = 1;
}

static void
say_clue(struct monst *mtmp, const char *line)
{
    (void) mtmp;
    SetVoice(mtmp, 0, 80, 0);
    verbalize("%s", line);
}

boolean
hinnom_chat(struct monst *mtmp)
{
    char buf[BUFSZ];

    if (!mtmp)
        return FALSE;

    if (mtmp->data == &mons[PM_MOLOCH] && moloch_disguise_holds())
        return TRUE;

    if (mtmp->data == &mons[PM_MICHAEL]) {
        SetVoice(mtmp, 0, 80, 0);
        if (Is_berachah_level(&u.uz)) {
            verbalize("Well done. Rest here a while."); /* PLACEHOLDER */
            return TRUE;
        }
        if (!Is_hinnom_level(&u.uz))
            return FALSE;
        if (!u.uevent.hinnom_cloak) {
            verbalize("Wear this, and the fortress will take you for its high priest."); /* PLACEHOLDER */
            give_cloak();
        } else {
            verbalize("The field must fall before my host can enter."); /* PLACEHOLDER */
        }
        return TRUE;
    }

    if (Is_berachah_level(&u.uz) && mtmp->data == &mons[PM_ANGEL]) {
        SetVoice(mtmp, 0, 80, 0);
        verbalize("Well done. Rest here a while."); /* PLACEHOLDER */
        return TRUE;
    }

    if (mtmp->data == &mons[PM_CHILD]) {
        SetVoice(mtmp, 0, 80, 0);
        if (Is_berachah_level(&u.uz)) {
            if (!u.uevent.berachah_tears) {
                verbalize("We saved these for you. Thank you for coming for us."); /* PLACEHOLDER */
                give_tears();
            } else {
                verbalize("We are safe now."); /* PLACEHOLDER */
            }
            return TRUE;
        }
        if (Is_hinnom_level(&u.uz)) {
            verbalize("Please don't let him take us."); /* PLACEHOLDER */
            return TRUE;
        }
    }

    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return FALSE;

    if (mtmp->data == &mons[PM_FORTRESS_GUARD]
        && !hinnom_inside_field(mtmp->mx, mtmp->my)) {
        SetVoice(mtmp, 0, 80, 0);
        if (!hinnom_field_up()
            || (levl[HX(41)][10].typ == DOOR
                && (levl[HX(41)][10].doormask & D_ISOPEN))) {
            verbalize("None enter without the High Priest's mark."); /* PLACEHOLDER */
            return TRUE;
        }
        if (moloch_disguise_holds()) {
            verbalize("Pass, Your Holiness."); /* PLACEHOLDER */
            open_field_gate();
        } else {
            verbalize("You are not one of us!"); /* PLACEHOLDER */
            anger_gate_guards();
        }
        return TRUE;
    }

    /* Cloak on, fortress quiet. Priests of the Great Dragon are
       included even if they have stepped outside the field box:
       priest_talk() would otherwise turn them hostile. */
    if (moloch_disguise_holds()
        && (fortress_monster(mtmp)
            || mtmp->data == &mons[PM_DOCTOR]
            || mtmp->data == &mons[PM_FORTRESS_GUARD]
            || mtmp->data == &mons[PM_ALIGNED_CLERIC]
            || mtmp->data == &mons[PM_HIGH_CLERIC])) {
        mtmp->mpeaceful = 1;
        if (mtmp->data == &mons[PM_DOCTOR]) {
            Sprintf(buf, "The engineers always start with the %s lever.",
                    metal_of_rank(1));
            say_clue(mtmp, buf);
            return TRUE;
        }
        if (mtmp->data == &mons[PM_FORTRESS_GUARD]) {
            Sprintf(buf, "After the first, they pull the %s one.",
                    metal_of_rank(2));
            say_clue(mtmp, buf);
            return TRUE;
        }
        if (mtmp->data == &mons[PM_ALIGNED_CLERIC]
            || mtmp->data == &mons[PM_HIGH_CLERIC]) {
            Sprintf(buf, "The %s lever is third. Never out of order.",
                    metal_of_rank(3));
            say_clue(mtmp, buf);
            return TRUE;
        }
        SetVoice(mtmp, 0, 80, 0);
        verbalize("The fires must never go out."); /* PLACEHOLDER */
        return TRUE;
    }
    return FALSE;
}

void
hinnom_child_died(struct monst *mtmp)
{
    if (!mtmp || mtmp->data != &mons[PM_CHILD])
        return;
    if (!Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return;
    adjalign(-5);
    change_luck(-2);
    pline("You feel a deep sorrow."); /* PLACEHOLDER */
}

boolean
michael_stands(struct monst *mtmp)
{
    if (!mtmp || mtmp->data != &mons[PM_MICHAEL])
        return FALSE;
    mtmp->mhp = mtmp->mhpmax;
    pline("%s stands firm.", pmname(&mons[PM_MICHAEL], MALE)); /* PLACEHOLDER */
    return TRUE;
}

const char *
berachah_shop_sign(int rtype)
{
    switch (rtype) {
    case SCROLLSHOP:
        return "the Scribes of Ezra";
    case BOOKSHOP:
        return "the Library of the Prophets";
    case SHOPBASE: /* general store */
        return "the Sheep Gate Market";
    default:
        return "the Sheep Gate Market";
    }
}

const char *
berachah_keeper(int rtype)
{
    switch (rtype) {
    case SCROLLSHOP:
        return "+Baruch";
    case BOOKSHOP:
        return "+Nathan";
    default:
        return "-Lydia";
    }
}

/*hinnom.c*/

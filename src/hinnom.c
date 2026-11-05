/* The Battle of Hinnom and the Valley of Berachah. */

#include "hack.h"

static boolean swap_pending = FALSE;
static int raze_turns;
static coordxy raze_x, raze_y;

static void put_wrapped(winid, const char *);
static void hinnom_screen(const char *const *, int);

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

/* The fortress is still the closed, peaceful place. The alarm is
   the hero's own break: an attack on the garrison, the cloak coming
   off, the gate forced, or the field falling. */
boolean
hinnom_fortress_closed(void)
{
    return (boolean) (Is_hinnom_level(&u.uz) && !u.uevent.berachah
                      && !u.uevent.hinnom_alarm);
}

/* Placed inhabitants. A random arrival of the same kind is treated
   as one of them so it does not fight the disguise. A stalker is not. */
boolean
hinnom_staff(struct monst *mtmp)
{
    struct permonst *ptr;

    if (!mtmp)
        return FALSE;
    ptr = mtmp->data;
    return (ptr == &mons[PM_FORTRESS_GUARD]
            || ptr == &mons[PM_DOCTOR]
            || ptr == &mons[PM_ALIGNED_CLERIC]
            || ptr == &mons[PM_HIGH_CLERIC]
            || ptr == &mons[PM_HORNED_DEVIL]
            || ptr == &mons[PM_BARBED_DEVIL]
            || ptr == &mons[PM_BONE_DEVIL]
            || ptr == &mons[PM_ICE_DEVIL]
            || ptr == &mons[PM_NALFESHNEE]
            || ptr == &mons[PM_PIT_FIEND]
            || ptr == &mons[PM_MOLOCH]);
}

boolean
fortress_monster(struct monst *mtmp)
{
    if (!mtmp || !Is_hinnom_level(&u.uz) || u.uevent.berachah)
        return FALSE;
    if (!hinnom_inside_field(mtmp->mx, mtmp->my))
        return FALSE;
    if (!hinnom_staff(mtmp))
        return FALSE;
    return TRUE;
}

/* Wanderers are not the garrison. While the fortress is closed they
   do not stand inside it and they do not fight the disguised hero. */
boolean
hinnom_keep_intruder_out(struct monst *mtmp)
{
    if (!mtmp || !hinnom_fortress_closed())
        return FALSE;
    if (mtmp->mtame || hinnom_staff(mtmp))
        return FALSE;
    if (mtmp->data == &mons[PM_MICHAEL] || mtmp->data == &mons[PM_ANGEL]
        || mtmp->data == &mons[PM_CHILD])
        return FALSE;
    return TRUE;
}

/* Put a wanderer back outside the bars. No message: it was never
   part of the peace. If no square is open, it waits and does not fight. */
void
hinnom_relocate_intruder(struct monst *mtmp)
{
    int tries;
    coordxy x, y;

    if (!mtmp || !hinnom_inside_field(mtmp->mx, mtmp->my))
        return;
    for (tries = 0; tries < 50; tries++) {
        x = rn1(COLNO - 2, 1);
        y = rn2(ROWNO);
        if (!goodpos(x, y, mtmp, GP_CHECKSCARY))
            continue;
        if (hinnom_inside_field(x, y) || hinnom_portal_room(x, y))
            continue;
        rloc_to(mtmp, x, y);
        return;
    }
    mtmp->mpeaceful = 1;
    mtmp->msleeping = 0;
    mtmp->mstrategy |= STRAT_WAITFORU;
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
    /* Once, since the alarm bit is set above. When the field has
       already fallen, the battle screen covers it and this is skipped. */
    if (hinnom_field_up()) {
        static const char *const para[] = {
            "A cry goes up from somewhere in the halls: \"An impostor! That is not our master!\"",
            "Heads turn. The meditating figures rise to their feet, one after another, and their eyes fill with hate. A bell begins to clang somewhere deep in the fortress, and the drums answer it.",
            "Your disguise is broken. Every servant of Moloch in this place now knows you for what you are, an enemy of their master, and they will not rest until you are dead.",
            "The field still stands. The Host cannot reach you. You are on your own.",
        };

        hinnom_screen(para, SIZE(para));
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
    /* Once: the bars are gone now, and the levers go dead. */
    {
        static const char *const para[] = {
            "The last lever falls into place. Deep beneath your feet, something groans and breaks.",
            "The air shudders. With a sound like a great sheet of glass shattering, the unholy field around the fortress flickers, cracks, and vanishes.",
            "For one breathless moment, all is silent.",
            "Then trumpets sound from the heavens. Light pours over the walls like the dawn breaking, and through it comes the Host of Heaven, with Michael the Archangel at its head and his sword blazing like the sun.",
            "\"NOW!\" his voice thunders across the valley. \"For the LORD and for the children!\"",
            "All around you, Moloch's servants shriek and scatter as the battle for Hinnom begins.",
        };

        hinnom_screen(para, SIZE(para));
    }
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
    /* No way down while Moloch lives: stairs, falls, and level
       teleport all pass through here. Up and same-depth exits stay
       open. Checked in prophet mode too, so it can be tested. */
    if (Is_hinnom_level(&u.uz) && !u.uevent.berachah
        && svm.mvitals[PM_MOLOCH].died == 0
        && depth(newlevel) > depth(&u.uz)) {
        pline("You cannot go down until Moloch is defeated."); /* PLACEHOLDER */
        return TRUE;
    }
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
    /* The hero is leaving. The Valley is built on the next visit,
       and the arrival text waits for that visit. */
}

/* Quest-style text window. Break only at spaces so the words stay
   the approved text. */
static void
put_wrapped(winid win, const char *text)
{
    char line[80];
    const char *p = text;

    while (*p) {
        int n, cut;

        while (*p == ' ')
            p++;
        if (!*p)
            break;
        n = 0;
        while (p[n] && n < 76)
            n++;
        cut = n;
        if (p[n] && p[n] != ' ') {
            while (cut > 40 && p[cut] != ' ')
                cut--;
            if (p[cut] != ' ')
                cut = n;
        }
        while (cut > 0 && p[cut - 1] == ' ')
            cut--;
        if (cut <= 0)
            cut = n;
        (void) strncpy(line, p, (size_t) cut);
        line[cut] = '\0';
        putstr(win, 0, line);
        p += cut;
    }
}

/* Blackout text window, a blank line between paragraphs. */
static void
hinnom_screen(const char *const *para, int n)
{
    winid win = create_nhwindow(NHW_TEXT);
    int i;

    for (i = 0; i < n; i++) {
        if (i)
            putstr(win, 0, "");
        put_wrapped(win, para[i]);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
}

/* PLACEHOLDER: the whole briefing is not final-approved. Shown on
   every #chat with Michael while the field stands. */
static void
michael_briefing(void)
{
    static const char *const para[] = {
        "Beyond these walls stands the fortress of Moloch, the devourer of children. For long ages his priests have fed his fires with the little ones of this land, and the cries of their mothers have risen up to Heaven. The LORD has heard them. This day we come to make an end of it.",
        "But Moloch is cunning. His fortress is sealed by a field of unholy power, and no angel of the Host can pass through it. We cannot break it from without. It must be broken from within.",
        "That is why you are here.",
        "You must go in disguise, wearing the cloak of Moloch's High Priest. In that garment, his servants will take you for their master. The guards at the gate will bow and let you pass. Inside, his people keep to their meditations, and you may walk among them and speak with them freely.",
        "Somewhere within lie five levers of five different metals. Pulled in the right order, they will bring down the field. Pulled wrongly, they will not. Listen well, and piece it together.",
        "When the field falls, we will descend on that place like fire from Heaven, and you will fight beside us.",
        "Heed this warning: do NOT approach that fortress without the cloak. If you are seen as you truly are, the guards will raise the alarm, the whole fortress will rise against you, and you will stand alone against all of them, with the field still between you and us.",
        "Go now. The children are waiting.",
    };
    const char *text[SIZE(para) + 1];
    char first[BUFSZ];
    int i;

    Sprintf(first, "Hear me, %s of the Most High.",
            rank_of(u.ulevel, Role_switch, flags.female));
    text[0] = first;
    for (i = 0; i < SIZE(para); i++)
        text[i + 1] = para[i];
    hinnom_screen(text, SIZE(text));
}

/* Once. The swap sets berachah while the hero is still recorded on
   the level, but that call is the departure: the Valley map is
   created on the way back in. Showing the text here, on arrival,
   is the first time the hero is standing in the Valley. */
void
berachah_intro(void)
{
    static const char *const para[] = {
        "The smoke of Moloch's fires is gone. Where the fortress once loomed, gates stand open and lamps burn in every window.",
        "As you pass through the gate, a great cheer goes up. Families pour out of the old fortress halls, now homes and shops, and children run ahead to tell everyone that the hero has come back. Old men weep openly. Someone presses bread into your hands, and someone else lays a garland about your neck.",
        "A white-haired elder steps forward and raises his voice over the crowd:",
        "\"Look! Here is the one who broke the altar of Moloch and set the captives free! This valley was once a place of fire and sorrow. Now we bless the LORD here, and it shall be called the Valley of Berachah forever.\"",
        "\"Rest, eat, and trade among us, friend. Whatever we have is yours.\"",
        "The name echoes the scripture: \"for there they blessed the LORD: therefore the name of the same place was called, The valley of Berachah, unto this day.\" (2 Chronicles 20:26)",
    };

    if (!Is_berachah_level(&u.uz) || u.uevent.berachah_intro)
        return;
    u.uevent.berachah_intro = 1;
    hinnom_screen(para, SIZE(para));
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

/* Jon's lines, in order. Michael is not an angel of this type and
   keeps his own speech. Fewer than six angels use the first lines.
   More than six cycle, and a line is skipped when it would repeat
   on an angel standing next to one who already has it. */
static const char *const angel_victory[] = {
    "The fire of Hinnom is quenched. Well fought, champion!",
    "Heaven rings with your name this day!",
    "The Dragon's servant has fallen. Glory to the Most High!",
    "I have watched a thousand battles. Few were fought so bravely.",
    "The children are avenged. Go in peace, faithful one.",
    "Where you stand, darkness has fled. Rejoice!",
};
#define ANGEL_VICTORY_N SIZE(angel_victory)

static boolean
angels_beside(struct monst *a, struct monst *b)
{
    int dx = a->mx - b->mx, dy = a->my - b->my;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    return (boolean) (dx <= 1 && dy <= 1 && (dx || dy));
}

static const char *
angel_victory_line(struct monst *speaker)
{
    struct monst *ang[128], *mtmp, *key;
    int n, i, k, line, guard;
    int assigned[128];

    n = 0;
    for (mtmp = fmon; mtmp && n < SIZE(ang); mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || mtmp->data != &mons[PM_ANGEL])
            continue;
        ang[n++] = mtmp;
    }
    for (i = 1; i < n; i++) {
        key = ang[i];
        k = i;
        while (k > 0 && ang[k - 1]->m_id > key->m_id) {
            ang[k] = ang[k - 1];
            k--;
        }
        ang[k] = key;
    }
    for (i = 0; i < n; i++) {
        if (n <= ANGEL_VICTORY_N) {
            assigned[i] = i;
            continue;
        }
        line = i % ANGEL_VICTORY_N;
        for (guard = 0; guard < ANGEL_VICTORY_N; guard++) {
            boolean clash = FALSE;

            for (k = 0; k < i; k++)
                if (assigned[k] == line && angels_beside(ang[k], ang[i]))
                    clash = TRUE;
            if (!clash)
                break;
            line = (line + 1) % ANGEL_VICTORY_N;
        }
        assigned[i] = line;
    }
    for (i = 0; i < n; i++)
        if (ang[i] == speaker)
            return angel_victory[assigned[i]];
    return angel_victory[0];
}

/* Before the assault, the four angels beside Michael. Placed in
   dat/hinnom.lua order, so the lowest four m_ids are angels 1-4.
   A later arrival takes the lines of angel (its place mod 4). */
static const char *const angel_eve[4][3] = {
    { "Today the fires of Hinnom go out forever!",
      "My sword has waited long for this hour.",
      "Strike true, and we will be right behind you." },
    { "Fear not! The LORD of hosts is with us.",
      "When that field falls, Heaven itself will charge.",
      "Every child in that place will see the morning." },
    { "Moloch has never faced the Host in battle. He will learn.",
      "Be bold in there. Be clever. Then be quick.",
      "I can almost hear the trumpets already." },
    { "Walk in faith, champion. You are not alone.",
      "Their master trembles, though he does not know it yet.",
      "This is the day the LORD has given us. Rejoice in it!" },
};

static const char *
angel_eve_line(struct monst *speaker)
{
    struct monst *mtmp;
    int place = 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp) && mtmp != speaker
            && mtmp->data == &mons[PM_ANGEL] && mtmp->m_id < speaker->m_id)
            place++;
    return angel_eve[place % 4][rn2(3)];
}

/* After the Tears have been given. Six children stand on Berachah,
   so the first six lines are used. A seventh would take line seven. */
static const char *const child_thanks[] = {
    "You came for us. Nobody else did. Thank you!",
    "My mother says you are a hero from the old stories.",
    "I am not afraid of the dark anymore.",
    "When I grow up, I want to be brave like you.",
    "We prayed every night that someone would come. And you came!",
    "Thank you for bringing us home.",
    "I made you a crown of flowers. Well, I tried to.",
    "The LORD sent you. I just know it.",
};
#define CHILD_THANKS_N SIZE(child_thanks)

static const char *
child_thanks_line(struct monst *speaker)
{
    struct monst *kids[32], *mtmp, *key;
    int n, i, k;

    n = 0;
    for (mtmp = fmon; mtmp && n < SIZE(kids); mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || mtmp->data != &mons[PM_CHILD])
            continue;
        kids[n++] = mtmp;
    }
    for (i = 1; i < n; i++) {
        key = kids[i];
        k = i;
        while (k > 0 && kids[k - 1]->m_id > key->m_id) {
            kids[k] = kids[k - 1];
            k--;
        }
        kids[k] = key;
    }
    for (i = 0; i < n; i++)
        if (kids[i] == speaker)
            return child_thanks[(n <= CHILD_THANKS_N) ? i
                                                      : (i % CHILD_THANKS_N)];
    return child_thanks[0];
}

/* PLACEHOLDER until Jon approves the town elder's lines. */
static const char *const elder_lines[] = {
    "Rest a while, champion. You have earned it.", /* PLACEHOLDER */
    "My grandfather told of the days before Moloch. Now my grandchildren will tell of you.", /* PLACEHOLDER */
    "Blessed be the LORD, who did not forget this valley.", /* PLACEHOLDER */
    "Our shops are open to you, and our hearts as well.", /* PLACEHOLDER */
};

static boolean
is_town_elder(struct monst *mtmp)
{
    return (boolean) (mtmp && has_mgivenname(mtmp)
                      && !strcmp(MGIVENNAME(mtmp), "town elder"));
}

/* A male prisoner named "town elder". He stays inside the gate
   while he is peaceful. An attack releases him. A guide would
   add the quest-guardian penalty on top of ordinary murder. */
void
berachah_elder_hold(struct monst *mtmp)
{
    if (!mtmp || !Is_berachah_level(&u.uz) || !mtmp->mpeaceful)
        return;
    if (!is_town_elder(mtmp))
        return;
    mtmp->mstrategy |= STRAT_WAITFORU;
    mtmp->msleeping = 0;
}

/* Court lines for fortress creatures who do not give a lever clue
   and are not Moloch. Nine are placed. The first eight each get
   one line. The ninth repeats the first, and that repeat is not
   on a square next to the creature who already has it. */
static const char *const fortress_court[] = {
    "Your Holiness honors us with a visit.",
    "The fires burn hot today, Your Holiness.",
    "Forgive me, Your Holiness. I did not see you there.",
    "All is quiet at my post, Your Holiness.",
    "The master grows restless. Will you speak with him?",
    "I keep to my meditations, as you commanded.",
    "May your shadow never shorten, Your Holiness.",
    "The servants of the Dragon await your word.",
};
#define FORTRESS_COURT_N SIZE(fortress_court)

static const char *const doctor_aside[] = {
    "I have been measuring how long a man can go without sleep. The answer disappoints me.",
    "The master wants stronger servants. I am... improving the recipe.",
    "Do not mind the screaming from the lower cells. It is purely scientific.",
    "I bled a prisoner with leeches for nine days. Remarkable endurance.",
    "Fear makes the heart beat faster. I am learning to make it beat forever.",
    "My latest patient grew a second shadow. I have not yet decided if that is a success.",
};

static const char *const captive_child[] = {
    "The doctor says I am his best patient. I don't want to be his patient.",
    "He makes us drink something bitter. Then everything spins.",
    "He writes in his book every time I cry.",
    "He took my friend to the lower cells. She hasn't come back.",
    "He says if we are good, the master will not need us. I try so hard to be good.",
    "Please don't tell him I talked to you.",
};

static const char *const moloch_lines[] = {
    "Every fire needs fuel, priest. Mine prefers the young.",
    "Their mothers weep, and I grow strong. Such is the old bargain.",
    "Bring me the little ones. The flames are hungry tonight.",
    "No song pleases me like the silence after the drums stop.",
};

static boolean moloch_taunted = FALSE;

static const char *
moloch_line(void)
{
    return moloch_lines[rn2(SIZE(moloch_lines))];
}

/* Once, the first time an attack on the hero is actually underway.
   Damage, the raze, and whether he swings are unchanged. */
void
moloch_opening_taunt(struct monst *mtmp)
{
    if (!mtmp || mtmp->data != &mons[PM_MOLOCH] || moloch_taunted || Deaf)
        return;
    moloch_taunted = TRUE;
    SetVoice(mtmp, 0, 80, 0);
    verbalize("%s", moloch_line());
}

static boolean
fortress_court_member(struct monst *mtmp)
{
    struct permonst *ptr;

    if (!hinnom_staff(mtmp))
        return FALSE;
    ptr = mtmp->data;
    if (ptr == &mons[PM_MOLOCH] || ptr == &mons[PM_DOCTOR]
        || ptr == &mons[PM_FORTRESS_GUARD]
        || ptr == &mons[PM_ALIGNED_CLERIC] || ptr == &mons[PM_HIGH_CLERIC])
        return FALSE;
    return TRUE;
}

static const char *
fortress_court_line(struct monst *speaker)
{
    struct monst *court[32], *mtmp, *key;
    int n, i, k, line, guard;
    int assigned[32];

    n = 0;
    for (mtmp = fmon; mtmp && n < SIZE(court); mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || !fortress_court_member(mtmp))
            continue;
        court[n++] = mtmp;
    }
    for (i = 1; i < n; i++) {
        key = court[i];
        k = i;
        while (k > 0 && court[k - 1]->m_id > key->m_id) {
            court[k] = court[k - 1];
            k--;
        }
        court[k] = key;
    }
    for (i = 0; i < n; i++) {
        if (n <= FORTRESS_COURT_N) {
            assigned[i] = i;
            continue;
        }
        line = i % FORTRESS_COURT_N;
        for (guard = 0; guard < FORTRESS_COURT_N; guard++) {
            boolean clash = FALSE;

            for (k = 0; k < i; k++)
                if (assigned[k] == line && angels_beside(court[k], court[i]))
                    clash = TRUE;
            if (!clash)
                break;
            line = (line + 1) % FORTRESS_COURT_N;
        }
        assigned[i] = line;
    }
    for (i = 0; i < n; i++)
        if (court[i] == speaker)
            return fortress_court[assigned[i]];
    return fortress_court[0];
}

boolean
hinnom_chat(struct monst *mtmp)
{
    char buf[BUFSZ];

    if (!mtmp)
        return FALSE;

    if (mtmp->data == &mons[PM_MOLOCH]) {
        SetVoice(mtmp, 0, 80, 0);
        verbalize("%s", moloch_line());
        return TRUE;
    }

    if (mtmp->data == &mons[PM_MICHAEL]) {
        SetVoice(mtmp, 0, 80, 0);
        if (Is_berachah_level(&u.uz)) {
            verbalize("Well done. Rest here a while."); /* PLACEHOLDER */
            return TRUE;
        }
        if (!Is_hinnom_level(&u.uz))
            return FALSE;
        if (hinnom_field_up())
            michael_briefing();
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
        verbalize("%s", angel_victory_line(mtmp));
        return TRUE;
    }

    if (mtmp->data == &mons[PM_ANGEL] && hinnom_field_up()
        && mtmp->mpeaceful) {
        SetVoice(mtmp, 0, 80, 0);
        verbalize("%s", angel_eve_line(mtmp));
        return TRUE;
    }

    if (Is_berachah_level(&u.uz) && is_town_elder(mtmp)) {
        SetVoice(mtmp, 0, 80, 0);
        verbalize("%s", elder_lines[rn2(SIZE(elder_lines))]); /* PLACEHOLDER */
        return TRUE;
    }

    if (mtmp->data == &mons[PM_CHILD]) {
        SetVoice(mtmp, 0, 80, 0);
        if (Is_berachah_level(&u.uz)) {
            if (!u.uevent.berachah_tears) {
                verbalize("We saved these for you. Thank you for coming for us."); /* PLACEHOLDER */
                give_tears();
            } else {
                verbalize("%s", child_thanks_line(mtmp));
            }
            return TRUE;
        }
        if (Is_hinnom_level(&u.uz)) {
            verbalize("%s", captive_child[rn2(SIZE(captive_child))]);
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
            hinnom_alarm_fortress();
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
            /* The clue is every chat, then one of his own lines.
               The lever order cannot be missed by drawing only flavor. */
            Sprintf(buf, "The engineers always start with the %s lever.",
                    metal_of_rank(1));
            say_clue(mtmp, buf);
            SetVoice(mtmp, 0, 80, 0);
            verbalize("%s", doctor_aside[rn2(SIZE(doctor_aside))]);
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
        verbalize("%s", fortress_court_line(mtmp));
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

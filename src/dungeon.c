/* NetHack 3.6	dungeon.c	$NHDT-Date: 1562187890 2019/07/03 21:04:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.105 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dgn_file.h"
#include "dlb.h"
#include "lev.h"

#define DUNGEON_FILE "dungeon.lua"

#define X_START "x-strt"
#define X_LOCATE "x-loca"
#define X_GOAL "x-goal"

struct proto_dungeon {
    struct tmpdungeon tmpdungeon[MAXDUNGEON];
    struct tmplevel tmplevel[LEV_LIMIT];
    s_level *final_lev[LEV_LIMIT]; /* corresponding level pointers */
    struct tmpbranch tmpbranch[BRANCH_LIMIT];

    int start;  /* starting index of current dungeon sp levels */
    int n_levs; /* number of tmplevel entries */
    int n_brs;  /* number of tmpbranch entries */
};

struct lchoice {
    int idx;
    schar lev[MAXLINFO];
    schar playerlev[MAXLINFO];
    xchar dgn[MAXLINFO];
    char menuletter;
};

#if 0
static void FDECL(Fread, (genericptr_t, int, int, dlb *));
#endif
static xchar FDECL(dname_to_dnum, (const char *));
static int FDECL(find_branch, (const char *, struct proto_dungeon *));
static xchar FDECL(parent_dnum, (const char *, struct proto_dungeon *));
static int FDECL(level_range, (XCHAR_P, int, int, int,
                                   struct proto_dungeon *, int *));
static xchar FDECL(parent_dlevel, (const char *, struct proto_dungeon *));
static int FDECL(correct_branch_type, (struct tmpbranch *));
static branch *FDECL(add_branch, (int, int, struct proto_dungeon *));
static void FDECL(add_level, (s_level *));
static void FDECL(init_level, (int, int, struct proto_dungeon *));
static int FDECL(possible_places, (int, boolean *,
                                       struct proto_dungeon *));
static xchar FDECL(pick_level, (boolean *, int));
static boolean FDECL(place_level, (int, struct proto_dungeon *));
static boolean FDECL(unplaced_floater, (struct dungeon *));
static boolean FDECL(unreachable_level, (d_level *, BOOLEAN_P));
static void FDECL(tport_menu, (winid, char *, struct lchoice *, d_level *,
                                   BOOLEAN_P));
static const char *FDECL(br_string, (int));
static char FDECL(chr_u_on_lvl, (d_level *));
static void FDECL(print_branch, (winid, int, int, int, BOOLEAN_P,
                                     struct lchoice *));
static mapseen *FDECL(load_mapseen, (NHFILE *));
static void FDECL(save_mapseen, (NHFILE *, mapseen *));
static mapseen *FDECL(find_mapseen, (d_level *));
static mapseen *FDECL(find_mapseen_by_str, (const char *));
static void FDECL(print_mapseen, (winid, mapseen *, int, int, BOOLEAN_P));
static boolean FDECL(interest_mapseen, (mapseen *));
static void FDECL(traverse_mapseenchn, (BOOLEAN_P, winid,
                                            int, int, int *));
static const char *FDECL(seen_string, (XCHAR_P, const char *));
static const char *FDECL(br_string2, (branch *));
static const char *FDECL(shop_string, (int));
static char *FDECL(tunesuffix, (mapseen *, char *));

#ifdef DEBUG
#define DD g.dungeons[i]
static void NDECL(dumpit);

static void
dumpit()
{
    int i;
    s_level *x;
    branch *br;

    if (!explicitdebug(__FILE__))
        return;

    for (i = 0; i < g.n_dgns; i++) {
        fprintf(stderr, "\n#%d \"%s\" (%s):\n", i, DD.dname, DD.proto);
        fprintf(stderr, "    num_dunlevs %d, dunlev_ureached %d\n",
                DD.num_dunlevs, DD.dunlev_ureached);
        fprintf(stderr, "    depth_start %d, ledger_start %d\n",
                DD.depth_start, DD.ledger_start);
        fprintf(stderr, "    flags:%s%s%s\n",
                DD.flags.rogue_like ? " rogue_like" : "",
                DD.flags.maze_like ? " maze_like" : "",
                DD.flags.hellish ? " hellish" : "");
        getchar();
    }
    fprintf(stderr, "\nSpecial levels:\n");
    for (x = g.sp_levchn; x; x = x->next) {
        fprintf(stderr, "%s (%d): ", x->proto, x->rndlevs);
        fprintf(stderr, "on %d, %d; ", x->dlevel.dnum, x->dlevel.dlevel);
        fprintf(stderr, "flags:%s%s%s%s\n",
                x->flags.rogue_like ? " rogue_like" : "",
                x->flags.maze_like ? " maze_like" : "",
                x->flags.hellish ? " hellish" : "",
                x->flags.town ? " town" : "");
        getchar();
    }
    fprintf(stderr, "\nBranches:\n");
    for (br = g.branches; br; br = br->next) {
        fprintf(stderr, "%d: %s, end1 %d %d, end2 %d %d, %s\n", br->id,
                br->type == BR_STAIR
                    ? "stair"
                    : br->type == BR_NO_END1
                        ? "no end1"
                        : br->type == BR_NO_END2
                            ? "no end2"
                            : br->type == BR_PORTAL
                                ? "portal"
                                : "unknown",
                br->end1.dnum, br->end1.dlevel, br->end2.dnum,
                br->end2.dlevel, br->end1_up ? "end1 up" : "end1 down");
    }
    getchar();
    fprintf(stderr, "\nDone\n");
    getchar();
}
#endif

/* Save the dungeon structures. */
void
save_dungeon(nhfp, perform_write, free_data)
NHFILE *nhfp;
boolean perform_write, free_data;
{
    int count;
    branch *curr, *next;
    mapseen *curr_ms, *next_ms;

    if (perform_write) {
        if(nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &g.n_dgns, sizeof g.n_dgns);
            bwrite(nhfp->fd, (genericptr_t) g.dungeons,
                   sizeof(dungeon) * (unsigned) g.n_dgns);
            bwrite(nhfp->fd, (genericptr_t) &g.dungeon_topology, sizeof g.dungeon_topology);
            bwrite(nhfp->fd, (genericptr_t) g.tune, sizeof tune);
        }
        for (count = 0, curr = g.branches; curr; curr = curr->next)
            count++;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &count, sizeof(count));

        for (curr = g.branches; curr; curr = curr->next) {
          if (nhfp->structlevel)
              bwrite(nhfp->fd, (genericptr_t) curr, sizeof(branch));
        }
        count = maxledgerno();
        if (nhfp->structlevel) { 
            bwrite(nhfp->fd, (genericptr_t) &count, sizeof count);
            bwrite(nhfp->fd, (genericptr_t) g.level_info,
                   (unsigned) count * sizeof(struct linfo));
            bwrite(nhfp->fd, (genericptr_t) &g.inv_pos, sizeof g.inv_pos);
        }
        for (count = 0, curr_ms = g.mapseenchn; curr_ms;
             curr_ms = curr_ms->next)
            count++;

        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &count, sizeof(count));

        for (curr_ms = g.mapseenchn; curr_ms; curr_ms = curr_ms->next) {
            save_mapseen(nhfp, curr_ms);
        }    
    }

    if (free_data) {
        for (curr = g.branches; curr; curr = next) {
            next = curr->next;
            free((genericptr_t) curr);
        }
        g.branches = 0;
        for (curr_ms = g.mapseenchn; curr_ms; curr_ms = next_ms) {
            next_ms = curr_ms->next;
            if (curr_ms->custom)
                free((genericptr_t) curr_ms->custom);
            if (curr_ms->final_resting_place)
                savecemetery(nhfp, &curr_ms->final_resting_place);
            free((genericptr_t) curr_ms);
        }
        g.mapseenchn = 0;
    }
}

/* Restore the dungeon structures. */
void
restore_dungeon(nhfp)
NHFILE *nhfp;
{
    branch *curr, *last;
    int count = 0, i;
    mapseen *curr_ms, *last_ms;

    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &g.n_dgns, sizeof(g.n_dgns));
        mread(nhfp->fd, (genericptr_t) g.dungeons, sizeof(dungeon) * (unsigned) g.n_dgns);
        mread(nhfp->fd, (genericptr_t) &g.dungeon_topology, sizeof g.dungeon_topology);
        mread(nhfp->fd, (genericptr_t) g.tune, sizeof tune);
    }
    last = g.branches = (branch *) 0;

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &count, sizeof(count));

    for (i = 0; i < count; i++) {
        curr = (branch *) alloc(sizeof(branch));
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) curr, sizeof(branch));
        curr->next = (branch *) 0;
        if (last)
            last->next = curr;
        else
            g.branches = curr;
        last = curr;
    }

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &count, sizeof(count));

    if (count >= MAXLINFO)
        panic("level information count larger (%d) than allocated size",
              count);
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) g.level_info,
              (unsigned) count * sizeof(struct linfo));

    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &g.inv_pos, sizeof g.inv_pos);
        mread(nhfp->fd, (genericptr_t) &count, sizeof(count));
    }

    last_ms = (mapseen *) 0;
    for (i = 0; i < count; i++) {
        curr_ms = load_mapseen(nhfp);
        curr_ms->next = (mapseen *) 0;
        if (last_ms)
            last_ms->next = curr_ms;
        else
            g.mapseenchn = curr_ms;
        last_ms = curr_ms;
    }
}

#if 0
static void
Fread(ptr, size, nitems, stream)
genericptr_t ptr;
int size, nitems;
dlb *stream;
{
    int cnt;

    if ((cnt = dlb_fread(ptr, size, nitems, stream)) != nitems) {
        panic(
  "Premature EOF on dungeon description file!\r\nExpected %d bytes - got %d.",
              (size * nitems), (size * cnt));
        nh_terminate(EXIT_FAILURE);
    }
}
#endif

static xchar
dname_to_dnum(s)
const char *s;
{
    xchar i;

    for (i = 0; i < g.n_dgns; i++)
        if (!strcmp(g.dungeons[i].dname, s))
            return i;

    panic("Couldn't resolve dungeon number for name \"%s\".", s);
    /*NOT REACHED*/
    return (xchar) 0;
}

s_level *
find_level(s)
const char *s;
{
    s_level *curr;
    for (curr = g.sp_levchn; curr; curr = curr->next)
        if (!strcmpi(s, curr->proto))
            break;
    return curr;
}

/* Find the branch that links the named dungeon. */
static int
find_branch(s, pd)
const char *s; /* dungeon name */
struct proto_dungeon *pd;
{
    int i;

    if (pd) {
        for (i = 0; i < pd->n_brs; i++)
            if (!strcmp(pd->tmpbranch[i].name, s))
                break;
        if (i == pd->n_brs)
            panic("find_branch: can't find %s", s);
    } else {
        /* support for level tport by name */
        branch *br;
        const char *dnam;

        for (br = g.branches; br; br = br->next) {
            dnam = g.dungeons[br->end2.dnum].dname;
            if (!strcmpi(dnam, s)
                || (!strncmpi(dnam, "The ", 4) && !strcmpi(dnam + 4, s)))
                break;
        }
        i = br ? ((ledger_no(&br->end1) << 8) | ledger_no(&br->end2)) : -1;
    }
    return i;
}

/*
 * Find the "parent" by searching the prototype branch list for the branch
 * listing, then figuring out to which dungeon it belongs.
 */
static xchar
parent_dnum(s, pd)
const char *s; /* dungeon name */
struct proto_dungeon *pd;
{
    int i;
    xchar pdnum;

    i = find_branch(s, pd);
    /*
     * Got branch, now find parent dungeon.  Stop if we have reached
     * "this" dungeon (if we haven't found it by now it is an error).
     */
    for (pdnum = 0; strcmp(pd->tmpdungeon[pdnum].name, s); pdnum++)
        if ((i -= pd->tmpdungeon[pdnum].branches) < 0)
            return pdnum;

    panic("parent_dnum: couldn't resolve branch.");
    /*NOT REACHED*/
    return (xchar) 0;
}

/*
 * Return a starting point and number of successive positions a level
 * or dungeon entrance can occupy.
 *
 * Note: This follows the acouple (instead of the rcouple) rules for a
 *       negative random component (randc < 0).  These rules are found
 *       in dgn_comp.y.  The acouple [absolute couple] section says that
 *       a negative random component means from the (adjusted) base to the
 *       end of the dungeon.
 */
static int
level_range(dgn, base, randc, chain, pd, adjusted_base)
xchar dgn;
int base, randc, chain;
struct proto_dungeon *pd;
int *adjusted_base;
{
    int lmax = g.dungeons[dgn].num_dunlevs;

    if (chain >= 0) { /* relative to a special level */
        s_level *levtmp = pd->final_lev[chain];
        if (!levtmp)
            panic("level_range: empty chain level!");

        base += levtmp->dlevel.dlevel;
    } else { /* absolute in the dungeon */
        /* from end of dungeon */
        if (base < 0)
            base = (lmax + base + 1);
    }

    if (base < 1 || base > lmax)
        panic("level_range: base value out of range");

    *adjusted_base = base;

    if (randc == -1) { /* from base to end of dungeon */
        return (lmax - base + 1);
    } else if (randc) {
        /* make sure we don't run off the end of the dungeon */
        return (((base + randc - 1) > lmax) ? lmax - base + 1 : randc);
    } /* else only one choice */
    return 1;
}

static xchar
parent_dlevel(s, pd)
const char *s;
struct proto_dungeon *pd;
{
    int i, j, num, base, dnum = parent_dnum(s, pd);
    branch *curr;

    i = find_branch(s, pd);
    num = level_range(dnum, pd->tmpbranch[i].lev.base,
                      pd->tmpbranch[i].lev.rand, pd->tmpbranch[i].chain, pd,
                      &base);

    /* KMH -- Try our best to find a level without an existing branch */
    i = j = rn2(num);
    do {
        if (++i >= num)
            i = 0;
        for (curr = g.branches; curr; curr = curr->next)
            if ((curr->end1.dnum == dnum && curr->end1.dlevel == base + i)
                || (curr->end2.dnum == dnum && curr->end2.dlevel == base + i))
                break;
    } while (curr && i != j);
    return (base + i);
}

/* Convert from the temporary branch type to the dungeon branch type. */
static int
correct_branch_type(tbr)
struct tmpbranch *tbr;
{
    switch (tbr->type) {
    case TBR_STAIR:
        return BR_STAIR;
    case TBR_NO_UP:
        return tbr->up ? BR_NO_END1 : BR_NO_END2;
    case TBR_NO_DOWN:
        return tbr->up ? BR_NO_END2 : BR_NO_END1;
    case TBR_PORTAL:
        return BR_PORTAL;
    }
    impossible("correct_branch_type: unknown branch type");
    return BR_STAIR;
}

/*
 * Add the given branch to the branch list.  The branch list is ordered
 * by end1 dungeon and level followed by end2 dungeon and level.  If
 * extract_first is true, then the branch is already part of the list
 * but needs to be repositioned.
 */
void
insert_branch(new_branch, extract_first)
branch *new_branch;
boolean extract_first;
{
    branch *curr, *prev;
    long new_val, curr_val, prev_val;

    if (extract_first) {
        for (prev = 0, curr = g.branches; curr; prev = curr, curr = curr->next)
            if (curr == new_branch)
                break;

        if (!curr)
            panic("insert_branch: not found");
        if (prev)
            prev->next = curr->next;
        else
            g.branches = curr->next;
    }
    new_branch->next = (branch *) 0;

/* Convert the branch into a unique number so we can sort them. */
#define branch_val(bp)                                                     \
    ((((long) (bp)->end1.dnum * (MAXLEVEL + 1) + (long) (bp)->end1.dlevel) \
      * (MAXDUNGEON + 1) * (MAXLEVEL + 1))                                 \
     + ((long) (bp)->end2.dnum * (MAXLEVEL + 1) + (long) (bp)->end2.dlevel))

    /*
     * Insert the new branch into the correct place in the branch list.
     */
    prev = (branch *) 0;
    prev_val = -1;
    new_val = branch_val(new_branch);
    for (curr = g.branches; curr;
         prev_val = curr_val, prev = curr, curr = curr->next) {
        curr_val = branch_val(curr);
        if (prev_val < new_val && new_val <= curr_val)
            break;
    }
    if (prev) {
        new_branch->next = curr;
        prev->next = new_branch;
    } else {
        new_branch->next = g.branches;
        g.branches = new_branch;
    }
}

/* Add a dungeon branch to the branch list. */
static branch *
add_branch(dgn, child_entry_level, pd)
int dgn;
int child_entry_level;
struct proto_dungeon *pd;
{
    static int branch_id = 0;
    int branch_num;
    branch *new_branch;

    branch_num = find_branch(g.dungeons[dgn].dname, pd);
    new_branch = (branch *) alloc(sizeof(branch));
    (void) memset((genericptr_t)new_branch, 0, sizeof(branch));
    new_branch->next = (branch *) 0;
    new_branch->id = branch_id++;
    new_branch->type = correct_branch_type(&pd->tmpbranch[branch_num]);
    new_branch->end1.dnum = parent_dnum(g.dungeons[dgn].dname, pd);
    new_branch->end1.dlevel = parent_dlevel(g.dungeons[dgn].dname, pd);
    new_branch->end2.dnum = dgn;
    new_branch->end2.dlevel = child_entry_level;
    new_branch->end1_up = pd->tmpbranch[branch_num].up ? TRUE : FALSE;

    insert_branch(new_branch, FALSE);
    return new_branch;
}

/*
 * Add new level to special level chain.  Insert it in level order with the
 * other levels in this dungeon.  This assumes that we are never given a
 * level that has a dungeon number less than the dungeon number of the
 * last entry.
 */
static void
add_level(new_lev)
s_level *new_lev;
{
    s_level *prev, *curr;

    prev = (s_level *) 0;
    for (curr = g.sp_levchn; curr; curr = curr->next) {
        if (curr->dlevel.dnum == new_lev->dlevel.dnum
            && curr->dlevel.dlevel > new_lev->dlevel.dlevel)
            break;
        prev = curr;
    }
    if (!prev) {
        new_lev->next = g.sp_levchn;
        g.sp_levchn = new_lev;
    } else {
        new_lev->next = curr;
        prev->next = new_lev;
    }
}

static void
init_level(dgn, proto_index, pd)
int dgn, proto_index;
struct proto_dungeon *pd;
{
    s_level *new_level;
    struct tmplevel *tlevel = &pd->tmplevel[proto_index];

    pd->final_lev[proto_index] = (s_level *) 0; /* no "real" level */
    if (!wizard && tlevel->chance <= rn2(100))
        return;

    pd->final_lev[proto_index] = new_level =
        (s_level *) alloc(sizeof(s_level));
    (void) memset((genericptr_t)new_level, 0, sizeof(s_level));
    /* load new level with data */
    Strcpy(new_level->proto, tlevel->name);
    new_level->boneid = tlevel->boneschar;
    new_level->dlevel.dnum = dgn;
    new_level->dlevel.dlevel = 0; /* for now */

    new_level->flags.town = !!(tlevel->flags & TOWN);
    new_level->flags.hellish = !!(tlevel->flags & HELLISH);
    new_level->flags.maze_like = !!(tlevel->flags & MAZELIKE);
    new_level->flags.rogue_like = !!(tlevel->flags & ROGUELIKE);
    new_level->flags.align = ((tlevel->flags & D_ALIGN_MASK) >> 4);
    if (!new_level->flags.align)
        new_level->flags.align =
            ((pd->tmpdungeon[dgn].flags & D_ALIGN_MASK) >> 4);

    new_level->rndlevs = tlevel->rndlevs;
    new_level->next = (s_level *) 0;
}

static int
possible_places(idx, map, pd)
int idx;      /* prototype index */
boolean *map; /* array MAXLEVEL+1 in length */
struct proto_dungeon *pd;
{
    int i, start, count;
    s_level *lev = pd->final_lev[idx];

    /* init level possibilities */
    for (i = 0; i <= MAXLEVEL; i++)
        map[i] = FALSE;

    /* get base and range and set those entries to true */
    count = level_range(lev->dlevel.dnum, pd->tmplevel[idx].lev.base,
                        pd->tmplevel[idx].lev.rand, pd->tmplevel[idx].chain,
                        pd, &start);
    for (i = start; i < start + count; i++)
        map[i] = TRUE;

    /* mark off already placed levels */
    for (i = pd->start; i < idx; i++) {
        if (pd->final_lev[i] && map[pd->final_lev[i]->dlevel.dlevel]) {
            map[pd->final_lev[i]->dlevel.dlevel] = FALSE;
            --count;
        }
    }

    return count;
}

/* Pick the nth TRUE entry in the given boolean array. */
static xchar
pick_level(map, nth)
boolean *map; /* an array MAXLEVEL+1 in size */
int nth;
{
    int i;
    for (i = 1; i <= MAXLEVEL; i++)
        if (map[i] && !nth--)
            return (xchar) i;
    panic("pick_level:  ran out of valid levels");
    return 0;
}

#ifdef DDEBUG
static void FDECL(indent, (int));

static void
indent(d)
int d;
{
    while (d-- > 0)
        fputs("    ", stderr);
}
#endif

/*
 * Place a level.  First, find the possible places on a dungeon map
 * template.  Next pick one.  Then try to place the next level.  If
 * successful, we're done.  Otherwise, try another (and another) until
 * all possible places have been tried.  If all possible places have
 * been exhausted, return false.
 */
static boolean
place_level(proto_index, pd)
int proto_index;
struct proto_dungeon *pd;
{
    boolean map[MAXLEVEL + 1]; /* valid levels are 1..MAXLEVEL inclusive */
    s_level *lev;
    int npossible;
#ifdef DDEBUG
    int i;
#endif

    if (proto_index == pd->n_levs)
        return TRUE; /* at end of proto levels */

    lev = pd->final_lev[proto_index];

    /* No level created for this prototype, goto next. */
    if (!lev)
        return place_level(proto_index + 1, pd);

    npossible = possible_places(proto_index, map, pd);

    for (; npossible; --npossible) {
        lev->dlevel.dlevel = pick_level(map, rn2(npossible));
#ifdef DDEBUG
        indent(proto_index - pd->start);
        fprintf(stderr, "%s: trying %d [ ", lev->proto, lev->dlevel.dlevel);
        for (i = 1; i <= MAXLEVEL; i++)
            if (map[i])
                fprintf(stderr, "%d ", i);
        fprintf(stderr, "]\n");
#endif
        if (place_level(proto_index + 1, pd))
            return TRUE;
        map[lev->dlevel.dlevel] = FALSE; /* this choice didn't work */
    }
#ifdef DDEBUG
    indent(proto_index - pd->start);
    fprintf(stderr, "%s: failed\n", lev->proto);
#endif
    return FALSE;
}

struct level_map {
    const char *lev_name;
    d_level *lev_spec;
} level_map[] = { { "air", &air_level },
                  { "asmodeus", &asmodeus_level },
                  { "astral", &astral_level },
                  { "baalz", &baalzebub_level },
                  { "bigrm", &bigroom_level },
                  { "castle", &stronghold_level },
                  { "earth", &earth_level },
                  { "fakewiz1", &portal_level },
                  { "fire", &fire_level },
                  { "juiblex", &juiblex_level },
                  { "knox", &knox_level },
                  { "medusa", &medusa_level },
                  { "oracle", &oracle_level },
                  { "orcus", &orcus_level },
                  { "rogue", &rogue_level },
                  { "sanctum", &sanctum_level },
                  { "valley", &valley_level },
                  { "water", &water_level },
                  { "wizard1", &wiz1_level },
                  { "wizard2", &wiz2_level },
                  { "wizard3", &wiz3_level },
                  { "minend", &mineend_level },
                  { "soko1", &sokoend_level },
                  { X_START, &qstart_level },
                  { X_LOCATE, &qlocate_level },
                  { X_GOAL, &nemesis_level },
                  { "", (d_level *) 0 } };

int
get_dgn_flags(L)
lua_State *L;
{
    int dgn_flags = 0;
    const char *const flagstrs[] = { "town", "hellish", "mazelike", "roguelike", NULL};
    const int flagstrs2i[] = { TOWN, HELLISH, MAZELIKE, ROGUELIKE, 0 };

    lua_getfield(L, -1, "flags");
    if (lua_type(L, -1) == LUA_TTABLE) {
        int f, nflags;

        lua_len(L, -1);
        nflags = (int) lua_tointeger(L, -1);
        lua_pop(L, 1);
        for (f = 0; f < nflags; f++) {
            lua_pushinteger(L, f+1);
            lua_gettable(L, -2);
            if (lua_type(L, -1) == LUA_TSTRING) {
                dgn_flags |= flagstrs2i[luaL_checkoption(L, -1, NULL, flagstrs)];
                lua_pop(L, 1);
            } else
                impossible("flags[%i] is not a string", f);
        }
    } else if (lua_type(L, -1) == LUA_TSTRING) {
        dgn_flags |= flagstrs2i[luaL_checkoption(L, -1, NULL, flagstrs)];
    } else if (lua_type(L, -1) != LUA_TNIL)
        impossible("flags is not an array or string");
    lua_pop(L, 1);

    return dgn_flags;
}

/* initialize the "dungeon" structs */
void
init_dungeons()
{
    const char *const dgnaligns[] = { "unaligned", "noalign", "lawful", "neutral", "chaotic", NULL};
    const int dgnaligns2i[] = { D_ALIGN_NONE, D_ALIGN_NONE, D_ALIGN_LAWFUL, D_ALIGN_NEUTRAL, D_ALIGN_CHAOTIC, D_ALIGN_NONE };
    lua_State *L;
    register int i, cl = 0;
    register s_level *x;
    struct proto_dungeon pd;
    struct level_map *lev_map;
    int tidx;

    (void) memset(&pd, 0, sizeof(struct proto_dungeon));
    pd.n_levs = pd.n_brs = 0;

    L = nhl_init();
    if (!nhl_loadlua(L, DUNGEON_FILE)) {
        char tbuf[BUFSZ];
        Sprintf(tbuf, "Cannot open dungeon description - \"%s", DUNGEON_FILE);
#ifdef DLBRSRC /* using a resource from the executable */
        Strcat(tbuf, "\" resource!");
#else /* using a file or DLB file */
#if defined(DLB)
        Strcat(tbuf, "\" from ");
#ifdef PREFIXES_IN_USE
        Strcat(tbuf, "\n\"");
        if (g.fqn_prefix[DATAPREFIX])
            Strcat(tbuf, g.fqn_prefix[DATAPREFIX]);
#else
        Strcat(tbuf, "\"");
#endif
        Strcat(tbuf, DLBFILE);
#endif
        Strcat(tbuf, "\" file!");
#endif
#ifdef WIN32
        interject_assistance(1, INTERJECT_PANIC, (genericptr_t) tbuf,
                             (genericptr_t) g.fqn_prefix[DATAPREFIX]);
#endif
        panic1(tbuf);
    }

    if (iflags.window_inited)
        clear_nhwindow(WIN_MAP);

    g.sp_levchn = (s_level *) 0;

    lua_settop(L, 0);

    lua_getglobal(L, "dungeon");
    if (!lua_istable(L, -1))
        panic("dungeon is not a lua table");

    lua_len(L, -1);
    g.n_dgns = (int) lua_tointeger(L, -1);
    lua_pop(L, 1);

    pd.start = 0;
    pd.n_levs = 0;
    pd.n_brs = 0;

    /*
     * Read in each dungeon and transfer the results to the internal
     * dungeon arrays.
     */

    if (g.n_dgns >= MAXDUNGEON)
        panic("init_dungeons: too many dungeons");

    tidx = lua_gettop(L);

    lua_pushnil(L); /* first key */
    i = 0;
    while (lua_next(L, tidx) != 0) {
        char *dgn_name, *dgn_bonetag, *dgn_protoname;
        int dgn_base, dgn_range, dgn_align, dgn_entry, dgn_chance, dgn_flags;

        if (!lua_istable(L, -1))
            panic("dungeon[%i] is not a lua table", i);

        dgn_name = get_table_str(L, "name");
        dgn_bonetag = get_table_str_opt(L, "bonetag", emptystr); /* TODO: single char or "none" */
        dgn_protoname = get_table_str_opt(L, "protofile", emptystr);
        dgn_base = get_table_int(L, "base");
        dgn_range = get_table_int_opt(L, "range", 0);
        dgn_align = dgnaligns2i[get_table_option(L, "alignment", "unaligned", dgnaligns)];
        dgn_entry = get_table_int_opt(L, "entry", 0);
        dgn_chance = get_table_int_opt(L, "chance", 100);
        dgn_flags = get_dgn_flags(L);

        debugpline4("DUNGEON[%i]: %s, base=(%i,%i)", i, dgn_name, dgn_base, dgn_range);

        if (!wizard && dgn_chance && (dgn_chance <= rn2(100))) {
            debugpline1("IGNORING %s", dgn_name);
            g.n_dgns--;
            lua_pop(L, 1); /* pop the dungeon table */
            continue;
        }

        /* levels begin */
        lua_getfield(L, -1, "levels");
        if (lua_type(L, -1) == LUA_TTABLE) {
            int f, nlevels;

            lua_len(L, -1);
            nlevels = (int) lua_tointeger(L, -1);
            pd.tmpdungeon[i].levels = nlevels;
            lua_pop(L, 1);
            for (f = 0; f < nlevels; f++) {
                lua_pushinteger(L, f+1);
                lua_gettable(L, -2);
                if (lua_type(L, -1) == LUA_TTABLE) {
                    int bi;
                    char *lvl_name = get_table_str(L, "name");
                    char *lvl_bonetag = get_table_str_opt(L, "bonetag", emptystr);
                    int lvl_base = get_table_int(L, "base");
                    int lvl_range = get_table_int_opt(L, "range", 0);
                    int lvl_nlevels = get_table_int_opt(L, "nlevels", 0);
                    int lvl_chance = get_table_int_opt(L, "chance", 100);
                    char *lvl_chain = get_table_str_opt(L, "chainlevel", NULL);
                    int lvl_align = dgnaligns2i[get_table_option(L, "alignment", "unaligned", dgnaligns)];
                    int lvl_flags = get_dgn_flags(L);
                    struct tmplevel *tmpl = &pd.tmplevel[pd.n_levs + f];

                    debugpline4("LEVEL[%i]:%s,(%i,%i)", f, lvl_name, lvl_base, lvl_range);
                    tmpl->name = lvl_name;
                    tmpl->chainlvl = lvl_chain;
                    tmpl->lev.base = lvl_base;
                    tmpl->lev.rand = lvl_range;
                    tmpl->chance = lvl_chance;
                    tmpl->rndlevs = lvl_nlevels;
                    tmpl->flags = lvl_flags | lvl_align;
                    tmpl->boneschar = *lvl_bonetag ? *lvl_bonetag : 0;
                    free(lvl_bonetag);
                    tmpl->chain = -1;
                    if (lvl_chain) {
                        debugpline1("CHAINLEVEL: %s", lvl_chain);
                        for (bi = 0; bi < pd.n_levs + f; bi++) {
                            debugpline2("checking(%i):%s", bi, pd.tmplevel[bi].name);
                            if (!strcmp(pd.tmplevel[bi].name, lvl_chain)) {
                                tmpl->chain = bi;
                                break;
                            }
                        }
                        if (tmpl->chain == -1)
                            panic("Could not chain level %s to %s", lvl_name, lvl_chain);
                        free(lvl_chain);
                    }
                } else
                    panic("dungeon[%i].levels[%i] is not a hash", i, f);
                lua_pop(L, 1);
            }
            pd.n_levs += nlevels;
            if (pd.n_levs > LEV_LIMIT)
                panic("init_dungeon: too many special levels");
        } else if (lua_type(L, -1) != LUA_TNIL)
            panic("dungeon[%i].levels is not an array of hashes", i);
        lua_pop(L, 1);
        /* levels end */

        /* branches begin */
        lua_getfield(L, -1, "branches");
        if (lua_type(L, -1) == LUA_TTABLE) {
            int f, nbranches;

            lua_len(L, -1);
            nbranches = (int) lua_tointeger(L, -1);
            pd.tmpdungeon[i].branches = nbranches;
            lua_pop(L, 1);
            for (f = 0; f < nbranches; f++) {
                lua_pushinteger(L, f+1);
                lua_gettable(L, -2);
                if (lua_type(L, -1) == LUA_TTABLE) {
                    int bi;
                    const char *const brdirstr[] = { "up", "down", NULL };
                    const int brdirstr2i[] = { TRUE, FALSE, FALSE };
                    const char *const brtypes[] = { "stair", "portal", "no_down", "no_up", NULL };
                    const int brtypes2i[] = { TBR_STAIR, TBR_PORTAL, TBR_NO_DOWN, TBR_NO_UP, TBR_STAIR };
                    char *br_name = get_table_str(L, "name");
                    int br_base = get_table_int(L, "base");
                    int br_range = get_table_int_opt(L, "range", 0);
                    int br_type = brtypes2i[get_table_option(L, "branchtype", "stair", brtypes)];
                    int br_up = brdirstr2i[get_table_option(L, "direction", "down", brdirstr)];
                    char *br_chain = get_table_str_opt(L, "chainlevel", NULL);
                    struct tmpbranch *tmpb = &pd.tmpbranch[pd.n_brs + f];

                    debugpline4("BRANCH[%i]:%s,(%i,%i)", f, br_name, br_base, br_range);
                    tmpb->name = br_name;
                    tmpb->lev.base = br_base;
                    tmpb->lev.rand = br_range;
                    tmpb->type = br_type;
                    tmpb->up = br_up;
                    tmpb->chain = -1;
                    if (br_chain) {
                        debugpline1("CHAINBRANCH:%s", br_chain);
                        for (bi = 0; bi < pd.n_levs + f - 1; bi++)
                            if (!strcmp(pd.tmplevel[bi].name, br_chain)) {
                                tmpb->chain = bi;
                                break;
                            }
                        if (tmpb->chain == -1)
                            panic("Could not chain branch %s to level %s", br_name, br_chain);
                        free(br_chain);
                    }
                } else
                    panic("dungeon[%i].branches[%i] is not a hash", i, f);
                lua_pop(L, 1);
            }
            pd.n_brs += nbranches;
            if (pd.n_brs > BRANCH_LIMIT)
                panic("init_dungeon: too many branches");
        } else if (lua_type(L, -1) != LUA_TNIL)
            panic("dungeon[%i].branches is not an array of hashes", i);
        lua_pop(L, 1);
        /* branches end */

        pd.tmpdungeon[i].name = dgn_name;
        pd.tmpdungeon[i].protoname = dgn_protoname;
        pd.tmpdungeon[i].boneschar = *dgn_bonetag ? *dgn_bonetag : 0;
        pd.tmpdungeon[i].lev.base = dgn_base;
        pd.tmpdungeon[i].lev.rand = dgn_range;
        pd.tmpdungeon[i].flags = dgn_flags;
        pd.tmpdungeon[i].align = dgn_align;
        pd.tmpdungeon[i].chance = dgn_chance;
        pd.tmpdungeon[i].entry_lev = dgn_entry;

        Strcpy(g.dungeons[i].dname, dgn_name); /* FIXME: dname length */
        Strcpy(g.dungeons[i].proto, dgn_protoname); /* FIXME: proto length */
        g.dungeons[i].boneid = *dgn_bonetag ? *dgn_bonetag : 0;
        free(dgn_protoname);
        free(dgn_bonetag);

        if (dgn_range)
            g.dungeons[i].num_dunlevs = (xchar) rn1(dgn_range, dgn_base);
        else
            g.dungeons[i].num_dunlevs = (xchar) dgn_base;

        if (!i) {
            g.dungeons[i].ledger_start = 0;
            g.dungeons[i].depth_start = 1;
            g.dungeons[i].dunlev_ureached = 1;
        } else {
            g.dungeons[i].ledger_start =
                g.dungeons[i - 1].ledger_start + g.dungeons[i - 1].num_dunlevs;
            g.dungeons[i].dunlev_ureached = 0;
        }

        g.dungeons[i].flags.hellish = !!(dgn_flags & HELLISH);
        g.dungeons[i].flags.maze_like = !!(dgn_flags & MAZELIKE);
        g.dungeons[i].flags.rogue_like = !!(dgn_flags & ROGUELIKE);
        g.dungeons[i].flags.align = dgn_align;

        /*
         * Set the entry level for this dungeon.  The entry value means:
         *              < 0     from bottom (-1 == bottom level)
         *                0     default (top)
         *              > 0     actual level (1 = top)
         *
         * Note that the entry_lev field in the dungeon structure is
         * redundant.  It is used only here and in print_dungeon().
         */
        if (dgn_entry < 0) {
            g.dungeons[i].entry_lev =
                g.dungeons[i].num_dunlevs + dgn_entry + 1;
            if (g.dungeons[i].entry_lev <= 0)
                g.dungeons[i].entry_lev = 1;
        } else if (dgn_entry > 0) {
            g.dungeons[i].entry_lev = dgn_entry;
            if (g.dungeons[i].entry_lev > g.dungeons[i].num_dunlevs)
                g.dungeons[i].entry_lev = g.dungeons[i].num_dunlevs;
        } else {                       /* default */
            g.dungeons[i].entry_lev = 1; /* defaults to top level */
        }

        if (i) { /* set depth */
            branch *br;
            schar from_depth;
            boolean from_up;

            br = add_branch(i, g.dungeons[i].entry_lev, &pd);

            /* Get the depth of the connecting end. */
            if (br->end1.dnum == i) {
                from_depth = depth(&br->end2);
                from_up = !br->end1_up;
            } else {
                from_depth = depth(&br->end1);
                from_up = br->end1_up;
            }

            /*
             * Calculate the depth of the top of the dungeon via
             * its branch.  First, the depth of the entry point:
             *
             *  depth of branch from "parent" dungeon
             *  + -1 or 1 depending on an up or down stair or
             *    0 if portal
             *
             * Followed by the depth of the top of the dungeon:
             *
             *  - (entry depth - 1)
             *
             * We'll say that portals stay on the same depth.
             */
            g.dungeons[i].depth_start =
                from_depth + (br->type == BR_PORTAL ? 0 : (from_up ? -1 : 1))
                - (g.dungeons[i].entry_lev - 1);
        }

        if (g.dungeons[i].num_dunlevs > MAXLEVEL)
            g.dungeons[i].num_dunlevs = MAXLEVEL;


       for (; cl < pd.n_levs; cl++) {
            init_level(i, cl, &pd);
        }

        /*
         * Recursively place the generated levels for this dungeon.  This
         * routine will attempt all possible combinations before giving
         * up.
         */
        if (!place_level(pd.start, &pd))
            panic("init_dungeon:  couldn't place levels");
#ifdef DDEBUG
        fprintf(stderr, "--- end of dungeon %d ---\n", i);
        fflush(stderr);
        getchar();
#endif

        for (; pd.start < pd.n_levs; pd.start++)
            if (pd.final_lev[pd.start])
                add_level(pd.final_lev[pd.start]);
        /* levels handling end */

        lua_pop(L, 1); /* pop the dungeon table */
        i++;
    }

    lua_pop(L, 1); /* get rid of the dungeon global */
    debugpline2("init_dungeon lua DONE (n_levs=%i, n_brs=%i)", pd.n_levs, pd.n_brs);

    for (i = 0; i < 5; i++)
        g.tune[i] = 'A' + rn2(7);
    g.tune[5] = 0;

    /*
     * Find most of the special levels and dungeons so we can access their
     * locations quickly.
     */
    for (lev_map = level_map; lev_map->lev_name[0]; lev_map++) {
        x = find_level(lev_map->lev_name);
        if (x) {
            assign_level(lev_map->lev_spec, &x->dlevel);
            if (!strncmp(lev_map->lev_name, "x-", 2)) {
                /* This is where the name substitution on the
                 * levels of the quest dungeon occur.
                 */
                Sprintf(x->proto, "%s%s", g.urole.filecode,
                        &lev_map->lev_name[1]);
            } else if (lev_map->lev_spec == &knox_level) {
                branch *br;
                /*
                 * Kludge to allow floating Knox entrance.  We
                 * specify a floating entrance by the fact that
                 * its entrance (end1) has a bogus dnum, namely
                 * n_dgns.
                 */
                for (br = g.branches; br; br = br->next)
                    if (on_level(&br->end2, &knox_level))
                        break;

                if (br)
                    br->end1.dnum = g.n_dgns;
                /* adjust the branch's position on the list */
                insert_branch(br, TRUE);
            }
        }
    }
    /*
     *  I hate hardwiring these names. :-(
     */
    quest_dnum = dname_to_dnum("The Quest");
    sokoban_dnum = dname_to_dnum("Sokoban");
    mines_dnum = dname_to_dnum("The Gnomish Mines");
    tower_dnum = dname_to_dnum("Vlad's Tower");

    /* one special fixup for dummy surface level */
    if ((x = find_level("dummy")) != 0) {
        i = x->dlevel.dnum;
        /* the code above puts earth one level above dungeon level #1,
           making the dummy level overlay level 1; but the whole reason
           for having the dummy level is to make earth have depth -1
           instead of 0, so adjust the start point to shift endgame up */
        if (dunlevs_in_dungeon(&x->dlevel) > 1 - g.dungeons[i].depth_start)
            g.dungeons[i].depth_start -= 1;
        /* TODO: strip "dummy" out all the way here,
           so that it's hidden from '#wizwhere' feedback. */
    }

    lua_close(L);

    for (i = 0; i < pd.n_brs; i++) {
        free(pd.tmpbranch[i].name);
    }
    for (i = 0; i < pd.n_levs; i++) {
        free(pd.tmplevel[i].name);
    }
    for (i = 0; i < g.n_dgns; i++) {
        free(pd.tmpdungeon[i].name);
    }

#ifdef DEBUG
    dumpit();
#endif
}

/* return the level number for lev in *this* dungeon */
xchar
dunlev(lev)
d_level *lev;
{
    return lev->dlevel;
}

/* return the lowest level number for *this* dungeon */
xchar
dunlevs_in_dungeon(lev)
d_level *lev;
{
    return g.dungeons[lev->dnum].num_dunlevs;
}

/* return the lowest level explored in the game*/
xchar
deepest_lev_reached(noquest)
boolean noquest;
{
    /* this function is used for three purposes: to provide a factor
     * of difficulty in monster generation; to provide a factor of
     * difficulty in experience calculations (botl.c and end.c); and
     * to insert the deepest level reached in the game in the topten
     * display.  the 'noquest' arg switch is required for the latter.
     *
     * from the player's point of view, going into the Quest is _not_
     * going deeper into the dungeon -- it is going back "home", where
     * the dungeon starts at level 1.  given the setup in dungeon.def,
     * the depth of the Quest (thought of as starting at level 1) is
     * never lower than the level of entry into the Quest, so we exclude
     * the Quest from the topten "deepest level reached" display
     * calculation.  _However_ the Quest is a difficult dungeon, so we
     * include it in the factor of difficulty calculations.
     */
    register int i;
    d_level tmp;
    register schar ret = 0;

    for (i = 0; i < g.n_dgns; i++) {
        if (noquest && i == quest_dnum)
            continue;
        tmp.dlevel = g.dungeons[i].dunlev_ureached;
        if (tmp.dlevel == 0)
            continue;
        tmp.dnum = i;
        if (depth(&tmp) > ret)
            ret = depth(&tmp);
    }
    return (xchar) ret;
}

/* return a bookkeeping level number for purpose of comparisons and
   save/restore */
xchar
ledger_no(lev)
d_level *lev;
{
    return (xchar) (lev->dlevel + g.dungeons[lev->dnum].ledger_start);
}

/*
 * The last level in the bookkeeping list of level is the bottom of the last
 * dungeon in the g.dungeons[] array.
 *
 * Maxledgerno() -- which is the max number of levels in the bookkeeping
 * list, should not be confused with dunlevs_in_dungeon(lev) -- which
 * returns the max number of levels in lev's dungeon, and both should
 * not be confused with deepest_lev_reached() -- which returns the lowest
 * depth visited by the player.
 */
xchar
maxledgerno()
{
    return (xchar) (g.dungeons[g.n_dgns - 1].ledger_start
                    + g.dungeons[g.n_dgns - 1].num_dunlevs);
}

/* return the dungeon that this ledgerno exists in */
xchar
ledger_to_dnum(ledgerno)
xchar ledgerno;
{
    register int i;

    /* find i such that (i->base + 1) <= ledgerno <= (i->base + i->count) */
    for (i = 0; i < g.n_dgns; i++)
        if (g.dungeons[i].ledger_start < ledgerno
            && ledgerno <= g.dungeons[i].ledger_start + g.dungeons[i].num_dunlevs)
            return (xchar) i;

    panic("level number out of range [ledger_to_dnum(%d)]", (int) ledgerno);
    /*NOT REACHED*/
    return (xchar) 0;
}

/* return the level of the dungeon this ledgerno exists in */
xchar
ledger_to_dlev(ledgerno)
xchar ledgerno;
{
    return (xchar) (ledgerno
                    - g.dungeons[ledger_to_dnum(ledgerno)].ledger_start);
}

/* returns the depth of a level, in floors below the surface
   (note levels in different dungeons can have the same depth) */
schar
depth(lev)
d_level *lev;
{
    return (schar) (g.dungeons[lev->dnum].depth_start + lev->dlevel - 1);
}

/* are "lev1" and "lev2" actually the same? */
boolean
on_level(lev1, lev2)
d_level *lev1, *lev2;
{
    return (boolean) (lev1->dnum == lev2->dnum
                      && lev1->dlevel == lev2->dlevel);
}

/* is this level referenced in the special level chain? */
s_level *
Is_special(lev)
d_level *lev;
{
    s_level *levtmp;

    for (levtmp = g.sp_levchn; levtmp; levtmp = levtmp->next)
        if (on_level(lev, &levtmp->dlevel))
            return levtmp;

    return (s_level *) 0;
}

/*
 * Is this a multi-dungeon branch level?  If so, return a pointer to the
 * branch.  Otherwise, return null.
 */
branch *
Is_branchlev(lev)
d_level *lev;
{
    branch *curr;

    for (curr = g.branches; curr; curr = curr->next) {
        if (on_level(lev, &curr->end1) || on_level(lev, &curr->end2))
            return curr;
    }
    return (branch *) 0;
}

/* returns True iff the branch 'lev' is in a branch which builds up */
boolean
builds_up(lev)
d_level *lev;
{
    dungeon *dptr = &g.dungeons[lev->dnum];
    /*
     * FIXME:  this misclassifies a single level branch reached via stairs
     * from below.  Saving grace is that no such branches currently exist.
     */
    return (boolean) (dptr->num_dunlevs > 1
                      && dptr->entry_lev == dptr->num_dunlevs);
}

/* goto the next level (or appropriate dungeon) */
void
next_level(at_stairs)
boolean at_stairs;
{
    if (at_stairs && u.ux == g.sstairs.sx && u.uy == g.sstairs.sy) {
        /* Taking a down dungeon branch. */
        goto_level(&g.sstairs.tolev, at_stairs, FALSE, FALSE);
    } else {
        /* Going down a stairs or jump in a trap door. */
        d_level newlevel;

        newlevel.dnum = u.uz.dnum;
        newlevel.dlevel = u.uz.dlevel + 1;
        goto_level(&newlevel, at_stairs, !at_stairs, FALSE);
    }
}

/* goto the previous level (or appropriate dungeon) */
void
prev_level(at_stairs)
boolean at_stairs;
{
    if (at_stairs && u.ux == g.sstairs.sx && u.uy == g.sstairs.sy) {
        /* Taking an up dungeon branch. */
        /* KMH -- Upwards branches are okay if not level 1 */
        /* (Just make sure it doesn't go above depth 1) */
        if (!u.uz.dnum && u.uz.dlevel == 1 && !u.uhave.amulet)
            done(ESCAPED);
        else
            goto_level(&g.sstairs.tolev, at_stairs, FALSE, FALSE);
    } else {
        /* Going up a stairs or rising through the ceiling. */
        d_level newlevel;
        newlevel.dnum = u.uz.dnum;
        newlevel.dlevel = u.uz.dlevel - 1;
        goto_level(&newlevel, at_stairs, FALSE, FALSE);
    }
}

void
u_on_newpos(x, y)
int x, y;
{
    if (!isok(x, y)) { /* validate location */
        void VDECL((*func), (const char *, ...)) PRINTF_F(1, 2);

        func = (x < 0 || y < 0 || x > COLNO - 1 || y > ROWNO - 1) ? panic
               : impossible;
        (*func)("u_on_newpos: trying to place hero off map <%d,%d>", x, y);
    }
    u.ux = x;
    u.uy = y;
#ifdef CLIPPING
    cliparound(u.ux, u.uy);
#endif
    /* ridden steed always shares hero's location */
    if (u.usteed)
        u.usteed->mx = u.ux, u.usteed->my = u.uy;
    /* when changing levels, don't leave old position set with
       stale values from previous level */
    if (!on_level(&u.uz, &u.uz0))
        u.ux0 = u.ux, u.uy0 = u.uy;
}

/* place you on a random location when arriving on a level */
void
u_on_rndspot(upflag)
int upflag;
{
    int up = (upflag & 1), was_in_W_tower = (upflag & 2);

    /*
     * Place the hero at a random location within the relevant region.
     * place_lregion(xTELE) -> put_lregion_here(xTELE) -> u_on_newpos()
     * Unspecified region (.lx == 0) defaults to entire level.
     */
    if (was_in_W_tower && On_W_tower_level(&u.uz))
        /* Stay inside the Wizard's tower when feasible.
           We use the W Tower's exclusion region for the
           destination instead of its enclosing region.
           Note: up vs down doesn't matter in this case
           because both specify the same exclusion area. */
        place_lregion(g.dndest.nlx, g.dndest.nly, g.dndest.nhx, g.dndest.nhy,
                      0, 0, 0, 0, LR_DOWNTELE, (d_level *) 0);
    else if (up)
        place_lregion(g.updest.lx, g.updest.ly, g.updest.hx, g.updest.hy,
                      g.updest.nlx, g.updest.nly, g.updest.nhx, g.updest.nhy,
                      LR_UPTELE, (d_level *) 0);
    else
        place_lregion(g.dndest.lx, g.dndest.ly, g.dndest.hx, g.dndest.hy,
                      g.dndest.nlx, g.dndest.nly, g.dndest.nhx, g.dndest.nhy,
                      LR_DOWNTELE, (d_level *) 0);

    /* might have just left solid rock and unblocked levitation */
    switch_terrain();
}

/* place you on the special staircase */
void
u_on_sstairs(upflag)
int upflag;
{
    if (g.sstairs.sx)
        u_on_newpos(g.sstairs.sx, g.sstairs.sy);
    else
        u_on_rndspot(upflag);
}

/* place you on upstairs (or special equivalent) */
void
u_on_upstairs()
{
    if (xupstair)
        u_on_newpos(xupstair, yupstair);
    else
        u_on_sstairs(0); /* destination upstairs implies moving down */
}

/* place you on dnstairs (or special equivalent) */
void
u_on_dnstairs()
{
    if (xdnstair)
        u_on_newpos(xdnstair, ydnstair);
    else
        u_on_sstairs(1); /* destination dnstairs implies moving up */
}

boolean
On_stairs(x, y)
xchar x, y;
{
    return (boolean) ((x == xupstair && y == yupstair)
                      || (x == xdnstair && y == ydnstair)
                      || (x == xdnladder && y == ydnladder)
                      || (x == xupladder && y == yupladder)
                      || (x == g.sstairs.sx && y == g.sstairs.sy));
}

boolean
Is_botlevel(lev)
d_level *lev;
{
    return (boolean) (lev->dlevel == g.dungeons[lev->dnum].num_dunlevs);
}

boolean
Can_dig_down(lev)
d_level *lev;
{
    return (boolean) (!g.level.flags.hardfloor
                      && !Is_botlevel(lev)
                      && !Invocation_lev(lev));
}

/*
 * Like Can_dig_down (above), but also allows falling through on the
 * stronghold level.  Normally, the bottom level of a dungeon resists
 * both digging and falling.
 */
boolean
Can_fall_thru(lev)
d_level *lev;
{
    return (boolean) (Can_dig_down(lev) || Is_stronghold(lev));
}

/*
 * True if one can rise up a level (e.g. cursed gain level).
 * This happens on intermediate dungeon levels or on any top dungeon
 * level that has a stairwell style branch to the next higher dungeon.
 * Checks for amulets and such must be done elsewhere.
 */
boolean
Can_rise_up(x, y, lev)
int x, y;
d_level *lev;
{
    /* can't rise up from inside the top of the Wizard's tower */
    /* KMH -- or in sokoban */
    if (In_endgame(lev) || In_sokoban(lev)
        || (Is_wiz1_level(lev) && In_W_tower(x, y, lev)))
        return FALSE;
    return (boolean) (lev->dlevel > 1
                      || (g.dungeons[lev->dnum].entry_lev == 1
                          && ledger_no(lev) != 1
                          && g.sstairs.sx && g.sstairs.up));
}

boolean
has_ceiling(lev)
d_level *lev;
{
    /* [what about level 1 of the quest?] */
    return (boolean) (!Is_airlevel(lev) && !Is_waterlevel(lev));
}

/*
 * It is expected that the second argument of get_level is a depth value,
 * either supplied by the user (teleport control) or randomly generated.
 * But more than one level can be at the same depth.  If the target level
 * is "above" the present depth location, get_level must trace "up" from
 * the player's location (through the ancestors dungeons) the dungeon
 * within which the target level is located.  With only one exception
 * which does not pass through this routine (see level_tele), teleporting
 * "down" is confined to the current dungeon.  At present, level teleport
 * in dungeons that build up is confined within them.
 */
void
get_level(newlevel, levnum)
d_level *newlevel;
int levnum;
{
    branch *br;
    xchar dgn = u.uz.dnum;

    if (levnum <= 0) {
        /* can only currently happen in endgame */
        levnum = u.uz.dlevel;
    } else if (levnum
               > g.dungeons[dgn].depth_start + g.dungeons[dgn].num_dunlevs - 1) {
        /* beyond end of dungeon, jump to last level */
        levnum = g.dungeons[dgn].num_dunlevs;
    } else {
        /* The desired level is in this dungeon or a "higher" one. */

        /*
         * Branch up the tree until we reach a dungeon that contains the
         * levnum.
         */
        if (levnum < g.dungeons[dgn].depth_start) {
            do {
                /*
                 * Find the parent dungeon of this dungeon.
                 *
                 * This assumes that end2 is always the "child" and it is
                 * unique.
                 */
                for (br = g.branches; br; br = br->next)
                    if (br->end2.dnum == dgn)
                        break;
                if (!br)
                    panic("get_level: can't find parent dungeon");

                dgn = br->end1.dnum;
            } while (levnum < g.dungeons[dgn].depth_start);
        }

        /* We're within the same dungeon; calculate the level. */
        levnum = levnum - g.dungeons[dgn].depth_start + 1;
    }

    newlevel->dnum = dgn;
    newlevel->dlevel = levnum;
}

/* are you in the quest dungeon? */
boolean
In_quest(lev)
d_level *lev;
{
    return (boolean) (lev->dnum == quest_dnum);
}

/* are you in the mines dungeon? */
boolean
In_mines(lev)
d_level *lev;
{
    return (boolean) (lev->dnum == mines_dnum);
}

/*
 * Return the branch for the given dungeon.
 *
 * This function assumes:
 *      + This is not called with "Dungeons of Doom".
 *      + There is only _one_ branch to a given dungeon.
 *      + Field end2 is the "child" dungeon.
 */
branch *
dungeon_branch(s)
const char *s;
{
    branch *br;
    xchar dnum;

    dnum = dname_to_dnum(s);

    /* Find the branch that connects to dungeon i's branch. */
    for (br = g.branches; br; br = br->next)
        if (br->end2.dnum == dnum)
            break;

    if (!br)
        panic("dgn_entrance: can't find entrance to %s", s);

    return br;
}

/*
 * This returns true if the hero is on the same level as the entrance to
 * the named dungeon.
 *
 * Called from do.c and mklev.c.
 *
 * Assumes that end1 is always the "parent".
 */
boolean
at_dgn_entrance(s)
const char *s;
{
    branch *br;

    br = dungeon_branch(s);
    return on_level(&u.uz, &br->end1) ? TRUE : FALSE;
}

/* is `lev' part of Vlad's tower? */
boolean
In_V_tower(lev)
d_level *lev;
{
    return (boolean) (lev->dnum == tower_dnum);
}

/* is `lev' a level containing the Wizard's tower? */
boolean
On_W_tower_level(lev)
d_level *lev;
{
    return (boolean) (Is_wiz1_level(lev)
                      || Is_wiz2_level(lev)
                      || Is_wiz3_level(lev));
}

/* is <x,y> of `lev' inside the Wizard's tower? */
boolean
In_W_tower(x, y, lev)
int x, y;
d_level *lev;
{
    if (!On_W_tower_level(lev))
        return FALSE;
    /*
     * Both of the exclusion regions for arriving via level teleport
     * (from above or below) define the tower's boundary.
     *  assert( g.updest.nIJ == g.dndest.nIJ for I={l|h},J={x|y} );
     */
    if (g.dndest.nlx > 0)
        return (boolean) within_bounded_area(x, y, g.dndest.nlx, g.dndest.nly,
                                             g.dndest.nhx, g.dndest.nhy);
    else
        impossible("No boundary for Wizard's Tower?");
    return FALSE;
}

/* are you in one of the Hell levels? */
boolean
In_hell(lev)
d_level *lev;
{
    return (boolean) (g.dungeons[lev->dnum].flags.hellish);
}

/* sets *lev to be the gateway to Gehennom... */
void
find_hell(lev)
d_level *lev;
{
    lev->dnum = valley_level.dnum;
    lev->dlevel = 1;
}

/* go directly to hell... */
void
goto_hell(at_stairs, falling)
boolean at_stairs, falling;
{
    d_level lev;

    find_hell(&lev);
    goto_level(&lev, at_stairs, falling, FALSE);
}

/* equivalent to dest = source */
void
assign_level(dest, src)
d_level *dest, *src;
{
    dest->dnum = src->dnum;
    dest->dlevel = src->dlevel;
}

/* dest = src + rn1(range) */
void
assign_rnd_level(dest, src, range)
d_level *dest, *src;
int range;
{
    dest->dnum = src->dnum;
    dest->dlevel = src->dlevel + ((range > 0) ? rnd(range) : -rnd(-range));

    if (dest->dlevel > dunlevs_in_dungeon(dest))
        dest->dlevel = dunlevs_in_dungeon(dest);
    else if (dest->dlevel < 1)
        dest->dlevel = 1;
}

int
induced_align(pct)
int pct;
{
    s_level *lev = Is_special(&u.uz);
    aligntyp al;

    if (lev && lev->flags.align)
        if (rn2(100) < pct)
            return lev->flags.align;

    if (g.dungeons[u.uz.dnum].flags.align)
        if (rn2(100) < pct)
            return g.dungeons[u.uz.dnum].flags.align;

    al = rn2(3) - 1;
    return Align2amask(al);
}

boolean
Invocation_lev(lev)
d_level *lev;
{
    return (boolean) (In_hell(lev)
                      && lev->dlevel == g.dungeons[lev->dnum].num_dunlevs - 1);
}

/* use instead of depth() wherever a degree of difficulty is made
 * dependent on the location in the dungeon (eg. monster creation).
 */
xchar
level_difficulty()
{
    int res;

    if (In_endgame(&u.uz)) {
        res = depth(&sanctum_level) + u.ulevel / 2;
    } else if (u.uhave.amulet) {
        res = deepest_lev_reached(FALSE);
    } else {
        res = depth(&u.uz);
        /* depth() is the number of elevation units (levels) below
           the theoretical surface; in a builds-up branch, that value
           ends up making the harder to reach levels be treated as if
           they were easier; adjust for the extra effort involved in
           going down to the entrance and then up to the location */
        if (builds_up(&u.uz))
            res += 2 * (g.dungeons[u.uz.dnum].entry_lev - u.uz.dlevel + 1);
            /*
             * 'Proof' by example:  suppose the entrance to sokoban is
             * on dungeon level 9, leading up to bottom sokoban level
             * of 8 [entry_lev].  When the hero is on sokoban level 8
             * [uz.dlevel], depth() yields eight but he has ventured
             * one level beyond 9, so difficulty depth should be 10:
             *   8 + 2 * (8 - 8 + 1) => 10.
             * Going up to 7, depth is 7 but hero will be two beyond 9:
             *   7 + 2 * (8 - 7 + 1) => 11.
             * When he goes up to level 6, three levels beyond 9:
             *   6 + 2 * (8 - 6 + 1) => 12.
             * And the top level of sokoban at 5, four levels beyond 9:
             *   5 + 2 * (8 - 5 + 1) => 13.
             * The same applies to Vlad's Tower, although the increment
             * there is inconsequential compared to overall depth.
             */
#if 0
        /*
         * The inside of the Wizard's Tower is also effectively a
         * builds-up area, reached from a portal an arbitrary distance
         * below rather than stairs 1 level beneath the entry level.
         */
        else if (On_W_tower_level(&u.uz) && In_W_tower(some_X, some_Y, &u.uz))
            res += (fakewiz1.dlev - u.uz.dlev);
            /*
             * Handling this properly would need more information here:
             * an inside/outside flag, or coordinates to calculate it.
             * Unfortunately level difficulty may be wanted before
             * coordinates have been chosen so simply extending this
             * routine to take extra arguments is not sufficient to cope.
             * The difference beyond naive depth-from-surface is small
             * relative to the overall depth, so just ignore complications
             * posed by W_tower.
             */
#endif /*0*/
    }
    return (xchar) res;
}

/* Take one word and try to match it to a level.
 * Recognized levels are as shown by print_dungeon().
 */
schar
lev_by_name(nam)
const char *nam;
{
    schar lev = 0;
    s_level *slev = (s_level *)0;
    d_level dlev;
    const char *p;
    int idx, idxtoo;
    char buf[BUFSZ];
    mapseen *mseen;

    /* look at the player's custom level annotations first */
    if ((mseen = find_mapseen_by_str(nam)) != 0) {
        dlev = mseen->lev;
    } else {
        /* no matching annotation, check whether they used a name we know */

        /* allow strings like "the oracle level" to find "oracle" */
        if (!strncmpi(nam, "the ", 4))
            nam += 4;
        if ((p = strstri(nam, " level")) != 0 && p == eos((char *) nam) - 6) {
            nam = strcpy(buf, nam);
            *(eos(buf) - 6) = '\0';
        }
        /* hell is the old name, and wouldn't match; gehennom would match its
           branch, yielding the castle level instead of valley of the dead */
        if (!strcmpi(nam, "gehennom") || !strcmpi(nam, "hell")) {
            if (In_V_tower(&u.uz))
                nam = " to Vlad's tower"; /* branch to... */
            else
                nam = "valley";
        }

        if ((slev = find_level(nam)) != 0)
            dlev = slev->dlevel;
    }

    if (mseen || slev) {
        idx = ledger_no(&dlev);
        if ((dlev.dnum == u.uz.dnum
             /* within same branch, or else main dungeon <-> gehennom */
             || (u.uz.dnum == valley_level.dnum
                 && dlev.dnum == medusa_level.dnum)
             || (u.uz.dnum == medusa_level.dnum
                 && dlev.dnum == valley_level.dnum))
            && (/* either wizard mode or else seen and not forgotten */
                wizard
                || (g.level_info[idx].flags & (FORGOTTEN | VISITED))
                       == VISITED)) {
            lev = depth(&dlev);
        }
    } else { /* not a specific level; try branch names */
        idx = find_branch(nam, (struct proto_dungeon *) 0);
        /* "<branch> to Xyzzy" */
        if (idx < 0 && (p = strstri(nam, " to ")) != 0)
            idx = find_branch(p + 4, (struct proto_dungeon *) 0);

        if (idx >= 0) {
            idxtoo = (idx >> 8) & 0x00FF;
            idx &= 0x00FF;
            /* either wizard mode, or else _both_ sides of branch seen */
            if (wizard
                || (((g.level_info[idx].flags & (FORGOTTEN | VISITED))
                     == VISITED)
                    && ((g.level_info[idxtoo].flags & (FORGOTTEN | VISITED))
                        == VISITED))) {
                if (ledger_to_dnum(idxtoo) == u.uz.dnum)
                    idx = idxtoo;
                dlev.dnum = ledger_to_dnum(idx);
                dlev.dlevel = ledger_to_dlev(idx);
                lev = depth(&dlev);
            }
        }
    }
    return lev;
}

static boolean
unplaced_floater(dptr)
struct dungeon *dptr;
{
    branch *br;
    int idx = (int) (dptr - g.dungeons);

    /* if other floating branches are added, this will need to change */
    if (idx != knox_level.dnum)
        return FALSE;
    for (br = g.branches; br; br = br->next)
        if (br->end1.dnum == g.n_dgns && br->end2.dnum == idx)
            return TRUE;
    return FALSE;
}

static boolean
unreachable_level(lvl_p, unplaced)
d_level *lvl_p;
boolean unplaced;
{
    s_level *dummy;

    if (unplaced)
        return TRUE;
    if (In_endgame(&u.uz) && !In_endgame(lvl_p))
        return TRUE;
    if ((dummy = find_level("dummy")) != 0 && on_level(lvl_p, &dummy->dlevel))
        return TRUE;
    return FALSE;
}

static void
tport_menu(win, entry, lchoices, lvl_p, unreachable)
winid win;
char *entry;
struct lchoice *lchoices;
d_level *lvl_p;
boolean unreachable;
{
    char tmpbuf[BUFSZ];
    anything any;

    lchoices->lev[lchoices->idx] = lvl_p->dlevel;
    lchoices->dgn[lchoices->idx] = lvl_p->dnum;
    lchoices->playerlev[lchoices->idx] = depth(lvl_p);
    any = cg.zeroany;
    if (unreachable) {
        /* not selectable, but still consumes next menuletter;
           prepend padding in place of missing menu selector */
        Sprintf(tmpbuf, "    %s", entry);
        entry = tmpbuf;
    } else {
        any.a_int = lchoices->idx + 1;
    }
    add_menu(win, NO_GLYPH, &any, lchoices->menuletter, 0, ATR_NONE, entry,
             MENU_UNSELECTED);
    /* this assumes there are at most 52 interesting levels */
    if (lchoices->menuletter == 'z')
        lchoices->menuletter = 'A';
    else
        lchoices->menuletter++;
    lchoices->idx++;
    return;
}

/* Convert a branch type to a string usable by print_dungeon(). */
static const char *
br_string(type)
int type;
{
    switch (type) {
    case BR_PORTAL:
        return "Portal";
    case BR_NO_END1:
        return "Connection";
    case BR_NO_END2:
        return "One way stair";
    case BR_STAIR:
        return "Stair";
    }
    return " (unknown)";
}

static char
chr_u_on_lvl(dlev)
d_level *dlev;
{
    return u.uz.dnum == dlev->dnum && u.uz.dlevel == dlev->dlevel ? '*' : ' ';
}

/* Print all child branches between the lower and upper bounds. */
static void
print_branch(win, dnum, lower_bound, upper_bound, bymenu, lchoices_p)
winid win;
int dnum;
int lower_bound;
int upper_bound;
boolean bymenu;
struct lchoice *lchoices_p;
{
    branch *br;
    char buf[BUFSZ];

    /* This assumes that end1 is the "parent". */
    for (br = g.branches; br; br = br->next) {
        if (br->end1.dnum == dnum && lower_bound < br->end1.dlevel
            && br->end1.dlevel <= upper_bound) {
            Sprintf(buf, "%c %s to %s: %d",
                    bymenu ? chr_u_on_lvl(&br->end1) : ' ',
                    br_string(br->type),
                    g.dungeons[br->end2.dnum].dname, depth(&br->end1));
            if (bymenu)
                tport_menu(win, buf, lchoices_p, &br->end1,
                           unreachable_level(&br->end1, FALSE));
            else
                putstr(win, 0, buf);
        }
    }
}

/* Print available dungeon information. */
schar
print_dungeon(bymenu, rlev, rdgn)
boolean bymenu;
schar *rlev;
xchar *rdgn;
{
    int i, last_level, nlev;
    char buf[BUFSZ];
    const char *descr;
    boolean first, unplaced;
    s_level *slev;
    dungeon *dptr;
    branch *br;
    anything any;
    struct lchoice lchoices;
    winid win = create_nhwindow(NHW_MENU);

    if (bymenu) {
        start_menu(win);
        lchoices.idx = 0;
        lchoices.menuletter = 'a';
    }

    for (i = 0, dptr = g.dungeons; i < g.n_dgns; i++, dptr++) {
        if (bymenu && In_endgame(&u.uz) && i != astral_level.dnum)
            continue;
        unplaced = unplaced_floater(dptr);
        descr = unplaced ? "depth" : "level";
        nlev = dptr->num_dunlevs;
        if (nlev > 1)
            Sprintf(buf, "%s: %s %d to %d", dptr->dname, makeplural(descr),
                    dptr->depth_start, dptr->depth_start + nlev - 1);
        else
            Sprintf(buf, "%s: %s %d", dptr->dname, descr, dptr->depth_start);

        /* Most entrances are uninteresting. */
        if (dptr->entry_lev != 1) {
            if (dptr->entry_lev == nlev)
                Strcat(buf, ", entrance from below");
            else
                Sprintf(eos(buf), ", entrance on %d",
                        dptr->depth_start + dptr->entry_lev - 1);
        }
        if (bymenu) {
            any = cg.zeroany;
            add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings, buf,
                     MENU_UNSELECTED);
        } else
            putstr(win, 0, buf);

        /*
         * Circle through the special levels to find levels that are in
         * this dungeon.
         */
        for (slev = g.sp_levchn, last_level = 0; slev; slev = slev->next) {
            if (slev->dlevel.dnum != i)
                continue;

            /* print any branches before this level */
            print_branch(win, i, last_level, slev->dlevel.dlevel, bymenu,
                         &lchoices);

            Sprintf(buf, "%c %s: %d",
                    chr_u_on_lvl(&slev->dlevel),
                    slev->proto, depth(&slev->dlevel));
            if (Is_stronghold(&slev->dlevel))
                Sprintf(eos(buf), " (tune %s)", g.tune);
            if (bymenu)
                tport_menu(win, buf, &lchoices, &slev->dlevel,
                           unreachable_level(&slev->dlevel, unplaced));
            else
                putstr(win, 0, buf);

            last_level = slev->dlevel.dlevel;
        }
        /* print branches after the last special level */
        print_branch(win, i, last_level, MAXLEVEL, bymenu, &lchoices);
    }

    if (bymenu) {
        int n;
        menu_item *selected;
        int idx;

        end_menu(win, "Level teleport to where:");
        n = select_menu(win, PICK_ONE, &selected);
        destroy_nhwindow(win);
        if (n > 0) {
            idx = selected[0].item.a_int - 1;
            free((genericptr_t) selected);
            if (rlev && rdgn) {
                *rlev = lchoices.lev[idx];
                *rdgn = lchoices.dgn[idx];
                return lchoices.playerlev[idx];
            }
        }
        return 0;
    }

    /* Print out floating branches (if any). */
    for (first = TRUE, br = g.branches; br; br = br->next) {
        if (br->end1.dnum == g.n_dgns) {
            if (first) {
                putstr(win, 0, "");
                putstr(win, 0, "Floating branches");
                first = FALSE;
            }
            Sprintf(buf, "   %s to %s", br_string(br->type),
                    g.dungeons[br->end2.dnum].dname);
            putstr(win, 0, buf);
        }
    }

    /* I hate searching for the invocation pos while debugging. -dean */
    if (Invocation_lev(&u.uz)) {
        putstr(win, 0, "");
        Sprintf(buf, "Invocation position @ (%d,%d), hero @ (%d,%d)",
                g.inv_pos.x, g.inv_pos.y, u.ux, u.uy);
        putstr(win, 0, buf);
    } else {
        struct trap *trap;

        /* if current level has a magic portal, report its location;
           this assumes that there is at most one magic portal on any
           given level; quest and ft.ludios have pairs (one in main
           dungeon matched with one in the corresponding branch), the
           elemental planes have singletons (connection to next plane) */
        *buf = '\0';
        for (trap = g.ftrap; trap; trap = trap->ntrap)
            if (trap->ttyp == MAGIC_PORTAL)
                break;

        if (trap)
            Sprintf(buf, "Portal @ (%d,%d), hero @ (%d,%d)",
                    trap->tx, trap->ty, u.ux, u.uy);

        /* only report "no portal found" when actually expecting a portal */
        else if (Is_earthlevel(&u.uz) || Is_waterlevel(&u.uz)
                 || Is_firelevel(&u.uz) || Is_airlevel(&u.uz)
                 || Is_qstart(&u.uz) || at_dgn_entrance("The Quest")
                 || Is_knox(&u.uz))
            Strcpy(buf, "No portal found.");

        /* only give output if we found a portal or expected one and didn't */
        if (*buf) {
            putstr(win, 0, "");
            putstr(win, 0, buf);
        }
    }

    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return 0;
}

/* Record that the player knows about a branch from a level. This function
 * will determine whether or not it was a "real" branch that was taken.
 * This function should not be called for a transition done via level
 * teleport or via the Eye.
 */
void
recbranch_mapseen(source, dest)
d_level *source;
d_level *dest;
{
    mapseen *mptr;
    branch *br;

    /* not a branch */
    if (source->dnum == dest->dnum)
        return;

    /* we only care about forward branches */
    for (br = g.branches; br; br = br->next) {
        if (on_level(source, &br->end1) && on_level(dest, &br->end2))
            break;
        if (on_level(source, &br->end2) && on_level(dest, &br->end1))
            return;
    }

    /* branch not found, so not a real branch. */
    if (!br)
        return;

    if ((mptr = find_mapseen(source)) != 0) {
        if (mptr->br && br != mptr->br)
            impossible("Two branches on the same level?");
        mptr->br = br;
    } else {
        impossible("Can't note branch for unseen level (%d, %d)",
                   source->dnum, source->dlevel);
    }
}

char *
get_annotation(lev)
d_level *lev;
{
    mapseen *mptr;

    if ((mptr = find_mapseen(lev)))
        return mptr->custom;
    return NULL;
}

/* #annotate command - add a custom name to the current level */
int
donamelevel()
{
    mapseen *mptr;
    char nbuf[BUFSZ]; /* Buffer for response */

    if (!(mptr = find_mapseen(&u.uz)))
        return 0;

    nbuf[0] = '\0';
#ifdef EDIT_GETLIN
    if (mptr->custom) {
        (void) strncpy(nbuf, mptr->custom, BUFSZ);
        nbuf[BUFSZ - 1] = '\0';
    }
#else
    if (mptr->custom) {
        char tmpbuf[BUFSZ];

        Sprintf(tmpbuf, "Replace annotation \"%.30s%s\" with?", mptr->custom,
                (strlen(mptr->custom) > 30) ? "..." : "");
        getlin(tmpbuf, nbuf);
    } else
#endif
        getlin("What do you want to call this dungeon level?", nbuf);

    /* empty input or ESC means don't add or change annotation;
       space-only means discard current annotation without adding new one */
    if (!*nbuf || *nbuf == '\033')
        return 0;
    /* strip leading and trailing spaces, compress out consecutive spaces */
    (void) mungspaces(nbuf);

    /* discard old annotation, if any */
    if (mptr->custom) {
        free((genericptr_t) mptr->custom);
        mptr->custom = (char *) 0;
        mptr->custom_lth = 0;
    }
    /* add new annotation, unless it's all spaces (which will be an
       empty string after mungspaces() above) */
    if (*nbuf && strcmp(nbuf, " ")) {
        mptr->custom = dupstr(nbuf);
        mptr->custom_lth = strlen(mptr->custom);
    }
    return 0;
}

/* find the particular mapseen object in the chain; may return null */
static mapseen *
find_mapseen(lev)
d_level *lev;
{
    mapseen *mptr;

    for (mptr = g.mapseenchn; mptr; mptr = mptr->next)
        if (on_level(&(mptr->lev), lev))
            break;

    return mptr;
}

static mapseen *
find_mapseen_by_str(s)
const char *s;
{
    mapseen *mptr;

    for (mptr = g.mapseenchn; mptr; mptr = mptr->next)
        if (mptr->custom && !strcmpi(s, mptr->custom))
            break;

    return mptr;
}


void
forget_mapseen(ledger_num)
int ledger_num;
{
    mapseen *mptr;
    struct cemetery *bp;

    for (mptr = g.mapseenchn; mptr; mptr = mptr->next)
        if (g.dungeons[mptr->lev.dnum].ledger_start + mptr->lev.dlevel
            == ledger_num)
            break;

    /* if not found, then nothing to forget */
    if (mptr) {
        mptr->flags.forgot = 1;
        mptr->br = (branch *) 0;

        /* custom names are erased, not just forgotten until revisited */
        if (mptr->custom) {
            mptr->custom_lth = 0;
            free((genericptr_t) mptr->custom);
            mptr->custom = (char *) 0;
        }
        (void) memset((genericptr_t) mptr->msrooms, 0, sizeof mptr->msrooms);
        for (bp = mptr->final_resting_place; bp; bp = bp->next)
            bp->bonesknown = FALSE;
    }
}

void
rm_mapseen(ledger_num)
int ledger_num;
{
    mapseen *mptr, *mprev = (mapseen *)0;
    struct cemetery *bp, *bpnext;

    for (mptr = g.mapseenchn; mptr; mprev = mptr, mptr = mptr->next)
        if (g.dungeons[mptr->lev.dnum].ledger_start + mptr->lev.dlevel
            == ledger_num)
            break;

    if (!mptr)
        return;

    if (mptr->custom)
        free((genericptr_t) mptr->custom);

    bp = mptr->final_resting_place;
    while (bp) {
        bpnext = bp->next;
        free(bp);
        bp = bpnext;
    }

    if (mprev) {
        mprev->next = mptr->next;
        free(mptr);
    } else {
        g.mapseenchn = mptr->next;
        free(mptr);
    }
}

static void
save_mapseen(nhfp, mptr)
NHFILE *nhfp;
mapseen *mptr;
{
    branch *curr;
    int brindx;

    for (brindx = 0, curr = g.branches; curr; curr = curr->next, ++brindx)
        if (curr == mptr->br)
            break;
    if (nhfp->structlevel)
        bwrite(nhfp->fd, (genericptr_t) &brindx, sizeof brindx);

    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &mptr->lev, sizeof mptr->lev);
        bwrite(nhfp->fd, (genericptr_t) &mptr->feat, sizeof mptr->feat);
        bwrite(nhfp->fd, (genericptr_t) &mptr->flags, sizeof mptr->flags);
        bwrite(nhfp->fd, (genericptr_t) &mptr->custom_lth, sizeof mptr->custom_lth);
    }

    if (mptr->custom_lth) {
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) mptr->custom, mptr->custom_lth);
        }
    }
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &mptr->msrooms, sizeof mptr->msrooms);
    }
    savecemetery(nhfp, &mptr->final_resting_place);
}

static mapseen *
load_mapseen(nhfp)
NHFILE *nhfp;
{
    int branchnum = 0, brindx;
    mapseen *load;
    branch *curr;

    load = (mapseen *) alloc(sizeof *load);

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &branchnum, sizeof branchnum);
    for (brindx = 0, curr = g.branches; curr; curr = curr->next, ++brindx)
        if (brindx == branchnum)
            break;
    load->br = curr;

    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &load->lev, sizeof load->lev);
        mread(nhfp->fd, (genericptr_t) &load->feat, sizeof load->feat);
        mread(nhfp->fd, (genericptr_t) &load->flags, sizeof load->flags);
        mread(nhfp->fd, (genericptr_t) &load->custom_lth, sizeof load->custom_lth);
    }

    if (load->custom_lth) {
        /* length doesn't include terminator (which isn't saved & restored) */
        load->custom = (char *) alloc(load->custom_lth + 1);
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) load->custom, load->custom_lth);
        }
        load->custom[load->custom_lth] = '\0';
    } else
        load->custom = 0;
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &load->msrooms, sizeof load->msrooms);
    }
    restcemetery(nhfp, &load->final_resting_place);
    return load;
}

/* to support '#stats' wizard-mode command */
void
overview_stats(win, statsfmt, total_count, total_size)
winid win;
const char *statsfmt;
long *total_count, *total_size;
{
    char buf[BUFSZ], hdrbuf[QBUFSZ];
    long ocount, osize, bcount, bsize, acount, asize;
    struct cemetery *ce;
    mapseen *mptr = find_mapseen(&u.uz);

    ocount = bcount = acount = osize = bsize = asize = 0L;
    for (mptr = g.mapseenchn; mptr; mptr = mptr->next) {
        ++ocount;
        osize += (long) sizeof *mptr;
        for (ce = mptr->final_resting_place; ce; ce = ce->next) {
            ++bcount;
            bsize += (long) sizeof *ce;
        }
        if (mptr->custom_lth) {
            ++acount;
            asize += (long) (mptr->custom_lth + 1);
        }
    }

    Sprintf(hdrbuf, "general, size %ld", (long) sizeof (mapseen));
    Sprintf(buf, statsfmt, hdrbuf, ocount, osize);
    putstr(win, 0, buf);
    if (bcount) {
        Sprintf(hdrbuf, "cemetery, size %ld",
                (long) sizeof (struct cemetery));
        Sprintf(buf, statsfmt, hdrbuf, bcount, bsize);
        putstr(win, 0, buf);
    }
    if (acount) {
        Sprintf(hdrbuf, "annotations, text");
        Sprintf(buf, statsfmt, hdrbuf, acount, asize);
        putstr(win, 0, buf);
    }
    *total_count += ocount + bcount + acount;
    *total_size += osize + bsize + asize;
}

/* Remove all mapseen objects for a particular dnum.
 * Useful during quest expulsion to remove quest levels.
 * [No longer deleted, just marked as unreachable.  #overview will
 * ignore such levels, end of game disclosure will include them.]
 */
void
remdun_mapseen(dnum)
int dnum;
{
    mapseen *mptr, **mptraddr;

    mptraddr = &g.mapseenchn;
    while ((mptr = *mptraddr) != 0) {
        if (mptr->lev.dnum == dnum) {
#if 1 /* use this... */
            mptr->flags.unreachable = 1;
        }
#else /* old deletion code */
            *mptraddr = mptr->next;
            if (mptr->custom)
                free((genericptr_t) mptr->custom);
            if (mptr->final_resting_place)
                savecemetery(-1, FREE_SAVE, &mptr->final_resting_place);
            free((genericptr_t) mptr);
        } else
#endif
        mptraddr = &mptr->next;
    }
}

void
init_mapseen(lev)
d_level *lev;
{
    /* Create a level and insert in "sorted" order.  This is an insertion
     * sort first by dungeon (in order of discovery) and then by level number.
     */
    mapseen *mptr, *init, *prev;

    init = (mapseen *) alloc(sizeof *init);
    (void) memset((genericptr_t) init, 0, sizeof *init);
    /* memset is fine for feature bits, flags, and rooms array;
       explicitly initialize pointers to null */
    init->next = 0, init->br = 0, init->custom = 0;
    init->final_resting_place = 0;
    /* g.lastseentyp[][] is reused for each level, so get rid of
       previous level's data */
    (void) memset((genericptr_t) g.lastseentyp, 0, sizeof g.lastseentyp);

    init->lev.dnum = lev->dnum;
    init->lev.dlevel = lev->dlevel;

    /* walk until we get to the place where we should insert init */
    for (mptr = g.mapseenchn, prev = 0; mptr; prev = mptr, mptr = mptr->next)
        if (mptr->lev.dnum > init->lev.dnum
            || (mptr->lev.dnum == init->lev.dnum
                && mptr->lev.dlevel > init->lev.dlevel))
            break;
    if (!prev) {
        init->next = g.mapseenchn;
        g.mapseenchn = init;
    } else {
        mptr = prev->next;
        prev->next = init;
        init->next = mptr;
    }
}

#define INTEREST(feat)                                                \
    ((feat).nfount || (feat).nsink || (feat).nthrone || (feat).naltar \
     || (feat).ngrave || (feat).ntree || (feat).nshop || (feat).ntemple)
  /* || (feat).water || (feat).ice || (feat).lava */

/* returns true if this level has something interesting to print out */
static boolean
interest_mapseen(mptr)
mapseen *mptr;
{
    if (on_level(&u.uz, &mptr->lev))
        return TRUE;
    if (mptr->flags.unreachable || mptr->flags.forgot)
        return FALSE;
    /* level is of interest if it has an auto-generated annotation */
    if (mptr->flags.oracle || mptr->flags.bigroom || mptr->flags.roguelevel
        || mptr->flags.castle || mptr->flags.valley
        || mptr->flags.msanctum || mptr->flags.vibrating_square
        || mptr->flags.quest_summons || mptr->flags.questing)
        return TRUE;
    /* when in Sokoban, list all sokoban levels visited; when not in it,
       list any visited Sokoban level which remains unsolved (will usually
       only be furthest one reached, but it's possible to enter pits and
       climb out on the far side on the first Sokoban level; also, wizard
       mode overrides teleport restrictions) */
    if (In_sokoban(&mptr->lev)
        && (In_sokoban(&u.uz) || !mptr->flags.sokosolved))
        return TRUE;
    /* when in the endgame, list all endgame levels visited, whether they
       have annotations or not, so that #overview doesn't become extremely
       sparse once the rest of the dungeon has been flagged as unreachable */
    if (In_endgame(&u.uz))
        return (boolean) In_endgame(&mptr->lev);
    /* level is of interest if it has non-zero feature count or known bones
       or user annotation or known connection to another dungeon branch
       or is the furthest level reached in its branch */
    return (boolean) (INTEREST(mptr->feat)
                      || (mptr->final_resting_place
                          && (mptr->flags.knownbones || wizard))
                      || mptr->custom || mptr->br
                      || (mptr->lev.dlevel
                          == g.dungeons[mptr->lev.dnum].dunlev_ureached));
}

/* recalculate mapseen for the current level */
void
recalc_mapseen()
{
    mapseen *mptr, *oth_mptr;
    struct monst *mtmp;
    struct cemetery *bp, **bonesaddr;
    struct trap *t;
    unsigned i, ridx;
    int x, y, ltyp, count, atmp;

    /* Should not happen in general, but possible if in the process
     * of being booted from the quest.  The mapseen object gets
     * removed during the expulsion but prior to leaving the level
     * [Since quest expulsion no longer deletes quest mapseen data,
     * null return from find_mapseen() should now be impossible.]
     */
    if (!(mptr = find_mapseen(&u.uz)))
        return;

    /* reset all features; mptr->feat.* = 0; */
    (void) memset((genericptr_t) &mptr->feat, 0, sizeof mptr->feat);
    /* reset most flags; some level-specific ones are left as-is */
    if (mptr->flags.unreachable) {
        mptr->flags.unreachable = 0; /* reached it; Eye of the Aethiopica? */
        if (In_quest(&u.uz)) {
            mapseen *mptrtmp = g.mapseenchn;

            /* when quest was unreachable due to ejection and portal removal,
               getting back to it via arti-invoke should revive annotation
               data for all quest levels, not just the one we're on now */
            do {
                if (mptrtmp->lev.dnum == mptr->lev.dnum)
                    mptrtmp->flags.unreachable = 0;
                mptrtmp = mptrtmp->next;
            } while (mptrtmp);
        }
    }
    mptr->flags.knownbones = 0;
    mptr->flags.sokosolved = In_sokoban(&u.uz) && !Sokoban;
    /* mptr->flags.bigroom retains previous value when hero can't see */
    if (!Blind)
        mptr->flags.bigroom = Is_bigroom(&u.uz);
    else if (mptr->flags.forgot)
        mptr->flags.bigroom = 0;
    mptr->flags.roguelevel = Is_rogue_level(&u.uz);
    mptr->flags.oracle = 0; /* recalculated during room traversal below */
    mptr->flags.castletune = 0;
    /* flags.castle retains previous value */
    mptr->flags.forgot = 0;
    /* flags.quest_summons disabled once quest finished */
    mptr->flags.quest_summons = (at_dgn_entrance("The Quest")
                                 && u.uevent.qcalled
                                 && !(u.uevent.qcompleted
                                      || u.uevent.qexpelled
                                      || g.quest_status.leader_is_dead));
    mptr->flags.questing = (on_level(&u.uz, &qstart_level)
                            && g.quest_status.got_quest);
    /* flags.msanctum, .valley, and .vibrating_square handled below */

    /* track rooms the hero is in */
    for (i = 0; i < SIZE(u.urooms); ++i) {
        if (!u.urooms[i])
            continue;

        ridx = u.urooms[i] - ROOMOFFSET;
        mptr->msrooms[ridx].seen = 1;
        mptr->msrooms[ridx].untended =
            (g.rooms[ridx].rtype >= SHOPBASE)
                ? (!(mtmp = shop_keeper(u.urooms[i])) || !inhishop(mtmp))
                : (g.rooms[ridx].rtype == TEMPLE)
                      ? (!(mtmp = findpriest(u.urooms[i]))
                         || !inhistemple(mtmp))
                      : 0;
    }

    /* recalculate room knowledge: for now, just shops and temples
     * this could be extended to an array of 0..SHOPBASE
     */
    for (i = 0; i < SIZE(mptr->msrooms); ++i) {
        if (mptr->msrooms[i].seen) {
            if (g.rooms[i].rtype >= SHOPBASE) {
                if (mptr->msrooms[i].untended)
                    mptr->feat.shoptype = SHOPBASE - 1;
                else if (!mptr->feat.nshop)
                    mptr->feat.shoptype = g.rooms[i].rtype;
                else if (mptr->feat.shoptype != (unsigned) g.rooms[i].rtype)
                    mptr->feat.shoptype = 0;
                count = mptr->feat.nshop + 1;
                if (count <= 3)
                    mptr->feat.nshop = count;
            } else if (g.rooms[i].rtype == TEMPLE) {
                /* altar and temple alignment handled below */
                count = mptr->feat.ntemple + 1;
                if (count <= 3)
                    mptr->feat.ntemple = count;
            } else if (g.rooms[i].orig_rtype == DELPHI) {
                mptr->flags.oracle = 1;
            }
        }
    }

    /* Update g.lastseentyp with typ if and only if it is in sight or the
     * hero can feel it on their current location (i.e. not levitating).
     * This *should* give the "last known typ" for each dungeon location.
     * (At the very least, it's a better assumption than determining what
     * the player knows from the glyph and the typ (which is isn't quite
     * enough information in some cases)).
     *
     * It was reluctantly added to struct rm to track.  Alternatively
     * we could track "features" and then update them all here, and keep
     * track of when new features are created or destroyed, but this
     * seemed the most elegant, despite adding more data to struct rm.
     * [3.6.0: we're using g.lastseentyp[][] rather than level.locations
     * to track the features seen.]
     *
     * Although no current windowing systems (can) do this, this would add
     * the ability to have non-dungeon glyphs float above the last known
     * dungeon glyph (i.e. items on fountains).
     */
    for (x = 1; x < COLNO; x++) {
        for (y = 0; y < ROWNO; y++) {
            if (cansee(x, y) || (x == u.ux && y == u.uy && !Levitation)) {
                ltyp = levl[x][y].typ;
                if (ltyp == DRAWBRIDGE_UP)
                    ltyp = db_under_typ(levl[x][y].drawbridgemask);
                if ((mtmp = m_at(x, y)) != 0
                    && M_AP_TYPE(mtmp) == M_AP_FURNITURE && canseemon(mtmp))
                    ltyp = cmap_to_type(mtmp->mappearance);
                g.lastseentyp[x][y] = ltyp;
            }

            switch (g.lastseentyp[x][y]) {
#if 0
            case ICE:
                count = mptr->feat.ice + 1;
                if (count <= 3)
                    mptr->feat.ice = count;
                break;
            case POOL:
            case MOAT:
            case WATER:
                count = mptr->feat.water + 1;
                if (count <= 3)
                    mptr->feat.water = count;
                break;
            case LAVAPOOL:
                count = mptr->feat.lava + 1;
                if (count <= 3)
                    mptr->feat.lava = count;
                break;
#endif
            case TREE:
                count = mptr->feat.ntree + 1;
                if (count <= 3)
                    mptr->feat.ntree = count;
                break;
            case FOUNTAIN:
                count = mptr->feat.nfount + 1;
                if (count <= 3)
                    mptr->feat.nfount = count;
                break;
            case THRONE:
                count = mptr->feat.nthrone + 1;
                if (count <= 3)
                    mptr->feat.nthrone = count;
                break;
            case SINK:
                count = mptr->feat.nsink + 1;
                if (count <= 3)
                    mptr->feat.nsink = count;
                break;
            case GRAVE:
                count = mptr->feat.ngrave + 1;
                if (count <= 3)
                    mptr->feat.ngrave = count;
                break;
            case ALTAR:
                atmp = (Is_astralevel(&u.uz)
                        && (levl[x][y].seenv & SVALL) != SVALL)
                         ? MSA_NONE
                         : Amask2msa(levl[x][y].altarmask);
                if (!mptr->feat.naltar)
                    mptr->feat.msalign = atmp;
                else if (mptr->feat.msalign != atmp)
                    mptr->feat.msalign = MSA_NONE;
                count = mptr->feat.naltar + 1;
                if (count <= 3)
                    mptr->feat.naltar = count;
                break;
            /*  An automatic annotation is added to the Castle and
             *  to Fort Ludios once their structure's main entrance
             *  has been seen (in person or via magic mapping).
             *  For the Fort, that entrance is just a secret door
             *  which will be converted into a regular one when
             *  located (or destroyed).
             * DOOR: possibly a lowered drawbridge's open portcullis;
             * DBWALL: a raised drawbridge's "closed door";
             * DRAWBRIDGE_DOWN: the span provided by lowered bridge,
             *  with moat or other terrain hidden underneath;
             * DRAWBRIDGE_UP: moat in front of a raised drawbridge,
             *  not recognizable as a bridge location unless/until
             *  the adjacent DBWALL has been seen.
             */
            case DOOR:
                if (Is_knox(&u.uz)) {
                    int ty, tx = x - 4;

                    /* Throne is four columns left, either directly in
                     * line or one row higher or lower, and doesn't have
                     * to have been seen yet.
                     *   ......|}}}.
                     *   ..\...S}...
                     *   ..\...S}...
                     *   ......|}}}.
                     * For 3.6.0 and earlier, it was always in direct line:
                     * both throne and door on the lower of the two rows.
                     */
                    for (ty = y - 1; ty <= y + 1; ++ty)
                        if (isok(tx, ty) && IS_THRONE(levl[tx][ty].typ)) {
                            mptr->flags.ludios = 1;
                            break;
                        }
                    break;
                }
                if (is_drawbridge_wall(x, y) < 0)
                    break;
                /*FALLTHRU*/
            case DBWALL:
            case DRAWBRIDGE_DOWN:
                if (Is_stronghold(&u.uz))
                    mptr->flags.castle = 1, mptr->flags.castletune = 1;
                break;
            default:
                break;
            }
        }
    }

    /* Moloch's Sanctum and the Valley of the Dead are normally given an
       automatic annotation when you enter a temple attended by a priest,
       but it is possible for the priest to be killed prior to that; we
       assume that both of those levels only contain one altar, so add the
       annotation if that altar has been mapped (seen or magic mapping) */
    if (Is_valley(&u.uz)) {
        /* don't clear valley if naltar==0; maybe altar got destroyed? */
        if (mptr->feat.naltar > 0)
            mptr->flags.valley = 1;

    /* Sanctum and Gateway-to-Sanctum are mutually exclusive automatic
       annotations but handling that is tricky because they're stored
       with data for different levels */
    } else if (Is_sanctum(&u.uz)) {
        if (mptr->feat.naltar > 0)
            mptr->flags.msanctum = 1;

        if (mptr->flags.msanctum) {
            d_level invocat_lvl;

            invocat_lvl = u.uz;
            invocat_lvl.dlevel -= 1;
            if ((oth_mptr = find_mapseen(&invocat_lvl)) != 0)
                oth_mptr->flags.vibrating_square = 0;
        }
    } else if (Invocation_lev(&u.uz)) {
        /* annotate vibrating square's level if vibr_sqr 'trap' has been
           found or if that trap is gone (indicating that invocation has
           happened) provided that the sanctum's annotation hasn't been
           added (either hero hasn't descended to that level yet or hasn't
           mapped its temple) */
        for (t = g.ftrap; t; t = t->ntrap)
            if (t->ttyp == VIBRATING_SQUARE)
                break;
        mptr->flags.vibrating_square = t ? t->tseen
                      /* no trap implies that invocation has been performed */
                             : ((oth_mptr = find_mapseen(&sanctum_level)) == 0
                                || !oth_mptr->flags.msanctum);
    }

    if (g.level.bonesinfo && !mptr->final_resting_place) {
        /* clone the bonesinfo so we aren't dependent upon this
           level being in memory */
        bonesaddr = &mptr->final_resting_place;
        bp = g.level.bonesinfo;
        do {
            *bonesaddr = (struct cemetery *) alloc(sizeof **bonesaddr);
            **bonesaddr = *bp;
            bp = bp->next;
            bonesaddr = &(*bonesaddr)->next;
        } while (bp);
        *bonesaddr = 0;
    }
    /* decide which past hero deaths have become known; there's no
       guarantee of either a grave or a ghost, so we go by whether the
       current hero has seen the map location where each old one died */
    for (bp = mptr->final_resting_place; bp; bp = bp->next)
        if (g.lastseentyp[bp->frpx][bp->frpy]) {
            bp->bonesknown = TRUE;
            mptr->flags.knownbones = 1;
        }
}

/*ARGUSED*/
/* valley and sanctum levels get automatic annotation once temple is entered */
void
mapseen_temple(priest)
struct monst *priest UNUSED; /* currently unused; might be useful someday */
{
    mapseen *mptr = find_mapseen(&u.uz);

    if (Is_valley(&u.uz))
        mptr->flags.valley = 1;
    else if (Is_sanctum(&u.uz))
        mptr->flags.msanctum = 1;
}

/* room entry message has just been delivered so learn room even if blind */
void
room_discovered(roomno)
int roomno;
{
    mapseen *mptr = find_mapseen(&u.uz);

    mptr->msrooms[roomno].seen = 1;
}

/* #overview command */
int
dooverview()
{
    show_overview(0, 0);
    return 0;
}

/* called for #overview or for end of game disclosure */
void
show_overview(why, reason)
int why;    /* 0 => #overview command,
               1 or 2 => final disclosure (1: hero lived, 2: hero died) */
int reason; /* how hero died; used when disclosing end-of-game level */
{
    winid win;
    int lastdun = -1;

    /* lazy initialization */
    (void) recalc_mapseen();

    win = create_nhwindow(NHW_MENU);
    /* show the endgame levels before the rest of the dungeon,
       so that the Planes (dnum 5-ish) come out above main dungeon (dnum 0) */
    if (In_endgame(&u.uz))
        traverse_mapseenchn(TRUE, win, why, reason, &lastdun);
    /* if game is over or we're not in the endgame yet, show the dungeon */
    if (why > 0 || !In_endgame(&u.uz))
        traverse_mapseenchn(FALSE, win, why, reason, &lastdun);
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
}

/* display endgame levels or non-endgame levels, not both */
static void
traverse_mapseenchn(viewendgame, win, why, reason, lastdun_p)
boolean viewendgame;
winid win;
int why, reason, *lastdun_p;
{
    mapseen *mptr;
    boolean showheader;

    for (mptr = g.mapseenchn; mptr; mptr = mptr->next) {
        if (viewendgame ^ In_endgame(&mptr->lev))
            continue;

        /* only print out info for a level or a dungeon if interest */
        if (why > 0 || interest_mapseen(mptr)) {
            showheader = (boolean) (mptr->lev.dnum != *lastdun_p);
            print_mapseen(win, mptr, why, reason, showheader);
            *lastdun_p = mptr->lev.dnum;
        }
    }
}

static const char *
seen_string(x, obj)
xchar x;
const char *obj;
{
    /* players are computer scientists: 0, 1, 2, n */
    switch (x) {
    case 0:
        return "no";
    /* an() returns too much.  index is ok in this case */
    case 1:
        return index(vowels, *obj) ? "an" : "a";
    case 2:
        return "some";
    case 3:
        return "many";
    }

    return "(unknown)";
}

/* better br_string */
static const char *
br_string2(br)
branch *br;
{
    /* Special case: quest portal says closed if kicked from quest */
    boolean closed_portal = (br->end2.dnum == quest_dnum
                             && u.uevent.qexpelled);

    switch (br->type) {
    case BR_PORTAL:
        return closed_portal ? "Sealed portal" : "Portal";
    case BR_NO_END1:
        return "Connection";
    case BR_NO_END2:
        return br->end1_up ? "One way stairs up" : "One way stairs down";
    case BR_STAIR:
        return br->end1_up ? "Stairs up" : "Stairs down";
    }

    return "(unknown)";
}

/* get the name of an endgame level; topten.c does something similar */
const char *
endgamelevelname(outbuf, indx)
char *outbuf;
int indx;
{
    const char *planename = 0;

    *outbuf = '\0';
    switch (indx) {
    case -5:
        Strcpy(outbuf, "Astral Plane");
        break;
    case -4:
        planename = "Water";
        break;
    case -3:
        planename = "Fire";
        break;
    case -2:
        planename = "Air";
        break;
    case -1:
        planename = "Earth";
        break;
    }
    if (planename)
        Sprintf(outbuf, "Plane of %s", planename);
    else if (!*outbuf)
        Sprintf(outbuf, "unknown plane #%d", indx);
    return outbuf;
}

static const char *
shop_string(rtype)
int rtype;
{
    const char *str = "shop"; /* catchall */

    /* Yuck, redundancy...but shclass.name doesn't cut it as a noun */
    switch (rtype) {
    case SHOPBASE - 1:
        str = "untended shop";
        break; /* see recalc_mapseen */
    case SHOPBASE:
        str = "general store";
        break;
    case ARMORSHOP:
        str = "armor shop";
        break;
    case SCROLLSHOP:
        str = "scroll shop";
        break;
    case POTIONSHOP:
        str = "potion shop";
        break;
    case WEAPONSHOP:
        str = "weapon shop";
        break;
    case FOODSHOP:
        str = "delicatessen";
        break;
    case RINGSHOP:
        str = "jewelers";
        break;
    case WANDSHOP:
        str = "wand shop";
        break;
    case BOOKSHOP:
        str = "bookstore";
        break;
    case FODDERSHOP:
        str = "health food store";
        break;
    case CANDLESHOP:
        str = "lighting shop";
        break;
    default:
        break;
    }
    return str;
}

/* if player knows about the mastermind tune, append it to Castle annotation;
   if drawbridge has been destroyed, flags.castletune will be zero */
static char *
tunesuffix(mptr, outbuf)
mapseen *mptr;
char *outbuf;
{
    *outbuf = '\0';
    if (mptr->flags.castletune && u.uevent.uheard_tune) {
        char tmp[BUFSZ];

        if (u.uevent.uheard_tune == 2)
            Sprintf(tmp, "notes \"%s\"", g.tune);
        else
            Strcpy(tmp, "5-note tune");
        Sprintf(outbuf, " (play %s to open or close drawbridge)", tmp);
    }
    return outbuf;
}

/* some utility macros for print_mapseen */
#define TAB "   " /* three spaces */
#if 0
#define BULLET "" /* empty; otherwise output becomes cluttered */
#define PREFIX TAB TAB BULLET
#else                   /*!0*/
/* K&R: don't require support for concatenation of adjacent string literals */
#define PREFIX "      " /* two TABs + empty BULLET: six spaces */
#endif
#define COMMA (i++ > 0 ? ", " : PREFIX)
/* "iterate" once; safe to use as ``if (cond) ADDTOBUF(); else whatever;'' */
#define ADDNTOBUF(nam, var)                                                  \
    do {                                                                     \
        if (var)                                                             \
            Sprintf(eos(buf), "%s%s %s%s", COMMA, seen_string((var), (nam)), \
                    (nam), plur(var));                                       \
    } while (0)
#define ADDTOBUF(nam, var)                           \
    do {                                             \
        if (var)                                     \
            Sprintf(eos(buf), "%s%s", COMMA, (nam)); \
    } while (0)

static void
print_mapseen(win, mptr, final, how, printdun)
winid win;
mapseen *mptr;
int final; /* 0: not final; 1: game over, alive; 2: game over, dead */
int how;   /* cause of death; only used if final==2 and mptr->lev==u.uz */
boolean printdun;
{
    char buf[BUFSZ], tmpbuf[BUFSZ];
    int i, depthstart, dnum;
    boolean died_here = (final == 2 && on_level(&u.uz, &mptr->lev));

    /* Damnable special cases */
    /* The quest and knox should appear to be level 1 to match
     * other text.
     */
    dnum = mptr->lev.dnum;
    if (dnum == quest_dnum || dnum == knox_level.dnum)
        depthstart = 1;
    else
        depthstart = g.dungeons[dnum].depth_start;

    if (printdun) {
        if (g.dungeons[dnum].dunlev_ureached == g.dungeons[dnum].entry_lev
            /* suppress the negative numbers in the endgame */
            || In_endgame(&mptr->lev))
            Sprintf(buf, "%s:", g.dungeons[dnum].dname);
        else if (builds_up(&mptr->lev))
            Sprintf(buf, "%s: levels %d up to %d",
                    g.dungeons[dnum].dname,
                    depthstart + g.dungeons[dnum].entry_lev - 1,
                    depthstart + g.dungeons[dnum].dunlev_ureached - 1);
        else
            Sprintf(buf, "%s: levels %d to %d",
                    g.dungeons[dnum].dname, depthstart,
                    depthstart + g.dungeons[dnum].dunlev_ureached - 1);
        putstr(win, !final ? ATR_INVERSE : 0, buf);
    }

    /* calculate level number */
    i = depthstart + mptr->lev.dlevel - 1;
    if (In_endgame(&mptr->lev))
        Sprintf(buf, "%s%s:", TAB, endgamelevelname(tmpbuf, i));
    else
        Sprintf(buf, "%sLevel %d:", TAB, i);

    /* wizmode prints out proto dungeon names for clarity */
    if (wizard) {
        s_level *slev;

        if ((slev = Is_special(&mptr->lev)) != 0)
            Sprintf(eos(buf), " [%s]", slev->proto);
    }
    /* [perhaps print custom annotation on its own line when it's long] */
    if (mptr->custom)
        Sprintf(eos(buf), " \"%s\"", mptr->custom);
    if (on_level(&u.uz, &mptr->lev))
        Sprintf(eos(buf), " <- You %s here.",
                (!final || (final == 1 && how == ASCENDED)) ? "are"
                  : (final == 1 && how == ESCAPED) ? "left from"
                    : "were");
    putstr(win, !final ? ATR_BOLD : 0, buf);

    if (mptr->flags.forgot)
        return;

    if (INTEREST(mptr->feat)) {
        buf[0] = 0;

        i = 0; /* interest counter */
        /* List interests in an order vaguely corresponding to
         * how important they are.
         */
        if (mptr->feat.nshop > 0) {
            if (mptr->feat.nshop > 1)
                ADDNTOBUF("shop", mptr->feat.nshop);
            else
                Sprintf(eos(buf), "%s%s", COMMA,
                        an(shop_string(mptr->feat.shoptype)));
        }
        if (mptr->feat.naltar > 0) {
            /* Temples + non-temple altars get munged into just "altars" */
            if (mptr->feat.ntemple != mptr->feat.naltar)
                ADDNTOBUF("altar", mptr->feat.naltar);
            else
                ADDNTOBUF("temple", mptr->feat.ntemple);

            /* only print out altar's god if they are all to your god */
            if (Amask2align(Msa2amask(mptr->feat.msalign)) == u.ualign.type)
                Sprintf(eos(buf), " to %s", align_gname(u.ualign.type));
        }
        ADDNTOBUF("throne", mptr->feat.nthrone);
        ADDNTOBUF("fountain", mptr->feat.nfount);
        ADDNTOBUF("sink", mptr->feat.nsink);
        ADDNTOBUF("grave", mptr->feat.ngrave);
        ADDNTOBUF("tree", mptr->feat.ntree);
#if 0
        ADDTOBUF("water", mptr->feat.water);
        ADDTOBUF("lava", mptr->feat.lava);
        ADDTOBUF("ice", mptr->feat.ice);
#endif
        /* capitalize afterwards */
        i = strlen(PREFIX);
        buf[i] = highc(buf[i]);
        /* capitalizing it makes it a sentence; terminate with '.' */
        Strcat(buf, ".");
        putstr(win, 0, buf);
    }

    /* we assume that these are mutually exclusive */
    *buf = '\0';
    if (mptr->flags.oracle) {
        Sprintf(buf, "%sOracle of Delphi.", PREFIX);
    } else if (In_sokoban(&mptr->lev)) {
        Sprintf(buf, "%s%s.", PREFIX,
                mptr->flags.sokosolved ? "Solved" : "Unsolved");
    } else if (mptr->flags.bigroom) {
        Sprintf(buf, "%sA very big room.", PREFIX);
    } else if (mptr->flags.roguelevel) {
        Sprintf(buf, "%sA primitive area.", PREFIX);
    } else if (on_level(&mptr->lev, &qstart_level)) {
        Sprintf(buf, "%sHome%s.", PREFIX,
                mptr->flags.unreachable ? " (no way back...)" : "");
        if (u.uevent.qcompleted)
            Sprintf(buf, "%sCompleted quest for %s.", PREFIX, ldrname());
        else if (mptr->flags.questing)
            Sprintf(buf, "%sGiven quest by %s.", PREFIX, ldrname());
    } else if (mptr->flags.ludios) {
        /* presence of the ludios branch in #overview output indicates that
           the player has made it onto the level; presence of this annotation
           indicates that the fort's entrance has been seen (or mapped) */
        Sprintf(buf, "%sFort Ludios.", PREFIX);
    } else if (mptr->flags.castle) {
        Sprintf(buf, "%sThe castle%s.", PREFIX, tunesuffix(mptr, tmpbuf));
    } else if (mptr->flags.valley) {
        Sprintf(buf, "%sValley of the Dead.", PREFIX);
    } else if (mptr->flags.vibrating_square) {
        Sprintf(buf, "%sGateway to Moloch's Sanctum.", PREFIX);
    } else if (mptr->flags.msanctum) {
        Sprintf(buf, "%sMoloch's Sanctum.", PREFIX);
    }
    if (*buf)
        putstr(win, 0, buf);
    /* quest entrance is not mutually-exclusive with bigroom or rogue level */
    if (mptr->flags.quest_summons) {
        Sprintf(buf, "%sSummoned by %s.", PREFIX, ldrname());
        putstr(win, 0, buf);
    }

    /* print out branches */
    if (mptr->br) {
        Sprintf(buf, "%s%s to %s", PREFIX, br_string2(mptr->br),
                g.dungeons[mptr->br->end2.dnum].dname);

        /* Since mapseen objects are printed out in increasing order
         * of dlevel, clarify which level this branch is going to
         * if the branch goes upwards.  Unless it's the end game.
         */
        if (mptr->br->end1_up && !In_endgame(&(mptr->br->end2)))
            Sprintf(eos(buf), ", level %d", depth(&(mptr->br->end2)));
        Strcat(buf, ".");
        putstr(win, 0, buf);
    }

    /* maybe print out bones details */
    if (mptr->final_resting_place || final) {
        struct cemetery *bp;
        int kncnt = !died_here ? 0 : 1;

        for (bp = mptr->final_resting_place; bp; bp = bp->next)
            if (bp->bonesknown || wizard || final)
                ++kncnt;
        if (kncnt) {
            Sprintf(buf, "%s%s", PREFIX, "Final resting place for");
            putstr(win, 0, buf);
            if (died_here) {
                /* disclosure occurs before bones creation, so listing dead
                   hero here doesn't give away whether bones are produced */
                formatkiller(tmpbuf, sizeof tmpbuf, how, TRUE);
                /* rephrase a few death reasons to work with "you" */
                (void) strsubst(tmpbuf, " himself", " yourself");
                (void) strsubst(tmpbuf, " herself", " yourself");
                (void) strsubst(tmpbuf, " his ", " your ");
                (void) strsubst(tmpbuf, " her ", " your ");
                Sprintf(buf, "%s%syou, %s%c", PREFIX, TAB, tmpbuf,
                        --kncnt ? ',' : '.');
                putstr(win, 0, buf);
            }
            for (bp = mptr->final_resting_place; bp; bp = bp->next) {
                if (bp->bonesknown || wizard || final) {
                    Sprintf(buf, "%s%s%s, %s%c", PREFIX, TAB, bp->who,
                            bp->how, --kncnt ? ',' : '.');
                    putstr(win, 0, buf);
                }
            }
        }
    }
}

/*dungeon.c*/

/* NetHack 3.7	sp_lev.c	$NHDT-Date: 1646428015 2022/03/04 21:06:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.259 $ */
/*      Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the various functions that are related to the special
 * levels.
 *
 * It contains also the special level loader.
 */

#define IN_SP_LEV_C

#include "hack.h"
#include "sp_lev.h"

typedef void (*select_iter_func)(int, int, genericptr);

extern void mkmap(lev_init *);

static boolean match_maptyps(xchar, xchar);
static void solidify_map(void);
static void lvlfill_maze_grid(int, int, int, int, schar);
static void lvlfill_solid(schar, schar);
static void lvlfill_swamp(schar, schar, schar);
static void flip_drawbridge_horizontal(struct rm *);
static void flip_drawbridge_vertical(struct rm *);
static void flip_visuals(int, int, int, int, int);
static int flip_encoded_direction_bits(int, int);
static void sel_set_wall_property(int, int, genericptr_t);
static void set_wall_property(xchar, xchar, xchar, xchar, int);
static void count_features(void);
static void remove_boundary_syms(void);
static void set_door_orientation(int, int);
static boolean shared_with_room(int, int, struct mkroom *);
static void maybe_add_door(int, int, struct mkroom *);
static void link_doors_rooms(void);
static int rnddoor(void);
static int rndtrap(void);
static void get_location(xchar *, xchar *, getloc_flags_t, struct mkroom *);
static void set_ok_location_func(boolean (*)(xchar, xchar));
static boolean is_ok_location(xchar, xchar, getloc_flags_t);
static unpacked_coord get_unpacked_coord(long, int);
static void get_room_loc(xchar *, xchar *, struct mkroom *);
static void get_free_room_loc(xchar *, xchar *, struct mkroom *,
                              packed_coord);
static boolean create_subroom(struct mkroom *, xchar, xchar, xchar,
                              xchar, xchar, xchar);
static void create_door(room_door *, struct mkroom *);
static void create_trap(spltrap *, struct mkroom *);
static int noncoalignment(aligntyp);
static boolean m_bad_boulder_spot(int, int);
static int pm_to_humidity(struct permonst *);
static unsigned int sp_amask_to_amask(unsigned int sp_amask);
static void create_monster(monster *, struct mkroom *);
static void create_object(object *, struct mkroom *);
static void create_altar(altar *, struct mkroom *);
static boolean search_door(struct mkroom *, xchar *, xchar *, xchar, int);
static void create_corridor(corridor *);
static struct mkroom *build_room(room *, struct mkroom *);
static void light_region(region *);
static void maze1xy(coord *, int);
static void fill_empty_maze(void);
static void splev_initlev(lev_init *);
#if 0
/* macosx complains that these are unused */
static long sp_code_jmpaddr(long, long);
static void spo_room(struct sp_coder *);
static void spo_trap(struct sp_coder *);
static void spo_gold(struct sp_coder *);
static void spo_corridor(struct sp_coder *);
static void spo_feature(struct sp_coder *);
static void spo_terrain(struct sp_coder *);
static void spo_replace_terrain(struct sp_coder *);
static void spo_levregion(struct sp_coder *);
static void spo_region(struct sp_coder *);
static void spo_drawbridge(struct sp_coder *);
static void spo_mazewalk(struct sp_coder *);
static void spo_wall_property(struct sp_coder *);
static void spo_room_door(struct sp_coder *);
static void spo_wallify(struct sp_coder *);
static void sel_set_wallify(int, int, genericptr_t);
#endif
static void spo_end_moninvent(void);
static void spo_pop_container(void);
static int l_create_stairway(lua_State *, boolean);
static void spo_endroom(struct sp_coder *);
static void l_table_getset_feature_flag(lua_State *, int, int, const char *,
                                        int);
static void sel_set_lit(int, int, genericptr_t);
static void add_doors_to_room(struct mkroom *);
static void selection_iterate(struct selectionvar *, select_iter_func,
                              genericptr_t);
static void sel_set_ter(int, int, genericptr_t);
static void sel_set_door(int, int, genericptr_t);
static void sel_set_feature(int, int, genericptr_t);
static void levregion_add(lev_region *);
static void get_table_xy_or_coord(lua_State *, lua_Integer *, lua_Integer *);
static int get_table_region(lua_State *, const char *, lua_Integer *,
                        lua_Integer *, lua_Integer *, lua_Integer *, boolean);
static void set_wallprop_in_selection(lua_State *, int);
static xchar random_wdir(void);
static int floodfillchk_match_under(int, int);
static int floodfillchk_match_accessible(int, int);
static boolean sel_flood_havepoint(int, int, xchar *, xchar *, int);
static long line_dist_coord(long, long, long, long, long, long);
static void l_push_mkroom_table(lua_State *, struct mkroom *);
static int get_table_align(lua_State *);
static int get_table_monclass(lua_State *);
static int find_montype(lua_State *, const char *, int *);
static int get_table_montype(lua_State *, int *);
static lua_Integer get_table_int_or_random(lua_State *, const char *, int);
static int get_table_buc(lua_State *);
static int get_table_objclass(lua_State *);
static int find_objtype(lua_State *, const char *);
static int get_table_objtype(lua_State *);
static const char *get_mkroom_name(int);
static int get_table_roomtype_opt(lua_State *, const char *, int);
static int get_table_traptype_opt(lua_State *, const char *, int);
static int get_traptype_byname(const char *);
static lua_Integer get_table_intarray_entry(lua_State *, int, int);
static struct sp_coder *sp_level_coder_init(void);

/* lua_CFunction prototypes */
int lspo_altar(lua_State *);
int lspo_branch(lua_State *);
int lspo_corridor(lua_State *);
int lspo_door(lua_State *);
int lspo_drawbridge(lua_State *);
int lspo_engraving(lua_State *);
int lspo_feature(lua_State *);
int lspo_gold(lua_State *);
int lspo_grave(lua_State *);
int lspo_ladder(lua_State *);
int lspo_level_flags(lua_State *);
int lspo_level_init(lua_State *);
int lspo_levregion(lua_State *);
int lspo_map(lua_State *);
int lspo_mazewalk(lua_State *);
int lspo_message(lua_State *);
int lspo_mineralize(lua_State *);
int lspo_monster(lua_State *);
int lspo_non_diggable(lua_State *);
int lspo_non_passwall(lua_State *);
int lspo_object(lua_State *);
int lspo_portal(lua_State *);
int lspo_random_corridors(lua_State *);
int lspo_region(lua_State *);
int lspo_replace_terrain(lua_State *);
int lspo_reset_level(lua_State *);
int lspo_finalize_level(lua_State *);
int lspo_room(lua_State *);
int lspo_stair(lua_State *);
int lspo_teleport_region(lua_State *);
int lspo_terrain(lua_State *);
int lspo_trap(lua_State *);
int lspo_wall_property(lua_State *);
int lspo_wallify(lua_State *);

#define LEFT 1
#define H_LEFT 2
#define CENTER 3
#define H_RIGHT 4
#define RIGHT 5

#define TOP 1
#define BOTTOM 5

#define sq(x) ((x) * (x))

#define XLIM 4
#define YLIM 3

#define New(type) (type *) alloc(sizeof (type))
#define NewTab(type, size) (type **) alloc(sizeof (type *) * (unsigned) size)
#define Free(ptr) \
    do {                                        \
        if (ptr)                                \
            free((genericptr_t) (ptr));         \
    } while (0)

    /*
     * No need for 'struct instance_globals g' to contain these.
     * sp_level_coder_init() always re-initializes them prior to use.
     */
static boolean splev_init_present, icedpools;
/* positions touched by level elements explicitly defined in the level */
static char SpLev_Map[COLNO][ROWNO];
#define MAX_CONTAINMENT 10
static int container_idx = 0; /* next slot in container_obj[] to use */
static struct obj *container_obj[MAX_CONTAINMENT];
static struct monst *invent_carrying_monster = (struct monst *) 0;
    /*
     * end of no 'g.'
     */

#define TYP_CANNOT_MATCH(typ) ((typ) == MAX_TYPE || (typ) == INVALID_TYPE)

/* Does typ match with levl[][].typ, considering special types
   MATCH_WALL and MAX_TYPE (aka transparency)? */
static boolean
match_maptyps(xchar typ, xchar levltyp)
{
    if ((typ == MATCH_WALL) && !IS_STWALL(levltyp))
        return FALSE;
    if ((typ < MAX_TYPE) && (typ != levltyp))
        return FALSE;
    return TRUE;
}

struct mapfragment *
mapfrag_fromstr(char *str)
{
    struct mapfragment *mf = (struct mapfragment *) alloc(sizeof *mf);

    char *tmps;

    mf->data = dupstr(str);

    (void) stripdigits(mf->data);
    mf->wid = str_lines_maxlen(mf->data);
    mf->hei = 0;
    tmps = mf->data;
    while (tmps && *tmps) {
        char *s1 = index(tmps, '\n');

        if (mf->hei > MAP_Y_LIM) {
            free(mf->data);
            free(mf);
            return NULL;
        }
        if (s1)
            s1++;
        tmps = s1;
        mf->hei++;
    }
    return mf;
}

void
mapfrag_free(struct mapfragment **mf)
{
    if (mf && *mf) {
        free((*mf)->data);
        free(*mf);
        *mf = NULL;
    }
}

schar
mapfrag_get(struct mapfragment *mf, int x, int y)
{
    if (y < 0 || x < 0 || y > mf->hei - 1 || x > mf->wid - 1)
        panic("outside mapfrag (%i,%i), wanted (%i,%i)",
              mf->wid, mf->hei, x, y);
    return splev_chr2typ(mf->data[y * (mf->wid + 1) + x]);
}

boolean
mapfrag_canmatch(struct mapfragment *mf)
{
    return ((mf->wid % 2) && (mf->hei % 2));
}

const char *
mapfrag_error(struct mapfragment *mf)
{
    const char *res = NULL;

    if (!mf) {
        res = "mapfragment error";
    } else if (!mapfrag_canmatch(mf)) {
        mapfrag_free(&mf);
        res = "mapfragment needs to have odd height and width";
    } else if (TYP_CANNOT_MATCH(mapfrag_get(mf, mf->wid / 2, mf->hei / 2))) {
        mapfrag_free(&mf);
        res = "mapfragment center must be valid terrain";
    }
    return res;
}

boolean
mapfrag_match(struct mapfragment* mf,  int x, int y)
{
    int rx, ry;

    for (rx = -(mf->wid / 2); rx <= (mf->wid / 2); rx++)
        for (ry = -(mf->hei / 2); ry <= (mf->hei / 2); ry++) {
            schar mapc = mapfrag_get(mf, rx + (mf->wid / 2),
                                     ry + (mf->hei / 2));
            schar levc = isok(x+rx, y+ry) ? levl[x+rx][y+ry].typ : STONE;

            if (!match_maptyps(mapc, levc))
                return FALSE;
        }
    return TRUE;
}

static void
solidify_map(void)
{
    xchar x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (IS_STWALL(levl[x][y].typ) && !SpLev_Map[x][y])
                levl[x][y].wall_info |= (W_NONDIGGABLE | W_NONPASSWALL);
}

static void
lvlfill_maze_grid(int x1, int y1, int x2, int y2, schar filling)
{
    int x, y;

    for (x = x1; x <= x2; x++)
        for (y = y1; y <= y2; y++) {
            if (g.level.flags.corrmaze)
                levl[x][y].typ = STONE;
            else
                levl[x][y].typ = (y < 2 || ((x % 2) && (y % 2))) ? STONE
                                                                 : filling;
        }
}

static void
lvlfill_solid(schar filling, schar lit)
{
    int x, y;

    for (x = 2; x <= g.x_maze_max; x++)
        for (y = 0; y <= g.y_maze_max; y++) {
            if (!set_levltyp_lit(x, y, filling, lit))
                continue;
            /* TODO: consolidate this w lspo_map ? */
            levl[x][y].flags = 0;
            levl[x][y].horizontal = 0;
            levl[x][y].roomno = 0;
            levl[x][y].edge = 0;
        }
}

static void
lvlfill_swamp(schar fg, schar bg, schar lit)
{
    int x, y;

    lvlfill_solid(bg, lit);

    /* "relaxed blockwise maze" algorithm, Jamis Buck */
    for (x = 2; x <= min(g.x_maze_max, COLNO-2); x += 2)
        for (y = 0; y <= min(g.y_maze_max, ROWNO-2); y += 2) {
            int c = 0;

            (void) set_levltyp_lit(x, y, fg, lit);
            if (levl[x + 1][y].typ == bg)
                ++c;
            if (levl[x][y + 1].typ == bg)
                ++c;
            if (levl[x + 1][y + 1].typ == bg)
                ++c;
            if (c == 3) {
                switch (rn2(3)) {
                case 0:
                    (void) set_levltyp_lit(x + 1,y, fg, lit);
                    break;
                case 1:
                    (void) set_levltyp_lit(x, y + 1, fg, lit);
                    break;
                case 2:
                    (void) set_levltyp_lit(x + 1, y + 1, fg, lit);
                    break;
                default:
                    break;
                }
            }
        }
}

static void
flip_drawbridge_horizontal(struct rm *lev)
{
    if (IS_DRAWBRIDGE(lev->typ)) {
        if ((lev->drawbridgemask & DB_DIR) == DB_WEST) {
            lev->drawbridgemask &= ~DB_WEST;
            lev->drawbridgemask |=  DB_EAST;
        } else if ((lev->drawbridgemask & DB_DIR) == DB_EAST) {
            lev->drawbridgemask &= ~DB_EAST;
            lev->drawbridgemask |=  DB_WEST;
        }
    }
}

static void
flip_drawbridge_vertical(struct rm *lev)
{
    if (IS_DRAWBRIDGE(lev->typ)) {
        if ((lev->drawbridgemask & DB_DIR) == DB_NORTH) {
            lev->drawbridgemask &= ~DB_NORTH;
            lev->drawbridgemask |=  DB_SOUTH;
        } else if ((lev->drawbridgemask & DB_DIR) == DB_SOUTH) {
            lev->drawbridgemask &= ~DB_SOUTH;
            lev->drawbridgemask |=  DB_NORTH;
        }
    }
}

/* for #wizfliplevel; not needed when flipping during level creation;
   update seen vector for whole flip area and glyph for known walls */
static void
flip_visuals(int flp, int minx, int miny, int maxx, int maxy)
{
    struct rm *lev;
    int x, y, seenv;

    for (y = miny; y <= maxy; ++y) {
        for (x = minx; x <= maxx; ++x) {
            lev = &levl[x][y];
            seenv = lev->seenv & 0xff;
            /* locations which haven't been seen can be skipped */
            if (seenv == 0)
                continue;
            /* flip <x,y>'s seen vector; not necessary for locations seen
               from all directions (the whole level after magic mapping) */
            if (seenv != SVALL) {
                /* SV2 SV1 SV0 *
                 * SV3 -+- SV7 *
                 * SV4 SV5 SV6 */
                if (flp & 1) { /* swap top and bottom */
                    seenv = swapbits(seenv, 2, 4);
                    seenv = swapbits(seenv, 1, 5);
                    seenv = swapbits(seenv, 0, 6);
                }
                if (flp & 2) { /* swap left and right */
                    seenv = swapbits(seenv, 2, 0);
                    seenv = swapbits(seenv, 3, 7);
                    seenv = swapbits(seenv, 4, 6);
                }
                lev->seenv = (uchar) seenv;
            }
            /* if <x,y> is displayed as a wall, reset its display glyph so
               that remembered, out of view T's and corners get flipped */
            if ((IS_WALL(lev->typ) || lev->typ == SDOOR)
                && glyph_is_cmap(lev->glyph))
                lev->glyph = back_to_glyph(x, y);
        }
    }
}

static int
flip_encoded_direction_bits(int flp, int val)
{
    /* These depend on xdir[] and ydir[] order */
    if (flp & 1) {
        val = swapbits(val, 1, 7);
        val = swapbits(val, 2, 6);
        val = swapbits(val, 3, 5);
    }
    if (flp & 2) {
        val = swapbits(val, 1, 3);
        val = swapbits(val, 0, 4);
        val = swapbits(val, 7, 5);
    }

    return val;
}

#define FlipX(val) ((maxx - (val)) + minx)
#define FlipY(val) ((maxy - (val)) + miny)
#define inFlipArea(x,y) \
    ((x) >= minx && (x) <= maxx && (y) >= miny && (y) <= maxy)
#define Flip_coord(cc) \
    do {                                            \
        if ((cc).x && inFlipArea((cc).x, (cc).y)) { \
            if (flp & 1)                            \
                (cc).y = FlipY((cc).y);             \
            if (flp & 2)                            \
                (cc).x = FlipX((cc).x);             \
        }                                           \
    } while (0)

/* transpose top with bottom or left with right or both; sometimes called
   for new special levels, or for any level via the #wizfliplevel command */
void
flip_level(int flp, boolean extras)
{
    int x, y, i, itmp;
    int minx, miny, maxx, maxy;
    struct rm trm;
    struct trap *ttmp;
    struct obj *otmp;
    struct monst *mtmp;
    struct engr *etmp;
    struct mkroom *sroom;
    timer_element *timer;
    boolean ball_active = FALSE, ball_fliparea = FALSE;
    stairway *stway;

    /* nothing to do unless (flp & 1) or (flp & 2) or both */
    if ((flp & 3) == 0)
        return;

    get_level_extends(&minx, &miny, &maxx, &maxy);
    /* get_level_extends() returns -1,-1 to COLNO,ROWNO at max */
    if (miny < 0)
        miny = 0;
    if (minx < 1)
        minx = 1;
    if (maxx >= COLNO)
        maxx = (COLNO - 1);
    if (maxy >= ROWNO)
        maxy = (ROWNO - 1);

    if (extras) {
        if (Punished && uball->where != OBJ_FREE) {
            ball_active = TRUE;
            /* if hero and ball and chain are all inside flip area,
               flip b&c coordinates along with other objects; if they
               are all outside, leave them to be rejected when flipping
               so that they stay as is; if some are inside and some are
               outside, un-place here and subsequently re-place them on
               hero's [possibly new] spot below */
            if (carried(uball))
                uball->ox = u.ux, uball->oy = u.uy;
            ball_fliparea = ((inFlipArea(uball->ox, uball->oy)
                              == inFlipArea(uchain->ox, uchain->oy))
                             && (inFlipArea(uball->ox, uball->oy)
                                 == inFlipArea(u.ux, u.uy)));
            if (!ball_fliparea)
                unplacebc();
        }
    }

    /* stairs and ladders */
    for (stway = g.stairs; stway; stway = stway->next) {
        if (flp & 1)
            stway->sy = FlipY(stway->sy);
        if (flp & 2)
            stway->sx = FlipX(stway->sx);
    }

    /* traps */
    for (ttmp = g.ftrap; ttmp; ttmp = ttmp->ntrap) {
        if (!inFlipArea(ttmp->tx, ttmp->ty))
            continue;
	if (flp & 1) {
	    ttmp->ty = FlipY(ttmp->ty);
	    if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		ttmp->launch.y = FlipY(ttmp->launch.y);
		ttmp->launch2.y = FlipY(ttmp->launch2.y);
	    } else if (is_pit(ttmp->ttyp) && ttmp->conjoined) {
                ttmp->conjoined = flip_encoded_direction_bits(flp,
                                                              ttmp->conjoined);
            }
	}
	if (flp & 2) {
	    ttmp->tx = FlipX(ttmp->tx);
	    if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		ttmp->launch.x = FlipX(ttmp->launch.x);
		ttmp->launch2.x = FlipX(ttmp->launch2.x);
	    } else if (is_pit(ttmp->ttyp) && ttmp->conjoined) {
                ttmp->conjoined = flip_encoded_direction_bits(flp,
                                                              ttmp->conjoined);
	    }
	}
    }

    /* objects */
    for (otmp = fobj; otmp; otmp = otmp->nobj) {
        if (!inFlipArea(otmp->ox, otmp->oy))
            continue;
	if (flp & 1)
	    otmp->oy = FlipY(otmp->oy);
	if (flp & 2)
	    otmp->ox = FlipX(otmp->ox);
    }

    /* buried objects */
    for (otmp = g.level.buriedobjlist; otmp; otmp = otmp->nobj) {
        if (!inFlipArea(otmp->ox, otmp->oy))
            continue;
	if (flp & 1)
	    otmp->oy = FlipY(otmp->oy);
	if (flp & 2)
	    otmp->ox = FlipX(otmp->ox);
    }

    /* monsters */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (mtmp->isgd && mtmp->mx == 0)
            continue;
        /* skip the occasional earth elemental outside the flip area */
        if (!inFlipArea(mtmp->mx, mtmp->my))
            continue;
	if (flp & 1)
	    mtmp->my = FlipY(mtmp->my);
	if (flp & 2)
	    mtmp->mx = FlipX(mtmp->mx);

        if (mtmp->ispriest) {
            Flip_coord(EPRI(mtmp)->shrpos);
        } else if (mtmp->isshk) {
            Flip_coord(ESHK(mtmp)->shk); /* shk's preferred spot */
            Flip_coord(ESHK(mtmp)->shd); /* shop door */
        } else if (mtmp->wormno) {
            if (flp & 1)
                flip_worm_segs_vertical(mtmp, miny, maxy);
            if (flp & 2)
		flip_worm_segs_horizontal(mtmp, minx, maxx);
	}
#if 0   /* not useful unless tracking also gets flipped */
        if (extras) {
            if (mtmp->tame && has_edog(mtmp))
                Flip_coord(EDOG(mtmp)->ogoal);
        }
#endif
    }

    /* engravings */
    for (etmp = head_engr; etmp; etmp = etmp->nxt_engr) {
	if (flp & 1)
	    etmp->engr_y = FlipY(etmp->engr_y);
	if (flp & 2)
	    etmp->engr_x = FlipX(etmp->engr_x);
    }

    /* level (teleport) regions */
    for (i = 0; i < g.num_lregions; i++) {
	if (flp & 1) {
	    g.lregions[i].inarea.y1 = FlipY(g.lregions[i].inarea.y1);
	    g.lregions[i].inarea.y2 = FlipY(g.lregions[i].inarea.y2);
	    if (g.lregions[i].inarea.y1 > g.lregions[i].inarea.y2) {
		itmp = g.lregions[i].inarea.y1;
		g.lregions[i].inarea.y1 = g.lregions[i].inarea.y2;
		g.lregions[i].inarea.y2 = itmp;
	    }

	    g.lregions[i].delarea.y1 = FlipY(g.lregions[i].delarea.y1);
	    g.lregions[i].delarea.y2 = FlipY(g.lregions[i].delarea.y2);
	    if (g.lregions[i].delarea.y1 > g.lregions[i].delarea.y2) {
		itmp = g.lregions[i].delarea.y1;
		g.lregions[i].delarea.y1 = g.lregions[i].delarea.y2;
		g.lregions[i].delarea.y2 = itmp;
	    }
	}
	if (flp & 2) {
	    g.lregions[i].inarea.x1 = FlipX(g.lregions[i].inarea.x1);
	    g.lregions[i].inarea.x2 = FlipX(g.lregions[i].inarea.x2);
	    if (g.lregions[i].inarea.x1 > g.lregions[i].inarea.x2) {
		itmp = g.lregions[i].inarea.x1;
		g.lregions[i].inarea.x1 = g.lregions[i].inarea.x2;
		g.lregions[i].inarea.x2 = itmp;
	    }

	    g.lregions[i].delarea.x1 = FlipX(g.lregions[i].delarea.x1);
	    g.lregions[i].delarea.x2 = FlipX(g.lregions[i].delarea.x2);
	    if (g.lregions[i].delarea.x1 > g.lregions[i].delarea.x2) {
		itmp = g.lregions[i].delarea.x1;
		g.lregions[i].delarea.x1 = g.lregions[i].delarea.x2;
		g.lregions[i].delarea.x2 = itmp;
	    }
	}
    }

    /* regions (poison clouds, etc) */
    for (i = 0; i < g.n_regions; i++) {
        int j, tmp1, tmp2;
        if (flp & 1) {
            tmp1 = FlipY(g.regions[i]->bounding_box.ly);
            tmp2 = FlipY(g.regions[i]->bounding_box.hy);
            g.regions[i]->bounding_box.ly = min(tmp1, tmp2);
            g.regions[i]->bounding_box.hy = max(tmp1, tmp2);
            for (j = 0; j < g.regions[i]->nrects; j++) {
                tmp1 = FlipY(g.regions[i]->rects[j].ly);
                tmp2 = FlipY(g.regions[i]->rects[j].hy);
                g.regions[i]->rects[j].ly = min(tmp1, tmp2);
                g.regions[i]->rects[j].hy = max(tmp1, tmp2);
            }
        }
        if (flp & 2) {
            tmp1 = FlipX(g.regions[i]->bounding_box.lx);
            tmp2 = FlipX(g.regions[i]->bounding_box.hx);
            g.regions[i]->bounding_box.lx = min(tmp1, tmp2);
            g.regions[i]->bounding_box.hx = max(tmp1, tmp2);
            for (j = 0; j < g.regions[i]->nrects; j++) {
                tmp1 = FlipX(g.regions[i]->rects[j].lx);
                tmp2 = FlipX(g.regions[i]->rects[j].hx);
                g.regions[i]->rects[j].lx = min(tmp1, tmp2);
                g.regions[i]->rects[j].hx = max(tmp1, tmp2);
            }
        }
    }

    /* rooms */
    for (sroom = &g.rooms[0]; ; sroom++) {
	if (sroom->hx < 0)
            break;

	if (flp & 1) {
	    sroom->ly = FlipY(sroom->ly);
	    sroom->hy = FlipY(sroom->hy);
	    if (sroom->ly > sroom->hy) {
		itmp = sroom->ly;
		sroom->ly = sroom->hy;
		sroom->hy = itmp;
	    }
	}
	if (flp & 2) {
	    sroom->lx = FlipX(sroom->lx);
	    sroom->hx = FlipX(sroom->hx);
	    if (sroom->lx > sroom->hx) {
		itmp = sroom->lx;
		sroom->lx = sroom->hx;
		sroom->hx = itmp;
	    }
	}

	if (sroom->nsubrooms)
	    for (i = 0; i < sroom->nsubrooms; i++) {
		struct mkroom *rroom = sroom->sbrooms[i];

		if (flp & 1) {
		    rroom->ly = FlipY(rroom->ly);
		    rroom->hy = FlipY(rroom->hy);
		    if (rroom->ly > rroom->hy) {
			itmp = rroom->ly;
			rroom->ly = rroom->hy;
			rroom->hy = itmp;
		    }
		}
		if (flp & 2) {
		    rroom->lx = FlipX(rroom->lx);
		    rroom->hx = FlipX(rroom->hx);
		    if (rroom->lx > rroom->hx) {
			itmp = rroom->lx;
			rroom->lx = rroom->hx;
			rroom->hx = itmp;
		    }
		}
	    }
    }

    /* doors */
    for (i = 0; i < g.doorindex; i++) {
	Flip_coord(g.doors[i]);
    }

    /* the map */
    if (flp & 1) {
	for (x = minx; x <= maxx; x++)
	    for (y = miny; y < (miny + ((maxy - miny + 1) / 2)); y++) {
                int ny = FlipY(y);

		flip_drawbridge_vertical(&levl[x][y]);
		flip_drawbridge_vertical(&levl[x][ny]);

		trm = levl[x][y];
		levl[x][y] = levl[x][ny];
		levl[x][ny] = trm;

		otmp = g.level.objects[x][y];
		g.level.objects[x][y] = g.level.objects[x][ny];
		g.level.objects[x][ny] = otmp;

		mtmp = g.level.monsters[x][y];
		g.level.monsters[x][y] = g.level.monsters[x][ny];
		g.level.monsters[x][ny] = mtmp;
	    }
    }
    if (flp & 2) {
	for (x = minx; x < (minx + ((maxx - minx + 1) / 2)); x++)
	    for (y = miny; y <= maxy; y++) {
                int nx = FlipX(x);

		flip_drawbridge_horizontal(&levl[x][y]);
		flip_drawbridge_horizontal(&levl[nx][y]);

		trm = levl[x][y];
		levl[x][y] = levl[nx][y];
		levl[nx][y] = trm;

		otmp = g.level.objects[x][y];
		g.level.objects[x][y] = g.level.objects[nx][y];
		g.level.objects[nx][y] = otmp;

		mtmp = g.level.monsters[x][y];
		g.level.monsters[x][y] = g.level.monsters[nx][y];
		g.level.monsters[nx][y] = mtmp;
	    }
    }

    /* timed effects */
    for (timer = g.timer_base; timer; timer = timer->next) {
        if (timer->func_index == MELT_ICE_AWAY) {
            long ty = timer->arg.a_long & 0xffff;
            long tx = (timer->arg.a_long >> 16) & 0xffff;

            if (flp & 1)
                ty = FlipY(ty);
            if (flp & 2)
                tx = FlipX(tx);
            timer->arg.a_long = ((tx << 16) | ty);
        }
    }

    if (extras) { /* for #wizfliplevel rather than during level creation */
        /* flip hero location only if inside the flippable area */
        if (inFlipArea(u.ux, u.uy)) {
            if (flp & 1)
                u.uy = FlipY(u.uy);
            if (flp & 2)
                u.ux = FlipX(u.ux);
            /* we could flip <ux0,uy0> too if it's inside the flip area,
               but have to resort to this if outside, so just do this */
            u.ux0 = u.ux, u.uy0 = u.uy;
        }
        if (ball_active && !ball_fliparea)
            placebc();
        Flip_coord(iflags.travelcc);
        Flip_coord(g.context.digging.pos);
    }

    fix_wall_spines(1, 0, COLNO - 1, ROWNO - 1);
    if (extras && flp) {
        set_wall_state();
        /* after wall_spines; flips seenv and wall joins */
        flip_visuals(flp, minx, miny, maxx, maxy);
    }
    vision_reset();
}

#undef FlipX
#undef FlipY
#undef inFlipArea

/* randomly transpose top with bottom or left with right or both;
   caller controls which transpositions are allowed */
void
flip_level_rnd(int flp, boolean extras)
{
    int c = 0;

    /* TODO?
     *  Might change rn2(2) to !rn2(3) or (rn2(5) < 2) in order to bias
     *  the outcome towards the traditional orientation.
     */
    if ((flp & 1) && rn2(2))
        c |= 1;
    if ((flp & 2) && rn2(2))
        c |= 2;

    if (c)
        flip_level(c, extras);
}


static void
sel_set_wall_property(int x, int y, genericptr_t arg)
{
    int prop = *(int *)arg;

    if (IS_STWALL(levl[x][y].typ) || IS_TREE(levl[x][y].typ)
        /* 3.6.2: made iron bars eligible to be flagged nondiggable
           (checked by chewing(hack.c) and zap_over_floor(zap.c)) */
        || levl[x][y].typ == IRONBARS)
        levl[x][y].wall_info |= prop;
}

/*
 * Make walls of the area (x1, y1, x2, y2) non diggable/non passwall-able
 */
static void
set_wall_property(xchar x1, xchar y1, xchar x2, xchar y2, int prop)
{
    register xchar x, y;

    x1 = max(x1, 1);
    x2 = min(x2, COLNO - 1);
    y1 = max(y1, 0);
    y2 = min(y2, ROWNO - 1);
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++) {
            sel_set_wall_property(x, y, (genericptr_t)&prop);
        }
}

/*
 * Count the different features (sinks, fountains) in the level.
 */
static void
count_features(void)
{
    xchar x, y;

    g.level.flags.nfountains = g.level.flags.nsinks = 0;
    for (y = 0; y < ROWNO; y++)
        for (x = 0; x < COLNO; x++) {
            int typ = levl[x][y].typ;
            if (typ == FOUNTAIN)
                g.level.flags.nfountains++;
            else if (typ == SINK)
                g.level.flags.nsinks++;
        }
}

static void
remove_boundary_syms(void)
{
    /*
     * If any CROSSWALLs are found, must change to ROOM after REGION's
     * are laid out.  CROSSWALLS are used to specify "invisible"
     * boundaries where DOOR syms look bad or aren't desirable.
     */
    xchar x, y;
    boolean has_bounds = FALSE;

    for (x = 0; x < COLNO - 1; x++)
        for (y = 0; y < ROWNO - 1; y++)
            if (levl[x][y].typ == CROSSWALL) {
                has_bounds = TRUE;
                break;
            }
    if (has_bounds) {
        for (x = 0; x < g.x_maze_max; x++)
            for (y = 0; y < g.y_maze_max; y++)
                if ((levl[x][y].typ == CROSSWALL) && SpLev_Map[x][y])
                    levl[x][y].typ = ROOM;
    }
}

/* used by sel_set_door() and link_doors_rooms() */
static void
set_door_orientation(int x, int y)
{
    boolean wleft, wright, wup, wdown;

    /* If there's a wall or door on either the left side or right
     * side (or both) of this secret door, make it be horizontal.
     *
     * It is feasible to put SDOOR in a corner, tee, or crosswall
     * position, although once the door is found and opened it won't
     * make a lot sense (diagonal access required).  Still, we try to
     * handle that as best as possible.  For top or bottom tee, using
     * horizontal is the best we can do.  For corner or crosswall,
     * either horizontal or vertical are just as good as each other;
     * we produce horizontal for corners and vertical for crosswalls.
     * For left or right tee, using vertical is best.
     *
     * A secret door with no adjacent walls is also feasible and makes
     * even less sense.  It will be displayed as a vertical wall while
     * hidden and become a vertical door when found.  Before resorting
     * to that, we check for solid rock which hasn't been wallified
     * yet (cf lower leftside of leader's room in Cav quest).
     */
    wleft  = (isok(x - 1, y) && (IS_WALL(levl[x - 1][y].typ)
                                 || IS_DOOR(levl[x - 1][y].typ)
                                 || levl[x - 1][y].typ == SDOOR));
    wright = (isok(x + 1, y) && (IS_WALL(levl[x + 1][y].typ)
                                 || IS_DOOR(levl[x + 1][y].typ)
                                 || levl[x + 1][y].typ == SDOOR));
    wup    = (isok(x, y - 1) && (IS_WALL(levl[x][y - 1].typ)
                                 || IS_DOOR(levl[x][y - 1].typ)
                                 || levl[x][y - 1].typ == SDOOR));
    wdown  = (isok(x, y + 1) && (IS_WALL(levl[x][y + 1].typ)
                                 || IS_DOOR(levl[x][y + 1].typ)
                                 || levl[x][y + 1].typ == SDOOR));
    if (!wleft && !wright && !wup && !wdown) {
        /* out of bounds is treated as implicit wall; should be academic
           because we don't expect to have doors so near the level's edge */
        wleft  = (!isok(x - 1, y) || IS_DOORJOIN(levl[x - 1][y].typ));
        wright = (!isok(x + 1, y) || IS_DOORJOIN(levl[x + 1][y].typ));
        wup    = (!isok(x, y - 1) || IS_DOORJOIN(levl[x][y - 1].typ));
        wdown  = (!isok(x, y + 1) || IS_DOORJOIN(levl[x][y + 1].typ));
    }
    levl[x][y].horizontal = ((wleft || wright) && !(wup && wdown)) ? 1 : 0;
}

/* is x,y right next to room droom? */
static boolean
shared_with_room(int x, int y, struct mkroom *droom)
{
    int rmno = (droom - g.rooms) + ROOMOFFSET;

    if (!isok(x,y))
        return FALSE;
    if ((int) levl[x][y].roomno == rmno && !levl[x][y].edge)
        return FALSE;
    if (isok(x-1, y) && (int) levl[x-1][y].roomno == rmno && x-1 <= droom->hx)
        return TRUE;
    if (isok(x+1, y) && (int) levl[x+1][y].roomno == rmno && x+1 >= droom->lx)
        return TRUE;
    if (isok(x, y-1) && (int) levl[x][y-1].roomno == rmno && y-1 <= droom->hy)
        return TRUE;
    if (isok(x, y+1) && (int) levl[x][y+1].roomno == rmno && y+1 >= droom->ly)
        return TRUE;
    return FALSE;
}

/* maybe add door at x,y to room droom */
static void
maybe_add_door(int x, int y, struct mkroom* droom)
{
    if (droom->hx >= 0 && g.doorindex < DOORMAX
        && ((!droom->irregular && inside_room(droom, x, y))
            || (int) levl[x][y].roomno == (droom - g.rooms) + ROOMOFFSET
            || shared_with_room(x, y, droom))) {
        add_door(x, y, droom);
    }
}

/* link all doors in the map to their corresponding rooms */
static void
link_doors_rooms(void)
{
    int x, y;
    int tmpi, m;

    for (y = 0; y < ROWNO; y++)
        for (x = 0; x < COLNO; x++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR) {
                /* in case this door was a '+' or 'S' from the
                   MAP...ENDMAP section without an explicit DOOR
                   directive, set/clear levl[][].horizontal for it */
                set_door_orientation(x, y);

                for (tmpi = 0; tmpi < g.nroom; tmpi++) {
                    maybe_add_door(x, y, &g.rooms[tmpi]);
                    for (m = 0; m < g.rooms[tmpi].nsubrooms; m++) {
                        maybe_add_door(x, y, g.rooms[tmpi].sbrooms[m]);
                    }
                }
            }
}

/*
 * Choose randomly the state (nodoor, open, closed or locked) for a door
 */
static int
rnddoor(void)
{
    static int state[] = { D_NODOOR, D_BROKEN, D_ISOPEN, D_CLOSED, D_LOCKED };

    return state[rn2(SIZE(state))];
}

/*
 * Select a random trap
 */
static int
rndtrap(void)
{
    int rtrap;

    do {
        rtrap = rnd(TRAPNUM - 1);
        switch (rtrap) {
        case HOLE: /* no random holes on special levels */
        case VIBRATING_SQUARE:
        case MAGIC_PORTAL:
            rtrap = NO_TRAP;
            break;
        case TRAPDOOR:
            if (!Can_dig_down(&u.uz))
                rtrap = NO_TRAP;
            break;
        case LEVEL_TELEP:
        case TELEP_TRAP:
            if (g.level.flags.noteleport)
                rtrap = NO_TRAP;
            break;
        case ROLLING_BOULDER_TRAP:
        case ROCKTRAP:
            if (In_endgame(&u.uz))
                rtrap = NO_TRAP;
            break;
        }
    } while (rtrap == NO_TRAP);
    return rtrap;
}

/*
 * Coordinates in special level files are handled specially:
 *
 *      if x or y is < 0, we generate a random coordinate.
 *      The "humidity" flag is used to ensure that engravings aren't
 *      created underwater, or eels on dry land.
 */
static void
get_location(
    xchar *x, xchar *y,
    getloc_flags_t humidity,
    struct mkroom *croom)
{
    int cpt = 0;
    int mx, my, sx, sy;

    if (croom) {
        mx = croom->lx;
        my = croom->ly;
        sx = croom->hx - mx + 1;
        sy = croom->hy - my + 1;
    } else {
        mx = g.xstart;
        my = g.ystart;
        sx = g.xsize;
        sy = g.ysize;
    }

    if (*x >= 0) { /* normal locations */
        *x += mx;
        *y += my;
    } else { /* random location */
        do {
            if (croom) { /* handle irregular areas */
                coord tmpc;
                (void) somexy(croom, &tmpc);
                *x = tmpc.x;
                *y = tmpc.y;
            } else {
                *x = mx + rn2((int) sx);
                *y = my + rn2((int) sy);
            }
            if (is_ok_location(*x, *y, humidity))
                break;
        } while (++cpt < 100);
        if (cpt >= 100) {
            register int xx, yy;

            /* last try */
            for (xx = 0; xx < sx; xx++)
                for (yy = 0; yy < sy; yy++) {
                    *x = mx + xx;
                    *y = my + yy;
                    if (is_ok_location(*x, *y, humidity))
                        goto found_it;
                }
            if (!(humidity & NO_LOC_WARN)) {
                impossible("get_location:  can't find a place!");
            } else {
                *x = *y = -1;
            }
        }
    }
 found_it:
    ;

    if (!(humidity & ANY_LOC) && !isok(*x, *y)) {
        if (!(humidity & NO_LOC_WARN)) {
            /*warning("get_location:  (%d,%d) out of bounds", *x, *y);*/
            *x = g.x_maze_max;
            *y = g.y_maze_max;
        } else {
            *x = *y = -1;
        }
    }
}

static boolean (*is_ok_location_func)(xchar, xchar) = NULL;

static void
set_ok_location_func(boolean (*func)(xchar, xchar))
{
    is_ok_location_func = func;
}

static boolean
is_ok_location(xchar x, xchar y, getloc_flags_t humidity)
{
    register int typ = levl[x][y].typ;

    if (Is_waterlevel(&u.uz))
        return TRUE; /* accept any spot */

    if (is_ok_location_func)
        return is_ok_location_func(x, y);

    /* TODO: Should perhaps check if wall is diggable/passwall? */
    if (humidity & ANY_LOC)
        return TRUE;

    if ((humidity & SOLID) && IS_ROCK(typ))
        return TRUE;

    if ((humidity & (DRY|SPACELOC)) && SPACE_POS(typ)) {
        boolean bould = (sobj_at(BOULDER, x, y) != NULL);

        if (!bould || (bould && (humidity & SOLID)))
            return TRUE;
    }
    if ((humidity & WET) && is_pool(x, y))
        return TRUE;
    if ((humidity & HOT) && is_lava(x, y))
        return TRUE;
    return FALSE;
}

boolean
pm_good_location(int x, int y, struct permonst* pm)
{
    return is_ok_location(x, y, pm_to_humidity(pm));
}

static unpacked_coord
get_unpacked_coord(long loc, int defhumidity)
{
    static unpacked_coord c;

    if (loc & SP_COORD_IS_RANDOM) {
        c.x = c.y = -1;
        c.is_random = 1;
        c.getloc_flags = (getloc_flags_t)(loc & ~SP_COORD_IS_RANDOM);
        if (!c.getloc_flags)
            c.getloc_flags = defhumidity;
    } else {
        c.is_random = 0;
        c.getloc_flags = defhumidity;
        c.x = SP_COORD_X(loc);
        c.y = SP_COORD_Y(loc);
    }
    return c;
}

void
get_location_coord(
    xchar *x, xchar *y,
    int humidity,
    struct mkroom *croom,
    long crd)
{
    unpacked_coord c;

    c = get_unpacked_coord(crd, humidity);
    *x = c.x;
    *y = c.y;
    get_location(x, y, c.getloc_flags | (c.is_random ? NO_LOC_WARN : 0),
                 croom);
    if (*x == -1 && *y == -1 && c.is_random)
        get_location(x, y, humidity, croom);
}

/*
 * Get a relative position inside a room.
 * negative values for x or y means RANDOM!
 */
static void
get_room_loc(xchar *x, xchar *y, struct mkroom *croom)
{
    coord c;

    if (*x < 0 && *y < 0) {
        if (somexy(croom, &c)) {
            *x = c.x;
            *y = c.y;
        } else
            panic("get_room_loc : can't find a place!");
    } else {
        if (*x < 0)
            *x = rn2(croom->hx - croom->lx + 1);
        if (*y < 0)
            *y = rn2(croom->hy - croom->ly + 1);
        *x += croom->lx;
        *y += croom->ly;
    }
}

/*
 * Get a relative position inside a room.
 * negative values for x or y means RANDOM!
 */
static void
get_free_room_loc(
    xchar *x, xchar *y,
    struct mkroom *croom,
    packed_coord pos)
{
    xchar try_x, try_y;
    register int trycnt = 0;

    get_location_coord(&try_x, &try_y, DRY, croom, pos);
    if (levl[try_x][try_y].typ != ROOM) {
        do {
            try_x = *x, try_y = *y;
            get_room_loc(&try_x, &try_y, croom);
        } while (levl[try_x][try_y].typ != ROOM && ++trycnt <= 100);

        if (trycnt > 100)
            panic("get_free_room_loc:  can't find a place!");
    }
    *x = try_x, *y = try_y;
}

boolean
check_room(
    xchar *lowx, xchar *ddx,
    xchar *lowy, xchar *ddy,
    boolean vault)
{
    register int x, y, hix = *lowx + *ddx, hiy = *lowy + *ddy;
    register struct rm *lev;
    int xlim, ylim, ymax;
    xchar s_lowx, s_ddx, s_lowy, s_ddy;

    s_lowx = *lowx; s_ddx = *ddx;
    s_lowy = *lowy; s_ddy = *ddy;

    xlim = XLIM + (vault ? 1 : 0);
    ylim = YLIM + (vault ? 1 : 0);

    if (*lowx < 3)
        *lowx = 3;
    if (*lowy < 2)
        *lowy = 2;
    if (hix > COLNO - 3)
        hix = COLNO - 3;
    if (hiy > ROWNO - 3)
        hiy = ROWNO - 3;
 chk:
    if (hix <= *lowx || hiy <= *lowy)
        return FALSE;

    if (g.in_mk_themerooms && (s_lowx != *lowx) && (s_ddx != *ddx)
        && (s_lowy != *lowy) && (s_ddy != *ddy))
        return FALSE;

    /* check area around room (and make room smaller if necessary) */
    for (x = *lowx - xlim; x <= hix + xlim; x++) {
        if (x <= 0 || x >= COLNO)
            continue;
        y = *lowy - ylim;
        ymax = hiy + ylim;
        if (y < 0)
            y = 0;
        if (ymax >= ROWNO)
            ymax = (ROWNO - 1);
        lev = &levl[x][y];
        for (; y <= ymax; y++) {
            if (lev++->typ != STONE) {
                if (!vault) {
                    debugpline2("strange area [%d,%d] in check_room.", x, y);
                }
                if (!rn2(3))
                    return FALSE;
                if (g.in_mk_themerooms)
                    return FALSE;
                if (x < *lowx)
                    *lowx = x + xlim + 1;
                else
                    hix = x - xlim - 1;
                if (y < *lowy)
                    *lowy = y + ylim + 1;
                else
                    hiy = y - ylim - 1;
                goto chk;
            }
        }
    }
    *ddx = hix - *lowx;
    *ddy = hiy - *lowy;

    if (g.in_mk_themerooms && (s_lowx != *lowx) && (s_ddx != *ddx)
        && (s_lowy != *lowy) && (s_ddy != *ddy))
        return FALSE;

    return TRUE;
}

/*
 * Create a new room.
 * This is still very incomplete...
 */
boolean
create_room(
    xchar x, xchar y,
    xchar w, xchar h,
    xchar xal, xchar yal,
    xchar rtype, xchar rlit)
{
    xchar xabs = 0, yabs = 0;
    int wtmp, htmp, xaltmp, yaltmp, xtmp, ytmp;
    NhRect *r1 = 0, r2;
    int trycnt = 0;
    boolean vault = FALSE;
    int xlim = XLIM, ylim = YLIM;

    if (rtype == -1) /* Is the type random ? */
        rtype = OROOM;

    if (rtype == VAULT) {
        vault = TRUE;
        xlim++;
        ylim++;
    }

    /* on low levels the room is lit (usually) */
    /* some other rooms may require lighting */

    /* is light state random ? */
    rlit = litstate_rnd(rlit);

    /*
     * Here we will try to create a room. If some parameters are
     * random we are willing to make several try before we give
     * it up.
     */
    do {
        xchar xborder, yborder;

        wtmp = w;
        htmp = h;
        xtmp = x;
        ytmp = y;
        xaltmp = xal;
        yaltmp = yal;

        /* First case : a totally random room */

        if ((xtmp < 0 && ytmp < 0 && wtmp < 0 && xaltmp < 0 && yaltmp < 0)
            || vault) {
            xchar hx, hy, lx, ly, dx, dy;

            r1 = rnd_rect(); /* Get a random rectangle */

            if (!r1) { /* No more free rectangles ! */
                debugpline0("No more rects...");
                return FALSE;
            }
            hx = r1->hx;
            hy = r1->hy;
            lx = r1->lx;
            ly = r1->ly;
            if (vault)
                dx = dy = 1;
            else {
                dx = 2 + rn2((hx - lx > 28) ? 12 : 8);
                dy = 2 + rn2(4);
                if (dx * dy > 50)
                    dy = 50 / dx;
            }
            xborder = (lx > 0 && hx < COLNO - 1) ? 2 * xlim : xlim + 1;
            yborder = (ly > 0 && hy < ROWNO - 1) ? 2 * ylim : ylim + 1;
            if (hx - lx < dx + 3 + xborder || hy - ly < dy + 3 + yborder) {
                r1 = 0;
                continue;
            }
            xabs = lx + (lx > 0 ? xlim : 3)
                   + rn2(hx - (lx > 0 ? lx : 3) - dx - xborder + 1);
            yabs = ly + (ly > 0 ? ylim : 2)
                   + rn2(hy - (ly > 0 ? ly : 2) - dy - yborder + 1);
            if (ly == 0 && hy >= (ROWNO - 1) && (!g.nroom || !rn2(g.nroom))
                && (yabs + dy > ROWNO / 2)) {
                yabs = rn1(3, 2);
                if (g.nroom < 4 && dy > 1)
                    dy--;
            }
            if (!check_room(&xabs, &dx, &yabs, &dy, vault)) {
                r1 = 0;
                continue;
            }
            wtmp = dx + 1;
            htmp = dy + 1;
            r2.lx = xabs - 1;
            r2.ly = yabs - 1;
            r2.hx = xabs + wtmp;
            r2.hy = yabs + htmp;
        } else { /* Only some parameters are random */
            int rndpos = 0;
            xchar dx, dy;

            if (xtmp < 0 && ytmp < 0) { /* Position is RANDOM */
                xtmp = rnd(5);
                ytmp = rnd(5);
                rndpos = 1;
            }
            if (wtmp < 0 || htmp < 0) { /* Size is RANDOM */
                wtmp = rn1(15, 3);
                htmp = rn1(8, 2);
            }
            if (xaltmp == -1) /* Horizontal alignment is RANDOM */
                xaltmp = rnd(3);
            if (yaltmp == -1) /* Vertical alignment is RANDOM */
                yaltmp = rnd(3);

            /* Try to generate real (absolute) coordinates here! */

            xabs = (((xtmp - 1) * COLNO) / 5) + 1;
            yabs = (((ytmp - 1) * ROWNO) / 5) + 1;
            switch (xaltmp) {
            case LEFT:
                break;
            case RIGHT:
                xabs += (COLNO / 5) - wtmp;
                break;
            case CENTER:
                xabs += ((COLNO / 5) - wtmp) / 2;
                break;
            }
            switch (yaltmp) {
            case TOP:
                break;
            case BOTTOM:
                yabs += (ROWNO / 5) - htmp;
                break;
            case CENTER:
                yabs += ((ROWNO / 5) - htmp) / 2;
                break;
            }

            if (xabs + wtmp - 1 > COLNO - 2)
                xabs = COLNO - wtmp - 3;
            if (xabs < 2)
                xabs = 2;
            if (yabs + htmp - 1 > ROWNO - 2)
                yabs = ROWNO - htmp - 3;
            if (yabs < 2)
                yabs = 2;

            /* Try to find a rectangle that fit our room ! */

            r2.lx = xabs - 1;
            r2.ly = yabs - 1;
            r2.hx = xabs + wtmp + rndpos;
            r2.hy = yabs + htmp + rndpos;
            r1 = get_rect(&r2);
            dx = wtmp;
            dy = htmp;

            if (r1 && !check_room(&xabs, &dx, &yabs, &dy, vault)) {
                r1 = 0;
            }
        }
    } while (++trycnt <= 100 && !r1);
    if (!r1) { /* creation of room failed ? */
        return FALSE;
    }
    split_rects(r1, &r2);

    if (!vault) {
        g.smeq[g.nroom] = g.nroom;
        add_room(xabs, yabs, xabs + wtmp - 1, yabs + htmp - 1, rlit, rtype,
                 FALSE);
    } else {
        g.rooms[g.nroom].lx = xabs;
        g.rooms[g.nroom].ly = yabs;
    }
    return TRUE;
}

/*
 * Create a subroom in room proom at pos x,y with width w & height h.
 * x & y are relative to the parent room.
 */
static boolean
create_subroom(
    struct mkroom *proom,
    xchar x, xchar y,
    xchar w, xchar h,
    xchar rtype, xchar rlit)
{
    xchar width, height;

    width = proom->hx - proom->lx + 1;
    height = proom->hy - proom->ly + 1;

    /* There is a minimum size for the parent room */
    if (width < 4 || height < 4)
        return FALSE;

    /* Check for random position, size, etc... */

    if (w == -1)
        w = rnd(width - 3);
    if (h == -1)
        h = rnd(height - 3);
    if (x == -1)
        x = rnd(width - w - 1) - 1;
    if (y == -1)
        y = rnd(height - h - 1) - 1;
    if (x == 1)
        x = 0;
    if (y == 1)
        y = 0;
    if ((x + w + 1) == width)
        x++;
    if ((y + h + 1) == height)
        y++;
    if (rtype == -1)
        rtype = OROOM;
    rlit = litstate_rnd(rlit);
    add_subroom(proom, proom->lx + x, proom->ly + y, proom->lx + x + w - 1,
                proom->ly + y + h - 1, rlit, rtype, FALSE);
    return TRUE;
}

/*
 * Create a new door in a room.
 * It's placed on a wall (north, south, east or west).
 */
static void
create_door(room_door *dd, struct mkroom *broom)
{
    int x = 0, y = 0;
    int trycnt;

    if (dd->secret == -1)
        dd->secret = rn2(2);

    if (dd->wall == W_RANDOM)
        dd->wall = W_ANY; /* speeds things up in the below loop */

    if (dd->mask == -1) {
        /* is it a locked door, closed, or a doorway? */
        if (!dd->secret) {
            if (!rn2(3)) {
                if (!rn2(5))
                    dd->mask = D_ISOPEN;
                else if (!rn2(6))
                    dd->mask = D_LOCKED;
                else
                    dd->mask = D_CLOSED;
                if (dd->mask != D_ISOPEN && !rn2(25))
                    dd->mask |= D_TRAPPED;
            } else
                dd->mask = D_NODOOR;
        } else {
            if (!rn2(5))
                dd->mask = D_LOCKED;
            else
                dd->mask = D_CLOSED;

            if (!rn2(20))
                dd->mask |= D_TRAPPED;
        }
    }

    for (trycnt = 0; trycnt < 100; ++trycnt) {
        int dwall = dd->wall, dpos = dd->pos;

        /* Convert wall and pos into an absolute coordinate! */
        switch (rn2(4)) {
        case 0:
            if (!(dwall & W_NORTH))
                continue;
            y = broom->ly - 1;
            x = broom->lx + ((dpos == -1) ? rn2(1 + broom->hx - broom->lx)
                                          : dpos);
            if (!isok(x, y - 1) || IS_ROCK(levl[x][y - 1].typ))
                continue;
            break;
        case 1:
            if (!(dwall & W_SOUTH))
                continue;
            y = broom->hy + 1;
            x = broom->lx + ((dpos == -1) ? rn2(1 + broom->hx - broom->lx)
                                          : dpos);
            if (!isok(x, y + 1) || IS_ROCK(levl[x][y + 1].typ))
                continue;
            break;
        case 2:
            if (!(dwall & W_WEST))
                continue;
            x = broom->lx - 1;
            y = broom->ly + ((dpos == -1) ? rn2(1 + broom->hy - broom->ly)
                                          : dpos);
            if (!isok(x - 1, y) || IS_ROCK(levl[x - 1][y].typ))
                continue;
            break;
        case 3:
            if (!(dwall & W_EAST))
                continue;
            x = broom->hx + 1;
            y = broom->ly + ((dpos == -1) ? rn2(1 + broom->hy - broom->ly)
                                          : dpos);
            if (!isok(x + 1, y) || IS_ROCK(levl[x + 1][y].typ))
                continue;
            break;
        default:
            /*NOTREACHED*/
            break;
        }

        if (okdoor(x, y))
            break;
    }
    if (trycnt >= 100) {
        impossible("create_door: Can't find a proper place!");
        return;
    }
    if (!set_levltyp(x, y, (dd->secret ? SDOOR : DOOR)))
        return;
    levl[x][y].doormask = dd->mask;
}

/*
 * Create a trap in a room.
 */
static void
create_trap(spltrap* t, struct mkroom* croom)
{
    xchar x = -1, y = -1;
    coord tm;
    int mktrap_flags = MKTRAP_MAZEFLAG;

    if (croom) {
        get_free_room_loc(&x, &y, croom, t->coord);
    } else {
        int trycnt = 0;

        do {
            get_location_coord(&x, &y, DRY, croom, t->coord);
        } while ((levl[x][y].typ == STAIRS || levl[x][y].typ == LADDER)
                 && ++trycnt <= 100);
        if (trycnt > 100)
            return;
    }

    if (!t->spider_on_web)
        mktrap_flags |= MKTRAP_NOSPIDERONWEB;
    if (t->seen)
        mktrap_flags |= MKTRAP_SEEN;

    tm.x = x;
    tm.y = y;

    mktrap(t->type, mktrap_flags, (struct mkroom *) 0, &tm);
}

/*
 * Create a monster in a room.
 */
static int
noncoalignment(aligntyp alignment)
{
    int k;

    k = rn2(2);
    if (!alignment)
        return (k ? -1 : 1);
    return (k ? -alignment : 0);
}

/* attempt to screen out locations where a mimic-as-boulder shouldn't occur */
static boolean
m_bad_boulder_spot(int x, int y)
{
    struct rm *lev;

    /* avoid trap locations */
    if (t_at(x, y))
        return TRUE;
    /* try to avoid locations which already have a boulder (this won't
       actually work; we get called before objects have been placed...) */
    if (sobj_at(BOULDER, x, y))
        return TRUE;
    /* avoid closed doors */
    lev = &levl[x][y];
    if (IS_DOOR(lev->typ) && (lev->doormask & (D_CLOSED | D_LOCKED)) != 0)
        return TRUE;
    /* spot is ok */
    return FALSE;
}

static int
pm_to_humidity(struct permonst* pm)
{
    int loc = DRY;

    if (!pm)
        return loc;
    if (pm->mlet == S_EEL || amphibious(pm) || is_swimmer(pm))
        loc = WET;
    if (is_flyer(pm) || is_floater(pm))
        loc |= (HOT | WET);
    if (passes_walls(pm) || noncorporeal(pm))
        loc |= SOLID;
    if (likes_fire(pm))
        loc |= HOT;
    return loc;
}

/*
 * Convert a special level alignment mask (an alignment mask with possible
 * extra values/flags) to a "normal" alignment mask (no extra flags).
 *
 * When random: there is an 80% chance that the altar will be co-aligned.
 */
static unsigned int
sp_amask_to_amask(unsigned int sp_amask)
{
    unsigned int amask;

    if (sp_amask == AM_SPLEV_CO)
        amask = Align2amask(u.ualignbase[A_ORIGINAL]);
    else if (sp_amask == AM_SPLEV_NONCO)
        amask = Align2amask(noncoalignment(u.ualignbase[A_ORIGINAL]));
    else if (sp_amask == AM_SPLEV_RANDOM)
        amask = induced_align(80);
    else
        amask = sp_amask & AM_MASK;

    return amask;
}

static void
create_monster(monster* m, struct mkroom* croom)
{
    struct monst *mtmp;
    xchar x, y;
    char class;
    unsigned int amask;
    coord cc;
    struct permonst *pm;
    unsigned g_mvflags;

    if (m->class >= 0)
        class = (char) def_char_to_monclass((char) m->class);
    else
        class = 0;

    if (class == MAXMCLASSES)
        panic("create_monster: unknown monster class '%c'", m->class);

    amask = sp_amask_to_amask(m->sp_amask);

    if (!class)
        pm = (struct permonst *) 0;
    else if (m->id != NON_PM) {
        pm = &mons[m->id];
        g_mvflags = (unsigned) g.mvitals[monsndx(pm)].mvflags;
        if ((pm->geno & G_UNIQ) && (g_mvflags & G_EXTINCT))
            return;
        else if (g_mvflags & G_GONE)    /* genocided or extinct */
            pm = (struct permonst *) 0; /* make random monster */
    } else {
        pm = mkclass(class, G_NOGEN);
        /* if we can't get a specific monster type (pm == 0) then the
           class has been genocided, so settle for a random monster */
    }
    if (In_mines(&u.uz) && pm && your_race(pm)
        && (Race_if(PM_DWARF) || Race_if(PM_GNOME)) && rn2(3))
        pm = (struct permonst *) 0;

    if (pm) {
        int loc = pm_to_humidity(pm);

        /* If water-liking monster, first try is without DRY */
        get_location_coord(&x, &y, loc | NO_LOC_WARN, croom, m->coord);
        if (x == -1 && y == -1) {
            loc |= DRY;
            get_location_coord(&x, &y, loc, croom, m->coord);
        }
    } else {
        get_location_coord(&x, &y, DRY, croom, m->coord);
    }

    /* try to find a close place if someone else is already there */
    if (MON_AT(x, y) && enexto(&cc, x, y, pm))
        x = cc.x, y = cc.y;

    if (croom && !inside_room(croom, x, y))
        return;

    if (m->sp_amask != AM_SPLEV_RANDOM)
        mtmp = mk_roamer(pm, Amask2align(amask), x, y, m->peaceful);
    else if (PM_ARCHEOLOGIST <= m->id && m->id <= PM_WIZARD)
        mtmp = mk_mplayer(pm, x, y, FALSE);
    else
        mtmp = makemon(pm, x, y, m->mm_flags);

    if (mtmp) {
        x = mtmp->mx, y = mtmp->my; /* sanity precaution */
        m->x = x, m->y = y;
        /* handle specific attributes for some special monsters */
        if (m->name.str)
            mtmp = christen_monst(mtmp, m->name.str);

        /*
         * This doesn't complain if an attempt is made to give a
         * non-mimic/non-shapechanger an appearance or to give a
         * shapechanger a non-monster shape, it just refuses to comply.
         */
        if (m->appear_as.str
            && ((mtmp->data->mlet == S_MIMIC)
                /* shapechanger (chameleons, et al, and vampires) */
                || (mtmp->cham >= LOW_PM && m->appear == M_AP_MONSTER))
            && !Protection_from_shape_changers) {
            int i;

            switch (m->appear) {
            case M_AP_NOTHING:
                impossible(
                 "create_monster: mon has an appearance, \"%s\", but no type",
                           m->appear_as.str);
                break;

            case M_AP_FURNITURE:
                for (i = 0; i < MAXPCHARS; i++)
                    if (!strcmp(defsyms[i].explanation, m->appear_as.str))
                        break;
                if (i == MAXPCHARS) {
                    impossible("create_monster: can't find feature \"%s\"",
                               m->appear_as.str);
                } else {
                    mtmp->m_ap_type = M_AP_FURNITURE;
                    mtmp->mappearance = i;
                }
                break;

            case M_AP_OBJECT:
                for (i = 0; i < NUM_OBJECTS; i++)
                    if (OBJ_NAME(objects[i])
                        && !strcmp(OBJ_NAME(objects[i]), m->appear_as.str))
                        break;
                if (i == NUM_OBJECTS) {
                    impossible("create_monster: can't find object \"%s\"",
                               m->appear_as.str);
                } else {
                    mtmp->m_ap_type = M_AP_OBJECT;
                    mtmp->mappearance = i;
                    /* try to avoid placing mimic boulder on a trap */
                    if (i == BOULDER && m->x < 0
                        && m_bad_boulder_spot(x, y)) {
                        int retrylimit = 10;

                        remove_monster(x, y);
                        do {
                            x = m->x;
                            y = m->y;
                            get_location(&x, &y, DRY, croom);
                            if (MON_AT(x, y) && enexto(&cc, x, y, pm))
                                x = cc.x, y = cc.y;
                        } while (m_bad_boulder_spot(x, y)
                                 && --retrylimit > 0);
                        place_monster(mtmp, x, y);
                        /* if we didn't find a good spot
                           then mimic something else */
                        if (!retrylimit)
                            set_mimic_sym(mtmp);
                    }
                }
                break;

            case M_AP_MONSTER: {
                int mndx, gender_name_var = NEUTRAL;

                if (!strcmpi(m->appear_as.str, "random"))
                    mndx = select_newcham_form(mtmp);
                else
                    mndx = name_to_mon(m->appear_as.str, &gender_name_var);

                if (mndx == NON_PM || (is_vampshifter(mtmp)
                                       && !validvamp(mtmp, &mndx, S_HUMAN))) {
                    impossible("create_monster: invalid %s (\"%s\")",
                               (mtmp->data->mlet == S_MIMIC)
                                 ? "mimic appearance"
                                 : (mtmp->data == &mons[PM_WIZARD_OF_YENDOR])
                                     ? "Wizard appearance"
                                     : is_vampshifter(mtmp)
                                         ? "vampire shape"
                                         : "chameleon shape",
                               m->appear_as.str);
                } else if (&mons[mndx] == mtmp->data) {
                    /* explicitly forcing a mimic to appear as itself */
                    mtmp->m_ap_type = M_AP_NOTHING;
                    mtmp->mappearance = 0;
                } else if (mtmp->data->mlet == S_MIMIC
                           || mtmp->data == &mons[PM_WIZARD_OF_YENDOR]) {
                    /* this is ordinarily only used for Wizard clones
                       and hasn't been exhaustively tested for mimics */
                    mtmp->m_ap_type = M_AP_MONSTER;
                    mtmp->mappearance = mndx;
                } else { /* chameleon or vampire */
                    struct permonst *mdat = &mons[mndx];
                    struct permonst *olddata = mtmp->data;

                    mgender_from_permonst(mtmp, mdat);
                    if (gender_name_var != NEUTRAL)
                        mtmp->female = gender_name_var;
                    set_mon_data(mtmp, mdat);
                    if (emits_light(olddata) != emits_light(mtmp->data)) {
                        /* used to give light, now doesn't, or vice versa,
                           or light's range has changed */
                        if (emits_light(olddata))
                            del_light_source(LS_MONSTER, monst_to_any(mtmp));
                        if (emits_light(mtmp->data))
                            new_light_source(mtmp->mx, mtmp->my,
                                             emits_light(mtmp->data),
                                             LS_MONSTER, monst_to_any(mtmp));
                    }
                    if (!mtmp->perminvis || pm_invisible(olddata))
                        mtmp->perminvis = pm_invisible(mdat);
                }
                break;
            }
            default:
                impossible(
                  "create_monster: unimplemented mon appear type [%d,\"%s\"]",
                           m->appear, m->appear_as.str);
                break;
            }
            if (does_block(x, y, &levl[x][y]))
                block_point(x, y);
        }

        mtmp->female = m->female;
        if (m->peaceful > BOOL_RANDOM) {
            mtmp->mpeaceful = m->peaceful;
            /* changed mpeaceful again; have to reset malign */
            set_malign(mtmp);
        }
        if (m->asleep > BOOL_RANDOM)
            mtmp->msleeping = m->asleep;
        if (m->seentraps)
            mtmp->mtrapseen = m->seentraps;
        if (m->cancelled)
            mtmp->mcan = 1;
        if (m->revived)
            mtmp->mrevived = 1;
        if (m->avenge)
            mtmp->mavenge = 1;
        if (m->stunned)
            mtmp->mstun = 1;
        if (m->confused)
            mtmp->mconf = 1;
        if (m->invis) {
            mtmp->minvis = mtmp->perminvis = 1;
        }
        if (m->blinded) {
            mtmp->mcansee = 0;
            mtmp->mblinded = (m->blinded % 127);
        }
        if (m->paralyzed) {
            mtmp->mcanmove = 0;
            mtmp->mfrozen = (m->paralyzed % 127);
        }
        if (m->fleeing) {
            mtmp->mflee = 1;
            mtmp->mfleetim = (m->fleeing % 127);
        }
        if (m->waiting) {
            mtmp->mstrategy |= STRAT_WAITFORU;
        }
        if (m->has_invent) {
            discard_minvent(mtmp, TRUE);
            invent_carrying_monster = mtmp;
        }
    }
}

/*
 * Create an object in a room.
 */
static void
create_object(object* o, struct mkroom* croom)
{
    struct obj *otmp;
    xchar x, y;
    char c;
    boolean named; /* has a name been supplied in level description? */

    named = o->name.str ? TRUE : FALSE;

    get_location_coord(&x, &y, DRY, croom, o->coord);

    if (o->class >= 0)
        c = o->class;
    else
        c = 0;

    if (!c) {
        otmp = mkobj_at(RANDOM_CLASS, x, y, !named);
    } else if (o->id != -1) {
        otmp = mksobj_at(o->id, x, y, TRUE, !named);
    } else {
        /*
         * The special levels are compiled with the default "text" object
         * class characters.  We must convert them to the internal format.
         */
        char oclass = (char) def_char_to_objclass(c);

        if (oclass == MAXOCLASSES)
            panic("create_object:  unexpected object class '%c'", c);

        /* KMH -- Create piles of gold properly */
        if (oclass == COIN_CLASS)
            otmp = mkgold(0L, x, y);
        else
            otmp = mkobj_at(oclass, x, y, !named);
    }

    if (o->spe != -127) /* That means NOT RANDOM! */
        otmp->spe = (schar) o->spe;

    switch (o->curse_state) {
    case 1: /* blessed */
        bless(otmp);
        break;
    case 2: /* uncursed */
        unbless(otmp);
        uncurse(otmp);
        break;
    case 3: /* cursed */
        curse(otmp);
        break;
    case 4: /* not cursed */
        uncurse(otmp);
        break;
    case 5: /* not uncursed */
        blessorcurse(otmp, 1);
        break;
    case 6: /* not blessed */
        unbless(otmp);
        break;
    default: /* random */
        break; /* keep what mkobj gave us */
    }

    /* corpsenm is "empty" if -1, random if -2, otherwise specific */
    if (o->corpsenm != NON_PM) {
        if (o->corpsenm == NON_PM - 1)
            set_corpsenm(otmp, rndmonnum());
        else
            set_corpsenm(otmp, o->corpsenm);
    }
    /* set_corpsenm() took care of egg hatch and corpse timers */

    if (named) {
        otmp = oname(otmp, o->name.str, ONAME_LEVEL_DEF);
        if (otmp->otyp == SPE_NOVEL) {
            /* needs to be an existing title */
            (void) lookup_novel(o->name.str, &otmp->novelidx);
        }
    }
    if (o->eroded) {
        if (o->eroded < 0) {
            otmp->oerodeproof = 1;
        } else {
            otmp->oeroded = (o->eroded % 4);
            otmp->oeroded2 = ((o->eroded >> 2) % 4);
        }
    }
    if (o->recharged)
        otmp->recharged = (o->recharged % 8);
    if (o->locked) {
        otmp->olocked = 1;
    } else if (o->broken) {
        otmp->obroken = 1;
        otmp->olocked = 0; /* obj generation may set */
    }
    if (o->trapped == 0 || o->trapped == 1)
        otmp->otrapped = o->trapped;
    if (o->greased)
        otmp->greased = 1;

    if (o->quan > 0 && objects[otmp->otyp].oc_merge) {
        otmp->quan = o->quan;
        otmp->owt = weight(otmp);
    }

    /* contents (of a container or monster's inventory) */
    if (o->containment & SP_OBJ_CONTENT || invent_carrying_monster) {
        if (!container_idx) {
            if (!invent_carrying_monster) {
                /*impossible("create_object: no container");*/
                /* don't complain, the monster may be gone legally
                   (eg. unique demon already generated)
                   TODO: In the case of unique demon lords, they should
                   get their inventories even when they get generated
                   outside the des-file.  Maybe another data file that
                   determines what inventories monsters get by default?
                 */
                ; /* ['otmp' remains on floor] */
            } else {
                remove_object(otmp);
                if (otmp->otyp == SADDLE)
                    put_saddle_on_mon(otmp, invent_carrying_monster);
                else
                    (void) mpickobj(invent_carrying_monster, otmp);
            }
        } else {
            struct obj *cobj = container_obj[container_idx - 1];

            remove_object(otmp);
            if (cobj) {
                otmp = add_to_container(cobj, otmp);
                cobj->owt = weight(cobj);
            } else {
                obj_extract_self(otmp);
                /* uncreate a random artifact created in a container */
                /* FIXME: it could be intentional rather than random */
                if (otmp->oartifact)
                    artifact_exists(otmp, safe_oname(otmp), FALSE,
                                    ONAME_NO_FLAGS); /* flags don't matter */
                obfree(otmp, NULL);
                return;
            }
        }
    }
    /* container */
    if (o->containment & SP_OBJ_CONTAINER) {
        delete_contents(otmp);
        if (container_idx < MAX_CONTAINMENT) {
            container_obj[container_idx] = otmp;
            container_idx++;
        } else
            impossible("create_object: too deeply nested containers.");
    }

    /* Medusa level special case: statues are petrified monsters, so they
     * are not stone-resistant and have monster inventory.  They also lack
     * other contents, but that can be specified as an empty container.
     */
    if (o->id == STATUE && Is_medusa_level(&u.uz) && o->corpsenm == NON_PM) {
        struct monst *was = NULL;
        struct obj *obj;
        int wastyp;
        int i = 0; /* prevent endless loop in case makemon always fails */

        /* Named random statues are of player types, and aren't stone-
         * resistant (if they were, we'd have to reset the name as well as
         * setting corpsenm).
         */
        for (wastyp = otmp->corpsenm; i < 1000; i++, wastyp = rndmonnum()) {
            /* makemon without rndmonst() might create a group */
            was = makemon(&mons[wastyp], 0, 0, MM_NOCOUNTBIRTH|MM_NOMSG);
            if (was) {
                if (!resists_ston(was) && !poly_when_stoned(&mons[wastyp])) {
                    (void) propagate(wastyp, TRUE, FALSE);
                    break;
                }
                mongone(was);
                was = NULL;
            }
        }
        if (was) {
            set_corpsenm(otmp, wastyp);
            while (was->minvent) {
                obj = was->minvent;
                obj->owornmask = 0;
                obj_extract_self(obj);
                (void) add_to_container(otmp, obj);
            }
            otmp->owt = weight(otmp);
            mongone(was);
        }
    }

    if (o->achievement) {
        static const char prize_warning[] = "multiple prizes on %s level";

        if (Is_mineend_level(&u.uz)) {
            if (!g.context.achieveo.mines_prize_oid) {
                g.context.achieveo.mines_prize_oid = otmp->o_id;
                g.context.achieveo.mines_prize_otyp = otmp->otyp;
                /* prevent stacking; cleared when achievement is recorded */
                otmp->nomerge = 1;
            } else {
                impossible(prize_warning, "mines end");
            }
        } else if (Is_sokoend_level(&u.uz)) {
            if (!g.context.achieveo.soko_prize_oid) {
                g.context.achieveo.soko_prize_oid = otmp->o_id;
                g.context.achieveo.soko_prize_otyp = otmp->otyp;
                otmp->nomerge = 1; /* redundant; Sokoban prizes don't stack */
            } else {
                impossible(prize_warning, "sokoban end");
            }
        } else if (!iflags.lua_testing) {
            char lbuf[QBUFSZ];

            (void) describe_level(lbuf, 1 | 2);
            impossible("create_object: unknown achievement (%s\"%s\")",
                       lbuf, simpleonames(otmp));
        }
    }

    if (!(o->containment & SP_OBJ_CONTENT)) {
        stackobj(otmp);

        if (o->lit)
            begin_burn(otmp, FALSE);

        if (o->buried) {
            boolean dealloced;

            (void) bury_an_obj(otmp, &dealloced);
            if (dealloced && container_idx) {
                container_obj[container_idx - 1] = NULL;
            }
        }
    }
}

/*
 * Create an altar in a room.
 */
static void
create_altar(altar* a, struct mkroom* croom)
{
    schar sproom;
    xchar x = -1, y = -1;
    unsigned int amask;
    boolean croom_is_temple = TRUE;

    if (croom) {
        get_free_room_loc(&x, &y, croom, a->coord);
        if (croom->rtype != TEMPLE)
            croom_is_temple = FALSE;
    } else {
        get_location_coord(&x, &y, DRY, croom, a->coord);
        if ((sproom = (schar) *in_rooms(x, y, TEMPLE)) != 0)
            croom = &g.rooms[sproom - ROOMOFFSET];
        else
            croom_is_temple = FALSE;
    }

    /* check for existing features */
    if (!set_levltyp(x, y, ALTAR))
        return;

    amask = sp_amask_to_amask(a->sp_amask);

    levl[x][y].altarmask = amask;

    if (a->shrine < 0)
        a->shrine = rn2(2); /* handle random case */

    if (!croom_is_temple || !a->shrine)
        return;

    if (a->shrine) { /* Is it a shrine  or sanctum? */
        priestini(&u.uz, croom, x, y, (a->shrine > 1));
        levl[x][y].altarmask |= AM_SHRINE;
        g.level.flags.has_temple = TRUE;
    }
}

/*
 * Search for a door in a room on a specified wall.
 */
static boolean
search_door(
    struct mkroom* croom,
    xchar *x, xchar * y,
    xchar wall, int cnt)
{
    int dx, dy;
    int xx, yy;

    switch (wall) {
    case W_SOUTH:
        dy = 0;
        dx = 1;
        xx = croom->lx;
        yy = croom->hy + 1;
        break;
    case W_NORTH:
        dy = 0;
        dx = 1;
        xx = croom->lx;
        yy = croom->ly - 1;
        break;
    case W_EAST:
        dy = 1;
        dx = 0;
        xx = croom->hx + 1;
        yy = croom->ly;
        break;
    case W_WEST:
        dy = 1;
        dx = 0;
        xx = croom->lx - 1;
        yy = croom->ly;
        break;
    default:
        dx = dy = xx = yy = 0;
        panic("search_door: Bad wall!");
        break;
    }
    while (xx <= croom->hx + 1 && yy <= croom->hy + 1) {
        if (IS_DOOR(levl[xx][yy].typ) || levl[xx][yy].typ == SDOOR) {
            *x = xx;
            *y = yy;
            if (cnt-- <= 0)
                return TRUE;
        }
        xx += dx;
        yy += dy;
    }
    return FALSE;
}

/*
 * Dig a corridor between two points.
 */
boolean
dig_corridor(
    coord *org,
    coord  *dest,
    boolean nxcor,
    schar ftyp,
    schar btyp)
{
    int dx = 0, dy = 0, dix, diy, cct;
    struct rm *crm;
    int tx, ty, xx, yy;

    xx = org->x;
    yy = org->y;
    tx = dest->x;
    ty = dest->y;
    if (xx <= 0 || yy <= 0 || tx <= 0 || ty <= 0 || xx > COLNO - 1
        || tx > COLNO - 1 || yy > ROWNO - 1 || ty > ROWNO - 1) {
        debugpline4("dig_corridor: bad coords <%d,%d> <%d,%d>.",
                    xx, yy, tx, ty);
        return FALSE;
    }
    if (tx > xx)
        dx = 1;
    else if (ty > yy)
        dy = 1;
    else if (tx < xx)
        dx = -1;
    else
        dy = -1;

    xx -= dx;
    yy -= dy;
    cct = 0;
    while (xx != tx || yy != ty) {
        /* loop: dig corridor at [xx,yy] and find new [xx,yy] */
        if (cct++ > 500 || (nxcor && !rn2(35)))
            return FALSE;

        xx += dx;
        yy += dy;

        if (xx >= COLNO - 1 || xx <= 0 || yy <= 0 || yy >= ROWNO - 1)
            return FALSE; /* impossible */

        crm = &levl[xx][yy];
        if (crm->typ == btyp) {
            if (ftyp != CORR || rn2(100)) {
                crm->typ = ftyp;
                if (nxcor && !rn2(50))
                    (void) mksobj_at(BOULDER, xx, yy, TRUE, FALSE);
            } else {
                crm->typ = SCORR;
            }
        } else if (crm->typ != ftyp && crm->typ != SCORR) {
            /* strange ... */
            return FALSE;
        }

        /* find next corridor position */
        dix = abs(xx - tx);
        diy = abs(yy - ty);

        if ((dix > diy) && diy && !rn2(dix - diy + 1)) {
            dix = 0;
        } else if ((diy > dix) && dix && !rn2(diy - dix + 1)) {
            diy = 0;
        }

        /* do we have to change direction ? */
        if (dy && dix > diy) {
            register int ddx = (xx > tx) ? -1 : 1;

            crm = &levl[xx + ddx][yy];
            if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
                dx = ddx;
                dy = 0;
                continue;
            }
        } else if (dx && diy > dix) {
            register int ddy = (yy > ty) ? -1 : 1;

            crm = &levl[xx][yy + ddy];
            if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
                dy = ddy;
                dx = 0;
                continue;
            }
        }

        /* continue straight on? */
        crm = &levl[xx + dx][yy + dy];
        if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
            continue;

        /* no, what must we do now?? */
        if (dx) {
            dx = 0;
            dy = (ty < yy) ? -1 : 1;
        } else {
            dy = 0;
            dx = (tx < xx) ? -1 : 1;
        }
        crm = &levl[xx + dx][yy + dy];
        if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
            continue;
        dy = -dy;
        dx = -dx;
    }
    return TRUE;
}

/*
 * Corridors always start from a door. But it can end anywhere...
 * Basically we search for door coordinates or for endpoints coordinates
 * (from a distance).
 */
static void
create_corridor(corridor *c)
{
    coord org, dest;

    if (c->src.room == -1) {
        makecorridors(); /*makecorridors(c->src.door);*/
        return;
    }

    /* Safety railings - if there's ever a case where des.corridor() needs to be
     * called with src/destwall="random", that logic first needs to be
     * implemented in search_door. */
    if (c->src.wall == W_ANY || c->src.wall == W_RANDOM
        || c->dest.wall == W_ANY || c->dest.wall == W_RANDOM) {
        impossible("create_corridor to/from a random wall");
        return;
    }
    if (!search_door(&g.rooms[c->src.room], &org.x, &org.y, c->src.wall,
                     c->src.door))
        return;
    if (c->dest.room != -1) {
        if (!search_door(&g.rooms[c->dest.room],
                         &dest.x, &dest.y, c->dest.wall, c->dest.door))
            return;
        switch (c->src.wall) {
        case W_NORTH:
            org.y--;
            break;
        case W_SOUTH:
            org.y++;
            break;
        case W_WEST:
            org.x--;
            break;
        case W_EAST:
            org.x++;
            break;
        }
        switch (c->dest.wall) {
        case W_NORTH:
            dest.y--;
            break;
        case W_SOUTH:
            dest.y++;
            break;
        case W_WEST:
            dest.x--;
            break;
        case W_EAST:
            dest.x++;
            break;
        }
        (void) dig_corridor(&org, &dest, FALSE, CORR, STONE);
    }
}

/*
 * Fill a room (shop, zoo, etc...) with appropriate stuff.
 */
void
fill_special_room(struct mkroom* croom)
{
    int i;

    if (!croom)
        return;

    /* First recurse into subrooms. We don't want to block an ordinary room with
     * a special subroom from having the subroom filled, or an unfilled outer
     * room preventing a special subroom from being filled. */
    for (i = 0; i < croom->nsubrooms; ++i) {
        fill_special_room(croom->sbrooms[i]);
    }

    if (croom->rtype == OROOM || croom->rtype == THEMEROOM
        || croom->needfill == FILL_NONE)
        return;

    if (croom->needfill == FILL_NORMAL) {
        int x, y;

        /* Shop ? */
        if (croom->rtype >= SHOPBASE) {
            stock_room(croom->rtype - SHOPBASE, croom);
            g.level.flags.has_shop = TRUE;
            return;
        }

        switch (croom->rtype) {
        case VAULT:
            for (x = croom->lx; x <= croom->hx; x++)
                for (y = croom->ly; y <= croom->hy; y++)
                    (void) mkgold((long) rn1(abs(depth(&u.uz)) * 100, 51),
                                  x, y);
            break;
        case COURT:
        case ZOO:
        case BEEHIVE:
        case ANTHOLE:
        case COCKNEST:
        case LEPREHALL:
        case MORGUE:
        case BARRACKS:
            fill_zoo(croom);
            break;
        }
    }
    switch (croom->rtype) {
    case VAULT:
        g.level.flags.has_vault = TRUE;
        break;
    case ZOO:
        g.level.flags.has_zoo = TRUE;
        break;
    case COURT:
        g.level.flags.has_court = TRUE;
        break;
    case MORGUE:
        g.level.flags.has_morgue = TRUE;
        break;
    case BEEHIVE:
        g.level.flags.has_beehive = TRUE;
        break;
    case BARRACKS:
        g.level.flags.has_barracks = TRUE;
        break;
    case TEMPLE:
        g.level.flags.has_temple = TRUE;
        break;
    case SWAMP:
        g.level.flags.has_swamp = TRUE;
        break;
    }
}

static struct mkroom *
build_room(room *r, struct mkroom* mkr)
{
    boolean okroom;
    struct mkroom *aroom;
    xchar rtype = (!r->chance || rn2(100) < r->chance) ? r->rtype : OROOM;

    if (mkr) {
        aroom = &g.subrooms[g.nsubroom];
        okroom = create_subroom(mkr, r->x, r->y, r->w, r->h, rtype, r->rlit);
    } else {
        aroom = &g.rooms[g.nroom];
        okroom = create_room(r->x, r->y, r->w, r->h, r->xalign, r->yalign,
                             rtype, r->rlit);
    }

    if (okroom) {
#ifdef SPECIALIZATION
        topologize(aroom, FALSE); /* set roomno */
#else
        topologize(aroom); /* set roomno */
#endif
        aroom->needfill = r->needfill;
        aroom->needjoining = r->joined;
        return aroom;
    }
    return (struct mkroom *) 0;
}

/*
 * set lighting in a region that will not become a room.
 */
static void
light_region(region* tmpregion)
{
    register boolean litstate = tmpregion->rlit ? 1 : 0;
    register int hiy = tmpregion->y2;
    register int x, y;
    register struct rm *lev;
    int lowy = tmpregion->y1;
    int lowx = tmpregion->x1, hix = tmpregion->x2;

    if (litstate) {
        /* adjust region size for walls, but only if lighted */
        lowx = max(lowx - 1, 1);
        hix = min(hix + 1, COLNO - 1);
        lowy = max(lowy - 1, 0);
        hiy = min(hiy + 1, ROWNO - 1);
    }
    for (x = lowx; x <= hix; x++) {
        lev = &levl[x][lowy];
        for (y = lowy; y <= hiy; y++) {
            if (lev->typ != LAVAPOOL) /* this overrides normal lighting */
                lev->lit = litstate;
            lev++;
        }
    }
}

void
wallify_map(int x1, int y1, int x2, int y2)
{
    int x, y, xx, yy, lo_xx, lo_yy, hi_xx, hi_yy;

    y1 = max(y1, 0);
    x1 = max(x1, 1);
    y2 = min(y2, ROWNO - 1);
    x2 = min(x2, COLNO - 1);
    for (y = y1; y <= y2; y++) {
        lo_yy = (y > 0) ? y - 1 : 0;
        hi_yy = (y < y2) ? y + 1 : y2;
        for (x = x1; x <= x2; x++) {
            if (levl[x][y].typ != STONE)
                continue;
            lo_xx = (x > 1) ? x - 1 : 1;
            hi_xx = (x < x2) ? x + 1 : x2;
            for (yy = lo_yy; yy <= hi_yy; yy++)
                for (xx = lo_xx; xx <= hi_xx; xx++)
                    if (IS_ROOM(levl[xx][yy].typ)
                        || levl[xx][yy].typ == CROSSWALL) {
                        levl[x][y].typ = (yy != y) ? HWALL : VWALL;
                        yy = hi_yy; /* end `yy' loop */
                        break;      /* end `xx' loop */
                    }
        }
    }
}

/*
 * Select a random coordinate in the maze.
 *
 * We want a place not 'touched' by the loader.  That is, a place in
 * the maze outside every part of the special level.
 */
static void
maze1xy(coord *m, int humidity)
{
    register int x, y, tryct = 2000;
    /* tryct:  normally it won't take more than ten or so tries due
       to the circumstances under which we'll be called, but the
       `humidity' screening might drastically change the chances */

    do {
        x = rn1(g.x_maze_max - 3, 3);
        y = rn1(g.y_maze_max - 3, 3);
        if (--tryct < 0)
            break; /* give up */
    } while (!(x % 2) || !(y % 2) || SpLev_Map[x][y]
             || !is_ok_location((xchar) x, (xchar) y, humidity));

    m->x = (xchar) x, m->y = (xchar) y;
}

/*
 * If there's a significant portion of maze unused by the special level,
 * we don't want it empty.
 *
 * Makes the number of traps, monsters, etc. proportional
 * to the size of the maze.
 */
static void
fill_empty_maze(void)
{
    int mapcountmax, mapcount, mapfact;
    xchar x, y;
    coord mm;

    mapcountmax = mapcount = (g.x_maze_max - 2) * (g.y_maze_max - 2);
    mapcountmax = mapcountmax / 2;

    for (x = 2; x < g.x_maze_max; x++)
        for (y = 0; y < g.y_maze_max; y++)
            if (SpLev_Map[x][y])
                mapcount--;

    if ((mapcount > (int) (mapcountmax / 10))) {
        mapfact = (int) ((mapcount * 100L) / mapcountmax);
        for (x = rnd((int) (20 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) mkobj_at(rn2(2) ? GEM_CLASS : RANDOM_CLASS, mm.x, mm.y,
                            TRUE);
        }
        for (x = rnd((int) (12 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) mksobj_at(BOULDER, mm.x, mm.y, TRUE, FALSE);
        }
        for (x = rn2(2); x; x--) {
            maze1xy(&mm, DRY);
            (void) makemon(&mons[PM_MINOTAUR], mm.x, mm.y, NO_MM_FLAGS);
        }
        for (x = rnd((int) (12 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) makemon((struct permonst *) 0, mm.x, mm.y, NO_MM_FLAGS);
        }
        for (x = rn2((int) (15 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) mkgold(0L, mm.x, mm.y);
        }
        for (x = rn2((int) (15 * mapfact) / 100); x; x--) {
            int trytrap;

            maze1xy(&mm, DRY);
            trytrap = rndtrap();
            if (sobj_at(BOULDER, mm.x, mm.y))
                while (is_pit(trytrap) || is_hole(trytrap))
                    trytrap = rndtrap();
            (void) maketrap(mm.x, mm.y, trytrap);
        }
    }
}

static void
splev_initlev(lev_init* linit)
{
    switch (linit->init_style) {
    default:
        impossible("Unrecognized level init style.");
        break;
    case LVLINIT_NONE:
        break;
    case LVLINIT_SOLIDFILL:
        if (linit->lit == BOOL_RANDOM)
            linit->lit = rn2(2);
        lvlfill_solid(linit->filling, linit->lit);
        break;
    case LVLINIT_MAZEGRID:
        lvlfill_maze_grid(2, 0, g.x_maze_max, g.y_maze_max, linit->bg);
        break;
    case LVLINIT_MAZE:
        create_maze(linit->corrwid, linit->wallthick, linit->rm_deadends);
        break;
    case LVLINIT_ROGUE:
        makeroguerooms();
        break;
    case LVLINIT_MINES:
        if (linit->lit == BOOL_RANDOM)
            linit->lit = rn2(2);
        if (linit->filling > -1)
            lvlfill_solid(linit->filling, 0);
        linit->icedpools = icedpools;
        mkmap(linit);
        break;
    case LVLINIT_SWAMP:
        if (linit->lit == BOOL_RANDOM)
            linit->lit = rn2(2);
        lvlfill_swamp(linit->fg, linit->bg, linit->lit);
        break;
    }
}

#if 0
static long
sp_code_jmpaddr(long curpos, long jmpaddr)
{
    return (curpos + jmpaddr);
}
#endif


/*ARGUSED*/
static void
spo_end_moninvent(void)
{
    if (invent_carrying_monster)
        m_dowear(invent_carrying_monster, TRUE);
    invent_carrying_monster = NULL;
}

/*ARGUSED*/
static void
spo_pop_container(void)
{
    if (container_idx > 0) {
        container_idx--;
        container_obj[container_idx] = NULL;
    }
}

/* push a table on lua stack: {width=wid, height=hei} */
static void
l_push_wid_hei_table(lua_State *L, int wid, int hei)
{
    lua_newtable(L);
    nhl_add_table_entry_int(L, "width", wid);
    nhl_add_table_entry_int(L, "height", hei);
}

/* push a table on lua stack containing room data */
static void
l_push_mkroom_table(lua_State *L, struct mkroom *tmpr)
{
    lua_newtable(L);
    nhl_add_table_entry_int(L, "width", 1 + (tmpr->hx - tmpr->lx));
    nhl_add_table_entry_int(L, "height", 1 + (tmpr->hy - tmpr->ly));
    nhl_add_table_entry_region(L, "region", tmpr->lx, tmpr->ly,
                               tmpr->hx, tmpr->hy);
    nhl_add_table_entry_bool(L, "lit", (boolean) tmpr->rlit);
    nhl_add_table_entry_bool(L, "irregular", tmpr->irregular);
    nhl_add_table_entry_bool(L, "needjoining", tmpr->needjoining);
    nhl_add_table_entry_str(L, "type", get_mkroom_name(tmpr->rtype));
}

/* message("What a strange feeling!"); */
int
lspo_message(lua_State *L)
{
    char *levmsg;
    int old_n, n;
    const char *msg;

    int argc = lua_gettop(L);

    if (argc < 1) {
        nhl_error(L, "Wrong parameters");
        return 0;
    }

    create_des_coder();

    msg = luaL_checkstring(L, 1);

    old_n = g.lev_message ? (Strlen(g.lev_message) + 1) : 0;
    n = Strlen(msg);

    levmsg = (char *) alloc(old_n + n + 1);
    if (old_n)
        levmsg[old_n - 1] = '\n';
    if (g.lev_message)
        (void) memcpy((genericptr_t) levmsg, (genericptr_t) g.lev_message,
                      old_n - 1);
    (void) memcpy((genericptr_t) &levmsg[old_n], msg, n);
    levmsg[old_n + n] = '\0';
    Free(g.lev_message);
    g.lev_message = levmsg;

    return 0;  /* number of results */
}

static int
get_table_align(lua_State *L)
{
    static const char *const gtaligns[] = {
        "noalign", "law", "neutral", "chaos",
        "coaligned", "noncoaligned", "random", NULL
    };
    static const int aligns2i[] = {
        AM_NONE, AM_LAWFUL, AM_NEUTRAL, AM_CHAOTIC,
        AM_SPLEV_CO, AM_SPLEV_NONCO, AM_SPLEV_RANDOM, 0
    };

    int a = aligns2i[get_table_option(L, "align", "random", gtaligns)];

    return a;
}

static int
get_table_monclass(lua_State *L)
{
    char *s = get_table_str_opt(L, "class", NULL);
    int ret = -1;

    if (s && strlen(s) == 1)
        ret = (int) *s;
    Free(s);
    return ret;
}

static int
find_montype(
    lua_State *L UNUSED,
    const char *s,
    int *mgender)
{
    int i, mgend = NEUTRAL;

    i = name_to_monplus(s, (const char **) 0, &mgend);
    if (i >= LOW_PM && i < NUMMONS) {
        if (is_male(&mons[i]) || is_female(&mons[i]))
            mgend = is_female(&mons[i]) ? FEMALE : MALE;
        else
            mgend = (mgend == FEMALE) ? FEMALE
                        : (mgend == MALE) ? MALE : rn2(2);
        if (mgender)
            *mgender = mgend;
        return i;
    }
    if (mgender)
        *mgender = NEUTRAL;
    return NON_PM;
}

static int
get_table_montype(lua_State *L, int *mgender)
{
    char *s = get_table_str_opt(L, "id", NULL);
    int ret = NON_PM;

    if (s) {
        ret = find_montype(L, s, mgender);
        Free(s);
        if (ret == NON_PM)
            nhl_error(L, "Unknown monster id");
    }
    return ret;
}

static void
get_table_xy_or_coord(lua_State *L, lua_Integer *x, lua_Integer *y)
{
    lua_Integer mx = get_table_int_opt(L, "x", -1);
    lua_Integer my = get_table_int_opt(L, "y", -1);

    if (mx == -1 && my == -1) {
        lua_getfield(L, 1, "coord");
        (void) get_coord(L, -1, &mx, &my);
        lua_pop(L, 1);
    }

    *x = mx;
    *y = my;
}

/* monster(); */
/* monster("wood nymph"); */
/* monster("D"); */
/* monster("giant eel",11,06); */
/* monster("hill giant", {08,06}); */
/* monster({ id = "giant mimic", appear_as = "obj:boulder" }); */
/* monster({ class = "H", peaceful = 0 }); */
int
lspo_monster(lua_State *L)
{
    int argc = lua_gettop(L);
    monster tmpmons;
    lua_Integer mx = -1, my = -1;
    int mgend = NEUTRAL;
    char *mappear = NULL;

    create_des_coder();

    tmpmons.peaceful = -1;
    tmpmons.asleep = -1;
    tmpmons.name.str = NULL;
    tmpmons.appear = 0;
    tmpmons.appear_as.str = (char *) 0;
    tmpmons.sp_amask = AM_SPLEV_RANDOM;
    tmpmons.female = 0;
    tmpmons.invis = 0;
    tmpmons.cancelled = 0;
    tmpmons.revived = 0;
    tmpmons.avenge = 0;
    tmpmons.fleeing = 0;
    tmpmons.blinded = 0;
    tmpmons.paralyzed = 0;
    tmpmons.stunned = 0;
    tmpmons.confused = 0;
    tmpmons.seentraps = 0;
    tmpmons.has_invent = 0;
    tmpmons.waiting = 0;
    tmpmons.mm_flags = NO_MM_FLAGS;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        const char *paramstr = luaL_checkstring(L, 1);

        if (strlen(paramstr) == 1) {
            tmpmons.class = *paramstr;
            tmpmons.id = NON_PM;
        } else {
            tmpmons.class = -1;
            tmpmons.id = find_montype(L, paramstr, &mgend);
            tmpmons.female = (mgend == FEMALE) ? FEMALE
                                : (mgend == MALE) ? MALE : rn2(2);
        }
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
               && lua_type(L, 2) == LUA_TTABLE) {
        const char *paramstr = luaL_checkstring(L, 1);

        (void) get_coord(L, 2, &mx, &my);

        if (strlen(paramstr) == 1) {
            tmpmons.class = *paramstr;
            tmpmons.id = NON_PM;
        } else {
            tmpmons.class = -1;
            tmpmons.id = find_montype(L, paramstr, &mgend);
            tmpmons.female = (mgend == FEMALE) ? FEMALE
                                : (mgend == MALE) ? MALE : rn2(2);
        }

    } else if (argc == 3) {
        const char *paramstr = luaL_checkstring(L, 1);

        mx = luaL_checkinteger(L, 2);
        my = luaL_checkinteger(L, 3);

        if (strlen(paramstr) == 1) {
            tmpmons.class = *paramstr;
            tmpmons.id = NON_PM;
        } else {
            tmpmons.class = -1;
            tmpmons.id = find_montype(L, paramstr, &mgend);
            tmpmons.female = (mgend == FEMALE) ? FEMALE
                                : (mgend == MALE) ? MALE : rn2(2);
        }
    } else {
        lcheck_param_table(L);

        tmpmons.peaceful = get_table_boolean_opt(L, "peaceful", BOOL_RANDOM);
        tmpmons.asleep = get_table_boolean_opt(L, "asleep", BOOL_RANDOM);
        tmpmons.name.str = get_table_str_opt(L, "name", NULL);
        tmpmons.appear = 0;
        tmpmons.appear_as.str = (char *) 0;
        tmpmons.sp_amask = get_table_align(L);
        tmpmons.female = get_table_boolean_opt(L, "female", BOOL_RANDOM);
        tmpmons.invis = get_table_boolean_opt(L, "invisible", FALSE);
        tmpmons.cancelled = get_table_boolean_opt(L, "cancelled", FALSE);
        tmpmons.revived = get_table_boolean_opt(L, "revived", FALSE);
        tmpmons.avenge = get_table_boolean_opt(L, "avenge", FALSE);
        tmpmons.fleeing = get_table_int_opt(L, "fleeing", 0);
        tmpmons.blinded = get_table_int_opt(L, "blinded", 0);
        tmpmons.paralyzed = get_table_int_opt(L, "paralyzed", 0);
        tmpmons.stunned = get_table_boolean_opt(L, "stunned", FALSE);
        tmpmons.confused = get_table_boolean_opt(L, "confused", FALSE);
        tmpmons.waiting = get_table_boolean_opt(L, "waiting", FALSE);
        tmpmons.seentraps = 0; /* TODO: list of trap names to bitfield */
        tmpmons.has_invent = 0;

        if (!get_table_boolean_opt(L, "tail", TRUE))
            tmpmons.mm_flags |= MM_NOTAIL;
        if (!get_table_boolean_opt(L, "group", TRUE))
            tmpmons.mm_flags |= MM_NOGRP;
        if (get_table_boolean_opt(L, "adjacentok", FALSE))
            tmpmons.mm_flags |= MM_ADJACENTOK;
        if (get_table_boolean_opt(L, "ignorewater", FALSE))
            tmpmons.mm_flags |= MM_IGNOREWATER;
        if (!get_table_boolean_opt(L, "countbirth", TRUE))
            tmpmons.mm_flags |= MM_NOCOUNTBIRTH;

        mappear = get_table_str_opt(L, "appear_as", NULL);
        if (mappear) {
            if (!strncmp("obj:", mappear, 4)) {
                tmpmons.appear = M_AP_OBJECT;
            } else if (!strncmp("mon:", mappear, 4)) {
                tmpmons.appear = M_AP_MONSTER;
            } else if (!strncmp("ter:", mappear, 4)) {
                tmpmons.appear = M_AP_FURNITURE;
            } else {
                nhl_error(L, "Unknown appear_as type");
            }
            tmpmons.appear_as.str = dupstr(&mappear[4]);
            Free(mappear);
        }

        get_table_xy_or_coord(L, &mx, &my);

        tmpmons.id = get_table_montype(L, &mgend);
        /* get_table_montype will return a random gender if the species isn't
         * all-male or all-female; if the level designer specified a certain
         * gender, override that random one now, unless it *is* a one-gender
         * species, in which case don't override (don't permit creation of a
         * male nymph or female Nazgul, etc.) */
        if (mgend != NEUTRAL
            && (tmpmons.female == BOOL_RANDOM || is_female(&mons[tmpmons.id])
                || is_male(&mons[tmpmons.id])))
            tmpmons.female = mgend;
        /* safety net - if find_montype did not find a gender for this species
         * (should cause a lua error anyway) */
        if (tmpmons.female == BOOL_RANDOM)
            tmpmons.female = 0;

        tmpmons.class = get_table_monclass(L);

        lua_getfield(L, 1, "inventory");
        if (!lua_isnil(L, -1)) {
            tmpmons.has_invent = 1;
        }
    }

    if (mx == -1 && my == -1)
        tmpmons.coord = SP_COORD_PACK_RANDOM(0);
    else
        tmpmons.coord = SP_COORD_PACK(mx, my);

    if (tmpmons.id != NON_PM && tmpmons.class == -1)
        tmpmons.class = def_monsyms[(int) mons[tmpmons.id].mlet].sym;

    create_monster(&tmpmons, g.coder->croom);

    if (tmpmons.has_invent && lua_type(L, -1) == LUA_TFUNCTION) {
        lua_remove(L, -2);
        lua_call(L, 0, 0);
        spo_end_moninvent();
    } else
        lua_pop(L, 1);

    Free(tmpmons.name.str);
    Free(tmpmons.appear_as.str);

    return 0;
}

/* the hash key 'name' is an integer or "random",
   or if not existent, also return rndval */
static lua_Integer
get_table_int_or_random(lua_State *L, const char *name, int rndval)
{
    lua_Integer ret;
    char buf[BUFSZ];

    lua_getfield(L, 1, name);
    if (lua_type(L, -1) == LUA_TNIL) {
        lua_pop(L, 1);
        return rndval;
    }
    if (!lua_isnumber(L, -1)) {
        const char *tmp = lua_tostring(L, -1);

        if (tmp && !strcmpi("random", tmp)) {
            lua_pop(L, 1);
            return rndval;
        }
        Sprintf(buf, "Expected integer or \"random\" for \"%s\", got ", name);
        if (tmp)
            Sprintf(eos(buf), "\"%s\"", tmp);
        else
            Strcat(buf, "<Null>");
        nhl_error(L, buf);
        lua_pop(L, 1);
        return 0;
    }
    ret = luaL_optinteger(L, -1, rndval);
    lua_pop(L, 1);
    return ret;
}

static int
get_table_buc(lua_State *L)
{
    static const char *const bucs[] = {
        "random", "blessed", "uncursed", "cursed",
        "not-cursed", "not-uncursed", "not-blessed", NULL,
    };
    static const int bucs2i[] = { 0, 1, 2, 3, 4, 5, 6, 0 };
    int curse_state = bucs2i[get_table_option(L, "buc", "random", bucs)];

    return curse_state;
}

static int
get_table_objclass(lua_State *L)
{
    char *s = get_table_str_opt(L, "class", NULL);
    int ret = -1;

    if (s && strlen(s) == 1)
        ret = (int) *s;
    Free(s);
    return ret;
}

static int
find_objtype(lua_State *L, const char *s)
{
    if (s) {
        int i;
        const char *objname;

        /* find by object name */
        for (i = 0; i < NUM_OBJECTS; i++) {
            objname = OBJ_NAME(objects[i]);
            if (objname && !strcmpi(s, objname))
                return i;
        }

        /*
         * FIXME:
         *  If the file specifies "orange potion", the actual object
         *  description is just "orange" and won't match.  [There's a
         *  reason that wish handling is insanely complicated.]  And
         *  even if that gets fixed, if the file specifies "gray stone"
         *  it will start matching but would always pick the first one.
         *
         *  "orange potion" is an unlikely thing to have in a special
         *  level description but "gray stone" is not....
         */

        /* find by object description */
        for (i = 0; i < NUM_OBJECTS; i++) {
            objname = OBJ_DESCR(objects[i]);
            if (objname && !strcmpi(s, objname))
                return i;
        }

        nhl_error(L, "Unknown object id");
    }
    return STRANGE_OBJECT;
}

static int
get_table_objtype(lua_State *L)
{
    char *s = get_table_str_opt(L, "id", NULL);
    int ret = find_objtype(L, s);

    Free(s);
    return ret;
}

/* object(); */
/* object("sack"); */
/* object("scimitar", 6,7); */
/* object("scimitar", {6,7}); */
/* object({ class = "%" }); */
/* object({ id = "boulder", x = 03, y = 12}); */
/* object({ id = "boulder", coord = {03,12} }); */
int
lspo_object(lua_State *L)
{
    static object zeroobject = { DUMMY };
#if 0
    int nparams = 0;
#endif
    long quancnt;
    object tmpobj;
    lua_Integer ox = -1, oy = -1;
    int argc = lua_gettop(L);
    int maybe_contents = 0;

    create_des_coder();

    tmpobj = zeroobject;
    tmpobj.name.str = (char *) 0;
    tmpobj.spe = -127;
    tmpobj.quan = -1;
    tmpobj.trapped = -1;
    tmpobj.corpsenm = NON_PM;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        const char *paramstr = luaL_checkstring(L, 1);

        if (strlen(paramstr) == 1) {
            tmpobj.class = *paramstr;
            tmpobj.id = STRANGE_OBJECT;
        } else {
            tmpobj.class = -1;
            tmpobj.id = find_objtype(L, paramstr);
        }
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
               && lua_type(L, 2) == LUA_TTABLE) {
        const char *paramstr = luaL_checkstring(L, 1);

        (void) get_coord(L, 2, &ox, &oy);

        if (strlen(paramstr) == 1) {
            tmpobj.class = *paramstr;
            tmpobj.id = STRANGE_OBJECT;
        } else {
            tmpobj.class = -1;
            tmpobj.id = find_objtype(L, paramstr);
        }
    } else if (argc == 3 && lua_type(L, 2) == LUA_TNUMBER
               && lua_type(L, 3) == LUA_TNUMBER) {
        const char *paramstr = luaL_checkstring(L, 1);

        ox = luaL_checkinteger(L, 2);
        oy = luaL_checkinteger(L, 3);

        if (strlen(paramstr) == 1) {
            tmpobj.class = *paramstr;
            tmpobj.id = STRANGE_OBJECT;
        } else {
            tmpobj.class = -1;
            tmpobj.id = find_objtype(L, paramstr);
        }
    } else {
        lcheck_param_table(L);

        tmpobj.spe = get_table_int_or_random(L, "spe", -127);
        tmpobj.curse_state = get_table_buc(L);
        tmpobj.corpsenm = NON_PM;
        tmpobj.name.str = get_table_str_opt(L, "name", (char *) 0);
        tmpobj.quan = get_table_int_or_random(L, "quantity", -1);
        tmpobj.buried = get_table_boolean_opt(L, "buried", 0);
        tmpobj.lit = get_table_boolean_opt(L, "lit", 0);
        tmpobj.eroded = get_table_int_opt(L, "eroded", 0);
        tmpobj.locked = get_table_boolean_opt(L, "locked", 0);
        tmpobj.trapped = get_table_int_opt(L, "trapped", -1);
        tmpobj.recharged = get_table_int_opt(L, "recharged", 0);
        tmpobj.greased = get_table_boolean_opt(L, "greased", 0);
        tmpobj.broken = get_table_boolean_opt(L, "broken", 0);
        tmpobj.achievement = get_table_boolean_opt(L, "achievement", 0);

        get_table_xy_or_coord(L, &ox, &oy);

        tmpobj.id = get_table_objtype(L);
        tmpobj.class = get_table_objclass(L);
        maybe_contents = 1;
    }

    if (ox == -1 && oy == -1)
        tmpobj.coord = SP_COORD_PACK_RANDOM(0);
    else
        tmpobj.coord = SP_COORD_PACK(ox, oy);

    if (tmpobj.class == -1 && tmpobj.id > STRANGE_OBJECT)
        tmpobj.class = objects[tmpobj.id].oc_class;
    else if (tmpobj.class > -1 && tmpobj.id == STRANGE_OBJECT)
        tmpobj.id = -1;

    if (tmpobj.id == STATUE || tmpobj.id == EGG
        || tmpobj.id == CORPSE || tmpobj.id == TIN
        || tmpobj.id == FIGURINE) {
        struct permonst *pm = NULL;
        int i;
        char *montype = get_table_str_opt(L, "montype", NULL);

        if (montype) {
            if (strlen(montype) == 1
                && def_char_to_monclass(*montype) != MAXMCLASSES) {
                pm = mkclass(def_char_to_monclass(*montype),
                             G_NOGEN | G_IGNORE);
            } else {
                for (i = LOW_PM; i < NUMMONS; i++)
                    if (!strcmpi(mons[i].pmnames[NEUTRAL], montype)
                        || (mons[i].pmnames[MALE] != 0
                            && !strcmpi(mons[i].pmnames[MALE], montype))
                        || (mons[i].pmnames[FEMALE] != 0
                            && !strcmpi(mons[i].pmnames[FEMALE], montype))) {
                        pm = &mons[i];
                        break;
                    }
            }
            free((genericptr_t) montype);
            if (pm)
                tmpobj.corpsenm = monsndx(pm);
            else
                nhl_error(L, "Unknown montype");
        }
        if (tmpobj.id == STATUE || tmpobj.id == CORPSE) {
            int lflags = 0;

            if (get_table_boolean_opt(L, "historic", 0))
                lflags |= CORPSTAT_HISTORIC;
            if (get_table_boolean_opt(L, "male", 0))
                lflags |= CORPSTAT_MALE;
            if (get_table_boolean_opt(L, "female", 0))
                lflags |= CORPSTAT_FEMALE;
            tmpobj.spe = lflags;
        } else if (tmpobj.id == EGG) {
            tmpobj.spe = get_table_boolean_opt(L, "laid_by_you", 0) ? 1 : 0;
        } else {
            tmpobj.spe = 0;
        }
    }

    quancnt = (tmpobj.id > STRANGE_OBJECT) ? tmpobj.quan : 0;

    if (container_idx)
        tmpobj.containment |= SP_OBJ_CONTENT;

    if (maybe_contents) {
        lua_getfield(L, 1, "contents");
        if (!lua_isnil(L, -1))
            tmpobj.containment |= SP_OBJ_CONTAINER;
    }

    do {
        create_object(&tmpobj, g.coder->croom);
        quancnt--;
    } while ((quancnt > 0) && ((tmpobj.id > STRANGE_OBJECT)
                               && !objects[tmpobj.id].oc_merge));

    if (lua_type(L, -1) == LUA_TFUNCTION) {
        lua_remove(L, -2);
        lua_call(L, 0, 0);
    } else
        lua_pop(L, 1);

    if ((tmpobj.containment & SP_OBJ_CONTAINER) != 0)
        spo_pop_container();

    Free(tmpobj.name.str);

    return 0;
}

/* level_flags("noteleport", "mazelevel", ... ); */
int
lspo_level_flags(lua_State *L)
{
    int argc = lua_gettop(L);
    int i;

    create_des_coder();

    if (argc < 1)
        nhl_error(L, "expected string params");

    for (i = 1; i <= argc; i++) {
        const char *s = luaL_checkstring(L, i);

        if (!strcmpi(s, "noteleport"))
            g.level.flags.noteleport = 1;
        else if (!strcmpi(s, "hardfloor"))
            g.level.flags.hardfloor = 1;
        else if (!strcmpi(s, "nommap"))
            g.level.flags.nommap = 1;
        else if (!strcmpi(s, "shortsighted"))
            g.level.flags.shortsighted = 1;
        else if (!strcmpi(s, "arboreal"))
            g.level.flags.arboreal = 1;
        else if (!strcmpi(s, "mazelevel"))
            g.level.flags.is_maze_lev = 1;
        else if (!strcmpi(s, "shroud"))
            g.level.flags.hero_memory = 1;
        else if (!strcmpi(s, "graveyard"))
            g.level.flags.graveyard = 1;
        else if (!strcmpi(s, "icedpools"))
            icedpools = 1;
        else if (!strcmpi(s, "corrmaze"))
            g.level.flags.corrmaze = 1;
        else if (!strcmpi(s, "premapped"))
            g.coder->premapped = 1;
        else if (!strcmpi(s, "solidify"))
            g.coder->solidify = 1;
        else if (!strcmpi(s, "inaccessibles"))
            g.coder->check_inaccessibles = 1;
        else if (!strcmpi(s, "noflipx"))
            g.coder->allow_flips &= ~2;
        else if (!strcmpi(s, "noflipy"))
            g.coder->allow_flips &= ~1;
        else if (!strcmpi(s, "noflip"))
            g.coder->allow_flips = 0;
        else {
            char buf[BUFSZ];
            Sprintf(buf, "Unknown level flag %s", s);
            nhl_error(L, buf);
        }
    }

    return 0;
}

/* level_init({ style = "solidfill", fg = " " }); */
/* level_init({ style = "mines", fg = ".", bg = "}",
                smoothed=true, joined=true, lit=0 }) */
int
lspo_level_init(lua_State *L)
{
    static const char *const initstyles[] = {
        "solidfill", "mazegrid", "maze", "rogue", "mines", "swamp", NULL
    };
    static const int initstyles2i[] = {
        LVLINIT_SOLIDFILL, LVLINIT_MAZEGRID, LVLINIT_MAZE, LVLINIT_ROGUE,
        LVLINIT_MINES, LVLINIT_SWAMP, 0
    };
    lev_init init_lev;

    create_des_coder();

    lcheck_param_table(L);

    splev_init_present = TRUE;

    init_lev.init_style
        = initstyles2i[get_table_option(L, "style", "solidfill", initstyles)];
    init_lev.fg = get_table_mapchr_opt(L, "fg", ROOM);
    init_lev.bg = get_table_mapchr_opt(L, "bg", INVALID_TYPE);
    init_lev.smoothed = get_table_boolean_opt(L, "smoothed", FALSE);
    init_lev.joined = get_table_boolean_opt(L, "joined", FALSE);
    init_lev.lit = get_table_boolean_opt(L, "lit", BOOL_RANDOM);
    init_lev.walled = get_table_boolean_opt(L, "walled", FALSE);
    init_lev.filling = get_table_mapchr_opt(L, "filling", init_lev.fg);
    init_lev.corrwid = get_table_int_opt(L, "corrwid", -1);
    init_lev.wallthick = get_table_int_opt(L, "wallthick", -1);
    init_lev.rm_deadends = !get_table_boolean_opt(L, "deadends", TRUE);

    g.coder->lvl_is_joined = init_lev.joined;

    if (init_lev.bg == INVALID_TYPE)
        init_lev.bg = (init_lev.init_style == LVLINIT_SWAMP) ? MOAT : STONE;

    splev_initlev(&init_lev);

    return 0;
}

/* engraving({ x = 1, y = 1, type="burn", text="Foo" }); */
/* engraving({ coord={1, 1}, type="burn", text="Foo" }); */
/* engraving({x,y}, "engrave", "Foo"); */
int
lspo_engraving(lua_State *L)
{
    static const char *const engrtypes[] = {
        "dust", "engrave", "burn", "mark", "blood", NULL
    };
    static const int engrtypes2i[] = {
        DUST, ENGRAVE, BURN, MARK, ENGR_BLOOD, 0
    };
    int etyp = DUST;
    char *txt = (char *) 0;
    long ecoord;
    xchar x = -1, y = -1;
    int argc = lua_gettop(L);

    create_des_coder();

    if (argc == 1) {
        lua_Integer ex, ey;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &ex, &ey);
        x = ex;
        y = ey;
        etyp = engrtypes2i[get_table_option(L, "type", "engrave", engrtypes)];
        txt = get_table_str(L, "text");
    } else if (argc == 3) {
        lua_Integer ex, ey;
        (void) get_coord(L, 1, &ex, &ey);
        x = ex;
        y = ey;
        etyp = engrtypes2i[luaL_checkoption(L, 2, "engrave", engrtypes)];
        txt = dupstr(luaL_checkstring(L, 3));
    } else {
        nhl_error(L, "Wrong parameters");
    }

    if (x == -1 && y == -1)
        ecoord = SP_COORD_PACK_RANDOM(0);
    else
        ecoord = SP_COORD_PACK(x, y);

    get_location_coord(&x, &y, DRY, g.coder->croom, ecoord);
    make_engr_at(x, y, txt, 0L, etyp);
    Free(txt);
    return 0;
}

int
lspo_mineralize(lua_State *L)
{
    int gem_prob, gold_prob, kelp_moat, kelp_pool;

    create_des_coder();

    lcheck_param_table(L);
    gem_prob = get_table_int_opt(L, "gem_prob", 0);
    gold_prob = get_table_int_opt(L, "gold_prob", 0);
    kelp_moat = get_table_int_opt(L, "kelp_moat", 0);
    kelp_pool = get_table_int_opt(L, "kelp_pool", 0);

    mineralize(kelp_pool, kelp_moat, gold_prob, gem_prob, TRUE);

    return 0;
}

static const struct {
    const char *name;
    int type;
} room_types[] = {
    { "ordinary", OROOM },
    { "themed", THEMEROOM },
    { "throne", COURT },
    { "swamp", SWAMP },
    { "vault", VAULT },
    { "beehive", BEEHIVE },
    { "morgue", MORGUE },
    { "barracks", BARRACKS },
    { "zoo", ZOO },
    { "delphi", DELPHI },
    { "temple", TEMPLE },
    { "anthole", ANTHOLE },
    { "cocknest", COCKNEST },
    { "leprehall", LEPREHALL },
    { "shop", SHOPBASE },
    { "armor shop", ARMORSHOP },
    { "scroll shop", SCROLLSHOP },
    { "potion shop", POTIONSHOP },
    { "weapon shop", WEAPONSHOP },
    { "food shop", FOODSHOP },
    { "ring shop", RINGSHOP },
    { "wand shop", WANDSHOP },
    { "tool shop", TOOLSHOP },
    { "book shop", BOOKSHOP },
    { "health food shop", FODDERSHOP },
    { "candle shop", CANDLESHOP },
    { 0, 0 }
};

static const char *
get_mkroom_name(int rtype)
{
    int i;

    for (i = 0; room_types[i].name; i++)
        if (room_types[i].type == rtype)
            return room_types[i].name;
    return NULL;
}

static int
get_table_roomtype_opt(lua_State *L, const char *name, int defval)
{
    char *roomstr = get_table_str_opt(L, name, emptystr);
    int i, res = defval;

    if (roomstr && *roomstr) {
        for (i = 0; room_types[i].name; i++)
            if (!strcmpi(roomstr, room_types[i].name)) {
                res = room_types[i].type;
                break;
            }
        if (!room_types[i].name)
            impossible("Unknown room type '%s'", roomstr);
    }
    Free(roomstr);
    return res;
}

/* room({ type="ordinary", lit=1, x=3,y=3, xalign="center",yalign="center", w=11,h=9 }); */
/* room({ lit=1, coord={3,3}, xalign="center",yalign="center", w=11,h=9 }); */
/* room({ coord={3,3}, xalign="center",yalign="center", w=11,h=9, contents=function(room) ... end }); */
int
lspo_room(lua_State *L)
{
    create_des_coder();

    if (g.in_mk_themerooms && g.themeroom_failed)
        return 0;

    lcheck_param_table(L);

    if (g.coder->n_subroom > MAX_NESTED_ROOMS) {
        panic("Too deeply nested rooms?!");
    } else {
        static const char *const left_or_right[] = {
            "left", "half-left", "center", "half-right", "right",
            "none", "random", NULL
        };
        static const int l_or_r2i[] = {
            LEFT, H_LEFT, CENTER, H_RIGHT, RIGHT, -1, -1, -1
        };
        static const char *const top_or_bot[] = {
            "top", "center", "bottom", "none", "random", NULL
        };
        static const int t_or_b2i[] = { TOP, CENTER, BOTTOM, -1, -1, -1 };
        room tmproom;
        struct mkroom *tmpcr;
        lua_Integer rx, ry;

        get_table_xy_or_coord(L, &rx, &ry);
        tmproom.x = rx, tmproom.y = ry;
        if ((tmproom.x == -1 || tmproom.y == -1) && tmproom.x != tmproom.y)
            nhl_error(L, "Room must have both x and y");

        tmproom.w = get_table_int_opt(L, "w", -1);
        tmproom.h = get_table_int_opt(L, "h", -1);

        if ((tmproom.w == -1 || tmproom.h == -1) && tmproom.w != tmproom.h)
            nhl_error(L, "Room must have both w and h");

        tmproom.xalign = l_or_r2i[get_table_option(L, "xalign", "random",
                                                   left_or_right)];
        tmproom.yalign = t_or_b2i[get_table_option(L, "yalign", "random",
                                                   top_or_bot)];
        tmproom.rtype = get_table_roomtype_opt(L, "type", OROOM);
        tmproom.chance = get_table_int_opt(L, "chance", 100);
        tmproom.rlit = get_table_int_opt(L, "lit", -1);
        /* theme rooms default to unfilled */
        tmproom.needfill = get_table_int_opt(L, "filled",
                                             g.in_mk_themerooms ? 0 : 1);
        tmproom.joined = get_table_boolean_opt(L, "joined", TRUE);

        if (!g.coder->failed_room[g.coder->n_subroom - 1]) {
            tmpcr = build_room(&tmproom, g.coder->croom);
            if (tmpcr) {
                g.coder->tmproomlist[g.coder->n_subroom] = tmpcr;
                g.coder->failed_room[g.coder->n_subroom] = FALSE;
                g.coder->n_subroom++;
                update_croom();
                lua_getfield(L, 1, "contents");
                if (lua_type(L, -1) == LUA_TFUNCTION) {
                    lua_remove(L, -2);
                    l_push_mkroom_table(L, tmpcr);
                    lua_call(L, 1, 0);
                } else
                    lua_pop(L, 1);
                spo_endroom(g.coder);
                add_doors_to_room(tmpcr);
                return 0;
            }
            if (g.in_mk_themerooms)
                g.themeroom_failed = TRUE;
        } /* failed to create parent room, so fail this too */
    }
    g.coder->tmproomlist[g.coder->n_subroom] = (struct mkroom *) 0;
    g.coder->failed_room[g.coder->n_subroom] = TRUE;
    g.coder->n_subroom++;
    update_croom();
    spo_endroom(g.coder);
    if (g.in_mk_themerooms)
        g.themeroom_failed = TRUE;

    return 0;
}

static void
spo_endroom(struct sp_coder* coder UNUSED)
{
    if (g.coder->n_subroom > 1) {
        g.coder->n_subroom--;
        g.coder->tmproomlist[g.coder->n_subroom] = NULL;
        g.coder->failed_room[g.coder->n_subroom] = TRUE;
    } else {
        /* no subroom, get out of top-level room */
        /* Need to ensure xstart/ystart/xsize/ysize have something sensible,
           in case there's some stuff to be created outside the outermost
           room, and there's no MAP. */
        if (g.xsize <= 1 && g.ysize <= 1) {
            g.xstart = 1;
            g.ystart = 0;
            g.xsize = COLNO - 1;
            g.ysize = ROWNO;
        }
    }
    update_croom();
}

/* callback for is_ok_location.
   stairs generated at random location shouldn't overwrite special terrain */
static boolean
good_stair_loc(xchar x, xchar y)
{
    schar typ = levl[x][y].typ;

    return (typ == ROOM || typ == CORR || typ == ICE);
}

static int
l_create_stairway(lua_State *L, boolean using_ladder)
{
    static const char *const stairdirs[] = { "down", "up", NULL };
    static const int stairdirs2i[] = { 0, 1 };
    int argc = lua_gettop(L);
    xchar x = -1, y = -1;
    struct trap *badtrap;

    long scoord;
    int up = 0; /* default is down */
    int ltype = lua_type(L, 1);

    create_des_coder();

    if (argc == 1 && ltype == LUA_TTABLE) {
        lua_Integer ax = -1, ay = -1;
        lcheck_param_table(L);
        get_table_xy_or_coord(L, &ax, &ay);
        up = stairdirs2i[get_table_option(L, "dir", "down", stairdirs)];
        x = ax;
        y = ay;
    } else {
        int ix = -1, iy = -1;
        if (argc > 0 && ltype == LUA_TSTRING) {
            up = stairdirs2i[luaL_checkoption(L, 1, "down", stairdirs)];
            lua_remove(L, 1);
        }
        nhl_get_xy_params(L, &ix, &iy);
        x = ix;
        y = iy;
    }

    if (x == -1 && y == -1) {
        set_ok_location_func(good_stair_loc);
        scoord = SP_COORD_PACK_RANDOM(0);
    } else
        scoord = SP_COORD_PACK(x, y);

    get_location_coord(&x, &y, DRY, g.coder->croom, scoord);
    set_ok_location_func(NULL);
    if ((badtrap = t_at(x, y)) != 0)
        deltrap(badtrap);
    SpLev_Map[x][y] = 1;

    if (using_ladder) {
        levl[x][y].typ = LADDER;
        if (up) {
            d_level dest;

            dest.dnum = u.uz.dnum;
            dest.dlevel = u.uz.dlevel - 1;
            stairway_add(x, y, TRUE, TRUE, &dest);
            levl[x][y].ladder = LA_UP;
        } else {
            d_level dest;

            dest.dnum = u.uz.dnum;
            dest.dlevel = u.uz.dlevel + 1;
            stairway_add(x, y, FALSE, TRUE, &dest);
            levl[x][y].ladder = LA_DOWN;
        }
    } else {
        mkstairs(x, y, (char) up, g.coder->croom,
                 !(scoord & SP_COORD_IS_RANDOM));
    }
    return 0;
}

/* stair("up"); */
/* stair({ dir = "down" }); */
/* stair({ dir = "down", x = 4, y = 7 }); */
/* stair({ dir = "down", coord = {x,y} }); */
/* stair("down", 4, 7); */
/* TODO: stair(selection, "down"); */
/* TODO: stair("up", {x,y}); */
int
lspo_stair(lua_State *L)
{
    return l_create_stairway(L, FALSE);
}

/* ladder("down"); */
/* ladder("up", 6,10); */
/* ladder({ x=11, y=05, dir="down" }); */
int
lspo_ladder(lua_State *L)
{
    return l_create_stairway(L, TRUE);
}

/* grave(); */
/* grave(x,y, "text"); */
/* grave({ x = 1, y = 1 }); */
/* grave({ x = 1, y = 1, text = "Foo" }); */
/* grave({ coord = {1, 1}, text = "Foo" }); */
int
lspo_grave(lua_State *L)
{
    int argc = lua_gettop(L);
    xchar x, y;
    long scoord;
    lua_Integer ax,ay;
    char *txt;

    create_des_coder();

    if (argc == 3) {
        x = ax = luaL_checkinteger(L, 1);
        y = ay = luaL_checkinteger(L, 2);
        txt = dupstr(luaL_checkstring(L, 3));
    } else {
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &ax, &ay);
        x = ax, y = ay;
        txt = get_table_str_opt(L, "text", NULL);
    }

    if (x == -1 && y == -1)
        scoord = SP_COORD_PACK_RANDOM(0);
    else
        scoord = SP_COORD_PACK(ax, ay);

    get_location_coord(&x, &y, DRY, g.coder->croom, scoord);

    if (isok(x, y) && !t_at(x, y)) {
        levl[x][y].typ = GRAVE;
        make_grave(x, y, txt); /* note: 'txt' might be Null */
    }
    Free(txt);
    return 0;
}

/* altar({ x=NN, y=NN, align=ALIGNMENT, type=SHRINE }); */
/* des.altar({ coord = {5, 10}, align="noalign", type="altar" }); */
int
lspo_altar(lua_State *L)
{
    static const char *const shrines[] = {
        "altar", "shrine", "sanctum", NULL
    };
    static const int shrines2i[] = { 0, 1, 2, 0 };

    altar tmpaltar;

    lua_Integer x, y;
    long acoord;
    int shrine;
    int al;

    create_des_coder();

    lcheck_param_table(L);

    get_table_xy_or_coord(L, &x, &y);

    al = get_table_align(L);
    shrine = shrines2i[get_table_option(L, "type", "altar", shrines)];

    if (x == -1 && y == -1)
        acoord = SP_COORD_PACK_RANDOM(0);
    else
        acoord = SP_COORD_PACK(x, y);

    tmpaltar.coord = acoord;
    tmpaltar.sp_amask = al;
    tmpaltar.shrine = shrine;

    create_altar(&tmpaltar, g.coder->croom);

    return 0;
}

static const struct {
    const char *name;
    int type;
} trap_types[] = { { "arrow", ARROW_TRAP },
                   { "dart", DART_TRAP },
                   { "falling rock", ROCKTRAP },
                   { "board", SQKY_BOARD },
                   { "bear", BEAR_TRAP },
                   { "land mine", LANDMINE },
                   { "rolling boulder", ROLLING_BOULDER_TRAP },
                   { "sleep gas", SLP_GAS_TRAP },
                   { "rust", RUST_TRAP },
                   { "fire", FIRE_TRAP },
                   { "pit", PIT },
                   { "spiked pit", SPIKED_PIT },
                   { "hole", HOLE },
                   { "trap door", TRAPDOOR },
                   { "teleport", TELEP_TRAP },
                   { "level teleport", LEVEL_TELEP },
                   { "magic portal", MAGIC_PORTAL },
                   { "web", WEB },
                   { "statue", STATUE_TRAP },
                   { "magic", MAGIC_TRAP },
                   { "anti magic", ANTI_MAGIC },
                   { "polymorph", POLY_TRAP },
                   { "vibrating square", VIBRATING_SQUARE },
                   { "random", -1 },
                   { 0, NO_TRAP } };

static int
get_table_traptype_opt(lua_State *L, const char *name, int defval)
{
    char *trapstr = get_table_str_opt(L, name, emptystr);
    int i, res = defval;

    if (trapstr && *trapstr) {
        for (i = 0; trap_types[i].name; i++)
            if (!strcmpi(trapstr, trap_types[i].name)) {
                res = trap_types[i].type;
                break;
            }
    }
    Free(trapstr);
    return res;
}

const char *
get_trapname_bytype(int ttyp)
{
    int i;

    for (i = 0; trap_types[i].name; i++)
        if (ttyp == trap_types[i].type)
            return trap_types[i].name;

    return NULL;
}

static int
get_traptype_byname(const char *trapname)
{
    int i;

    for (i = 0; trap_types[i].name; i++)
        if (!strcmpi(trapname, trap_types[i].name))
            return trap_types[i].type;

    return NO_TRAP;
}

/* trap({ type = "hole", x = 1, y = 1 }); */
/* trap({ type = "web", spider_on_web = 0 }); */
/* trap("hole", 3, 4); */
/* trap("level teleport", {5, 8}); */
/* trap("rust") */
/* trap(); */
int
lspo_trap(lua_State *L)
{
    spltrap tmptrap;
    lua_Integer x, y;
    int argc = lua_gettop(L);

    create_des_coder();

    tmptrap.spider_on_web = TRUE;
    tmptrap.seen = FALSE;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        const char *trapstr = luaL_checkstring(L, 1);

        tmptrap.type = get_traptype_byname(trapstr);
        x = y = -1;
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
               && lua_type(L, 2) == LUA_TTABLE) {
        const char *trapstr = luaL_checkstring(L, 1);

        tmptrap.type = get_traptype_byname(trapstr);
        (void) get_coord(L, 2, &x, &y);
    } else if (argc == 3) {
        const char *trapstr = luaL_checkstring(L, 1);

        tmptrap.type = get_traptype_byname(trapstr);
        x = luaL_checkinteger(L, 2);
        y = luaL_checkinteger(L, 3);
    } else {
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &x, &y);
        tmptrap.type = get_table_traptype_opt(L, "type", -1);
        tmptrap.spider_on_web = get_table_boolean_opt(L, "spider_on_web", 1);
        tmptrap.seen = get_table_boolean_opt(L, "seen", FALSE);

        lua_getfield(L, -1, "launchfrom");
        if (lua_type(L, -1) == LUA_TTABLE) {
            lua_Integer lx = -1, ly = -1;

            (void) get_coord(L, -1, &lx, &ly);
            lua_pop(L, 1);
            g.launchplace.x = lx;
            g.launchplace.y = ly;
        }
    }

    if (tmptrap.type == NO_TRAP)
        nhl_error(L, "Unknown trap type");

    if (x == -1 && y == -1)
        tmptrap.coord = SP_COORD_PACK_RANDOM(0);
    else
        tmptrap.coord = SP_COORD_PACK(x, y);

    create_trap(&tmptrap, g.coder->croom);
    g.launchplace.x = g.launchplace.y = 0;

    return 0;
}

/* gold(500, 3,5); */
/* gold(500, {5, 6}); */
/* gold({ amount = 500, x = 2, y = 5 });*/
/* gold({ amount = 500, coord = {2, 5} });*/
/* gold(); */
int
lspo_gold(lua_State *L)
{
    int argc = lua_gettop(L);
    xchar x, y;
    long amount;
    long gcoord;
    lua_Integer gx, gy;

    create_des_coder();

    if (argc == 3) {
        amount = luaL_checkinteger(L, 1);
        x = gx = luaL_checkinteger(L, 2);
        y = gy = luaL_checkinteger(L, 2);
    } else if (argc == 2 && lua_type(L, 2) == LUA_TTABLE) {
        amount = luaL_checkinteger(L, 1);
        (void) get_coord(L, 2, &gx, &gy);
        x = gx;
        y = gy;
    } else if (argc == 0 || (argc == 1 && lua_type(L, 1) == LUA_TTABLE)) {
        lcheck_param_table(L);

        amount = get_table_int_opt(L, "amount", -1);
        get_table_xy_or_coord(L, &gx, &gy);
        x = gx, y = gy;
    } else {
        nhl_error(L, "Wrong parameters");
        return 0;
    }

    if (x == -1 && y == -1)
        gcoord = SP_COORD_PACK_RANDOM(0);
    else
        gcoord = SP_COORD_PACK(gx, gy);

    get_location_coord(&x, &y, DRY, g.coder->croom, gcoord);
    if (amount < 0)
        amount = rnd(200);
    mkgold(amount, x, y);

    return 0;
}

/* corridor({ srcroom=1, srcdoor=2, srcwall="north", destroom=2, destdoor=1, destwall="west" });*/
int
lspo_corridor(lua_State *L)
{
    static const char *const walldirs[] = {
        "all", "random", "north", "west", "east", "south", NULL
    };
    static const int walldirs2i[] = {
        W_ANY, W_RANDOM, W_NORTH, W_WEST, W_EAST, W_SOUTH, 0
    };
    corridor tc;

    create_des_coder();

    lcheck_param_table(L);

    tc.src.room = get_table_int(L, "srcroom");
    tc.src.door = get_table_int(L, "srcdoor");
    tc.src.wall = walldirs2i[get_table_option(L, "srcwall", "all", walldirs)];
    tc.dest.room = get_table_int(L, "destroom");
    tc.dest.door = get_table_int(L, "destdoor");
    tc.dest.wall = walldirs2i[get_table_option(L, "destwall", "all", walldirs)];

    create_corridor(&tc);

    return 0;
}

/* random_corridors(); */
int
lspo_random_corridors(lua_State *L UNUSED)
{
    corridor tc;

    create_des_coder();

    tc.src.room = -1;
    tc.src.door = -1;
    tc.src.wall = -1;
    tc.dest.room = -1;
    tc.dest.door = -1;
    tc.dest.wall = -1;

    create_corridor(&tc);

    return 0;
}

/* selection */
struct selectionvar *
selection_new(void)
{
    struct selectionvar *tmps = (struct selectionvar *) alloc(sizeof *tmps);

    tmps->wid = COLNO;
    tmps->hei = ROWNO;
    tmps->map = (char *) alloc((COLNO * ROWNO) + 1);
    (void) memset(tmps->map, 1, (COLNO * ROWNO));
    tmps->map[(COLNO * ROWNO)] = '\0';

    return tmps;
}

void
selection_free(struct selectionvar* sel, boolean freesel)
{
    if (sel) {
        Free(sel->map);
        sel->map = NULL;
        if (freesel)
            free((genericptr_t) sel);
        else
            sel->wid = sel->hei = 0;
    }
}

struct selectionvar *
selection_clone(struct selectionvar* sel)
{
    struct selectionvar *tmps = (struct selectionvar *) alloc(sizeof *tmps);

    tmps->wid = sel->wid;
    tmps->hei = sel->hei;
    tmps->map = dupstr(sel->map);

    return tmps;
}

xchar
selection_getpoint(int x, int y, struct selectionvar* sel)
{
    if (!sel || !sel->map)
        return 0;
    if (x < 0 || y < 0 || x >= sel->wid || y >= sel->hei)
        return 0;

    return (sel->map[sel->wid * y + x] - 1);
}

void
selection_setpoint(int x, int y, struct selectionvar* sel, xchar c)
{
    if (!sel || !sel->map)
        return;
    if (x < 0 || y < 0 || x >= sel->wid || y >= sel->hei)
        return;

    sel->map[sel->wid * y + x] = (char) (c + 1);
}

struct selectionvar *
selection_not(struct selectionvar* s)
{
    int x, y;


    for (x = 0; x < s->wid; x++)
        for (y = 0; y < s->hei; y++)
            selection_setpoint(x, y, s, selection_getpoint(x, y, s) ? 0 : 1);

    return s;
}

struct selectionvar *
selection_filter_mapchar(struct selectionvar* ov,  xchar typ, int lit)
{
    int x, y;
    struct selectionvar *ret = selection_new();

    if (!ov || !ret)
        return NULL;

    for (x = 1; x < ret->wid; x++)
        for (y = 0; y < ret->hei; y++)
            if (selection_getpoint(x, y, ov)
                && match_maptyps(typ, levl[x][y].typ)) {
                switch (lit) {
                default:
                case -2:
                    selection_setpoint(x, y, ret, 1);
                    break;
                case -1:
                    selection_setpoint(x, y, ret, rn2(2));
                    break;
                case 0:
                case 1:
                    if (levl[x][y].lit == (unsigned int) lit)
                        selection_setpoint(x, y, ret, 1);
                    break;
                }
            }
    return ret;
}

void
selection_filter_percent(struct selectionvar* ov, int percent)
{
    int x, y;

    if (!ov)
        return;
    for (x = 0; x < ov->wid; x++)
        for (y = 0; y < ov->hei; y++)
            if (selection_getpoint(x, y, ov) && (rn2(100) >= percent))
                selection_setpoint(x, y, ov, 0);
}

int
selection_rndcoord(struct selectionvar* ov, xchar *x, xchar *y, boolean removeit)
{
    int idx = 0;
    int c;
    int dx, dy;

    for (dx = 1; dx < ov->wid; dx++)
        for (dy = 0; dy < ov->hei; dy++)
            if (selection_getpoint(dx, dy, ov))
                idx++;

    if (idx) {
        c = rn2(idx);
        for (dx = 1; dx < ov->wid; dx++)
            for (dy = 0; dy < ov->hei; dy++)
                if (selection_getpoint(dx, dy, ov)) {
                    if (!c) {
                        *x = dx;
                        *y = dy;
                        if (removeit)
                            selection_setpoint(dx, dy, ov, 0);
                        return 1;
                    }
                    c--;
                }
    }
    *x = *y = -1;
    return 0;
}

/* Choose a single random W_* direction. */
static xchar
random_wdir(void)
{
    static const xchar wdirs[4] = { W_NORTH, W_SOUTH, W_EAST, W_WEST };
    return wdirs[rn2(4)];
}

void
selection_do_grow(struct selectionvar* ov, int dir)
{
    int x, y;
    struct selectionvar *tmp = selection_new();

    if (!ov || !tmp)
        return;

    if (dir == W_RANDOM)
        dir = random_wdir();

    for (x = 1; x < ov->wid; x++)
        for (y = 0; y < ov->hei; y++) {
            /* note:  dir is a mask of multiple directions, but the only
               way to specify diagonals is by including the two adjacent
               orthogonal directions, which effectively specifies three-
               way growth [WEST|NORTH => WEST plus WEST|NORTH plus NORTH] */
            if (((dir & W_WEST) && selection_getpoint(x + 1, y, ov))
                || (((dir & (W_WEST | W_NORTH)) == (W_WEST | W_NORTH))
                    && selection_getpoint(x + 1, y + 1, ov))
                || ((dir & W_NORTH) && selection_getpoint(x, y + 1, ov))
                || (((dir & (W_NORTH | W_EAST)) == (W_NORTH | W_EAST))
                    && selection_getpoint(x - 1, y + 1, ov))
                || ((dir & W_EAST) && selection_getpoint(x - 1, y, ov))
                || (((dir & (W_EAST | W_SOUTH)) == (W_EAST | W_SOUTH))
                    && selection_getpoint(x - 1, y - 1, ov))
                || ((dir & W_SOUTH) && selection_getpoint(x, y - 1, ov))
                ||  (((dir & (W_SOUTH | W_WEST)) == (W_SOUTH | W_WEST))
                     && selection_getpoint(x + 1, y - 1, ov))) {
                selection_setpoint(x, y, tmp, 1);
            }
        }

    for (x = 1; x < ov->wid; x++)
        for (y = 0; y < ov->hei; y++)
            if (selection_getpoint(x, y, tmp))
                selection_setpoint(x, y, ov, 1);

    selection_free(tmp, TRUE);
}

static int (*selection_flood_check_func)(int, int);
static schar floodfillchk_match_under_typ;

void
set_selection_floodfillchk(int (*f)(int, int))
{
    selection_flood_check_func = f;
}

static int
floodfillchk_match_under(int x, int y)
{
    return (floodfillchk_match_under_typ == levl[x][y].typ);
}

void
set_floodfillchk_match_under(xchar typ)
{
    floodfillchk_match_under_typ = typ;
    set_selection_floodfillchk(floodfillchk_match_under);
}

static int
floodfillchk_match_accessible(int x, int y)
{
    return (ACCESSIBLE(levl[x][y].typ)
            || levl[x][y].typ == SDOOR
            || levl[x][y].typ == SCORR);
}

/* check whethere <x,y> is already in xs[],ys[] */
static boolean
sel_flood_havepoint(int x, int y, xchar xs[], xchar ys[], int n)
{
    xchar xx = (xchar) x, yy = (xchar) y;

    while (n > 0) {
        --n;
        if (xs[n] == xx && ys[n] == yy)
            return TRUE;
    }
    return FALSE;
}

void
selection_floodfill(struct selectionvar* ov, int x, int y, boolean diagonals)
{
    struct selectionvar *tmp = selection_new();
#define SEL_FLOOD_STACK (COLNO * ROWNO)
#define SEL_FLOOD(nx, ny) \
    do {                                      \
        if (idx < SEL_FLOOD_STACK) {          \
            dx[idx] = (nx);                   \
            dy[idx] = (ny);                   \
            idx++;                            \
        } else                                \
            panic(floodfill_stack_overrun);   \
    } while (0)
#define SEL_FLOOD_CHKDIR(mx, my, sel) \
    do {                                                        \
        if (isok((mx), (my))                                    \
            && (*selection_flood_check_func)((mx), (my))        \
            && !selection_getpoint((mx), (my), (sel))           \
            && !sel_flood_havepoint((mx), (my), dx, dy, idx))   \
            SEL_FLOOD((mx), (my));                              \
    } while (0)
    static const char floodfill_stack_overrun[] = "floodfill stack overrun";
    int idx = 0;
    xchar dx[SEL_FLOOD_STACK];
    xchar dy[SEL_FLOOD_STACK];

    if (selection_flood_check_func == (int (*)(int, int)) 0) {
        selection_free(tmp, TRUE);
        return;
    }
    SEL_FLOOD(x, y);
    do {
        idx--;
        x = dx[idx];
        y = dy[idx];
        if (isok(x, y)) {
            selection_setpoint(x, y, ov, 1);
            selection_setpoint(x, y, tmp, 1);
        }
        SEL_FLOOD_CHKDIR((x + 1), y, tmp);
        SEL_FLOOD_CHKDIR((x - 1), y, tmp);
        SEL_FLOOD_CHKDIR(x, (y + 1), tmp);
        SEL_FLOOD_CHKDIR(x, (y - 1), tmp);
        if (diagonals) {
            SEL_FLOOD_CHKDIR((x + 1), (y + 1), tmp);
            SEL_FLOOD_CHKDIR((x - 1), (y - 1), tmp);
            SEL_FLOOD_CHKDIR((x - 1), (y + 1), tmp);
            SEL_FLOOD_CHKDIR((x + 1), (y - 1), tmp);
        }
    } while (idx > 0);
#undef SEL_FLOOD
#undef SEL_FLOOD_STACK
#undef SEL_FLOOD_CHKDIR
    selection_free(tmp, TRUE);
}

/* McIlroy's Ellipse Algorithm */
void
selection_do_ellipse(
    struct selectionvar *ov,
    int xc, int yc,
    int a, int b,
    int filled)
{ /* e(x,y) = b^2*x^2 + a^2*y^2 - a^2*b^2 */
    int x = 0, y = b;
    long a2 = (long) a * a, b2 = (long) b * b;
    long crit1 = -(a2 / 4 + a % 2 + b2);
    long crit2 = -(b2 / 4 + b % 2 + a2);
    long crit3 = -(b2 / 4 + b % 2);
    long t = -a2 * y; /* e(x+1/2,y-1/2) - (a^2+b^2)/4 */
    long dxt = 2 * b2 * x, dyt = -2 * a2 * y;
    long d2xt = 2 * b2, d2yt = 2 * a2;
    long width = 1;
    long i;

    if (!ov)
        return;

    filled = !filled;

    if (!filled) {
        while (y >= 0 && x <= a) {
            selection_setpoint(xc + x, yc + y, ov, 1);
            if (x != 0 || y != 0)
                selection_setpoint(xc - x, yc - y, ov, 1);
            if (x != 0 && y != 0) {
                selection_setpoint(xc + x, yc - y, ov, 1);
                selection_setpoint(xc - x, yc + y, ov, 1);
            }
            if (t + b2 * x <= crit1       /* e(x+1,y-1/2) <= 0 */
                || t + a2 * y <= crit3) { /* e(x+1/2,y) <= 0 */
                x++;
                dxt += d2xt;
                t += dxt;
            } else if (t - a2 * y > crit2) { /* e(x+1/2,y-1) > 0 */
                y--;
                dyt += d2yt;
                t += dyt;
            } else {
                x++;
                dxt += d2xt;
                t += dxt;
                y--;
                dyt += d2yt;
                t += dyt;
            }
        }
    } else {
        while (y >= 0 && x <= a) {
            if (t + b2 * x <= crit1       /* e(x+1,y-1/2) <= 0 */
                || t + a2 * y <= crit3) { /* e(x+1/2,y) <= 0 */
                x++;
                dxt += d2xt;
                t += dxt;
                width += 2;
            } else if (t - a2 * y > crit2) { /* e(x+1/2,y-1) > 0 */
                for (i = 0; i < width; i++)
                    selection_setpoint(xc - x + i, yc - y, ov, 1);
                if (y != 0)
                    for (i = 0; i < width; i++)
                        selection_setpoint(xc - x + i, yc + y, ov, 1);
                y--;
                dyt += d2yt;
                t += dyt;
            } else {
                for (i = 0; i < width; i++)
                    selection_setpoint(xc - x + i, yc - y, ov, 1);
                if (y != 0)
                    for (i = 0; i < width; i++)
                        selection_setpoint(xc - x + i, yc + y, ov, 1);
                x++;
                dxt += d2xt;
                t += dxt;
                y--;
                dyt += d2yt;
                t += dyt;
                width += 2;
            }
        }
    }
}

/* distance from line segment (x1,y1, x2,y2) to point (x3,y3) */
static long
line_dist_coord(long x1, long y1, long x2, long y2, long x3, long y3)
{
    long px = x2 - x1;
    long py = y2 - y1;
    long s = px * px + py * py;
    long x, y, dx, dy, dist = 0;
    float lu = 0;

    if (x1 == x2 && y1 == y2)
        return isqrt(dist2(x1, y1, x3, y3));

    lu = ((x3 - x1) * px + (y3 - y1) * py) / (float) s;
    if (lu > 1)
        lu = 1;
    else if (lu < 0)
        lu = 0;

    x = x1 + lu * px;
    y = y1 + lu * py;
    dx = x - x3;
    dy = y - y3;
    dist = isqrt(dx * dx + dy * dy);

    return dist;
}

void
selection_do_gradient(
    struct selectionvar *ov,
    long x, long y,
    long x2,long y2,
    long gtyp,
    long mind, long maxd, long limit)
{
    long dx, dy, dofs;

    if (mind > maxd) {
        long tmp = mind;
        mind = maxd;
        maxd = tmp;
    }

    dofs = maxd - mind;
    if (dofs < 1)
        dofs = 1;

    switch (gtyp) {
    default:
        impossible("Unrecognized gradient type! Defaulting to radial...");
        /* FALLTHRU */
    case SEL_GRADIENT_RADIAL: {
        for (dx = 0; dx < COLNO; dx++)
            for (dy = 0; dy < ROWNO; dy++) {
                long d0 = line_dist_coord(x, y, x2, y2, dx, dy);
                if (d0 >= mind && (!limit || d0 <= maxd)) {
                    if (d0 - mind > rn2(dofs))
                        selection_setpoint(dx, dy, ov, 1);
                }
            }
        break;
    }
    case SEL_GRADIENT_SQUARE: {
        for (dx = 0; dx < COLNO; dx++)
            for (dy = 0; dy < ROWNO; dy++) {
                long d1 = line_dist_coord(x, y, x2, y2, x, dy);
                long d2 = line_dist_coord(x, y, x2, y2, dx, y);
                long d3 = line_dist_coord(x, y, x2, y2, x2, dy);
                long d4 = line_dist_coord(x, y, x2, y2, dx, y2);
                long d5 = line_dist_coord(x, y, x2, y2, dx, dy);
                long d0 = min(d5, min(max(d1, d2), max(d3, d4)));

                if (d0 >= mind && (!limit || d0 <= maxd)) {
                    if (d0 - mind > rn2(dofs))
                        selection_setpoint(dx, dy, ov, 1);
                }
            }
        break;
    } /*case*/
    } /*switch*/
}

/* bresenham line algo */
void
selection_do_line(
    xchar x1, xchar y1,
    xchar x2, xchar y2,
    struct selectionvar *ov)
{
    int d0, dx, dy, ai, bi, xi, yi;

    if (x1 < x2) {
        xi = 1;
        dx = x2 - x1;
    } else {
        xi = -1;
        dx = x1 - x2;
    }
    if (y1 < y2) {
        yi = 1;
        dy = y2 - y1;
    } else {
        yi = -1;
        dy = y1 - y2;
    }

    selection_setpoint(x1, y1, ov, 1);

    if (!dx && !dy) {
        /* single point - already all done */
        ;
    } else if (dx > dy) {
        ai = (dy - dx) * 2;
        bi = dy * 2;
        d0 = bi - dx;
        do {
            if (d0 >= 0) {
                y1 += yi;
                d0 += ai;
            } else
                d0 += bi;
            x1 += xi;
            selection_setpoint(x1, y1, ov, 1);
        } while (x1 != x2);
    } else {
        ai = (dx - dy) * 2;
        bi = dx * 2;
        d0 = bi - dy;
        do {
            if (d0 >= 0) {
                x1 += xi;
                d0 += ai;
            } else
                d0 += bi;
            y1 += yi;
            selection_setpoint(x1, y1, ov, 1);
        } while (y1 != y2);
    }
}

void
selection_do_randline(
    xchar x1, xchar y1,
    xchar x2, xchar y2,
    schar rough,
    schar rec,
    struct selectionvar *ov)
{
    int mx, my;
    int dx, dy;

    if (rec < 1 || (x2 == x1 && y2 == y1))
        return;

    if (rough > max(abs(x2 - x1), abs(y2 - y1)))
        rough = max(abs(x2 - x1), abs(y2 - y1));

    if (rough < 2) {
        mx = ((x1 + x2) / 2);
        my = ((y1 + y2) / 2);
    } else {
        do {
            dx = rn2(rough) - (rough / 2);
            dy = rn2(rough) - (rough / 2);
            mx = ((x1 + x2) / 2) + dx;
            my = ((y1 + y2) / 2) + dy;
        } while ((mx > COLNO - 1 || mx < 0 || my < 0 || my > ROWNO - 1));
    }

    if (!selection_getpoint(mx, my, ov)) {
        selection_setpoint(mx, my, ov, 1);
    }

    rough = (rough * 2) / 3;

    rec--;

    selection_do_randline(x1, y1, mx, my, rough, rec, ov);
    selection_do_randline(mx, my, x2, y2, rough, rec, ov);

    selection_setpoint(x2, y2, ov, 1);
}

static void
selection_iterate(
    struct selectionvar *ov,
    select_iter_func func,
    genericptr_t arg)
{
    int x, y;

    if (!ov)
        return;

    /* yes, this is very naive, but it's not _that_ expensive. */
    for (x = 0; x < ov->wid; x++)
        for (y = 0; y < ov->hei; y++)
            if (isok(x,y) && selection_getpoint(x, y, ov))
                (*func)(x, y, arg);
}

static void
sel_set_ter(int x, int y, genericptr_t arg)
{
    terrain terr;

    terr = *(terrain *) arg;
    if (!set_levltyp_lit(x, y, terr.ter, terr.tlit))
        return;
    /* TODO: move this below into set_levltyp? */
    /* handle doors and secret doors */
    if (levl[x][y].typ == SDOOR || IS_DOOR(levl[x][y].typ)) {
        if (levl[x][y].typ == SDOOR)
            levl[x][y].doormask = D_CLOSED;
        if (x && (IS_WALL(levl[x - 1][y].typ) || levl[x - 1][y].horizontal))
            levl[x][y].horizontal = 1;
    }
}

static void
sel_set_feature(int x, int y, genericptr_t arg)
{
    if (IS_FURNITURE(levl[x][y].typ))
        return;
    levl[x][y].typ = (*(int *) arg);
}

static void
sel_set_door(int dx, int dy, genericptr_t arg)
{
    xchar typ = *(xchar *) arg;
    xchar x = dx, y = dy;

    if (!IS_DOOR(levl[x][y].typ) && levl[x][y].typ != SDOOR)
        levl[x][y].typ = (typ & D_SECRET) ? SDOOR : DOOR;
    if (typ & D_SECRET) {
        typ &= ~D_SECRET;
        if (typ < D_CLOSED)
            typ = D_CLOSED;
    }
    set_door_orientation(x, y); /* set/clear levl[x][y].horizontal */
    levl[x][y].doormask = typ;
    SpLev_Map[x][y] = 1;
}

/* door({ x = 1, y = 1, state = "nodoor" }); */
/* door({ coord = {1, 1}, state = "nodoor" }); */
/* door({ wall = "north", pos = 3, state="secret" }); */
/* door("nodoor", 1, 2); */
int
lspo_door(lua_State *L)
{
    static const char *const doorstates[] = {
        "random", "open", "closed", "locked", "nodoor", "broken",
        "secret", NULL
    };
    static const int doorstates2i[] = {
        -1, D_ISOPEN, D_CLOSED, D_LOCKED, D_NODOOR, D_BROKEN, D_SECRET
    };
    int msk;
    xchar x, y;
    xchar typ;
    int argc = lua_gettop(L);

    create_des_coder();

    if (argc == 3) {
        msk = doorstates2i[luaL_checkoption(L, 1, "random", doorstates)];
        x = luaL_checkinteger(L, 2);
        y = luaL_checkinteger(L, 3);

    } else {
        lua_Integer dx, dy;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &dx, &dy);
        x = dx, y = dy;
        msk = doorstates2i[get_table_option(L, "state", "random", doorstates)];
    }

    typ = (msk == -1) ? rnddoor() : (xchar) msk;

    if (x == -1 && y == -1) {
        static const char *const walldirs[] = {
            "all", "random", "north", "west", "east", "south", NULL
        };
        /* Note that "random" is also W_ANY, because create_door just wants a
         * mask of acceptable walls */
        static const int walldirs2i[] = {
            W_ANY, W_ANY, W_NORTH, W_WEST, W_EAST, W_SOUTH, 0
        };
        room_door tmpd;

        tmpd.secret = (typ == D_SECRET) ? 1 : 0;
        tmpd.mask = msk;
        tmpd.pos = get_table_int_opt(L, "pos", -1);
        tmpd.wall = walldirs2i[get_table_option(L, "wall", "all", walldirs)];

        create_door(&tmpd, g.coder->croom);
    } else {
        /*selection_iterate(sel, sel_set_door, (genericptr_t) &typ);*/
        get_location_coord(&x, &y, ANY_LOC, g.coder->croom,
                           SP_COORD_PACK(x, y));
        if (!isok(x, y))
            nhl_error(L, "door coord not ok");
        sel_set_door(x, y, (genericptr_t) &typ);
    }

    return 0;
}

static void
l_table_getset_feature_flag(
    lua_State *L,
    int x, int y,
    const char *name,
    int flag)
{
    int val = get_table_boolean_opt(L, name, -2);

    if (val != -2) {
        if (val == -1)
            val = rn2(2);
        if (val)
            levl[x][y].flags |= flag;
        else
            levl[x][y].flags &= ~flag;
    }
}

/* convert relative coordinate to map-absolute.
  local ax,ay = nh.abscoord(rx, ry);
  local pt = nh.abscoord({ x = 10, y = 5 });
 */
int
nhl_abs_coord(lua_State *L)
{
    int argc = lua_gettop(L);
    xchar x = -1, y = -1;

    if (argc == 2) {
        x = (xchar) lua_tointeger(L, 1);
        y = (xchar) lua_tointeger(L, 2);
        x += g.xstart;
        y += g.ystart;
        lua_pushinteger(L, x);
        lua_pushinteger(L, y);
        return 2;
    } else if (argc == 1 && lua_type(L, 1) == LUA_TTABLE) {
        x = (xchar) get_table_int(L, "x");
        y = (xchar) get_table_int(L, "y");
        x += g.xstart;
        y += g.ystart;
        lua_newtable(L);
        nhl_add_table_entry_int(L, "x", x);
        nhl_add_table_entry_int(L, "y", y);
        return 1;
    } else {
        nhl_error(L, "nhl_abs_coord: Wrong args");
        return 0;
    }
}

/* feature("fountain", x, y); */
/* feature("fountain", {x,y}); */
/* feature({ type="fountain", x=NN, y=NN }); */
/* feature({ type="fountain", coord={NN, NN} }); */
/* feature({ type="tree", coord={NN, NN}, swarm=true, looted=false }); */
int
lspo_feature(lua_State *L)
{
    static const char *const features[] = { "fountain", "sink", "pool",
                                            "throne", "tree", NULL };
    static const int features2i[] = { FOUNTAIN, SINK, POOL,
                                      THRONE, TREE, STONE };
    xchar x, y;
    int typ;
    int argc = lua_gettop(L);
    boolean can_have_flags = FALSE;

    create_des_coder();

    if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
        && lua_type(L, 2) == LUA_TTABLE) {
        lua_Integer fx, fy;
        typ = features2i[luaL_checkoption(L, 1, NULL, features)];
        (void) get_coord(L, 2, &fx, &fy);
        x = fx;
        y = fy;
    } else if (argc == 3) {
        typ = features2i[luaL_checkoption(L, 1, NULL, features)];
        x = luaL_checkinteger(L, 2);
        y = luaL_checkinteger(L, 3);
    } else {
        lua_Integer fx, fy;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &fx, &fy);
        x = fx, y = fy;
        typ = features2i[get_table_option(L, "type", NULL, features)];
        can_have_flags = TRUE;
    }

    get_location_coord(&x, &y, ANY_LOC, g.coder->croom, SP_COORD_PACK(x, y));

    if (typ == STONE)
        impossible("feature has unknown type param.");
    else
        sel_set_feature(x, y, (genericptr_t) &typ);

    if (levl[x][y].typ != typ || !can_have_flags)
        return 0;

    switch (typ) {
    default:
        break;
    case FOUNTAIN:
        l_table_getset_feature_flag(L, x, y, "looted", F_LOOTED);
        l_table_getset_feature_flag(L, x, y, "warned", F_WARNED);
        break;
    case SINK:
        l_table_getset_feature_flag(L, x, y, "pudding", S_LPUDDING);
        l_table_getset_feature_flag(L, x, y, "dishwasher", S_LDWASHER);
        l_table_getset_feature_flag(L, x, y, "ring", S_LRING);
        break;
    case THRONE:
        l_table_getset_feature_flag(L, x, y, "looted", T_LOOTED);
        break;
    case TREE:
        l_table_getset_feature_flag(L, x, y, "looted", TREE_LOOTED);
        l_table_getset_feature_flag(L, x, y, "swarm", TREE_SWARM);
        break;
    }

    return 0;
}

/*
 * [lit_state: 1 On, 0 Off, -1 random, -2 leave as-is]
 * terrain({ x=NN, y=NN, typ=MAPCHAR, lit=lit_state });
 * terrain({ coord={X, Y}, typ=MAPCHAR, lit=lit_state });
 * terrain({ selection=SELECTION, typ=MAPCHAR, lit=lit_state });
 * terrain( SELECTION, MAPCHAR [, lit_state ] );
 * terrain({x,y}, MAPCHAR);
 * terrain(x,y, MAPCHAR);
 */
int
lspo_terrain(lua_State *L)
{
    terrain tmpterrain;
    xchar x = 0, y = 0;
    struct selectionvar *sel = NULL;
    int argc = lua_gettop(L);

    create_des_coder();
    tmpterrain.tlit = SET_LIT_NOCHANGE;
    tmpterrain.ter = INVALID_TYPE;

    if (argc == 1) {
        lua_Integer tx, ty;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &tx, &ty);
        x = tx, y = ty;
        if (tx == -1 && ty == -1) {
            lua_getfield(L, 1, "selection");
            sel = l_selection_check(L, -1);
            lua_pop(L, 1);
        }
        tmpterrain.ter = get_table_mapchr(L, "typ");
        tmpterrain.tlit = get_table_int_opt(L, "lit", SET_LIT_NOCHANGE);
    } else if (argc == 2 && lua_type(L, 1) == LUA_TTABLE
               && lua_type(L, 2) == LUA_TSTRING) {
        lua_Integer tx, ty;
        tmpterrain.ter = check_mapchr(luaL_checkstring(L, 2));
        lua_pop(L, 1);
        (void) get_coord(L, 1, &tx, &ty);
        x = tx;
        y = ty;
    } else if (argc == 2) {
        sel = l_selection_check(L, 1);
        tmpterrain.ter = check_mapchr(luaL_checkstring(L, 2));
    } else if (argc == 3) {
        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);
        tmpterrain.ter = check_mapchr(luaL_checkstring(L, 3));
    } else {
        nhl_error(L, "wrong parameters");
    }

    if (tmpterrain.ter == INVALID_TYPE)
        nhl_error(L, "Erroneous map char");

    if (sel) {
        selection_iterate(sel, sel_set_ter, (genericptr_t) &tmpterrain);
    } else {
        get_location_coord(&x, &y, ANY_LOC, g.coder->croom,
                           SP_COORD_PACK(x, y));
        sel_set_ter(x, y, (genericptr_t) &tmpterrain);
    }

    return 0;
}

/*
 * replace_terrain({ x1=NN,y1=NN, x2=NN,y2=NN, fromterrain=MAPCHAR,
 *                   toterrain=MAPCHAR, lit=N, chance=NN });
 * replace_terrain({ region={x1,y1, x2,y2}, fromterrain=MAPCHAR,
 *                   toterrain=MAPCHAR, lit=N, chance=NN });
 * replace_terrain({ selection=selection.area(2,5, 40,10),
 *                   fromterrain=MAPCHAR, toterrain=MAPCHAR });
 * replace_terrain({ selection=SEL, mapfragment=[[...]],
 *                   toterrain=MAPCHAR });
 */
int
lspo_replace_terrain(lua_State *L)
{
    xchar totyp, fromtyp;
    struct mapfragment *mf = NULL;
    struct selectionvar *sel = NULL;
    boolean freesel = FALSE;
    int x, y;
    lua_Integer x1, y1, x2, y2;
    int chance;
    int tolit;

    create_des_coder();

    lcheck_param_table(L);

    totyp = get_table_mapchr(L, "toterrain");

    if (totyp >= MAX_TYPE)
        return 0;

    fromtyp = get_table_mapchr_opt(L, "fromterrain", INVALID_TYPE);

    if (fromtyp == INVALID_TYPE) {
        const char *err;
        char *tmpstr = get_table_str(L, "mapfragment");
        mf = mapfrag_fromstr(tmpstr);
        free(tmpstr);

        if ((err = mapfrag_error(mf)) != NULL) {
            nhl_error(L, err);
            /*NOTREACHED*/
        }
    }

    chance = get_table_int_opt(L, "chance", 100);
    tolit = get_table_int_opt(L, "lit", SET_LIT_NOCHANGE);
    x1 = get_table_int_opt(L, "x1", -1);
    y1 = get_table_int_opt(L, "y1", -1);
    x2 = get_table_int_opt(L, "x2", -1);
    y2 = get_table_int_opt(L, "y2", -1);

    if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) {
        get_table_region(L, "region", &x1, &y1, &x2, &y2, TRUE);
    }

    if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) {
        lua_getfield(L, 1, "selection");
        if (lua_type(L, -1) != LUA_TNIL)
            sel = l_selection_check(L, -1);
        lua_pop(L, 1);
    }

    if (!sel) {
        sel = selection_new();
        freesel = TRUE;

        if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) {
            (void) selection_not(sel);
        } else {
            xchar rx1, ry1, rx2, ry2;
            rx1 = x1, ry1 = y1, rx2 = x2, ry2 = y2;
            get_location(&rx1, &ry1, ANY_LOC, g.coder->croom);
            get_location(&rx2, &ry2, ANY_LOC, g.coder->croom);
            for (x = max(rx1, 0); x <= min(rx2, COLNO - 1); x++)
                for (y = max(ry1, 0); y <= min(ry2, ROWNO - 1); y++)
                    selection_setpoint(x, y, sel, 1);
        }
    }

    for (y = 0; y <= sel->hei; y++)
        for (x = 1; x < sel->wid; x++)
            if (selection_getpoint(x, y,sel)) {
                if (mf) {
                    if (mapfrag_match(mf, x, y) && (rn2(100)) < chance)
                        (void) set_levltyp_lit(x, y, totyp, tolit);
                } else {
                    if (levl[x][y].typ == fromtyp && rn2(100) < chance)
                        (void) set_levltyp_lit(x, y, totyp, tolit);
                }
            }

    if (freesel)
        selection_free(sel, TRUE);

    mapfrag_free(&mf);

    return 0;
}

static boolean
generate_way_out_method(
    int nx, int ny,
    struct selectionvar *ov)
{
    static const int escapeitems[] = {
        PICK_AXE, DWARVISH_MATTOCK, WAN_DIGGING,
        WAN_TELEPORTATION, SCR_TELEPORTATION, RIN_TELEPORTATION
    };
    struct selectionvar *ov2 = selection_new(), *ov3;
    xchar x, y;
    boolean res = TRUE;

    selection_floodfill(ov2, nx, ny, TRUE);
    ov3 = selection_clone(ov2);

    /* try to make a secret door */
    while (selection_rndcoord(ov3, &x, &y, TRUE)) {
        if (isok(x + 1, y) && !selection_getpoint(x + 1, y, ov)
            && IS_WALL(levl[x + 1][y].typ)
            && isok(x + 2, y) &&  selection_getpoint(x + 2, y, ov)
            && ACCESSIBLE(levl[x + 2][y].typ)) {
            levl[x + 1][y].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x - 1, y) && !selection_getpoint(x - 1, y, ov)
            && IS_WALL(levl[x - 1][y].typ)
            && isok(x - 2, y) && selection_getpoint(x - 2, y, ov)
            && ACCESSIBLE(levl[x - 2][y].typ)) {
            levl[x - 1][y].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x, y + 1) && !selection_getpoint(x, y + 1, ov)
            && IS_WALL(levl[x][y + 1].typ)
            && isok(x, y + 2) && selection_getpoint(x, y + 2, ov)
            && ACCESSIBLE(levl[x][y + 2].typ)) {
            levl[x][y + 1].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x, y - 1) && !selection_getpoint(x, y - 1, ov)
            && IS_WALL(levl[x][y - 1].typ)
            && isok(x, y - 2) && selection_getpoint(x, y - 2, ov)
            && ACCESSIBLE(levl[x][y - 2].typ)) {
            levl[x][y - 1].typ = SDOOR;
            goto gotitdone;
        }
    }

    /* try to make a hole or a trapdoor */
    if (Can_fall_thru(&u.uz)) {
        selection_free(ov3, TRUE);
        ov3 = selection_clone(ov2);
        while (selection_rndcoord(ov3, &x, &y, TRUE)) {
            if (maketrap(x, y, rn2(2) ? HOLE : TRAPDOOR))
                goto gotitdone;
        }
    }

    /* generate one of the escape items */
    if (selection_rndcoord(ov2, &x, &y, FALSE)) {
        mksobj_at(escapeitems[rn2(SIZE(escapeitems))], x, y, TRUE, FALSE);
        goto gotitdone;
    }

    res = FALSE;
 gotitdone:
    selection_free(ov2, TRUE);
    selection_free(ov3, TRUE);
    return res;
}

static void
ensure_way_out(void)
{
    struct selectionvar *ov = selection_new();
    struct trap *ttmp = g.ftrap;
    int x, y;
    boolean ret = TRUE;
    stairway *stway = g.stairs;

    set_selection_floodfillchk(floodfillchk_match_accessible);

    while (stway) {
        if (stway->tolev.dnum == u.uz.dnum)
            selection_floodfill(ov, stway->sx, stway->sy, TRUE);
        stway = stway->next;
    }

    while (ttmp) {
        if ((undestroyable_trap(ttmp->ttyp) || is_hole(ttmp->ttyp))
            && !selection_getpoint(ttmp->tx, ttmp->ty, ov))
            selection_floodfill(ov, ttmp->tx, ttmp->ty, TRUE);
        ttmp = ttmp->ntrap;
    }

    do {
        ret = TRUE;
        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++)
                if (ACCESSIBLE(levl[x][y].typ)
                    && !selection_getpoint(x, y, ov)) {
                    if (generate_way_out_method(x, y, ov))
                        selection_floodfill(ov, x, y, TRUE);
                    ret = FALSE;
                    goto outhere;
                }
 outhere:
        ;
    } while (!ret);
    selection_free(ov, TRUE);
}

static lua_Integer
get_table_intarray_entry(lua_State *L, int tableidx, int entrynum)
{
    lua_Integer ret = 0;
    if (tableidx < 0)
        tableidx--;

    lua_pushinteger(L, entrynum);
    lua_gettable(L, tableidx);
    if (lua_isnumber(L, -1)) {
        ret = lua_tointeger(L, -1);
    } else {
        char buf[BUFSZ];

        Sprintf(buf, "Array entry #%i is %s, expected number",
                1, luaL_typename(L, -1));
        nhl_error(L, buf);
    }
    lua_pop(L, 1);
    return ret;
}

static int
get_table_region(
    lua_State *L,
    const char *name,
    lua_Integer *x1, lua_Integer *y1,
    lua_Integer *x2, lua_Integer *y2,
    boolean optional)
{
    lua_Integer arrlen;

    lua_getfield(L, 1, name);
    if (optional && lua_type(L, -1) == LUA_TNIL) {
        lua_pop(L, 1);
        return 1;
    }

    luaL_checktype(L, -1, LUA_TTABLE);

    lua_len(L, -1);
    arrlen = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (arrlen != 4) {
        nhl_error(L, "Not a region");
        lua_pop(L, 1);
        return 0;
    }

    *x1 = get_table_intarray_entry(L, -1, 1);
    *y1 = get_table_intarray_entry(L, -1, 2);
    *x2 = get_table_intarray_entry(L, -1, 3);
    *y2 = get_table_intarray_entry(L, -1, 4);

    lua_pop(L, 1);
    return 1;
}

boolean
get_coord(lua_State *L, int i, lua_Integer *x, lua_Integer *y)
{
    boolean ret = FALSE;
    int ltyp = lua_type(L, i);

    if (ltyp == LUA_TTABLE) {
        int arrlen;
        boolean gotx = FALSE;

        lua_getfield(L, i, "x");
        if (!lua_isnil(L, -1)) {
            *x = luaL_checkinteger(L, -1);
            gotx = TRUE;
        }
        lua_pop(L, 1);

        if (gotx) {
            lua_getfield(L, i, "y");
            if (!lua_isnil(L, -1)) {
                *y = luaL_checkinteger(L, -1);
                lua_pop(L, 1);
                ret = TRUE;
            } else {
                nhl_error(L, "Not a coordinate");
                return FALSE;
            }
        } else {
            lua_len(L, i);
            arrlen = lua_tointeger(L, -1);
            lua_pop(L, 1);
            if (arrlen != 2) {
                nhl_error(L, "Not a coordinate");
                return FALSE;
            }

            *x = get_table_intarray_entry(L, i, 1);
            *y = get_table_intarray_entry(L, i, 2);

            return TRUE;
        }
    } else if (ltyp != LUA_TNIL) {
        /* non-existent coord is ok */
        nhl_error(L, "non-table coord specified");
    }
    return ret;
}

static void
levregion_add(lev_region* lregion)
{
    if (!lregion->in_islev) {
        get_location(&lregion->inarea.x1, &lregion->inarea.y1, ANY_LOC,
                     (struct mkroom *) 0);
        get_location(&lregion->inarea.x2, &lregion->inarea.y2, ANY_LOC,
                     (struct mkroom *) 0);
    }

    if (!lregion->del_islev) {
        get_location(&lregion->delarea.x1, &lregion->delarea.y1,
                     ANY_LOC, (struct mkroom *) 0);
        get_location(&lregion->delarea.x2, &lregion->delarea.y2,
                     ANY_LOC, (struct mkroom *) 0);
    }
    if (g.num_lregions) {
        /* realloc the lregion space to add the new one */
        lev_region *newl = (lev_region *) alloc(
            sizeof (lev_region) * (unsigned) (1 + g.num_lregions));

        (void) memcpy((genericptr_t) (newl), (genericptr_t) g.lregions,
                      sizeof (lev_region) * g.num_lregions);
        Free(g.lregions);
        g.num_lregions++;
        g.lregions = newl;
    } else {
        g.num_lregions = 1;
        g.lregions = (lev_region *) alloc(sizeof (lev_region));
    }
    (void) memcpy(&g.lregions[g.num_lregions - 1], lregion,
                  sizeof (lev_region));
}

/* teleport_region({ region = { x1,y1, x2,y2} }); */
/* teleport_region({ region = { x1,y1, x2,y2}, [ region_islev = 1, ] exclude = { x1,y1, x2,y2}, [ exclude_islen = 1, ] [ dir = "up" ] }); */
/* TODO: maybe allow using selection, with a new selection method "getextents()"? */
int
lspo_teleport_region(lua_State *L)
{
    static const char *const teledirs[] = { "both", "down", "up", NULL };
    static const int teledirs2i[] = { LR_TELE, LR_DOWNTELE, LR_UPTELE, -1 };
    lev_region tmplregion;
    lua_Integer x1,y1,x2,y2;

    create_des_coder();

    lcheck_param_table(L);

    get_table_region(L, "region", &x1, &y1, &x2, &y2, FALSE);
    tmplregion.inarea.x1 = x1;
    tmplregion.inarea.y1 = y1;
    tmplregion.inarea.x2 = x2;
    tmplregion.inarea.y2 = y2;

    x1 = y1 = x2 = y2 = 0;
    get_table_region(L, "exclude", &x1, &y1, &x2, &y2, TRUE);
    tmplregion.delarea.x1 = x1;
    tmplregion.delarea.y1 = y1;
    tmplregion.delarea.x2 = x2;
    tmplregion.delarea.y2 = y2;

    tmplregion.in_islev = get_table_boolean_opt(L, "region_islev", 0);
    tmplregion.del_islev = get_table_boolean_opt(L, "exclude_islev", 0);

    tmplregion.rtype = teledirs2i[get_table_option(L, "dir", "both",
                                                   teledirs)];
    tmplregion.padding = 0;
    tmplregion.rname.str = NULL;

    levregion_add(&tmplregion);

    return 0;
}

/* TODO: FIXME
   from lev_comp SPO_LEVREGION was called as:
   - STAIR:(x1,y1,x2,y2),(x1,y1,x2,y2),dir
   - PORTAL:(x1,y1,x2,y2),(x1,y1,x2,y2),string
   - BRANCH:(x1,y1,x2,y2),(x1,y1,x2,y2),dir

*/
/* levregion({ region = { x1,y1, x2,y2 }, exclude = { x1,y1, x2,y2 }, type = "portal", name="air" }); */
/* TODO: allow region to be optional, defaulting to whole level */
int
lspo_levregion(lua_State *L)
{
    static const char *const regiontypes[] = {
        "stair-down", "stair-up", "portal", "branch",
        "teleport", "teleport-up", "teleport-down", NULL
    };
    static const int regiontypes2i[] = {
        LR_DOWNSTAIR, LR_UPSTAIR, LR_PORTAL, LR_BRANCH,
        LR_TELE, LR_UPTELE, LR_DOWNTELE, 0
    };
    lev_region tmplregion;
    lua_Integer x1,y1,x2,y2;

    create_des_coder();

    lcheck_param_table(L);

    get_table_region(L, "region", &x1, &y1, &x2, &y2, FALSE);

    tmplregion.inarea.x1 = x1;
    tmplregion.inarea.y1 = y1;
    tmplregion.inarea.x2 = x2;
    tmplregion.inarea.y2 = y2;

    x1 = y1 = x2 = y2 = 0;
    get_table_region(L, "exclude", &x1, &y1, &x2, &y2, TRUE);

    tmplregion.delarea.x1 = x1;
    tmplregion.delarea.y1 = y1;
    tmplregion.delarea.x2 = x2;
    tmplregion.delarea.y2 = y2;

    tmplregion.in_islev = get_table_boolean_opt(L, "region_islev", 0);
    tmplregion.del_islev = get_table_boolean_opt(L, "exclude_islev", 0);
    tmplregion.rtype = regiontypes2i[get_table_option(L, "type", "stair-down",
                                                      regiontypes)];
    tmplregion.padding = get_table_int_opt(L, "padding", 0);
    tmplregion.rname.str = get_table_str_opt(L, "name", NULL);

    levregion_add(&tmplregion);
    return 0;
}

static void
sel_set_lit(int x, int y, genericptr_t arg)
{
     int lit = *(int *)arg;

     levl[x][y].lit = (levl[x][y].typ == LAVAPOOL) ? 1 : lit;
}

/* Add to the room any doors within/bordering it */
static void
add_doors_to_room(struct mkroom *croom)
{
    int x, y;

    for (x = croom->lx - 1; x <= croom->hx + 1; x++)
        for (y = croom->ly - 1; y <= croom->hy + 1; y++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR)
                maybe_add_door(x, y, croom);
}

/* region(selection, lit); */
/* region({ x1=NN, y1=NN, x2=NN, y2=NN, lit=BOOL, type=ROOMTYPE, joined=BOOL,
            irregular=BOOL, filled=NN [ , contents = FUNCTION ] }); */
/* region({ region={x1,y1, x2,y2}, type="ordinary" }); */
int
lspo_region(lua_State *L)
{
    xchar dx1, dy1, dx2, dy2;
    register struct mkroom *troom;
    boolean do_arrival_room = FALSE, room_not_needed,
            irregular = FALSE, joined = TRUE;
    int rtype = OROOM, rlit = 1, needfill = 0;
    int argc = lua_gettop(L);

    create_des_coder();

    if (argc <= 1) {
        lcheck_param_table(L);

        /* TODO: "unfilled" ==> filled=0, "filled" ==> filled=1, and
         * "lvflags_only" ==> filled=2, probably in a get_table_needfill_opt */
        needfill = get_table_int_opt(L, "filled", 0);
        irregular = get_table_boolean_opt(L, "irregular", 0);
        joined = get_table_boolean_opt(L, "joined", TRUE);
        do_arrival_room = get_table_boolean_opt(L, "arrival_room", 0);
        rtype = get_table_roomtype_opt(L, "type", OROOM);
        rlit = get_table_int_opt(L, "lit", -1);
        dx1 = get_table_int_opt(L, "x1", -1); /* TODO: area */
        dy1 = get_table_int_opt(L, "y1", -1);
        dx2 = get_table_int_opt(L, "x2", -1);
        dy2 = get_table_int_opt(L, "y2", -1);

        if (dx1 == -1 && dy1 == -1 && dx2 == -1 && dy2 == -1) {
            lua_Integer rx1, ry1, rx2, ry2;
            get_table_region(L, "region", &rx1, &ry1, &rx2, &ry2, FALSE);
            dx1 = rx1; dy1 = ry1;
            dx2 = rx2; dy2 = ry2;
        }
        if (dx1 == -1 && dy1 == -1 && dx2 == -1 && dy2 == -1) {
            nhl_error(L, "region needs region");
        }

    } else if (argc == 2) {
        /* region(selection, "lit"); */
        static const char *const lits[] = { "unlit", "lit", NULL };
        struct selectionvar *sel = l_selection_check(L, 1);

        rlit = luaL_checkoption(L, 2, "lit", lits);

        /*
    TODO: adjust region size for wall, but only if lit
    TODO: lit=random
        */
        if (rlit)
            selection_do_grow(sel, W_ANY);
        selection_iterate(sel, sel_set_lit, (genericptr_t) &rlit);

        /* TODO: skip the rest of this function? */
        return 0;
    } else {
        nhl_error(L, "Wrong parameters");
        return 0;
    }

    rlit = litstate_rnd(rlit);

    get_location(&dx1, &dy1, ANY_LOC, (struct mkroom *) 0);
    get_location(&dx2, &dy2, ANY_LOC, (struct mkroom *) 0);

    /* Many regions are simple, rectangular areas that just need to set
     * lighting in an area. In that case, we don't need to do anything
     * complicated by creating a room. The exceptions are:
     *  - Special rooms (which usually need to be filled).
     *  - Irregular regions (more convenient to use the room-making code).
     *  - Themed room regions (which often have contents).
     *  - When a room is desired to constrain the arrival of migrating monsters
     *    (see the mon_arrive function for details).
     */
    room_not_needed = (rtype == OROOM && !irregular
                       && !do_arrival_room && !g.in_mk_themerooms);
    if (room_not_needed || g.nroom >= MAXNROFROOMS) {
        region tmpregion;
        if (!room_not_needed)
            impossible("Too many rooms on new level!");
        tmpregion.rlit = rlit;
        tmpregion.x1 = dx1;
        tmpregion.y1 = dy1;
        tmpregion.x2 = dx2;
        tmpregion.y2 = dy2;
        light_region(&tmpregion);

        return 0;
    }

    troom = &g.rooms[g.nroom];

    /* mark rooms that must be filled, but do it later */
    troom->needfill = needfill;

    troom->needjoining = joined;

    if (irregular) {
        g.min_rx = g.max_rx = dx1;
        g.min_ry = g.max_ry = dy1;
        g.smeq[g.nroom] = g.nroom;
        flood_fill_rm(dx1, dy1, g.nroom + ROOMOFFSET, rlit, TRUE);
        add_room(g.min_rx, g.min_ry, g.max_rx, g.max_ry, FALSE, rtype, TRUE);
        troom->rlit = rlit;
        troom->irregular = TRUE;
    } else {
        add_room(dx1, dy1, dx2, dy2, rlit, rtype, TRUE);
#ifdef SPECIALIZATION
        topologize(troom, FALSE); /* set roomno */
#else
        topologize(troom); /* set roomno */
#endif
    }

    if (!room_not_needed) {
        if (g.coder->n_subroom > 1)
            impossible("region as subroom");
        else {
            g.coder->tmproomlist[g.coder->n_subroom] = troom;
            g.coder->failed_room[g.coder->n_subroom] = FALSE;
            g.coder->n_subroom++;
            update_croom();
            lua_getfield(L, 1, "contents");
            if (lua_type(L, -1) == LUA_TFUNCTION) {
                lua_remove(L, -2);
                lua_call(L, 0, 0);
            } else
                lua_pop(L, 1);
            spo_endroom(g.coder);
            add_doors_to_room(troom);
        }
    }

    return 0;
}

/* drawbridge({ dir="east", state="closed", x=05,y=08 }); */
/* drawbridge({ dir="east", state="closed", coord={05,08} }); */
int
lspo_drawbridge(lua_State *L)
{
    static const char *const mwdirs[] = {
        "north", "south", "west", "east", "random", NULL
    };
    static const int mwdirs2i[] = {
        DB_NORTH, DB_SOUTH, DB_WEST, DB_EAST, -1, -2
    };
    static const char *const dbopens[] = {
        "open", "closed", "random", NULL
    };
    static const int dbopens2i[] = { 1, 0, -1, -2 };
    xchar x, y;
    lua_Integer mx, my;
    int dir;
    int db_open;
    long dcoord;

    create_des_coder();

    lcheck_param_table(L);

    get_table_xy_or_coord(L, &mx, &my);

    dir = mwdirs2i[get_table_option(L, "dir", "random", mwdirs)];
    dcoord = SP_COORD_PACK(mx, my);
    db_open = dbopens2i[get_table_option(L, "state", "random", dbopens)];
    x = mx;
    y = my;

    get_location_coord(&x, &y, DRY | WET | HOT, g.coder->croom, dcoord);
    if (db_open == -1)
        db_open = !rn2(2);
    if (!create_drawbridge(x, y, dir, db_open ? TRUE : FALSE))
        impossible("Cannot create drawbridge.");
    SpLev_Map[x][y] = 1;

    return 0;
}

/* mazewalk({ x = NN, y = NN, typ = ".", dir = "north", stocked = 0 }); */
/* mazewalk({ coord = {XX, YY}, typ = ".", dir = "north", stocked = 0 }); */
/* mazewalk(x,y,dir); */
int
lspo_mazewalk(lua_State *L)
{
    static const char *const mwdirs[] = {
        "north", "south", "east", "west", "random", NULL
    };
    static const int mwdirs2i[] = {
        W_NORTH, W_SOUTH, W_EAST, W_WEST, W_RANDOM, -2
    };
    xchar x, y;
    lua_Integer mx, my;
    xchar ftyp = ROOM;
    int fstocked = 1, dir = -1;
    long mcoord;
    int argc = lua_gettop(L);

    create_des_coder();

    if (argc == 3) {
        mx = luaL_checkinteger(L, 1);
        my = luaL_checkinteger(L, 2);
        dir = mwdirs2i[luaL_checkoption(L, 3, "random", mwdirs)];
    } else {
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &mx, &my);
        ftyp = get_table_mapchr_opt(L, "typ", ROOM);
        fstocked = get_table_boolean_opt(L, "stocked", 1);
        dir = mwdirs2i[get_table_option(L, "dir", "random", mwdirs)];
    }

    mcoord = SP_COORD_PACK(mx, my);
    x = mx;
    y = my;

    get_location_coord(&x, &y, ANY_LOC, g.coder->croom, mcoord);

    if (!isok(x, y))
        return 0;

    if (ftyp < 1) {
        ftyp = g.level.flags.corrmaze ? CORR : ROOM;
    }

    if (dir == W_RANDOM)
        dir = random_wdir();

    /* don't use move() - it doesn't use W_NORTH, etc. */
    switch (dir) {
    case W_NORTH:
        --y;
        break;
    case W_SOUTH:
        y++;
        break;
    case W_EAST:
        x++;
        break;
    case W_WEST:
        --x;
        break;
    default:
        impossible("mazewalk: Bad direction");
    }

    if (!IS_DOOR(levl[x][y].typ)) {
        levl[x][y].typ = ftyp;
        levl[x][y].flags = 0;
    }

    /*
     * We must be sure that the parity of the coordinates for
     * walkfrom() is odd.  But we must also take into account
     * what direction was chosen.
     */
    if (!(x % 2)) {
        if (dir == W_EAST)
            x++;
        else
            x--;

        /* no need for IS_DOOR check; out of map bounds */
        levl[x][y].typ = ftyp;
        levl[x][y].flags = 0;
    }

    if (!(y % 2)) {
        if (dir == W_SOUTH)
            y++;
        else
            y--;
    }

    walkfrom(x, y, ftyp);
    if (fstocked)
        fill_empty_maze();

    return 0;
}

/* wall_property({ x1=0, y1=0, x2=78, y2=20, property="nondiggable" }); */
/* wall_property({ region = {1,0, 78,20}, property="nonpasswall" }); */
int
lspo_wall_property(lua_State *L)
{
    static const char *const wprops[] = { "nondiggable", "nonpasswall", NULL };
    static const int wprop2i[] = { W_NONDIGGABLE, W_NONPASSWALL, -1 };
    xchar dx1 = -1, dy1 = -1, dx2 = -1, dy2 = -1;
    int wprop;

    create_des_coder();

    lcheck_param_table(L);

    dx1 = get_table_int_opt(L, "x1", -1);
    dy1 = get_table_int_opt(L, "y1", -1);
    dx2 = get_table_int_opt(L, "x2", -1);
    dy2 = get_table_int_opt(L, "y2", -1);

    if (dx1 == -1 && dy1 == -1 && dx2 == -1 && dy2 == -1) {
        lua_Integer rx1, ry1, rx2, ry2;
        get_table_region(L, "region", &rx1, &ry1, &rx2, &ry2, FALSE);
        dx1 = rx1; dy1 = ry1;
        dx2 = rx2; dy2 = ry2;
    }

    wprop = wprop2i[get_table_option(L, "property", "nondiggable", wprops)];

    if (dx1 == -1)
        dx1 = g.xstart - 1;
    if (dy1 == -1)
        dy1 = g.ystart - 1;
    if (dx2 == -1)
        dx2 = g.xstart + g.xsize + 1;
    if (dy2 == -1)
        dy2 = g.ystart + g.ysize + 1;

    get_location(&dx1, &dy1, ANY_LOC, (struct mkroom *) 0);
    get_location(&dx2, &dy2, ANY_LOC, (struct mkroom *) 0);

    set_wall_property(dx1, dy1, dx2, dy2, wprop);

    return 0;
}

static void
set_wallprop_in_selection(lua_State *L, int prop)
{
    int argc = lua_gettop(L);
    boolean freesel = FALSE;
    struct selectionvar *sel = (struct selectionvar *) 0;

    create_des_coder();

    if (argc == 1) {
        sel = l_selection_check(L, -1);
    } else if (argc == 0) {
        freesel = TRUE;
        sel = selection_new();
        selection_not(sel);
    }

    if (sel) {
        selection_iterate(sel, sel_set_wall_property, (genericptr_t) &prop);
        if (freesel)
            selection_free(sel, TRUE);
    }
}

/* non_diggable(selection); */
/* non_diggable(); */
int
lspo_non_diggable(lua_State *L)
{
    set_wallprop_in_selection(L, W_NONDIGGABLE);
    return 0;
}

/* non_passwall(selection); */
/* non_passwall(); */
int
lspo_non_passwall(lua_State *L)
{
    set_wallprop_in_selection(L, W_NONPASSWALL);
    return 0;
}

#if 0
/*ARGSUSED*/
static void
sel_set_wallify(int x, int y, genericptr_t arg UNUSED)
{
    wallify_map(x, y, x, y);
}
#endif

/* TODO: wallify(selection) */
/* wallify({ x1=NN,y1=NN, x2=NN,y2=NN }); */
/* wallify(); */
int
lspo_wallify(lua_State *L)
{
    int dx1 = -1, dy1 = -1, dx2 = -1, dy2 = -1;

    /* TODO: clamp coord values */
    /* TODO: maybe allow wallify({x1,y1}, {x2,y2}) */
    /* TODO: is_table_coord(), is_table_area(),
             get_table_coord(), get_table_area() */

    create_des_coder();

    if (lua_gettop(L) == 1) {
        dx1 = get_table_int(L, "x1");
        dy1 = get_table_int(L, "y1");
        dx2 = get_table_int(L, "x2");
        dy2 = get_table_int(L, "y2");
    }

    wallify_map(dx1 < 0 ? (g.xstart - 1) : dx1,
                dy1 < 0 ? (g.ystart - 1) : dy1,
                dx2 < 0 ? (g.xstart + g.xsize + 1) : dx2,
                dy2 < 0 ? (g.ystart + g.ysize + 1) : dy2);

    return 0;
}

/* reset_level is only needed for testing purposes */
int
lspo_reset_level(lua_State *L UNUSED)
{
    boolean wtower = In_W_tower(u.ux, u.uy, &u.uz);

    iflags.lua_testing = TRUE;
    if (L)
        create_des_coder();
    makemap_prepost(TRUE, wtower);
    g.in_mklev = TRUE;
    oinit(); /* assign level dependent obj probabilities */
    clear_level_structures();
    return 0;
}

/* finalize_level is only needed for testing purposes */
int
lspo_finalize_level(lua_State *L UNUSED)
{
    boolean wtower = In_W_tower(u.ux, u.uy, &u.uz);
    int i;

    if (L)
        create_des_coder();

    link_doors_rooms();
    remove_boundary_syms();

    /* TODO: ensure_way_out() needs rewrite */
    if (L && g.coder->check_inaccessibles)
        ensure_way_out();

    /* FIXME: Ideally, we want this call to only cover areas of the map
     * which were not inserted directly by the special level file (see
     * the insect legs on Baalzebub's level, for instance). Since that
     * is currently not possible, we overload the corrmaze flag for this
     * purpose.
     */
    if (!g.level.flags.corrmaze)
        wallification(1, 0, COLNO - 1, ROWNO - 1);

    if (L)
        flip_level_rnd(g.coder->allow_flips, FALSE);

    count_features();

    if (L && g.coder->solidify)
        solidify_map();

    /* This must be done before sokoban_detect(),
     * otherwise branch stairs won't be premapped. */
    fixup_special();

    if (L && g.coder->premapped)
        sokoban_detect();

    level_finalize_topology();

    for (i = 0; i < g.nroom; ++i) {
        fill_special_room(&g.rooms[i]);
    }

    makemap_prepost(FALSE, wtower);
    iflags.lua_testing = FALSE;
    return 0;
}

/* map({ x = 10, y = 10, map = [[...]] }); */
/* map({ coord = {10, 10}, map = [[...]] }); */
/* map({ halign = "center", valign = "center", map = [[...]] }); */
/* map({ map = [[...]], contents = function(map) ... end }); */
/* map([[...]]) */
int
lspo_map(lua_State *L)
{
    /*
TODO: allow passing an array of strings as map data
TODO: handle if map lines aren't same length
TODO: g.coder->croom needs to be updated
     */

    static const char *const left_or_right[] = {
        "left", "half-left", "center", "half-right", "right", "none", NULL
    };
    static const int l_or_r2i[] = {
        LEFT, H_LEFT, CENTER, H_RIGHT, RIGHT, -1, -1
    };
    static const char *const top_or_bot[] = {
        "top", "center", "bottom", "none", NULL
    };
    static const int t_or_b2i[] = { TOP, CENTER, BOTTOM, -1, -1 };
    int lr, tb;
    lua_Integer x = -1, y = -1;
    struct mapfragment *mf;
    char *tmpstr;
    int argc = lua_gettop(L);
    boolean has_contents = FALSE;
    int tryct = 0;
    int ox, oy;

    create_des_coder();

    if (g.in_mk_themerooms && g.themeroom_failed)
        return 0;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        tmpstr = dupstr(luaL_checkstring(L, 1));
        lr = tb = CENTER;
        mf = mapfrag_fromstr(tmpstr);
        free(tmpstr);
    } else {
        lcheck_param_table(L);
        lr = l_or_r2i[get_table_option(L, "halign", "none", left_or_right)];
        tb = t_or_b2i[get_table_option(L, "valign", "none", top_or_bot)];
        get_table_xy_or_coord(L, &x, &y);
        tmpstr = get_table_str(L, "map");
        lua_getfield(L, 1, "contents");
        if (lua_type(L, -1) == LUA_TFUNCTION) {
            lua_remove(L, -2);
            has_contents = TRUE;
        } else {
            lua_pop(L, 1);
        }
        mf = mapfrag_fromstr(tmpstr);
        free(tmpstr);
    }

    if (!mf) {
        nhl_error(L, "Map data error");
        return 0;
    }

    ox = x;
    oy = y;
 redo_maploc:
    g.xsize = mf->wid;
    g.ysize = mf->hei;

    if (lr == -1 && tb == -1) {
        if (g.in_mk_themerooms && (ox == -1 || oy == -1)) {
            if (ox == -1) {
                if (g.coder->croom) {
                    x = somex(g.coder->croom) - mf->wid;
                    if (x < 1)
                        x = 1;
                } else {
                    x = 1 + rn2(COLNO - 1 - mf->wid);
                }
            }

            if (oy == -1) {
                if (g.coder->croom) {
                    y = somey(g.coder->croom) - mf->hei;
                    if (y < 1)
                        y = 1;
                } else {
                    y = rn2(ROWNO - mf->wid);
                }
            }
        }

        if (isok(x, y)) {
            /* x,y is given, place map starting at x,y */
            if (g.coder->croom) {
                /* in a room? adjust to room relative coords */
                g.xstart = x + g.coder->croom->lx;
                g.ystart = y + g.coder->croom->ly;
                g.xsize = min(mf->wid,
                              (g.coder->croom->hx - g.coder->croom->lx));
                g.ysize = min(mf->hei,
                              (g.coder->croom->hy - g.coder->croom->ly));
            } else {
                g.xsize = mf->wid;
                g.ysize = mf->hei;
                g.xstart = x;
                g.ystart = y;
            }
        } else {
            mapfrag_free(&mf);
            nhl_error(L, "Map requires either x,y or halign,valign params");
            return 0;
        }
    } else {
        /* place map starting at halign,valign */
        switch (lr) {
        case LEFT:
            g.xstart = splev_init_present ? 1 : 3;
            break;
        case H_LEFT:
            g.xstart = 2 + ((g.x_maze_max - 2 - g.xsize) / 4);
            break;
        case CENTER:
            g.xstart = 2 + ((g.x_maze_max - 2 - g.xsize) / 2);
            break;
        case H_RIGHT:
            g.xstart = 2 + ((g.x_maze_max - 2 - g.xsize) * 3 / 4);
            break;
        case RIGHT:
            g.xstart = g.x_maze_max - g.xsize - 1;
            break;
        }
        switch (tb) {
        case TOP:
            g.ystart = 3;
            break;
        case CENTER:
            g.ystart = 2 + ((g.y_maze_max - 2 - g.ysize) / 2);
            break;
        case BOTTOM:
            g.ystart = g.y_maze_max - g.ysize - 1;
            break;
        }
        if (!(g.xstart % 2))
            g.xstart++;
        if (!(g.ystart % 2))
            g.ystart++;
    }

    if (g.ystart < 0 || g.ystart + g.ysize > ROWNO) {
        if (g.in_mk_themerooms) {
            g.themeroom_failed = TRUE;
            goto skipmap;
        }
        /* try to move the start a bit */
        g.ystart += (g.ystart > 0) ? -2 : 2;
        if (g.ysize == ROWNO)
            g.ystart = 0;
        if (g.ystart < 0 || g.ystart + g.ysize > ROWNO)
            g.ystart = 0;
    }
    if (g.xsize <= 1 && g.ysize <= 1) {
        g.xstart = 1;
        g.ystart = 0;
        g.xsize = COLNO - 1;
        g.ysize = ROWNO;
    } else {
        xchar mptyp;

        /* Themed rooms should never overwrite anything */
        if (g.in_mk_themerooms) {
            boolean isokp = TRUE;
            for (y = g.ystart - 1; y < min(ROWNO, g.ystart + g.ysize) + 1; y++)
                for (x = g.xstart - 1; x < min(COLNO, g.xstart + g.xsize) + 1;
                     x++) {
                    if (!isok(x, y)) {
                        isokp = FALSE;
                    } else if (y < g.ystart || y >= (g.ystart + g.ysize)
                               || x < g.xstart || x >= (g.xstart + g.xsize)) {
                        if (levl[x][y].typ != STONE
                            || levl[x][y].roomno != NO_ROOM)
                            isokp = FALSE;
                    } else {
                        mptyp = mapfrag_get(mf, x - g.xstart, y - g.ystart);
                        if (mptyp >= MAX_TYPE)
                            continue;
                        if ((levl[x][y].typ != STONE
                             && levl[x][y].typ != mptyp)
                            || levl[x][y].roomno != NO_ROOM)
                            isokp = FALSE;
                    }
                    if (!isokp) {
                        if (tryct++ < 100 && (lr == -1 || tb == -1))
                            goto redo_maploc;
                        g.themeroom_failed = TRUE;
                        goto skipmap;
                    }
                }
        }

        /* Load the map */
        for (y = g.ystart; y < min(ROWNO, g.ystart + g.ysize); y++)
            for (x = g.xstart; x < min(COLNO, g.xstart + g.xsize); x++) {
                mptyp = mapfrag_get(mf, (x - g.xstart), (y - g.ystart));
                if (mptyp == INVALID_TYPE) {
                    /* TODO: warn about illegal map char */
                    continue;
                }
                if (mptyp >= MAX_TYPE)
                    continue;
                levl[x][y].typ = mptyp;
                levl[x][y].lit = FALSE;
                /* clear out levl: load_common_data may set them */
                levl[x][y].flags = 0;
                levl[x][y].horizontal = 0;
                levl[x][y].roomno = 0;
                levl[x][y].edge = 0;
                SpLev_Map[x][y] = 1;
                /*
                 *  Set secret doors to closed (why not trapped too?).  Set
                 *  the horizontal bit.
                 */
                if (levl[x][y].typ == SDOOR || IS_DOOR(levl[x][y].typ)) {
                    if (levl[x][y].typ == SDOOR)
                        levl[x][y].doormask = D_CLOSED;
                    /*
                     *  If there is a wall to the left that connects to a
                     *  (secret) door, then it is horizontal.  This does
                     *  not allow (secret) doors to be corners of rooms.
                     */
                    if (x != g.xstart && (IS_WALL(levl[x - 1][y].typ)
                                          || levl[x - 1][y].horizontal))
                        levl[x][y].horizontal = 1;
                } else if (levl[x][y].typ == HWALL
                           || levl[x][y].typ == IRONBARS)
                    levl[x][y].horizontal = 1;
                else if (levl[x][y].typ == LAVAPOOL)
                    levl[x][y].lit = 1;
                else if (splev_init_present && levl[x][y].typ == ICE)
                    levl[x][y].icedpool = icedpools ? ICED_POOL : ICED_MOAT;
            }
        if (g.coder->lvl_is_joined && !g.in_mk_themerooms)
            remove_rooms(g.xstart, g.ystart,
                         g.xstart + g.xsize, g.ystart + g.ysize);
    }

 skipmap:
    mapfrag_free(&mf);

    if (has_contents && !(g.in_mk_themerooms && g.themeroom_failed)) {
        l_push_wid_hei_table(L, g.xsize, g.ysize);
        lua_call(L, 1, 0);
    }

    return 0;
}

void
update_croom(void)
{
    if (!g.coder)
        return;

    if (g.coder->n_subroom)
        g.coder->croom = g.coder->tmproomlist[g.coder->n_subroom - 1];
    else
        g.coder->croom = NULL;
}

static struct sp_coder *
sp_level_coder_init(void)
{
    int tmpi;
    struct sp_coder *coder = (struct sp_coder *) alloc(sizeof *coder);

    coder->premapped = FALSE;
    coder->solidify = FALSE;
    coder->check_inaccessibles = FALSE;
    coder->allow_flips = 3; /* allow flipping level horiz/vert */
    coder->croom = NULL;
    coder->n_subroom = 1;
    coder->lvl_is_joined = FALSE;
    coder->room_stack = 0;

    splev_init_present = FALSE;
    icedpools = FALSE;

    for (tmpi = 0; tmpi <= MAX_NESTED_ROOMS; tmpi++) {
        coder->tmproomlist[tmpi] = (struct mkroom *) 0;
        coder->failed_room[tmpi] = FALSE;
    }

    update_croom();

    for (tmpi = 0; tmpi < MAX_CONTAINMENT; tmpi++)
        container_obj[tmpi] = NULL;
    container_idx = 0;

    invent_carrying_monster = NULL;

    (void) memset((genericptr_t) SpLev_Map, 0, sizeof SpLev_Map);

    g.level.flags.is_maze_lev = 0;

    g.xstart = 1; /* column [0] is off limits */
    g.ystart = 0;
    g.xsize = COLNO - 1; /* 1..COLNO-1 */
    g.ysize = ROWNO; /* 0..ROWNO-1 */

    return coder;
}


static const struct luaL_Reg nhl_functions[] = {
    { "message", lspo_message },
    { "monster", lspo_monster },
    { "object", lspo_object },
    { "level_flags", lspo_level_flags },
    { "level_init", lspo_level_init },
    { "engraving", lspo_engraving },
    { "mineralize", lspo_mineralize },
    { "door", lspo_door },
    { "stair", lspo_stair },
    { "ladder", lspo_ladder },
    { "grave", lspo_grave },
    { "altar", lspo_altar },
    { "map", lspo_map },
    { "feature", lspo_feature },
    { "terrain", lspo_terrain },
    { "replace_terrain", lspo_replace_terrain },
    { "room", lspo_room },
    { "corridor", lspo_corridor },
    { "random_corridors", lspo_random_corridors },
    { "gold", lspo_gold },
    { "trap", lspo_trap },
    { "mazewalk", lspo_mazewalk },
    { "drawbridge", lspo_drawbridge },
    { "region", lspo_region },
    { "levregion", lspo_levregion },
    { "wallify", lspo_wallify },
    { "wall_property", lspo_wall_property },
    { "non_diggable", lspo_non_diggable },
    { "non_passwall", lspo_non_passwall },
    { "teleport_region", lspo_teleport_region },
    { "reset_level", lspo_reset_level },
    { "finalize_level", lspo_finalize_level },
    /* TODO: { "branch", lspo_branch }, */
    /* TODO: { "portal", lspo_portal }, */
    { NULL, NULL }
};

/* TODO:

 - if des-file used MAZE_ID to start a level, the level needs
   des.level_flags("mazelevel")
 - expose g.coder->croom or g.[xy]start and g.xy[size] to lua.
 - detect a "subroom" automatically.
 - new function get_mapchar(x,y) to return the mapchar on map
 - many params should accept their normal type (eg, int or bool), AND "random"
 - automatically add shuffle(array)
 - automatically add align = { "law", "neutral", "chaos" } and shuffle it.
   (remove from lua files)
 - grab the header comments from des-files and add add them to the lua files

*/

void
l_register_des(lua_State *L)
{
    /* register des -table, and functions for it */
    lua_newtable(L);
    luaL_setfuncs(L, nhl_functions, 0);
    lua_setglobal(L, "des");
}

void
create_des_coder(void)
{
    if (!g.coder)
        g.coder = sp_level_coder_init();
}

/*
 * General loader
 */
boolean
load_special(const char *name)
{
    boolean result = FALSE;

    create_des_coder();

    if (!load_lua(name))
        goto give_up;

    link_doors_rooms();
    remove_boundary_syms();

    /* TODO: ensure_way_out() needs rewrite */
    if (g.coder->check_inaccessibles)
        ensure_way_out();

    /* FIXME: Ideally, we want this call to only cover areas of the map
     * which were not inserted directly by the special level file (see
     * the insect legs on Baalzebub's level, for instance). Since that
     * is currently not possible, we overload the corrmaze flag for this
     * purpose.
     */
    if (!g.level.flags.corrmaze)
        wallification(1, 0, COLNO - 1, ROWNO - 1);

    flip_level_rnd(g.coder->allow_flips, FALSE);

    count_features();

    if (g.coder->solidify)
        solidify_map();

    /* This must be done before sokoban_detect(),
     * otherwise branch stairs won't be premapped. */
    fixup_special();

    if (g.coder->premapped)
        sokoban_detect();

    result = TRUE;
 give_up:
    Free(g.coder);
    g.coder = NULL;

    return result;
}

/*sp_lev.c*/

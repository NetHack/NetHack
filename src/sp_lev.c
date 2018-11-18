/* NetHack 3.6	sp_lev.c	$NHDT-Date: 1524287226 2018/04/21 05:07:06 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.98 $ */
/*      Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the various functions that are related to the special
 * levels.
 *
 * It contains also the special level loader.
 */

#include "hack.h"
#include "dlb.h"
#include "sp_lev.h"

#ifdef _MSC_VER
 #pragma warning(push)
 #pragma warning(disable : 4244)
#endif

typedef void FDECL((*select_iter_func), (int, int, genericptr));

extern void FDECL(mkmap, (lev_init *));

STATIC_DCL void NDECL(solidify_map);
STATIC_DCL void FDECL(splev_stack_init, (struct splevstack *));
STATIC_DCL void FDECL(splev_stack_done, (struct splevstack *));
STATIC_DCL void FDECL(splev_stack_push, (struct splevstack *,
                                         struct opvar *));
STATIC_DCL struct opvar *FDECL(splev_stack_pop, (struct splevstack *));
STATIC_DCL struct splevstack *FDECL(splev_stack_reverse,
                                    (struct splevstack *));
STATIC_DCL struct opvar *FDECL(opvar_new_str, (char *));
STATIC_DCL struct opvar *FDECL(opvar_new_int, (long));
STATIC_DCL struct opvar *FDECL(opvar_new_coord, (int, int));
#if 0
STATIC_DCL struct opvar * FDECL(opvar_new_region, (int,int, int,int));
#endif /*0*/
STATIC_DCL struct opvar *FDECL(opvar_clone, (struct opvar *));
STATIC_DCL struct opvar *FDECL(opvar_var_conversion, (struct sp_coder *,
                                                      struct opvar *));
STATIC_DCL struct splev_var *FDECL(opvar_var_defined, (struct sp_coder *,
                                                       char *));
STATIC_DCL struct opvar *FDECL(splev_stack_getdat, (struct sp_coder *,
                                                    XCHAR_P));
STATIC_DCL struct opvar *FDECL(splev_stack_getdat_any, (struct sp_coder *));
STATIC_DCL void FDECL(variable_list_del, (struct splev_var *));
STATIC_DCL void FDECL(lvlfill_maze_grid, (int, int, int, int, SCHAR_P));
STATIC_DCL void FDECL(lvlfill_solid, (SCHAR_P, SCHAR_P));
STATIC_DCL void FDECL(set_wall_property, (XCHAR_P, XCHAR_P, XCHAR_P, XCHAR_P,
                                          int));
STATIC_DCL void NDECL(shuffle_alignments);
STATIC_DCL void NDECL(count_features);
STATIC_DCL void NDECL(remove_boundary_syms);
STATIC_DCL void FDECL(set_door_orientation, (int, int));
STATIC_DCL void FDECL(maybe_add_door, (int, int, struct mkroom *));
STATIC_DCL void NDECL(link_doors_rooms);
STATIC_DCL void NDECL(fill_rooms);
STATIC_DCL int NDECL(rnddoor);
STATIC_DCL int NDECL(rndtrap);
STATIC_DCL void FDECL(get_location, (schar *, schar *, int, struct mkroom *));
STATIC_DCL boolean FDECL(is_ok_location, (SCHAR_P, SCHAR_P, int));
STATIC_DCL unpacked_coord FDECL(get_unpacked_coord, (long, int));
STATIC_DCL void FDECL(get_location_coord, (schar *, schar *, int,
                                           struct mkroom *, long));
STATIC_DCL void FDECL(get_room_loc, (schar *, schar *, struct mkroom *));
STATIC_DCL void FDECL(get_free_room_loc, (schar *, schar *,
                                          struct mkroom *, packed_coord));
STATIC_DCL boolean FDECL(create_subroom, (struct mkroom *, XCHAR_P, XCHAR_P,
                                          XCHAR_P, XCHAR_P, XCHAR_P, XCHAR_P));
STATIC_DCL void FDECL(create_door, (room_door *, struct mkroom *));
STATIC_DCL void FDECL(create_trap, (spltrap *, struct mkroom *));
STATIC_DCL int FDECL(noncoalignment, (ALIGNTYP_P));
STATIC_DCL boolean FDECL(m_bad_boulder_spot, (int, int));
STATIC_DCL int FDECL(pm_to_humidity, (struct permonst *));
STATIC_DCL void FDECL(create_monster, (monster *, struct mkroom *));
STATIC_DCL void FDECL(create_object, (object *, struct mkroom *));
STATIC_DCL void FDECL(create_altar, (altar *, struct mkroom *));
STATIC_DCL void FDECL(replace_terrain, (replaceterrain *, struct mkroom *));
STATIC_DCL boolean FDECL(search_door, (struct mkroom *,
                                       xchar *, xchar *, XCHAR_P, int));
STATIC_DCL void NDECL(fix_stair_rooms);
STATIC_DCL void FDECL(create_corridor, (corridor *));
STATIC_DCL struct mkroom *FDECL(build_room, (room *, struct mkroom *));
STATIC_DCL void FDECL(light_region, (region *));
STATIC_DCL void FDECL(wallify_map, (int, int, int, int));
STATIC_DCL void FDECL(maze1xy, (coord *, int));
STATIC_DCL void NDECL(fill_empty_maze);
STATIC_DCL boolean FDECL(sp_level_loader, (dlb *, sp_lev *));
STATIC_DCL boolean FDECL(sp_level_free, (sp_lev *));
STATIC_DCL void FDECL(splev_initlev, (lev_init *));
STATIC_DCL struct sp_frame *FDECL(frame_new, (long));
STATIC_DCL void FDECL(frame_del, (struct sp_frame *));
STATIC_DCL void FDECL(spo_frame_push, (struct sp_coder *));
STATIC_DCL void FDECL(spo_frame_pop, (struct sp_coder *));
STATIC_DCL long FDECL(sp_code_jmpaddr, (long, long));
STATIC_DCL void FDECL(spo_call, (struct sp_coder *));
STATIC_DCL void FDECL(spo_return, (struct sp_coder *));
STATIC_DCL void FDECL(spo_end_moninvent, (struct sp_coder *));
STATIC_DCL void FDECL(spo_pop_container, (struct sp_coder *));
STATIC_DCL void FDECL(spo_message, (struct sp_coder *));
STATIC_DCL void FDECL(spo_monster, (struct sp_coder *));
STATIC_DCL void FDECL(spo_object, (struct sp_coder *));
STATIC_DCL void FDECL(spo_level_flags, (struct sp_coder *));
STATIC_DCL void FDECL(spo_initlevel, (struct sp_coder *));
STATIC_DCL void FDECL(spo_engraving, (struct sp_coder *));
STATIC_DCL void FDECL(spo_mineralize, (struct sp_coder *));
STATIC_DCL void FDECL(spo_room, (struct sp_coder *));
STATIC_DCL void FDECL(spo_endroom, (struct sp_coder *));
STATIC_DCL void FDECL(spo_stair, (struct sp_coder *));
STATIC_DCL void FDECL(spo_ladder, (struct sp_coder *));
STATIC_DCL void FDECL(spo_grave, (struct sp_coder *));
STATIC_DCL void FDECL(spo_altar, (struct sp_coder *));
STATIC_DCL void FDECL(spo_trap, (struct sp_coder *));
STATIC_DCL void FDECL(spo_gold, (struct sp_coder *));
STATIC_DCL void FDECL(spo_corridor, (struct sp_coder *));
STATIC_DCL void FDECL(selection_setpoint, (int, int, struct opvar *, XCHAR_P));
STATIC_DCL struct opvar *FDECL(selection_not, (struct opvar *));
STATIC_DCL struct opvar *FDECL(selection_logical_oper, (struct opvar *,
                                                     struct opvar *, CHAR_P));
STATIC_DCL struct opvar *FDECL(selection_filter_mapchar, (struct opvar *,
                                                          struct opvar *));
STATIC_DCL void FDECL(selection_filter_percent, (struct opvar *, int));
STATIC_DCL int FDECL(selection_rndcoord, (struct opvar *, schar *, schar *,
                                          BOOLEAN_P));
STATIC_DCL void FDECL(selection_do_grow, (struct opvar *, int));
STATIC_DCL int FDECL(floodfillchk_match_under, (int, int));
STATIC_DCL int FDECL(floodfillchk_match_accessible, (int, int));
STATIC_DCL boolean FDECL(sel_flood_havepoint, (int, int,
                                               xchar *, xchar *, int));
STATIC_DCL void FDECL(selection_do_ellipse, (struct opvar *, int, int,
                                             int, int, int));
STATIC_DCL long FDECL(line_dist_coord, (long, long, long, long, long, long));
STATIC_DCL void FDECL(selection_do_gradient, (struct opvar *, long, long, long,
                                              long, long, long, long, long));
STATIC_DCL void FDECL(selection_do_line, (SCHAR_P, SCHAR_P, SCHAR_P, SCHAR_P,
                                          struct opvar *));
STATIC_DCL void FDECL(selection_do_randline, (SCHAR_P, SCHAR_P, SCHAR_P,
                                              SCHAR_P, SCHAR_P, SCHAR_P,
                                              struct opvar *));
STATIC_DCL void FDECL(selection_iterate, (struct opvar *, select_iter_func,
                                          genericptr_t));
STATIC_DCL void FDECL(sel_set_ter, (int, int, genericptr_t));
STATIC_DCL void FDECL(sel_set_feature, (int, int, genericptr_t));
STATIC_DCL void FDECL(sel_set_door, (int, int, genericptr_t));
STATIC_DCL void FDECL(spo_door, (struct sp_coder *));
STATIC_DCL void FDECL(spo_feature, (struct sp_coder *));
STATIC_DCL void FDECL(spo_terrain, (struct sp_coder *));
STATIC_DCL void FDECL(spo_replace_terrain, (struct sp_coder *));
STATIC_DCL boolean FDECL(generate_way_out_method, (int, int, struct opvar *));
STATIC_DCL void NDECL(ensure_way_out);
STATIC_DCL void FDECL(spo_levregion, (struct sp_coder *));
STATIC_DCL void FDECL(spo_region, (struct sp_coder *));
STATIC_DCL void FDECL(spo_drawbridge, (struct sp_coder *));
STATIC_DCL void FDECL(spo_mazewalk, (struct sp_coder *));
STATIC_DCL void FDECL(spo_wall_property, (struct sp_coder *));
STATIC_DCL void FDECL(spo_room_door, (struct sp_coder *));
STATIC_DCL void FDECL(sel_set_wallify, (int, int, genericptr_t));
STATIC_DCL void FDECL(spo_wallify, (struct sp_coder *));
STATIC_DCL void FDECL(spo_map, (struct sp_coder *));
STATIC_DCL void FDECL(spo_jmp, (struct sp_coder *, sp_lev *));
STATIC_DCL void FDECL(spo_conditional_jump, (struct sp_coder *, sp_lev *));
STATIC_DCL void FDECL(spo_var_init, (struct sp_coder *));
#if 0
STATIC_DCL long FDECL(opvar_array_length, (struct sp_coder *));
#endif /*0*/
STATIC_DCL void FDECL(spo_shuffle_array, (struct sp_coder *));
STATIC_DCL boolean FDECL(sp_level_coder, (sp_lev *));

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

#define Fread (void) dlb_fread
#define Fgetc (schar) dlb_fgetc
#define New(type) (type *) alloc(sizeof(type))
#define NewTab(type, size) (type **) alloc(sizeof(type *) * (unsigned) size)
#define Free(ptr) if (ptr) free((genericptr_t) (ptr))

extern struct engr *head_engr;

extern int min_rx, max_rx, min_ry, max_ry; /* from mkmap.c */

/* positions touched by level elements explicitly defined in the des-file */
static char SpLev_Map[COLNO][ROWNO];

static aligntyp ralign[3] = { AM_CHAOTIC, AM_NEUTRAL, AM_LAWFUL };
static NEARDATA xchar xstart, ystart;
static NEARDATA char xsize, ysize;

char *lev_message = 0;
lev_region *lregions = 0;
int num_lregions = 0;

static boolean splev_init_present = FALSE;
static boolean icedpools = FALSE;
static int mines_prize_count = 0, soko_prize_count = 0; /* achievements */

static struct obj *container_obj[MAX_CONTAINMENT];
static int container_idx = 0;
static struct monst *invent_carrying_monster = NULL;

#define SPLEV_STACK_RESERVE 128

void
solidify_map()
{
    xchar x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (IS_STWALL(levl[x][y].typ) && !SpLev_Map[x][y])
                levl[x][y].wall_info |= (W_NONDIGGABLE | W_NONPASSWALL);
}

void
splev_stack_init(st)
struct splevstack *st;
{
    if (st) {
        st->depth = 0;
        st->depth_alloc = SPLEV_STACK_RESERVE;
        st->stackdata =
           (struct opvar **) alloc(st->depth_alloc * sizeof (struct opvar *));
    }
}

void
splev_stack_done(st)
struct splevstack *st;
{
    if (st) {
        int i;

        if (st->stackdata && st->depth) {
            for (i = 0; i < st->depth; i++) {
                switch (st->stackdata[i]->spovartyp) {
                default:
                case SPOVAR_NULL:
                case SPOVAR_COORD:
                case SPOVAR_REGION:
                case SPOVAR_MAPCHAR:
                case SPOVAR_MONST:
                case SPOVAR_OBJ:
                case SPOVAR_INT:
                    break;
                case SPOVAR_VARIABLE:
                case SPOVAR_STRING:
                case SPOVAR_SEL:
                    Free(st->stackdata[i]->vardata.str);
                    st->stackdata[i]->vardata.str = NULL;
                    break;
                }
                Free(st->stackdata[i]);
                st->stackdata[i] = NULL;
            }
        }
        Free(st->stackdata);
        st->stackdata = NULL;
        st->depth = st->depth_alloc = 0;
        Free(st);
    }
}

void
splev_stack_push(st, v)
struct splevstack *st;
struct opvar *v;
{
    if (!st || !v)
        return;
    if (!st->stackdata)
        panic("splev_stack_push: no stackdata allocated?");

    if (st->depth >= st->depth_alloc) {
        struct opvar **tmp = (struct opvar **) alloc(
           (st->depth_alloc + SPLEV_STACK_RESERVE) * sizeof (struct opvar *));

        (void) memcpy(tmp, st->stackdata,
                      st->depth_alloc * sizeof(struct opvar *));
        Free(st->stackdata);
        st->stackdata = tmp;
        st->depth_alloc += SPLEV_STACK_RESERVE;
    }

    st->stackdata[st->depth] = v;
    st->depth++;
}

struct opvar *
splev_stack_pop(st)
struct splevstack *st;
{
    struct opvar *ret = NULL;

    if (!st)
        return ret;
    if (!st->stackdata)
        panic("splev_stack_pop: no stackdata allocated?");

    if (st->depth) {
        st->depth--;
        ret = st->stackdata[st->depth];
        st->stackdata[st->depth] = NULL;
        return ret;
    } else
        impossible("splev_stack_pop: empty stack?");
    return ret;
}

struct splevstack *
splev_stack_reverse(st)
struct splevstack *st;
{
    long i;
    struct opvar *tmp;

    if (!st)
        return NULL;
    if (!st->stackdata)
        panic("splev_stack_reverse: no stackdata allocated?");
    for (i = 0; i < (st->depth / 2); i++) {
        tmp = st->stackdata[i];
        st->stackdata[i] = st->stackdata[st->depth - i - 1];
        st->stackdata[st->depth - i - 1] = tmp;
    }
    return st;
}

#define OV_typ(o) (o->spovartyp)
#define OV_i(o) (o->vardata.l)
#define OV_s(o) (o->vardata.str)

#define OV_pop_i(x) (x = splev_stack_getdat(coder, SPOVAR_INT))
#define OV_pop_c(x) (x = splev_stack_getdat(coder, SPOVAR_COORD))
#define OV_pop_r(x) (x = splev_stack_getdat(coder, SPOVAR_REGION))
#define OV_pop_s(x) (x = splev_stack_getdat(coder, SPOVAR_STRING))
#define OV_pop(x) (x = splev_stack_getdat_any(coder))
#define OV_pop_typ(x, typ) (x = splev_stack_getdat(coder, typ))

struct opvar *
opvar_new_str(s)
char *s;
{
    struct opvar *tmpov = (struct opvar *) alloc(sizeof (struct opvar));

    tmpov->spovartyp = SPOVAR_STRING;
    if (s) {
        int len = strlen(s);
        tmpov->vardata.str = (char *) alloc(len + 1);
        (void) memcpy((genericptr_t) tmpov->vardata.str, (genericptr_t) s,
                      len);
        tmpov->vardata.str[len] = '\0';
    } else
        tmpov->vardata.str = NULL;
    return tmpov;
}

struct opvar *
opvar_new_int(i)
long i;
{
    struct opvar *tmpov = (struct opvar *) alloc(sizeof (struct opvar));

    tmpov->spovartyp = SPOVAR_INT;
    tmpov->vardata.l = i;
    return tmpov;
}

struct opvar *
opvar_new_coord(x, y)
int x, y;
{
    struct opvar *tmpov = (struct opvar *) alloc(sizeof (struct opvar));

    tmpov->spovartyp = SPOVAR_COORD;
    tmpov->vardata.l = SP_COORD_PACK(x, y);
    return tmpov;
}

#if 0
struct opvar *
opvar_new_region(x1,y1,x2,y2)
     int x1,y1,x2,y2;
{
    struct opvar *tmpov = (struct opvar *) alloc(sizeof (struct opvar));

    tmpov->spovartyp = SPOVAR_REGION;
    tmpov->vardata.l = SP_REGION_PACK(x1,y1,x2,y2);
    return tmpov;
}
#endif /*0*/

void
opvar_free_x(ov)
struct opvar *ov;
{
    if (!ov)
        return;
    switch (ov->spovartyp) {
    case SPOVAR_COORD:
    case SPOVAR_REGION:
    case SPOVAR_MAPCHAR:
    case SPOVAR_MONST:
    case SPOVAR_OBJ:
    case SPOVAR_INT:
        break;
    case SPOVAR_VARIABLE:
    case SPOVAR_STRING:
    case SPOVAR_SEL:
        Free(ov->vardata.str);
        break;
    default:
        impossible("Unknown opvar value type (%i)!", ov->spovartyp);
    }
    Free(ov);
}

/*
 * Name of current function for use in messages:
 * __func__     -- C99 standard;
 * __FUNCTION__ -- gcc extension, starting before C99 and continuing after;
 *                 picked up by other compilers (or vice versa?);
 * __FUNC__     -- supported by Borland;
 * nhFunc       -- slightly intrusive but fully portable nethack construct
 *                 for any version of any compiler.
 */
#define opvar_free(ov)                                    \
    do {                                                  \
        if (ov) {                                         \
            opvar_free_x(ov);                             \
            ov = NULL;                                    \
        } else                                            \
            impossible("opvar_free(), %s", nhFunc);       \
    } while (0)

struct opvar *
opvar_clone(ov)
struct opvar *ov;
{
    struct opvar *tmpov;

    if (!ov)
        panic("no opvar to clone");
    tmpov = (struct opvar *) alloc(sizeof(struct opvar));
    tmpov->spovartyp = ov->spovartyp;
    switch (ov->spovartyp) {
    case SPOVAR_COORD:
    case SPOVAR_REGION:
    case SPOVAR_MAPCHAR:
    case SPOVAR_MONST:
    case SPOVAR_OBJ:
    case SPOVAR_INT:
        tmpov->vardata.l = ov->vardata.l;
        break;
    case SPOVAR_VARIABLE:
    case SPOVAR_STRING:
    case SPOVAR_SEL:
        tmpov->vardata.str = dupstr(ov->vardata.str);
        break;
    default:
        impossible("Unknown push value type (%i)!", ov->spovartyp);
    }
    return tmpov;
}

struct opvar *
opvar_var_conversion(coder, ov)
struct sp_coder *coder;
struct opvar *ov;
{
    static const char nhFunc[] = "opvar_var_conversion";
    struct splev_var *tmp;
    struct opvar *tmpov;
    struct opvar *array_idx = NULL;

    if (!coder || !ov)
        return NULL;
    if (ov->spovartyp != SPOVAR_VARIABLE)
        return ov;
    tmp = coder->frame->variables;
    while (tmp) {
        if (!strcmp(tmp->name, OV_s(ov))) {
            if ((tmp->svtyp & SPOVAR_ARRAY)) {
                array_idx = opvar_var_conversion(coder,
                                               splev_stack_pop(coder->stack));
                if (!array_idx || OV_typ(array_idx) != SPOVAR_INT)
                    panic("array idx not an int");
                if (tmp->array_len < 1)
                    panic("array len < 1");
                OV_i(array_idx) = (OV_i(array_idx) % tmp->array_len);
                tmpov = opvar_clone(tmp->data.arrayvalues[OV_i(array_idx)]);
                opvar_free(array_idx);
                return tmpov;
            } else {
                tmpov = opvar_clone(tmp->data.value);
                return tmpov;
            }
        }
        tmp = tmp->next;
    }
    return NULL;
}

struct splev_var *
opvar_var_defined(coder, name)
struct sp_coder *coder;
char *name;
{
    struct splev_var *tmp;

    if (!coder)
        return NULL;
    tmp = coder->frame->variables;
    while (tmp) {
        if (!strcmp(tmp->name, name))
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

struct opvar *
splev_stack_getdat(coder, typ)
struct sp_coder *coder;
xchar typ;
{
    static const char nhFunc[] = "splev_stack_getdat";
    if (coder && coder->stack) {
        struct opvar *tmp = splev_stack_pop(coder->stack);
        struct opvar *ret = NULL;

        if (!tmp)
            panic("no value type %i in stack.", typ);
        if (tmp->spovartyp == SPOVAR_VARIABLE) {
            ret = opvar_var_conversion(coder, tmp);
            opvar_free(tmp);
            tmp = ret;
        }
        if (tmp->spovartyp == typ)
            return tmp;
        else opvar_free(tmp);
    }
    return NULL;
}

struct opvar *
splev_stack_getdat_any(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "splev_stack_getdat_any";
    if (coder && coder->stack) {
        struct opvar *tmp = splev_stack_pop(coder->stack);
        if (tmp && tmp->spovartyp == SPOVAR_VARIABLE) {
            struct opvar *ret = opvar_var_conversion(coder, tmp);
            opvar_free(tmp);
            return ret;
        }
        return tmp;
    }
    return NULL;
}

void
variable_list_del(varlist)
struct splev_var *varlist;
{
    static const char nhFunc[] = "variable_list_del";
    struct splev_var *tmp = varlist;

    if (!tmp)
        return;
    while (tmp) {
        Free(tmp->name);
        if ((tmp->svtyp & SPOVAR_ARRAY)) {
            long idx = tmp->array_len;

            while (idx-- > 0) {
                opvar_free(tmp->data.arrayvalues[idx]);
            };
            Free(tmp->data.arrayvalues);
        } else {
            opvar_free(tmp->data.value);
        }
        tmp = varlist->next;
        Free(varlist);
        varlist = tmp;
    }
}

void
lvlfill_maze_grid(x1, y1, x2, y2, filling)
int x1, y1, x2, y2;
schar filling;
{
    int x, y;

    for (x = x1; x <= x2; x++)
        for (y = y1; y <= y2; y++) {
            if (level.flags.corrmaze)
                levl[x][y].typ = STONE;
            else
                levl[x][y].typ = (y < 2 || ((x % 2) && (y % 2))) ? STONE
                                                                 : filling;
        }
}

void
lvlfill_solid(filling, lit)
schar filling;
schar lit;
{
    int x, y;

    for (x = 2; x <= x_maze_max; x++)
        for (y = 0; y <= y_maze_max; y++) {
            SET_TYPLIT(x, y, filling, lit);
        }
}

/*
 * Make walls of the area (x1, y1, x2, y2) non diggable/non passwall-able
 */
STATIC_OVL void
set_wall_property(x1, y1, x2, y2, prop)
xchar x1, y1, x2, y2;
int prop;
{
    register xchar x, y;
    struct rm *lev;

    x1 = max(x1, 1);
    x2 = min(x2, COLNO - 1);
    y1 = max(y1, 0);
    y2 = min(y2, ROWNO - 1);
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++) {
            lev = &levl[x][y];
            if (IS_STWALL(lev->typ) || IS_TREE(lev->typ)
                /* 3.6.2: made iron bars eligible to be flagged nondiggable
                   (checked by chewing(hack.c) and zap_over_floor(zap.c)) */
                || lev->typ == IRONBARS)
                lev->wall_info |= prop;
        }
}

STATIC_OVL void
shuffle_alignments()
{
    int i;
    aligntyp atmp;

    /* shuffle 3 alignments */
    i = rn2(3);
    atmp = ralign[2];
    ralign[2] = ralign[i];
    ralign[i] = atmp;
    if (rn2(2)) {
        atmp = ralign[1];
        ralign[1] = ralign[0];
        ralign[0] = atmp;
    }
}

/*
 * Count the different features (sinks, fountains) in the level.
 */
STATIC_OVL void
count_features()
{
    xchar x, y;

    level.flags.nfountains = level.flags.nsinks = 0;
    for (y = 0; y < ROWNO; y++)
        for (x = 0; x < COLNO; x++) {
            int typ = levl[x][y].typ;
            if (typ == FOUNTAIN)
                level.flags.nfountains++;
            else if (typ == SINK)
                level.flags.nsinks++;
        }
}

void
remove_boundary_syms()
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
        for (x = 0; x < x_maze_max; x++)
            for (y = 0; y < y_maze_max; y++)
                if ((levl[x][y].typ == CROSSWALL) && SpLev_Map[x][y])
                    levl[x][y].typ = ROOM;
    }
}

/* used by sel_set_door() and link_doors_rooms() */
STATIC_OVL void
set_door_orientation(x, y)
int x, y;
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

STATIC_OVL void
maybe_add_door(x, y, droom)
int x, y;
struct mkroom *droom;
{
    if (droom->hx >= 0 && doorindex < DOORMAX && inside_room(droom, x, y))
        add_door(x, y, droom);
}

STATIC_OVL void
link_doors_rooms()
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

                for (tmpi = 0; tmpi < nroom; tmpi++) {
                    maybe_add_door(x, y, &rooms[tmpi]);
                    for (m = 0; m < rooms[tmpi].nsubrooms; m++) {
                        maybe_add_door(x, y, rooms[tmpi].sbrooms[m]);
                    }
                }
            }
}

void
fill_rooms()
{
    int tmpi, m;

    for (tmpi = 0; tmpi < nroom; tmpi++) {
        if (rooms[tmpi].needfill)
            fill_room(&rooms[tmpi], (rooms[tmpi].needfill == 2));
        for (m = 0; m < rooms[tmpi].nsubrooms; m++)
            if (rooms[tmpi].sbrooms[m]->needfill)
                fill_room(rooms[tmpi].sbrooms[m], FALSE);
    }
}

/*
 * Choose randomly the state (nodoor, open, closed or locked) for a door
 */
STATIC_OVL int
rnddoor()
{
    int i = 1 << rn2(5);

    i >>= 1;
    return i;
}

/*
 * Select a random trap
 */
STATIC_OVL int
rndtrap()
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
            if (level.flags.noteleport)
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
 *      The "humidity" flag is used to insure that engravings aren't
 *      created underwater, or eels on dry land.
 */
STATIC_OVL void
get_location(x, y, humidity, croom)
schar *x, *y;
int humidity;
struct mkroom *croom;
{
    int cpt = 0;
    int mx, my, sx, sy;

    if (croom) {
        mx = croom->lx;
        my = croom->ly;
        sx = croom->hx - mx + 1;
        sy = croom->hy - my + 1;
    } else {
        mx = xstart;
        my = ystart;
        sx = xsize;
        sy = ysize;
    }

    if (*x >= 0) { /* normal locations */
        *x += mx;
        *y += my;
    } else { /* random location */
        do {
            if (croom) { /* handle irregular areas */
                coord tmpc;
                somexy(croom, &tmpc);
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
            *x = x_maze_max;
            *y = y_maze_max;
        } else {
            *x = *y = -1;
        }
    }
}

STATIC_OVL boolean
is_ok_location(x, y, humidity)
register schar x, y;
register int humidity;
{
    register int typ;

    if (Is_waterlevel(&u.uz))
        return TRUE; /* accept any spot */

    /* TODO: Should perhaps check if wall is diggable/passwall? */
    if (humidity & ANY_LOC)
        return TRUE;

    if ((humidity & SOLID) && IS_ROCK(levl[x][y].typ))
        return TRUE;

    if (humidity & DRY) {
        typ = levl[x][y].typ;
        if (typ == ROOM || typ == AIR || typ == CLOUD || typ == ICE
            || typ == CORR)
            return TRUE;
    }
    if ((humidity & SPACELOC) && SPACE_POS(levl[x][y].typ))
        return TRUE;
    if ((humidity & WET) && is_pool(x, y))
        return TRUE;
    if ((humidity & HOT) && is_lava(x, y))
        return TRUE;
    return FALSE;
}

unpacked_coord
get_unpacked_coord(loc, defhumidity)
long loc;
int defhumidity;
{
    static unpacked_coord c;

    if (loc & SP_COORD_IS_RANDOM) {
        c.x = c.y = -1;
        c.is_random = 1;
        c.getloc_flags = (loc & ~SP_COORD_IS_RANDOM);
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

STATIC_OVL void
get_location_coord(x, y, humidity, croom, crd)
schar *x, *y;
int humidity;
struct mkroom *croom;
long crd;
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

STATIC_OVL void
get_room_loc(x, y, croom)
schar *x, *y;
struct mkroom *croom;
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
STATIC_OVL void
get_free_room_loc(x, y, croom, pos)
schar *x, *y;
struct mkroom *croom;
packed_coord pos;
{
    schar try_x, try_y;
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
check_room(lowx, ddx, lowy, ddy, vault)
xchar *lowx, *ddx, *lowy, *ddy;
boolean vault;
{
    register int x, y, hix = *lowx + *ddx, hiy = *lowy + *ddy;
    register struct rm *lev;
    int xlim, ylim, ymax;

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
            if (lev++->typ) {
                if (!vault) {
                    debugpline2("strange area [%d,%d] in check_room.", x, y);
                }
                if (!rn2(3))
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
    return TRUE;
}

/*
 * Create a new room.
 * This is still very incomplete...
 */
boolean
create_room(x, y, w, h, xal, yal, rtype, rlit)
xchar x, y;
xchar w, h;
xchar xal, yal;
xchar rtype, rlit;
{
    xchar xabs, yabs;
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
    if (rlit == -1)
        rlit = (rnd(1 + abs(depth(&u.uz))) < 11 && rn2(77)) ? TRUE : FALSE;

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
            if (ly == 0 && hy >= (ROWNO - 1) && (!nroom || !rn2(nroom))
                && (yabs + dy > ROWNO / 2)) {
                yabs = rn1(3, 2);
                if (nroom < 4 && dy > 1)
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
        }
    } while (++trycnt <= 100 && !r1);
    if (!r1) { /* creation of room failed ? */
        return FALSE;
    }
    split_rects(r1, &r2);

    if (!vault) {
        smeq[nroom] = nroom;
        add_room(xabs, yabs, xabs + wtmp - 1, yabs + htmp - 1, rlit, rtype,
                 FALSE);
    } else {
        rooms[nroom].lx = xabs;
        rooms[nroom].ly = yabs;
    }
    return TRUE;
}

/*
 * Create a subroom in room proom at pos x,y with width w & height h.
 * x & y are relative to the parent room.
 */
STATIC_OVL boolean
create_subroom(proom, x, y, w, h, rtype, rlit)
struct mkroom *proom;
xchar x, y;
xchar w, h;
xchar rtype, rlit;
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
    if (rlit == -1)
        rlit = (rnd(1 + abs(depth(&u.uz))) < 11 && rn2(77)) ? TRUE : FALSE;
    add_subroom(proom, proom->lx + x, proom->ly + y, proom->lx + x + w - 1,
                proom->ly + y + h - 1, rlit, rtype, FALSE);
    return TRUE;
}

/*
 * Create a new door in a room.
 * It's placed on a wall (north, south, east or west).
 */
STATIC_OVL void
create_door(dd, broom)
room_door *dd;
struct mkroom *broom;
{
    int x = 0, y = 0;
    int trycnt = 0, wtry = 0;

    if (dd->secret == -1)
        dd->secret = rn2(2);

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

    do {
        register int dwall, dpos;

        dwall = dd->wall;
        if (dwall == -1) /* The wall is RANDOM */
            dwall = 1 << rn2(4);

        dpos = dd->pos;

        /* Convert wall and pos into an absolute coordinate! */
        wtry = rn2(4);
        switch (wtry) {
        case 0:
            if (!(dwall & W_NORTH))
                goto redoloop;
            y = broom->ly - 1;
            x = broom->lx
                + ((dpos == -1) ? rn2(1 + (broom->hx - broom->lx)) : dpos);
            if (IS_ROCK(levl[x][y - 1].typ))
                goto redoloop;
            goto outdirloop;
        case 1:
            if (!(dwall & W_SOUTH))
                goto redoloop;
            y = broom->hy + 1;
            x = broom->lx
                + ((dpos == -1) ? rn2(1 + (broom->hx - broom->lx)) : dpos);
            if (IS_ROCK(levl[x][y + 1].typ))
                goto redoloop;
            goto outdirloop;
        case 2:
            if (!(dwall & W_WEST))
                goto redoloop;
            x = broom->lx - 1;
            y = broom->ly
                + ((dpos == -1) ? rn2(1 + (broom->hy - broom->ly)) : dpos);
            if (IS_ROCK(levl[x - 1][y].typ))
                goto redoloop;
            goto outdirloop;
        case 3:
            if (!(dwall & W_EAST))
                goto redoloop;
            x = broom->hx + 1;
            y = broom->ly
                + ((dpos == -1) ? rn2(1 + (broom->hy - broom->ly)) : dpos);
            if (IS_ROCK(levl[x + 1][y].typ))
                goto redoloop;
            goto outdirloop;
        default:
            x = y = 0;
            panic("create_door: No wall for door!");
            goto outdirloop;
        }
    outdirloop:
        if (okdoor(x, y))
            break;
    redoloop:
        ;
    } while (++trycnt <= 100);
    if (trycnt > 100) {
        impossible("create_door: Can't find a proper place!");
        return;
    }
    levl[x][y].typ = (dd->secret ? SDOOR : DOOR);
    levl[x][y].doormask = dd->mask;
}

/*
 * Create a secret door in croom on any one of the specified walls.
 */
void
create_secret_door(croom, walls)
struct mkroom *croom;
xchar walls; /* any of W_NORTH | W_SOUTH | W_EAST | W_WEST (or W_ANY) */
{
    xchar sx, sy; /* location of the secret door */
    int count;

    for (count = 0; count < 100; count++) {
        sx = rn1(croom->hx - croom->lx + 1, croom->lx);
        sy = rn1(croom->hy - croom->ly + 1, croom->ly);

        switch (rn2(4)) {
        case 0: /* top */
            if (!(walls & W_NORTH))
                continue;
            sy = croom->ly - 1;
            break;
        case 1: /* bottom */
            if (!(walls & W_SOUTH))
                continue;
            sy = croom->hy + 1;
            break;
        case 2: /* left */
            if (!(walls & W_EAST))
                continue;
            sx = croom->lx - 1;
            break;
        case 3: /* right */
            if (!(walls & W_WEST))
                continue;
            sx = croom->hx + 1;
            break;
        }

        if (okdoor(sx, sy)) {
            levl[sx][sy].typ = SDOOR;
            levl[sx][sy].doormask = D_CLOSED;
            return;
        }
    }

    impossible("couldn't create secret door on any walls 0x%x", walls);
}

/*
 * Create a trap in a room.
 */
STATIC_OVL void
create_trap(t, croom)
spltrap *t;
struct mkroom *croom;
{
    schar x = -1, y = -1;
    coord tm;

    if (croom)
        get_free_room_loc(&x, &y, croom, t->coord);
    else {
        int trycnt = 0;
        do {
            get_location_coord(&x, &y, DRY, croom, t->coord);
        } while ((levl[x][y].typ == STAIRS || levl[x][y].typ == LADDER)
                 && ++trycnt <= 100);
        if (trycnt > 100)
            return;
    }

    tm.x = x;
    tm.y = y;

    mktrap(t->type, 1, (struct mkroom *) 0, &tm);
}

/*
 * Create a monster in a room.
 */
STATIC_OVL int
noncoalignment(alignment)
aligntyp alignment;
{
    int k;

    k = rn2(2);
    if (!alignment)
        return (k ? -1 : 1);
    return (k ? -alignment : 0);
}

/* attempt to screen out locations where a mimic-as-boulder shouldn't occur */
STATIC_OVL boolean
m_bad_boulder_spot(x, y)
int x, y;
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

STATIC_OVL int
pm_to_humidity(pm)
struct permonst *pm;
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
    if (flaming(pm))
        loc |= HOT;
    return loc;
}

STATIC_OVL void
create_monster(m, croom)
monster *m;
struct mkroom *croom;
{
    struct monst *mtmp;
    schar x, y;
    char class;
    aligntyp amask;
    coord cc;
    struct permonst *pm;
    unsigned g_mvflags;

    if (m->class >= 0)
        class = (char) def_char_to_monclass((char) m->class);
    else
        class = 0;

    if (class == MAXMCLASSES)
        panic("create_monster: unknown monster class '%c'", m->class);

    amask = (m->align == AM_SPLEV_CO)
               ? Align2amask(u.ualignbase[A_ORIGINAL])
               : (m->align == AM_SPLEV_NONCO)
                  ? Align2amask(noncoalignment(u.ualignbase[A_ORIGINAL]))
                  : (m->align <= -(MAX_REGISTERS + 1))
                     ? induced_align(80)
                     : (m->align < 0 ? ralign[-m->align - 1] : m->align);

    if (!class)
        pm = (struct permonst *) 0;
    else if (m->id != NON_PM) {
        pm = &mons[m->id];
        g_mvflags = (unsigned) mvitals[monsndx(pm)].mvflags;
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

    if (m->align != -(MAX_REGISTERS + 2))
        mtmp = mk_roamer(pm, Amask2align(amask), x, y, m->peaceful);
    else if (PM_ARCHEOLOGIST <= m->id && m->id <= PM_WIZARD)
        mtmp = mk_mplayer(pm, x, y, FALSE);
    else
        mtmp = makemon(pm, x, y, NO_MM_FLAGS);

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
                int mndx;

                if (!strcmpi(m->appear_as.str, "random"))
                    mndx = select_newcham_form(mtmp);
                else
                    mndx = name_to_mon(m->appear_as.str);

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
                    set_mon_data(mtmp, mdat, 0);
                    if (emits_light(olddata) != emits_light(mtmp->data)) {
                        /* used to give light, now doesn't, or vice versa,
                           or light's range has changed */
                        if (emits_light(olddata))
                            del_light_source(LS_MONSTER, (genericptr_t) mtmp);
                        if (emits_light(mtmp->data))
                            new_light_source(mtmp->mx, mtmp->my,
                                             emits_light(mtmp->data),
                                             LS_MONSTER, (genericptr_t) mtmp);
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

        if (m->peaceful >= 0) {
            mtmp->mpeaceful = m->peaceful;
            /* changed mpeaceful again; have to reset malign */
            set_malign(mtmp);
        }
        if (m->asleep >= 0) {
#ifdef UNIXPC
            /* optimizer bug strikes again */
            if (m->asleep)
                mtmp->msleeping = 1;
            else
                mtmp->msleeping = 0;
#else
            mtmp->msleeping = m->asleep;
#endif
        }
        if (m->seentraps)
            mtmp->mtrapseen = m->seentraps;
        if (m->female)
            mtmp->female = 1;
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

        if (m->has_invent) {
            discard_minvent(mtmp);
            invent_carrying_monster = mtmp;
        }
    }
}

/*
 * Create an object in a room.
 */
STATIC_OVL void
create_object(o, croom)
object *o;
struct mkroom *croom;
{
    struct obj *otmp;
    schar x, y;
    char c;
    boolean named; /* has a name been supplied in level description? */

    named = o->name.str ? TRUE : FALSE;

    get_location_coord(&x, &y, DRY, croom, o->coord);

    if (o->class >= 0)
        c = o->class;
    else
        c = 0;

    if (!c)
        otmp = mkobj_at(RANDOM_CLASS, x, y, !named);
    else if (o->id != -1)
        otmp = mksobj_at(o->id, x, y, TRUE, !named);
    else {
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
    case 1:
        bless(otmp);
        break; /* BLESSED */
    case 2:
        unbless(otmp);
        uncurse(otmp);
        break; /* uncursed */
    case 3:
        curse(otmp);
        break; /* CURSED */
    default:
        break; /* Otherwise it's random and we're happy
                * with what mkobj gave us! */
    }

    /* corpsenm is "empty" if -1, random if -2, otherwise specific */
    if (o->corpsenm != NON_PM) {
        if (o->corpsenm == NON_PM - 1)
            set_corpsenm(otmp, rndmonnum());
        else
            set_corpsenm(otmp, o->corpsenm);
    }
    /* set_corpsenm() took care of egg hatch and corpse timers */

    if (named)
        otmp = oname(otmp, o->name.str);

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
#ifdef INVISIBLE_OBJECTS
    if (o->invis)
        otmp->oinvis = 1;
#endif

    if (o->quan > 0 && objects[otmp->otyp].oc_merge) {
        otmp->quan = o->quan;
        otmp->owt = weight(otmp);
    }

    /* contents */
    if (o->containment & SP_OBJ_CONTENT) {
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
            } else {
                int ci;
                struct obj *objcheck = otmp;
                int inuse = -1;

                for (ci = 0; ci < container_idx; ci++)
                    if (container_obj[ci] == objcheck)
                        inuse = ci;
                remove_object(otmp);
                if (mpickobj(invent_carrying_monster, otmp)) {
                    if (inuse > -1) {
                        impossible(
                     "container given to monster was merged or deallocated.");
                        for (ci = inuse; ci < container_idx - 1; ci++)
                            container_obj[ci] = container_obj[ci + 1];
                        container_obj[container_idx] = NULL;
                        container_idx--;
                    }
                    /* we lost track of it. */
                    return;
                }
            }
        } else {
            struct obj *cobj = container_obj[container_idx - 1];

            remove_object(otmp);
            if (cobj) {
                (void) add_to_container(cobj, otmp);
                cobj->owt = weight(cobj);
            } else {
                obj_extract_self(otmp);
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
        struct monst *was;
        struct obj *obj;
        int wastyp;
        int i = 0; /* prevent endless loop in case makemon always fails */

        /* Named random statues are of player types, and aren't stone-
         * resistant (if they were, we'd have to reset the name as well as
         * setting corpsenm).
         */
        for (wastyp = otmp->corpsenm; i < 1000; i++, wastyp = rndmonnum()) {
            /* makemon without rndmonst() might create a group */
            was = makemon(&mons[wastyp], 0, 0, MM_NOCOUNTBIRTH);
            if (was) {
                if (!resists_ston(was)) {
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

    if (o->id != -1) {
        static const char prize_warning[] = "multiple prizes on %s level";

        /* if this is a specific item of the right type and it is being
           created on the right level, flag it as the designated item
           used to detect a special achievement (to whit, reaching and
           exploring the target level, although the exploration part
           might be short-circuited if a monster brings object to hero) */
        if (Is_mineend_level(&u.uz)) {
            if (otmp->otyp == iflags.mines_prize_type) {
                otmp->record_achieve_special = MINES_PRIZE;
                if (++mines_prize_count > 1)
                    impossible(prize_warning, "mines end");
            }
        } else if (Is_sokoend_level(&u.uz)) {
            if (otmp->otyp == iflags.soko_prize_type1) {
                otmp->record_achieve_special = SOKO_PRIZE1;
                if (++soko_prize_count > 1)
                    impossible(prize_warning, "sokoban end");
            } else if (otmp->otyp == iflags.soko_prize_type2) {
                otmp->record_achieve_special = SOKO_PRIZE2;
                if (++soko_prize_count > 1)
                    impossible(prize_warning, "sokoban end");
            }
        }
    }

    stackobj(otmp);

    if (o->lit) {
        begin_burn(otmp, FALSE);
    }

    if (o->buried) {
        boolean dealloced;

        (void) bury_an_obj(otmp, &dealloced);
        if (dealloced && container_idx) {
            container_obj[container_idx - 1] = NULL;
        }
    }
}

/*
 * Create an altar in a room.
 */
STATIC_OVL void
create_altar(a, croom)
altar *a;
struct mkroom *croom;
{
    schar sproom, x = -1, y = -1;
    aligntyp amask;
    boolean croom_is_temple = TRUE;
    int oldtyp;

    if (croom) {
        get_free_room_loc(&x, &y, croom, a->coord);
        if (croom->rtype != TEMPLE)
            croom_is_temple = FALSE;
    } else {
        get_location_coord(&x, &y, DRY, croom, a->coord);
        if ((sproom = (schar) *in_rooms(x, y, TEMPLE)) != 0)
            croom = &rooms[sproom - ROOMOFFSET];
        else
            croom_is_temple = FALSE;
    }

    /* check for existing features */
    oldtyp = levl[x][y].typ;
    if (oldtyp == STAIRS || oldtyp == LADDER)
        return;

    /* Is the alignment random ?
     * If so, it's an 80% chance that the altar will be co-aligned.
     *
     * The alignment is encoded as amask values instead of alignment
     * values to avoid conflicting with the rest of the encoding,
     * shared by many other parts of the special level code.
     */
    amask = (a->align == AM_SPLEV_CO)
               ? Align2amask(u.ualignbase[A_ORIGINAL])
               : (a->align == AM_SPLEV_NONCO)
                  ? Align2amask(noncoalignment(u.ualignbase[A_ORIGINAL]))
                  : (a->align == -(MAX_REGISTERS + 1))
                     ? induced_align(80)
                     : (a->align < 0 ? ralign[-a->align - 1] : a->align);

    levl[x][y].typ = ALTAR;
    levl[x][y].altarmask = amask;

    if (a->shrine < 0)
        a->shrine = rn2(2); /* handle random case */

    if (!croom_is_temple || !a->shrine)
        return;

    if (a->shrine) { /* Is it a shrine  or sanctum? */
        priestini(&u.uz, croom, x, y, (a->shrine > 1));
        levl[x][y].altarmask |= AM_SHRINE;
        level.flags.has_temple = TRUE;
    }
}

void
replace_terrain(terr, croom)
replaceterrain *terr;
struct mkroom *croom;
{
    schar x, y, x1, y1, x2, y2;

    if (terr->toter >= MAX_TYPE)
        return;

    x1 = terr->x1;
    y1 = terr->y1;
    get_location(&x1, &y1, ANY_LOC, croom);

    x2 = terr->x2;
    y2 = terr->y2;
    get_location(&x2, &y2, ANY_LOC, croom);

    for (x = max(x1, 0); x <= min(x2, COLNO - 1); x++)
        for (y = max(y1, 0); y <= min(y2, ROWNO - 1); y++)
            if (levl[x][y].typ == terr->fromter && rn2(100) < terr->chance) {
                SET_TYPLIT(x, y, terr->toter, terr->tolit);
            }
}

/*
 * Search for a door in a room on a specified wall.
 */
STATIC_OVL boolean
search_door(croom, x, y, wall, cnt)
struct mkroom *croom;
xchar *x, *y;
xchar wall;
int cnt;
{
    int dx, dy;
    int xx, yy;

    switch (wall) {
    case W_NORTH:
        dy = 0;
        dx = 1;
        xx = croom->lx;
        yy = croom->hy + 1;
        break;
    case W_SOUTH:
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
dig_corridor(org, dest, nxcor, ftyp, btyp)
coord *org, *dest;
boolean nxcor;
schar ftyp, btyp;
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

        if ((dix > diy) && diy && !rn2(dix-diy+1)) {
            dix = 0;
        } else if ((diy > dix) && dix && !rn2(diy-dix+1)) {
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
 * Disgusting hack: since special levels have their rooms filled before
 * sorting the rooms, we have to re-arrange the speed values upstairs_room
 * and dnstairs_room after the rooms have been sorted.  On normal levels,
 * stairs don't get created until _after_ sorting takes place.
 */
STATIC_OVL void
fix_stair_rooms()
{
    int i;
    struct mkroom *croom;

    if (xdnstair
        && !((dnstairs_room->lx <= xdnstair && xdnstair <= dnstairs_room->hx)
             && (dnstairs_room->ly <= ydnstair
                 && ydnstair <= dnstairs_room->hy))) {
        for (i = 0; i < nroom; i++) {
            croom = &rooms[i];
            if ((croom->lx <= xdnstair && xdnstair <= croom->hx)
                && (croom->ly <= ydnstair && ydnstair <= croom->hy)) {
                dnstairs_room = croom;
                break;
            }
        }
        if (i == nroom)
            panic("Couldn't find dnstair room in fix_stair_rooms!");
    }
    if (xupstair
        && !((upstairs_room->lx <= xupstair && xupstair <= upstairs_room->hx)
             && (upstairs_room->ly <= yupstair
                 && yupstair <= upstairs_room->hy))) {
        for (i = 0; i < nroom; i++) {
            croom = &rooms[i];
            if ((croom->lx <= xupstair && xupstair <= croom->hx)
                && (croom->ly <= yupstair && yupstair <= croom->hy)) {
                upstairs_room = croom;
                break;
            }
        }
        if (i == nroom)
            panic("Couldn't find upstair room in fix_stair_rooms!");
    }
}

/*
 * Corridors always start from a door. But it can end anywhere...
 * Basically we search for door coordinates or for endpoints coordinates
 * (from a distance).
 */
STATIC_OVL void
create_corridor(c)
corridor *c;
{
    coord org, dest;

    if (c->src.room == -1) {
        fix_stair_rooms();
        makecorridors(); /*makecorridors(c->src.door);*/
        return;
    }

    if (!search_door(&rooms[c->src.room], &org.x, &org.y, c->src.wall,
                     c->src.door))
        return;

    if (c->dest.room != -1) {
        if (!search_door(&rooms[c->dest.room], &dest.x, &dest.y, c->dest.wall,
                         c->dest.door))
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
fill_room(croom, prefilled)
struct mkroom *croom;
boolean prefilled;
{
    if (!croom || croom->rtype == OROOM)
        return;

    if (!prefilled) {
        int x, y;

        /* Shop ? */
        if (croom->rtype >= SHOPBASE) {
            stock_room(croom->rtype - SHOPBASE, croom);
            level.flags.has_shop = TRUE;
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
        level.flags.has_vault = TRUE;
        break;
    case ZOO:
        level.flags.has_zoo = TRUE;
        break;
    case COURT:
        level.flags.has_court = TRUE;
        break;
    case MORGUE:
        level.flags.has_morgue = TRUE;
        break;
    case BEEHIVE:
        level.flags.has_beehive = TRUE;
        break;
    case BARRACKS:
        level.flags.has_barracks = TRUE;
        break;
    case TEMPLE:
        level.flags.has_temple = TRUE;
        break;
    case SWAMP:
        level.flags.has_swamp = TRUE;
        break;
    }
}

struct mkroom *
build_room(r, mkr)
room *r;
struct mkroom *mkr;
{
    boolean okroom;
    struct mkroom *aroom;
    xchar rtype = (!r->chance || rn2(100) < r->chance) ? r->rtype : OROOM;

    if (mkr) {
        aroom = &subrooms[nsubroom];
        okroom = create_subroom(mkr, r->x, r->y, r->w, r->h, rtype, r->rlit);
    } else {
        aroom = &rooms[nroom];
        okroom = create_room(r->x, r->y, r->w, r->h, r->xalign, r->yalign,
                             rtype, r->rlit);
    }

    if (okroom) {
#ifdef SPECIALIZATION
        topologize(aroom, FALSE); /* set roomno */
#else
        topologize(aroom); /* set roomno */
#endif
        aroom->needfill = r->filled;
        aroom->needjoining = r->joined;
        return aroom;
    }
    return (struct mkroom *) 0;
}

/*
 * set lighting in a region that will not become a room.
 */
STATIC_OVL void
light_region(tmpregion)
region *tmpregion;
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

STATIC_OVL void
wallify_map(x1, y1, x2, y2)
int x1, y1, x2, y2;
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
STATIC_OVL void
maze1xy(m, humidity)
coord *m;
int humidity;
{
    register int x, y, tryct = 2000;
    /* tryct:  normally it won't take more than ten or so tries due
       to the circumstances under which we'll be called, but the
       `humidity' screening might drastically change the chances */

    do {
        x = rn1(x_maze_max - 3, 3);
        y = rn1(y_maze_max - 3, 3);
        if (--tryct < 0)
            break; /* give up */
    } while (!(x % 2) || !(y % 2) || SpLev_Map[x][y]
             || !is_ok_location((schar) x, (schar) y, humidity));

    m->x = (xchar) x, m->y = (xchar) y;
}

/*
 * If there's a significant portion of maze unused by the special level,
 * we don't want it empty.
 *
 * Makes the number of traps, monsters, etc. proportional
 * to the size of the maze.
 */
STATIC_OVL void
fill_empty_maze()
{
    int mapcountmax, mapcount, mapfact;
    xchar x, y;
    coord mm;

    mapcountmax = mapcount = (x_maze_max - 2) * (y_maze_max - 2);
    mapcountmax = mapcountmax / 2;

    for (x = 2; x < x_maze_max; x++)
        for (y = 0; y < y_maze_max; y++)
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

/*
 * special level loader
 */
STATIC_OVL boolean
sp_level_loader(fd, lvl)
dlb *fd;
sp_lev *lvl;
{
    long n_opcode = 0;
    struct opvar *opdat;
    int opcode;

    Fread((genericptr_t) & (lvl->n_opcodes), 1, sizeof(lvl->n_opcodes), fd);
    lvl->opcodes = (_opcode *) alloc(sizeof(_opcode) * (lvl->n_opcodes));

    while (n_opcode < lvl->n_opcodes) {
        Fread((genericptr_t) &lvl->opcodes[n_opcode].opcode, 1,
              sizeof(lvl->opcodes[n_opcode].opcode), fd);
        opcode = lvl->opcodes[n_opcode].opcode;

        opdat = NULL;

        if (opcode < SPO_NULL || opcode >= MAX_SP_OPCODES)
            panic("sp_level_loader: impossible opcode %i.", opcode);

        if (opcode == SPO_PUSH) {
            int nsize;
            struct opvar *ov = (struct opvar *) alloc(sizeof(struct opvar));

            opdat = ov;
            ov->spovartyp = SPO_NULL;
            ov->vardata.l = 0;
            Fread((genericptr_t) & (ov->spovartyp), 1, sizeof(ov->spovartyp),
                  fd);

            switch (ov->spovartyp) {
            case SPOVAR_NULL:
                break;
            case SPOVAR_COORD:
            case SPOVAR_REGION:
            case SPOVAR_MAPCHAR:
            case SPOVAR_MONST:
            case SPOVAR_OBJ:
            case SPOVAR_INT:
                Fread((genericptr_t) & (ov->vardata.l), 1,
                      sizeof(ov->vardata.l), fd);
                break;
            case SPOVAR_VARIABLE:
            case SPOVAR_STRING:
            case SPOVAR_SEL: {
                char *opd;

                Fread((genericptr_t) &nsize, 1, sizeof(nsize), fd);
                opd = (char *) alloc(nsize + 1);

                if (nsize)
                    Fread(opd, 1, nsize, fd);
                opd[nsize] = 0;
                ov->vardata.str = opd;
                break;
            }
            default:
                panic("sp_level_loader: unknown opvar type %i",
                      ov->spovartyp);
            }
        }

        lvl->opcodes[n_opcode].opdat = opdat;
        n_opcode++;
    } /*while*/

    return TRUE;
}

/* Frees the memory allocated for special level creation structs */
STATIC_OVL boolean
sp_level_free(lvl)
sp_lev *lvl;
{
    static const char nhFunc[] = "sp_level_free";
    long n_opcode = 0;

    while (n_opcode < lvl->n_opcodes) {
        int opcode = lvl->opcodes[n_opcode].opcode;
        struct opvar *opdat = lvl->opcodes[n_opcode].opdat;

        if (opcode < SPO_NULL || opcode >= MAX_SP_OPCODES)
            panic("sp_level_free: unknown opcode %i", opcode);

        if (opdat)
            opvar_free(opdat);
        n_opcode++;
    }
    Free(lvl->opcodes);
    lvl->opcodes = NULL;
    return TRUE;
}

void
splev_initlev(linit)
lev_init *linit;
{
    switch (linit->init_style) {
    default:
        impossible("Unrecognized level init style.");
        break;
    case LVLINIT_NONE:
        break;
    case LVLINIT_SOLIDFILL:
        if (linit->lit == -1)
            linit->lit = rn2(2);
        lvlfill_solid(linit->filling, linit->lit);
        break;
    case LVLINIT_MAZEGRID:
        lvlfill_maze_grid(2, 0, x_maze_max, y_maze_max, linit->filling);
        break;
    case LVLINIT_ROGUE:
        makeroguerooms();
        break;
    case LVLINIT_MINES:
        if (linit->lit == -1)
            linit->lit = rn2(2);
        if (linit->filling > -1)
            lvlfill_solid(linit->filling, 0);
        linit->icedpools = icedpools;
        mkmap(linit);
        break;
    }
}

struct sp_frame *
frame_new(execptr)
long execptr;
{
    struct sp_frame *frame =
        (struct sp_frame *) alloc(sizeof(struct sp_frame));

    frame->next = NULL;
    frame->variables = NULL;
    frame->n_opcode = execptr;
    frame->stack = (struct splevstack *) alloc(sizeof(struct splevstack));
    splev_stack_init(frame->stack);
    return frame;
}

void
frame_del(frame)
struct sp_frame *frame;
{
    if (!frame)
        return;
    if (frame->stack) {
        splev_stack_done(frame->stack);
        frame->stack = NULL;
    }
    if (frame->variables) {
        variable_list_del(frame->variables);
        frame->variables = NULL;
    }
    Free(frame);
}

void
spo_frame_push(coder)
struct sp_coder *coder;
{
    struct sp_frame *tmpframe = frame_new(coder->frame->n_opcode);

    tmpframe->next = coder->frame;
    coder->frame = tmpframe;
}

void
spo_frame_pop(coder)
struct sp_coder *coder;
{
    if (coder->frame && coder->frame->next) {
        struct sp_frame *tmpframe = coder->frame->next;

        frame_del(coder->frame);
        coder->frame = tmpframe;
        coder->stack = coder->frame->stack;
    }
}

long
sp_code_jmpaddr(curpos, jmpaddr)
long curpos, jmpaddr;
{
    return (curpos + jmpaddr);
}

void
spo_call(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_call";
    struct opvar *addr;
    struct opvar *params;
    struct sp_frame *tmpframe;

    if (!OV_pop_i(addr) || !OV_pop_i(params))
        return;
    if (OV_i(params) < 0)
        return;

    tmpframe = frame_new(sp_code_jmpaddr(coder->frame->n_opcode,
                                         OV_i(addr) - 1));

    while (OV_i(params)-- > 0) {
        splev_stack_push(tmpframe->stack, splev_stack_getdat_any(coder));
    }
    splev_stack_reverse(tmpframe->stack);

    /* push a frame */
    tmpframe->next = coder->frame;
    coder->frame = tmpframe;

    opvar_free(addr);
    opvar_free(params);
}

void
spo_return(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_return";
    struct opvar *params;

    if (!coder->frame || !coder->frame->next)
        panic("return: no frame.");
    if (!OV_pop_i(params))
        return;
    if (OV_i(params) < 0)
        return;

    while (OV_i(params)-- > 0) {
        splev_stack_push(coder->frame->next->stack,
                         splev_stack_pop(coder->stack));
    }

    /* pop the frame */
    if (coder->frame->next) {
        struct sp_frame *tmpframe = coder->frame->next;
        frame_del(coder->frame);
        coder->frame = tmpframe;
        coder->stack = coder->frame->stack;
    }

    opvar_free(params);
}

/*ARGUSED*/
void
spo_end_moninvent(coder)
struct sp_coder *coder UNUSED;
{
    if (invent_carrying_monster)
        m_dowear(invent_carrying_monster, TRUE);
    invent_carrying_monster = NULL;
}

/*ARGUSED*/
void
spo_pop_container(coder)
struct sp_coder *coder UNUSED;
{
    if (container_idx > 0) {
        container_idx--;
        container_obj[container_idx] = NULL;
    }
}

void
spo_message(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_message";
    struct opvar *op;
    char *msg, *levmsg;
    int old_n, n;

    if (!OV_pop_s(op))
        return;
    msg = OV_s(op);
    if (!msg)
        return;

    old_n = lev_message ? (strlen(lev_message) + 1) : 0;
    n = strlen(msg);

    levmsg = (char *) alloc(old_n + n + 1);
    if (old_n)
        levmsg[old_n - 1] = '\n';
    if (lev_message)
        (void) memcpy((genericptr_t) levmsg, (genericptr_t) lev_message,
                      old_n - 1);
    (void) memcpy((genericptr_t) &levmsg[old_n], msg, n);
    levmsg[old_n + n] = '\0';
    Free(lev_message);
    lev_message = levmsg;
    opvar_free(op);
}

void
spo_monster(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_monster";
    int nparams = 0;
    struct opvar *varparam;
    struct opvar *id, *mcoord, *has_inv;
    monster tmpmons;

    tmpmons.peaceful = -1;
    tmpmons.asleep = -1;
    tmpmons.name.str = (char *) 0;
    tmpmons.appear = 0;
    tmpmons.appear_as.str = (char *) 0;
    tmpmons.align = -MAX_REGISTERS - 2;
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

    if (!OV_pop_i(has_inv))
        return;

    if (!OV_pop_i(varparam))
        return;

    while ((nparams++ < (SP_M_V_END + 1)) && (OV_typ(varparam) == SPOVAR_INT)
           && (OV_i(varparam) >= 0) && (OV_i(varparam) < SP_M_V_END)) {
        struct opvar *parm = NULL;

        OV_pop(parm);
        switch (OV_i(varparam)) {
        case SP_M_V_NAME:
            if ((OV_typ(parm) == SPOVAR_STRING) && !tmpmons.name.str)
                tmpmons.name.str = dupstr(OV_s(parm));
            break;
        case SP_M_V_APPEAR:
            if ((OV_typ(parm) == SPOVAR_INT) && !tmpmons.appear_as.str) {
                tmpmons.appear = OV_i(parm);
                opvar_free(parm);
                OV_pop(parm);
                tmpmons.appear_as.str = dupstr(OV_s(parm));
            }
            break;
        case SP_M_V_ASLEEP:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.asleep = OV_i(parm);
            break;
        case SP_M_V_ALIGN:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.align = OV_i(parm);
            break;
        case SP_M_V_PEACEFUL:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.peaceful = OV_i(parm);
            break;
        case SP_M_V_FEMALE:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.female = OV_i(parm);
            break;
        case SP_M_V_INVIS:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.invis = OV_i(parm);
            break;
        case SP_M_V_CANCELLED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.cancelled = OV_i(parm);
            break;
        case SP_M_V_REVIVED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.revived = OV_i(parm);
            break;
        case SP_M_V_AVENGE:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.avenge = OV_i(parm);
            break;
        case SP_M_V_FLEEING:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.fleeing = OV_i(parm);
            break;
        case SP_M_V_BLINDED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.blinded = OV_i(parm);
            break;
        case SP_M_V_PARALYZED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.paralyzed = OV_i(parm);
            break;
        case SP_M_V_STUNNED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.stunned = OV_i(parm);
            break;
        case SP_M_V_CONFUSED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.confused = OV_i(parm);
            break;
        case SP_M_V_SEENTRAPS:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpmons.seentraps = OV_i(parm);
            break;
        case SP_M_V_END:
            nparams = SP_M_V_END + 1;
            break;
        default:
            impossible("MONSTER with unknown variable param type!");
            break;
        }
        opvar_free(parm);
        if (OV_i(varparam) != SP_M_V_END) {
            opvar_free(varparam);
            OV_pop(varparam);
        }
    }

    if (!OV_pop_c(mcoord))
        panic("no monster coord?");

    if (!OV_pop_typ(id, SPOVAR_MONST))
        panic("no mon type");

    tmpmons.id = SP_MONST_PM(OV_i(id));
    tmpmons.class = SP_MONST_CLASS(OV_i(id));
    tmpmons.coord = OV_i(mcoord);
    tmpmons.has_invent = OV_i(has_inv);

    create_monster(&tmpmons, coder->croom);

    Free(tmpmons.name.str);
    Free(tmpmons.appear_as.str);
    opvar_free(id);
    opvar_free(mcoord);
    opvar_free(has_inv);
    opvar_free(varparam);
}

void
spo_object(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_object";
    int nparams = 0;
    long quancnt;
    struct opvar *varparam;
    struct opvar *id, *containment;
    object tmpobj;

    tmpobj.spe = -127;
    tmpobj.curse_state = -1;
    tmpobj.corpsenm = NON_PM;
    tmpobj.name.str = (char *) 0;
    tmpobj.quan = -1;
    tmpobj.buried = 0;
    tmpobj.lit = 0;
    tmpobj.eroded = 0;
    tmpobj.locked = 0;
    tmpobj.trapped = -1;
    tmpobj.recharged = 0;
    tmpobj.invis = 0;
    tmpobj.greased = 0;
    tmpobj.broken = 0;
    tmpobj.coord = SP_COORD_PACK_RANDOM(0);

    if (!OV_pop_i(containment))
        return;

    if (!OV_pop_i(varparam))
        return;

    while ((nparams++ < (SP_O_V_END + 1)) && (OV_typ(varparam) == SPOVAR_INT)
           && (OV_i(varparam) >= 0) && (OV_i(varparam) < SP_O_V_END)) {
        struct opvar *parm;

        OV_pop(parm);
        switch (OV_i(varparam)) {
        case SP_O_V_NAME:
            if ((OV_typ(parm) == SPOVAR_STRING) && !tmpobj.name.str)
                tmpobj.name.str = dupstr(OV_s(parm));
            break;
        case SP_O_V_CORPSENM:
            if (OV_typ(parm) == SPOVAR_MONST) {
                char monclass = SP_MONST_CLASS(OV_i(parm));
                int monid = SP_MONST_PM(OV_i(parm));

                if (monid >= LOW_PM && monid < NUMMONS) {
                    tmpobj.corpsenm = monid;
                    break; /* we're done! */
                } else {
                    struct permonst *pm = (struct permonst *) 0;

                    if (def_char_to_monclass(monclass) != MAXMCLASSES) {
                        pm = mkclass(def_char_to_monclass(monclass), G_NOGEN);
                    } else {
                        pm = rndmonst();
                    }
                    if (pm)
                        tmpobj.corpsenm = monsndx(pm);
                }
            }
            break;
        case SP_O_V_CURSE:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.curse_state = OV_i(parm);
            break;
        case SP_O_V_SPE:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.spe = OV_i(parm);
            break;
        case SP_O_V_QUAN:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.quan = OV_i(parm);
            break;
        case SP_O_V_BURIED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.buried = OV_i(parm);
            break;
        case SP_O_V_LIT:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.lit = OV_i(parm);
            break;
        case SP_O_V_ERODED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.eroded = OV_i(parm);
            break;
        case SP_O_V_LOCKED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.locked = OV_i(parm);
            break;
        case SP_O_V_TRAPPED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.trapped = OV_i(parm);
            break;
        case SP_O_V_RECHARGED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.recharged = OV_i(parm);
            break;
        case SP_O_V_INVIS:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.invis = OV_i(parm);
            break;
        case SP_O_V_GREASED:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.greased = OV_i(parm);
            break;
        case SP_O_V_BROKEN:
            if (OV_typ(parm) == SPOVAR_INT)
                tmpobj.broken = OV_i(parm);
            break;
        case SP_O_V_COORD:
            if (OV_typ(parm) != SPOVAR_COORD)
                panic("no coord for obj?");
            tmpobj.coord = OV_i(parm);
            break;
        case SP_O_V_END:
            nparams = SP_O_V_END + 1;
            break;
        default:
            impossible("OBJECT with unknown variable param type!");
            break;
        }
        opvar_free(parm);
        if (OV_i(varparam) != SP_O_V_END) {
            opvar_free(varparam);
            OV_pop(varparam);
        }
    }

    if (!OV_pop_typ(id, SPOVAR_OBJ))
        panic("no obj type");

    tmpobj.id = SP_OBJ_TYP(OV_i(id));
    tmpobj.class = SP_OBJ_CLASS(OV_i(id));
    tmpobj.containment = OV_i(containment);

    quancnt = (tmpobj.id > STRANGE_OBJECT) ? tmpobj.quan : 0;

    do {
        create_object(&tmpobj, coder->croom);
        quancnt--;
    } while ((quancnt > 0) && ((tmpobj.id > STRANGE_OBJECT)
                               && !objects[tmpobj.id].oc_merge));

    Free(tmpobj.name.str);
    opvar_free(varparam);
    opvar_free(id);
    opvar_free(containment);
}

void
spo_level_flags(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_level_flags";
    struct opvar *flagdata;
    long lflags;

    if (!OV_pop_i(flagdata))
        return;
    lflags = OV_i(flagdata);

    if (lflags & NOTELEPORT)
        level.flags.noteleport = 1;
    if (lflags & HARDFLOOR)
        level.flags.hardfloor = 1;
    if (lflags & NOMMAP)
        level.flags.nommap = 1;
    if (lflags & SHORTSIGHTED)
        level.flags.shortsighted = 1;
    if (lflags & ARBOREAL)
        level.flags.arboreal = 1;
    if (lflags & MAZELEVEL)
        level.flags.is_maze_lev = 1;
    if (lflags & PREMAPPED)
        coder->premapped = TRUE;
    if (lflags & SHROUD)
        level.flags.hero_memory = 0;
    if (lflags & GRAVEYARD)
        level.flags.graveyard = 1;
    if (lflags & ICEDPOOLS)
        icedpools = TRUE;
    if (lflags & SOLIDIFY)
        coder->solidify = TRUE;
    if (lflags & CORRMAZE)
        level.flags.corrmaze = TRUE;
    if (lflags & CHECK_INACCESSIBLES)
        coder->check_inaccessibles = TRUE;

    opvar_free(flagdata);
}

void
spo_initlevel(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_initlevel";
    lev_init init_lev;
    struct opvar *init_style, *fg, *bg, *smoothed, *joined, *lit, *walled,
        *filling;

    if (!OV_pop_i(fg) || !OV_pop_i(bg) || !OV_pop_i(smoothed)
        || !OV_pop_i(joined) || !OV_pop_i(lit) || !OV_pop_i(walled)
        || !OV_pop_i(filling) || !OV_pop_i(init_style))
        return;

    splev_init_present = TRUE;

    init_lev.init_style = OV_i(init_style);
    init_lev.fg = OV_i(fg);
    init_lev.bg = OV_i(bg);
    init_lev.smoothed = OV_i(smoothed);
    init_lev.joined = OV_i(joined);
    init_lev.lit = OV_i(lit);
    init_lev.walled = OV_i(walled);
    init_lev.filling = OV_i(filling);

    coder->lvl_is_joined = OV_i(joined);

    splev_initlev(&init_lev);

    opvar_free(init_style);
    opvar_free(fg);
    opvar_free(bg);
    opvar_free(smoothed);
    opvar_free(joined);
    opvar_free(lit);
    opvar_free(walled);
    opvar_free(filling);
}

void
spo_engraving(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_engraving";
    struct opvar *etyp, *txt, *ecoord;
    xchar x, y;

    if (!OV_pop_i(etyp) || !OV_pop_s(txt) || !OV_pop_c(ecoord))
        return;

    get_location_coord(&x, &y, DRY, coder->croom, OV_i(ecoord));
    make_engr_at(x, y, OV_s(txt), 0L, OV_i(etyp));

    opvar_free(etyp);
    opvar_free(txt);
    opvar_free(ecoord);
}

void
spo_mineralize(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_mineralize";
    struct opvar *kelp_pool, *kelp_moat, *gold_prob, *gem_prob;

    if (!OV_pop_i(gem_prob) || !OV_pop_i(gold_prob) || !OV_pop_i(kelp_moat)
        || !OV_pop_i(kelp_pool))
        return;

    mineralize(OV_i(kelp_pool), OV_i(kelp_moat), OV_i(gold_prob),
               OV_i(gem_prob), TRUE);

    opvar_free(gem_prob);
    opvar_free(gold_prob);
    opvar_free(kelp_moat);
    opvar_free(kelp_pool);
}

void
spo_room(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_room";

    if (coder->n_subroom > MAX_NESTED_ROOMS) {
        panic("Too deeply nested rooms?!");
    } else {
        struct opvar *rflags, *h, *w, *yalign, *xalign, *y, *x, *rlit,
            *chance, *rtype;
        room tmproom;
        struct mkroom *tmpcr;

        if (!OV_pop_i(h) || !OV_pop_i(w) || !OV_pop_i(y) || !OV_pop_i(x)
            || !OV_pop_i(yalign) || !OV_pop_i(xalign) || !OV_pop_i(rflags)
            || !OV_pop_i(rlit) || !OV_pop_i(chance) || !OV_pop_i(rtype))
            return;

        tmproom.x = OV_i(x);
        tmproom.y = OV_i(y);
        tmproom.w = OV_i(w);
        tmproom.h = OV_i(h);
        tmproom.xalign = OV_i(xalign);
        tmproom.yalign = OV_i(yalign);
        tmproom.rtype = OV_i(rtype);
        tmproom.chance = OV_i(chance);
        tmproom.rlit = OV_i(rlit);
        tmproom.filled = (OV_i(rflags) & (1 << 0));
        /*tmproom.irregular = (OV_i(rflags) & (1 << 1));*/
        tmproom.joined = !(OV_i(rflags) & (1 << 2));

        opvar_free(x);
        opvar_free(y);
        opvar_free(w);
        opvar_free(h);
        opvar_free(xalign);
        opvar_free(yalign);
        opvar_free(rtype);
        opvar_free(chance);
        opvar_free(rlit);
        opvar_free(rflags);

        if (!coder->failed_room[coder->n_subroom - 1]) {
            tmpcr = build_room(&tmproom, coder->croom);
            if (tmpcr) {
                coder->tmproomlist[coder->n_subroom] = tmpcr;
                coder->failed_room[coder->n_subroom] = FALSE;
                coder->n_subroom++;
                return;
            }
        } /* failed to create parent room, so fail this too */
    }
    coder->tmproomlist[coder->n_subroom] = (struct mkroom *) 0;
    coder->failed_room[coder->n_subroom] = TRUE;
    coder->n_subroom++;
}

void
spo_endroom(coder)
struct sp_coder *coder;
{
    if (coder->n_subroom > 1) {
        coder->n_subroom--;
        coder->tmproomlist[coder->n_subroom] = NULL;
        coder->failed_room[coder->n_subroom] = TRUE;
    } else {
        /* no subroom, get out of top-level room */
        /* Need to ensure xstart/ystart/xsize/ysize have something sensible,
           in case there's some stuff to be created outside the outermost
           room,
           and there's no MAP.
        */
        if (xsize <= 1 && ysize <= 1) {
            xstart = 1;
            ystart = 0;
            xsize = COLNO - 1;
            ysize = ROWNO;
        }
    }
}

void
spo_stair(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_stair";
    xchar x, y;
    struct opvar *up, *scoord;
    struct trap *badtrap;

    if (!OV_pop_i(up) || !OV_pop_c(scoord))
        return;

    get_location_coord(&x, &y, DRY, coder->croom, OV_i(scoord));
    if ((badtrap = t_at(x, y)) != 0)
        deltrap(badtrap);
    mkstairs(x, y, (char) OV_i(up), coder->croom);
    SpLev_Map[x][y] = 1;

    opvar_free(scoord);
    opvar_free(up);
}

void
spo_ladder(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_ladder";
    xchar x, y;
    struct opvar *up, *lcoord;

    if (!OV_pop_i(up) || !OV_pop_c(lcoord))
        return;

    get_location_coord(&x, &y, DRY, coder->croom, OV_i(lcoord));

    levl[x][y].typ = LADDER;
    SpLev_Map[x][y] = 1;
    if (OV_i(up)) {
        xupladder = x;
        yupladder = y;
        levl[x][y].ladder = LA_UP;
    } else {
        xdnladder = x;
        ydnladder = y;
        levl[x][y].ladder = LA_DOWN;
    }
    opvar_free(lcoord);
    opvar_free(up);
}

void
spo_grave(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_grave";
    struct opvar *gcoord, *typ, *txt;
    schar x, y;

    if (!OV_pop_i(typ) || !OV_pop_s(txt) || !OV_pop_c(gcoord))
        return;

    get_location_coord(&x, &y, DRY, coder->croom, OV_i(gcoord));

    if (isok(x, y) && !t_at(x, y)) {
        levl[x][y].typ = GRAVE;
        switch (OV_i(typ)) {
        case 2:
            make_grave(x, y, OV_s(txt));
            break;
        case 1:
            make_grave(x, y, NULL);
            break;
        default:
            del_engr_at(x, y);
            break;
        }
    }

    opvar_free(gcoord);
    opvar_free(typ);
    opvar_free(txt);
}

void
spo_altar(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_altar";
    struct opvar *al, *shrine, *acoord;
    altar tmpaltar;

    if (!OV_pop_i(al) || !OV_pop_i(shrine) || !OV_pop_c(acoord))
        return;

    tmpaltar.coord = OV_i(acoord);
    tmpaltar.align = OV_i(al);
    tmpaltar.shrine = OV_i(shrine);

    create_altar(&tmpaltar, coder->croom);

    opvar_free(acoord);
    opvar_free(shrine);
    opvar_free(al);
}

void
spo_trap(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_trap";
    struct opvar *type;
    struct opvar *tcoord;
    spltrap tmptrap;

    if (!OV_pop_i(type) || !OV_pop_c(tcoord))
        return;

    tmptrap.coord = OV_i(tcoord);
    tmptrap.type = OV_i(type);

    create_trap(&tmptrap, coder->croom);
    opvar_free(tcoord);
    opvar_free(type);
}

void
spo_gold(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_gold";
    struct opvar *gcoord, *amt;
    schar x, y;
    long amount;

    if (!OV_pop_c(gcoord) || !OV_pop_i(amt))
        return;
    amount = OV_i(amt);
    get_location_coord(&x, &y, DRY, coder->croom, OV_i(gcoord));
    if (amount == -1)
        amount = rnd(200);
    mkgold(amount, x, y);
    opvar_free(gcoord);
    opvar_free(amt);
}

void
spo_corridor(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_corridor";
    struct opvar *deswall, *desdoor, *desroom, *srcwall, *srcdoor, *srcroom;
    corridor tc;

    if (!OV_pop_i(deswall) || !OV_pop_i(desdoor) || !OV_pop_i(desroom)
        || !OV_pop_i(srcwall) || !OV_pop_i(srcdoor) || !OV_pop_i(srcroom))
        return;

    tc.src.room = OV_i(srcroom);
    tc.src.door = OV_i(srcdoor);
    tc.src.wall = OV_i(srcwall);
    tc.dest.room = OV_i(desroom);
    tc.dest.door = OV_i(desdoor);
    tc.dest.wall = OV_i(deswall);

    create_corridor(&tc);

    opvar_free(deswall);
    opvar_free(desdoor);
    opvar_free(desroom);
    opvar_free(srcwall);
    opvar_free(srcdoor);
    opvar_free(srcroom);
}

struct opvar *
selection_opvar(nbuf)
char *nbuf;
{
    struct opvar *ov;
    char buf[(COLNO * ROWNO) + 1];

    if (!nbuf) {
        (void) memset(buf, 1, sizeof(buf));
        buf[(COLNO * ROWNO)] = '\0';
        ov = opvar_new_str(buf);
    } else {
        ov = opvar_new_str(nbuf);
    }
    ov->spovartyp = SPOVAR_SEL;
    return ov;
}

xchar
selection_getpoint(x, y, ov)
int x, y;
struct opvar *ov;
{
    if (!ov || ov->spovartyp != SPOVAR_SEL)
        return 0;
    if (x < 0 || y < 0 || x >= COLNO || y >= ROWNO)
        return 0;

    return (ov->vardata.str[COLNO * y + x] - 1);
}

void
selection_setpoint(x, y, ov, c)
int x, y;
struct opvar *ov;
xchar c;
{
    if (!ov || ov->spovartyp != SPOVAR_SEL)
        return;
    if (x < 0 || y < 0 || x >= COLNO || y >= ROWNO)
        return;

    ov->vardata.str[COLNO * y + x] = (char) (c + 1);
}

struct opvar *
selection_not(s)
struct opvar *s;
{
    struct opvar *ov;
    int x, y;

    ov = selection_opvar((char *) 0);
    if (!ov)
        return NULL;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (!selection_getpoint(x, y, s))
                selection_setpoint(x, y, ov, 1);

    return ov;
}

struct opvar *
selection_logical_oper(s1, s2, oper)
struct opvar *s1, *s2;
char oper;
{
    struct opvar *ov;
    int x, y;

    ov = selection_opvar((char *) 0);
    if (!ov)
        return NULL;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            switch (oper) {
            default:
            case '|':
                if (selection_getpoint(x, y, s1)
                    || selection_getpoint(x, y, s2))
                    selection_setpoint(x, y, ov, 1);
                break;
            case '&':
                if (selection_getpoint(x, y, s1)
                    && selection_getpoint(x, y, s2))
                    selection_setpoint(x, y, ov, 1);
                break;
            }
        }

    return ov;
}

struct opvar *
selection_filter_mapchar(ov, mc)
struct opvar *ov;
struct opvar *mc;
{
    int x, y;
    schar mapc;
    xchar lit;
    struct opvar *ret = selection_opvar((char *) 0);

    if (!ov || !mc || !ret)
        return NULL;
    mapc = SP_MAPCHAR_TYP(OV_i(mc));
    lit = SP_MAPCHAR_LIT(OV_i(mc));
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (selection_getpoint(x, y, ov) && (levl[x][y].typ == mapc)) {
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
                    if (levl[x][y].lit == lit)
                        selection_setpoint(x, y, ret, 1);
                    break;
                }
            }
    return ret;
}

void
selection_filter_percent(ov, percent)
struct opvar *ov;
int percent;
{
    int x, y;

    if (!ov)
        return;
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (selection_getpoint(x, y, ov) && (rn2(100) >= percent))
                selection_setpoint(x, y, ov, 0);
}

STATIC_OVL int
selection_rndcoord(ov, x, y, removeit)
struct opvar *ov;
schar *x, *y;
boolean removeit;
{
    int idx = 0;
    int c;
    int dx, dy;

    for (dx = 0; dx < COLNO; dx++)
        for (dy = 0; dy < ROWNO; dy++)
            if (isok(dx, dy) && selection_getpoint(dx, dy, ov))
                idx++;

    if (idx) {
        c = rn2(idx);
        for (dx = 0; dx < COLNO; dx++)
            for (dy = 0; dy < ROWNO; dy++)
                if (isok(dx, dy) && selection_getpoint(dx, dy, ov)) {
                    if (!c) {
                        *x = dx;
                        *y = dy;
                        if (removeit) selection_setpoint(dx, dy, ov, 0);
                        return 1;
                    }
                    c--;
                }
    }
    *x = *y = -1;
    return 0;
}

void
selection_do_grow(ov, dir)
struct opvar *ov;
int dir;
{
    int x, y;
    char tmp[COLNO][ROWNO];

    if (ov->spovartyp != SPOVAR_SEL)
        return;
    if (!ov)
        return;

    (void) memset(tmp, 0, sizeof tmp);

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
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
                tmp[x][y] = 1;
            }
        }

    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (tmp[x][y])
                selection_setpoint(x, y, ov, 1);
}

STATIC_VAR int FDECL((*selection_flood_check_func), (int, int));
STATIC_VAR schar floodfillchk_match_under_typ;

void
set_selection_floodfillchk(f)
int FDECL((*f), (int, int));
{
    selection_flood_check_func = f;
}

STATIC_OVL int
floodfillchk_match_under(x,y)
int x,y;
{
    return (floodfillchk_match_under_typ == levl[x][y].typ);
}

STATIC_OVL int
floodfillchk_match_accessible(x, y)
int x, y;
{
    return (ACCESSIBLE(levl[x][y].typ)
            || levl[x][y].typ == SDOOR
            || levl[x][y].typ == SCORR);
}

/* check whethere <x,y> is already in xs[],ys[] */
STATIC_OVL boolean
sel_flood_havepoint(x, y, xs, ys, n)
int x, y;
xchar xs[], ys[];
int n;
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
selection_floodfill(ov, x, y, diagonals)
struct opvar *ov;
int x, y;
boolean diagonals;
{
    static const char nhFunc[] = "selection_floodfill";
    struct opvar *tmp = selection_opvar((char *) 0);
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

    if (selection_flood_check_func == (int FDECL((*), (int, int))) 0) {
        opvar_free(tmp);
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
    opvar_free(tmp);
}

/* McIlroy's Ellipse Algorithm */
void
selection_do_ellipse(ov, xc, yc, a, b, filled)
struct opvar *ov;
int xc, yc, a, b, filled;
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
long
line_dist_coord(x1, y1, x2, y2, x3, y3)
long x1, y1, x2, y2, x3, y3;
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
selection_do_gradient(ov, x, y, x2, y2, gtyp, mind, maxd, limit)
struct opvar *ov;
long x, y, x2, y2, gtyp, mind, maxd, limit;
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
selection_do_line(x1, y1, x2, y2, ov)
schar x1, y1, x2, y2;
struct opvar *ov;
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

    if (dx > dy) {
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
selection_do_randline(x1, y1, x2, y2, rough, rec, ov)
schar x1, y1, x2, y2, rough, rec;
struct opvar *ov;
{
    int mx, my;
    int dx, dy;

    if (rec < 1) {
        return;
    }

    if ((x2 == x1) && (y2 == y1)) {
        selection_setpoint(x1, y1, ov, 1);
        return;
    }

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

    selection_setpoint(mx, my, ov, 1);

    rough = (rough * 2) / 3;

    rec--;

    selection_do_randline(x1, y1, mx, my, rough, rec, ov);
    selection_do_randline(mx, my, x2, y2, rough, rec, ov);
}

void
selection_iterate(ov, func, arg)
struct opvar *ov;
select_iter_func func;
genericptr_t arg;
{
    int x, y;

    /* yes, this is very naive, but it's not _that_ expensive. */
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (selection_getpoint(x, y, ov))
                (*func)(x, y, arg);
}

void
sel_set_ter(x, y, arg)
int x, y;
genericptr_t arg;
{
    terrain terr;

    terr = *(terrain *) arg;
    SET_TYPLIT(x, y, terr.ter, terr.tlit);
    /* handle doors and secret doors */
    if (levl[x][y].typ == SDOOR || IS_DOOR(levl[x][y].typ)) {
        if (levl[x][y].typ == SDOOR)
            levl[x][y].doormask = D_CLOSED;
        if (x && (IS_WALL(levl[x - 1][y].typ) || levl[x - 1][y].horizontal))
            levl[x][y].horizontal = 1;
    }
}

void
sel_set_feature(x, y, arg)
int x, y;
genericptr_t arg;
{
    if (IS_FURNITURE(levl[x][y].typ))
        return;
    levl[x][y].typ = (*(int *) arg);
}

void
sel_set_door(dx, dy, arg)
int dx, dy;
genericptr_t arg;
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

void
spo_door(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_door";
    struct opvar *msk, *sel;
    xchar typ;

    if (!OV_pop_i(msk) || !OV_pop_typ(sel, SPOVAR_SEL))
        return;

    typ = OV_i(msk) == -1 ? rnddoor() : (xchar) OV_i(msk);

    selection_iterate(sel, sel_set_door, (genericptr_t) &typ);

    opvar_free(sel);
    opvar_free(msk);
}

void
spo_feature(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_feature";
    struct opvar *sel;
    int typ;

    if (!OV_pop_typ(sel, SPOVAR_SEL))
        return;

    switch (coder->opcode) {
    default:
        impossible("spo_feature called with wrong opcode %i.", coder->opcode);
        break;
    case SPO_FOUNTAIN:
        typ = FOUNTAIN;
        break;
    case SPO_SINK:
        typ = SINK;
        break;
    case SPO_POOL:
        typ = POOL;
        break;
    }
    selection_iterate(sel, sel_set_feature, (genericptr_t) &typ);
    opvar_free(sel);
}

void
spo_terrain(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_terrain";
    terrain tmpterrain;
    struct opvar *ter, *sel;

    if (!OV_pop_typ(ter, SPOVAR_MAPCHAR) || !OV_pop_typ(sel, SPOVAR_SEL))
        return;

    tmpterrain.ter = SP_MAPCHAR_TYP(OV_i(ter));
    tmpterrain.tlit = SP_MAPCHAR_LIT(OV_i(ter));
    selection_iterate(sel, sel_set_ter, (genericptr_t) &tmpterrain);

    opvar_free(ter);
    opvar_free(sel);
}

void
spo_replace_terrain(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_replace_terrain";
    replaceterrain rt;
    struct opvar *reg, *from_ter, *to_ter, *chance;

    if (!OV_pop_i(chance) || !OV_pop_typ(to_ter, SPOVAR_MAPCHAR)
        || !OV_pop_typ(from_ter, SPOVAR_MAPCHAR) || !OV_pop_r(reg))
        return;

    rt.chance = OV_i(chance);
    rt.tolit = SP_MAPCHAR_LIT(OV_i(to_ter));
    rt.toter = SP_MAPCHAR_TYP(OV_i(to_ter));
    rt.fromter = SP_MAPCHAR_TYP(OV_i(from_ter));
    /* TODO: use SP_MAPCHAR_LIT(OV_i(from_ter)) too */
    rt.x1 = SP_REGION_X1(OV_i(reg));
    rt.y1 = SP_REGION_Y1(OV_i(reg));
    rt.x2 = SP_REGION_X2(OV_i(reg));
    rt.y2 = SP_REGION_Y2(OV_i(reg));

    replace_terrain(&rt, coder->croom);

    opvar_free(reg);
    opvar_free(from_ter);
    opvar_free(to_ter);
    opvar_free(chance);
}

STATIC_OVL boolean
generate_way_out_method(nx,ny, ov)
int nx,ny;
struct opvar *ov;
{
    static const char nhFunc[] = "generate_way_out_method";
    const int escapeitems[] = { PICK_AXE,
                                DWARVISH_MATTOCK,
                                WAN_DIGGING,
                                WAN_TELEPORTATION,
                                SCR_TELEPORTATION,
                                RIN_TELEPORTATION };
    struct opvar *ov2 = selection_opvar((char *) 0), *ov3;
    schar x, y;
    boolean res = TRUE;

    selection_floodfill(ov2, nx, ny, TRUE);
    ov3 = opvar_clone(ov2);

    /* try to make a secret door */
    while (selection_rndcoord(ov3, &x, &y, TRUE)) {
        if (isok(x+1, y) && !selection_getpoint(x+1, y, ov)
            && IS_WALL(levl[x+1][y].typ)
            && isok(x+2, y) &&  selection_getpoint(x+2, y, ov)
            && ACCESSIBLE(levl[x+2][y].typ)) {
            levl[x+1][y].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x-1, y) && !selection_getpoint(x-1, y, ov)
            && IS_WALL(levl[x-1][y].typ)
            && isok(x-2, y) &&  selection_getpoint(x-2, y, ov)
            && ACCESSIBLE(levl[x-2][y].typ)) {
            levl[x-1][y].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x, y+1) && !selection_getpoint(x, y+1, ov)
            && IS_WALL(levl[x][y+1].typ)
            && isok(x, y+2) &&  selection_getpoint(x, y+2, ov)
            && ACCESSIBLE(levl[x][y+2].typ)) {
            levl[x][y+1].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x, y-1) && !selection_getpoint(x, y-1, ov)
            && IS_WALL(levl[x][y-1].typ)
            && isok(x, y-2) &&  selection_getpoint(x, y-2, ov)
            && ACCESSIBLE(levl[x][y-2].typ)) {
            levl[x][y-1].typ = SDOOR;
            goto gotitdone;
        }
    }

    /* try to make a hole or a trapdoor */
    if (Can_fall_thru(&u.uz)) {
        opvar_free(ov3);
        ov3 = opvar_clone(ov2);
        while (selection_rndcoord(ov3, &x, &y, TRUE)) {
            if (maketrap(x,y, rn2(2) ? HOLE : TRAPDOOR))
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
    opvar_free(ov2);
    opvar_free(ov3);
    return res;
}

STATIC_OVL void
ensure_way_out()
{
    static const char nhFunc[] = "ensure_way_out";
    struct opvar *ov = selection_opvar((char *) 0);
    struct trap *ttmp = ftrap;
    int x,y;
    boolean ret = TRUE;

    set_selection_floodfillchk(floodfillchk_match_accessible);

    if (xupstair && !selection_getpoint(xupstair, yupstair, ov))
        selection_floodfill(ov, xupstair, yupstair, TRUE);
    if (xdnstair && !selection_getpoint(xdnstair, ydnstair, ov))
        selection_floodfill(ov, xdnstair, ydnstair, TRUE);
    if (xupladder && !selection_getpoint(xupladder, yupladder, ov))
        selection_floodfill(ov, xupladder, yupladder, TRUE);
    if (xdnladder && !selection_getpoint(xdnladder, ydnladder, ov))
        selection_floodfill(ov, xdnladder, ydnladder, TRUE);

    while (ttmp) {
        if ((ttmp->ttyp == MAGIC_PORTAL || ttmp->ttyp == VIBRATING_SQUARE
             || is_hole(ttmp->ttyp))
            && !selection_getpoint(ttmp->tx, ttmp->ty, ov))
            selection_floodfill(ov, ttmp->tx, ttmp->ty, TRUE);
        ttmp = ttmp->ntrap;
    }

    do {
        ret = TRUE;
        for (x = 0; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++)
                if (ACCESSIBLE(levl[x][y].typ)
                    && !selection_getpoint(x, y, ov)) {
                    if (generate_way_out_method(x,y, ov))
                        selection_floodfill(ov, x,y, TRUE);
                    ret = FALSE;
                    goto outhere;
                }
    outhere: ;
    } while (!ret);
    opvar_free(ov);
}

void
spo_levregion(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_levregion";
    struct opvar *rname, *padding, *rtype, *del_islev, *dy2, *dx2, *dy1, *dx1,
        *in_islev, *iy2, *ix2, *iy1, *ix1;

    lev_region *tmplregion;

    if (!OV_pop_s(rname) || !OV_pop_i(padding) || !OV_pop_i(rtype)
        || !OV_pop_i(del_islev) || !OV_pop_i(dy2) || !OV_pop_i(dx2)
        || !OV_pop_i(dy1) || !OV_pop_i(dx1) || !OV_pop_i(in_islev)
        || !OV_pop_i(iy2) || !OV_pop_i(ix2) || !OV_pop_i(iy1)
        || !OV_pop_i(ix1))
        return;

    tmplregion = (lev_region *) alloc(sizeof(lev_region));

    tmplregion->inarea.x1 = OV_i(ix1);
    tmplregion->inarea.y1 = OV_i(iy1);
    tmplregion->inarea.x2 = OV_i(ix2);
    tmplregion->inarea.y2 = OV_i(iy2);

    tmplregion->delarea.x1 = OV_i(dx1);
    tmplregion->delarea.y1 = OV_i(dy1);
    tmplregion->delarea.x2 = OV_i(dx2);
    tmplregion->delarea.y2 = OV_i(dy2);

    tmplregion->in_islev = OV_i(in_islev);
    tmplregion->del_islev = OV_i(del_islev);
    tmplregion->rtype = OV_i(rtype);
    tmplregion->padding = OV_i(padding);
    tmplregion->rname.str = dupstr(OV_s(rname));

    if (!tmplregion->in_islev) {
        get_location(&tmplregion->inarea.x1, &tmplregion->inarea.y1, ANY_LOC,
                     (struct mkroom *) 0);
        get_location(&tmplregion->inarea.x2, &tmplregion->inarea.y2, ANY_LOC,
                     (struct mkroom *) 0);
    }

    if (!tmplregion->del_islev) {
        get_location(&tmplregion->delarea.x1, &tmplregion->delarea.y1,
                     ANY_LOC, (struct mkroom *) 0);
        get_location(&tmplregion->delarea.x2, &tmplregion->delarea.y2,
                     ANY_LOC, (struct mkroom *) 0);
    }
    if (num_lregions) {
        /* realloc the lregion space to add the new one */
        lev_region *newl = (lev_region *) alloc(
            sizeof(lev_region) * (unsigned) (1 + num_lregions));

        (void) memcpy((genericptr_t) (newl), (genericptr_t) lregions,
                      sizeof(lev_region) * num_lregions);
        Free(lregions);
        num_lregions++;
        lregions = newl;
    } else {
        num_lregions = 1;
        lregions = (lev_region *) alloc(sizeof(lev_region));
    }
    (void) memcpy(&lregions[num_lregions - 1], tmplregion,
                  sizeof(lev_region));
    free(tmplregion);

    opvar_free(dx1);
    opvar_free(dy1);
    opvar_free(dx2);
    opvar_free(dy2);

    opvar_free(ix1);
    opvar_free(iy1);
    opvar_free(ix2);
    opvar_free(iy2);

    opvar_free(del_islev);
    opvar_free(in_islev);
    opvar_free(rname);
    opvar_free(rtype);
    opvar_free(padding);
}

void
spo_region(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_region";
    struct opvar *rtype, *rlit, *rflags, *area;
    xchar dx1, dy1, dx2, dy2;
    register struct mkroom *troom;
    boolean prefilled, room_not_needed, irregular, joined;

    if (!OV_pop_i(rflags) || !OV_pop_i(rtype) || !OV_pop_i(rlit)
        || !OV_pop_r(area))
        return;

    prefilled = !(OV_i(rflags) & (1 << 0));
    irregular = (OV_i(rflags) & (1 << 1));
    joined = !(OV_i(rflags) & (1 << 2));

    if (OV_i(rtype) > MAXRTYPE) {
        OV_i(rtype) -= MAXRTYPE + 1;
        prefilled = TRUE;
    } else
        prefilled = FALSE;

    if (OV_i(rlit) < 0)
        OV_i(rlit) =
            (rnd(1 + abs(depth(&u.uz))) < 11 && rn2(77)) ? TRUE : FALSE;

    dx1 = SP_REGION_X1(OV_i(area));
    dy1 = SP_REGION_Y1(OV_i(area));
    dx2 = SP_REGION_X2(OV_i(area));
    dy2 = SP_REGION_Y2(OV_i(area));

    get_location(&dx1, &dy1, ANY_LOC, (struct mkroom *) 0);
    get_location(&dx2, &dy2, ANY_LOC, (struct mkroom *) 0);

    /* for an ordinary room, `prefilled' is a flag to force
       an actual room to be created (such rooms are used to
       control placement of migrating monster arrivals) */
    room_not_needed = (OV_i(rtype) == OROOM && !irregular && !prefilled);
    if (room_not_needed || nroom >= MAXNROFROOMS) {
        region tmpregion;
        if (!room_not_needed)
            impossible("Too many rooms on new level!");
        tmpregion.rlit = OV_i(rlit);
        tmpregion.x1 = dx1;
        tmpregion.y1 = dy1;
        tmpregion.x2 = dx2;
        tmpregion.y2 = dy2;
        light_region(&tmpregion);

        opvar_free(area);
        opvar_free(rflags);
        opvar_free(rlit);
        opvar_free(rtype);

        return;
    }

    troom = &rooms[nroom];

    /* mark rooms that must be filled, but do it later */
    if (OV_i(rtype) != OROOM)
        troom->needfill = (prefilled ? 2 : 1);

    troom->needjoining = joined;

    if (irregular) {
        min_rx = max_rx = dx1;
        min_ry = max_ry = dy1;
        smeq[nroom] = nroom;
        flood_fill_rm(dx1, dy1, nroom + ROOMOFFSET, OV_i(rlit), TRUE);
        add_room(min_rx, min_ry, max_rx, max_ry, FALSE, OV_i(rtype), TRUE);
        troom->rlit = OV_i(rlit);
        troom->irregular = TRUE;
    } else {
        add_room(dx1, dy1, dx2, dy2, OV_i(rlit), OV_i(rtype), TRUE);
#ifdef SPECIALIZATION
        topologize(troom, FALSE); /* set roomno */
#else
        topologize(troom); /* set roomno */
#endif
    }

    if (!room_not_needed) {
        if (coder->n_subroom > 1)
            impossible("region as subroom");
        else {
            coder->tmproomlist[coder->n_subroom] = troom;
            coder->failed_room[coder->n_subroom] = FALSE;
            coder->n_subroom++;
        }
    }

    opvar_free(area);
    opvar_free(rflags);
    opvar_free(rlit);
    opvar_free(rtype);
}

void
spo_drawbridge(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_drawbridge";
    xchar x, y;
    struct opvar *dir, *db_open, *dcoord;

    if (!OV_pop_i(dir) || !OV_pop_i(db_open) || !OV_pop_c(dcoord))
        return;

    get_location_coord(&x, &y, DRY | WET | HOT, coder->croom, OV_i(dcoord));
    if (!create_drawbridge(x, y, OV_i(dir), OV_i(db_open)))
        impossible("Cannot create drawbridge.");
    SpLev_Map[x][y] = 1;

    opvar_free(dcoord);
    opvar_free(db_open);
    opvar_free(dir);
}

void
spo_mazewalk(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_mazewalk";
    xchar x, y;
    struct opvar *ftyp, *fstocked, *fdir, *mcoord;
    int dir;

    if (!OV_pop_i(ftyp) || !OV_pop_i(fstocked) || !OV_pop_i(fdir)
        || !OV_pop_c(mcoord))
        return;

    dir = OV_i(fdir);

    get_location_coord(&x, &y, ANY_LOC, coder->croom, OV_i(mcoord));
    if (!isok(x, y))
        return;

    if (OV_i(ftyp) < 1) {
        OV_i(ftyp) = level.flags.corrmaze ? CORR : ROOM;
    }

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
        impossible("spo_mazewalk: Bad MAZEWALK direction");
    }

    if (!IS_DOOR(levl[x][y].typ)) {
        levl[x][y].typ = OV_i(ftyp);
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
        levl[x][y].typ = OV_i(ftyp);
        levl[x][y].flags = 0;
    }

    if (!(y % 2)) {
        if (dir == W_SOUTH)
            y++;
        else
            y--;
    }

    walkfrom(x, y, OV_i(ftyp));
    if (OV_i(fstocked))
        fill_empty_maze();

    opvar_free(mcoord);
    opvar_free(fdir);
    opvar_free(fstocked);
    opvar_free(ftyp);
}

void
spo_wall_property(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_wall_property";
    struct opvar *r;
    xchar dx1, dy1, dx2, dy2;
    int wprop = (coder->opcode == SPO_NON_DIGGABLE)
                   ? W_NONDIGGABLE
                   : W_NONPASSWALL;

    if (!OV_pop_r(r))
        return;

    dx1 = SP_REGION_X1(OV_i(r));
    dy1 = SP_REGION_Y1(OV_i(r));
    dx2 = SP_REGION_X2(OV_i(r));
    dy2 = SP_REGION_Y2(OV_i(r));

    get_location(&dx1, &dy1, ANY_LOC, (struct mkroom *) 0);
    get_location(&dx2, &dy2, ANY_LOC, (struct mkroom *) 0);

    set_wall_property(dx1, dy1, dx2, dy2, wprop);

    opvar_free(r);
}

void
spo_room_door(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_room_door";
    struct opvar *wall, *secret, *mask, *pos;
    room_door tmpd;

    if (!OV_pop_i(wall) || !OV_pop_i(secret) || !OV_pop_i(mask)
        || !OV_pop_i(pos) || !coder->croom)
        return;

    tmpd.secret = OV_i(secret);
    tmpd.mask = OV_i(mask);
    tmpd.pos = OV_i(pos);
    tmpd.wall = OV_i(wall);

    create_door(&tmpd, coder->croom);

    opvar_free(wall);
    opvar_free(secret);
    opvar_free(mask);
    opvar_free(pos);
}

/*ARGSUSED*/
void
sel_set_wallify(x, y, arg)
int x, y;
genericptr_t arg UNUSED;
{
    wallify_map(x, y, x, y);
}

void
spo_wallify(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_wallify";
    struct opvar *typ, *r;
    int dx1, dy1, dx2, dy2;

    if (!OV_pop_i(typ))
        return;
    switch (OV_i(typ)) {
    default:
    case 0:
        if (!OV_pop_r(r))
            return;
        dx1 = (xchar) SP_REGION_X1(OV_i(r));
        dy1 = (xchar) SP_REGION_Y1(OV_i(r));
        dx2 = (xchar) SP_REGION_X2(OV_i(r));
        dy2 = (xchar) SP_REGION_Y2(OV_i(r));
        wallify_map(dx1 < 0 ? (xstart - 1) : dx1,
                    dy1 < 0 ? (ystart - 1) : dy1,
                    dx2 < 0 ? (xstart + xsize + 1) : dx2,
                    dy2 < 0 ? (ystart + ysize + 1) : dy2);
        break;
    case 1:
        if (!OV_pop_typ(r, SPOVAR_SEL))
            return;
        selection_iterate(r, sel_set_wallify, NULL);
        break;
    }
    opvar_free(r);
    opvar_free(typ);
}

void
spo_map(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_map";
    mazepart tmpmazepart;
    struct opvar *mpxs, *mpys, *mpmap, *mpa, *mpkeepr, *mpzalign;
    xchar halign, valign;
    xchar tmpxstart, tmpystart, tmpxsize, tmpysize;
    unpacked_coord upc;

    if (!OV_pop_i(mpxs) || !OV_pop_i(mpys) || !OV_pop_s(mpmap)
        || !OV_pop_i(mpkeepr) || !OV_pop_i(mpzalign) || !OV_pop_c(mpa))
        return;

    tmpmazepart.xsize = OV_i(mpxs);
    tmpmazepart.ysize = OV_i(mpys);
    tmpmazepart.zaligntyp = OV_i(mpzalign);

    upc = get_unpacked_coord(OV_i(mpa), ANY_LOC);
    tmpmazepart.halign = upc.x;
    tmpmazepart.valign = upc.y;

    tmpxsize = xsize;
    tmpysize = ysize;
    tmpxstart = xstart;
    tmpystart = ystart;

    halign = tmpmazepart.halign;
    valign = tmpmazepart.valign;
    xsize = tmpmazepart.xsize;
    ysize = tmpmazepart.ysize;
    switch (tmpmazepart.zaligntyp) {
    default:
    case 0:
        break;
    case 1:
        switch ((int) halign) {
        case LEFT:
            xstart = splev_init_present ? 1 : 3;
            break;
        case H_LEFT:
            xstart = 2 + ((x_maze_max - 2 - xsize) / 4);
            break;
        case CENTER:
            xstart = 2 + ((x_maze_max - 2 - xsize) / 2);
            break;
        case H_RIGHT:
            xstart = 2 + ((x_maze_max - 2 - xsize) * 3 / 4);
            break;
        case RIGHT:
            xstart = x_maze_max - xsize - 1;
            break;
        }
        switch ((int) valign) {
        case TOP:
            ystart = 3;
            break;
        case CENTER:
            ystart = 2 + ((y_maze_max - 2 - ysize) / 2);
            break;
        case BOTTOM:
            ystart = y_maze_max - ysize - 1;
            break;
        }
        if (!(xstart % 2))
            xstart++;
        if (!(ystart % 2))
            ystart++;
        break;
    case 2:
        if (!coder->croom) {
            xstart = 1;
            ystart = 0;
            xsize = COLNO - 1 - tmpmazepart.xsize;
            ysize = ROWNO - tmpmazepart.ysize;
        }
        get_location_coord(&halign, &valign, ANY_LOC, coder->croom,
                           OV_i(mpa));
        xsize = tmpmazepart.xsize;
        ysize = tmpmazepart.ysize;
        xstart = halign;
        ystart = valign;
        break;
    }
    if (ystart < 0 || ystart + ysize > ROWNO) {
        /* try to move the start a bit */
        ystart += (ystart > 0) ? -2 : 2;
        if (ysize == ROWNO)
            ystart = 0;
        if (ystart < 0 || ystart + ysize > ROWNO)
            panic("reading special level with ysize too large");
    }
    if (xsize <= 1 && ysize <= 1) {
        xstart = 1;
        ystart = 0;
        xsize = COLNO - 1;
        ysize = ROWNO;
    } else {
        xchar x, y, mptyp;

        /* Load the map */
        for (y = ystart; y < ystart + ysize; y++)
            for (x = xstart; x < xstart + xsize; x++) {
                mptyp = (mpmap->vardata.str[(y - ystart) * xsize
                                                  + (x - xstart)] - 1);
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
                    if (x != xstart && (IS_WALL(levl[x - 1][y].typ)
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
        if (coder->lvl_is_joined)
            remove_rooms(xstart, ystart, xstart + xsize, ystart + ysize);
    }
    if (!OV_i(mpkeepr)) {
        xstart = tmpxstart;
        ystart = tmpystart;
        xsize = tmpxsize;
        ysize = tmpysize;
    }

    opvar_free(mpxs);
    opvar_free(mpys);
    opvar_free(mpmap);
    opvar_free(mpa);
    opvar_free(mpkeepr);
    opvar_free(mpzalign);
}

void
spo_jmp(coder, lvl)
struct sp_coder *coder;
sp_lev *lvl;
{
    static const char nhFunc[] = "spo_jmp";
    struct opvar *tmpa;
    long a;

    if (!OV_pop_i(tmpa))
        return;
    a = sp_code_jmpaddr(coder->frame->n_opcode, (OV_i(tmpa) - 1));
    if ((a >= 0) && (a < lvl->n_opcodes) && (a != coder->frame->n_opcode))
        coder->frame->n_opcode = a;
    opvar_free(tmpa);
}

void
spo_conditional_jump(coder, lvl)
struct sp_coder *coder;
sp_lev *lvl;
{
    static const char nhFunc[] = "spo_conditional_jump";
    struct opvar *oa, *oc;
    long a, c;
    int test = 0;

    if (!OV_pop_i(oa) || !OV_pop_i(oc))
        return;

    a = sp_code_jmpaddr(coder->frame->n_opcode, (OV_i(oa) - 1));
    c = OV_i(oc);

    switch (coder->opcode) {
    default:
        impossible("spo_conditional_jump: illegal opcode");
        break;
    case SPO_JL:
        test = (c & SP_CPUFLAG_LT);
        break;
    case SPO_JLE:
        test = (c & (SP_CPUFLAG_LT | SP_CPUFLAG_EQ));
        break;
    case SPO_JG:
        test = (c & SP_CPUFLAG_GT);
        break;
    case SPO_JGE:
        test = (c & (SP_CPUFLAG_GT | SP_CPUFLAG_EQ));
        break;
    case SPO_JE:
        test = (c & SP_CPUFLAG_EQ);
        break;
    case SPO_JNE:
        test = (c & ~SP_CPUFLAG_EQ);
        break;
    }

    if ((test) && (a >= 0) && (a < lvl->n_opcodes)
        && (a != coder->frame->n_opcode))
        coder->frame->n_opcode = a;

    opvar_free(oa);
    opvar_free(oc);
}

void
spo_var_init(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_var_init";
    struct opvar *vname;
    struct opvar *arraylen;
    struct opvar *vvalue;
    struct splev_var *tmpvar;
    struct splev_var *tmp2;
    long idx;

    OV_pop_s(vname);
    OV_pop_i(arraylen);

    if (!vname || !arraylen)
        panic("no values for SPO_VAR_INIT");

    tmpvar = opvar_var_defined(coder, OV_s(vname));

    if (tmpvar) {
        /* variable redefinition */
        if (OV_i(arraylen) < 0) {
            /* copy variable */
            if (tmpvar->array_len) {
                idx = tmpvar->array_len;
                while (idx-- > 0) {
                    opvar_free(tmpvar->data.arrayvalues[idx]);
                }
                Free(tmpvar->data.arrayvalues);
            } else {
                opvar_free(tmpvar->data.value);
            }
            tmpvar->data.arrayvalues = NULL;
            goto copy_variable;
        } else if (OV_i(arraylen)) {
            /* redefined array */
            idx = tmpvar->array_len;
            while (idx-- > 0) {
                opvar_free(tmpvar->data.arrayvalues[idx]);
            }
            Free(tmpvar->data.arrayvalues);
            tmpvar->data.arrayvalues = NULL;
            goto create_new_array;
        } else {
            /* redefined single value */
            OV_pop(vvalue);
            if (tmpvar->svtyp != vvalue->spovartyp)
                panic("redefining variable as different type");
            opvar_free(tmpvar->data.value);
            tmpvar->data.value = vvalue;
            tmpvar->array_len = 0;
        }
    } else {
        /* new variable definition */
        tmpvar = (struct splev_var *) alloc(sizeof(struct splev_var));
        tmpvar->next = coder->frame->variables;
        tmpvar->name = dupstr(OV_s(vname));
        coder->frame->variables = tmpvar;

        if (OV_i(arraylen) < 0) {
        /* copy variable */
        copy_variable:
            OV_pop(vvalue);
            tmp2 = opvar_var_defined(coder, OV_s(vvalue));
            if (!tmp2)
                panic("no copyable var");
            tmpvar->svtyp = tmp2->svtyp;
            tmpvar->array_len = tmp2->array_len;
            if (tmpvar->array_len) {
                idx = tmpvar->array_len;
                tmpvar->data.arrayvalues =
                    (struct opvar **) alloc(sizeof(struct opvar *) * idx);
                while (idx-- > 0) {
                    tmpvar->data.arrayvalues[idx] =
                        opvar_clone(tmp2->data.arrayvalues[idx]);
                }
            } else {
                tmpvar->data.value = opvar_clone(tmp2->data.value);
            }
            opvar_free(vvalue);
        } else if (OV_i(arraylen)) {
        /* new array */
        create_new_array:
            idx = OV_i(arraylen);
            tmpvar->array_len = idx;
            tmpvar->data.arrayvalues =
                (struct opvar **) alloc(sizeof(struct opvar *) * idx);
            while (idx-- > 0) {
                OV_pop(vvalue);
                if (!vvalue)
                    panic("no value for arrayvariable");
                tmpvar->data.arrayvalues[idx] = vvalue;
            }
            tmpvar->svtyp = SPOVAR_ARRAY;
        } else {
            /* new single value */
            OV_pop(vvalue);
            if (!vvalue)
                panic("no value for variable");
            tmpvar->svtyp = OV_typ(vvalue);
            tmpvar->data.value = vvalue;
            tmpvar->array_len = 0;
        }
    }

    opvar_free(vname);
    opvar_free(arraylen);

}

#if 0
STATIC_OVL long
opvar_array_length(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "opvar_array_length";
    struct opvar *vname;
    struct splev_var *tmp;
    long len = 0;

    if (!coder)
        return 0;

    vname = splev_stack_pop(coder->stack);
    if (!vname)
        return 0;
    if (vname->spovartyp != SPOVAR_VARIABLE)
        goto pass;

    tmp = coder->frame->variables;
    while (tmp) {
        if (!strcmp(tmp->name, OV_s(vname))) {
            if ((tmp->svtyp & SPOVAR_ARRAY)) {
                len = tmp->array_len;
                if (len < 1)
                    len = 0;
            }
            goto pass;
        }
        tmp = tmp->next;
    }

pass:
    opvar_free(vname);
    return len;
}
#endif /*0*/

void
spo_shuffle_array(coder)
struct sp_coder *coder;
{
    static const char nhFunc[] = "spo_shuffle_array";
    struct opvar *vname;
    struct splev_var *tmp;
    struct opvar *tmp2;
    long i, j;

    if (!OV_pop_s(vname))
        return;

    tmp = opvar_var_defined(coder, OV_s(vname));
    if (!tmp || (tmp->array_len < 1)) {
        opvar_free(vname);
        return;
    }
    for (i = tmp->array_len - 1; i > 0; i--) {
        if ((j = rn2(i + 1)) == i)
            continue;
        tmp2 = tmp->data.arrayvalues[j];
        tmp->data.arrayvalues[j] = tmp->data.arrayvalues[i];
        tmp->data.arrayvalues[i] = tmp2;
    }

    opvar_free(vname);
}

/* Special level coder, creates the special level from the sp_lev codes.
 * Does not free the allocated memory.
 */
STATIC_OVL boolean
sp_level_coder(lvl)
sp_lev *lvl;
{
    static const char nhFunc[] = "sp_level_coder";
    unsigned long exec_opcodes = 0;
    int tmpi;
    long room_stack = 0;
    unsigned long max_execution = SPCODER_MAX_RUNTIME;
    struct sp_coder *coder =
        (struct sp_coder *) alloc(sizeof (struct sp_coder));

    coder->frame = frame_new(0);
    coder->stack = NULL;
    coder->premapped = FALSE;
    coder->solidify = FALSE;
    coder->check_inaccessibles = FALSE;
    coder->croom = NULL;
    coder->n_subroom = 1;
    coder->exit_script = FALSE;
    coder->lvl_is_joined = 0;

    splev_init_present = FALSE;
    icedpools = FALSE;
    /* achievement tracking; static init would suffice except we need to
       reset if #wizmakemap is used to recreate mines' end or sokoban end;
       once either level is created, these values can be forgotten */
    mines_prize_count = soko_prize_count = 0;

    if (wizard) {
        char *met = nh_getenv("SPCODER_MAX_RUNTIME");

        if (met && met[0] == '1')
            max_execution = (1 << 30) - 1;
    }

    for (tmpi = 0; tmpi <= MAX_NESTED_ROOMS; tmpi++) {
        coder->tmproomlist[tmpi] = (struct mkroom *) 0;
        coder->failed_room[tmpi] = FALSE;
    }

    shuffle_alignments();

    for (tmpi = 0; tmpi < MAX_CONTAINMENT; tmpi++)
        container_obj[tmpi] = NULL;
    container_idx = 0;

    invent_carrying_monster = NULL;

    (void) memset((genericptr_t) &SpLev_Map[0][0], 0, sizeof SpLev_Map);

    level.flags.is_maze_lev = 0;

    xstart = 1;
    ystart = 0;
    xsize = COLNO - 1;
    ysize = ROWNO;

    while (coder->frame->n_opcode < lvl->n_opcodes && !coder->exit_script) {
        coder->opcode = lvl->opcodes[coder->frame->n_opcode].opcode;
        coder->opdat = lvl->opcodes[coder->frame->n_opcode].opdat;

        coder->stack = coder->frame->stack;

        if (exec_opcodes++ > max_execution) {
            impossible("Level script is taking too much time, stopping.");
            coder->exit_script = TRUE;
        }

        if (coder->failed_room[coder->n_subroom - 1]
            && coder->opcode != SPO_ENDROOM && coder->opcode != SPO_ROOM
            && coder->opcode != SPO_SUBROOM)
            goto next_opcode;

        coder->croom = coder->tmproomlist[coder->n_subroom - 1];

        switch (coder->opcode) {
        case SPO_NULL:
            break;
        case SPO_EXIT:
            coder->exit_script = TRUE;
            break;
        case SPO_FRAME_PUSH:
            spo_frame_push(coder);
            break;
        case SPO_FRAME_POP:
            spo_frame_pop(coder);
            break;
        case SPO_CALL:
            spo_call(coder);
            break;
        case SPO_RETURN:
            spo_return(coder);
            break;
        case SPO_END_MONINVENT:
            spo_end_moninvent(coder);
            break;
        case SPO_POP_CONTAINER:
            spo_pop_container(coder);
            break;
        case SPO_POP: {
            struct opvar *ov = splev_stack_pop(coder->stack);

            opvar_free(ov);
            break;
        }
        case SPO_PUSH:
            splev_stack_push(coder->stack, opvar_clone(coder->opdat));
            break;
        case SPO_MESSAGE:
            spo_message(coder);
            break;
        case SPO_MONSTER:
            spo_monster(coder);
            break;
        case SPO_OBJECT:
            spo_object(coder);
            break;
        case SPO_LEVEL_FLAGS:
            spo_level_flags(coder);
            break;
        case SPO_INITLEVEL:
            spo_initlevel(coder);
            break;
        case SPO_ENGRAVING:
            spo_engraving(coder);
            break;
        case SPO_MINERALIZE:
            spo_mineralize(coder);
            break;
        case SPO_SUBROOM:
        case SPO_ROOM:
            if (!coder->failed_room[coder->n_subroom - 1]) {
                spo_room(coder);
            } else
                room_stack++;
            break;
        case SPO_ENDROOM:
            if (coder->failed_room[coder->n_subroom - 1]) {
                if (!room_stack)
                    spo_endroom(coder);
                else
                    room_stack--;
            } else {
                spo_endroom(coder);
            }
            break;
        case SPO_DOOR:
            spo_door(coder);
            break;
        case SPO_STAIR:
            spo_stair(coder);
            break;
        case SPO_LADDER:
            spo_ladder(coder);
            break;
        case SPO_GRAVE:
            spo_grave(coder);
            break;
        case SPO_ALTAR:
            spo_altar(coder);
            break;
        case SPO_SINK:
        case SPO_POOL:
        case SPO_FOUNTAIN:
            spo_feature(coder);
            break;
        case SPO_TRAP:
            spo_trap(coder);
            break;
        case SPO_GOLD:
            spo_gold(coder);
            break;
        case SPO_CORRIDOR:
            spo_corridor(coder);
            break;
        case SPO_TERRAIN:
            spo_terrain(coder);
            break;
        case SPO_REPLACETERRAIN:
            spo_replace_terrain(coder);
            break;
        case SPO_LEVREGION:
            spo_levregion(coder);
            break;
        case SPO_REGION:
            spo_region(coder);
            break;
        case SPO_DRAWBRIDGE:
            spo_drawbridge(coder);
            break;
        case SPO_MAZEWALK:
            spo_mazewalk(coder);
            break;
        case SPO_NON_PASSWALL:
        case SPO_NON_DIGGABLE:
            spo_wall_property(coder);
            break;
        case SPO_ROOM_DOOR:
            spo_room_door(coder);
            break;
        case SPO_WALLIFY:
            spo_wallify(coder);
            break;
        case SPO_COPY: {
            struct opvar *a = splev_stack_pop(coder->stack);

            splev_stack_push(coder->stack, opvar_clone(a));
            splev_stack_push(coder->stack, opvar_clone(a));
            opvar_free(a);
            break;
        }
        case SPO_DEC: {
            struct opvar *a;

            if (!OV_pop_i(a))
                break;
            OV_i(a)--;
            splev_stack_push(coder->stack, a);
            break;
        }
        case SPO_INC: {
            struct opvar *a;

            if (!OV_pop_i(a))
                break;
            OV_i(a)++;
            splev_stack_push(coder->stack, a);
            break;
        }
        case SPO_MATH_SIGN: {
            struct opvar *a;

            if (!OV_pop_i(a))
                break;
            OV_i(a) = ((OV_i(a) < 0) ? -1 : ((OV_i(a) > 0) ? 1 : 0));
            splev_stack_push(coder->stack, a);
            break;
        }
        case SPO_MATH_ADD: {
            struct opvar *a, *b;

            if (!OV_pop(b) || !OV_pop(a))
                break;
            if (OV_typ(b) == OV_typ(a)) {
                if (OV_typ(a) == SPOVAR_INT) {
                    OV_i(a) = OV_i(a) + OV_i(b);
                    splev_stack_push(coder->stack, a);
                    opvar_free(b);
                } else if (OV_typ(a) == SPOVAR_STRING) {
                    struct opvar *c;
                    char *tmpbuf = (char *) alloc(strlen(OV_s(a))
                                                  + strlen(OV_s(b)) + 1);

                    (void) sprintf(tmpbuf, "%s%s", OV_s(a), OV_s(b));
                    c = opvar_new_str(tmpbuf);
                    splev_stack_push(coder->stack, c);
                    opvar_free(a);
                    opvar_free(b);
                    Free(tmpbuf);
                } else {
                    splev_stack_push(coder->stack, a);
                    opvar_free(b);
                    impossible("adding weird types");
                }
            } else {
                splev_stack_push(coder->stack, a);
                opvar_free(b);
                impossible("adding different types");
            }
            break;
        }
        case SPO_MATH_SUB: {
            struct opvar *a, *b;

            if (!OV_pop_i(b) || !OV_pop_i(a))
                break;
            OV_i(a) = OV_i(a) - OV_i(b);
            splev_stack_push(coder->stack, a);
            opvar_free(b);
            break;
        }
        case SPO_MATH_MUL: {
            struct opvar *a, *b;

            if (!OV_pop_i(b) || !OV_pop_i(a))
                break;
            OV_i(a) = OV_i(a) * OV_i(b);
            splev_stack_push(coder->stack, a);
            opvar_free(b);
            break;
        }
        case SPO_MATH_DIV: {
            struct opvar *a, *b;

            if (!OV_pop_i(b) || !OV_pop_i(a))
                break;
            if (OV_i(b) >= 1) {
                OV_i(a) = OV_i(a) / OV_i(b);
            } else {
                OV_i(a) = 0;
            }
            splev_stack_push(coder->stack, a);
            opvar_free(b);
            break;
        }
        case SPO_MATH_MOD: {
            struct opvar *a, *b;

            if (!OV_pop_i(b) || !OV_pop_i(a))
                break;
            if (OV_i(b) > 0) {
                OV_i(a) = OV_i(a) % OV_i(b);
            } else {
                OV_i(a) = 0;
            }
            splev_stack_push(coder->stack, a);
            opvar_free(b);
            break;
        }
        case SPO_CMP: {
            struct opvar *a;
            struct opvar *b;
            struct opvar *c;
            long val = 0;

            OV_pop(b);
            OV_pop(a);
            if (!a || !b) {
                impossible("spo_cmp: no values in stack");
                break;
            }
            if (OV_typ(a) != OV_typ(b)) {
                impossible("spo_cmp: trying to compare differing datatypes");
                break;
            }
            switch (OV_typ(a)) {
            case SPOVAR_COORD:
            case SPOVAR_REGION:
            case SPOVAR_MAPCHAR:
            case SPOVAR_MONST:
            case SPOVAR_OBJ:
            case SPOVAR_INT:
                if (OV_i(b) > OV_i(a))
                    val |= SP_CPUFLAG_LT;
                if (OV_i(b) < OV_i(a))
                    val |= SP_CPUFLAG_GT;
                if (OV_i(b) == OV_i(a))
                    val |= SP_CPUFLAG_EQ;
                c = opvar_new_int(val);
                break;
            case SPOVAR_STRING:
                c = opvar_new_int(!strcmp(OV_s(b), OV_s(a))
                                     ? SP_CPUFLAG_EQ
                                     : 0);
                break;
            default:
                c = opvar_new_int(0);
                break;
            }
            splev_stack_push(coder->stack, c);
            opvar_free(a);
            opvar_free(b);
            break;
        }
        case SPO_JMP:
            spo_jmp(coder, lvl);
            break;
        case SPO_JL:
        case SPO_JLE:
        case SPO_JG:
        case SPO_JGE:
        case SPO_JE:
        case SPO_JNE:
            spo_conditional_jump(coder, lvl);
            break;
        case SPO_RN2: {
            struct opvar *tmpv;
            struct opvar *t;

            if (!OV_pop_i(tmpv))
                break;
            t = opvar_new_int((OV_i(tmpv) > 1) ? rn2(OV_i(tmpv)) : 0);
            splev_stack_push(coder->stack, t);
            opvar_free(tmpv);
            break;
        }
        case SPO_DICE: {
            struct opvar *a, *b, *t;

            if (!OV_pop_i(b) || !OV_pop_i(a))
                break;
            if (OV_i(b) < 1)
                OV_i(b) = 1;
            if (OV_i(a) < 1)
                OV_i(a) = 1;
            t = opvar_new_int(d(OV_i(a), OV_i(b)));
            splev_stack_push(coder->stack, t);
            opvar_free(a);
            opvar_free(b);
            break;
        }
        case SPO_MAP:
            spo_map(coder);
            break;
        case SPO_VAR_INIT:
            spo_var_init(coder);
            break;
        case SPO_SHUFFLE_ARRAY:
            spo_shuffle_array(coder);
            break;
        case SPO_SEL_ADD: /* actually, logical or */
        {
            struct opvar *sel1, *sel2, *pt;

            if (!OV_pop_typ(sel1, SPOVAR_SEL))
                panic("no sel1 for add");
            if (!OV_pop_typ(sel2, SPOVAR_SEL))
                panic("no sel2 for add");
            pt = selection_logical_oper(sel1, sel2, '|');
            opvar_free(sel1);
            opvar_free(sel2);
            splev_stack_push(coder->stack, pt);
            break;
        }
        case SPO_SEL_COMPLEMENT: {
            struct opvar *sel, *pt;

            if (!OV_pop_typ(sel, SPOVAR_SEL))
                panic("no sel for not");
            pt = selection_not(sel);
            opvar_free(sel);
            splev_stack_push(coder->stack, pt);
            break;
        }
        case SPO_SEL_FILTER: /* sorta like logical and */
        {
            struct opvar *filtertype;

            if (!OV_pop_i(filtertype))
                panic("no sel filter type");
            switch (OV_i(filtertype)) {
            case SPOFILTER_PERCENT: {
                struct opvar *tmp1, *sel;

                if (!OV_pop_i(tmp1))
                    panic("no sel filter percent");
                if (!OV_pop_typ(sel, SPOVAR_SEL))
                    panic("no sel filter");
                selection_filter_percent(sel, OV_i(tmp1));
                splev_stack_push(coder->stack, sel);
                opvar_free(tmp1);
                break;
            }
            case SPOFILTER_SELECTION: /* logical and */
            {
                struct opvar *pt, *sel1, *sel2;

                if (!OV_pop_typ(sel1, SPOVAR_SEL))
                    panic("no sel filter sel1");
                if (!OV_pop_typ(sel2, SPOVAR_SEL))
                    panic("no sel filter sel2");
                pt = selection_logical_oper(sel1, sel2, '&');
                splev_stack_push(coder->stack, pt);
                opvar_free(sel1);
                opvar_free(sel2);
                break;
            }
            case SPOFILTER_MAPCHAR: {
                struct opvar *pt, *tmp1, *sel;

                if (!OV_pop_typ(sel, SPOVAR_SEL))
                    panic("no sel filter");
                if (!OV_pop_typ(tmp1, SPOVAR_MAPCHAR))
                    panic("no sel filter mapchar");
                pt = selection_filter_mapchar(sel, tmp1);
                splev_stack_push(coder->stack, pt);
                opvar_free(tmp1);
                opvar_free(sel);
                break;
            }
            default:
                panic("unknown sel filter type");
            }
            opvar_free(filtertype);
            break;
        }
        case SPO_SEL_POINT: {
            struct opvar *tmp;
            struct opvar *pt = selection_opvar((char *) 0);
            schar x, y;

            if (!OV_pop_c(tmp))
                panic("no ter sel coord");
            get_location_coord(&x, &y, ANY_LOC, coder->croom, OV_i(tmp));
            selection_setpoint(x, y, pt, 1);
            splev_stack_push(coder->stack, pt);
            opvar_free(tmp);
            break;
        }
        case SPO_SEL_RECT:
        case SPO_SEL_FILLRECT: {
            struct opvar *tmp, *pt = selection_opvar((char *) 0);
            schar x, y, x1, y1, x2, y2;

            if (!OV_pop_r(tmp))
                panic("no ter sel region");
            x1 = min(SP_REGION_X1(OV_i(tmp)), SP_REGION_X2(OV_i(tmp)));
            y1 = min(SP_REGION_Y1(OV_i(tmp)), SP_REGION_Y2(OV_i(tmp)));
            x2 = max(SP_REGION_X1(OV_i(tmp)), SP_REGION_X2(OV_i(tmp)));
            y2 = max(SP_REGION_Y1(OV_i(tmp)), SP_REGION_Y2(OV_i(tmp)));
            get_location(&x1, &y1, ANY_LOC, coder->croom);
            get_location(&x2, &y2, ANY_LOC, coder->croom);
            x1 = (x1 < 0) ? 0 : x1;
            y1 = (y1 < 0) ? 0 : y1;
            x2 = (x2 >= COLNO) ? COLNO - 1 : x2;
            y2 = (y2 >= ROWNO) ? ROWNO - 1 : y2;
            if (coder->opcode == SPO_SEL_RECT) {
                for (x = x1; x <= x2; x++) {
                    selection_setpoint(x, y1, pt, 1);
                    selection_setpoint(x, y2, pt, 1);
                }
                for (y = y1; y <= y2; y++) {
                    selection_setpoint(x1, y, pt, 1);
                    selection_setpoint(x2, y, pt, 1);
                }
            } else {
                for (x = x1; x <= x2; x++)
                    for (y = y1; y <= y2; y++)
                        selection_setpoint(x, y, pt, 1);
            }
            splev_stack_push(coder->stack, pt);
            opvar_free(tmp);
            break;
        }
        case SPO_SEL_LINE: {
            struct opvar *tmp = NULL, *tmp2 = NULL,
                *pt = selection_opvar((char *) 0);
            schar x1, y1, x2, y2;

            if (!OV_pop_c(tmp))
                panic("no ter sel linecoord1");
            if (!OV_pop_c(tmp2))
                panic("no ter sel linecoord2");
            get_location_coord(&x1, &y1, ANY_LOC, coder->croom, OV_i(tmp));
            get_location_coord(&x2, &y2, ANY_LOC, coder->croom, OV_i(tmp2));
            x1 = (x1 < 0) ? 0 : x1;
            y1 = (y1 < 0) ? 0 : y1;
            x2 = (x2 >= COLNO) ? COLNO - 1 : x2;
            y2 = (y2 >= ROWNO) ? ROWNO - 1 : y2;
            selection_do_line(x1, y1, x2, y2, pt);
            splev_stack_push(coder->stack, pt);
            opvar_free(tmp);
            opvar_free(tmp2);
            break;
        }
        case SPO_SEL_RNDLINE: {
            struct opvar *tmp = NULL, *tmp2 = NULL, *tmp3,
                *pt = selection_opvar((char *) 0);
            schar x1, y1, x2, y2;

            if (!OV_pop_i(tmp3))
                panic("no ter sel randline1");
            if (!OV_pop_c(tmp))
                panic("no ter sel randline2");
            if (!OV_pop_c(tmp2))
                panic("no ter sel randline3");
            get_location_coord(&x1, &y1, ANY_LOC, coder->croom, OV_i(tmp));
            get_location_coord(&x2, &y2, ANY_LOC, coder->croom, OV_i(tmp2));
            x1 = (x1 < 0) ? 0 : x1;
            y1 = (y1 < 0) ? 0 : y1;
            x2 = (x2 >= COLNO) ? COLNO - 1 : x2;
            y2 = (y2 >= ROWNO) ? ROWNO - 1 : y2;
            selection_do_randline(x1, y1, x2, y2, OV_i(tmp3), 12, pt);
            splev_stack_push(coder->stack, pt);
            opvar_free(tmp);
            opvar_free(tmp2);
            opvar_free(tmp3);
            break;
        }
        case SPO_SEL_GROW: {
            struct opvar *dirs, *pt;

            if (!OV_pop_i(dirs))
                panic("no dirs for grow");
            if (!OV_pop_typ(pt, SPOVAR_SEL))
                panic("no selection for grow");
            selection_do_grow(pt, OV_i(dirs));
            splev_stack_push(coder->stack, pt);
            opvar_free(dirs);
            break;
        }
        case SPO_SEL_FLOOD: {
            struct opvar *tmp;
            schar x, y;

            if (!OV_pop_c(tmp))
                panic("no ter sel flood coord");
            get_location_coord(&x, &y, ANY_LOC, coder->croom, OV_i(tmp));
            if (isok(x, y)) {
                struct opvar *pt = selection_opvar((char *) 0);

                set_selection_floodfillchk(floodfillchk_match_under);
                floodfillchk_match_under_typ = levl[x][y].typ;
                selection_floodfill(pt, x, y, FALSE);
                splev_stack_push(coder->stack, pt);
            }
            opvar_free(tmp);
            break;
        }
        case SPO_SEL_RNDCOORD: {
            struct opvar *pt;
            schar x, y;

            if (!OV_pop_typ(pt, SPOVAR_SEL))
                panic("no selection for rndcoord");
            if (selection_rndcoord(pt, &x, &y, FALSE)) {
                x -= xstart;
                y -= ystart;
            }
            splev_stack_push(coder->stack, opvar_new_coord(x, y));
            opvar_free(pt);
            break;
        }
        case SPO_SEL_ELLIPSE: {
            struct opvar *filled, *xaxis, *yaxis, *pt;
            struct opvar *sel = selection_opvar((char *) 0);
            schar x, y;

            if (!OV_pop_i(filled))
                panic("no filled for ellipse");
            if (!OV_pop_i(yaxis))
                panic("no yaxis for ellipse");
            if (!OV_pop_i(xaxis))
                panic("no xaxis for ellipse");
            if (!OV_pop_c(pt))
                panic("no pt for ellipse");
            get_location_coord(&x, &y, ANY_LOC, coder->croom, OV_i(pt));
            selection_do_ellipse(sel, x, y, OV_i(xaxis), OV_i(yaxis),
                                 OV_i(filled));
            splev_stack_push(coder->stack, sel);
            opvar_free(filled);
            opvar_free(yaxis);
            opvar_free(xaxis);
            opvar_free(pt);
            break;
        }
        case SPO_SEL_GRADIENT: {
            struct opvar *gtyp, *glim, *mind, *maxd, *gcoord, *coord2;
            struct opvar *sel;
            schar x, y, x2, y2;

            if (!OV_pop_i(gtyp))
                panic("no gtyp for grad");
            if (!OV_pop_i(glim))
                panic("no glim for grad");
            if (!OV_pop_c(coord2))
                panic("no coord2 for grad");
            if (!OV_pop_c(gcoord))
                panic("no coord for grad");
            if (!OV_pop_i(maxd))
                panic("no maxd for grad");
            if (!OV_pop_i(mind))
                panic("no mind for grad");
            get_location_coord(&x, &y, ANY_LOC, coder->croom, OV_i(gcoord));
            get_location_coord(&x2, &y2, ANY_LOC, coder->croom, OV_i(coord2));

            sel = selection_opvar((char *) 0);
            selection_do_gradient(sel, x, y, x2, y2, OV_i(gtyp), OV_i(mind),
                                  OV_i(maxd), OV_i(glim));
            splev_stack_push(coder->stack, sel);

            opvar_free(gtyp);
            opvar_free(glim);
            opvar_free(gcoord);
            opvar_free(coord2);
            opvar_free(maxd);
            opvar_free(mind);
            break;
        }
        default:
            panic("sp_level_coder: Unknown opcode %i", coder->opcode);
        }

    next_opcode:
        coder->frame->n_opcode++;
    } /*while*/

    link_doors_rooms();
    fill_rooms();
    remove_boundary_syms();

    if (coder->check_inaccessibles)
        ensure_way_out();
    /* FIXME: Ideally, we want this call to only cover areas of the map
     * which were not inserted directly by the special level file (see
     * the insect legs on Baalzebub's level, for instance). Since that
     * is currently not possible, we overload the corrmaze flag for this
     * purpose.
     */
    if (!level.flags.corrmaze)
        wallification(1, 0, COLNO - 1, ROWNO - 1);

    count_features();

    if (coder->solidify)
        solidify_map();

    /* This must be done before sokoban_detect(),
     * otherwise branch stairs won't be premapped. */
    fixup_special();

    if (coder->premapped)
        sokoban_detect();

    if (coder->frame) {
        struct sp_frame *tmpframe;
        do {
            tmpframe = coder->frame->next;
            frame_del(coder->frame);
            coder->frame = tmpframe;
        } while (coder->frame);
    }
    Free(coder);

    return TRUE;
}

/*
 * General loader
 */
boolean
load_special(name)
const char *name;
{
    dlb *fd;
    sp_lev *lvl = NULL;
    boolean result = FALSE;
    struct version_info vers_info;

    fd = dlb_fopen(name, RDBMODE);
    if (!fd)
        return FALSE;
    Fread((genericptr_t) &vers_info, sizeof vers_info, 1, fd);
    if (!check_version(&vers_info, name, TRUE)) {
        (void) dlb_fclose(fd);
        goto give_up;
    }

    lvl = (sp_lev *) alloc(sizeof (sp_lev));
    result = sp_level_loader(fd, lvl);
    (void) dlb_fclose(fd);
    if (result)
        result = sp_level_coder(lvl);
    sp_level_free(lvl);
    Free(lvl);

give_up:
    return result;
}

#ifdef _MSC_VER
 #pragma warning(pop)
#endif

/*sp_lev.c*/

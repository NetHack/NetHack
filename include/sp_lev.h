/* NetHack 3.7	sp_lev.h	$NHDT-Date: 1599434249 2020/09/06 23:17:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.39 $ */
/* Copyright (c) 1989 by Jean-Christophe Collet			  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SP_LEV_H
#define SP_LEV_H

/* wall directions */
#define W_NORTH 1
#define W_SOUTH 2
#define W_EAST 4
#define W_WEST 8
#define W_ANY (W_NORTH | W_SOUTH | W_EAST | W_WEST)

/* MAP limits */
#define MAP_X_LIM 76
#define MAP_Y_LIM 21

/* Per level flags */
#define NOTELEPORT 0x00000001L
#define HARDFLOOR 0x00000002L
#define NOMMAP 0x00000004L
#define SHORTSIGHTED 0x00000008L
#define ARBOREAL 0x00000010L
#define MAZELEVEL 0x00000020L
#define PREMAPPED 0x00000040L /* premapped level & sokoban rules */
#define SHROUD 0x00000080L
#define GRAVEYARD 0x00000100L
#define ICEDPOOLS 0x00000200L /* for ice locations: ICED_POOL vs ICED_MOAT \
                                 */
#define SOLIDIFY 0x00000400L  /* outer areas are nondiggable & nonpasswall */
#define CORRMAZE 0x00000800L  /* for maze levels only */
#define CHECK_INACCESSIBLES 0x00001000L /* check for inaccessible areas and
   generate ways to escape from them */

/* different level layout initializers */
enum lvlinit_types {
    LVLINIT_NONE = 0,
    LVLINIT_SOLIDFILL,
    LVLINIT_MAZEGRID,
    LVLINIT_MAZE,
    LVLINIT_MINES,
    LVLINIT_ROGUE,
    LVLINIT_SWAMP
};

/* max. nested depth of subrooms */
#define MAX_NESTED_ROOMS 5

/* When creating objects, we need to know whether
 * it's a container and/or contents.
 */
#define SP_OBJ_CONTENT 0x1
#define SP_OBJ_CONTAINER 0x2

/* SPO_FILTER types */
#define SPOFILTER_PERCENT 0
#define SPOFILTER_SELECTION 1
#define SPOFILTER_MAPCHAR 2

/* gradient filter types */
#define SEL_GRADIENT_RADIAL 0
#define SEL_GRADIENT_SQUARE 1

/* light states for terrain replacements, specifically for SET_TYPLIT
 * (not used for init_level) */
#define SET_LIT_RANDOM -1
#define SET_LIT_NOCHANGE -2

#define SP_COORD_IS_RANDOM 0x01000000L
/* Humidity flags for get_location() and friends, used with
 * SP_COORD_PACK_RANDOM() */
#define DRY         0x01
#define WET         0x02
#define HOT         0x04
#define SOLID       0x08
#define ANY_LOC     0x10 /* even outside the level */
#define NO_LOC_WARN 0x20 /* no complaints and set x & y to -1, if no loc */
#define SPACELOC    0x40 /* like DRY, but accepts furniture too */

#define SP_COORD_X(l) (l & 0xff)
#define SP_COORD_Y(l) ((l >> 16) & 0xff)
#define SP_COORD_PACK(x, y) (((x) & 0xff) + (((y) & 0xff) << 16))
#define SP_COORD_PACK_RANDOM(f) (SP_COORD_IS_RANDOM | (f))

struct sp_coder {
    int premapped;
    boolean solidify;
    struct mkroom *croom;
    int room_stack;
    struct mkroom *tmproomlist[MAX_NESTED_ROOMS + 1];
    boolean failed_room[MAX_NESTED_ROOMS + 1];
    int n_subroom;
    int lvl_is_joined;
    boolean check_inaccessibles;
    int allow_flips;
};

/*
 * Structures manipulated by the special levels loader & compiler
 */

#define packed_coord long
typedef struct {
    xchar is_random;
    long getloc_flags;
    int x, y;
} unpacked_coord;

typedef struct {
    xchar init_style; /* one of LVLINIT_foo */
    long flags;
    schar filling;
    boolean init_present, padding;
    char fg, bg;
    boolean smoothed, joined;
    xchar lit, walled;
    boolean icedpools;
    int corrwid, wallthick;
    boolean rm_deadends;
} lev_init;

typedef struct {
    xchar wall, pos, secret, mask;
} room_door;

typedef struct {
    packed_coord coord;
    xchar x, y, type;
} spltrap;

typedef struct {
    Str_or_Len name, appear_as;
    short id;
    aligntyp align;
    packed_coord coord;
    xchar x, y, class, appear;
    schar peaceful, asleep;
    short female, invis, cancelled, revived, avenge, fleeing, blinded,
        paralyzed, stunned, confused;
    long seentraps;
    short has_invent;
} monster;

typedef struct {
    Str_or_Len name;
    int corpsenm;
    short id, spe;
    packed_coord coord;
    xchar x, y, class, containment;
    schar curse_state;
    int quan;
    short buried;
    short lit;
    short eroded, locked, trapped, recharged, invis, greased, broken,
          achievement;
} object;

typedef struct {
    packed_coord coord;
    xchar x, y;
    aligntyp align;
    xchar shrine;
} altar;

typedef struct {
    xchar x1, y1, x2, y2;
    xchar rtype, rlit, rirreg;
} region;

typedef struct {
    xchar ter, tlit;
} terrain;

typedef struct {
    struct {
        xchar room;
        xchar wall;
        xchar door;
    } src, dest;
} corridor;

typedef struct _room {
    Str_or_Len name;
    Str_or_Len parent;
    xchar x, y, w, h;
    xchar xalign, yalign;
    xchar rtype, chance, rlit, filled, joined;
} room;

struct mapfragment {
    int wid, hei;
    char *data;
};

#define SET_TYPLIT(x, y, ttyp, llit) \
    {                                                             \
        if ((x) >= 1 && (y) >= 0 && (x) < COLNO && (y) < ROWNO) { \
            if ((ttyp) < MAX_TYPE)                                \
                levl[(x)][(y)].typ = (ttyp);                      \
            if ((ttyp) == LAVAPOOL)                               \
                levl[(x)][(y)].lit = 1;                           \
            else if ((schar)(llit) != SET_LIT_NOCHANGE) {         \
                if ((schar)(llit) == SET_LIT_RANDOM)              \
                    levl[(x)][(y)].lit = rn2(2);                  \
                else                                              \
                    levl[(x)][(y)].lit = (llit);                  \
            }                                                     \
        }                                                         \
    }

#endif /* SP_LEV_H */

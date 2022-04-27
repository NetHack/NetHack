/* NetHack 3.7	rm.h	$NHDT-Date: 1651099392 2022/04/27 22:43:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.94 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RM_H
#define RM_H

/*
 * The dungeon presentation graphics code and data structures were rewritten
 * and generalized for NetHack's release 2 by Eric S. Raymond (eric@snark)
 * building on Don G Kneller's MS-DOS implementation.	See drawing.c for
 * the code that permits the user to set the contents of the symbol structure.
 *
 * The door representation was changed by Ari
 * Huttunen(ahuttune@niksula.hut.fi)
 */

/*
 * TLCORNER     TDWALL          TRCORNER
 * +-           -+-             -+
 * |             |               |
 *
 * TRWALL       CROSSWALL       TLWALL          HWALL
 * |             |               |
 * +-           -+-             -+              ---
 * |             |               |
 *
 * BLCORNER     TUWALL          BRCORNER        VWALL
 * |             |               |              |
 * +-           -+-             -+              |
 */

/* Level location types.  [Some debugging code in src/display.c
   defines array type_names[] which contains an entry for each of
   these, so needs to be kept in sync if any new types are added
   or existing ones renumbered.] */
enum levl_typ_types {
    STONE     =  0,
    VWALL     =  1,
    HWALL     =  2,
    TLCORNER  =  3,
    TRCORNER  =  4,
    BLCORNER  =  5,
    BRCORNER  =  6,
    CROSSWALL =  7, /* For pretty mazes and special levels */
    TUWALL    =  8,
    TDWALL    =  9,
    TLWALL    = 10,
    TRWALL    = 11,
    DBWALL    = 12,
    TREE      = 13, /* KMH */
    SDOOR     = 14,
    SCORR     = 15,
    POOL      = 16,
    MOAT      = 17, /* pool that doesn't boil, adjust messages */
    WATER     = 18,
    DRAWBRIDGE_UP = 19,
    LAVAPOOL  = 20,
    IRONBARS  = 21, /* KMH */
    DOOR      = 22,
    CORR      = 23,
    ROOM      = 24,
    STAIRS    = 25,
    LADDER    = 26,
    FOUNTAIN  = 27,
    THRONE    = 28,
    SINK      = 29,
    GRAVE     = 30,
    ALTAR     = 31,
    ICE       = 32,
    DRAWBRIDGE_DOWN = 33,
    AIR       = 34,
    CLOUD     = 35,

    MAX_TYPE  = 36,
    MATCH_WALL = 37,
    INVALID_TYPE = 127
};

/*
 * Avoid using the level types in inequalities:
 * these types are subject to change.
 * Instead, use one of the macros below.
 */
#define IS_WALL(typ) ((typ) && (typ) <= DBWALL)
#define IS_STWALL(typ) ((typ) <= DBWALL) /* STONE <= (typ) <= DBWALL */
#define IS_ROCK(typ) ((typ) < POOL)      /* absolutely nonaccessible */
#define IS_DOOR(typ) ((typ) == DOOR)
#define IS_DOORJOIN(typ) (IS_ROCK(typ) || (typ) == IRONBARS)
#define IS_TREE(typ)                                            \
    ((typ) == TREE || (g.level.flags.arboreal && (typ) == STONE))
#define ACCESSIBLE(typ) ((typ) >= DOOR) /* good position */
#define IS_ROOM(typ) ((typ) >= ROOM)    /* ROOM, STAIRS, furniture.. */
#define ZAP_POS(typ) ((typ) >= POOL)
#define SPACE_POS(typ) ((typ) > DOOR)
#define IS_POOL(typ) ((typ) >= POOL && (typ) <= DRAWBRIDGE_UP)
#define IS_THRONE(typ) ((typ) == THRONE)
#define IS_FOUNTAIN(typ) ((typ) == FOUNTAIN)
#define IS_SINK(typ) ((typ) == SINK)
#define IS_GRAVE(typ) ((typ) == GRAVE)
#define IS_ALTAR(typ) ((typ) == ALTAR)
#define IS_DRAWBRIDGE(typ) \
    ((typ) == DRAWBRIDGE_UP || (typ) == DRAWBRIDGE_DOWN)
#define IS_FURNITURE(typ) ((typ) >= STAIRS && (typ) <= ALTAR)
#define IS_AIR(typ) ((typ) == AIR || (typ) == CLOUD)
#define IS_SOFT(typ) ((typ) == AIR || (typ) == CLOUD || IS_POOL(typ))
#define IS_WATERWALL(typ) ((typ) == WATER)

/*
 *      Note:  secret doors (SDOOR) want to use both rm.doormask and
 *      rm.wall_info but those both overload rm.flags.  SDOOR only
 *      has 2 states (closed or locked).  However, it can't specify
 *      D_CLOSED due to that conflicting with WM_MASK (below).  When
 *      a secret door is revealed, the door gets set to D_CLOSED iff
 *      it isn't set to D_LOCKED (see cvt_sdoor_to_door() in detect.c).
 *
 *      D_LOCKED conflicts with W_NONDIGGABLE but the latter is not
 *      expected to be used on door locations.  D_TRAPPED conflicts
 *      with W_NONPASSWALL but secret doors aren't trapped.
 *      D_SECRET would not fit within struct rm's 5-bit 'flags' field.
 */

/*
 * The 5 possible states of doors.
 * For historical reasons they are numbered as mask bits rather than 0..4.
 * The trapped flag is OR'd onto the state and only valid if that state
 * is closed or locked.
 * The no-door state allows egress when moving diagonally, others do not.
 */
#define D_NODOOR  0x00
#define D_BROKEN  0x01
#define D_ISOPEN  0x02
#define D_CLOSED  0x04
#define D_LOCKED  0x08

#define D_TRAPPED 0x10
#define D_SECRET  0x20 /* only used by sp_lev.c, NOT in rm-struct */

/*
 * Thrones should only be looted once.
 */
#define T_LOOTED 1

/*
 * Trees have more than one kick result.
 */
#define TREE_LOOTED 1
#define TREE_SWARM 2

/*
 * Fountains have limits, and special warnings.
 */
#define F_LOOTED 1
#define F_WARNED 2
#define FOUNTAIN_IS_WARNED(x, y) (levl[x][y].looted & F_WARNED)
#define FOUNTAIN_IS_LOOTED(x, y) (levl[x][y].looted & F_LOOTED)
#define SET_FOUNTAIN_WARNED(x, y) levl[x][y].looted |= F_WARNED;
#define SET_FOUNTAIN_LOOTED(x, y) levl[x][y].looted |= F_LOOTED;
#define CLEAR_FOUNTAIN_WARNED(x, y) levl[x][y].looted &= ~F_WARNED;
#define CLEAR_FOUNTAIN_LOOTED(x, y) levl[x][y].looted &= ~F_LOOTED;

/*
 * doors are even worse :-) The special warning has a side effect
 * of instantly trapping the door, and if it was defined as trapped,
 * the guards consider that you have already been warned!
 */
#define D_WARNED 16

/*
 * Sinks have 3 different types of loot that shouldn't be abused
 */
#define S_LPUDDING 1
#define S_LDWASHER 2
#define S_LRING 4

/*
 * The four directions for a DrawBridge.
 */
#define DB_NORTH 0
#define DB_SOUTH 1
#define DB_EAST 2
#define DB_WEST 3
#define DB_DIR 3 /* mask for direction */

/*
 * What's under a drawbridge.
 */
#define DB_MOAT 0
#define DB_LAVA 4
#define DB_ICE 8
#define DB_FLOOR 16
#define DB_UNDER 28 /* mask for underneath */

/*
 * Wall information.  Nondiggable also applies to iron bars.
 */
#define WM_MASK 0x07 /* wall mode (bottom three bits) */
#define W_NONDIGGABLE 0x08
#define W_NONPASSWALL 0x10

/*
 * Ladders (in Vlad's tower) may be up or down.
 */
#define LA_UP 1
#define LA_DOWN 2

/*
 * Room areas may be iced pools
 */
#define ICED_POOL 8
#define ICED_MOAT 16

/*
 * The structure describing a coordinate position.
 * Before adding fields, remember that this will significantly affect
 * the size of temporary files and save files.
 *
 * Also remember that the run-length encoding for some ports in save.c
 * must be updated to consider the field.
 */
struct rm {
    int glyph;               /* what the hero thinks is there */
    schar typ;               /* what is really there  [why is this signed?] */
    uchar seenv;             /* seen vector */
    Bitfield(flags, 5);      /* extra information for typ */
    Bitfield(horizontal, 1); /* wall/door/etc is horiz. (more typ info) */
    Bitfield(lit, 1);        /* speed hack for lit rooms */
    Bitfield(waslit, 1);     /* remember if a location was lit */

    Bitfield(roomno, 6); /* room # for special rooms */
    Bitfield(edge, 1);   /* marks boundaries for special rooms*/
    Bitfield(candig, 1); /* Exception to Can_dig_down; was a trapdoor */
};

/* light states for terrain replacements, for set_levltyp_lit */
#define SET_LIT_RANDOM -1
#define SET_LIT_NOCHANGE -2

#define CAN_OVERWRITE_TERRAIN(ttyp) \
    (iflags.debug_overwrite_stairs || !((ttyp) == LADDER || (ttyp) == STAIRS))

/*
 * Add wall angle viewing by defining "modes" for each wall type.  Each
 * mode describes which parts of a wall are finished (seen as as wall)
 * and which are unfinished (seen as rock).
 *
 * We use the bottom 3 bits of the flags field for the mode.  This comes
 * in conflict with secret doors, but we avoid problems because until
 * a secret door becomes discovered, we know what sdoor's bottom three
 * bits are.
 *
 * The following should cover all of the cases.
 *
 *      type    mode                            Examples: R=rock, F=finished
 *      -----   ----                            ----------------------------
 *      WALL:   0 none                          hwall, mode 1
 *              1 left/top (1/2 rock)                   RRR
 *              2 right/bottom (1/2 rock)               ---
 *                                                      FFF
 *
 *      CORNER: 0 none                          trcorn, mode 2
 *              1 outer (3/4 rock)                      FFF
 *              2 inner (1/4 rock)                      F+-
 *                                                      F|R
 *
 *      TWALL:  0 none                          tlwall, mode 3
 *              1 long edge (1/2 rock)                  F|F
 *              2 bottom left (on a tdwall)             -+F
 *              3 bottom right (on a tdwall)            R|F
 *
 *      CRWALL: 0 none                          crwall, mode 5
 *              1 top left (1/4 rock)                   R|F
 *              2 top right (1/4 rock)                  -+-
 *              3 bottom left (1/4 rock)                F|R
 *              4 bottom right (1/4 rock)
 *              5 top left & bottom right (1/2 rock)
 *              6 bottom left & top right (1/2 rock)
 */

#define WM_W_LEFT 1 /* vertical or horizontal wall */
#define WM_W_RIGHT 2
#define WM_W_TOP WM_W_LEFT
#define WM_W_BOTTOM WM_W_RIGHT

#define WM_C_OUTER 1 /* corner wall */
#define WM_C_INNER 2

#define WM_T_LONG 1 /* T wall */
#define WM_T_BL 2
#define WM_T_BR 3

#define WM_X_TL 1 /* cross wall */
#define WM_X_TR 2
#define WM_X_BL 3
#define WM_X_BR 4
#define WM_X_TLBR 5
#define WM_X_BLTR 6

/*
 * Seen vector values.  The seen vector is an array of 8 bits, one for each
 * octant around a given center x:
 *
 *              0 1 2
 *              7 x 3
 *              6 5 4
 *
 * In the case of walls, a single wall square can be viewed from 8 possible
 * directions.  If we know the type of wall and the directions from which
 * it has been seen, then we can determine what it looks like to the hero.
 */
#define SV0   0x01
#define SV1   0x02
#define SV2   0x04
#define SV3   0x08
#define SV4   0x10
#define SV5   0x20
#define SV6   0x40
#define SV7   0x80
#define SVALL 0xFF

/* if these get changed or expanded, make sure wizard-mode wishing becomes
   aware of the new usage */
#define doormask   flags /* door, sdoor (note conflict with wall_info) */
#define altarmask  flags /* alignment and maybe temple */
#define wall_info  flags /* wall, sdoor (note conflict with doormask) */
#define ladder     flags /* up or down */
#define drawbridgemask flags /* what's underneath when the span is open */
#define looted     flags /* used for throne, tree, fountain, sink, door */
#define icedpool   flags /* used for ice (in case it melts) */
/* horizonal applies to walls, doors (including sdoor); also to iron bars
   even though they don't have separate symbols for horizontal and vertical */
#define blessedftn horizontal /* a fountain that grants attribs */
#define disturbed  horizontal /* a grave that has been disturbed */

struct damage {
    struct damage *next;
    long when, cost;
    coord place;
    schar typ; /* from struct rm */
    uchar flags; /* also from struct rm; an unsigned 5-bit field there */
};

/* for bones levels:  identify the dead character, who might have died on
   an existing bones level; if so, most recent victim will be first in list */
struct cemetery {
    struct cemetery *next; /* next struct is previous dead character... */
    /* "g.plname" + "-ROLe" + "-RACe" + "-GENder" + "-ALIgnment" + \0 */
    char who[PL_NSIZ + 4 * (1 + 3) + 1];
    /* death reason, same as in score/log file */
    char how[100 + 1]; /* [DTHSZ+1] */
    /* date+time in string of digits rather than binary */
    char when[4 + 2 + 2 + 2 + 2 + 2 + 1]; /* "YYYYMMDDhhmmss\0" */
    /* final resting place spot */
    xchar frpx, frpy;
    boolean bonesknown;
};

struct levelflags {
    uchar nfountains; /* number of fountains on level */
    uchar nsinks;     /* number of sinks on the level */
    /* Several flags that give hints about what's on the level */
    Bitfield(has_shop, 1);
    Bitfield(has_vault, 1);
    Bitfield(has_zoo, 1);
    Bitfield(has_court, 1);
    Bitfield(has_morgue, 1);
    Bitfield(has_beehive, 1);
    Bitfield(has_barracks, 1);
    Bitfield(has_temple, 1);

    Bitfield(has_swamp, 1);
    Bitfield(noteleport, 1);
    Bitfield(hardfloor, 1);
    Bitfield(nommap, 1);
    Bitfield(hero_memory, 1);   /* hero has memory */
    Bitfield(shortsighted, 1);  /* monsters are shortsighted */
    Bitfield(graveyard, 1);     /* has_morgue, but remains set */
    Bitfield(sokoban_rules, 1); /* fill pits and holes w/ boulders */

    Bitfield(is_maze_lev, 1);
    Bitfield(is_cavernous_lev, 1);
    Bitfield(arboreal, 1);     /* Trees replace rock */
    Bitfield(wizard_bones, 1); /* set if level came from a bones file
                                  which was created in wizard mode (or
                                  normal mode descendant of such) */
    Bitfield(corrmaze, 1);     /* Whether corridors are used for the maze
                                  rather than ROOM */
};

typedef struct {
    struct rm locations[COLNO][ROWNO];
    struct obj *objects[COLNO][ROWNO];
    struct monst *monsters[COLNO][ROWNO];
    struct obj *objlist;
    struct obj *buriedobjlist;
    struct monst *monlist;
    struct damage *damagelist;
    struct cemetery *bonesinfo;
    struct levelflags flags;
} dlevel_t;

/*
 * Macros for compatibility with old code. Someday these will go away.
 */
#define levl g.level.locations
#define fobj g.level.objlist
#define fmon g.level.monlist

/*
 * Covert a trap number into the defsym graphics array.
 * Convert a defsym number into a trap number.
 * Assumes that arrow trap will always be the first trap.
 */
#define trap_to_defsym(t) (S_arrow_trap + (t) - 1)
#define defsym_to_trap(d) ((d) - S_arrow_trap + 1)

#define OBJ_AT(x, y) (g.level.objects[x][y] != (struct obj *) 0)
/*
 * Macros for encapsulation of level.monsters references.
 */
#if 0
#define MON_AT(x, y) \
    (g.level.monsters[x][y] != (struct monst *) 0 \
     && !(g.level.monsters[x][y])->mburied)
#define MON_BURIED_AT(x, y) \
    (g.level.monsters[x][y] != (struct monst *) 0 \
     && (g.level.monsters[x][y])->mburied)
#else   /* without 'mburied' */
#define MON_AT(x, y) (g.level.monsters[x][y] != (struct monst *) 0)
#endif
#ifdef EXTRA_SANITY_CHECKS
#define place_worm_seg(m, x, y) \
    do {                                                            \
        if (g.level.monsters[x][y] && g.level.monsters[x][y] != m)  \
            impossible("place_worm_seg over mon");                  \
        g.level.monsters[x][y] = m;                                 \
    } while(0)
#define remove_monster(x, y) \
    do {                                                  \
        if (!g.level.monsters[x][y])                      \
            impossible("no monster to remove");           \
        g.level.monsters[x][y] = (struct monst *) 0;      \
    } while(0)
#else
#define place_worm_seg(m, x, y) g.level.monsters[x][y] = m
#define remove_monster(x, y) g.level.monsters[x][y] = (struct monst *) 0
#endif
#define m_at(x, y) (MON_AT(x, y) ? g.level.monsters[x][y] : (struct monst *) 0)
#define m_buried_at(x, y) \
    (MON_BURIED_AT(x, y) ? g.level.monsters[x][y] : (struct monst *) 0)

/* restricted movement, potential luck penalties */
#define Sokoban g.level.flags.sokoban_rules

/*
 * These prototypes are in extern.h but some of the code which uses them
 * includes config.h instead of hack.h so doesn't see extern.h.
 */
/* ### drawing.c ### */
extern int def_char_to_objclass(char);
extern int def_char_to_monclass(char);
extern int def_char_is_furniture(char);

#endif /* RM_H */

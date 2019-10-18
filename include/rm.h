/* NetHack 3.6	rm.h	$NHDT-Date: 1571313651 2019/10/17 12:00:51 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.63 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RM_H
#define RM_H

/*
 * The dungeon presentation graphics code and data structures were rewritten
 * and generalized for NetHack's release 2 by Eric S. Raymond (eric@snark)
 * building on Don G. Kneller's MS-DOS implementation.	See drawing.c for
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
    ((typ) == TREE || (level.flags.arboreal && (typ) == STONE))
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

/*
 * The screen symbols may be the default or defined at game startup time.
 * See drawing.c for defaults.
 * Note: {ibm|dec|curses}_graphics[] arrays (also in drawing.c) must be kept in
 * synch.
 */

/* begin dungeon characters */
enum screen_symbols {
    S_stone     =  0,
    S_vwall     =  1,
    S_hwall     =  2,
    S_tlcorn    =  3,
    S_trcorn    =  4,
    S_blcorn    =  5,
    S_brcorn    =  6,
    S_crwall    =  7,
    S_tuwall    =  8,
    S_tdwall    =  9,
    S_tlwall    = 10,
    S_trwall    = 11,
    S_ndoor     = 12,
    S_vodoor    = 13,
    S_hodoor    = 14,
    S_vcdoor    = 15, /* closed door, vertical wall */
    S_hcdoor    = 16, /* closed door, horizontal wall */
    S_bars      = 17, /* KMH -- iron bars */
    S_tree      = 18, /* KMH */
    S_room      = 19,
    S_darkroom  = 20,
    S_corr      = 21,
    S_litcorr   = 22,
    S_upstair   = 23,
    S_dnstair   = 24,
    S_upladder  = 25,
    S_dnladder  = 26,
    S_altar     = 27,
    S_grave     = 28,
    S_throne    = 29,
    S_sink      = 30,
    S_fountain  = 31,
    S_pool      = 32,
    S_ice       = 33,
    S_lava      = 34,
    S_vodbridge = 35,
    S_hodbridge = 36,
    S_vcdbridge = 37, /* closed drawbridge, vertical wall */
    S_hcdbridge = 38, /* closed drawbridge, horizontal wall */
    S_air       = 39,
    S_cloud     = 40,
    S_water     = 41,

/* end dungeon characters, begin traps */

    S_arrow_trap           = 42,
    S_dart_trap            = 43,
    S_falling_rock_trap    = 44,
    S_squeaky_board        = 45,
    S_bear_trap            = 46,
    S_land_mine            = 47,
    S_rolling_boulder_trap = 48,
    S_sleeping_gas_trap    = 49,
    S_rust_trap            = 50,
    S_fire_trap            = 51,
    S_pit                  = 52,
    S_spiked_pit           = 53,
    S_hole                 = 54,
    S_trap_door            = 55,
    S_teleportation_trap   = 56,
    S_level_teleporter     = 57,
    S_magic_portal         = 58,
    S_web                  = 59,
    S_statue_trap          = 60,
    S_magic_trap           = 61,
    S_anti_magic_trap      = 62,
    S_polymorph_trap       = 63,
    S_vibrating_square     = 64, /* for display rather than any trap effect */

/* end traps, begin special effects */

    S_vbeam     = 65, /* The 4 zap beam symbols.  Do NOT separate. */
    S_hbeam     = 66, /* To change order or add, see function      */
    S_lslant    = 67, /* zapdir_to_glyph() in display.c.           */
    S_rslant    = 68,
    S_digbeam   = 69, /* dig beam symbol */
    S_flashbeam = 70, /* camera flash symbol */
    S_boomleft  = 71, /* thrown boomerang, open left, e.g ')'    */
    S_boomright = 72, /* thrown boomerang, open right, e.g. '('  */
    S_ss1       = 73, /* 4 magic shield ("resistance sparkle") glyphs */
    S_ss2       = 74,
    S_ss3       = 75,
    S_ss4       = 76,
    S_poisoncloud = 77,
    S_goodpos   = 78, /* valid position for targeting via getpos() */

/* The 8 swallow symbols.  Do NOT separate.  To change order or add, */
/* see the function swallow_to_glyph() in display.c.                 */
    S_sw_tl     = 79, /* swallow top left [1]             */
    S_sw_tc     = 80, /* swallow top center [2]    Order: */
    S_sw_tr     = 81, /* swallow top right [3]            */
    S_sw_ml     = 82, /* swallow middle left [4]   1 2 3  */
    S_sw_mr     = 83, /* swallow middle right [6]  4 5 6  */
    S_sw_bl     = 84, /* swallow bottom left [7]   7 8 9  */
    S_sw_bc     = 85, /* swallow bottom center [8]        */
    S_sw_br     = 86, /* swallow bottom right [9]         */

    S_explode1  = 87, /* explosion top left               */
    S_explode2  = 88, /* explosion top center             */
    S_explode3  = 89, /* explosion top right        Ex.   */
    S_explode4  = 90, /* explosion middle left            */
    S_explode5  = 91, /* explosion middle center    /-\   */
    S_explode6  = 92, /* explosion middle right     |@|   */
    S_explode7  = 93, /* explosion bottom left      \-/   */
    S_explode8  = 94, /* explosion bottom center          */
    S_explode9  = 95, /* explosion bottom right           */

/* end effects */

    MAXPCHARS   = 96  /* maximum number of mapped characters */
};

#define MAXDCHARS (S_water - S_stone + 1) /* mapped dungeon characters */
#define MAXTCHARS (S_vibrating_square - S_arrow_trap + 1) /* trap chars */
#define MAXECHARS (S_explode9 - S_vbeam + 1) /* mapped effects characters */
#define MAXEXPCHARS 9 /* number of explosion characters */

#define DARKROOMSYM (Is_rogue_level(&u.uz) ? S_stone : S_darkroom)

#define is_cmap_trap(i) ((i) >= S_arrow_trap && (i) <= S_polymorph_trap)
#define is_cmap_drawbridge(i) ((i) >= S_vodbridge && (i) <= S_hcdbridge)
#define is_cmap_door(i) ((i) >= S_vodoor && (i) <= S_hcdoor)
#define is_cmap_wall(i) ((i) >= S_stone && (i) <= S_trwall)
#define is_cmap_room(i) ((i) >= S_room && (i) <= S_darkroom)
#define is_cmap_corr(i) ((i) >= S_corr && (i) <= S_litcorr)
#define is_cmap_furniture(i) ((i) >= S_upstair && (i) <= S_fountain)
#define is_cmap_water(i) ((i) == S_pool || (i) == S_water)
#define is_cmap_lava(i) ((i) == S_lava)


struct symdef {
    uchar sym;
    const char *explanation;
#ifdef TEXTCOLOR
    uchar color;
#endif
};

struct symparse {
    unsigned range;
#define SYM_CONTROL 1 /* start/finish markers */
#define SYM_PCHAR 2   /* index into showsyms  */
#define SYM_OC 3      /* index into oc_syms   */
#define SYM_MON 4     /* index into monsyms   */
#define SYM_OTH 5     /* misc                 */
    int idx;
    const char *name;
};

/* misc symbol definitions */
#define SYM_BOULDER 0
#define SYM_INVISIBLE 1
#define MAXOTHER 2

/* linked list of symsets and their characteristics */
struct symsetentry {
    struct symsetentry *next; /* next in list                         */
    char *name;               /* ptr to symset name                   */
    char *desc;               /* ptr to description                   */
    int idx;                  /* an index value                       */
    int handling;             /* known handlers value                 */
    Bitfield(nocolor, 1);     /* don't use color if set               */
    Bitfield(primary, 1);     /* restricted for use as primary set    */
    Bitfield(rogue, 1);       /* restricted for use as rogue lev set  */
    Bitfield(fallback, 1);    /* no explicit symset set               */
                              /* 4 free bits */
};

/*
 * Graphics sets for display symbols
 */
#define DEFAULT_GRAPHICS 0 /* regular characters: '-', '+', &c */
#define PRIMARY 0          /* primary graphics set        */
#define ROGUESET 1         /* rogue graphics set          */
#define NUM_GRAPHICS 2

/*
 * special symbol set handling types ( for invoking callbacks, etc.)
 * Must match the order of the known_handlers strings
 * in drawing.c
 */
#define H_UNK     0
#define H_IBM     1
#define H_DEC     2
#define H_CURS    3
#define H_MAC     4 /* obsolete; needed so that the listing of available
                     * symsets by 'O' can skip it for !MAC_GRAPHICS_ENV */

extern const struct symdef defsyms[MAXPCHARS]; /* defaults */
extern const struct symdef def_warnsyms[WARNCOUNT];
extern int currentgraphics; /* from drawing.c */
extern nhsym showsyms[];
extern nhsym l_syms[];
extern nhsym r_syms[];

extern struct symsetentry symset[NUM_GRAPHICS]; /* from drawing.c */
#define SYMHANDLING(ht) (symset[currentgraphics].handling == (ht))

/*
 * The 5 possible states of doors
 */

#define D_NODOOR 0
#define D_BROKEN 1
#define D_ISOPEN 2
#define D_CLOSED 4
#define D_LOCKED 8
#define D_TRAPPED 16
#define D_SECRET 32 /* only used by sp_lev.c, NOT in rm-struct */

/*
 * Some altars are considered as shrines, so we need a flag.
 */
#define AM_SHRINE 8

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
 * Doors are even worse :-) The special warning has a side effect
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
    schar typ;               /* what is really there */
    uchar seenv;             /* seen vector */
    Bitfield(flags, 5);      /* extra information for typ */
    Bitfield(horizontal, 1); /* wall/door/etc is horiz. (more typ info) */
    Bitfield(lit, 1);        /* speed hack for lit rooms */
    Bitfield(waslit, 1);     /* remember if a location was lit */

    Bitfield(roomno, 6); /* room # for special rooms */
    Bitfield(edge, 1);   /* marks boundaries for special rooms*/
    Bitfield(candig, 1); /* Exception to Can_dig_down; was a trapdoor */
};

#define SET_TYPLIT(x, y, ttyp, llit)                              \
    {                                                             \
        if ((x) >= 0 && (y) >= 0 && (x) < COLNO && (y) < ROWNO) { \
            if ((ttyp) < MAX_TYPE)                                \
                levl[(x)][(y)].typ = (ttyp);                      \
            if ((ttyp) == LAVAPOOL)                               \
                levl[(x)][(y)].lit = 1;                           \
            else if ((schar)(llit) != -2) {                       \
                if ((schar)(llit) == -1)                          \
                    levl[(x)][(y)].lit = rn2(2);                  \
                else                                              \
                    levl[(x)][(y)].lit = (llit);                  \
            }                                                     \
        }                                                         \
    }

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

#define doormask flags
#define altarmask flags
#define wall_info flags
#define ladder flags
#define drawbridgemask flags
#define looted flags
#define icedpool flags

#define blessedftn horizontal /* a fountain that grants attribs */
#define disturbed horizontal  /* a grave that has been disturbed */

struct damage {
    struct damage *next;
    long when, cost;
    coord place;
    schar typ;
};

/* for bones levels:  identify the dead character, who might have died on
   an existing bones level; if so, most recent victim will be first in list */
struct cemetery {
    struct cemetery *next; /* next struct is previous dead character... */
    /* "plname" + "-ROLe" + "-RACe" + "-GENder" + "-ALIgnment" + \0 */
    char who[PL_NSIZ + 4 * (1 + 3) + 1];
    /* death reason, same as in score/log file */
    char how[100 + 1]; /* [DTHSZ+1] */
    /* date+time in string of digits rather than binary */
    char when[4 + 2 + 2 + 2 + 2 + 2 + 1]; /* "YYYYMMDDhhmmss\0" */
    /* final resting place spot */
    schar frpx, frpy;
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
#ifndef MICROPORT_BUG
    struct obj *objects[COLNO][ROWNO];
    struct monst *monsters[COLNO][ROWNO];
#else
    struct obj *objects[1][ROWNO];
    char *yuk1[COLNO - 1][ROWNO];
    struct monst *monsters[1][ROWNO];
    char *yuk2[COLNO - 1][ROWNO];
#endif
    struct obj *objlist;
    struct obj *buriedobjlist;
    struct monst *monlist;
    struct damage *damagelist;
    struct cemetery *bonesinfo;
    struct levelflags flags;
} dlevel_t;

extern schar lastseentyp[COLNO][ROWNO]; /* last seen/touched dungeon typ */

extern dlevel_t level; /* structure describing the current level */

/*
 * Macros for compatibility with old code. Someday these will go away.
 */
#define levl level.locations
#define fobj level.objlist
#define fmon level.monlist

/*
 * Covert a trap number into the defsym graphics array.
 * Convert a defsym number into a trap number.
 * Assumes that arrow trap will always be the first trap.
 */
#define trap_to_defsym(t) (S_arrow_trap + (t) -1)
#define defsym_to_trap(d) ((d) -S_arrow_trap + 1)

#define OBJ_AT(x, y) (level.objects[x][y] != (struct obj *) 0)
/*
 * Macros for encapsulation of level.monsters references.
 */
#if 0
#define MON_AT(x, y)                            \
    (level.monsters[x][y] != (struct monst *) 0 \
     && !(level.monsters[x][y])->mburied)
#define MON_BURIED_AT(x, y)                     \
    (level.monsters[x][y] != (struct monst *) 0 \
     && (level.monsters[x][y])->mburied)
#else   /* without 'mburied' */
#define MON_AT(x, y) (level.monsters[x][y] != (struct monst *) 0)
#endif
#ifdef EXTRA_SANITY_CHECKS
#define place_worm_seg(m, x, y) \
    do {                                                        \
        if (level.monsters[x][y] && level.monsters[x][y] != m)  \
            impossible("place_worm_seg over mon");              \
        level.monsters[x][y] = m;                               \
    } while(0)
#define remove_monster(x, y) \
    do {                                                \
        if (!level.monsters[x][y])                      \
            impossible("no monster to remove");         \
        level.monsters[x][y] = (struct monst *) 0;      \
    } while(0)
#else
#define place_worm_seg(m, x, y) level.monsters[x][y] = m
#define remove_monster(x, y) level.monsters[x][y] = (struct monst *) 0
#endif
#define m_at(x, y) (MON_AT(x, y) ? level.monsters[x][y] : (struct monst *) 0)
#define m_buried_at(x, y) \
    (MON_BURIED_AT(x, y) ? level.monsters[x][y] : (struct monst *) 0)

/* restricted movement, potential luck penalties */
#define Sokoban level.flags.sokoban_rules

#endif /* RM_H */

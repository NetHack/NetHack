/*	SCCS Id: @(#)rm.h	3.4	1999/12/12	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RM_H
#define RM_H

/*
 * The dungeon presentation graphics code and data structures were rewritten
 * and generalized for NetHack's release 2 by Eric S. Raymond (eric@snark)
 * building on Don G. Kneller's MS-DOS implementation.	See drawing.c for
 * the code that permits the user to set the contents of the symbol structure.
 *
 * The door representation was changed by Ari Huttunen(ahuttune@niksula.hut.fi)
 */

/*
 * TLCORNER	TDWALL		TRCORNER
 * +-		-+-		-+
 * |		 |		 |
 *
 * TRWALL	CROSSWALL	TLWALL		HWALL
 * |		 |		 |
 * +-		-+-		-+		---
 * |		 |		 |
 *
 * BLCORNER	TUWALL		BRCORNER	VWALL
 * |		 |		 |		|
 * +-		-+-		-+		|
 */

/* Level location types */
#define STONE		0
#define VWALL		1
#define HWALL		2
#define TLCORNER	3
#define TRCORNER	4
#define BLCORNER	5
#define BRCORNER	6
#define CROSSWALL	7	/* For pretty mazes and special levels */
#define TUWALL		8
#define TDWALL		9
#define TLWALL		10
#define TRWALL		11
#define DBWALL		12
#define TREE		13	/* KMH */
#define SDOOR		14
#define SCORR		15
#define POOL		16
#define MOAT		17	/* pool that doesn't boil, adjust messages */
#define WATER		18
#define DRAWBRIDGE_UP	19
#define LAVAPOOL	20
#define IRONBARS	21	/* KMH */
#define DOOR		22
#define CORR		23
#define ROOM		24
#define STAIRS		25
#define LADDER		26
#define FOUNTAIN	27
#define THRONE		28
#define SINK		29
#define GRAVE		30
#define ALTAR		31
#define ICE		32
#define DRAWBRIDGE_DOWN 33
#define AIR		34
#define CLOUD		35

#define MAX_TYPE	36
#define INVALID_TYPE	127

/*
 * Avoid using the level types in inequalities:
 * these types are subject to change.
 * Instead, use one of the macros below.
 */
#define IS_WALL(typ)	((typ) && (typ) <= DBWALL)
#define IS_STWALL(typ)	((typ) <= DBWALL)	/* STONE <= (typ) <= DBWALL */
#define IS_ROCK(typ)	((typ) < POOL)		/* absolutely nonaccessible */
#define IS_DOOR(typ)	((typ) == DOOR)
#define IS_TREE(typ)	((typ) == TREE || \
			(level.flags.arboreal && (typ) == STONE))
#define ACCESSIBLE(typ) ((typ) >= DOOR)		/* good position */
#define IS_ROOM(typ)	((typ) >= ROOM)		/* ROOM, STAIRS, furniture.. */
#define ZAP_POS(typ)	((typ) >= POOL)
#define SPACE_POS(typ)	((typ) > DOOR)
#define IS_POOL(typ)	((typ) >= POOL && (typ) <= DRAWBRIDGE_UP)
#define IS_THRONE(typ)	((typ) == THRONE)
#define IS_FOUNTAIN(typ) ((typ) == FOUNTAIN)
#define IS_SINK(typ)	((typ) == SINK)
#define IS_GRAVE(typ)	((typ) == GRAVE)
#define IS_ALTAR(typ)	((typ) == ALTAR)
#define IS_DRAWBRIDGE(typ) ((typ) == DRAWBRIDGE_UP || (typ) == DRAWBRIDGE_DOWN)
#define IS_FURNITURE(typ) ((typ) >= STAIRS && (typ) <= ALTAR)
#define IS_AIR(typ)	((typ) == AIR || (typ) == CLOUD)
#define IS_SOFT(typ)	((typ) == AIR || (typ) == CLOUD || IS_POOL(typ))

/*
 * The screen symbols may be the default or defined at game startup time.
 * See drawing.c for defaults.
 * Note: {ibm|dec}_graphics[] arrays (also in drawing.c) must be kept in synch.
 */

/* begin dungeon characters */

#define S_stone		0
#define S_vwall		1
#define S_hwall		2
#define S_tlcorn	3
#define S_trcorn	4
#define S_blcorn	5
#define S_brcorn	6
#define S_crwall	7
#define S_tuwall	8
#define S_tdwall	9
#define S_tlwall	10
#define S_trwall	11
#define S_ndoor		12
#define S_vodoor	13
#define S_hodoor	14
#define S_vcdoor	15	/* closed door, vertical wall */
#define S_hcdoor	16	/* closed door, horizontal wall */
#define S_bars		17	/* KMH -- iron bars */
#define S_tree		18	/* KMH */
#define S_room		19
#define S_corr		20
#define S_litcorr	21
#define S_upstair	22
#define S_dnstair	23
#define S_upladder	24
#define S_dnladder	25
#define S_altar		26
#define S_grave		27
#define S_throne	28
#define S_sink		29
#define S_fountain	30
#define S_pool		31
#define S_ice		32
#define S_lava		33
#define S_vodbridge	34
#define S_hodbridge	35
#define S_vcdbridge	36	/* closed drawbridge, vertical wall */
#define S_hcdbridge	37	/* closed drawbridge, horizontal wall */
#define S_air		38
#define S_cloud		39
#define S_water		40

/* end dungeon characters, begin traps */

#define S_arrow_trap		41
#define S_dart_trap		42
#define S_falling_rock_trap	43
#define S_squeaky_board		44
#define S_bear_trap		45
#define S_land_mine		46
#define S_rolling_boulder_trap	47
#define S_sleeping_gas_trap	48
#define S_rust_trap		49
#define S_fire_trap		50
#define S_pit			51
#define S_spiked_pit		52
#define S_hole			53
#define S_trap_door		54
#define S_teleportation_trap	55
#define S_level_teleporter	56
#define S_magic_portal		57
#define S_web			58
#define S_statue_trap		59
#define S_magic_trap		60
#define S_anti_magic_trap	61
#define S_polymorph_trap	62

/* end traps, begin special effects */

#define S_vbeam		63	/* The 4 zap beam symbols.  Do NOT separate. */
#define S_hbeam		64	/* To change order or add, see function     */
#define S_lslant	65	/* zapdir_to_glyph() in display.c.	    */
#define S_rslant	66
#define S_digbeam	67	/* dig beam symbol */
#define S_flashbeam	68	/* camera flash symbol */
#define S_boomleft	69	/* thrown boomerang, open left, e.g ')'    */
#define S_boomright	70	/* thrown boomerand, open right, e.g. '('  */
#define S_ss1		71	/* 4 magic shield glyphs */
#define S_ss2		72
#define S_ss3		73
#define S_ss4		74

/* The 8 swallow symbols.  Do NOT separate.  To change order or add, see */
/* the function swallow_to_glyph() in display.c.			 */
#define S_sw_tl		75	/* swallow top left [1]			*/
#define S_sw_tc		76	/* swallow top center [2]	Order:	*/
#define S_sw_tr		77	/* swallow top right [3]		*/
#define S_sw_ml		78	/* swallow middle left [4]	1 2 3	*/
#define S_sw_mr		79	/* swallow middle right [6]	4 5 6	*/
#define S_sw_bl		80	/* swallow bottom left [7]	7 8 9	*/
#define S_sw_bc		81	/* swallow bottom center [8]		*/
#define S_sw_br		82	/* swallow bottom right [9]		*/

#define S_explode1	83	/* explosion top left			*/
#define S_explode2	84	/* explosion top center			*/
#define S_explode3	85	/* explosion top right		 Ex.	*/
#define S_explode4	86	/* explosion middle left		*/
#define S_explode5	87	/* explosion middle center	 /-\	*/
#define S_explode6	88	/* explosion middle right	 |@|	*/
#define S_explode7	89	/* explosion bottom left	 \-/	*/
#define S_explode8	90	/* explosion bottom center		*/
#define S_explode9	91	/* explosion bottom right		*/

/* end effects */

#define MAXPCHARS	92	/* maximum number of mapped characters */
#define MAXDCHARS	41	/* maximum of mapped dungeon characters */
#define MAXTCHARS	22	/* maximum of mapped trap characters */
#define MAXECHARS	29	/* maximum of mapped effects characters */
#define MAXEXPCHARS	9	/* number of explosion characters */

struct symdef {
    uchar sym;
    const char	*explanation;
#ifdef TEXTCOLOR
    uchar color;
#endif
};

extern const struct symdef defsyms[MAXPCHARS];	/* defaults */
extern uchar showsyms[MAXPCHARS];
extern const struct symdef def_warnsyms[WARNCOUNT];

/*
 * Graphics sets for display symbols
 */
#define ASCII_GRAPHICS	0	/* regular characters: '-', '+', &c */
#define IBM_GRAPHICS	1	/* PC graphic characters */
#define DEC_GRAPHICS	2	/* VT100 line drawing characters */
#define MAC_GRAPHICS	3	/* Macintosh drawing characters */

/*
 * The 5 possible states of doors
 */

#define D_NODOOR	0
#define D_BROKEN	1
#define D_ISOPEN	2
#define D_CLOSED	4
#define D_LOCKED	8
#define D_TRAPPED	16

/*
 * Some altars are considered as shrines, so we need a flag.
 */
#define AM_SHRINE	8

/*
 * Thrones should only be looted once.
 */
#define T_LOOTED	1

/*
 * Trees have more than one kick result.
 */
#define TREE_LOOTED	1
#define TREE_SWARM	2

/*
 * Fountains have limits, and special warnings.
 */
#define F_LOOTED	1
#define F_WARNED	2
#define FOUNTAIN_IS_WARNED(x,y)		(levl[x][y].looted & F_WARNED)
#define FOUNTAIN_IS_LOOTED(x,y)		(levl[x][y].looted & F_LOOTED)
#define SET_FOUNTAIN_WARNED(x,y)	levl[x][y].looted |= F_WARNED;
#define SET_FOUNTAIN_LOOTED(x,y)	levl[x][y].looted |= F_LOOTED;
#define CLEAR_FOUNTAIN_WARNED(x,y)	levl[x][y].looted &= ~F_WARNED;
#define CLEAR_FOUNTAIN_LOOTED(x,y)	levl[x][y].looted &= ~F_LOOTED;

/*
 * Doors are even worse :-) The special warning has a side effect
 * of instantly trapping the door, and if it was defined as trapped,
 * the guards consider that you have already been warned!
 */
#define D_WARNED	16

/*
 * Sinks have 3 different types of loot that shouldn't be abused
 */
#define S_LPUDDING	1
#define S_LDWASHER	2
#define S_LRING		4

/*
 * The four directions for a DrawBridge.
 */
#define DB_NORTH	0
#define DB_SOUTH	1
#define DB_EAST		2
#define DB_WEST		3
#define DB_DIR		3	/* mask for direction */

/*
 * What's under a drawbridge.
 */
#define DB_MOAT		0
#define DB_LAVA		4
#define DB_ICE		8
#define DB_FLOOR	16
#define DB_UNDER	28	/* mask for underneath */

/*
 * Wall information.
 */
#define WM_MASK		0x07	/* wall mode (bottom three bits) */
#define W_NONDIGGABLE	0x08
#define W_NONPASSWALL	0x10

/*
 * Ladders (in Vlad's tower) may be up or down.
 */
#define LA_UP		1
#define LA_DOWN		2

/*
 * Room areas may be iced pools
 */
#define ICED_POOL	8
#define ICED_MOAT	16

/*
 * The structure describing a coordinate position.
 * Before adding fields, remember that this will significantly affect
 * the size of temporary files and save files.
 */
struct rm {
	int glyph;		/* what the hero thinks is there */
	schar typ;		/* what is really there */
	uchar seenv;		/* seen vector */
	Bitfield(flags,5);	/* extra information for typ */
	Bitfield(horizontal,1); /* wall/door/etc is horiz. (more typ info) */
	Bitfield(lit,1);	/* speed hack for lit rooms */
	Bitfield(waslit,1);	/* remember if a location was lit */
	Bitfield(roomno,6);	/* room # for special rooms */
	Bitfield(edge,1);	/* marks boundaries for special rooms*/
};

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
 *	type	mode				Examples: R=rock, F=finished
 *	-----	----				----------------------------
 *	WALL:	0 none				hwall, mode 1
 *		1 left/top (1/2 rock)			RRR
 *		2 right/bottom (1/2 rock)		---
 *							FFF
 *
 *	CORNER: 0 none				trcorn, mode 2
 *		1 outer (3/4 rock)			FFF
 *		2 inner (1/4 rock)			F+-
 *							F|R
 *
 *	TWALL:	0 none				tlwall, mode 3
 *		1 long edge (1/2 rock)			F|F
 *		2 bottom left (on a tdwall)		-+F
 *		3 bottom right (on a tdwall)		R|F
 *
 *	CRWALL: 0 none				crwall, mode 5
 *		1 top left (1/4 rock)			R|F
 *		2 top right (1/4 rock)			-+-
 *		3 bottom left (1/4 rock)		F|R
 *		4 bottom right (1/4 rock)
 *		5 top left & bottom right (1/2 rock)
 *		6 bottom left & top right (1/2 rock)
 */

#define WM_W_LEFT 1			/* vertical or horizontal wall */
#define WM_W_RIGHT 2
#define WM_W_TOP WM_W_LEFT
#define WM_W_BOTTOM WM_W_RIGHT

#define WM_C_OUTER 1			/* corner wall */
#define WM_C_INNER 2

#define WM_T_LONG 1			/* T wall */
#define WM_T_BL   2
#define WM_T_BR   3

#define WM_X_TL   1			/* cross wall */
#define WM_X_TR   2
#define WM_X_BL   3
#define WM_X_BR   4
#define WM_X_TLBR 5
#define WM_X_BLTR 6

/*
 * Seen vector values.	The seen vector is an array of 8 bits, one for each
 * octant around a given center x:
 *
 *			0 1 2
 *			7 x 3
 *			6 5 4
 *
 * In the case of walls, a single wall square can be viewed from 8 possible
 * directions.	If we know the type of wall and the directions from which
 * it has been seen, then we can determine what it looks like to the hero.
 */
#define SV0 0x1
#define SV1 0x2
#define SV2 0x4
#define SV3 0x8
#define SV4 0x10
#define SV5 0x20
#define SV6 0x40
#define SV7 0x80
#define SVALL 0xFF



#define doormask	flags
#define altarmask	flags
#define wall_info	flags
#define ladder		flags
#define drawbridgemask	flags
#define looted		flags
#define icedpool	flags

#define blessedftn	horizontal  /* a fountain that grants attribs */
#define disturbed	horizontal  /* a grave that has been disturbed */

struct damage {
	struct damage *next;
	long when, cost;
	coord place;
	schar typ;
};

struct levelflags {
	uchar	nfountains;		/* number of fountains on level */
	uchar	nsinks;			/* number of sinks on the level */
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
	Bitfield(noteleport,1);
	Bitfield(hardfloor,1);
	Bitfield(nommap,1);
	Bitfield(hero_memory,1);	/* hero has memory */
	Bitfield(shortsighted,1);	/* monsters are shortsighted */
	Bitfield(graveyard,1);		/* has_morgue, but remains set */
	Bitfield(is_maze_lev,1);

	Bitfield(is_cavernous_lev,1);
	Bitfield(arboreal, 1);		/* Trees replace rock */
};

typedef struct
{
    struct rm		locations[COLNO][ROWNO];
#ifndef MICROPORT_BUG
    struct obj		*objects[COLNO][ROWNO];
    struct monst	*monsters[COLNO][ROWNO];
#else
    struct obj		*objects[1][ROWNO];
    char		*yuk1[COLNO-1][ROWNO];
    struct monst	*monsters[1][ROWNO];
    char		*yuk2[COLNO-1][ROWNO];
#endif
    struct obj		*objlist;
    struct obj		*buriedobjlist;
    struct monst	*monlist;
    struct damage	*damagelist;
    struct levelflags	flags;
}
dlevel_t;

extern dlevel_t level;	/* structure describing the current level */

/*
 * Macros for compatibility with old code. Someday these will go away.
 */
#define levl		level.locations
#define fobj		level.objlist
#define fmon		level.monlist

/*
 * Covert a trap number into the defsym graphics array.
 * Convert a defsym number into a trap number.
 * Assumes that arrow trap will always be the first trap.
 */
#define trap_to_defsym(t) (S_arrow_trap+(t)-1)
#define defsym_to_trap(d) ((d)-S_arrow_trap+1)

#define OBJ_AT(x,y)	(level.objects[x][y] != (struct obj *)0)
/*
 * Macros for encapsulation of level.monsters references.
 */
#define MON_AT(x,y)	(level.monsters[x][y] != (struct monst *)0 && \
			 !(level.monsters[x][y])->mburied)
#define MON_BURIED_AT(x,y)	(level.monsters[x][y] != (struct monst *)0 && \
				(level.monsters[x][y])->mburied)
#ifndef STEED
#define place_monster(m,x,y)	((m)->mx=(x),(m)->my=(y),\
				 level.monsters[(m)->mx][(m)->my]=(m))
#endif
#define place_worm_seg(m,x,y)	level.monsters[x][y] = m
#define remove_monster(x,y)	level.monsters[x][y] = (struct monst *)0
#define m_at(x,y)		(MON_AT(x,y) ? level.monsters[x][y] : \
						(struct monst *)0)
#define m_buried_at(x,y)	(MON_BURIED_AT(x,y) ? level.monsters[x][y] : \
						       (struct monst *)0)

#endif /* RM_H */

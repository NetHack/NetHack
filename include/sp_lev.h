/*	SCCS Id: @(#)sp_lev.h	3.4	1996/05/08	*/
/* Copyright (c) 1989 by Jean-Christophe Collet			  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SP_LEV_H
#define SP_LEV_H

    /* wall directions */
#define W_NORTH		1
#define W_SOUTH		2
#define W_EAST		4
#define W_WEST		8
#define W_ANY		(W_NORTH|W_SOUTH|W_EAST|W_WEST)

    /* MAP limits */
#define MAP_X_LIM	76
#define MAP_Y_LIM	21

    /* Per level flags */
#define NOTELEPORT	1
#define HARDFLOOR	2
#define NOMMAP		4
#define SHORTSIGHTED	8
#define ARBOREAL	16

    /* special level types */
#define SP_LEV_ROOMS	1
#define SP_LEV_MAZE	2

/*
 * Structures manipulated by the special levels loader & compiler
 */

typedef union str_or_len {
	char *str;
	int   len;
} Str_or_Len;

typedef struct {
	boolean init_present, padding;
	char	fg, bg;
	boolean smoothed, joined;
	xchar	lit, walled;
} lev_init;

typedef struct {
	xchar x, y, mask;
} door;

typedef struct {
	xchar wall, pos, secret, mask;
} room_door;

typedef struct {
	xchar x, y, chance, type;
} trap;

typedef struct {
	Str_or_Len name, appear_as;
	short id;
	aligntyp align;
	xchar x, y, chance, class, appear;
	schar peaceful, asleep;
} monster;

typedef struct {
	Str_or_Len name;
	int   corpsenm;
	short id, spe;
	xchar x, y, chance, class, containment;
	schar curse_state;
} object;

typedef struct {
	xchar		x, y;
	aligntyp	align;
	xchar		shrine;
} altar;

typedef struct {
	xchar x, y, dir, db_open;
} drawbridge;

typedef struct {
	xchar x, y, dir;
} walk;

typedef struct {
	xchar x1, y1, x2, y2;
} digpos;

typedef struct {
	xchar x, y, up;
} lad;

typedef struct {
	xchar x, y, up;
} stair;

typedef struct {
	xchar x1, y1, x2, y2;
	xchar rtype, rlit, rirreg;
} region;

/* values for rtype are defined in dungeon.h */
typedef struct {
	struct { xchar x1, y1, x2, y2; } inarea;
	struct { xchar x1, y1, x2, y2; } delarea;
	boolean in_islev, del_islev;
	xchar rtype, padding;
	Str_or_Len rname;
} lev_region;

typedef struct {
	xchar x, y;
	int   amount;
} gold;

typedef struct {
	xchar x, y;
	Str_or_Len engr;
	xchar etype;
} engraving;

typedef struct {
	xchar x, y;
} fountain;

typedef struct {
	xchar x, y;
} sink;

typedef struct {
	xchar x, y;
} pool;

typedef struct {
	char halign, valign;
	char xsize, ysize;
	char **map;
	char nrobjects;
	char *robjects;
	char nloc;
	char *rloc_x;
	char *rloc_y;
	char nrmonst;
	char *rmonst;
	char nreg;
	region **regions;
	char nlreg;
	lev_region **lregions;
	char ndoor;
	door **doors;
	char ntrap;
	trap **traps;
	char nmonster;
	monster **monsters;
	char nobject;
	object **objects;
	char ndrawbridge;
	drawbridge **drawbridges;
	char nwalk;
	walk **walks;
	char ndig;
	digpos **digs;
	char npass;
	digpos **passs;
	char nlad;
	lad **lads;
	char nstair;
	stair **stairs;
	char naltar;
	altar **altars;
	char ngold;
	gold **golds;
	char nengraving;
	engraving **engravings;
	char nfountain;
	fountain **fountains;
} mazepart;

typedef struct {
	long flags;
	lev_init init_lev;
	schar filling;
	char numpart;
	mazepart **parts;
} specialmaze;

typedef struct _room {
	char  *name;
	char  *parent;
	xchar x, y, w, h;
	xchar xalign, yalign;
	xchar rtype, chance, rlit, filled;
	char ndoor;
	room_door **doors;
	char ntrap;
	trap **traps;
	char nmonster;
	monster **monsters;
	char nobject;
	object **objects;
	char naltar;
	altar **altars;
	char nstair;
	stair **stairs;
	char ngold;
	gold **golds;
	char nengraving;
	engraving **engravings;
	char nfountain;
	fountain **fountains;
	char nsink;
	sink **sinks;
	char npool;
	pool **pools;
	/* These three fields are only used when loading the level... */
	int nsubroom;
	struct _room *subrooms[MAX_SUBROOMS];
	struct mkroom *mkr;
} room;

typedef struct {
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} src, dest;
} corridor;

/* used only by lev_comp */
typedef struct {
	long flags;
	lev_init init_lev;
	char nrobjects;
	char *robjects;
	char nrmonst;
	char *rmonst;
	xchar nroom;
	room **rooms;
	xchar ncorr;
	corridor **corrs;
} splev;

#endif /* SP_LEV_H */

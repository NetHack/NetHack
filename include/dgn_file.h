/* NetHack 3.6	dgn_file.h	$NHDT-Date: 1432512780 2015/05/25 00:13:00 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) 1989 by M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DGN_FILE_H
#define DGN_FILE_H

#ifndef ALIGN_H
#include "align.h"
#endif

/*
 * Structures manipulated by the dungeon loader & compiler
 */

struct couple {
    short base, rand;
};

struct tmpdungeon {
    char name[24], protoname[24];
    struct couple lev;
    int flags, chance, levels, branches,
        entry_lev; /* entry level for this dungeon */
    char boneschar;
};

struct tmplevel {
    char name[24];
    struct couple lev;
    int chance, rndlevs, chain, flags;
    char boneschar;
};

struct tmpbranch {
    char name[24]; /* destination dungeon name */
    struct couple lev;
    int chain; /* index into tmplevel array (chained branch)*/
    int type;  /* branch type (see below) */
    int up;    /* branch is up or down */
};

/*
 *	Values for type for tmpbranch structure.
 */
#define TBR_STAIR 0   /* connection with both ends having a staircase */
#define TBR_NO_UP 1   /* connection with no up staircase */
#define TBR_NO_DOWN 2 /* connection with no down staircase */
#define TBR_PORTAL 3  /* portal connection */

/*
 *	Flags that map into the dungeon flags bitfields.
 */
#define TOWN 1 /* levels only */
#define HELLISH 2
#define MAZELIKE 4
#define ROGUELIKE 8

#define D_ALIGN_NONE 0
#define D_ALIGN_CHAOTIC (AM_CHAOTIC << 4)
#define D_ALIGN_NEUTRAL (AM_NEUTRAL << 4)
#define D_ALIGN_LAWFUL (AM_LAWFUL << 4)

#define D_ALIGN_MASK 0x70

/*
 *	Max number of prototype levels and branches.
 */
#define LEV_LIMIT 50
#define BRANCH_LIMIT 32

#endif /* DGN_FILE_H */

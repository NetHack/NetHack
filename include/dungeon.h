/* NetHack 3.7	dungeon.h	$NHDT-Date: 1596498535 2020/08/03 23:48:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.39 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DUNGEON_H
#define DUNGEON_H

typedef struct d_level { /* basic dungeon level element */
    xint16 dnum;          /* dungeon number */
    xint16 dlevel;        /* level number */
} d_level;

#if !defined(MAKEDEFS_C) && !defined(MDLIB_C)

typedef struct d_flags {     /* dungeon/level type flags */
    Bitfield(town, 1);       /* is this a town? (levels only) */
    Bitfield(hellish, 1);    /* is this part of hell? */
    Bitfield(maze_like, 1);  /* is this a maze? */
    Bitfield(rogue_like, 1); /* is this an old-fashioned presentation? */
    Bitfield(align, 3);      /* dungeon alignment. */
    Bitfield(unconnected, 1); /* dungeon not connected to any branch */
} d_flags;

typedef struct s_level { /* special dungeon level element */
    struct s_level *next;
    d_level dlevel; /* dungeon & level numbers */
    char proto[15]; /* name of prototype file (eg. "tower") */
    char boneid;    /* character to id level in bones files */
    uchar rndlevs;  /* no. of randomly available similar levels */
    d_flags flags;  /* type flags */
} s_level;

typedef struct stairway { /* basic stairway identifier */
    coordxy sx, sy;         /* x / y location of the stair */
    d_level tolev;        /* where does it go */
    boolean up;           /* up or down? */
    boolean isladder;     /* ladder or stairway? */
    boolean u_traversed;  /* hero has traversed this stair */
    struct stairway *next;
} stairway;

/* level region types */
enum level_region_types {
    LR_DOWNSTAIR = 0,
    LR_UPSTAIR,
    LR_PORTAL,
    LR_BRANCH,
    LR_TELE,
    LR_UPTELE,
    LR_DOWNTELE
};

typedef struct dest_area { /* non-stairway level change identifier */
    coordxy lx, ly;          /* "lower" left corner (near [0,0]) */
    coordxy hx, hy;          /* "upper" right corner (near [COLNO,ROWNO]) */
    coordxy nlx, nly;        /* outline of invalid area */
    coordxy nhx, nhy;        /* opposite corner of invalid area */
} dest_area;

typedef struct dungeon {   /* basic dungeon identifier */
    char dname[24];        /* name of the dungeon (eg. "Hell") */
    char proto[15];        /* name of prototype file (eg. "tower") */
    char fill_lvl[15];     /* name of "fill" level protype file */
    char themerms[15];     /* lua file name containing themed rooms */
    char boneid;           /* character to id dungeon in bones files */
    d_flags flags;         /* dungeon flags */
    xint16 entry_lev;       /* entry level */
    xint16 num_dunlevs;     /* number of levels in this dungeon */
    xint16 dunlev_ureached; /* how deep you have been in this dungeon */
    int ledger_start,      /* the starting depth in "real" terms */
        depth_start;       /* the starting depth in "logical" terms */
} dungeon;

/*
 * A branch structure defines the connection between two dungeons.  They
 * will be ordered by the dungeon number/level number of 'end1'.  Ties
 * are resolved by 'end2'.  'Type' uses 'end1' arbitrarily as the primary
 * point.
 */
typedef struct branch {
    struct branch *next; /* next in the branch chain */
    int id;              /* branch identifier */
    int type;            /* type of branch */
    d_level end1;        /* "primary" end point */
    d_level end2;        /* other end point */
    boolean end1_up;     /* does end1 go up? */
} branch;

/* branch types */
#define BR_STAIR 0   /* "Regular" connection, 2 staircases. */
#define BR_NO_END1 1 /* "Regular" connection.  However, no stair from
                        end1 to end2.  There is a stair from end2 to end1. */
#define BR_NO_END2 2 /* "Regular" connection.  However, no stair from
                        end2 to end1.  There is a stair from end1 to end2. */
#define BR_PORTAL 3  /* Connection by magic portals (traps) */

/* A particular dungeon contains num_dunlevs d_levels with dlevel 1..
 * num_dunlevs.  Ledger_start and depth_start are bases that are added
 * to the dlevel of a particular d_level to get the effective ledger_no
 * and depth for that d_level.
 *
 * Ledger_no is a bookkeeping number that gives a unique identifier for a
 * particular d_level (for level.?? files, e.g.).
 *
 * Depth corresponds to the number of floors below the surface.
 */

/* These both can't be zero, or dungeon_topology isn't init'd / restored */
#define Lassigned(y) ((y)->dlevel || (y)->dnum)
#define Lcheck(x,z) (Lassigned(z) && on_level(x, z))

#define Is_astralevel(x)    (Lcheck(x, &astral_level))
#define Is_earthlevel(x)    (Lcheck(x, &earth_level))
#define Is_waterlevel(x)    (Lcheck(x, &water_level))
#define Is_firelevel(x)     (Lcheck(x, &fire_level))
#define Is_airlevel(x)      (Lcheck(x, &air_level))
#define Is_medusa_level(x)  (Lcheck(x, &medusa_level))
#define Is_oracle_level(x)  (Lcheck(x, &oracle_level))
#define Is_valley(x)        (Lcheck(x, &valley_level))
#define Is_juiblex_level(x) (Lcheck(x, &juiblex_level))
#define Is_asmo_level(x)    (Lcheck(x, &asmodeus_level))
#define Is_baal_level(x)    (Lcheck(x, &baalzebub_level))
#define Is_wiz1_level(x)    (Lcheck(x, &wiz1_level))
#define Is_wiz2_level(x)    (Lcheck(x, &wiz2_level))
#define Is_wiz3_level(x)    (Lcheck(x, &wiz3_level))
#define Is_sanctum(x)       (Lcheck(x, &sanctum_level))
#define Is_portal_level(x)  (Lcheck(x, &portal_level))
#define Is_rogue_level(x)   (Lcheck(x, &rogue_level))
#define Is_stronghold(x)    (Lcheck(x, &stronghold_level))
#define Is_bigroom(x)       (Lcheck(x, &bigroom_level))
#define Is_qstart(x)        (Lcheck(x, &qstart_level))
#define Is_qlocate(x)       (Lcheck(x, &qlocate_level))
#define Is_nemesis(x)       (Lcheck(x, &nemesis_level))
#define Is_knox(x)          (Lcheck(x, &knox_level))
#define Is_mineend_level(x) (Lcheck(x, &mineend_level))
#define Is_sokoend_level(x) (Lcheck(x, &sokoend_level))

#define In_sokoban(x) ((x)->dnum == sokoban_dnum)
#define Inhell In_hell(&u.uz) /* now gehennom */
#define In_endgame(x) ((x)->dnum == astral_level.dnum)

#define within_bounded_area(X, Y, LX, LY, HX, HY) \
    ((X) >= (LX) && (X) <= (HX) && (Y) >= (LY) && (Y) <= (HY))

/* monster and object migration codes */

#define MIGR_NOWHERE (-1) /* failure flag for down_gate() */
#define MIGR_RANDOM 0
#define MIGR_APPROX_XY 1 /* approximate coordinates */
#define MIGR_EXACT_XY 2  /* specific coordinates */
#define MIGR_STAIRS_UP 3
#define MIGR_STAIRS_DOWN 4
#define MIGR_LADDER_UP 5
#define MIGR_LADDER_DOWN 6
#define MIGR_SSTAIRS 7      /* dungeon branch */
#define MIGR_PORTAL 8       /* magic portal */
#define MIGR_WITH_HERO 9    /* mon: followers; obj: trap door */
#define MIGR_NOBREAK 1024   /* bitmask: don't break on delivery */
#define MIGR_NOSCATTER 2048 /* don't scatter on delivery */
#define MIGR_TO_SPECIES 4096 /* migrating to species as they are made */
#define MIGR_LEFTOVERS 8192  /* grab remaining MIGR_TO_SPECIES objects */
/* level information (saved via ledger number) */

struct linfo {
    unsigned char flags;
#define VISITED 0x01      /* hero has visited this level */
/* 0x02 was FORGOTTEN, when amnesia made you forget maps */
#define LFILE_EXISTS 0x04 /* a level file exists for this level */
        /* Note:  VISITED and LFILE_EXISTS are currently almost always
         * set at the same time.  However they _mean_ different things.
         */
};

/* types and structures for dungeon map recording
 *
 * It is designed to eliminate the need for an external notes file for some
 * mundane dungeon elements.  "Where was the last altar I passed?" etc...
 * Presumably the character can remember this sort of thing even if, months
 * later in real time picking up an old save game, I can't.
 *
 * To be consistent, one can assume that this map is in the player's mind and
 * has no physical correspondence (eliminating illiteracy/blind/hands/hands
 * free concerns).  Therefore, this map is not exhaustive nor detailed ("some
 * fountains").  This makes it also subject to player conditions (amnesia).
 */

/* what the player knows about a single dungeon level */
/* initialized in mklev() */
typedef struct mapseen {
    struct mapseen *next; /* next map in the chain */
    branch *br;           /* knows about branch via taking it in goto_level */
    d_level lev;          /* corresponding dungeon level */
    struct mapseen_feat {
        /* feature knowledge that must be calculated from levl array */
        Bitfield(nfount, 2);
        Bitfield(nsink, 2);
        Bitfield(naltar, 2);
        Bitfield(nthrone, 2);

        Bitfield(ngrave, 2);
        Bitfield(ntree, 2);
        Bitfield(water, 2);
        Bitfield(lava, 2);

        Bitfield(ice, 2);
        /* calculated from rooms array */
        Bitfield(nshop, 2);
        Bitfield(ntemple, 2);
        /* altar alignment; MSA_NONE if there is more than one and
           they aren't all the same */
        Bitfield(msalign, 2);

        Bitfield(shoptype, 5);
    } feat;
    struct mapseen_flags {
        Bitfield(unreachable, 1); /* can't get back to this level */
        Bitfield(forgot, 1);      /* player has forgotten about this level */
        Bitfield(knownbones, 1);  /* player aware of bones */
        Bitfield(oracle, 1);
        Bitfield(sokosolved, 1);
        Bitfield(bigroom, 1);
        Bitfield(castle, 1);
        Bitfield(castletune, 1); /* add tune hint to castle annotation */

        Bitfield(valley, 1);
        Bitfield(msanctum, 1);
        Bitfield(ludios, 1);
        Bitfield(roguelevel, 1);
        /* quest annotations: quest_summons is for main dungeon level
           with entry portal and is reset once quest has been finished;
           questing is for quest home (level 1) */
        Bitfield(quest_summons, 1); /* heard summons from leader */
        Bitfield(questing, 1); /* quest leader has unlocked quest stairs */
        /* "gateway to sanctum" */
        Bitfield(vibrating_square, 1); /* found vibrating square 'trap';
                                        * flag cleared once the msanctum
                                        * annotation has been added (on
                                        * the next dungeon level; temple
                                        * entered or high altar mapped) */
        Bitfield(spare1, 1); /* not used */
    } flags;
    /* custom naming */
    char *custom;
    unsigned custom_lth;
    struct mapseen_rooms {
        Bitfield(seen, 1);
        Bitfield(untended, 1);         /* flag for shop without shk */
    } msrooms[(MAXNROFROOMS + 1) * 2]; /* same size as gr.rooms[] */
    /* dead heroes; might not have graves or ghosts */
    struct cemetery *final_resting_place; /* same as level.bonesinfo */
} mapseen;

#endif /* !MAKEDEFS_C && !MDLIB_C */

#endif /* DUNGEON_H */

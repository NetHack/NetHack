/* NetHack 3.6  decl.h  $NHDT-Date: 1580600478 2020/02/01 23:41:18 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.221 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2007. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DECL_H
#define DECL_H

#define E extern

/* max size of a windowtype option */
#define WINTYPELEN 16

struct dgn_topology { /* special dungeon levels for speed */
    d_level d_oracle_level;
    d_level d_bigroom_level; /* unused */
    d_level d_rogue_level;
    d_level d_medusa_level;
    d_level d_stronghold_level;
    d_level d_valley_level;
    d_level d_wiz1_level;
    d_level d_wiz2_level;
    d_level d_wiz3_level;
    d_level d_juiblex_level;
    d_level d_orcus_level;
    d_level d_baalzebub_level; /* unused */
    d_level d_asmodeus_level;  /* unused */
    d_level d_portal_level;    /* only in goto_level() [do.c] */
    d_level d_sanctum_level;
    d_level d_earth_level;
    d_level d_water_level;
    d_level d_fire_level;
    d_level d_air_level;
    d_level d_astral_level;
    xchar d_tower_dnum;
    xchar d_sokoban_dnum;
    xchar d_mines_dnum, d_quest_dnum;
    d_level d_qstart_level, d_qlocate_level, d_nemesis_level;
    d_level d_knox_level;
    d_level d_mineend_level;
    d_level d_sokoend_level;
};

/* macros for accessing the dungeon levels by their old names */
/* clang-format off */
#define oracle_level            (g.dungeon_topology.d_oracle_level)
#define bigroom_level           (g.dungeon_topology.d_bigroom_level)
#define rogue_level             (g.dungeon_topology.d_rogue_level)
#define medusa_level            (g.dungeon_topology.d_medusa_level)
#define stronghold_level        (g.dungeon_topology.d_stronghold_level)
#define valley_level            (g.dungeon_topology.d_valley_level)
#define wiz1_level              (g.dungeon_topology.d_wiz1_level)
#define wiz2_level              (g.dungeon_topology.d_wiz2_level)
#define wiz3_level              (g.dungeon_topology.d_wiz3_level)
#define juiblex_level           (g.dungeon_topology.d_juiblex_level)
#define orcus_level             (g.dungeon_topology.d_orcus_level)
#define baalzebub_level         (g.dungeon_topology.d_baalzebub_level)
#define asmodeus_level          (g.dungeon_topology.d_asmodeus_level)
#define portal_level            (g.dungeon_topology.d_portal_level)
#define sanctum_level           (g.dungeon_topology.d_sanctum_level)
#define earth_level             (g.dungeon_topology.d_earth_level)
#define water_level             (g.dungeon_topology.d_water_level)
#define fire_level              (g.dungeon_topology.d_fire_level)
#define air_level               (g.dungeon_topology.d_air_level)
#define astral_level            (g.dungeon_topology.d_astral_level)
#define tower_dnum              (g.dungeon_topology.d_tower_dnum)
#define sokoban_dnum            (g.dungeon_topology.d_sokoban_dnum)
#define mines_dnum              (g.dungeon_topology.d_mines_dnum)
#define quest_dnum              (g.dungeon_topology.d_quest_dnum)
#define qstart_level            (g.dungeon_topology.d_qstart_level)
#define qlocate_level           (g.dungeon_topology.d_qlocate_level)
#define nemesis_level           (g.dungeon_topology.d_nemesis_level)
#define knox_level              (g.dungeon_topology.d_knox_level)
#define mineend_level           (g.dungeon_topology.d_mineend_level)
#define sokoend_level           (g.dungeon_topology.d_sokoend_level)
/* clang-format on */

#define xdnstair (g.dnstair.sx)
#define ydnstair (g.dnstair.sy)
#define xupstair (g.upstair.sx)
#define yupstair (g.upstair.sy)

#define xdnladder (g.dnladder.sx)
#define ydnladder (g.dnladder.sy)
#define xupladder (g.upladder.sx)
#define yupladder (g.upladder.sy)

#define dunlev_reached(x) (g.dungeons[(x)->dnum].dunlev_ureached)

#include "quest.h"

E NEARDATA char tune[6];

#define MAXLINFO (MAXDUNGEON * MAXLEVEL)

struct sinfo {
    int gameover;  /* self explanatory? */
    int stopprint; /* inhibit further end of game disclosure */
#ifdef HANGUPHANDLING
    volatile int done_hup; /* SIGHUP or moral equivalent received
                            * -- no more screen output */
    int preserve_locks;    /* don't remove level files prior to exit */
#endif
    int something_worth_saving; /* in case of panic */
    int panicking;              /* `panic' is in progress */
    int exiting;                /* an exit handler is executing */
    int in_moveloop;
    int in_impossible;
    int in_self_recover;
    int in_parseoptions;        /* in parseoptions */
    int config_error_ready;     /* config_error_add is ready, available */
#ifdef PANICLOG
    int in_paniclog;
#endif
    int wizkit_wishing;
};

/* Flags for controlling uptodate */
#define UTD_CHECKSIZES                 0x01
#define UTD_CHECKFIELDCOUNTS           0x02
#define UTD_SKIP_SANITY1               0x04
#define UTD_SKIP_SAVEFILEINFO          0x08

/* NetHack ftypes */
#define NHF_LEVELFILE       1
#define NHF_SAVEFILE        2 
#define NHF_BONESFILE       3
/* modes */
#define READING  0x0
#define COUNTING 0x1
#define WRITING  0x2
#define FREEING  0x4
#define MAX_BMASK 4
/* operations of the various saveXXXchn & co. routines */
#define perform_bwrite(nhfp) ((nhfp)->mode & (COUNTING | WRITING))
#define release_data(nhfp) ((nhfp)->mode & FREEING)

/* Content types for fieldlevel files */
struct fieldlevel_content {
    boolean deflt;        /* individual fields */
    boolean binary;       /* binary rather than text */
    boolean json;         /* JSON */
};
    
typedef struct {
    int fd;               /* for traditional structlevel binary writes */
    int mode;             /* holds READING, WRITING, or FREEING modes  */
    int ftype;            /* NHF_LEVELFILE, NHF_SAVEFILE, or NHF_BONESFILE */
    int fnidx;            /* index of procs for fieldlevel saves */
    long count;           /* holds current line count for default style file,
                             field count for binary style */
    boolean structlevel;  /* traditional structure binary saves */
    boolean fieldlevel;   /* fieldlevel saves saves each field individually */
    boolean addinfo;      /* if set, some additional context info from core */
    boolean eof;          /* place to mark eof reached */
    boolean bendian;      /* set to true if executing on big-endian machine */
    FILE *fpdef;          /* file pointer for fieldlevel default style */
    FILE *fpdefmap;       /* file pointer mapfile for def format */
    FILE *fplog;          /* file pointer logfile */
    FILE *fpdebug;        /* file pointer debug info */
    struct fieldlevel_content style;
} NHFILE;

E const char quitchars[];
E const char vowels[];
E const char ynchars[];
E const char ynqchars[];
E const char ynaqchars[];
E const char ynNaqchars[];
E NEARDATA long yn_number;

E const char disclosure_options[];

struct kinfo {
    struct kinfo *next; /* chain of delayed killers */
    int id;             /* uprop keys to ID a delayed killer */
    int format;         /* one of the killer formats */
#define KILLED_BY_AN 0
#define KILLED_BY 1
#define NO_KILLER_PREFIX 2
    char name[BUFSZ]; /* actual killer name */
};

E const schar xdir[], ydir[], zdir[];

struct multishot {
    int n, i;
    short o;
    boolean s;
};

E NEARDATA boolean has_strong_rngseed;
E const int shield_static[];

#include "spell.h"

#include "color.h"
#ifdef TEXTCOLOR
E const int zapcolors[];
#endif

E const struct class_sym def_oc_syms[MAXOCLASSES]; /* default class symbols */
E uchar oc_syms[MAXOCLASSES];                      /* current class symbols */
E const struct class_sym def_monsyms[MAXMCLASSES]; /* default class symbols */
E uchar monsyms[MAXMCLASSES];                      /* current class symbols */

#include "obj.h"
E NEARDATA struct obj *uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
    *uarmu, /* under-wear, so to speak */
    *uskin, *uamul, *uleft, *uright, *ublindf, *uwep, *uswapwep, *uquiver;

E NEARDATA struct obj *uchain; /* defined only when punished */
E NEARDATA struct obj *uball;

#include "engrave.h"
E struct engr *head_engr;

#include "you.h"
E NEARDATA struct you u;
E NEARDATA time_t ubirthday;
E NEARDATA struct u_realtime urealtime;

#include "onames.h"
#ifndef PM_H /* (pm.h has already been included via youprop.h) */
#include "pm.h"
#endif

struct mvitals {
    uchar born;
    uchar died;
    uchar mvflags;
};

struct c_color_names {
    const char *const c_black, *const c_amber, *const c_golden,
        *const c_light_blue, *const c_red, *const c_green, *const c_silver,
        *const c_blue, *const c_purple, *const c_white, *const c_orange;
};

E NEARDATA const struct c_color_names c_color_names;

#define NH_BLACK c_color_names.c_black
#define NH_AMBER c_color_names.c_amber
#define NH_GOLDEN c_color_names.c_golden
#define NH_LIGHT_BLUE c_color_names.c_light_blue
#define NH_RED c_color_names.c_red
#define NH_GREEN c_color_names.c_green
#define NH_SILVER c_color_names.c_silver
#define NH_BLUE c_color_names.c_blue
#define NH_PURPLE c_color_names.c_purple
#define NH_WHITE c_color_names.c_white
#define NH_ORANGE c_color_names.c_orange

/* The names of the colors used for gems, etc. */
E const char *c_obj_colors[];

struct c_common_strings {
    const char *const c_nothing_happens, *const c_thats_enough_tries,
        *const c_silly_thing_to, *const c_shudder_for_moment,
        *const c_something, *const c_Something, *const c_You_can_move_again,
        *const c_Never_mind, *c_vision_clears, *const c_the_your[2],
        *const c_fakename[2];
};

E const struct c_common_strings c_common_strings;

#define nothing_happens c_common_strings.c_nothing_happens
#define thats_enough_tries c_common_strings.c_thats_enough_tries
#define silly_thing_to c_common_strings.c_silly_thing_to
#define shudder_for_moment c_common_strings.c_shudder_for_moment
#define something c_common_strings.c_something
#define Something c_common_strings.c_Something
#define You_can_move_again c_common_strings.c_You_can_move_again
#define Never_mind c_common_strings.c_Never_mind
#define vision_clears c_common_strings.c_vision_clears
#define the_your c_common_strings.c_the_your
/* fakename[] used occasionally so vtense() won't be fooled by an assigned
   name ending in 's' */
#define fakename c_common_strings.c_fakename

/* material strings */
E const char *materialnm[];

/* empty string that is non-const for parameter use */
E char emptystr[];

/* Monster name articles */
#define ARTICLE_NONE 0
#define ARTICLE_THE 1
#define ARTICLE_A 2
#define ARTICLE_YOUR 3

/* Monster name suppress masks */
#define SUPPRESS_IT 0x01
#define SUPPRESS_INVISIBLE 0x02
#define SUPPRESS_HALLUCINATION 0x04
#define SUPPRESS_SADDLE 0x08
#define EXACT_NAME 0x0F
#define SUPPRESS_NAME 0x10

/* Window system stuff */
E NEARDATA winid WIN_MESSAGE;
E NEARDATA winid WIN_STATUS;
E NEARDATA winid WIN_MAP, WIN_INVEN;

/* pline (et al) for a single string argument (suppress compiler warning) */
#define pline1(cstr) pline("%s", cstr)
#define Your1(cstr) Your("%s", cstr)
#define You1(cstr) You("%s", cstr)
#define verbalize1(cstr) verbalize("%s", cstr)
#define You_hear1(cstr) You_hear("%s", cstr)
#define Sprintf1(buf, cstr) Sprintf(buf, "%s", cstr)
#define panic1(cstr) panic("%s", cstr)

#ifndef TCAP_H
E struct tc_gbl_data {   /* also declared in tcap.h */
    char *tc_AS, *tc_AE; /* graphics start and end (tty font swapping) */
    int tc_LI, tc_CO;    /* lines and columns */
} tc_gbl_data;
#define AS g.tc_gbl_data.tc_AS
#define AE g.tc_gbl_data.tc_AE
#define LI g.tc_gbl_data.tc_LI
#define CO g.tc_gbl_data.tc_CO
#endif

/* xxxexplain[] is in drawing.c */
E const char *const monexplain[], invisexplain[], *const oclass_names[];

/* Some systems want to use full pathnames for some subsets of file names,
 * rather than assuming that they're all in the current directory.  This
 * provides all the subclasses that seem reasonable, and sets up for all
 * prefixes being null.  Port code can set those that it wants.
 */
#define HACKPREFIX	0	/* shared, RO */
#define LEVELPREFIX	1	/* per-user, RW */
#define SAVEPREFIX	2	/* per-user, RW */
#define BONESPREFIX	3	/* shared, RW */
#define DATAPREFIX	4	/* dungeon/dlb; must match value in dlb.c */
#define SCOREPREFIX	5	/* shared, RW */
#define LOCKPREFIX	6	/* shared, RW */
#define SYSCONFPREFIX	7	/* shared, RO */
#define CONFIGPREFIX	8
#define TROUBLEPREFIX	9	/* shared or per-user, RW (append-only) */
#define PREFIX_COUNT	10
/* used in files.c; xxconf.h can override if needed */
#ifndef FQN_MAX_FILENAME
#define FQN_MAX_FILENAME 512
#endif

#if defined(NOCWD_ASSUMPTIONS) || defined(VAR_PLAYGROUND)
/* the bare-bones stuff is unconditional above to simplify coding; for
 * ports that actually use prefixes, add some more localized things
 */
#define PREFIXES_IN_USE
#endif

#ifdef WIN32
E boolean fqn_prefix_locked[PREFIX_COUNT];
#endif
#ifdef PREFIXES_IN_USE
E const char *fqn_prefix_names[PREFIX_COUNT];
#endif

struct restore_info {
	const char *name;
	int mread_flags;
};
E struct restore_info restoreinfo;

E NEARDATA struct savefile_info sfcap, sfrestinfo, sfsaveinfo;

struct selectionvar {
    int wid, hei;
    char *map;
};

struct autopickup_exception {
    struct nhregex *regex;
    char *pattern;
    boolean grab;
    struct autopickup_exception *next;
};

struct plinemsg_type {
    xchar msgtype;  /* one of MSGTYP_foo */
    struct nhregex *regex;
    char *pattern;
    struct plinemsg_type *next;
};

#define MSGTYP_NORMAL   0
#define MSGTYP_NOREP    1
#define MSGTYP_NOSHOW   2
#define MSGTYP_STOP     3
/* bitmask for callers of hide_unhide_msgtypes() */
#define MSGTYP_MASK_REP_SHOW ((1 << MSGTYP_NOREP) | (1 << MSGTYP_NOSHOW))


enum bcargs {override_restriction = -1};
struct breadcrumbs {
    const char *funcnm;
    int linenum;
    boolean in_effect;
};
#ifdef PANICTRACE
E const char *ARGV0;
#endif

enum earlyarg {ARG_DEBUG, ARG_VERSION, ARG_SHOWPATHS
#ifdef WIN32
    ,ARG_WINDOWS
#endif
};

struct early_opt {
    enum earlyarg e;
    const char *name;
    int minlength;
    boolean valallowed;
};

/* special key functions */
enum nh_keyfunc {
    NHKF_ESC = 0,
    NHKF_DOAGAIN,

    NHKF_REQMENU,

    /* run ... clicklook need to be in a continuous block */
    NHKF_RUN,
    NHKF_RUN2,
    NHKF_RUSH,
    NHKF_FIGHT,
    NHKF_FIGHT2,
    NHKF_NOPICKUP,
    NHKF_RUN_NOPICKUP,
    NHKF_DOINV,
    NHKF_TRAVEL,
    NHKF_CLICKLOOK,

    NHKF_REDRAW,
    NHKF_REDRAW2,
    NHKF_GETDIR_SELF,
    NHKF_GETDIR_SELF2,
    NHKF_GETDIR_HELP,
    NHKF_COUNT,
    NHKF_GETPOS_SELF,
    NHKF_GETPOS_PICK,
    NHKF_GETPOS_PICK_Q,  /* quick */
    NHKF_GETPOS_PICK_O,  /* once */
    NHKF_GETPOS_PICK_V,  /* verbose */
    NHKF_GETPOS_SHOWVALID,
    NHKF_GETPOS_AUTODESC,
    NHKF_GETPOS_MON_NEXT,
    NHKF_GETPOS_MON_PREV,
    NHKF_GETPOS_OBJ_NEXT,
    NHKF_GETPOS_OBJ_PREV,
    NHKF_GETPOS_DOOR_NEXT,
    NHKF_GETPOS_DOOR_PREV,
    NHKF_GETPOS_UNEX_NEXT,
    NHKF_GETPOS_UNEX_PREV,
    NHKF_GETPOS_INTERESTING_NEXT,
    NHKF_GETPOS_INTERESTING_PREV,
    NHKF_GETPOS_VALID_NEXT,
    NHKF_GETPOS_VALID_PREV,
    NHKF_GETPOS_HELP,
    NHKF_GETPOS_MENU,
    NHKF_GETPOS_LIMITVIEW,
    NHKF_GETPOS_MOVESKIP,

    NUM_NHKF
};

/* commands[] is used to directly access cmdlist[] instead of looping
   through it to find the entry for a given input character;
   move_X is the character used for moving one step in direction X;
   alphadirchars corresponds to old sdir,
   dirchars corresponds to ``iflags.num_pad ? ndir : sdir'';
   pcHack_compat and phone_layout only matter when num_pad is on,
   swap_yz only matters when it's off */
struct cmd {
    unsigned serialno;     /* incremented after each update */
    boolean num_pad;       /* same as iflags.num_pad except during updates */
    boolean pcHack_compat; /* for numpad:  affects 5, M-5, and M-0 */
    boolean phone_layout;  /* inverted keypad:  1,2,3 above, 7,8,9 below */
    boolean swap_yz;       /* QWERTZ keyboards; use z to move NW, y to zap */
    char move_W, move_NW, move_N, move_NE, move_E, move_SE, move_S, move_SW;
    const char *dirchars;      /* current movement/direction characters */
    const char *alphadirchars; /* same as dirchars if !numpad */
    const struct ext_func_tab *commands[256]; /* indexed by input character */
    char spkeys[NUM_NHKF];
};


#define ENTITIES 2

struct entity {
    struct monst *emon;     /* youmonst for the player */
    struct permonst *edata; /* must be non-zero for record to be valid */
    int ex, ey;
};

/* these probably ought to be generated by makedefs, like LAST_GEM */
#define FIRST_GEM DILITHIUM_CRYSTAL
#define FIRST_AMULET AMULET_OF_ESP
#define LAST_AMULET AMULET_OF_YENDOR

struct valuable_data {
    long count;
    int typ;
};

struct val_list {
    struct valuable_data *list;
    int size;
};

/* at most one of `door' and `box' should be non-null at any given time */
struct xlock_s {
    struct rm *door;
    struct obj *box;
    int picktyp, /* key|pick|card for unlock, sharp vs blunt for #force */
        chance, usedtime;
    boolean magic_key;
};

struct trapinfo {
    struct obj *tobj;
    xchar tx, ty;
    int time_needed;
    boolean force_bungle;
};

typedef struct {
    xchar gnew; /* perhaps move this bit into the rm structure. */
    int glyph;
} gbuf_entry;

enum vanq_order_modes {
    VANQ_MLVL_MNDX = 0,
    VANQ_MSTR_MNDX,
    VANQ_ALPHA_SEP,
    VANQ_ALPHA_MIX,
    VANQ_MCLS_HTOL,
    VANQ_MCLS_LTOH,
    VANQ_COUNT_H_L,
    VANQ_COUNT_L_H,

    NUM_VANQ_ORDER_MODES
};

struct rogueroom {
    xchar rlx, rly;
    xchar dx, dy;
    boolean real;
    uchar doortable;
    int nroom; /* Only meaningful for "real" rooms */
};

typedef struct ls_t {
    struct ls_t *next;
    xchar x, y;  /* source's position */
    short range; /* source's current range */
    short flags;
    short type;  /* type of light source */
    anything id; /* source's identifier */
} light_source;

struct container {
    struct container *next;
    xchar x, y;
    short what;
    genericptr_t list;
};

enum bubble_contains_types {
    CONS_OBJ = 0,
    CONS_MON,
    CONS_HERO,
    CONS_TRAP
};

#define MAX_BMASK 4

struct bubble {
    xchar x, y;   /* coordinates of the upper left corner */
    schar dx, dy; /* the general direction of the bubble's movement */
    uchar bm[MAX_BMASK + 2];    /* bubble bit mask */
    struct bubble *prev, *next; /* need to traverse the list up and down */
    struct container *cons;
};

struct musable {
    struct obj *offensive;
    struct obj *defensive;
    struct obj *misc;
    int has_offense, has_defense, has_misc;
    /* =0, no capability; otherwise, different numbers.
     * If it's an object, the object is also set (it's 0 otherwise).
     */
};

struct h2o_ctx {
    int dkn_boom, unk_boom; /* track dknown, !dknown separately */
    boolean ctx_valid;
};

struct launchplace {
    struct obj *obj;
    xchar x, y;
};

struct repo { /* repossession context */
    struct monst *shopkeeper;
    coord location;
};

/* from options.c */
#define MAX_MENU_MAPPED_CMDS 32 /* some number */

/* player selection constants */
#define BP_ALIGN 0
#define BP_GEND 1
#define BP_RACE 2
#define BP_ROLE 3
#define NUM_BP 4

#define NUM_ROLES (13)
struct role_filter {
    boolean roles[NUM_ROLES+1];
    short mask;
};

/* instance_globals holds engine state that does not need to be
 * persisted upon game exit.  The initialization state is well defined
 * an set in decl.c during early early engine initialization.
 *
 * unlike instance_flags, values in the structure can be of any type. */

#define BSIZE 20
#define WIZKIT_MAX 128
#define CVT_BUF_SIZE 64

#define LUA_VER_BUFSIZ 20
#define LUA_COPYRIGHT_BUFSIZ 120

struct instance_globals {

    /* apply.c */
    int jumping_is_magic; /* current jump result of magic */
    int polearm_range_min;
    int polearm_range_max;
    struct trapinfo trapinfo;

    /* artifcat.c */
    int spec_dbon_applies; /* coordinate effects from spec_dbon() with
                              messages in artifact_hit() */
    /* flags including which artifacts have already been created */
    boolean artiexist[1 + NROFARTIFACTS + 1];
    /* and a discovery list for them (no dummy first entry here) */
    xchar artidisco[NROFARTIFACTS];
    int mkot_trap_warn_count;

    /* botl.c */
    int mrank_sz; /* loaded by max_rank_sz */
    struct istat_s blstats[2][MAXBLSTATS];
    boolean blinit;
    boolean update_all;
    boolean valset[MAXBLSTATS];
#ifdef STATUS_HILITES
    long bl_hilite_moves;
#endif
    unsigned long cond_hilites[BL_ATTCLR_MAX];
    int now_or_before_idx;   /* 0..1 for array[2][] first index */
    int condmenu_sortorder;

    /* cmd.c */
    struct cmd Cmd; /* flag.h */
    /* Provide a means to redo the last command.  The flag `in_doagain' is set
     * to true while redoing the command.  This flag is tested in commands that
     * require additional input (like `throw' which requires a thing and a
     * direction), and the input prompt is not shown.  Also, while in_doagain is
     * TRUE, no keystrokes can be saved into the saveq.
     */
    char pushq[BSIZE];
    char saveq[BSIZE];
    int phead;
    int ptail;
    int shead;
    int stail;
    coord clicklook_cc;
    winid en_win;
    boolean en_via_menu;
    int last_multi;

    /* dbridge.c */
    struct entity occupants[ENTITIES];

    /* decl.c */
    int NDECL((*occupation));
    int NDECL((*afternmv));
    const char *hname; /* name of the game (argv[0] of main) */
    int hackpid; /* current process id */
    char chosen_windowtype[WINTYPELEN];
    int bases[MAXOCLASSES];
    int multi;
    const char *multi_reason;
    int nroom;
    int nsubroom;
    int occtime;
    int warn_obj_cnt; /* count of monsters meeting criteria */
    int x_maze_max;
    int y_maze_max;
    int otg_temp; /* used by object_to_glyph() [otg] */
    int in_doagain;
    stairway dnstair; /* stairs down */
    stairway upstair; /* stairs up */
    stairway dnladder; /* ladder down */
    stairway upladder; /* ladder up */
    int smeq[MAXNROFROOMS + 1];
    int doorindex;
    char *save_cm;
    long done_money;
    long domove_attempting;
    long domove_succeeded;
#define DOMOVE_WALK         0x00000001
#define DOMOVE_RUSH         0x00000002
    const char *nomovemsg;
    char plname[PL_NSIZ]; /* player name */
    char pl_character[PL_CSIZ];
    char pl_race; /* character's race */
    char pl_fruit[PL_FSIZ];
    struct fruit *ffruit;
    char tune[6];
    const char *occtxt; /* defined when occupation != NULL */
    schar tbx;  /* mthrowu: target x */
    schar tby;  /* mthrowu: target y */
    s_level * sp_levchn;
    /* for xname handling of multiple shot missile volleys:
       number of shots, index of current one, validity check, shoot vs throw */
    struct multishot m_shot;
    dungeon dungeons[MAXDUNGEON]; /* ini'ed by init_dungeon() */
    stairway sstairs;
    dest_area updest;
    dest_area dndest;
    coord inv_pos;
    boolean defer_see_monsters;
    boolean in_mklev;
    boolean stoned; /* done to monsters hit by 'c' */
    boolean unweapon;
    boolean mrg_to_wielded; /* weapon picked is merged with wielded one */
    struct plinemsg_type *plinemsg_types;
    char toplines[TBUFSZ];
    struct mkroom *upstairs_room;
    struct mkroom *dnstairs_room;
    struct mkroom *sstairs_room;
    coord bhitpos; /* place where throw or zap hits or stops */
    boolean in_steed_dismounting;
    coord doors[DOORMAX];
    struct menucoloring *menu_colorings;
    schar lastseentyp[COLNO][ROWNO]; /* last seen/touched dungeon typ */
    struct spell spl_book[MAXSPELL + 1];
    struct linfo level_info[MAXLINFO];
    struct trap *ftrap;
    /* some objects need special handling during destruction or placement */
    struct obj *current_wand;  /* wand currently zapped/applied */
    struct obj *thrownobj;     /* object in flight due to throwing */
    struct obj *kickedobj;     /* object in flight due to kicking */
    struct dgn_topology dungeon_topology;
    struct kinfo killer;
    struct mkroom rooms[(MAXNROFROOMS + 1) * 2];
    struct mkroom *subrooms;
    dlevel_t level; /* level map */
    long moves;
    long monstermoves; /* moves and monstermoves diverge when player is Fast */
    long wailmsg;
    struct obj *migrating_objs; /* objects moving to another dungeon level */    
    struct obj *billobjs; /* objects not yet paid for */
#if defined(MICRO) || defined(WIN32)
    char hackdir[PATHLEN]; /* where rumors, help, record are */
#endif /* MICRO || WIN32 */
    struct monst youmonst;
    struct obj *invent;
    struct context_info context;
    char *fqn_prefix[PREFIX_COUNT];
    /* Windowing stuff that's really tty oriented, but present for all ports */
    struct tc_gbl_data tc_gbl_data; /* AS,AE, LI,CO */     
#if defined(UNIX) || defined(VMS)
    int locknum; /* max num of simultaneous users */
#endif
#ifdef DEF_PAGER
    char *catmore; /* default pager */
#endif
#ifdef MICRO
    char levels[PATHLEN]; /* where levels are */
#endif /* MICRO */
#ifdef MFLOPPY
    char permbones[PATHLEN]; /* where permanent copy of bones go */
    int ramdisk = FALSE;     /* whether to copy bones to levels or not */
    int saveprompt = TRUE;
    const char *alllevels = "levels.*";
    const char *allbones = "bones*.*";
#endif
    struct sinfo program_state;

    /* dig.c */

    boolean did_dig_msg;

    /* display.c */
    gbuf_entry gbuf[ROWNO][COLNO];
    char gbuf_start[ROWNO];
    char gbuf_stop[ROWNO];


    /* do.c */
    boolean at_ladder;
    char *dfr_pre_msg;  /* pline() before level change */
    char *dfr_post_msg; /* pline() after level change */
    d_level save_dlevel;

    /* do_name.c */
    struct selectionvar *gloc_filter_map;
    int gloc_filter_floodfill_match_glyph;
    int via_naming;

    /* do_wear.c */
    /* starting equipment gets auto-worn at beginning of new game,
       and we don't want stealth or displacement feedback then */
    boolean initial_don; /* manipulated in set_wear() */

    /* dog.c */
    int petname_used; /* user preferred pet name has been used */
    xchar gtyp;  /* type of dog's current goal */
    xchar gx; /* x position of dog's current goal */
    char  gy; /* y position of dog's current goal */
    char dogname[PL_PSIZ];
    char catname[PL_PSIZ];
    char horsename[PL_PSIZ];
    char preferred_pet; /* '\0', 'c', 'd', 'n' (none) */    
    struct monst *mydogs; /* monsters that went down/up together with @ */
    struct monst *migrating_mons; /* monsters moving to another level */
    struct autopickup_exception *apelist;
    struct mvitals mvitals[NUMMONS];

    /* dokick.c */
    struct rm *maploc;
    struct rm nowhere;
    const char *gate_str;

    /* drawing */
    struct symsetentry symset[NUM_GRAPHICS];
    int currentgraphics;
    nhsym showsyms[SYM_MAX]; /* symbols to be displayed */
    nhsym primary_syms[SYM_MAX];   /* loaded primary symbols          */
    nhsym rogue_syms[SYM_MAX];   /* loaded rogue symbols           */
    nhsym ov_primary_syms[SYM_MAX];   /* loaded primary symbols          */
    nhsym ov_rogue_syms[SYM_MAX];   /* loaded rogue symbols           */
    nhsym warnsyms[WARNCOUNT]; /* the current warning display symbols */

    /* dungeon.c */
    int n_dgns; /* number of dungeons (also used in mklev.c and do.c) */
    branch *branches; /* dungeon branch list */
    mapseen *mapseenchn; /*DUNGEON_OVERVIEW*/

    /* eat.c */
    boolean force_save_hs;
    char *eatmbuf; /* set by cpostfx() */


    /* end.c */
    struct valuable_data gems[LAST_GEM + 1 - FIRST_GEM + 1]; /* +1 for glass */
    struct valuable_data amulets[LAST_AMULET + 1 - FIRST_AMULET];
    struct val_list valuables[3];
    int vanq_sortmode;

    /* extralev.c */
    struct rogueroom r[3][3];

    /* files.c */
    char wizkit[WIZKIT_MAX];
    int lockptr;
    char *config_section_chosen;
    char *config_section_current;
    int nesting;
    int symset_count;             /* for pick-list building only */
    boolean chosen_symset_start;
    boolean chosen_symset_end;
    int symset_which_set;
    /* SAVESIZE, BONESSIZE, LOCKNAMESIZE are defined in "fnamesiz.h" */
    char SAVEF[SAVESIZE]; /* relative path of save file from playground */
#ifdef MICRO
    char SAVEP[SAVESIZE]; /* holds path of directory for save file */
#endif
    char bones[BONESSIZE];
    char lock[LOCKNAMESIZE];

    /* hack.c */
    anything tmp_anything;
    int wc; /* current weight_cap(); valid after call to inv_weight() */

    /* insight.c */

    /* invent.c */
    int lastinvnr;  /* 0 ... 51 (never saved&restored) */
    unsigned sortlootmode; /* set by sortloot() for use by sortloot_cmp(); 
                            * reset by sortloot when done */
    char *invbuf;
    unsigned invbufsiz;
    /* for perm_invent when operating on a partial inventory display, so that
       persistent one doesn't get shrunk during filtering for item selection
       then regrown to full inventory, possibly being resized in the process */
    winid cached_pickinv_win;
    /* query objlist callback: return TRUE if obj type matches "this_type" */
    int this_type;
    /* query objlist callback: return TRUE if obj is at given location */
    coord only;

    /* light.c */
    light_source *light_base;

    /* lock.c */
    struct xlock_s xlock;

    /* makemon.c */

    /* mhitm.c */
    boolean vis;
    boolean far_noise;
    long noisetime;
    struct obj *otmp;
    int dieroll; /* Needed for the special case of monsters wielding vorpal
                  * blades (rare). If we use this a lot it should probably
                  * be a parameter to mdamagem() instead of a global variable.
                  */

    /* mhitu.c */
    int mhitu_dieroll;

    /* mklev.c */
    xchar vault_x;
    xchar vault_y;
    boolean made_branch; /* used only during level creation */

    /* mkmap.c */
    char *new_locations;
    int min_rx; /* rectangle bounds for regions */
    int max_rx;
    int min_ry;
    int max_ry; 
    int n_loc_filled;

    /* mkmaze.c */
    lev_region bughack; /* for preserving the insect legs when wallifying
                         * baalz level */
    boolean was_waterlevel; /* ugh... this shouldn't be needed */
    struct bubble *bbubbles;
    struct bubble *ebubbles;
    struct trap *wportal;
    int xmin, ymin, xmax, ymax; /* level boundaries */
    boolean ransacked;

    /* mon.c */
    boolean vamp_rise_msg;
    boolean disintegested;
    short *animal_list; /* list of PM values for animal monsters */
    int animal_list_count;

    /* mthrowu.c */
    int mesg_given; /* for m_throw()/thitu() 'miss' message */
    struct monst *mtarget;  /* monster being shot by another monster */
    struct monst *marcher; /* monster that is shooting */

    /* muse.c */
    boolean m_using; /* kludge to use mondided instead of killed */
    int trapx;
    int trapy;
    boolean zap_oseen; /* for wands which use mbhitm and are zapped at
                        * players.  We usually want an oseen local to
                        * the function, but this is impossible since the
                        * function mbhitm has to be compatible with the
                        * normal zap routines, and those routines don't
                        * remember who zapped the wand. */
    struct musable m;

    /* nhlan.c */
#ifdef MAX_LAN_USERNAME
    char lusername[MAX_LAN_USERNAME];
    int lusername_size;
#endif

    /* o_init.c */
    short disco[NUM_OBJECTS];

    /* objname.c */
    /* distantname used by distant_name() to pass extra information to
       xname_flags(); it would be much cleaner if this were a parameter,
       but that would require all of the xname() and doname() calls to be
       modified */
    int distantname;

    /* options.c */
    struct symsetentry *symset_list; /* files.c will populate this with
                                        list of available sets */
    /*
        * Allow the user to map incoming characters to various menu commands.
        * The accelerator list must be a valid C string.
        */
    char mapped_menu_cmds[MAX_MENU_MAPPED_CMDS + 1]; /* exported */
    char mapped_menu_op[MAX_MENU_MAPPED_CMDS + 1];
    short n_menu_mapped;
    boolean opt_initial;
    boolean opt_from_file;
    boolean opt_need_redraw; /* for doset() */

    /* pickup.c */
    int oldcap; /* last encumberance */
    /* current_container is set in use_container(), to be used by the
       callback routines in_container() and out_container() from askchain()
       and use_container(). Also used by menu_loot() and container_gone(). */
    struct obj *current_container;
    boolean abort_looting;
    /* Value set by query_objlist() for n_or_more(). */
    long val_for_n_or_more;
    /* list of valid menu classes for query_objlist() and allow_category callback
       (with room for all object classes, 'u'npaid, BUCX, and terminator) */
    char valid_menu_classes[MAXOCLASSES + 1 + 4 + 1];
    boolean class_filter;
    boolean bucx_filter;
    boolean shop_filter;

    /* pline.c */
    unsigned pline_flags;
    char prevmsg[BUFSZ];
#ifdef DUMPLOG
    unsigned saved_pline_index;  /* slot in saved_plines[] to use next */
    char *saved_plines[DUMPLOG_MSG_COUNT];
#endif
    /* work buffer for You(), &c and verbalize() */
    char *you_buf;
    int you_buf_siz;

    /* polyself.c */
    int sex_change_ok; /* controls whether taking on new form or becoming new
                          man can also change sex (ought to be an arg to
                          polymon() and newman() instead) */

    /* potion.c */
    boolean notonhead; /* for long worms */
    int potion_nothing;
    int potion_unkn;

    /* pray.c */
    /* values calculated when prayer starts, and used when completed */
    aligntyp p_aligntyp;
    int p_trouble;
    int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

    /* quest.c */
    struct q_score quest_status;

    /* questpgr.c */
    char cvt_buf[CVT_BUF_SIZE];
    /* used by ldrname() and neminame(), then copied into cvt_buf */
    char nambuf[CVT_BUF_SIZE];

    /* read.c */
    boolean known;

    /* region.c */
    NhRegion **regions;
    int n_regions;
    int max_regions;

    /* restore.c */
    int n_ids_mapped;
    struct bucket *id_map;
    boolean restoring;
    struct fruit *oldfruit;
    long omoves;

    /* rip.c */
    char **rip;

    /* role.c */
    struct Role urole; /* player's role. May be munged in role_init() */
    struct Race urace; /* player's race. May be munged in role_init() */
    char role_pa[NUM_BP];
    char role_post_attribs;
    struct role_filter rfilter;

    /* rumors.c */
    long true_rumor_size; /* rumor size variables are signed so that value -1
                            can be used as a flag */
    long false_rumor_size;
    unsigned long true_rumor_start; /* rumor start offsets are unsigned because
                                       they're handled via %lx format */
    unsigned long false_rumor_start;
    long true_rumor_end; /* rumor end offsets are signed because they're
                            compared with [dlb_]ftell() */
    long false_rumor_end;
    int oracle_flg; /* -1=>don't use, 0=>need init, 1=>init done */
    unsigned oracle_cnt; /* oracles are handled differently from rumors... */
    unsigned long *oracle_loc;

    /* save.c */
    boolean havestate;
    unsigned ustuck_id; /* need to preserve during save */
    unsigned usteed_id; /* need to preserve during save */

    /* shk.c */
    /* auto-response flag for/from "sell foo?" 'a' => 'y', 'q' => 'n' */
    char sell_response;
    int sell_how;
    /* can't just use sell_response='y' for auto_credit because 'a' response
       shouldn't carry over from ordinary selling to credit selling */
    boolean auto_credit;
    struct repo repo;
    long int followmsg; /* last time of follow message */

    /* sp_lev.c */
    char *lev_message;
    lev_region *lregions;
    int num_lregions;
    struct sp_coder *coder;
    xchar xstart, ystart;
    xchar xsize, ysize;

    /* spells.c */
    int spl_sortmode;   /* index into spl_sortchoices[] */
    int *spl_orderindx; /* array of g.spl_book[] indices */

    /* teleport.c */
    struct obj *telescroll; /* non-null when teleporting via this scroll */

    /* timeout.c */
    /* ordered timer list */
    struct fe *timer_base; /* "active" */
    unsigned long timer_id;

    /* topten.c */
    winid toptenwin;
#ifdef UPDATE_RECORD_IN_PLACE
    long final_fpos;
#endif


    /* trap.c */
    int force_mintrap; /* mintrap() should take a flags argument, but for time
                          being we use this */
    /* context for water_damage(), managed by water_damage_chain();
        when more than one stack of potions of acid explode while processing
        a chain of objects, use alternate phrasing after the first message */
    struct h2o_ctx acid_ctx;
    /*
     * The following are used to track launched objects to
     * prevent them from vanishing if you are killed. They
     * will reappear at the launchplace in bones files.
     */
    struct launchplace launchplace;


    /* u_init.c */
    short nocreate;
    short nocreate2;
    short nocreate3;
    short nocreate4;
    /* uhitm.c */
    boolean override_confirmation; /* Used to flag attacks caused by
                                      Stormbringer's maliciousness. */

    /* vision.c */
    char **viz_array; /* used in cansee() and couldsee() macros */
    char *viz_rmin;			/* min could see indices */
    char *viz_rmax;			/* max could see indices */
    boolean vision_full_recalc;

    /* weapon.c */
    struct obj *propellor;

    /* windows.c */
    struct win_choices *last_winchoice;

    /* zap.c */
    int  poly_zapped;
    boolean obj_zapped;

    /* new stuff */
    char lua_ver[LUA_VER_BUFSIZ];
    char lua_copyright[LUA_COPYRIGHT_BUFSIZ];

    /* per-level glyph mapping flags */
    long glyphmap_perlevel_flags;

    unsigned long magic; /* validate that structure layout is preserved */
};

E struct instance_globals g;

struct const_globals {
    const struct obj zeroobj;      /* used to zero out a struct obj */
    const struct monst zeromonst;  /* used to zero out a struct monst */
    const anything zeroany;        /* used to zero out union any */
};

E const struct const_globals cg;

#undef E

#endif /* DECL_H */



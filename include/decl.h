/* NetHack 3.7  decl.h  $NHDT-Date: 1657918080 2022/07/15 20:48:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.303 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2007. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DECL_H
#define DECL_H

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
    xint16 d_tower_dnum;
    xint16 d_sokoban_dnum;
    xint16 d_mines_dnum, d_quest_dnum;
    d_level d_qstart_level, d_qlocate_level, d_nemesis_level;
    d_level d_knox_level;
    d_level d_mineend_level;
    d_level d_sokoend_level;
};

/* macros for accessing the dungeon levels by their old names */
/* clang-format off */
#define oracle_level            (gd.dungeon_topology.d_oracle_level)
#define bigroom_level           (gd.dungeon_topology.d_bigroom_level)
#define rogue_level             (gd.dungeon_topology.d_rogue_level)
#define medusa_level            (gd.dungeon_topology.d_medusa_level)
#define stronghold_level        (gd.dungeon_topology.d_stronghold_level)
#define valley_level            (gd.dungeon_topology.d_valley_level)
#define wiz1_level              (gd.dungeon_topology.d_wiz1_level)
#define wiz2_level              (gd.dungeon_topology.d_wiz2_level)
#define wiz3_level              (gd.dungeon_topology.d_wiz3_level)
#define juiblex_level           (gd.dungeon_topology.d_juiblex_level)
#define orcus_level             (gd.dungeon_topology.d_orcus_level)
#define baalzebub_level         (gd.dungeon_topology.d_baalzebub_level)
#define asmodeus_level          (gd.dungeon_topology.d_asmodeus_level)
#define portal_level            (gd.dungeon_topology.d_portal_level)
#define sanctum_level           (gd.dungeon_topology.d_sanctum_level)
#define earth_level             (gd.dungeon_topology.d_earth_level)
#define water_level             (gd.dungeon_topology.d_water_level)
#define fire_level              (gd.dungeon_topology.d_fire_level)
#define air_level               (gd.dungeon_topology.d_air_level)
#define astral_level            (gd.dungeon_topology.d_astral_level)
#define tower_dnum              (gd.dungeon_topology.d_tower_dnum)
#define sokoban_dnum            (gd.dungeon_topology.d_sokoban_dnum)
#define mines_dnum              (gd.dungeon_topology.d_mines_dnum)
#define quest_dnum              (gd.dungeon_topology.d_quest_dnum)
#define qstart_level            (gd.dungeon_topology.d_qstart_level)
#define qlocate_level           (gd.dungeon_topology.d_qlocate_level)
#define nemesis_level           (gd.dungeon_topology.d_nemesis_level)
#define knox_level              (gd.dungeon_topology.d_knox_level)
#define mineend_level           (gd.dungeon_topology.d_mineend_level)
#define sokoend_level           (gd.dungeon_topology.d_sokoend_level)
/* clang-format on */

#define dunlev_reached(x) (gd.dungeons[(x)->dnum].dunlev_ureached)

#include "quest.h"

extern NEARDATA char tune[6];

#define MAXLINFO (MAXDUNGEON * MAXLEVEL)

/* structure for 'program_state'; not saved and restored */
struct sinfo {
    int gameover;               /* self explanatory? */
    int stopprint;              /* inhibit further end of game disclosure */
#ifdef HANGUPHANDLING
    volatile int done_hup;      /* SIGHUP or moral equivalent received
                                 * -- no more screen output */
    int preserve_locks;         /* don't remove level files prior to exit */
#endif
    int something_worth_saving; /* in case of panic */
    int panicking;              /* `panic' is in progress */
    int exiting;                /* an exit handler is executing */
    int saving;                 /* creating a save file */
    int restoring;              /* reloading a save file */
    int in_moveloop;            /* normal gameplay in progress */
    int in_impossible;          /* reportig a warning */
    int in_docrt;               /* in docrt(): redrawing the whole screen */
    int in_self_recover;        /* processsing orphaned level files */
    int in_checkpoint;          /* saving insurance checkpoint */
    int in_parseoptions;        /* in parseoptions */
    int in_role_selection;      /* role/race/&c selection menus in progress */
    int config_error_ready;     /* config_error_add is ready, available */
    int beyond_savefile_load;   /* set when past savefile loading */
#ifdef PANICLOG
    int in_paniclog;            /* writing a panicloc entry */
#endif
    int wizkit_wishing;         /* starting wizard mode game w/ WIZKIT file */
    /* getting_a_command:  only used for ALTMETA config to process ESC, but
       present and updated unconditionally; set by parse() when requesting
       next command keystroke, reset by readchar() as it returns a key */
    int getting_a_command;      /* next key pressed will be entering a cmnd */
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

extern const char quitchars[];
extern const char vowels[];
extern const char ynchars[];
extern const char ynqchars[];
extern const char ynaqchars[];
extern const char ynNaqchars[];
extern NEARDATA long yn_number;

extern const char disclosure_options[];

struct kinfo {
    struct kinfo *next; /* chain of delayed killers */
    int id;             /* uprop keys to ID a delayed killer */
    int format;         /* one of the killer formats */
#define KILLED_BY_AN 0
#define KILLED_BY 1
#define NO_KILLER_PREFIX 2
    char name[BUFSZ]; /* actual killer name */
};

/* game events log */
struct gamelog_line {
    long turn; /* turn when this happened */
    long flags; /* LL_foo flags */
    char *text;
    struct gamelog_line *next;
};

enum movemodes {
    MV_ANY = -1,
    MV_WALK,
    MV_RUN,
    MV_RUSH,

    N_MOVEMODES
};

enum movementdirs {
    DIR_ERR = -1,
    DIR_W,
    DIR_NW,
    DIR_N,
    DIR_NE,
    DIR_E,
    DIR_SE,
    DIR_S,
    DIR_SW,
    DIR_DOWN,
    DIR_UP,

    N_DIRS_Z
};
/* N_DIRS_Z, minus up & down */
#define N_DIRS (N_DIRS_Z - 2)
/* direction adjustments */
#define DIR_180(dir) (((dir) + 4) % N_DIRS)
#define DIR_LEFT(dir) (((dir) + 7) % N_DIRS)
#define DIR_RIGHT(dir) (((dir) + 1) % N_DIRS)
#define DIR_LEFT2(dir) (((dir) + 6) % N_DIRS)
#define DIR_RIGHT2(dir) (((dir) + 2) % N_DIRS)
#define DIR_CLAMP(dir) (((dir) + N_DIRS) % N_DIRS)

extern const schar xdir[], ydir[], zdir[], dirs_ord[];

struct multishot {
    int n, i;
    short o;
    boolean s;
};

extern NEARDATA boolean has_strong_rngseed;
extern const int shield_static[];

#include "spell.h"

/* default object class symbols */
extern const struct class_sym def_oc_syms[MAXOCLASSES];
/* current object class symbols */
extern uchar oc_syms[MAXOCLASSES];

/* default mon class symbols */
extern const struct class_sym def_monsyms[MAXMCLASSES];
/* current mon class symbols */
extern uchar monsyms[MAXMCLASSES];

#include "obj.h"
extern NEARDATA struct obj *uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
    *uarmu, /* under-wear, so to speak */
    *uskin, *uamul, *uleft, *uright, *ublindf, *uwep, *uswapwep, *uquiver;

extern NEARDATA struct obj *uchain; /* defined only when punished */
extern NEARDATA struct obj *uball;

#include "engrave.h"
extern struct engr *head_engr;

#include "you.h"
extern NEARDATA struct you u;
extern NEARDATA time_t ubirthday;
extern NEARDATA struct u_realtime urealtime;

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

extern NEARDATA const struct c_color_names c_color_names;

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
extern const char *c_obj_colors[];

struct c_common_strings {
    const char *const c_nothing_happens, *const c_thats_enough_tries,
        *const c_silly_thing_to, *const c_shudder_for_moment,
        *const c_something, *const c_Something, *const c_You_can_move_again,
        *const c_Never_mind, *c_vision_clears, *const c_the_your[2],
        *const c_fakename[2];
};

extern const struct c_common_strings c_common_strings;

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
extern const char *materialnm[];

/* empty string that is non-const for parameter use */
extern char emptystr[];

/* Monster name articles */
#define ARTICLE_NONE 0
#define ARTICLE_THE 1
#define ARTICLE_A 2
#define ARTICLE_YOUR 3

/* x_monnam() monster name suppress masks */
#define SUPPRESS_IT 0x01
#define SUPPRESS_INVISIBLE 0x02
#define SUPPRESS_HALLUCINATION 0x04
#define SUPPRESS_SADDLE 0x08
#define EXACT_NAME 0x0F
#define SUPPRESS_NAME 0x10
#define AUGMENT_IT 0x20 /* use "someone" or "something" instead of "it" */

/* Window system stuff */
extern NEARDATA winid WIN_MESSAGE;
extern NEARDATA winid WIN_STATUS;
extern NEARDATA winid WIN_MAP, WIN_INVEN;

/* pline (et al) for a single string argument (suppress compiler warning) */
#define pline1(cstr) pline("%s", cstr)
#define Your1(cstr) Your("%s", cstr)
#define You1(cstr) You("%s", cstr)
#define verbalize1(cstr) verbalize("%s", cstr)
#define You_hear1(cstr) You_hear("%s", cstr)
#define Sprintf1(buf, cstr) Sprintf(buf, "%s", cstr)
#define panic1(cstr) panic("%s", cstr)

#ifndef TCAP_H
extern struct tc_gbl_data {   /* also declared in tcap.h */
    char *tc_AS, *tc_AE; /* graphics start and end (tty font swapping) */
    int tc_LI, tc_CO;    /* lines and columns */
} tc_gbl_data;
#define AS gt.tc_gbl_data.tc_AS
#define AE gt.tc_gbl_data.tc_AE
#define LI gt.tc_gbl_data.tc_LI
#define CO gt.tc_gbl_data.tc_CO
#endif

/* Some systems want to use full pathnames for some subsets of file names,
 * rather than assuming that they're all in the current directory.  This
 * provides all the subclasses that seem reasonable, and sets up for all
 * prefixes being null.  Port code can set those that it wants.
 */
#define HACKPREFIX      0  /* shared, RO */
#define LEVELPREFIX     1  /* per-user, RW */
#define SAVEPREFIX      2  /* per-user, RW */
#define BONESPREFIX     3  /* shared, RW */
#define DATAPREFIX      4  /* dungeon/dlb; must match value in dlb.c */
#define SCOREPREFIX     5  /* shared, RW */
#define LOCKPREFIX      6  /* shared, RW */
#define SYSCONFPREFIX   7  /* shared, RO */
#define CONFIGPREFIX    8
#define TROUBLEPREFIX   9  /* shared or per-user, RW (append-only) */
#define PREFIX_COUNT   10
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
extern boolean fqn_prefix_locked[PREFIX_COUNT];
#endif
#ifdef PREFIXES_IN_USE
extern const char *fqn_prefix_names[PREFIX_COUNT];
#endif

struct restore_info {
    const char *name;
    int mread_flags;
};
extern struct restore_info restoreinfo;

extern NEARDATA struct savefile_info sfcap, sfrestinfo, sfsaveinfo;

struct selectionvar {
    int wid, hei;
    boolean bounds_dirty;
    NhRect bounds; /* use selection_getbounds() */
    char *map;
};

struct autopickup_exception {
    struct nhregex *regex;
    char *pattern;
    boolean grab;
    struct autopickup_exception *next;
};

struct plinemsg_type {
    xint16 msgtype;  /* one of MSGTYP_foo */
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
extern const char *ARGV0;
#endif

enum earlyarg {
    ARG_DEBUG, ARG_VERSION, ARG_SHOWPATHS
#ifndef NODUMPENUMS
    , ARG_DUMPENUMS
#endif
#ifdef ENHANCED_SYMBOLS
    , ARG_DUMPGLYPHIDS
#endif
#ifdef WIN32
    , ARG_WINDOWS
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

    NHKF_GETDIR_SELF,
    NHKF_GETDIR_SELF2,
    NHKF_GETDIR_HELP,
    NHKF_GETDIR_MOUSE,   /* simulated click for #therecmdmenu; use '_' as
                          * direction to initiate, then getpos() finishing
                          * with ',' (left click) or '.' (right click) */
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
    const char *dirchars;      /* current movement/direction characters */
    const char *alphadirchars; /* same as dirchars if !numpad */
    const struct ext_func_tab *commands[256]; /* indexed by input character */
    const struct ext_func_tab *mousebtn[NUM_MOUSE_BUTTONS];
    char spkeys[NUM_NHKF];
    char extcmd_char;      /* key that starts an extended command ('#') */
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
    coordxy tx, ty;
    int time_needed;
    boolean force_bungle;
};

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
    coordxy rlx, rly;
    coordxy dx, dy;
    boolean real;
    uchar doortable;
    int nroom; /* Only meaningful for "real" rooms */
};

typedef struct ls_t {
    struct ls_t *next;
    coordxy x, y;  /* source's position */
    short range; /* source's current range */
    short flags;
    short type;  /* type of light source */
    anything id; /* source's identifier */
} light_source;

struct container {
    struct container *next;
    coordxy x, y;
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
    coordxy x, y;   /* coordinates of the upper left corner */
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
    coordxy x, y;
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

/* read.c, create_particular() & create_particular_parse() */
struct _create_particular_data {
    int quan;
    int which;
    int fem;        /* -1, MALE, FEMALE, NEUTRAL */
    int genderconf;    /* conflicting gender */
    char monclass;
    boolean randmonst;
    boolean maketame, makepeaceful, makehostile;
    boolean sleeping, saddled, invisible, hidden;
};

/* some array sizes for 'g' */
#define WIZKIT_MAX 128
#define CVT_BUF_SIZE 64

#define LUA_VER_BUFSIZ 20
#define LUA_COPYRIGHT_BUFSIZ 120

/*
 * Rudimentary command queue.
 * Allows the code to put keys and extended commands into the queue,
 * and they're executed just as if the user did them.  Time passes
 * normally when doing queued actions.  The queue will get cleared
 * if hero is interrupted.
 */
enum cmdq_cmdtypes {
    CMDQ_KEY = 0, /* a literal character, cmdq_add_key() */
    CMDQ_EXTCMD,  /* extended command, cmdq_add_ec() */
    CMDQ_DIR,     /* direction, cmdq_add_dir() */
    CMDQ_USER_INPUT, /* placeholder for user input, cmdq_add_userinput() */
    CMDQ_INT,     /* integer value, cmdq_add_int() */
};

struct _cmd_queue {
    int typ;
    char key;
    schar dirx, diry, dirz;
    int intval;
    const struct ext_func_tab *ec_entry;
    struct _cmd_queue *next;
};

struct enum_dump {
    int val;
    const char *nm;
};

typedef long cmdcount_nht;    /* Command counts */

enum {
    CQ_CANNED = 0, /* internal canned sequence */
    CQ_REPEAT,     /* user-inputted, if gi.in_doagain, replayed */
    NUM_CQS
};

/*
 * 'gX' -- instance_globals holds engine state that does not need to be
 * persisted upon game exit.  The initialization state is well defined
 * and set in decl.c during early early engine initialization.
 *
 * Unlike instance_flags, values in the structure can be of any type.
 *
 * Pulled from other files to be grouped in one place.  Some comments
 * which came with them don't make much sense out of their original context.
 */

struct instance_globals_a {
    /* decl.c */
    int (*afternmv)(void);

    /* detect.c */
    int already_found_flag; /* used to augment first "already found a monster"
                             * message if 'cmdassist' is Off */
    /* do.c */
    boolean at_ladder;

    /* dog.c */
    struct autopickup_exception *apelist;

    /* end.c */
    struct valuable_data amulets[LAST_AMULET + 1 - FIRST_AMULET];

    /* mon.c */
    short *animal_list; /* list of PM values for animal monsters */
    int animal_list_count;

    /* pickup.c */
    boolean abort_looting;

    /* shk.c */
    boolean auto_credit;

    /* trap.c */
    /* context for water_damage(), managed by water_damage_chain();
        when more than one stack of potions of acid explode while processing
        a chain of objects, use alternate phrasing after the first message */
    struct h2o_ctx acid_ctx;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_b {

    /* botl.c */
    struct istat_s blstats[2][MAXBLSTATS];
    boolean blinit;
#ifdef STATUS_HILITES
    long bl_hilite_moves;
#endif

    /* decl.c */
    int bases[MAXOCLASSES + 1];
    coord bhitpos; /* place where throw or zap hits or stops */
    struct obj *billobjs; /* objects not yet paid for */

    /* dungeon.c */
    branch *branches; /* dungeon branch list */

    /* files.c */
    char bones[BONESSIZE];

    /* hack.c */
    unsigned bldrpush_oid; /* id of last boulder pushed */
    long bldrpushtime;     /* turn that a message was given for pushing
                            * a boulder; used in lieu of Norep() */

    /* mkmaze.c */
    lev_region bughack; /* for preserving the insect legs when wallifying
                         * baalz level */
    struct bubble *bbubbles;

    /* pickup.c */
    boolean bucx_filter;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_c {

    struct _cmd_queue *command_queue[NUM_CQS];

    /* botl.c */
    unsigned long cond_hilites[BL_ATTCLR_MAX];
    int condmenu_sortorder;

    /* cmd.c */
    struct cmd Cmd; /* flag.h */
    /* Provide a means to redo the last command.  The flag `in_doagain'
       (decl.c below) is set to true while redoing the command.  This flag
       is tested in commands that require additional input (like `throw'
       which requires a thing and a direction), and the input prompt is
       not shown.  Also, while in_doagain is TRUE, no keystrokes can be
       saved into the saveq. */
    coord clicklook_cc;
    /* decl.c */
    char chosen_windowtype[WINTYPELEN];
    char command_line[COLNO];
    cmdcount_nht command_count;
    /* some objects need special handling during destruction or placement */
    struct obj *current_wand;  /* wand currently zapped/applied */
#ifdef DEF_PAGER
    const char *catmore; /* external pager; from getenv() or DEF_PAGER */
#endif
    struct context_info context;

    /* dog.c */
    char catname[PL_PSIZ];

    /* symbols.c */
    int currentgraphics;

    /* files.c */
    char *cmdline_rcfile;  /* set in unixmain.c, used in options.c */
    char *config_section_chosen;
    char *config_section_current;
    boolean chosen_symset_start;
    boolean chosen_symset_end;
 
    /* invent.c */
    /* for perm_invent when operating on a partial inventory display, so that
       persistent one doesn't get shrunk during filtering for item selection
       then regrown to full inventory, possibly being resized in the process */
    winid cached_pickinv_win;
    int core_invent_state;

    /* options.c */
    char *cmdline_windowsys; /* set in unixmain.c */
    struct menucoloring *color_colorings; /* alternate set of menu colors */

    /* pickup.c */
    /* current_container is set in use_container(), to be used by the
       callback routines in_container() and out_container() from askchain()
       and use_container().  Also used by menu_loot() and container_gone(). */
    struct obj *current_container;
    boolean class_filter;

    /* questpgr.c */
    char cvt_buf[CVT_BUF_SIZE];

    /* sp_lev.c */
    struct sp_coder *coder;

    /* uhitm.c */
    short corpsenm_digested; /* monster type being digested, set by gulpum */

    /* zap.c */
    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_d {

    /* decl.c */
    int doorindex;
    long done_money;
    long domove_attempting;
    long domove_succeeded;
#define DOMOVE_WALK         0x00000001
#define DOMOVE_RUSH         0x00000002
    dungeon dungeons[MAXDUNGEON]; /* ini'ed by init_dungeon() */
    dest_area dndest;
    boolean defer_see_monsters;
    struct dgn_topology dungeon_topology;
    int doors_alloc; /* doors-array allocated size */
    coord *doors; /* array of door locations */

    /* dig.c */
    boolean did_dig_msg;

    /* do.c */
    char *dfr_pre_msg;  /* pline() before level change */
    char *dfr_post_msg; /* pline() after level change */
    int did_nothing_flag; /* to augment the no-rest-next-to-monster message */

    /* dog.c */
    char dogname[PL_PSIZ];

    /* mon.c */
    boolean disintegested;

    /* o_init.c */
    short disco[NUM_OBJECTS];

    /* objname.c */
    /* distantname used by distant_name() to pass extra information to
       xname_flags(); it would be much cleaner if this were a parameter,
       but that would require all xname() and doname() calls to be modified */
    int distantname;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_e {

    /* cmd.c */
    winid en_win;
    boolean en_via_menu;
    struct ext_func_tab *ext_tlist; /* info for rhack() from doextcmd() */

    /* eat.c */
    char *eatmbuf; /* set by cpostfx() */

    /* mkmaze.c */
    struct bubble *ebubbles;

    /* new stuff */
    int early_raw_messages;   /* if raw_prints occurred early prior
                                 to gb.beyond_savefile_load */

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_f {

    /* decl.c */
    struct trap *ftrap;
    char *fqn_prefix[PREFIX_COUNT];
    struct fruit *ffruit;

    /* eat.c */
    boolean force_save_hs;

    /* mhitm.c */
    boolean far_noise;

    /* rumors.c */
    long false_rumor_size;
    unsigned long false_rumor_start;
    long false_rumor_end;

    /* shk.c */
    long int followmsg; /* last time of follow message */

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_g {

    /* display.c */
    gbuf_entry gbuf[ROWNO][COLNO];
    coordxy gbuf_start[ROWNO];
    coordxy gbuf_stop[ROWNO];

    /* do_name.c */
    struct selectionvar *gloc_filter_map;
    int gloc_filter_floodfill_match_glyph;

    /* dog.c */
    xint16 gtyp;  /* type of dog's current goal */
    coordxy gx; /* x position of dog's current goal */
    coordxy gy; /* y position of dog's current goal */

    /* dokick.c */
    const char *gate_str;

    /* end.c */
    struct valuable_data gems[LAST_GEM + 1 - FIRST_GEM + 1]; /* +1 for glass */

    /* invent.c */
    long glyph_reset_timestamp;

    /* pline.c */
    struct gamelog_line *gamelog;

    /* region.c */
    boolean gas_cloud_diss_within;
    int gas_cloud_diss_seen;

    /* new stuff */
    /* per-level glyph mapping flags */
    long glyphmap_perlevel_flags;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_h {

    /* decl.c */
    const char *hname; /* name of the game (argv[0] of main) */
    int hackpid; /* current process id */
#if defined(MICRO) || defined(WIN32)
    char hackdir[PATHLEN]; /* where rumors, help, record are */
#endif /* MICRO || WIN32 */
    long hero_seq; /* 'moves*8 + n' where n is updated each hero move during
                    * the current turn */

    /* dog.c */
    char horsename[PL_PSIZ];

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_i {

    /* decl.c */
    int in_doagain;
    coord inv_pos;
    boolean in_mklev;
    boolean in_steed_dismounting;
    struct obj *invent;

    /* do_wear.c */
    /* starting equipment gets auto-worn at beginning of new game,
       and we don't want stealth or displacement feedback then */
    boolean initial_don; /* manipulated in set_wear() */

    /* invent.c */
    char *invbuf;
    unsigned invbufsiz;
    int in_sync_perminvent;

    /* restore.c */
    struct bucket *id_map;

    /* sp_lev.c */
    boolean in_mk_themerooms;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_j {

    /* apply.c */
    int jumping_is_magic; /* current jump result of magic */

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_k {

    /* decl.c */
    struct obj *kickedobj;     /* object in flight due to kicking */
    struct kinfo killer;

    /* read.c */
    boolean known;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_l {

    /* cmd.c */
    cmdcount_nht last_command_count;

    /* dbridge.c */
    schar lastseentyp[COLNO][ROWNO]; /* last seen/touched dungeon typ */
    struct linfo level_info[MAXLINFO];
    dlevel_t level; /* level map */
#if defined(UNIX) || defined(VMS)
    int locknum; /* max num of simultaneous users */
#endif
#ifdef MICRO
    char levels[PATHLEN]; /* where levels are */
#endif /* MICRO */

    /* files.c */
    int lockptr;
    char lock[LOCKNAMESIZE];

    /* invent.c */
    int lastinvnr;  /* 0 ... 51 (never saved&restored) */

    /* light.c */
    light_source *light_base;

    /* mklev.c */
    genericptr_t luathemes[MAXDUNGEON];

    /* nhlan.c */
#ifdef MAX_LAN_USERNAME
    char lusername[MAX_LAN_USERNAME];
    int lusername_size;
#endif

    /* nhlua.c */
    genericptr_t luacore; /* lua_State * */
    char lua_warnbuf[BUFSZ];

    /* options.c */
    boolean loot_reset_justpicked;

    /* save.c */
    struct obj *looseball;  /* track uball during save and... */
    struct obj *loosechain; /* track uchain since saving might free it */

    /* sp_lev.c */
    char *lev_message;
    lev_region *lregions;

    /* trap.c */
    struct launchplace launchplace;

    /* windows.c */
    struct win_choices *last_winchoice;

    /* new stuff */
    char lua_ver[LUA_VER_BUFSIZ];
    char lua_copyright[LUA_COPYRIGHT_BUFSIZ];

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_m {

    /* apply.c */
    int mkot_trap_warn_count;

    /* botl.c */
    int mrank_sz; /* loaded by max_rank_sz */

    /* decl.c */
    cmdcount_nht multi;
    const char *multi_reason;
    char multireasonbuf[QBUFSZ]; /* note: smaller than usual [BUFSZ] */
    /* for xname handling of multiple shot missile volleys:
       number of shots, index of current one, validity check, shoot vs throw */
    struct multishot m_shot;
    boolean mrg_to_wielded; /* weapon picked is merged with wielded one */
    struct menucoloring *menu_colorings;
    long moves; /* turn counter */
    struct obj *migrating_objs; /* objects moving to another dungeon level */

    /* dog.c */
    struct monst *mydogs; /* monsters that went down/up together with @ */
    struct monst *migrating_mons; /* monsters moving to another level */
    struct mvitals mvitals[NUMMONS];

    /* dokick.c */
    struct rm *maploc;

    /* dungeon.c */
    mapseen *mapseenchn; /*DUNGEON_OVERVIEW*/

    /* mhitu.c */
    int mhitu_dieroll;

    /* mklev.c */
    boolean made_branch; /* used only during level creation */

    /* mkmap.c */
    int min_rx; /* rectangle bounds for regions */
    int max_rx;
    int min_ry;
    int max_ry;

    /* mkobj.c */
    boolean mkcorpstat_norevive; /* for trolls */

    /* mthrowu.c */
    int mesg_given; /* for m_throw()/thitu() 'miss' message */
    struct monst *mtarget;  /* monster being shot by another monster */
    struct monst *marcher; /* monster that is shooting */

    /* muse.c */
    boolean m_using; /* kludge to use mondided instead of killed */
    struct musable m;

    /* options.c */
    /* Allow the user to map incoming characters to various menu commands. */
    char mapped_menu_cmds[MAX_MENU_MAPPED_CMDS + 1]; /* exported */
    char mapped_menu_op[MAX_MENU_MAPPED_CMDS + 1];

    /* region.c */
    int max_regions;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_n {

    /* botl.c */
    int now_or_before_idx;   /* 0..1 for array[2][] first index */

    /* decl.c */
    const char *nomovemsg;
    int nroom;
    int nsubroom;

    /* dokick.c */
    struct rm nowhere;

    /* dungeon.c */
    int n_dgns; /* number of dungeons (also used in mklev.c and do.c) */

    /* files.c */
    int nesting;
    int no_sound_notified; /* run-time option processing: warn once if built
                            * without USER_SOUNDS and config file contains
                            * SOUND=foo or SOUNDDIR=bar */

    /* mhitm.c */
    long noisetime;

    /* mkmap.c */
    char *new_locations;
    int n_loc_filled;

    /* options.c */
    short n_menu_mapped;

    /* potion.c */
    boolean notonhead; /* for long worms */

    /* questpgr.c */
    char nambuf[CVT_BUF_SIZE];

    /* region.c */
    int n_regions;

    /* restore.c */
    int n_ids_mapped;

    /* sp_lev.c */
    int num_lregions;

    /* u_init.c */
    short nocreate;
    short nocreate2;
    short nocreate3;
    short nocreate4;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_o {

    /* dbridge.c */
    struct entity occupants[ENTITIES];

    /* decl.c */
    int (*occupation)(void);
    int occtime;
    int otg_temp; /* used by object_to_glyph() [otg] */
    const char *occtxt; /* defined when occupation != NULL */

    /* symbols.c */
    nhsym ov_primary_syms[SYM_MAX];   /* loaded primary symbols          */
    nhsym ov_rogue_syms[SYM_MAX];   /* loaded rogue symbols           */

    /* invent.c */
    /* query objlist callback: return TRUE if obj is at given location */
    coord only;

    /* o_init.c */
    short oclass_prob_totals[MAXOCLASSES];

    /* options.c */

    /* options processing */
    boolean opt_initial;
    boolean opt_from_file;
    boolean opt_need_redraw; /* for doset() */
    boolean opt_need_glyph_reset;

    /* pickup.c */
    int oldcap; /* last encumberance */

    /* restore.c */
    struct fruit *oldfruit;
    long omoves;

    /* rumors.c */
    int oracle_flg; /* -1=>don't use, 0=>need init, 1=>init done */
    unsigned oracle_cnt; /* oracles are handled differently from rumors... */
    unsigned long *oracle_loc;

    /* uhitm.c */
    boolean override_confirmation; /* Used to flag attacks caused by
                                    * Stormbringer's maliciousness. */
    /* zap.c */
    boolean obj_zapped;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_p {

    /* apply.c */
    int polearm_range_min;
    int polearm_range_max;

    /* decl.c */
    char plname[PL_NSIZ]; /* player name */
    int plnamelen; /* length of plname[] if that came from getlogin() */
    char pl_character[PL_CSIZ];
    char pl_race; /* character's race */
    char pl_fruit[PL_FSIZ];
    struct plinemsg_type *plinemsg_types;
    struct sinfo program_state; /* flags describing game's current state */

    /* dog.c */
    int petname_used; /* user preferred pet name has been used */
    char preferred_pet; /* '\0', 'c', 'd', 'n' (none) */

    /* symbols.c */
    nhsym primary_syms[SYM_MAX];   /* loaded primary symbols          */

    /* invent.c */
    int perm_invent_toggling_direction;

    /* pickup.c */
    boolean picked_filter;

    /* pline.c */
    unsigned pline_flags;
    char prevmsg[BUFSZ];

    /* potion.c */
    int potion_nothing;
    int potion_unkn;

    /* pray.c */
    /* values calculated when prayer starts, and used when completed */
    aligntyp p_aligntyp;
    int p_trouble;
    int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

    /* weapon.c */
    struct obj *propellor;

    /* zap.c */
    int  poly_zapped;

    /* new stuff */
    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_q {

    /* quest.c */
    struct q_score quest_status;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_r {

    /* decl.c */
    struct mkroom rooms[(MAXNROFROOMS + 1) * 2];

    /* symbols.c */
    nhsym rogue_syms[SYM_MAX];   /* loaded rogue symbols           */

    /* extralev.c */
    struct rogueroom r[3][3];

    /* mkmaze.c */
    boolean ransacked;

    /* region.c */
    NhRegion **regions;

    /* rip.c */
    char **rip;

    /* role.c */
    char role_pa[NUM_BP];
    char role_post_attribs;
    struct role_filter rfilter;

    /* shk.c */
    struct repo repo;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_s {

    /* artifact.c */
    int spec_dbon_applies; /* coordinate effects from spec_dbon() with
                              messages in artifact_hit() */

    /* decl.c */
    s_level * sp_levchn;
    stairway *stairs;
    int smeq[MAXNROFROOMS + 1];
    boolean stoned; /* done to monsters hit by 'c' */
    struct spell spl_book[MAXSPELL + 1];
    struct mkroom *subrooms;

    /* do.c */
    d_level save_dlevel; /* ? [even back in 3.4.3, only used in bones.c] */

    /* symbols.c */
    struct symsetentry symset[NUM_GRAPHICS];
#ifdef ENHANCED_SYMBOLS
    struct symset_customization sym_customizations[NUM_GRAPHICS + 1]; /* adds UNICODESET */
#endif
    nhsym showsyms[SYM_MAX]; /* symbols to be displayed */

    /* files.c */
    int symset_count;             /* for pick-list building only */
    int symset_which_set;
    /* SAVESIZE, BONESSIZE, LOCKNAMESIZE are defined in "fnamesiz.h" */
    char SAVEF[SAVESIZE]; /* relative path of save file from playground */
#ifdef MICRO
    char SAVEP[SAVESIZE]; /* holds path of directory for save file */
#endif

    /* invent.c */
    unsigned sortlootmode; /* set by sortloot() for use by sortloot_cmp();
                            * reset by sortloot when done */
    /* mhitm.c */
    boolean skipdrin; /* mind flayer against headless target */

    /* mon.c */
    boolean somebody_can_move;

    /* options.c */
    struct symsetentry *symset_list; /* files.c will populate this with
                                      * list of available sets */
    boolean save_menucolors; /* copy of iflags.use_menu_colors */
    struct menucoloring *save_colorings; /* copy of gm.menu_colorings */

    /* pickup.c */
    boolean shop_filter;

    /* pline.c */
#ifdef DUMPLOG
    unsigned saved_pline_index;  /* slot in saved_plines[] to use next */
    char *saved_plines[DUMPLOG_MSG_COUNT];
#endif

    /* polyself.c */
    int sex_change_ok; /* controls whether taking on new form or becoming new
                          man can also change sex (ought to be an arg to
                          polymon() and newman() instead) */

    /* shk.c */
    /* auto-response flag for/from "sell foo?" 'a' => 'y', 'q' => 'n' */
    char sell_response;
    int sell_how;

    /* spells.c */
    int spl_sortmode;   /* index into spl_sortchoices[] */
    int *spl_orderindx; /* array of gs.spl_book[] indices */

    /* steal.c */
    unsigned int stealoid; /* object to be stolen */
    unsigned int stealmid; /* monster doing the stealing */

    /* vision.c */
    int seethru; /* 'bubble' debugging: clouds and water don't block light */

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_t {

    /* apply.c */
    struct trapinfo trapinfo;

    /* decl.c */
    char tune[6];
    schar tbx;  /* mthrowu: target x */
    schar tby;  /* mthrowu: target y */
    char toplines[TBUFSZ];
    struct obj *thrownobj;     /* object in flight due to throwing */
    /* Windowing stuff that's really tty oriented, but present for all ports */
    struct tc_gbl_data tc_gbl_data; /* AS,AE, LI,CO */

    /* hack.c */
    anything tmp_anything;
    struct selectionvar *travelmap;

    /* invent.c */
    /* query objlist callback: return TRUE if obj type matches "this_type" */
    int this_type;
    const char *this_title; /* title for inventory list of specific type */

    /* muse.c */
    int trapx;
    int trapy;

    /* rumors.c */
    long true_rumor_size; /* rumor size variables are signed so that value -1
                            can be used as a flag */
    unsigned long true_rumor_start; /* rumor start offsets are unsigned because
                                       they're handled via %lx format */
    long true_rumor_end; /* rumor end offsets are signed because they're
                            compared with [dlb_]ftell() */

    /* sp_lev.c */
    boolean themeroom_failed;

    /* timeout.c */
    /* ordered timer list */
    struct fe *timer_base; /* "active" */
    unsigned long timer_id;

    /* topten.c */
    winid toptenwin;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_u {

    /* botl.c */
    boolean update_all;

    /* decl.c */
    dest_area updest;
    boolean unweapon;

    /* role.c */
    struct Role urole; /* player's role. May be munged in role_init() */
    struct Race urace; /* player's race. May be munged in role_init() */

    /* save.c */
    unsigned ustuck_id; /* need to preserve during save */
    unsigned usteed_id; /* need to preserve during save */
    d_level uz_save;

    /* new stuff */
    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_v {

    /* botl.c */
    boolean valset[MAXBLSTATS];

    /* end.c */
    struct val_list valuables[3];
    int vanq_sortmode;

    /* mhitm.c */
    boolean vis;

    /* mklev.c */
    coordxy vault_x;
    coordxy vault_y;

    /* mon.c */
    boolean vamp_rise_msg;

    /* pickup.c */
    long val_for_n_or_more;
    /* list of menu classes for query_objlist() and allow_category callback
       (with room for all object classes, 'u'npaid, BUCX, and terminator) */
    char valid_menu_classes[MAXOCLASSES + 1 + 4 + 1];

    /* vision.c */
    seenV **viz_array;   /* used in cansee() and couldsee() macros */
    coordxy *viz_rmin;   /* min could see indices */
    coordxy *viz_rmax;   /* max could see indices */
    boolean vision_full_recalc;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_w {

    /* decl.c */
    int warn_obj_cnt; /* count of monsters meeting criteria */
    long wailmsg;

    /* symbols.c */
    nhsym warnsyms[WARNCOUNT]; /* the current warning display symbols */

    /* files.c */
    char wizkit[WIZKIT_MAX];

    /* hack.c */
    int wc; /* current weight_cap(); valid after call to inv_weight() */

    /* mkmaze.c */
    struct trap *wportal;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_x {

    /* decl.c */
    int x_maze_max;

    /* lock.c */
    struct xlock_s xlock;

    /* mkmaze.c */
    int xmin, xmax; /* level boundaries x */

    /* sp_lev.c */
    coordxy xstart, xsize;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_y {

    /* decl.c */
    int y_maze_max;
    struct monst youmonst;

    /* mkmaze.c */
    int ymin, ymax; /* level boundaries y */

    /* pline.c */
    /* work buffer for You(), &c and verbalize() */
    char *you_buf;
    int you_buf_siz;

    /* sp_lev.c */
    coordxy ystart, ysize;

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

struct instance_globals_z {

    /* mon.c */
    boolean zombify;

    /* muse.c */
    boolean zap_oseen; /* for wands which use mbhitm and are zapped at
                        * players.  We usually want an oseen local to
                        * the function, but this is impossible since the
                        * function mbhitm has to be compatible with the
                        * normal zap routines, and those routines don't
                        * remember who zapped the wand. */

    boolean havestate;
    unsigned long magic; /* validate that structure layout is preserved */
};

extern struct instance_globals_a ga;
extern struct instance_globals_b gb;
extern struct instance_globals_c gc;
extern struct instance_globals_d gd;
extern struct instance_globals_e ge;
extern struct instance_globals_f gf;
extern struct instance_globals_g gg;
extern struct instance_globals_h gh;
extern struct instance_globals_i gi;
extern struct instance_globals_j gj;
extern struct instance_globals_k gk;
extern struct instance_globals_l gl;
extern struct instance_globals_m gm;
extern struct instance_globals_n gn;
extern struct instance_globals_o go;
extern struct instance_globals_p gp;
extern struct instance_globals_q gq;
extern struct instance_globals_r gr;
extern struct instance_globals_s gs;
extern struct instance_globals_t gt;
extern struct instance_globals_u gu;
extern struct instance_globals_v gv;
extern struct instance_globals_w gw;
extern struct instance_globals_x gx;
extern struct instance_globals_y gy;
extern struct instance_globals_z gz;

struct const_globals {
    const struct obj zeroobj;      /* used to zero out a struct obj */
    const struct monst zeromonst;  /* used to zero out a struct monst */
    const anything zeroany;        /* used to zero out union any */
};

extern const struct const_globals cg;

#endif /* DECL_H */



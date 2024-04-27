/* NetHack 3.7	hack.h	$NHDT-Date: 1713334806 2024/04/17 06:20:06 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.253 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef HACK_H
#define HACK_H

#ifndef CONFIG_H
#include "config.h"
#endif
#include "lint.h"

#include "align.h"
#include "dungeon.h"
#include "stairs.h"
#include "objclass.h"
#include "wintype.h"
#include "flag.h"
#include "rect.h"
#include "sym.h"
#include "trap.h"
#include "youprop.h"
#include "display.h"

#include "botl.h"
#include "context.h"
#include "engrave.h"
#include "mkroom.h"
#include "obj.h"
#include "quest.h"
#include "rect.h"
#include "region.h"
#include "rm.h"
#include "selvar.h"
#include "sndprocs.h"
#include "spell.h"
#include "sym.h"
#include "sys.h"
#include "timeout.h"
#include "winprocs.h"
#include "wintype.h"
#include "vision.h"
#include "you.h"

#define TELL 1
#define NOTELL 0
#define ON 1
#define OFF 0
#define BOLT_LIM 8        /* from this distance ranged attacks will be made */
#define MAX_CARR_CAP 1000 /* so that boulders can be heavier */
#define DUMMY { 0 }       /* array initializer, letting [1..N-1] default */
#define DEF_NOTHING ' '   /* default symbol for NOTHING and UNEXPLORED  */

/* Macros for how a rumor was delivered in outrumor() */
#define BY_ORACLE 0
#define BY_COOKIE 1
#define BY_PAPER 2
#define BY_OTHER 9

/* bitmask flags for corpse_xname();
   PFX_THE takes precedence over ARTICLE, NO_PFX takes precedence over both */
#define CXN_NORMAL 0    /* no special handling */
#define CXN_SINGULAR 1  /* override quantity if greater than 1 */
#define CXN_NO_PFX 2    /* suppress "the" from "the Unique Monst */
#define CXN_PFX_THE 4   /* prefix with "the " (unless pname) */
#define CXN_ARTICLE 8   /* include a/an/the prefix */
#define CXN_NOCORPSE 16 /* suppress " corpse" suffix */

/* weight increment of heavy iron ball */
#define IRON_BALL_W_INCR 160

/* number of turns it takes for vault guard to show up */
#define VAULT_GUARD_TIME 30

/* sellobj_state() states */
#define SELL_NORMAL (0)
#define SELL_DELIBERATE (1)
#define SELL_DONTSELL (2)

#define SHOP_DOOR_COST 400L /* cost of a destroyed shop door */
#define SHOP_BARS_COST 300L /* cost of iron bars */
#define SHOP_HOLE_COST 200L /* cost of making hole/trapdoor */
#define SHOP_WALL_COST 200L /* cost of destroying a wall */
#define SHOP_WALL_DMG  (10L * ACURRSTR) /* damaging a wall */
#define SHOP_PIT_COST  100L /* cost of making a pit */
#define SHOP_WEB_COST   30L /* cost of removing a web */

/* flags for look_here() */
#define LOOKHERE_NOFLAGS       0U
#define LOOKHERE_PICKED_SOME   1U
#define LOOKHERE_SKIP_DFEATURE 2U

/* max size of a windowtype option */
#define WINTYPELEN 16

/* str_or_len from sp_lev.h */
typedef union str_or_len {
    char *str;
    int len;
} Str_or_Len;

enum artifacts_nums {
#define ARTI_ENUM
#include "artilist.h"
#undef ARTI_ENUM
    AFTER_LAST_ARTIFACT
};

enum misc_arti_nums {
    NROFARTIFACTS = (AFTER_LAST_ARTIFACT - 1)
};

/* related to breadcrumb struct */
enum bcargs {override_restriction = -1};

struct breadcrumbs {
    const char *funcnm;
    int linenum;
    boolean in_effect;
};

/* types of calls to bhit() */
enum bhit_call_types {
    ZAPPED_WAND   = 0,
    THROWN_WEAPON = 1,
    THROWN_TETHERED_WEAPON = 2,
    KICKED_WEAPON = 3,
    FLASHED_LIGHT = 4,
    INVIS_BEAM    = 5
};

/* Macros for messages referring to hands, eyes, feet, etc... */
enum bodypart_types {
    NO_PART   = -1,
    ARM       =  0,
    EYE       =  1,
    FACE      =  2,
    FINGER    =  3,
    FINGERTIP =  4,
    FOOT      =  5,
    HAND      =  6,
    HANDED    =  7,
    HEAD      =  8,
    LEG       =  9,
    LIGHT_HEADED = 10,
    NECK      = 11,
    SPINE     = 12,
    TOE       = 13,
    HAIR      = 14,
    BLOOD     = 15,
    LUNG      = 16,
    NOSE      = 17,
    STOMACH   = 18
};

#define MAX_BMASK 4

struct bubble {
    coordxy x, y;   /* coordinates of the upper left corner */
    schar dx, dy; /* the general direction of the bubble's movement */
    uchar bm[MAX_BMASK + 2];    /* bubble bit mask */
    struct bubble *prev, *next; /* need to traverse the list up and down */
    struct container *cons;
};

enum bubble_contains_types {
    CONS_OBJ = 0,
    CONS_MON,
    CONS_HERO,
    CONS_TRAP
};

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

enum {
    CQ_CANNED = 0, /* internal canned sequence */
    CQ_REPEAT,     /* user-inputted, if gi.in_doagain, replayed */
    NUM_CQS
};

typedef long cmdcount_nht;    /* Command counts */


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

struct c_color_names {
    const char *const c_black, *const c_amber, *const c_golden,
        *const c_light_blue, *const c_red, *const c_green, *const c_silver,
        *const c_blue, *const c_purple, *const c_white, *const c_orange;
};

struct c_common_strings {
    const char *const c_nothing_happens, *const c_nothing_seems_to_happen,
        *const c_thats_enough_tries, *const c_silly_thing_to,
        *const c_shudder_for_moment, *const c_something, *const c_Something,
        *const c_You_can_move_again, *const c_Never_mind,
        *const c_vision_clears, *const c_the_your[2], *const c_fakename[2];
};

struct container {
    struct container *next;
    coordxy x, y;
    short what;
    genericptr_t list;
};

/* alteration types--keep in synch with costly_alteration(mkobj.c) */
enum cost_alteration_types {
    COST_CANCEL  =  0, /* standard cancellation */
    COST_DRAIN   =  1, /* drain life upon an object */
    COST_UNCHRG  =  2, /* cursed charging */
    COST_UNBLSS  =  3, /* unbless (devalues holy water) */
    COST_UNCURS  =  4, /* uncurse (devalues unholy water) */
    COST_DECHNT  =  5, /* disenchant weapons or armor */
    COST_DEGRD   =  6, /* removal of rustproofing, dulling via engraving */
    COST_DILUTE  =  7, /* potion dilution */
    COST_ERASE   =  8, /* scroll or spellbook blanking */
    COST_BURN    =  9, /* dipped into flaming oil */
    COST_NUTRLZ  = 10, /* neutralized via unicorn horn */
    COST_DSTROY  = 11, /* wand breaking (bill first, useup later) */
    COST_SPLAT   = 12, /* cream pie to own face (ditto) */
    COST_BITE    = 13, /* start eating food */
    COST_OPEN    = 14, /* open tin */
    COST_BRKLCK  = 15, /* break box/chest's lock */
    COST_RUST    = 16, /* rust damage */
    COST_ROT     = 17, /* rotting attack */
    COST_CORRODE = 18, /* acid damage */
    COST_CRACK   = 19, /* damage to crystal armor */
};

/* used by unpaid_cost(shk.h) */
enum unpaid_cost_flags {
    COST_NOCONTENTS = 0,
    COST_CONTENTS   = 1,
    COST_SINGLEOBJ  = 2,
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

/* Dismount: causes for why you are no longer riding */
enum dismount_types {
    DISMOUNT_GENERIC  = 0,
    DISMOUNT_FELL     = 1,
    DISMOUNT_THROWN   = 2,
    DISMOUNT_KNOCKED  = 3, /* hero hit for knockback effect */
    DISMOUNT_POLY     = 4,
    DISMOUNT_ENGULFED = 5,
    DISMOUNT_BONES    = 6,
    DISMOUNT_BYCHOICE = 7
};

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
    xint16 d_tutorial_dnum;
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
#define tutorial_dnum           (gd.dungeon_topology.d_tutorial_dnum)
#define qstart_level            (gd.dungeon_topology.d_qstart_level)
#define qlocate_level           (gd.dungeon_topology.d_qlocate_level)
#define nemesis_level           (gd.dungeon_topology.d_nemesis_level)
#define knox_level              (gd.dungeon_topology.d_knox_level)
#define mineend_level           (gd.dungeon_topology.d_mineend_level)
#define sokoend_level           (gd.dungeon_topology.d_sokoend_level)
/* clang-format on */

#define dunlev_reached(x) (gd.dungeons[(x)->dnum].dunlev_ureached)
#define MAXLINFO (MAXDUNGEON * MAXLEVEL)

enum lua_theme_group {
    all_themes = 1,  /* for end of game */
    most_themes = 2, /* for entering endgame */
    tut_themes = 3,  /* for leaving tutorial */
};

enum earlyarg {
    ARG_DEBUG, ARG_VERSION, ARG_SHOWPATHS
#ifndef NODUMPENUMS
    , ARG_DUMPENUMS
#endif
    , ARG_DUMPGLYPHIDS
#ifdef WIN32
    , ARG_WINDOWS
#endif
#if defined(CRASHREPORT)
    , ARG_BIDSHOW
#endif
};

struct early_opt {
    enum earlyarg e;
    const char *name;
    int minlength;
    boolean valallowed;
};

/* symbolic names for capacity levels */
enum encumbrance_types {
    UNENCUMBERED = 0,
    SLT_ENCUMBER = 1, /* Burdened */
    MOD_ENCUMBER = 2, /* Stressed */
    HVY_ENCUMBER = 3, /* Strained */
    EXT_ENCUMBER = 4, /* Overtaxed */
    OVERLOADED   = 5  /* Overloaded */
};

struct entity {
    struct monst *emon;     /* youmonst for the player */
    struct permonst *edata; /* must be non-zero for record to be valid */
    int ex, ey;
};

struct enum_dump {
    int val;
    const char *nm;
};

/*
 * This is the way the game ends.  If these are rearranged, the arrays
 * in end.c and topten.c will need to be changed.  Some parts of the
 * code assume that PANICKED separates the deaths from the non-deaths.
 */
enum game_end_types {
    DIED         =  0,
    CHOKING      =  1,
    POISONING    =  2,
    STARVING     =  3,
    DROWNING     =  4,
    BURNING      =  5,
    DISSOLVED    =  6,
    CRUSHING     =  7,
    STONING      =  8,
    TURNED_SLIME =  9,
    GENOCIDED    = 10,
    PANICKED     = 11,
    TRICKED      = 12,
    QUIT         = 13,
    ESCAPED      = 14,
    ASCENDED     = 15
};

/* game events log */
struct gamelog_line {
    long turn; /* turn when this happened */
    long flags; /* LL_foo flags */
    char *text;
    struct gamelog_line *next;
};


/* values returned from getobj() callback functions */
enum getobj_callback_returns {
    /* generally invalid - can't be used for this purpose. will give a "silly
     * thing" message if the player tries to pick it, unless a more specific
     * failure message is in getobj itself - e.g. "You cannot foo gold". */
    GETOBJ_EXCLUDE = -3,
    /* invalid because it is not in inventory; used when the hands/self
     * possibility is queried and the player passed up something on the
     * floor before getobj. */
    GETOBJ_EXCLUDE_NONINVENT = -2,
    /* invalid because it is an inaccessible or unwanted piece of gear, but
     * pseudo-valid for the purposes of allowing the player to select it and
     * getobj to return it if there is a prompt instead of getting "silly
     * thing", in order for the getobj caller to present a specific failure
     * message. Other than that, the only thing this does differently from
     * GETOBJ_EXCLUDE is that it inserts an "else" in "You don't have anything
     * else to foo". */
    GETOBJ_EXCLUDE_INACCESS = -1,
    /* invalid for purposes of not showing a prompt if nothing is valid but
     * pseudo-valid for selecting - identical to GETOBJ_EXCLUDE_INACCESS but
     * without the "else" in "You don't have anything else to foo". */
    GETOBJ_EXCLUDE_SELECTABLE = 0,
    /* valid - invlet not presented in the summary or the ? menu as a
     * recommendation, but is selectable if the player enters it anyway.
     * Used for objects that are actually valid but unimportantly so, such
     * as shirts for reading. */
    GETOBJ_DOWNPLAY = 1,
    /* valid - will be shown in summary and ? menu */
    GETOBJ_SUGGEST  = 2,
};

/* getpos() return values */
enum getpos_retval {
    LOOK_TRADITIONAL = 0, /* '.' -- ask about "more info?" */
    LOOK_QUICK       = 1, /* ',' -- skip "more info?" */
    LOOK_ONCE        = 2, /* ';' -- skip and stop looping */
    LOOK_VERBOSE     = 3  /* ':' -- show more info w/o asking */
};

struct h2o_ctx {
    int dkn_boom, unk_boom; /* track dknown, !dknown separately */
    boolean ctx_valid;
};

/* attack mode for hmon() */
enum hmon_atkmode_types {
    HMON_MELEE   = 0, /* hand-to-hand */
    HMON_THROWN  = 1, /* normal ranged (or spitting while poly'd) */
    HMON_KICKED  = 2, /* alternate ranged */
    HMON_APPLIED = 3, /* polearm, treated as ranged */
    HMON_DRAGGED = 4  /* attached iron ball, pulled into mon */
};

/* hunger states - see hu_stat in eat.c */
enum hunger_state_types {
    SATIATED   = 0,
    NOT_HUNGRY = 1,
    HUNGRY     = 2,
    WEAK       = 3,
    FAINTING   = 4,
    FAINTED    = 5,
    STARVED    = 6
};

/* inventory counts (slots in tty parlance)
 * a...zA..Z    invlet_basic (52)
 * $a...zA..Z#  2 special additions
 */
enum inventory_counts {
    invlet_basic = 52,
    invlet_gold = 1,
    invlet_overflow = 1,
    invlet_max = invlet_basic + invlet_gold + invlet_overflow,
    /* 2023/11/30 invlet_max is not yet used anywhere */
};

struct kinfo {
    struct kinfo *next; /* chain of delayed killers */
    int id;             /* uprop keys to ID a delayed killer */
    int format;         /* one of the killer formats */
#define KILLED_BY_AN 0
#define KILLED_BY 1
#define NO_KILLER_PREFIX 2
    char name[BUFSZ]; /* actual killer name */
};

struct launchplace {
    struct obj *obj;
    coordxy x, y;
};

/* light source */
typedef struct ls_t {
    struct ls_t *next;
    coordxy x, y;  /* source's position */
    short range; /* source's current range */
    short flags;
    short type;  /* type of light source */
    anything id; /* source's identifier */
} light_source;

struct menucoloring {
    struct nhregex *match;
    char *origstr;
    int color, attr;
    struct menucoloring *next;
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

struct multishot {
    int n, i;
    short o;
    boolean s;
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

struct mvitals {
    uchar born;
    uchar died;
    uchar mvflags;
};


/* Lua callback functions */
enum nhcore_calls {
    NHCORE_START_NEW_GAME = 0,
    NHCORE_RESTORE_OLD_GAME,
    NHCORE_MOVELOOP_TURN,
    NHCORE_GAME_EXIT,
    NHCORE_GETPOS_TIP,
    NHCORE_ENTER_TUTORIAL,
    NHCORE_LEAVE_TUTORIAL,

    NUM_NHCORE_CALLS
};

/* Lua callbacks. TODO: Merge with NHCORE */
enum nhcb_calls {
    NHCB_CMD_BEFORE = 0,
    NHCB_LVL_ENTER,
    NHCB_LVL_LEAVE,
    NHCB_END_TURN,

    NUM_NHCB
};

/*
 * option setting restrictions
 */

enum optset_restrictions {
    set_in_sysconf = 0, /* system config file option only */
    set_in_config  = 1, /* config file option only */
    set_viaprog    = 2, /* may be set via extern program, not seen in game */
    set_gameview   = 3, /* may be set via extern program, displayed in game */
    set_in_game    = 4, /* may be set via extern program or set in the game */
    set_wizonly    = 5, /* may be set in the game if wizmode */
    set_wiznofuz   = 6, /* wizard-mode only, but not by fuzzer */
    set_hidden     = 7  /* placeholder for prefixed entries, never show it  */
};
#define SET__IS_VALUE_VALID(s) ((s < set_in_sysconf) || (s > set_wiznofuz))

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

/* polyself flags */
enum polyself_flags {
    POLY_NOFLAGS    = 0x00,
    POLY_CONTROLLED = 0x01,
    POLY_MONSTER    = 0x02,
    POLY_REVERT     = 0x04,
    POLY_LOW_CTRL   = 0x08
};

struct repo { /* repossession context */
    struct monst *shopkeeper;
    coord location;
};

struct restore_info {
    const char *name;
    int mread_flags;
};

enum restore_stages {
    REST_GSTATE = 1,    /* restoring current level and game state */
    REST_LEVELS = 2,    /* restoring remainder of dungeon */
};

struct rogueroom {
    coordxy rlx, rly;
    coordxy dx, dy;
    boolean real;
    uchar doortable;
    int nroom; /* Only meaningful for "real" rooms */
};

#define NUM_ROLES (13)
struct role_filter {
    boolean roles[NUM_ROLES + 1];
    short mask;
};

enum saveformats {
    invalid = 0,
    historical = 1,     /* entire struct, binary, as-is */
    lendian = 2,        /* each field, binary, little-endian */
    ascii = 3           /* each field, ascii text (just proof of concept) */
};

struct selectionvar {
    int wid, hei;
    boolean bounds_dirty;
    NhRect bounds; /* use selection_getbounds() */
    char *map;
};

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
    int in_getlev;              /* in getlev() */
    int in_moveloop;            /* normal gameplay in progress */
    int in_impossible;          /* reporting a warning */
    int in_docrt;               /* in docrt(): redrawing the whole screen */
    int in_self_recover;        /* processing orphaned level files */
    int in_checkpoint;          /* saving insurance checkpoint */
    int in_parseoptions;        /* in parseoptions */
    int in_role_selection;      /* role/race/&c selection menus in progress */
    int in_getlin;              /* inside interface getlin routine */
    int config_error_ready;     /* config_error_add is ready, available */
    int beyond_savefile_load;   /* set when past savefile loading */
#ifdef PANICLOG
    int in_paniclog;            /* writing a panicloc entry */
#endif
    int wizkit_wishing;         /* starting wizard mode game w/ WIZKIT file */
    /* input_state:  used in the core for the 'altmeta' option to process ESC;
       used in the curses interface to avoid arrow keys when user is doing
       something other than entering a command or direction and in the Qt
       interface to suppress menu commands in similar conditions;
       readchar() always resets it to 'otherInp' prior to returning */
    int input_state; /* whether next key pressed will be entering a command */
#ifdef TTY_GRAPHICS
    /* resize_pending only matters when handling a SIGWINCH signal for tty;
       getting_char is used along with that and also separately for UNIX;
       we minimize #if conditionals for them to avoid unnecessary clutter */
    volatile int resize_pending; /* set by signal handler */
    volatile int getting_char;  /* referenced during signal handling */
#endif
};

/* value of program_state.input_state, significant during readchar();
   get_count() expects digits then a command so sets it to commandInp */
enum InputState {
    otherInp   = 0, /* 'other' */
    commandInp = 1, /* readchar() */
    getposInp  = 2, /* getpos() */
    getdirInp  = 3, /* getdir() */
};

/* sortloot() return type; needed before extern.h */
struct sortloot_item {
    struct obj *obj;
    char *str; /* result of loot_xname(obj) in some cases, otherwise null */
    /* these need to be signed; 'indx' should be big enough to hold a count
       of the largest pile of items, the others fit within char */
    int indx;        /* index into original list (used as tie-breaker) */
    int8 orderclass; /* order rather than object class; 0 => not yet init'd */
    int8 subclass;   /* subclass for some classes */
    int8 disco;      /* discovery status */
    int8 inuse;      /* 0: not in-use or not sorting by inuse_only;
                      * 1: lit candle/lamp or attached leash; 2: worn armor;
                      * 3: wielded weapon (including uswapwep and uquiver);
                      * 4: worn accessory (amulet, rings, blindfold). */
};
typedef struct sortloot_item Loot;

typedef struct strbuf {
    int    len;
    char  *str;
    char   buf[256];
} strbuf_t;

struct trapinfo {
    struct obj *tobj;
    coordxy tx, ty;
    int time_needed;
    boolean force_bungle;
};

/* values for rtype are defined in dungeon.h */
/* lev_region from sp_lev.h */
typedef struct {
    struct {
        coordxy x1, y1, x2, y2;
    } inarea;
    struct {
        coordxy x1, y1, x2, y2;
    } delarea;
    boolean in_islev, del_islev;
    coordxy rtype, padding;
    Str_or_Len rname;
} lev_region;

/* Flags for controlling uptodate */
#define UTD_CHECKSIZES                 0x01
#define UTD_CHECKFIELDCOUNTS           0x02
#define UTD_SKIP_SANITY1               0x04
#define UTD_SKIP_SAVEFILEINFO          0x08
#define UTD_WITHOUT_WAITSYNCH_PERFILE  0x10

#define ENTITIES 2
struct valuable_data {
    long count;
    int typ;
};

struct val_list {
    struct valuable_data *list;
    int size;
};

enum vanq_order_modes {
    VANQ_MLVL_MNDX = 0, /* t - traditional: by monster level */
    VANQ_MSTR_MNDX,     /* d - by difficulty rating */
    VANQ_ALPHA_SEP,     /* a - alphabetical, first uniques, then ordinary */
    VANQ_ALPHA_MIX,     /* A - alpha with uniques and ordinary intermixed */
    VANQ_MCLS_HTOL,     /* C - by class, high to low within class */
    VANQ_MCLS_LTOH,     /* c - by class, low to high within class */
    VANQ_COUNT_H_L,     /* n - by count, high to low */
    VANQ_COUNT_L_H,     /* z - by count, low to high */

    NUM_VANQ_ORDER_MODES
};

struct autopickup_exception {
    struct nhregex *regex;
    char *pattern;
    boolean grab;
    struct autopickup_exception *next;
};

/* at most one of `door' and `box' should be non-null at any given time */
struct xlock_s {
    struct rm *door;
    struct obj *box;
    int picktyp, /* key|pick|card for unlock, sharp vs blunt for #force */
        chance, usedtime;
    boolean magic_key;
};

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
    boolean fieldlevel;   /* fieldlevel saves each field individually */
    boolean addinfo;      /* if set, some additional context info from core */
    boolean eof;          /* place to mark eof reached */
    boolean bendian;      /* set to true if executing on big-endian machine */
    FILE *fpdef;          /* file pointer for fieldlevel default style */
    FILE *fpdefmap;       /* file pointer mapfile for def format */
    FILE *fplog;          /* file pointer logfile */
    FILE *fpdebug;        /* file pointer debug info */
    struct fieldlevel_content style;
} NHFILE;

/* Monster name articles */
#define ARTICLE_NONE 0
#define ARTICLE_THE 1
#define ARTICLE_A 2
#define ARTICLE_YOUR 3

/* x_monnam() monster name suppress masks */
#define SUPPRESS_IT            0x01
#define SUPPRESS_INVISIBLE     0x02
#define SUPPRESS_HALLUCINATION 0x04
#define SUPPRESS_SADDLE        0x08
#define SUPPRESS_MAPPEARANCE   0x10
#define EXACT_NAME             0x1F
#define SUPPRESS_NAME 0x20
#define AUGMENT_IT    0x40 /* use "someone" or "something" instead of "it" */

/* pline (et al) for a single string argument (suppress compiler warning) */
#define pline1(cstr) pline("%s", cstr)
#define Your1(cstr) Your("%s", cstr)
#define You1(cstr) You("%s", cstr)
#define verbalize1(cstr) verbalize("%s", cstr)
#define You_hear1(cstr) You_hear("%s", cstr)
#define Sprintf1(buf, cstr) Sprintf(buf, "%s", cstr)
#define panic1(cstr) panic("%s", cstr)

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

/* from options.c */
#define MAX_MENU_MAPPED_CMDS 32 /* some number */

/* player selection constants */
#define BP_ALIGN 0
#define BP_GEND 1
#define BP_RACE 2
#define BP_ROLE 3
#define NUM_BP 4

/* some array sizes for 'g?' */
#define WIZKIT_MAX 128
#define CVT_BUF_SIZE 64

#define LUA_VER_BUFSIZ 20
#define LUA_COPYRIGHT_BUFSIZ 120

/* Symbol offsets */
#define SYM_OFF_P (0)
#define SYM_OFF_O (SYM_OFF_P + MAXPCHARS)   /* MAXPCHARS from sym.h */
#define SYM_OFF_M (SYM_OFF_O + MAXOCLASSES) /* MAXOCLASSES from objclass.h */
#define SYM_OFF_W (SYM_OFF_M + MAXMCLASSES) /* MAXMCLASSES from sym.h*/
#define SYM_OFF_X (SYM_OFF_W + WARNCOUNT)
#define SYM_MAX (SYM_OFF_X + MAXOTHER)

/* The UNDEFINED macros are used to initialize variables whose
   initialized value is not relied upon.
   UNDEFINED_VALUE: used to initialize any scalar type except pointers.
   UNDEFINED_VALUES: used to initialize any non scalar type without pointers.
   UNDEFINED_PTR: can be used only on pointer types. */
#define UNDEFINED_VALUE 0
#define UNDEFINED_VALUES { 0 }
#define UNDEFINED_PTR NULL

/* The UNDEFINED_ROLE macro is used to initialize Role variables */
#define UNDEFINED_ROLE \
    {                                           \
      /* role name, set of rank names */        \
      { NULL, NULL }, { { NULL, NULL } },       \
      /* strings: pantheon deity names */       \
      NULL, NULL, NULL,                         \
      /* file code, quest home+goal names */    \
      NULL, NULL, NULL,                         \
      /* indices: base mon type, pet */         \
      NON_PM, NON_PM,                           \
      /* quest leader, guardians, nemesis */    \
      NON_PM, NON_PM, NON_PM,                   \
      /* quest enemy types (index, symbol) */   \
      NON_PM, NON_PM, '\0', '\0',               \
      /* quest artifact object index */         \
      STRANGE_OBJECT,                           \
      /* Bitmasks */                            \
      0,                                        \
      /* Attributes */                          \
      {0}, {0}, {0}, {0}, 0, 0,                 \
      /* spell statistics */                    \
      0, 0, 0, 0, 0, 0, 0 }

/* The UNDEFINED_RACE macro is used to initialize Race variables */
#define UNDEFINED_RACE \
    {                                           \
      /* strings */                             \
      NULL, NULL, NULL, NULL, { NULL, NULL },   \
      /* Indices: base race, mummy, zombie */   \
      NON_PM, NON_PM, NON_PM,                   \
      /* Bitmasks */                            \
      0, 0, 0, 0,                               \
      /* Characteristic limits */               \
      {0}, {0},                                 \
      /* Level change HP and Pw adjustments */  \
      {0}, {0}                                  \
    }

#define MATCH_WARN_OF_MON(mon) \
    (Warn_of_mon                                                        \
     && ((gc.context.warntype.obj & (mon)->data->mflags2) != 0           \
         || (gc.context.warntype.polyd & (mon)->data->mflags2) != 0      \
         || (gc.context.warntype.species                                 \
             && (gc.context.warntype.species == (mon)->data))))

typedef uint32_t mmflags_nht;     /* makemon MM_ flags */


/* flags to control makemon(); goodpos() uses some plus has some of its own*/
#define NO_MM_FLAGS     0x00000000L /* use this rather than plain 0 */
#define NO_MINVENT      0x00000001L /* suppress minvent when creating mon */
#define MM_NOWAIT       0x00000002L /* don't set STRAT_WAITMASK flags */
#define MM_NOCOUNTBIRTH 0x00000004L /* don't incr born count (for revival) */
#define MM_IGNOREWATER  0x00000008L /* ignore water when positioning */
#define MM_ADJACENTOK   0x00000010L /* ok to use adjacent coordinates */
#define MM_ANGRY        0x00000020L /* monster is created angry */
#define MM_NONAME       0x00000040L /* monster is not christened */
#define MM_EGD          0x00000080L /* add egd structure */
#define MM_EPRI         0x00000100L /* add epri structure */
#define MM_ESHK         0x00000200L /* add eshk structure */
#define MM_EMIN         0x00000400L /* add emin structure */
#define MM_EDOG         0x00000800L /* add edog structure */
#define MM_ASLEEP       0x00001000L /* monsters should be generated asleep */
#define MM_NOGRP        0x00002000L /* suppress creation of monster groups */
#define MM_NOTAIL       0x00004000L /* if a long worm, don't give it a tail */
#define MM_MALE         0x00008000L /* male variation */
#define MM_FEMALE       0x00010000L /* female variation */
#define MM_NOMSG        0x00020000L /* no appear message */
#define MM_NOEXCLAM     0x00040000L /* more sedate "<mon> appears."
                                     * mesg for ^G */
#define MM_IGNORELAVA   0x00080000L /* ignore lava when positioning */
#define MM_MINVIS       0x00100000L /* for ^G/create_particular */
/* if more MM_ flag masks are added, skip or renumber the GP_ one(s) */
#define GP_ALLOW_XY     0x00200000L /* [actually used by enexto() to decide
                                     * whether to make extra call to goodpos()] */
#define GP_ALLOW_U      0x00400000L /* don't reject hero's location */
#define GP_CHECKSCARY   0x00800000L /* check monster for onscary() */
#define GP_AVOID_MONPOS 0x01000000L /* don't accept existing mon location */
/* 25 bits used */

/* flags for mhidden_description() (pager.c; used for mimics and hiders) */
#define MHID_PREFIX  1 /* include ", mimicking " prefix */
#define MHID_ARTICLE 2 /* include "a " or "an " after prefix */
#define MHID_ALTMON  4 /* if mimicking a monster, include that */

/* flags for make_corpse() and mkcorpstat(); 0..7 are recorded in obj->spe */
#define CORPSTAT_NONE     0x00
#define CORPSTAT_GENDER   0x03 /* 0x01 | 0x02 */
#define CORPSTAT_HISTORIC 0x04 /* historic statue; not used for corpse */
#define CORPSTAT_SPE_VAL  0x07 /* 0x03 | 0x04 */
#define CORPSTAT_INIT     0x08 /* pass init flag to mkcorpstat */
#define CORPSTAT_BURIED   0x10 /* bury the corpse or statue */
/* note: gender flags have different values from those used for monsters
   so that 0 can be unspecified/random instead of male */
#define CORPSTAT_RANDOM 0
#define CORPSTAT_FEMALE 1
#define CORPSTAT_MALE   2
#define CORPSTAT_NEUTER 3

/* flag bits for collect_coords(); combining ring_pairs with unshuffled
   makes no sense--if both are specified unshuffled takes precedence */
#define CC_NO_FLAGS    0x00 /* skip center, collect in distinct rings and
                             * shuffle each ring, ignore monster occupants */
#define CC_INCL_CENTER 0x01 /* include center point as ring #0 */
#define CC_UNSHUFFLED  0x02 /* don't shuffle the rings */
#define CC_RING_PAIRS  0x04 /* shuffle w/ odd and next even rings together */
#define CC_SKIP_MONS   0x08 /* skip locations occupied by monsters */
#define CC_SKIP_INACCS 0x10 /* skip !ZAP_POS: reject rock and wall locations
                             * but allow pools, unlike !ACCESSIBLE */

/* flags for decide_to_shift() */
#define SHIFT_SEENMSG 0x01 /* put out a message if in sight */
#define SHIFT_MSG 0x02     /* always put out a message */

/* m_poisongas_ok() return values */
#define M_POISONGAS_BAD   0 /* poison gas is bad */
#define M_POISONGAS_MINOR 1 /* poison gas is ok, maybe causes coughing */
#define M_POISONGAS_OK    2 /* ignores poison gas completely */

/* flags for deliver_obj_to_mon */
#define DF_NONE     0x00
#define DF_RANDOM   0x01
#define DF_ALL      0x04

/* special mhpmax value when loading bones monster to flag as extinct or
 * genocided */
#define DEFUNCT_MONSTER (-100)

/* macro form of adjustments of physical damage based on Half_physical_damage.
 * Can be used on-the-fly with the 1st parameter to losehp() if you don't
 * need to retain the dmg value beyond that call scope.
 * Take care to ensure it doesn't get used more than once in other instances.
 */
#define Maybe_Half_Phys(dmg) \
    ((Half_physical_damage) ? (((dmg) + 1) / 2) : (dmg))

/* flags for special ggetobj status returns */
#define ALL_FINISHED 0x01 /* called routine already finished the job */

/* flags to control query_objlist() */
#define BY_NEXTHERE       0x0001 /* follow objlist by nexthere field */
#define INCLUDE_VENOM     0x0002 /* include venom objects if present */
#define AUTOSELECT_SINGLE 0x0004 /* if only 1 object, don't ask */
#define USE_INVLET        0x0008 /* use object's invlet */
#define INVORDER_SORT     0x0010 /* sort objects by packorder */
#define SIGNAL_NOMENU     0x0020 /* return -1 rather than 0 if none allowed */
#define SIGNAL_ESCAPE     0x0040 /* return -2 rather than 0 for ESC */
#define FEEL_COCKATRICE   0x0080 /* engage cockatrice checks and react */
#define INCLUDE_HERO      0x0100 /* show hero among engulfer's inventory */

/* Flags to control query_category() */
/* BY_NEXTHERE and INCLUDE_VENOM are used by query_category() too, so
   skip 0x0001 and 0x0002 */
#define UNPAID_TYPES      0x0004
#define GOLD_TYPES        0x0008
#define WORN_TYPES        0x0010
#define ALL_TYPES         0x0020
#define BILLED_TYPES      0x0040
#define CHOOSE_ALL        0x0080
#define BUC_BLESSED       0x0100
#define BUC_CURSED        0x0200
#define BUC_UNCURSED      0x0400
#define BUC_UNKNOWN       0x0800
#define JUSTPICKED        0x1000
#define BUC_ALLBKNOWN (BUC_BLESSED | BUC_CURSED | BUC_UNCURSED)
#define BUCX_TYPES (BUC_ALLBKNOWN | BUC_UNKNOWN)
#define ALL_TYPES_SELECTED -2

/* Flags for oname(), artifact_exists(), artifact_origin() */
#define ONAME_NO_FLAGS   0U /* none of the below; they apply to artifacts */
/*                       0x0001U is reserved for 'exists' */
/* flags indicating where an artifact came from */
#define ONAME_VIA_NAMING 0x0002U /* oname() is being called by do_oname();
                                  * only matters if creating Sting|Orcrist */
#define ONAME_WISH       0x0004U /* created via wish */
#define ONAME_GIFT       0x0008U /* created as a divine reward after #offer or
                                  * special #pray result of being crowned */
#define ONAME_VIA_DIP    0x0010U /* created Excalibur in a fountain */
#define ONAME_LEVEL_DEF  0x0020U /* placed by a special level's definition */
#define ONAME_BONES      0x0040U /* object came from bones; in its original
                                  * game it had one of the other bits but we
                                  * don't care which one */
#define ONAME_RANDOM     0x0080U /* something created an artifact randomly
                                  * with mk_artifact() (mksboj or mk_player)
                                  * or m_initweap() (lawful Angel) */
/* flag controlling potential livelog event of finding an artifact */
#define ONAME_KNOW_ARTI  0x0100U /* hero is already aware of this artifact */
/* flag for suppressing perm_invent update when name gets assigned */
#define ONAME_SKIP_INVUPD 0x0200U /* don't call update_inventory() */

/* Flags to control find_mid() */
#define FM_FMON 0x01    /* search the fmon chain */
#define FM_MIGRATE 0x02 /* search the migrating monster chain */
#define FM_MYDOGS 0x04  /* search gm.mydogs */
#define FM_EVERYWHERE (FM_FMON | FM_MIGRATE | FM_MYDOGS)

/* Flags to control pick_[race,role,gend,align] routines in role.c */
#define PICK_RANDOM 0
#define PICK_RIGID 1

/* Flags to control dotrap() and mintrap() in trap.c */
#define NO_TRAP_FLAGS 0x00U
#define FORCETRAP     0x01U /* triggering not left to chance */
#define NOWEBMSG      0x02U /* suppress stumble into web message */
#define FORCEBUNGLE   0x04U /* adjustments appropriate for bungling */
#define RECURSIVETRAP 0x08U /* trap changed into another type this same turn */
#define TOOKPLUNGE    0x10U /* used '>' to enter pit below you */
#define VIASITTING    0x20U /* #sit while at trap location (affects message) */
#define FAILEDUNTRAP  0x40U /* trap activated by failed untrap attempt */
#define HURTLING      0x80U /* monster is hurtling through air */

/* Flags to control test_move in hack.c */
#define DO_MOVE 0   /* really doing the move */
#define TEST_MOVE 1 /* test a normal move (move there next) */
#define TEST_TRAV 2 /* test a future travel location */
#define TEST_TRAP 3 /* check if a future travel loc is a trap */

/* m_move return values */
#define MMOVE_NOTHING 0
#define MMOVE_MOVED   1 /* monster moved */
#define MMOVE_DIED    2 /* monster died */
#define MMOVE_DONE    3 /* monster used up all actions */
#define MMOVE_NOMOVES 4 /* monster has no valid locations to move to */

/*** some utility macros ***/
#define y_n(query) yn_function(query, ynchars, 'n', TRUE)
#define ynq(query) yn_function(query, ynqchars, 'q', TRUE)
#define ynaq(query) yn_function(query, ynaqchars, 'y', TRUE)
#define nyaq(query) yn_function(query, ynaqchars, 'n', TRUE)
#define nyNaq(query) yn_function(query, ynNaqchars, 'n', TRUE)
#define ynNaq(query) yn_function(query, ynNaqchars, 'y', TRUE)
/* YN() is same as y_n() except doesn't save the response in do-again buffer */
#define YN(query) yn_function(query, ynchars, 'n', FALSE)

/* Macros for scatter */
#define VIS_EFFECTS 0x01 /* display visual effects */
#define MAY_HITMON 0x02  /* objects may hit monsters */
#define MAY_HITYOU 0x04  /* objects may hit you */
#define MAY_HIT (MAY_HITMON | MAY_HITYOU)
#define MAY_DESTROY 0x08  /* objects may be destroyed at random */
#define MAY_FRACTURE 0x10 /* boulders & statues may fracture */

/* Macros for launching objects */
#define ROLL 0x01          /* the object is rolling */
#define FLING 0x02         /* the object is flying thru the air */
#define LAUNCH_UNSEEN 0x40 /* hero neither caused nor saw it */
#define LAUNCH_KNOWN 0x80  /* the hero caused this by explicit action */

/* enlightenment control flags */
#define BASICENLIGHTENMENT 1 /* show mundane stuff */
#define MAGICENLIGHTENMENT 2 /* show intrinsics and such */
#define ENL_GAMEINPROGRESS 0
#define ENL_GAMEOVERALIVE  1 /* ascension, escape, quit, trickery */
#define ENL_GAMEOVERDEAD   2

/* control flags for sortloot() */
#define SORTLOOT_PACK   0x01
#define SORTLOOT_INVLET 0x02
#define SORTLOOT_LOOT   0x04
#define SORTLOOT_INUSE  0x08 /* for inventory, in-use items first */
#define SORTLOOT_PETRIFY 0x20 /* override filter func for c-trice corpses */

/* flags for xkilled() [note: meaning of first bit used to be reversed,
   1 to give message and 0 to suppress] */
#define XKILL_GIVEMSG   0
#define XKILL_NOMSG     1
#define XKILL_NOCORPSE  2
#define XKILL_NOCONDUCT 4

/* pline_flags; mask values for custompline()'s first argument */
/* #define PLINE_ORDINARY 0 */
#define PLINE_NOREPEAT   1
#define OVERRIDE_MSGTYPE 2
#define SUPPRESS_HISTORY 4
#define URGENT_MESSAGE   8
#define PLINE_VERBALIZE 16
#define PLINE_SPEECH    32
#define NO_CURS_ON_U    64

/* get_count flags */
#define GC_NOFLAGS   0
#define GC_SAVEHIST  1 /* save "Count: 123" in message history */
#define GC_CONDHIST  2 /* save "Count: N" in message history unless the
                        * first digit is passed in and N matches it */
#define GC_ECHOFIRST 4 /* echo "Count: 1" even when there's only one digit */

/* rloc() flags */
#define RLOC_NONE    0x00
#define RLOC_ERR     0x01 /* allow impossible() if no rloc */
#define RLOC_MSG     0x02 /* show vanish/appear msg */
#define RLOC_NOMSG   0x04 /* prevent appear msg, even for STRAT_APPEARMSG */

/* indices for some special tin types */
#define ROTTEN_TIN 0
#define HOMEMADE_TIN 1
#define SPINACH_TIN (-1)
#define RANDOM_TIN (-2)
#define HEALTHY_TIN (-3)

/* Corpse aging */
#define TAINT_AGE (50L)        /* age when corpses go bad */
#define TROLL_REVIVE_CHANCE 37 /* 1/37 chance for 50 turns ~ 75% chance */
#define ROT_AGE (250L)         /* age when corpses rot away */

/* Some misc definitions */
#define POTION_OCCUPANT_CHANCE(n) (13 + 2 * (n))
#define WAND_BACKFIRE_CHANCE 100
#define WAND_WREST_CHANCE 121
#define BALL_IN_MON (u.uswallow && uball && uball->where == OBJ_FREE)
#define CHAIN_IN_MON (u.uswallow && uchain && uchain->where == OBJ_FREE)
#define NODIAG(monnum) ((monnum) == PM_GRID_BUG)

/* Flags to control menus */
#define MENUTYPELEN sizeof("traditional ")
#define MENU_TRADITIONAL 0
#define MENU_COMBINATION 1
#define MENU_FULL 2
#define MENU_PARTIAL 3

/* flags to control teleds() */
#define TELEDS_NO_FLAGS   0
#define TELEDS_ALLOW_DRAG 1
#define TELEDS_TELEPORT   2

/* flags for mktrap() */
#define MKTRAP_NOFLAGS       0x0U
#define MKTRAP_SEEN          0x1U /* trap is seen */
#define MKTRAP_MAZEFLAG      0x2U /* choose random coords instead of room */
#define MKTRAP_NOSPIDERONWEB 0x4U /* web will not generate a spider */
#define MKTRAP_NOVICTIM      0x8U /* no victim corpse or items on it */

#define MON_POLE_DIST 5 /* How far monsters can use pole-weapons */
#define PET_MISSILE_RANGE2 36 /* Square of distance within which pets shoot */

/* flags passed to getobj() to control how it responds to player input */
#define GETOBJ_NOFLAGS  0x0
#define GETOBJ_ALLOWCNT 0x1 /* is a count allowed with this command? */
#define GETOBJ_PROMPT   0x2 /* should it force a prompt for input? (prevents it
                               exiting early with "You don't have anything to
                               foo" if nothing in inventory is valid) */

/* flags for hero_breaks() and hits_bars(); BRK_KNOWN* let callers who have
   already called breaktest() prevent it from being called again since it
   has a random factor which makes it be non-deterministic */
#define BRK_BY_HERO        0x01
#define BRK_FROM_INV       0x02
#define BRK_KNOWN2BREAK    0x04
#define BRK_KNOWN2NOTBREAK 0x08
#define BRK_KNOWN_OUTCOME  (BRK_KNOWN2BREAK | BRK_KNOWN2NOTBREAK)
#define BRK_MELEE          0x10

/* extended command return values */
#define ECMD_OK     0x00 /* cmd done successfully */
#define ECMD_TIME   0x01 /* cmd took time, uses up a turn */
#define ECMD_CANCEL 0x02 /* cmd canceled by user */
#define ECMD_FAIL   0x04 /* cmd failed to finish, maybe with a yafm */

/* flags for newcham() */
#define NO_NC_FLAGS          0U
#define NC_SHOW_MSG          0x01U
#define NC_VIA_WAND_OR_SPELL 0x02U

/* constant passed to explode() for gas spores because gas spores are weird
 * Specifically, this is an exception to the whole "explode() uses dobuzz types"
 * system (the range -1 to -9 isn't used by it, for some reason), where this is
 * effectively an extra dobuzz type, and some zap.c code needs to be aware of
 * it.  */
#define PHYS_EXPL_TYPE -1

/* macros for dobuzz() type */
#define BZ_VALID_ADTYP(adtyp) ((adtyp) >= AD_MAGM && (adtyp) <= AD_SPC2)

#define BZ_OFS_AD(adtyp) (abs((adtyp) - AD_MAGM) % 10)
#define BZ_OFS_WAN(otyp) (abs((otyp) - WAN_MAGIC_MISSILE) % 10)
#define BZ_OFS_SPE(otyp) (abs((otyp) - SPE_MAGIC_MISSILE) % 10)
/* hero shooting a wand */
#define BZ_U_WAND(bztyp) (0 + (bztyp))     /*  0..9  */
/* hero casting a spell */
#define BZ_U_SPELL(bztyp) (10 + (bztyp))   /* 10..19 */
/* hero breathing as a monster */
#define BZ_U_BREATH(bztyp) (20 + (bztyp))  /* 20..29 */
/* monster casting a spell */
#define BZ_M_SPELL(bztyp) (-10 - (bztyp))  /* -19..-10 */
/* monster breathing */
#define BZ_M_BREATH(bztyp) (-20 - (bztyp)) /* -29..-20 */
/* monster shooting a wand; note: not -9 to -0 because -0 is ambiguous  */
#define BZ_M_WAND(bztyp) (-30 - (bztyp))   /* -39..-30 */

/* pick a random entry from array */
#define ROLL_FROM(array) array[rn2(SIZE(array))]
/* array with terminator variation */
/* #define ROLL_FROMT(array) array[rn2(SIZE(array) - 1)] */

/* validate index of array */
#define IndexOk(idx, array) \
    ((idx) >= 0 && (idx) < SIZE(array))
/* array with terminator variation */
#define IndexOkT(idx, array) \
    ((idx) >= 0 && (idx) < (SIZE(array) - 1))

#define FEATURE_NOTICE_VER(major, minor, patch)                    \
    (((unsigned long) major << 24) | ((unsigned long) minor << 16) \
     | ((unsigned long) patch << 8) | ((unsigned long) 0))

#define FEATURE_NOTICE_VER_MAJ (flags.suppress_alert >> 24)
#define FEATURE_NOTICE_VER_MIN \
    (((unsigned long) (0x0000000000FF0000L & flags.suppress_alert)) >> 16)
#define FEATURE_NOTICE_VER_PATCH \
    (((unsigned long) (0x000000000000FF00L & flags.suppress_alert)) >> 8)

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif
#define plur(x) (((x) == 1) ? "" : "s")

/* Cast to int, but limit value to range. */
#define LIMIT_TO_RANGE_INT(lo, hi, var) \
    ((int) ((var) < (lo) ? (lo) : (var) > (hi) ? (hi) : (var)))

#define ARM_BONUS(obj) \
    (objects[(obj)->otyp].a_ac + (obj)->spe                             \
     - min((int) greatest_erosion(obj), objects[(obj)->otyp].a_ac))

#define makeknown(x) discover_object((x), TRUE, TRUE)
#define distu(xx, yy) dist2((coordxy) (xx), (coordxy) (yy), u.ux, u.uy)
#define mdistu(mon) distu((mon)->mx, (mon)->my)
#define onlineu(xx, yy) online2((coordxy)(xx), (coordxy)(yy), u.ux, u.uy)

#define rn1(x, y) (rn2(x) + (y))

/* negative armor class is randomly weakened to prevent invulnerability */
#define AC_VALUE(AC) ((AC) >= 0 ? (AC) : -rnd(-(AC)))

#if defined(MICRO) && !defined(__DJGPP__)
#define getuid() 1
#define getlogin() ((char *) 0)
#endif /* MICRO */

/* The function argument to qsort() requires a particular
 * calling convention under WINCE which is not the default
 * in that environment.
 */
#if defined(WIN_CE)
#define QSORTCALLBACK __cdecl
#else
#define QSORTCALLBACK
#endif

#define SIG_RET_TYPE void (*)(int)

#define DEVTEAM_EMAIL "devteam@nethack.org"
#define DEVTEAM_URL "https://www.nethack.org/"

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
#include "nhlua.h"
#endif

#include "extern.h"
#include "decl.h"

#endif /* HACK_H */


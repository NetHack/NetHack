/* NetHack 3.6  decl.h  $NHDT-Date: 1496531104 2017/06/03 23:05:04 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.82 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2007. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DECL_H
#define DECL_H

#define E extern

E int NDECL((*occupation));
E int NDECL((*afternmv));

E const char *hname;
E int hackpid;
#if defined(UNIX) || defined(VMS)
E int locknum;
#endif
#ifdef DEF_PAGER
E char *catmore;
#endif /* DEF_PAGER */

E char SAVEF[];
#ifdef MICRO
E char SAVEP[];
#endif

/* max size of a windowtype option */
#define WINTYPELEN 16
E char chosen_windowtype[WINTYPELEN];

E NEARDATA int bases[MAXOCLASSES];

E NEARDATA int multi;
E const char *multi_reason;
E NEARDATA int nroom;
E NEARDATA int nsubroom;
E NEARDATA int occtime;

E NEARDATA int warn_obj_cnt; /* count of monsters meeting criteria */

E int x_maze_max, y_maze_max;
E int otg_temp;

E NEARDATA int in_doagain;

E struct dgn_topology { /* special dungeon levels for speed */
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
} dungeon_topology;
/* macros for accessing the dungeon levels by their old names */
/* clang-format off */
#define oracle_level            (dungeon_topology.d_oracle_level)
#define bigroom_level           (dungeon_topology.d_bigroom_level)
#define rogue_level             (dungeon_topology.d_rogue_level)
#define medusa_level            (dungeon_topology.d_medusa_level)
#define stronghold_level        (dungeon_topology.d_stronghold_level)
#define valley_level            (dungeon_topology.d_valley_level)
#define wiz1_level              (dungeon_topology.d_wiz1_level)
#define wiz2_level              (dungeon_topology.d_wiz2_level)
#define wiz3_level              (dungeon_topology.d_wiz3_level)
#define juiblex_level           (dungeon_topology.d_juiblex_level)
#define orcus_level             (dungeon_topology.d_orcus_level)
#define baalzebub_level         (dungeon_topology.d_baalzebub_level)
#define asmodeus_level          (dungeon_topology.d_asmodeus_level)
#define portal_level            (dungeon_topology.d_portal_level)
#define sanctum_level           (dungeon_topology.d_sanctum_level)
#define earth_level             (dungeon_topology.d_earth_level)
#define water_level             (dungeon_topology.d_water_level)
#define fire_level              (dungeon_topology.d_fire_level)
#define air_level               (dungeon_topology.d_air_level)
#define astral_level            (dungeon_topology.d_astral_level)
#define tower_dnum              (dungeon_topology.d_tower_dnum)
#define sokoban_dnum            (dungeon_topology.d_sokoban_dnum)
#define mines_dnum              (dungeon_topology.d_mines_dnum)
#define quest_dnum              (dungeon_topology.d_quest_dnum)
#define qstart_level            (dungeon_topology.d_qstart_level)
#define qlocate_level           (dungeon_topology.d_qlocate_level)
#define nemesis_level           (dungeon_topology.d_nemesis_level)
#define knox_level              (dungeon_topology.d_knox_level)
#define mineend_level           (dungeon_topology.d_mineend_level)
#define sokoend_level           (dungeon_topology.d_sokoend_level)
/* clang-format on */

E NEARDATA stairway dnstair, upstair; /* stairs up and down */
#define xdnstair (dnstair.sx)
#define ydnstair (dnstair.sy)
#define xupstair (upstair.sx)
#define yupstair (upstair.sy)

E NEARDATA stairway dnladder, upladder; /* ladders up and down */
#define xdnladder (dnladder.sx)
#define ydnladder (dnladder.sy)
#define xupladder (upladder.sx)
#define yupladder (upladder.sy)

E NEARDATA stairway sstairs;

E NEARDATA dest_area updest, dndest; /* level-change destination areas */

E NEARDATA coord inv_pos;
E NEARDATA dungeon dungeons[];
E NEARDATA s_level *sp_levchn;
#define dunlev_reached(x) (dungeons[(x)->dnum].dunlev_ureached)

#include "quest.h"
E struct q_score quest_status;

E NEARDATA char pl_character[PL_CSIZ];
E NEARDATA char pl_race; /* character's race */

E NEARDATA char pl_fruit[PL_FSIZ];
E NEARDATA struct fruit *ffruit;

E NEARDATA char tune[6];

#define MAXLINFO (MAXDUNGEON * MAXLEVEL)
E struct linfo level_info[MAXLINFO];

E NEARDATA struct sinfo {
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
#ifdef PANICLOG
    int in_paniclog;
#endif
    int wizkit_wishing;
} program_state;

E boolean restoring;
E boolean ransacked;

E const char quitchars[];
E const char vowels[];
E const char ynchars[];
E const char ynqchars[];
E const char ynaqchars[];
E const char ynNaqchars[];
E NEARDATA long yn_number;

E const char disclosure_options[];

E NEARDATA int smeq[];
E NEARDATA int doorindex;
E NEARDATA char *save_cm;

E NEARDATA struct kinfo {
    struct kinfo *next; /* chain of delayed killers */
    int id;             /* uprop keys to ID a delayed killer */
    int format;         /* one of the killer formats */
#define KILLED_BY_AN 0
#define KILLED_BY 1
#define NO_KILLER_PREFIX 2
    char name[BUFSZ]; /* actual killer name */
} killer;

E long done_money;
E NEARDATA char plname[PL_NSIZ];
E NEARDATA char dogname[];
E NEARDATA char catname[];
E NEARDATA char horsename[];
E char preferred_pet;

E const char *occtxt; /* defined when occupation != NULL */
E const char *nomovemsg;
E char lock[];

E const schar xdir[], ydir[], zdir[];

E NEARDATA schar tbx, tby; /* set in mthrowu.c */

E NEARDATA struct multishot {
    int n, i;
    short o;
    boolean s;
} m_shot;

E NEARDATA long moves, monstermoves;
E NEARDATA long wailmsg;

E NEARDATA boolean in_mklev;
E NEARDATA boolean stoned;
E NEARDATA boolean unweapon;
E NEARDATA boolean mrg_to_wielded;
E NEARDATA boolean defer_see_monsters;

E NEARDATA boolean in_steed_dismounting;

E const int shield_static[];

#include "spell.h"
E NEARDATA struct spell spl_book[]; /* sized in decl.c */

#include "color.h"
#ifdef TEXTCOLOR
E const int zapcolors[];
#endif

E const struct class_sym def_oc_syms[MAXOCLASSES]; /* default class symbols */
E uchar oc_syms[MAXOCLASSES];                      /* current class symbols */
E const struct class_sym def_monsyms[MAXMCLASSES]; /* default class symbols */
E uchar monsyms[MAXMCLASSES];                      /* current class symbols */

#include "obj.h"
E NEARDATA struct obj *invent, *uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
    *uarmu, /* under-wear, so to speak */
    *uskin, *uamul, *uleft, *uright, *ublindf, *uwep, *uswapwep, *uquiver;

E NEARDATA struct obj *uchain; /* defined only when punished */
E NEARDATA struct obj *uball;
E NEARDATA struct obj *migrating_objs;
E NEARDATA struct obj *billobjs;
E NEARDATA struct obj *current_wand, *thrownobj, *kickedobj;

E NEARDATA struct obj zeroobj; /* for init; &zeroobj used as special value */

E NEARDATA anything zeroany;   /* init'd and defined in decl.c */

#include "you.h"
E NEARDATA struct you u;
E NEARDATA time_t ubirthday;
E NEARDATA struct u_realtime urealtime;

#include "onames.h"
#ifndef PM_H /* (pm.h has already been included via youprop.h) */
#include "pm.h"
#endif

E NEARDATA struct monst zeromonst; /* for init of new or temp monsters */
E NEARDATA struct monst youmonst; /* monster details when hero is poly'd */
E NEARDATA struct monst *mydogs, *migrating_mons;

E NEARDATA struct mvitals {
    uchar born;
    uchar died;
    uchar mvflags;
} mvitals[NUMMONS];

E NEARDATA struct c_color_names {
    const char *const c_black, *const c_amber, *const c_golden,
        *const c_light_blue, *const c_red, *const c_green, *const c_silver,
        *const c_blue, *const c_purple, *const c_white, *const c_orange;
} c_color_names;
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

E struct c_common_strings {
    const char *const c_nothing_happens, *const c_thats_enough_tries,
        *const c_silly_thing_to, *const c_shudder_for_moment,
        *const c_something, *const c_Something, *const c_You_can_move_again,
        *const c_Never_mind, *c_vision_clears, *const c_the_your[2];
} c_common_strings;
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

/* material strings */
E const char *materialnm[];

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

/* Vision */
E NEARDATA boolean vision_full_recalc; /* TRUE if need vision recalc */
E NEARDATA char **viz_array;           /* could see/in sight row pointers */

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

E char toplines[];
#ifndef TCAP_H
E struct tc_gbl_data {   /* also declared in tcap.h */
    char *tc_AS, *tc_AE; /* graphics start and end (tty font swapping) */
    int tc_LI, tc_CO;    /* lines and columns */
} tc_gbl_data;
#define AS tc_gbl_data.tc_AS
#define AE tc_gbl_data.tc_AE
#define LI tc_gbl_data.tc_LI
#define CO tc_gbl_data.tc_CO
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

E char *fqn_prefix[PREFIX_COUNT];
#ifdef PREFIXES_IN_USE
E char *fqn_prefix_names[PREFIX_COUNT];
#endif

E NEARDATA struct savefile_info sfcap, sfrestinfo, sfsaveinfo;

struct opvar {
    xchar spovartyp; /* one of SPOVAR_foo */
    union {
        char *str;
        long l;
    } vardata;
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

E struct plinemsg_type *plinemsg_types;

#ifdef PANICTRACE
E const char *ARGV0;
#endif

enum earlyarg {ARG_DEBUG, ARG_VERSION
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

/* instance_globals holds engine state that does not need to be
 * persisted upon game exit.  The initialization state is well defined
 * an set in decl.c during early early engine initialization.
 *
 * unlike instance_flags, values in the structure can be of any type. */

#define BSIZE 20

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
    long bl_hilite_moves;
    unsigned long cond_hilites[BL_ATTCLR_MAX];

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

    /* dbridge.c */
    struct entity occupants[ENTITIES];

    /* dig.c */

    boolean did_dig_msg;

    /* do.c */
    boolean at_ladder;
    char *dfr_pre_msg;  /* pline() before level change */
    char *dfr_post_msg; /* pline() after level change */
    d_level save_dlevel;

    /* do_name.c */
    struct opvar *gloc_filter_map;
    int gloc_filter_floodfill_match_glyph;
    int via_naming;

    /* dog.c */
    int petname_used; /* user preferred pet name has been used */
    xchar gtyp;  /* type of dog's current goal */
    xchar gx; /* x position of dog's current goal */
    char  gy; /* y position of dog's current goal */

    /* dokick.c */
    struct rm *maploc;
    struct rm nowhere;
    const char *gate_str;

    /* drawing */
    struct symsetentry symset[NUM_GRAPHICS];
    int currentgraphics;
    nhsym showsyms[SYM_MAX]; /* symbols to be displayed */
    nhsym l_syms[SYM_MAX];   /* loaded symbols          */
    nhsym r_syms[SYM_MAX];   /* rogue symbols           */
    nhsym warnsyms[WARNCOUNT]; /* the current warning display symbols */

    /* dungeon.c */
    int n_dgns; /* number of dungeons (also used in mklev.c and do.c) */
    branch *branches; /* dungeon branch list */
    mapseen *mapseenchn; /*DUNGEON_OVERVIEW*/

    /* eat.c */
    boolean force_save_hs;

    /* end.c */
    struct valuable_data gems[LAST_GEM + 1 - FIRST_GEM + 1]; /* +1 for glass */
    struct valuable_data amulets[LAST_AMULET + 1 - FIRST_AMULET];
    struct val_list valuables[3];
    
    /* hack.c */
    anything tmp_anything;
    int wc; /* current weight_cap(); valid after call to inv_weight() */

    /* invent.c */
    int lastinvnr;  /* 0 ... 51 (never saved&restored) */
    unsigned sortlootmode; /* set by sortloot() for use by sortloot_cmp(); 
                            * reset by sortloot when done */
    char *invbuf;
    unsigned invbufsiz;

    /* lock.c */
    struct xlock_s xlock;

    /* makemon.c */
    struct {
        int choice_count;
        char mchoices[SPECIAL_PM]; /* value range is 0..127 */
    } rndmonst_state;

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

    /* mon.c */
    boolean vamp_rise_msg;
    boolean disintegested;

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

    /* objname.c */
    /* distantname used by distant_name() to pass extra information to
       xname_flags(); it would be much cleaner if this were a parameter,
       but that would require all of the xname() and doname() calls to be
       modified */
    int distantname;

    /* pickup.c */
    int oldcap; /* last encumberance */
    /* current_container is set in use_container(), to be used by the
       callback routines in_container() and out_container() from askchain()
       and use_container(). Also used by menu_loot() and container_gone(). */
    struct obj *current_container;
    boolean abort_looting;

    /* pline.c */
    unsigned pline_flags;
    char prevmsg[BUFSZ];
#ifdef DUMPLOG
    unsigned saved_pline_index;  /* slot in saved_plines[] to use next */
    char *saved_plines[DUMPLOG_MSG_COUNT];
#endif

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


    /* read.c */
    boolean known;

    /* restore.c */
    int n_ids_mapped;
    struct bucket *id_map;
    boolean restoring;
    struct fruit *oldfruit;
    long omoves;

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

    /* sp_lev.c */
    char *lev_message;
    lev_region *lregions;
    int num_lregions;

    /* trap.c */
    int force_mintrap; /* mintrap() should take a flags argument, but for time
                          being we use this */
    /* u_init.c */
    short nocreate;
    short nocreate2;
    short nocreate3;
    short nocreate4;
    /* uhitm.c */
    boolean override_confirmation; /* Used to flag attacks caused by
                                      Stormbringer's maliciousness. */
    /* weapon.c */
    struct obj *propellor;
    /* zap.c */
    int  poly_zapped;
    boolean obj_zapped;

    unsigned long magic; /* validate that structure layout is preserved */
};

E struct instance_globals g;

#undef E

#endif /* DECL_H */

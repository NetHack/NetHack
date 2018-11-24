/* NetHack 3.6	decl.c	$NHDT-Date: 1446975463 2015/11/08 09:37:43 $  $NHDT-Branch: master $:$NHDT-Revision: 1.62 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

int NDECL((*afternmv));
int NDECL((*occupation));

/* from xxxmain.c */
const char *hname = 0; /* name of the game (argv[0] of main) */
int hackpid = 0;       /* current process id */
#if defined(UNIX) || defined(VMS)
int locknum = 0; /* max num of simultaneous users */
#endif
#ifdef DEF_PAGER
char *catmore = 0; /* default pager */
#endif

NEARDATA int bases[MAXOCLASSES] = DUMMY;

NEARDATA int multi = 0;
const char *multi_reason = NULL;
NEARDATA int nroom = 0;
NEARDATA int nsubroom = 0;
NEARDATA int occtime = 0;

/* maze limits must be even; masking off lowest bit guarantees that */
int x_maze_max = (COLNO - 1) & ~1, y_maze_max = (ROWNO - 1) & ~1;

int otg_temp; /* used by object_to_glyph() [otg] */

NEARDATA int in_doagain = 0;

/*
 *      The following structure will be initialized at startup time with
 *      the level numbers of some "important" things in the game.
 */
struct dgn_topology dungeon_topology = { DUMMY };

struct q_score quest_status = DUMMY;

NEARDATA int warn_obj_cnt = 0;
NEARDATA int smeq[MAXNROFROOMS + 1] = DUMMY;
NEARDATA int doorindex = 0;
NEARDATA char *save_cm = 0;

NEARDATA struct kinfo killer = DUMMY;
NEARDATA long done_money = 0;
const char *nomovemsg = 0;
NEARDATA char plname[PL_NSIZ] = DUMMY; /* player name */
NEARDATA char pl_character[PL_CSIZ] = DUMMY;
NEARDATA char pl_race = '\0';

NEARDATA char pl_fruit[PL_FSIZ] = DUMMY;
NEARDATA struct fruit *ffruit = (struct fruit *) 0;

NEARDATA char tune[6] = DUMMY;
NEARDATA boolean ransacked = 0;

const char *occtxt = DUMMY;
const char quitchars[] = " \r\n\033";
const char vowels[] = "aeiouAEIOU";
const char ynchars[] = "yn";
const char ynqchars[] = "ynq";
const char ynaqchars[] = "ynaq";
const char ynNaqchars[] = "yn#aq";
NEARDATA long yn_number = 0L;

const char disclosure_options[] = "iavgco";

#if defined(MICRO) || defined(WIN32)
char hackdir[PATHLEN]; /* where rumors, help, record are */
#ifdef MICRO
char levels[PATHLEN]; /* where levels are */
#endif
#endif /* MICRO || WIN32 */

#ifdef MFLOPPY
char permbones[PATHLEN]; /* where permanent copy of bones go */
int ramdisk = FALSE;     /* whether to copy bones to levels or not */
int saveprompt = TRUE;
const char *alllevels = "levels.*";
const char *allbones = "bones*.*";
#endif

struct linfo level_info[MAXLINFO];

NEARDATA struct sinfo program_state;

/* x/y/z deltas for the 10 movement directions (8 compass pts, 2 up/down) */
const schar xdir[10] = { -1, -1, 0, 1, 1, 1, 0, -1, 0, 0 };
const schar ydir[10] = { 0, -1, -1, -1, 0, 1, 1, 1, 0, 0 };
const schar zdir[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 };

NEARDATA schar tbx = 0, tby = 0; /* mthrowu: target */

/* for xname handling of multiple shot missile volleys:
   number of shots, index of current one, validity check, shoot vs throw */
NEARDATA struct multishot m_shot = { 0, 0, STRANGE_OBJECT, FALSE };

NEARDATA dungeon dungeons[MAXDUNGEON]; /* ini'ed by init_dungeon() */
NEARDATA s_level *sp_levchn;
NEARDATA stairway upstair = { 0, 0, { 0, 0 }, 0 },
                  dnstair = { 0, 0, { 0, 0 }, 0 };
NEARDATA stairway upladder = { 0, 0, { 0, 0 }, 0 },
                  dnladder = { 0, 0, { 0, 0 }, 0 };
NEARDATA stairway sstairs = { 0, 0, { 0, 0 }, 0 };
NEARDATA dest_area updest = { 0, 0, 0, 0, 0, 0, 0, 0 };
NEARDATA dest_area dndest = { 0, 0, 0, 0, 0, 0, 0, 0 };
NEARDATA coord inv_pos = { 0, 0 };

NEARDATA boolean defer_see_monsters = FALSE;
NEARDATA boolean in_mklev = FALSE;
NEARDATA boolean stoned = FALSE; /* done to monsters hit by 'c' */
NEARDATA boolean unweapon = FALSE;
NEARDATA boolean mrg_to_wielded = FALSE;
/* weapon picked is merged with wielded one */

NEARDATA boolean in_steed_dismounting = FALSE;

NEARDATA coord bhitpos = DUMMY;
NEARDATA coord doors[DOORMAX] = { DUMMY };

NEARDATA struct mkroom rooms[(MAXNROFROOMS + 1) * 2] = { DUMMY };
NEARDATA struct mkroom *subrooms = &rooms[MAXNROFROOMS + 1];
struct mkroom *upstairs_room, *dnstairs_room, *sstairs_room;

dlevel_t level; /* level map */
struct trap *ftrap = (struct trap *) 0;
NEARDATA struct monst youmonst = DUMMY;
NEARDATA struct context_info context = DUMMY;
NEARDATA struct flag flags = DUMMY;
#ifdef SYSFLAGS
NEARDATA struct sysflag sysflags = DUMMY;
#endif
NEARDATA struct instance_flags iflags = DUMMY;
NEARDATA struct you u = DUMMY;
NEARDATA time_t ubirthday = DUMMY;
NEARDATA struct u_realtime urealtime = DUMMY;

schar lastseentyp[COLNO][ROWNO] = {
    DUMMY
}; /* last seen/touched dungeon typ */

NEARDATA struct obj
    *invent = (struct obj *) 0,
    *uwep = (struct obj *) 0, *uarm = (struct obj *) 0,
    *uswapwep = (struct obj *) 0,
    *uquiver = (struct obj *) 0,       /* quiver */
        *uarmu = (struct obj *) 0,     /* under-wear, so to speak */
            *uskin = (struct obj *) 0, /* dragon armor, if a dragon */
                *uarmc = (struct obj *) 0, *uarmh = (struct obj *) 0,
    *uarms = (struct obj *) 0, *uarmg = (struct obj *) 0,
    *uarmf = (struct obj *) 0, *uamul = (struct obj *) 0,
    *uright = (struct obj *) 0, *uleft = (struct obj *) 0,
    *ublindf = (struct obj *) 0, *uchain = (struct obj *) 0,
    *uball = (struct obj *) 0;
/* some objects need special handling during destruction or placement */
NEARDATA struct obj
    *current_wand = 0,  /* wand currently zapped/applied */
    *thrownobj = 0,     /* object in flight due to throwing */
    *kickedobj = 0;     /* object in flight due to kicking */

#ifdef TEXTCOLOR
/*
 *  This must be the same order as used for buzz() in zap.c.
 */
const int zapcolors[NUM_ZAP] = {
    HI_ZAP,     /* 0 - missile */
    CLR_ORANGE, /* 1 - fire */
    CLR_WHITE,  /* 2 - frost */
    HI_ZAP,     /* 3 - sleep */
    CLR_BLACK,  /* 4 - death */
    CLR_WHITE,  /* 5 - lightning */
    CLR_YELLOW, /* 6 - poison gas */
    CLR_GREEN,  /* 7 - acid */
};
#endif /* text color */

const int shield_static[SHIELD_COUNT] = {
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4, /* 7 per row */
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
};

NEARDATA struct spell spl_book[MAXSPELL + 1] = { DUMMY };

NEARDATA long moves = 1L, monstermoves = 1L;
/* These diverge when player is Fast */
NEARDATA long wailmsg = 0L;

/* objects that are moving to another dungeon level */
NEARDATA struct obj *migrating_objs = (struct obj *) 0;
/* objects not yet paid for */
NEARDATA struct obj *billobjs = (struct obj *) 0;

/* used to zero all elements of a struct obj and a struct monst */
NEARDATA struct obj zeroobj = DUMMY;
NEARDATA struct monst zeromonst = DUMMY;
/* used to zero out union any; initializer deliberately omitted */
NEARDATA anything zeroany;

/* originally from dog.c */
NEARDATA char dogname[PL_PSIZ] = DUMMY;
NEARDATA char catname[PL_PSIZ] = DUMMY;
NEARDATA char horsename[PL_PSIZ] = DUMMY;
char preferred_pet; /* '\0', 'c', 'd', 'n' (none) */
int petname_used = 0;
/* monsters that went down/up together with @ */
NEARDATA struct monst *mydogs = (struct monst *) 0;
/* monsters that are moving to another dungeon level */
NEARDATA struct monst *migrating_mons = (struct monst *) 0;

NEARDATA struct mvitals mvitals[NUMMONS];

/* originally from pickup.c */
int oldcap = 0; /* encumbrance */

NEARDATA struct c_color_names c_color_names = {
    "black",  "amber", "golden", "light blue", "red",   "green",
    "silver", "blue",  "purple", "white",      "orange"
};

struct menucoloring *menu_colorings = NULL;

const char *c_obj_colors[] = {
    "black",          /* CLR_BLACK */
    "red",            /* CLR_RED */
    "green",          /* CLR_GREEN */
    "brown",          /* CLR_BROWN */
    "blue",           /* CLR_BLUE */
    "magenta",        /* CLR_MAGENTA */
    "cyan",           /* CLR_CYAN */
    "gray",           /* CLR_GRAY */
    "transparent",    /* no_color */
    "orange",         /* CLR_ORANGE */
    "bright green",   /* CLR_BRIGHT_GREEN */
    "yellow",         /* CLR_YELLOW */
    "bright blue",    /* CLR_BRIGHT_BLUE */
    "bright magenta", /* CLR_BRIGHT_MAGENTA */
    "bright cyan",    /* CLR_BRIGHT_CYAN */
    "white",          /* CLR_WHITE */
};

struct c_common_strings c_common_strings = { "Nothing happens.",
                                             "That's enough tries!",
                                             "That is a silly thing to %s.",
                                             "shudder for a moment.",
                                             "something",
                                             "Something",
                                             "You can move again.",
                                             "Never mind.",
                                             "vision quickly clears.",
                                             { "the", "your" } };

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
const char *materialnm[] = { "mysterious", "liquid",  "wax",        "organic",
                             "flesh",      "paper",   "cloth",      "leather",
                             "wooden",     "bone",    "dragonhide", "iron",
                             "metal",      "copper",  "silver",     "gold",
                             "platinum",   "mithril", "plastic",    "glass",
                             "gemstone",   "stone" };

/* Vision */
NEARDATA boolean vision_full_recalc = 0;
NEARDATA char **viz_array = 0; /* used in cansee() and couldsee() macros */

/* Global windowing data, defined here for multi-window-system support */
NEARDATA winid WIN_MESSAGE = WIN_ERR;
NEARDATA winid WIN_STATUS = WIN_ERR;
NEARDATA winid WIN_MAP = WIN_ERR, WIN_INVEN = WIN_ERR;
char toplines[TBUFSZ];
/* Windowing stuff that's really tty oriented, but present for all ports */
struct tc_gbl_data tc_gbl_data = { 0, 0, 0, 0 }; /* AS,AE, LI,CO */

char *fqn_prefix[PREFIX_COUNT] = { (char *) 0, (char *) 0, (char *) 0,
                                   (char *) 0, (char *) 0, (char *) 0,
                                   (char *) 0, (char *) 0, (char *) 0,
                                   (char *) 0 };

#ifdef PREFIXES_IN_USE
char *fqn_prefix_names[PREFIX_COUNT] = {
    "hackdir",  "leveldir", "savedir",    "bonesdir",  "datadir",
    "scoredir", "lockdir",  "sysconfdir", "configdir", "troubledir"
};
#endif

#ifdef PLAYAGAIN
const struct savefile_info default_sfinfo = {
#ifdef NHSTDC
    0x00000000UL
#else
    0x00000000L
#endif
#if defined(COMPRESS) || defined(ZLIB_COMP)
        | SFI1_EXTERNALCOMP
#endif
#if defined(ZEROCOMP)
        | SFI1_ZEROCOMP
#endif
#if defined(RLECOMP)
        | SFI1_RLECOMP
#endif
    ,
#ifdef NHSTDC
    0x00000000UL, 0x00000000UL
#else
    0x00000000L, 0x00000000L
#endif
};
#endif

NEARDATA struct savefile_info sfcap = {
#ifdef NHSTDC
    0x00000000UL
#else
    0x00000000L
#endif
#if defined(COMPRESS) || defined(ZLIB_COMP)
        | SFI1_EXTERNALCOMP
#endif
#if defined(ZEROCOMP)
        | SFI1_ZEROCOMP
#endif
#if defined(RLECOMP)
        | SFI1_RLECOMP
#endif
    ,
#ifdef NHSTDC
    0x00000000UL, 0x00000000UL
#else
    0x00000000L, 0x00000000L
#endif
};

NEARDATA struct savefile_info sfrestinfo, sfsaveinfo = {
#ifdef NHSTDC
    0x00000000UL
#else
    0x00000000L
#endif
#if defined(COMPRESS) || defined(ZLIB_COMP)
        | SFI1_EXTERNALCOMP
#endif
#if defined(ZEROCOMP)
        | SFI1_ZEROCOMP
#endif
#if defined(RLECOMP)
        | SFI1_RLECOMP
#endif
    ,
#ifdef NHSTDC
    0x00000000UL, 0x00000000UL
#else
    0x00000000L, 0x00000000L
#endif
};

struct plinemsg_type *plinemsg_types = (struct plinemsg_type *) 0;

#ifdef PANICTRACE
const char *ARGV0;
#endif

/* support for lint.h */
unsigned nhUse_dummy = 0;

/* dummy routine used to force linkage */
void
decl_init()
{
    return;
}

#ifdef PLAYAGAIN

static boolean s_firstStart = TRUE;

static boolean
decl_is_block_zero(p, n)
unsigned char * p;
int n;
{
    static unsigned char zeroblock[512] = { 0 };
    unsigned char * sentinel = p + n;
    while (p < sentinel) {
        int c = (n < sizeof(zeroblock) ? n : sizeof(zeroblock));
        if (memcmp(p, zeroblock, c) != 0) return FALSE;
        p += c;
    }
    return TRUE;
}

static void
decl_zero_block(p, n)
unsigned char * p;
int n;
{
    nhassert(!s_firstStart || decl_is_block_zero(p, n));
    memset(p, 0, n);
}

#define ZEROARRAY(x) decl_zero_block((void *) &x[0], 0, sizeof(x))
#define ZEROARRAYN(x,n) decl_zero_block((void *) &x[0], 0, sizeof(x[0])*(n))
#define ZERO(x) decl_zero_block((void *) &x, 0, sizeof(x))
#define ZEROPTR(x) { nhassert(x == NULL); x = NULL; }
#define ZEROPTRNOCHECK(x) { x = NULL; }

/* decl_early_init() is called when we are starting a game.  On first
 * start, it validates that global state is in the expected state.
 * When called on subsequent starts, it initializes global state to
 * expected state.
 *
 * In the case that of global pointers, on subsequent starts it will
 * panic if it finds a non-NULL pointer with the assumption that a
 * pointer has leaked.  */

void
decl_early_init()
{
    hackpid = 0;
#if defined(UNIX) || defined(VMS)
    locknum = 0;
#endif
#ifdef DEF_PAGER
    catmore = 0;
#endif

    ZEROARRAY(bases);

    multi = 0;
    multi_reason = NULL;
    nroom = 0;
    nsubroom = 0;
    occtime = 0;

    x_maze_max = (COLNO - 1) & ~1;
    y_maze_max = (ROWNO - 1) & ~1;

    otg_temp = 0;

    ZERO(dungeon_topology);
    ZERO(quest_status);

    warn_obj_cnt = 0;
    ZEROARRAYN(smeq, MAXNROFROOMS + 1);
    doorindex = 0;
    save_cm = NULL;

    ZERO(killer);
    done_money = 0;
    nomovemsg = NULL;
    ZEROARRAY(plname);
    ZEROARRAY(pl_character);
    pl_race = '\0';

    ZEROARRAY(pl_fruit);
    ffruit = NULL;

    ZEROARRAY(tune);

    occtxt = NULL;

    yn_number = 0;

#if defined(MICRO) || defined(WIN32)
    ZEROARRAYN(hackdir, PATHLEN);
#ifdef MICRO
    ZEROARRAYN(levels, PATHLEN);
#endif /* MICRO */
#endif /* MICRO || WIN32 */

#ifdef MFLOPPY
    ZEROARRAYN(permbones, PATHLEN);
    ramdisk = FALSE;
    saveprompt = TRUE;
#endif

    ZEROARRAY(level_info);

    ZERO(program_state);

    tbx = 0;
    tby = 0;

    ZERO(m_shot);

    ZEROARRAYN(dungeons, MAXDUNGEON);
    ZEROPTR(sp_levchn);
    ZERO(upstair);
    ZERO(dnstair);
    ZERO(upladder);
    ZERO(dnladder);
    ZERO(sstairs);
    ZERO(updest);
    ZERO(dndest);
    ZERO(inv_pos);

    defer_see_monsters = FALSE;
    in_mklev = FALSE;
    stoned = FALSE;
    unweapon = FALSE;
    mrg_to_wielded = FALSE;

    in_steed_dismounting = FALSE;

    ZERO(bhitpos);
    ZEROARRAY(doors);

    ZEROARRAY(rooms);
    subrooms = &rooms[MAXNROFROOMS + 1];
    upstairs_room = NULL;
    dnstairs_room = NULL;
    sstairs_room = NULL;

    ZERO(level);
    ZEROPTR(ftrap);
    ZERO(youmonst);
    ZERO(context);
    ZERO(flags);
#ifdef SYSFLAGS
    ZERO(sysflags);
#endif
    ZERO(iflags);
    ZERO(u);
    ZERO(ubirthday);
    ZERO(urealtime);

    ZERO(oldcap);

    ZEROARRAY(lastseentyp);

    ZEROPTR(invent);
    ZEROPTRNOCHECK(uwep);
    ZEROPTRNOCHECK(uarm);
    ZEROPTRNOCHECK(uswapwep);
    ZEROPTRNOCHECK(uquiver);
    ZEROPTRNOCHECK(uarmu);
    ZEROPTRNOCHECK(uskin);
    ZEROPTRNOCHECK(uarmc);
    ZEROPTRNOCHECK(uarmh);
    ZEROPTRNOCHECK(uarms);
    ZEROPTRNOCHECK(uarmg);
    ZEROPTRNOCHECK(uarmf);
    ZEROPTRNOCHECK(uamul);
    ZEROPTRNOCHECK(uright);
    ZEROPTRNOCHECK(uleft);
    ZEROPTRNOCHECK(ublindf);
    ZEROPTRNOCHECK(uchain);
    ZEROPTRNOCHECK(uball);

    ZEROPTR(current_wand);
    ZEROPTR(thrownobj);
    ZEROPTR(kickedobj);

    ZEROARRAYN(spl_book, MAXSPELL + 1);

    moves = 1;
    monstermoves = 1;

    wailmsg = 0L;

    ZEROPTR(migrating_objs);
    ZEROPTR(billobjs);

    ZERO(zeroobj);
    ZERO(zeromonst);
    ZERO(zeroany);

    ZEROARRAYN(dogname, PL_PSIZ);
    ZEROARRAYN(catname, PL_PSIZ);
    ZEROARRAYN(horsename, PL_PSIZ);
    ZERO(preferred_pet);
    ZERO(petname_used);
    ZEROPTR(mydogs);
    ZEROPTR(migrating_mons);

    ZEROARRAY(mvitals);

    ZEROPTR(menu_colorings);

    vision_full_recalc = 0;
    viz_array = NULL;

    WIN_MESSAGE = WIN_ERR;
#ifndef STATUS_VIA_WINDOWPORT
    WIN_STATUS = WIN_ERR;
#endif
    WIN_MAP = WIN_ERR;
    WIN_INVEN = WIN_ERR;

    ZEROARRAYN(toplines, TBUFSZ);
    ZERO(tc_gbl_data);
    ZEROARRAYN(fqn_prefix, PREFIX_COUNT);

    sfcap = default_sfinfo;
    sfrestinfo = default_sfinfo;
    sfsaveinfo = default_sfinfo;

    ZEROPTR(plinemsg_types);

#ifdef PANICTRACE
    ARGV0 = NULL;
#endif

    nhUse_dummy = 0;

    s_firstStart = FALSE;
}
#endif

/*decl.c*/

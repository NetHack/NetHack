/* NetHack 3.6	hack.h	$NHDT-Date: 1580600495 2020/02/01 23:41:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.128 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef HACK_H
#define HACK_H

#ifndef CONFIG_H
#include "config.h"
#endif
#include "lint.h"

#define TELL 1
#define NOTELL 0
#define ON 1
#define OFF 0
#define BOLT_LIM 8        /* from this distance ranged attacks will be made */
#define MAX_CARR_CAP 1000 /* so that boulders can be heavier */
#define DUMMY { 0 }       /* array initializer, letting [1..N-1] default */
#define DEF_NOTHING ' '   /* default symbol for NOTHING and UNEXPLORED  */

/* The UNDEFINED macros are used to initialize variables whose
   initialized value is not relied upon.
   UNDEFINED_VALUE: used to initialize any scalar type except pointers.
   UNDEFINED_VALUES: used to initialize any non scalar type without pointers.
   UNDEFINED_PTR: can be used only on pointer types. */
#define UNDEFINED_VALUE 0
#define UNDEFINED_VALUES { 0 }
#define UNDEFINED_PTR NULL

/* symbolic names for capacity levels */
enum encumbrance_types {
    UNENCUMBERED = 0,
    SLT_ENCUMBER = 1, /* Burdened */
    MOD_ENCUMBER = 2, /* Stressed */
    HVY_ENCUMBER = 3, /* Strained */
    EXT_ENCUMBER = 4, /* Overtaxed */
    OVERLOADED   = 5  /* Overloaded */
};

/* weight increment of heavy iron ball */
#define IRON_BALL_W_INCR 160

/* number of turns it takes for vault guard to show up */
#define VAULT_GUARD_TIME 30

#define SHOP_DOOR_COST 400L /* cost of a destroyed shop door */
#define SHOP_BARS_COST 300L /* cost of iron bars */
#define SHOP_HOLE_COST 200L /* cost of making hole/trapdoor */
#define SHOP_WALL_COST 200L /* cost of destroying a wall */
#define SHOP_WALL_DMG  (10L * ACURRSTR) /* damaging a wall */

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

/* Macros for how a rumor was delivered in outrumor() */
#define BY_ORACLE 0
#define BY_COOKIE 1
#define BY_PAPER 2
#define BY_OTHER 9

/* Macros for why you are no longer riding */
enum dismount_types {
    DISMOUNT_GENERIC  = 0,
    DISMOUNT_FELL     = 1,
    DISMOUNT_THROWN   = 2,
    DISMOUNT_POLY     = 3,
    DISMOUNT_ENGULFED = 4,
    DISMOUNT_BONES    = 5,
    DISMOUNT_BYCHOICE = 6
};

/* mgflags for mapglyph() */
#define MG_FLAG_NORMAL     0x00
#define MG_FLAG_NOOVERRIDE 0x01

/* Special returns from mapglyph() */
#define MG_CORPSE  0x0001
#define MG_INVIS   0x0002
#define MG_DETECT  0x0004
#define MG_PET     0x0008
#define MG_RIDDEN  0x0010
#define MG_STATUE  0x0020
#define MG_OBJPILE 0x0040  /* more than one stack of objects */
#define MG_BW_LAVA 0x0080  /* 'black & white lava': highlight lava if it
                              can't be distringuished from water by color */
#define MG_BW_ICE  0x0100  /* similar for ice vs floor */
#define MG_NOTHING 0x0200  /* char represents GLYPH_NOTHING */
#define MG_UNEXPL  0x0400  /* char represents GLYPH_UNEXPLORED */

/* sellobj_state() states */
#define SELL_NORMAL (0)
#define SELL_DELIBERATE (1)
#define SELL_DONTSELL (2)

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
    COST_CORRODE = 18 /* acid damage */
};

/* bitmask flags for corpse_xname();
   PFX_THE takes precedence over ARTICLE, NO_PFX takes precedence over both */
#define CXN_NORMAL 0    /* no special handling */
#define CXN_SINGULAR 1  /* override quantity if greather than 1 */
#define CXN_NO_PFX 2    /* suppress "the" from "the Unique Monst */
#define CXN_PFX_THE 4   /* prefix with "the " (unless pname) */
#define CXN_ARTICLE 8   /* include a/an/the prefix */
#define CXN_NOCORPSE 16 /* suppress " corpse" suffix */

/* flags for look_here() */
#define LOOKHERE_PICKED_SOME   1
#define LOOKHERE_SKIP_DFEATURE 2

/* getpos() return values */
enum getpos_retval {
    LOOK_TRADITIONAL = 0, /* '.' -- ask about "more info?" */
    LOOK_QUICK       = 1, /* ',' -- skip "more info?" */
    LOOK_ONCE        = 2, /* ';' -- skip and stop looping */
    LOOK_VERBOSE     = 3  /* ':' -- show more info w/o asking */
};

/*
 * This is the way the game ends.  If these are rearranged, the arrays
 * in end.c and topten.c will need to be changed.  Some parts of the
 * code assume that PANIC separates the deaths from the non-deaths.
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

typedef struct strbuf {
    int    len;
    char * str;
    char   buf[256];
} strbuf_t;

/* str_or_len from sp_lev.h */
typedef union str_or_len {
    char *str;
    int len;
} Str_or_Len;

/* values for rtype are defined in dungeon.h */
/* lev_region from sp_lev.h */
typedef struct {
    struct {
        xchar x1, y1, x2, y2;
    } inarea;
    struct {
        xchar x1, y1, x2, y2;
    } delarea;
    boolean in_islev, del_islev;
    xchar rtype, padding;
    Str_or_Len rname;
} lev_region;


#include "align.h"
#include "dungeon.h"
#include "monsym.h"
#include "mkroom.h"
#include "objclass.h"
#include "youprop.h"
#include "wintype.h"
#include "context.h"
#include "rm.h"
#include "botl.h"

/* Symbol offsets */
#define SYM_OFF_P (0)
#define SYM_OFF_O (SYM_OFF_P + MAXPCHARS)   /* MAXPCHARS from rm.h */
#define SYM_OFF_M (SYM_OFF_O + MAXOCLASSES) /* MAXOCLASSES from objclass.h */
#define SYM_OFF_W (SYM_OFF_M + MAXMCLASSES) /* MAXMCLASSES from monsym.h*/
#define SYM_OFF_X (SYM_OFF_W + WARNCOUNT)
#define SYM_MAX (SYM_OFF_X + MAXOTHER)

#include "rect.h"
#include "region.h"
#include "decl.h"
#include "timeout.h"

/* types of calls to bhit() */
enum bhit_call_types {
    ZAPPED_WAND   = 0,
    THROWN_WEAPON = 1,
    THROWN_TETHERED_WEAPON = 2,
    KICKED_WEAPON = 3,
    FLASHED_LIGHT = 4,
    INVIS_BEAM    = 5
};

/* attack mode for hmon() */
enum hmon_atkmode_types {
    HMON_MELEE   = 0, /* hand-to-hand */
    HMON_THROWN  = 1, /* normal ranged (or spitting while poly'd) */
    HMON_KICKED  = 2, /* alternate ranged */
    HMON_APPLIED = 3, /* polearm, treated as ranged */
    HMON_DRAGGED = 4  /* attached iron ball, pulled into mon */
};

enum saveformats {
    invalid = 0,
    historical = 1,     /* entire struct, binary, as-is */
    lendian = 2,        /* each field, binary, little-endian */
    ascii = 3           /* each field, ascii text (just proof of concept) */
};

/* sortloot() return type; needed before extern.h */
struct sortloot_item {
    struct obj *obj;
    char *str; /* result of loot_xname(obj) in some cases, otherwise null */
    int indx; /* signed int, because sortloot()'s qsort comparison routine
                 assumes (a->indx - b->indx) might yield a negative result */
    xchar orderclass; /* order rather than object class; 0 => not yet init'd */
    xchar subclass; /* subclass for some classes */
    xchar disco; /* discovery status */
};
typedef struct sortloot_item Loot;

#define MATCH_WARN_OF_MON(mon) \
    (Warn_of_mon                                                        \
     && ((g.context.warntype.obj & (mon)->data->mflags2) != 0           \
         || (g.context.warntype.polyd & (mon)->data->mflags2) != 0      \
         || (g.context.warntype.species                                 \
             && (g.context.warntype.species == (mon)->data))))

#include "trap.h"
#include "flag.h"
#include "vision.h"
#include "display.h"
#include "engrave.h"

#ifdef USE_TRAMPOLI /* this doesn't belong here, but we have little choice */
#undef NDECL
#define NDECL(f) f()
#endif

#include "extern.h"
#include "winprocs.h"
#include "sys.h"

#ifdef USE_TRAMPOLI
#include "wintty.h"
#undef WINTTY_H
#include "trampoli.h"
#undef EXTERN_H
#include "extern.h"
#endif /* USE_TRAMPOLI */

/* flags to control makemon(); goodpos() uses some plus has some of its own*/
#define NO_MM_FLAGS 0x000000L /* use this rather than plain 0 */
#define NO_MINVENT  0x000001L /* suppress minvent when creating mon */
#define MM_NOWAIT   0x000002L /* don't set STRAT_WAITMASK flags */
#define MM_NOCOUNTBIRTH 0x0004L /* don't increment born count (for revival) */
#define MM_IGNOREWATER  0x0008L /* ignore water when positioning */
#define MM_ADJACENTOK   0x0010L /* acceptable to use adjacent coordinates */
#define MM_ANGRY    0x000020L /* monster is created angry */
#define MM_NONAME   0x000040L /* monster is not christened */
#define MM_EGD      0x000100L /* add egd structure */
#define MM_EPRI     0x000200L /* add epri structure */
#define MM_ESHK     0x000400L /* add eshk structure */
#define MM_EMIN     0x000800L /* add emin structure */
#define MM_EDOG     0x001000L /* add edog structure */
#define MM_ASLEEP   0x002000L /* monsters should be generated asleep */
#define MM_NOGRP    0x004000L /* suppress creation of monster groups */
/* if more MM_ flag masks are added, skip or renumber the GP_ one(s) */
#define GP_ALLOW_XY 0x008000L /* [actually used by enexto() to decide whether
                               * to make an extra call to goodpos()]        */
#define GP_ALLOW_U  0x010000L /* don't reject hero's location */

/* flags for make_corpse() and mkcorpstat() */
#define CORPSTAT_NONE 0x00
#define CORPSTAT_INIT 0x01   /* pass init flag to mkcorpstat */
#define CORPSTAT_BURIED 0x02 /* bury the corpse or statue */

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
#define BY_NEXTHERE     0x01   /* follow objlist by nexthere field */
#define AUTOSELECT_SINGLE 0x02 /* if only 1 object, don't ask */
#define USE_INVLET      0x04   /* use object's invlet */
#define INVORDER_SORT   0x08   /* sort objects by packorder */
#define SIGNAL_NOMENU   0x10   /* return -1 rather than 0 if none allowed */
#define SIGNAL_ESCAPE   0x20   /* return -2 rather than 0 for ESC */
#define FEEL_COCKATRICE 0x40   /* engage cockatrice checks and react */
#define INCLUDE_HERO    0x80   /* show hero among engulfer's inventory */

/* Flags to control query_category() */
/* BY_NEXTHERE used by query_category() too, so skip 0x01 */
#define UNPAID_TYPES 0x002
#define GOLD_TYPES   0x004
#define WORN_TYPES   0x008
#define ALL_TYPES    0x010
#define BILLED_TYPES 0x020
#define CHOOSE_ALL   0x040
#define BUC_BLESSED  0x080
#define BUC_CURSED   0x100
#define BUC_UNCURSED 0x200
#define BUC_UNKNOWN  0x400
#define BUC_ALLBKNOWN (BUC_BLESSED | BUC_CURSED | BUC_UNCURSED)
#define BUCX_TYPES (BUC_ALLBKNOWN | BUC_UNKNOWN)
#define ALL_TYPES_SELECTED -2

/* Flags to control find_mid() */
#define FM_FMON 0x01    /* search the fmon chain */
#define FM_MIGRATE 0x02 /* search the migrating monster chain */
#define FM_MYDOGS 0x04  /* search g.mydogs */
#define FM_EVERYWHERE (FM_FMON | FM_MIGRATE | FM_MYDOGS)

/* Flags to control pick_[race,role,gend,align] routines in role.c */
#define PICK_RANDOM 0
#define PICK_RIGID 1

/* Flags to control dotrap() in trap.c */
#define FORCETRAP 0x01     /* triggering not left to chance */
#define NOWEBMSG 0x02      /* suppress stumble into web message */
#define FORCEBUNGLE 0x04   /* adjustments appropriate for bungling */
#define RECURSIVETRAP 0x08 /* trap changed into another type this same turn */
#define TOOKPLUNGE 0x10    /* used '>' to enter pit below you */
#define VIASITTING 0x20    /* #sit while at trap location (affects message) */
#define FAILEDUNTRAP 0x40  /* trap activated by failed untrap attempt */

/* Flags to control test_move in hack.c */
#define DO_MOVE 0   /* really doing the move */
#define TEST_MOVE 1 /* test a normal move (move there next) */
#define TEST_TRAV 2 /* test a future travel location */
#define TEST_TRAP 3 /* check if a future travel loc is a trap */

/*** some utility macros ***/
#define yn(query) yn_function(query, ynchars, 'n')
#define ynq(query) yn_function(query, ynqchars, 'q')
#define ynaq(query) yn_function(query, ynaqchars, 'y')
#define nyaq(query) yn_function(query, ynaqchars, 'n')
#define nyNaq(query) yn_function(query, ynNaqchars, 'n')
#define ynNaq(query) yn_function(query, ynNaqchars, 'y')

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

/* Macros for messages referring to hands, eyes, feet, etc... */
enum bodypart_types {
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

/*
 * option setting restrictions
 */

enum optset_restrictions {
    set_in_sysconf = 0, /* system config file option only */
    set_in_config  = 1, /* config file option only */
    set_viaprog    = 2, /* may be set via extern program, not seen in game */
    set_gameview   = 3, /* may be set via extern program, displayed in game */
    set_in_game    = 4, /* may be set via extern program or set in the game */
    set_wizonly    = 5, /* may be set set in the game if wizmode */
    set_hidden     = 6  /* placeholder for prefixed entries, never show it  */
};
#define SET__IS_VALUE_VALID(s) ((s < set_in_sysconf) || (s > set_wizonly))

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

#define ARM_BONUS(obj)                      \
    (objects[(obj)->otyp].a_ac + (obj)->spe \
     - min((int) greatest_erosion(obj), objects[(obj)->otyp].a_ac))

#define makeknown(x) discover_object((x), TRUE, TRUE)
#define distu(xx, yy) dist2((int)(xx), (int)(yy), (int) u.ux, (int) u.uy)
#define onlineu(xx, yy) online2((int)(xx), (int)(yy), (int) u.ux, (int) u.uy)

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
#define CFDECLSPEC __cdecl
#else
#define CFDECLSPEC
#endif

#define DEVTEAM_EMAIL "devteam@nethack.org"
#define DEVTEAM_URL "http://www.nethack.org"

#endif /* HACK_H */

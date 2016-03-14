/* NetHack 3.6	hack.h	$NHDT-Date: 1434056948 2015/06/11 21:09:08 $  $NHDT-Branch: master $:$NHDT-Revision: 1.66 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
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
#define DUMMY \
    {         \
        0     \
    }

/* symbolic names for capacity levels */
#define UNENCUMBERED 0
#define SLT_ENCUMBER 1 /* Burdened */
#define MOD_ENCUMBER 2 /* Stressed */
#define HVY_ENCUMBER 3 /* Strained */
#define EXT_ENCUMBER 4 /* Overtaxed */
#define OVERLOADED 5   /* Overloaded */

/* hunger states - see hu_stat in eat.c */
#define SATIATED 0
#define NOT_HUNGRY 1
#define HUNGRY 2
#define WEAK 3
#define FAINTING 4
#define FAINTED 5
#define STARVED 6

/* Macros for how a rumor was delivered in outrumor() */
#define BY_ORACLE 0
#define BY_COOKIE 1
#define BY_PAPER 2
#define BY_OTHER 9

/* Macros for why you are no longer riding */
#define DISMOUNT_GENERIC 0
#define DISMOUNT_FELL 1
#define DISMOUNT_THROWN 2
#define DISMOUNT_POLY 3
#define DISMOUNT_ENGULFED 4
#define DISMOUNT_BONES 5
#define DISMOUNT_BYCHOICE 6

/* Special returns from mapglyph() */
#define MG_CORPSE 0x01
#define MG_INVIS 0x02
#define MG_DETECT 0x04
#define MG_PET 0x08
#define MG_RIDDEN 0x10
#define MG_STATUE 0x20
#define MG_OBJPILE 0x40  /* more than one stack of objects */

/* sellobj_state() states */
#define SELL_NORMAL (0)
#define SELL_DELIBERATE (1)
#define SELL_DONTSELL (2)

/* alteration types--keep in synch with costly_alteration(mkobj.c) */
#define COST_CANCEL 0   /* standard cancellation */
#define COST_DRAIN 1    /* drain life upon an object */
#define COST_UNCHRG 2   /* cursed charging */
#define COST_UNBLSS 3   /* unbless (devalues holy water) */
#define COST_UNCURS 4   /* uncurse (devalues unholy water) */
#define COST_DECHNT 5   /* disenchant weapons or armor */
#define COST_DEGRD 6    /* removal of rustproofing, dulling via engraving */
#define COST_DILUTE 7   /* potion dilution */
#define COST_ERASE 8    /* scroll or spellbook blanking */
#define COST_BURN 9     /* dipped into flaming oil */
#define COST_NUTRLZ 10  /* neutralized via unicorn horn */
#define COST_DSTROY 11  /* wand breaking (bill first, useup later) */
#define COST_SPLAT 12   /* cream pie to own face (ditto) */
#define COST_BITE 13    /* start eating food */
#define COST_OPEN 14    /* open tin */
#define COST_BRKLCK 15  /* break box/chest's lock */
#define COST_RUST 16    /* rust damage */
#define COST_ROT 17     /* rotting attack */
#define COST_CORRODE 18 /* acid damage */

/* bitmask flags for corpse_xname();
   PFX_THE takes precedence over ARTICLE, NO_PFX takes precedence over both */
#define CXN_NORMAL 0    /* no special handling */
#define CXN_SINGULAR 1  /* override quantity if greather than 1 */
#define CXN_NO_PFX 2    /* suppress "the" from "the Unique Monst */
#define CXN_PFX_THE 4   /* prefix with "the " (unless pname) */
#define CXN_ARTICLE 8   /* include a/an/the prefix */
#define CXN_NOCORPSE 16 /* suppress " corpse" suffix */

/*
 * This is the way the game ends.  If these are rearranged, the arrays
 * in end.c and topten.c will need to be changed.  Some parts of the
 * code assume that PANIC separates the deaths from the non-deaths.
 */
#define DIED 0
#define CHOKING 1
#define POISONING 2
#define STARVING 3
#define DROWNING 4
#define BURNING 5
#define DISSOLVED 6
#define CRUSHING 7
#define STONING 8
#define TURNED_SLIME 9
#define GENOCIDED 10
#define PANICKED 11
#define TRICKED 12
#define QUIT 13
#define ESCAPED 14
#define ASCENDED 15

#include "align.h"
#include "dungeon.h"
#include "monsym.h"
#include "mkroom.h"
#include "objclass.h"
#include "youprop.h"
#include "wintype.h"
#include "context.h"
#include "decl.h"
#include "timeout.h"

NEARDATA extern coord bhitpos; /* place where throw or zap hits or stops */

/* types of calls to bhit() */
#define ZAPPED_WAND 0
#define THROWN_WEAPON 1
#define KICKED_WEAPON 2
#define FLASHED_LIGHT 3
#define INVIS_BEAM 4

/* attack mode for hmon() */
#define HMON_MELEE 0   /* hand-to-hand */
#define HMON_THROWN 1  /* normal ranged (or spitting while poly'd) */
#define HMON_KICKED 2  /* alternate ranged */
#define HMON_APPLIED 3 /* polearm, treated as ranged */
#define HMON_DRAGGED 4 /* attached iron ball, pulled into mon */

#define MATCH_WARN_OF_MON(mon)                                               \
    (Warn_of_mon && ((context.warntype.obj                                   \
                      && (context.warntype.obj & (mon)->data->mflags2))      \
                     || (context.warntype.polyd                              \
                         && (context.warntype.polyd & (mon)->data->mflags2)) \
                     || (context.warntype.species                            \
                         && (context.warntype.species == (mon)->data))))

#include "trap.h"
#include "flag.h"
#include "rm.h"
#include "vision.h"
#include "display.h"
#include "engrave.h"
#include "rect.h"
#include "region.h"

/* Symbol offsets */
#define SYM_OFF_P (0)
#define SYM_OFF_O (SYM_OFF_P + MAXPCHARS)
#define SYM_OFF_M (SYM_OFF_O + MAXOCLASSES)
#define SYM_OFF_W (SYM_OFF_M + MAXMCLASSES)
#define SYM_OFF_X (SYM_OFF_W + WARNCOUNT)
#define SYM_MAX (SYM_OFF_X + MAXOTHER)

#ifdef USE_TRAMPOLI /* This doesn't belong here, but we have little choice \
                       */
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

/* flags to control makemon() */
#define NO_MM_FLAGS 0x00000 /* use this rather than plain 0 */
#define NO_MINVENT 0x00001  /* suppress minvent when creating mon */
#define MM_NOWAIT 0x00002   /* don't set STRAT_WAITMASK flags */
#define MM_NOCOUNTBIRTH \
    0x00004 /* don't increment born counter (for revival) */
#define MM_IGNOREWATER 0x00008 /* ignore water when positioning */
#define MM_ADJACENTOK \
    0x00010               /* it is acceptable to use adjacent coordinates */
#define MM_ANGRY 0x00020  /* monster is created angry */
#define MM_NONAME 0x00040 /* monster is not christened */
#define MM_EGD 0x00100    /* add egd structure */
#define MM_EPRI 0x00200   /* add epri structure */
#define MM_ESHK 0x00400   /* add eshk structure */
#define MM_EMIN 0x00800   /* add emin structure */
#define MM_EDOG 0x01000   /* add edog structure */

/* flags for make_corpse() and mkcorpstat() */
#define CORPSTAT_NONE 0x00
#define CORPSTAT_INIT 0x01   /* pass init flag to mkcorpstat */
#define CORPSTAT_BURIED 0x02 /* bury the corpse or statue */

/* flags for decide_to_shift() */
#define SHIFT_SEENMSG 0x01 /* put out a message if in sight */
#define SHIFT_MSG 0x02     /* always put out a message */

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
#define BY_NEXTHERE 0x1       /* follow objlist by nexthere field */
#define AUTOSELECT_SINGLE 0x2 /* if only 1 object, don't ask */
#define USE_INVLET 0x4        /* use object's invlet */
#define INVORDER_SORT 0x8     /* sort objects by packorder */
#define SIGNAL_NOMENU 0x10    /* return -1 rather than 0 if none allowed */
#define SIGNAL_ESCAPE 0x20    /* return -2 rather than 0 for ESC */
#define FEEL_COCKATRICE 0x40  /* engage cockatrice checks and react */
#define INCLUDE_HERO 0x80     /* show hero among engulfer's inventory */

/* Flags to control query_category() */
/* BY_NEXTHERE used by query_category() too, so skip 0x01 */
#define UNPAID_TYPES 0x02
#define GOLD_TYPES 0x04
#define WORN_TYPES 0x08
#define ALL_TYPES 0x10
#define BILLED_TYPES 0x20
#define CHOOSE_ALL 0x40
#define BUC_BLESSED 0x80
#define BUC_CURSED 0x100
#define BUC_UNCURSED 0x200
#define BUC_UNKNOWN 0x400
#define BUC_ALLBKNOWN (BUC_BLESSED | BUC_CURSED | BUC_UNCURSED)
#define ALL_TYPES_SELECTED -2

/* Flags to control find_mid() */
#define FM_FMON 0x01    /* search the fmon chain */
#define FM_MIGRATE 0x02 /* search the migrating monster chain */
#define FM_MYDOGS 0x04  /* search mydogs */
#define FM_EVERYWHERE (FM_FMON | FM_MIGRATE | FM_MYDOGS)

/* Flags to control pick_[race,role,gend,align] routines in role.c */
#define PICK_RANDOM 0
#define PICK_RIGID 1

/* Flags to control dotrap() in trap.c */
#define FORCETRAP 0x01     /* triggering not left to chance */
#define NOWEBMSG 0x02      /* suppress stumble into web message */
#define FORCEBUNGLE 0x04   /* adjustments appropriate for bungling */
#define RECURSIVETRAP 0x08 /* trap changed into another type this same turn \
                              */
#define TOOKPLUNGE 0x10    /* used '>' to enter pit below you */

/* Flags to control test_move in hack.c */
#define DO_MOVE 0   /* really doing the move */
#define TEST_MOVE 1 /* test a normal move (move there next) */
#define TEST_TRAV 2 /* test a future travel location */

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

/* Macros for explosion types */
#define EXPL_DARK 0
#define EXPL_NOXIOUS 1
#define EXPL_MUDDY 2
#define EXPL_WET 3
#define EXPL_MAGICAL 4
#define EXPL_FIERY 5
#define EXPL_FROSTY 6
#define EXPL_MAX 7

/* enlightenment control flags */
#define BASICENLIGHTENMENT 1 /* show mundane stuff */
#define MAGICENLIGHTENMENT 2 /* show intrinsics and such */
#define ENL_GAMEINPROGRESS 0
#define ENL_GAMEOVERALIVE 1 /* ascension, escape, quit, trickery */
#define ENL_GAMEOVERDEAD 2

/* Macros for messages referring to hands, eyes, feet, etc... */
#define ARM 0
#define EYE 1
#define FACE 2
#define FINGER 3
#define FINGERTIP 4
#define FOOT 5
#define HAND 6
#define HANDED 7
#define HEAD 8
#define LEG 9
#define LIGHT_HEADED 10
#define NECK 11
#define SPINE 12
#define TOE 13
#define HAIR 14
#define BLOOD 15
#define LUNG 16
#define NOSE 17
#define STOMACH 18

/* indices for some special tin types */
#define ROTTEN_TIN 0
#define HOMEMADE_TIN 1
#define SPINACH_TIN (-1)
#define RANDOM_TIN (-2)
#define HEALTHY_TIN (-3)

/* Some misc definitions */
#define POTION_OCCUPANT_CHANCE(n) (13 + 2 * (n))
#define WAND_BACKFIRE_CHANCE 100
#define BALL_IN_MON (u.uswallow && uball && uball->where == OBJ_FREE)
#define NODIAG(monnum) ((monnum) == PM_GRID_BUG)

/* Flags to control menus */
#define MENUTYPELEN sizeof("traditional ")
#define MENU_TRADITIONAL 0
#define MENU_COMBINATION 1
#define MENU_FULL 2
#define MENU_PARTIAL 3

#define MENU_SELECTED TRUE
#define MENU_UNSELECTED FALSE

/*
 * Option flags
 * Each higher number includes the characteristics of the numbers
 * below it.
 */
/* XXX This should be replaced with a bitmap. */
#define SET_IN_SYS 0   /* system config file option only */
#define SET_IN_FILE 1  /* config file option only */
#define SET_VIA_PROG 2 /* may be set via extern program, not seen in game */
#define DISP_IN_GAME 3 /* may be set via extern program, displayed in game \
                          */
#define SET_IN_GAME 4  /* may be set via extern program or set in the game */
#define SET__IS_VALUE_VALID(s) ((s < SET_IN_SYS) || (s > SET_IN_GAME))

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

#if defined(OVERLAY)
#define USE_OVLx
#define STATIC_DCL extern
#define STATIC_OVL
#define STATIC_VAR

#else /* !OVERLAY */

#define STATIC_DCL static
#define STATIC_OVL static
#define STATIC_VAR static

#endif /* OVERLAY */

/* Macro for a few items that are only static if we're not overlaid.... */
#if defined(USE_TRAMPOLI) || defined(USE_OVLx)
#define STATIC_PTR
#else
#define STATIC_PTR static
#endif

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

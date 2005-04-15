/*	SCCS Id: @(#)hack.h	3.5	2005/03/28	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef HACK_H
#define HACK_H

#ifndef CONFIG_H
#include "config.h"
#endif

/*	For debugging beta code.	*/
#ifdef BETA
#define Dpline	pline
#endif

#define TELL		1
#define NOTELL		0
#define ON		1
#define OFF		0
#define BOLT_LIM	8 /* from this distance ranged attacks will be made */
#define MAX_CARR_CAP	1000	/* so that boulders can be heavier */
#define DUMMY { 0 }

/* symbolic names for capacity levels */
#define UNENCUMBERED	0
#define SLT_ENCUMBER	1	/* Burdened */
#define MOD_ENCUMBER	2	/* Stressed */
#define HVY_ENCUMBER	3	/* Strained */
#define EXT_ENCUMBER	4	/* Overtaxed */
#define OVERLOADED	5	/* Overloaded */

/* Macros for how a rumor was delivered in outrumor() */
#define BY_ORACLE	0
#define BY_COOKIE	1
#define BY_PAPER	2
#define BY_OTHER	9

#ifdef STEED
/* Macros for why you are no longer riding */
#define DISMOUNT_GENERIC	0
#define DISMOUNT_FELL		1
#define DISMOUNT_THROWN		2
#define DISMOUNT_POLY		3
#define DISMOUNT_ENGULFED	4
#define DISMOUNT_BONES		5
#define DISMOUNT_BYCHOICE	6
#endif

/* Special returns from mapglyph() */
#define MG_CORPSE	0x01
#define MG_INVIS	0x02
#define MG_DETECT	0x04
#define MG_PET		0x08
#define MG_RIDDEN	0x10

/* sellobj_state() states */
#define SELL_NORMAL	(0)
#define SELL_DELIBERATE	(1)
#define SELL_DONTSELL	(2)

/* alteration types--keep in synch with costly_alteration(mkobj.c) */
#define COST_CANCEL  0	/* standard cancellation */
#define COST_DRAIN   1	/* drain life upon an object */
#define COST_UNCHRG  2	/* cursed charging */
#define COST_UNBLSS  3	/* unbless (devalues holy water) */
#define COST_UNCURS  4	/* uncurse (devalues unholy water) */
#define COST_DECHNT  5	/* disenchant weapons or armor */
#define COST_DEGRD   6	/* removal of rustproofing, dulling via engraving */
#define COST_DILUTE  7	/* potion dilution */
#define COST_ERASE   8	/* scroll or spellbook blanking */
#define COST_BURN    9	/* dipped into flaming oil */
#define COST_NUTRLZ 10	/* neutralized via unicorn horn */
#define COST_DSTROY 11	/* wand breaking (bill first, useup later) */
#define COST_SPLAT  12	/* cream pie to own face (ditto) */
#define COST_BITE   13	/* start eating food */
#define COST_OPEN   14	/* open tin */

/*
 * This is the way the game ends.  If these are rearranged, the arrays
 * in end.c and topten.c will need to be changed.  Some parts of the
 * code assume that PANIC separates the deaths from the non-deaths.
 */
#define DIED		 0
#define CHOKING		 1
#define POISONING	 2
#define STARVING	 3
#define DROWNING	 4
#define BURNING		 5
#define DISSOLVED	 6
#define CRUSHING	 7
#define STONING		 8
#define TURNED_SLIME	 9
#define GENOCIDED	10
#define PANICKED	11
#define TRICKED		12
#define QUIT		13
#define ESCAPED		14
#define ASCENDED	15

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

NEARDATA extern coord bhitpos;	/* place where throw or zap hits or stops */

/* types of calls to bhit() */
#define ZAPPED_WAND	0
#define THROWN_WEAPON	1
#define KICKED_WEAPON	2
#define FLASHED_LIGHT	3
#define INVIS_BEAM	4

#define MATCH_WARN_OF_MON(mon)	 (Warn_of_mon && \
				 ((context.warntype.obj && \
		   		  (context.warntype.obj & (mon)->data->mflags2)) || \
				  (context.warntype.polyd && \
		   		  (context.warntype.polyd & (mon)->data->mflags2)) || \
		   		  (context.warntype.species && \
				  (context.warntype.species == (mon)->data))))

#include "trap.h"
#include "flag.h"
#include "rm.h"
#include "vision.h"
#include "display.h"
#include "engrave.h"
#include "rect.h"
#include "region.h"

#ifdef USE_TRAMPOLI /* This doesn't belong here, but we have little choice */
#undef NDECL
#define NDECL(f) f()
#endif

#include "extern.h"
#include "winprocs.h"

#ifdef USE_TRAMPOLI
#include "wintty.h"
#undef WINTTY_H
#include "trampoli.h"
#undef EXTERN_H
#include "extern.h"
#endif /* USE_TRAMPOLI */

#define NO_SPELL	0

/* flags to control makemon() */
#define NO_MM_FLAGS	  0x00	/* use this rather than plain 0 */
#define NO_MINVENT	  0x01	/* suppress minvent when creating mon */
#define MM_NOWAIT	  0x02	/* don't set STRAT_WAITMASK flags */
#define MM_EDOG		  0x04	/* add edog structure */
#define MM_EMIN		  0x08	/* add emin structure */
#define MM_ANGRY	  0x10  /* monster is created angry */
#define MM_NONAME	  0x20  /* monster is not christened */
#define MM_NOCOUNTBIRTH	  0x40  /* don't increment born counter (for revival) */
#define MM_IGNOREWATER	  0x80	/* ignore water when positioning */
#define MM_ADJACENTOK	  0x100 /* it is acceptable to use adjacent coordinates */

/* flags for make_corpse() and mkcorpstat() */
#define CORPSTAT_NONE	  0x00
#define CORPSTAT_INIT	  0x01	/* pass init flag to mkcorpstat */
#define CORPSTAT_BURIED	  0x02	/* bury the corpse or statue */

/* flags for decide_to_shift() */
#define SHIFT_SEENMSG	  0x01	/* put out a message if in sight */
#define SHIFT_MSG	  0x02  /* always put out a message */
 
/* special mhpmax value when loading bones monster to flag as extinct or genocided */
#define DEFUNCT_MONSTER	(-100)

/* macro form of adjustments of physical damage based on Half_physical_damage.
 * Can be used on-the-fly with the 1st parameter to losehp() if you don't
 * need to retain the dmg value beyond that call scope.
 * Take care to ensure it doesn't get used more than once in other instances.
 */
#define Maybe_Half_Phys(dmg)	((Half_physical_damage) ? (((dmg) + 1) / 2) : (dmg))

/* flags for special ggetobj status returns */
#define ALL_FINISHED	  0x01  /* called routine already finished the job */

/* flags to control query_objlist() */
#define BY_NEXTHERE	  0x1	/* follow objlist by nexthere field */
#define AUTOSELECT_SINGLE 0x2	/* if only 1 object, don't ask */
#define USE_INVLET	  0x4	/* use object's invlet */
#define INVORDER_SORT	  0x8	/* sort objects by packorder */
#define SIGNAL_NOMENU	  0x10	/* return -1 rather than 0 if none allowed */
#define FEEL_COCKATRICE   0x20  /* engage cockatrice checks and react */

/* Flags to control query_category() */
/* BY_NEXTHERE used by query_category() too, so skip 0x01 */
#define UNPAID_TYPES 0x02
#define GOLD_TYPES   0x04
#define WORN_TYPES   0x08
#define ALL_TYPES    0x10
#define BILLED_TYPES 0x20
#define CHOOSE_ALL   0x40
#define BUC_BLESSED  0x80
#define BUC_CURSED   0x100
#define BUC_UNCURSED 0x200
#define BUC_UNKNOWN  0x400
#define BUC_ALLBKNOWN (BUC_BLESSED|BUC_CURSED|BUC_UNCURSED)
#define ALL_TYPES_SELECTED -2

/* Flags to control find_mid() */
#define FM_FMON	       0x01	/* search the fmon chain */
#define FM_MIGRATE     0x02	/* search the migrating monster chain */
#define FM_MYDOGS      0x04	/* search mydogs */
#define FM_EVERYWHERE  (FM_FMON | FM_MIGRATE | FM_MYDOGS)

/* Flags to control pick_[race,role,gend,align] routines in role.c */
#define PICK_RANDOM	0
#define PICK_RIGID	1

/* Flags to control dotrap() in trap.c */
#define NOWEBMSG	0x01	/* suppress stumble into web message */
#define FORCEBUNGLE	0x02	/* adjustments appropriate for bungling */
#define RECURSIVETRAP	0x04	/* trap changed into another type this same turn */
#define TOOKPLUNGE	0x08	/* used '>' to enter pit below you */

/* Flags to control test_move in hack.c */
#define DO_MOVE		0	/* really doing the move */
#define TEST_MOVE	1	/* test a normal move (move there next) */
#define TEST_TRAV	2	/* test a future travel location */

/*** some utility macros ***/
#define yn(query) yn_function(query,ynchars, 'n')
#define ynq(query) yn_function(query,ynqchars, 'q')
#define ynaq(query) yn_function(query,ynaqchars, 'y')
#define nyaq(query) yn_function(query,ynaqchars, 'n')
#define nyNaq(query) yn_function(query,ynNaqchars, 'n')
#define ynNaq(query) yn_function(query,ynNaqchars, 'y')

/* Macros for scatter */
#define VIS_EFFECTS	0x01	/* display visual effects */
#define MAY_HITMON	0x02	/* objects may hit monsters */
#define MAY_HITYOU	0x04	/* objects may hit you */
#define MAY_HIT		(MAY_HITMON|MAY_HITYOU)
#define MAY_DESTROY	0x08	/* objects may be destroyed at random */
#define MAY_FRACTURE	0x10	/* boulders & statues may fracture */

/* Macros for launching objects */
#define ROLL		0x01	/* the object is rolling */
#define FLING		0x02	/* the object is flying thru the air */
#define LAUNCH_UNSEEN	0x40	/* hero neither caused nor saw it */
#define LAUNCH_KNOWN	0x80	/* the hero caused this by explicit action */

/* Macros for explosion types */
#define EXPL_DARK	0
#define EXPL_NOXIOUS	1
#define EXPL_MUDDY	2
#define EXPL_WET	3
#define EXPL_MAGICAL	4
#define EXPL_FIERY	5
#define EXPL_FROSTY	6
#define EXPL_MAX	7

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
#define ROTTEN_TIN    0
#define HOMEMADE_TIN  1
#define SPINACH_TIN (-1)
#define RANDOM_TIN  (-2)
#define HEALTHY_TIN (-3)

/* Flags to control menus */
#define MENUTYPELEN sizeof("traditional ")
#define MENU_TRADITIONAL 0
#define MENU_COMBINATION 1
#define MENU_PARTIAL	 2
#define MENU_FULL	 3

#define MENU_SELECTED	TRUE
#define MENU_UNSELECTED FALSE

/*
 * Option flags
 * Each higher number includes the characteristics of the numbers
 * below it.
 */
#define SET_IN_FILE	0 /* config file option only */
#define SET_VIA_PROG	1 /* may be set via extern program, not seen in game */
#define DISP_IN_GAME	2 /* may be set via extern program, displayed in game */
#define SET_IN_GAME	3 /* may be set via extern program or set in the game */

#define FEATURE_NOTICE_VER(major,minor,patch) (((unsigned long)major << 24) | \
	((unsigned long)minor << 16) | \
	((unsigned long)patch << 8) | \
	((unsigned long)0))

#define FEATURE_NOTICE_VER_MAJ	  (flags.suppress_alert >> 24)
#define FEATURE_NOTICE_VER_MIN	  (((unsigned long)(0x0000000000FF0000L & flags.suppress_alert)) >> 16)
#define FEATURE_NOTICE_VER_PATCH  (((unsigned long)(0x000000000000FF00L & flags.suppress_alert)) >>  8)

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#define plur(x) (((x) == 1) ? "" : "s")

#define ARM_BONUS(obj)	(objects[(obj)->otyp].a_ac + (obj)->spe \
			 - min((int)greatest_erosion(obj),objects[(obj)->otyp].a_ac))

#define makeknown(x)	discover_object((x),TRUE,TRUE)
#define distu(xx,yy)	dist2((int)(xx),(int)(yy),(int)u.ux,(int)u.uy)
#define onlineu(xx,yy)	online2((int)(xx),(int)(yy),(int)u.ux,(int)u.uy)

#define rn1(x,y)	(rn2(x)+(y))

/* negative armor class is randomly weakened to prevent invulnerability */
#define AC_VALUE(AC)	((AC) >= 0 ? (AC) : -rnd(-(AC)))

#if defined(MICRO) && !defined(__DJGPP__)
#define getuid() 1
#define getlogin() ((char *)0)
#endif /* MICRO */

#if defined(OVERLAY)
# define USE_OVLx
# define STATIC_DCL extern
# define STATIC_OVL
# define STATIC_VAR

#else	/* !OVERLAY */

# define STATIC_DCL static
# define STATIC_OVL static
# define STATIC_VAR static

#endif	/* OVERLAY */

/* Macro for a few items that are only static if we're not overlaid.... */
#if defined(USE_TRAMPOLI) || defined(USE_OVLx)
# define STATIC_PTR
#else
# define STATIC_PTR static
#endif

/* The function argument to qsort() requires a particular
 * calling convention under WINCE which is not the default
 * in that environment.
 */
#if defined(WIN_CE)
# define CFDECLSPEC __cdecl
#else
# define CFDECLSPEC
#endif
 
#endif /* HACK_H */

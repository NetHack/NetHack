/*	SCCS Id: @(#)prop.h	3.4	1999/07/07	*/
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PROP_H
#define PROP_H

/*** What the properties are ***/
#define FIRE_RES		 1
#define COLD_RES		 2
#define SLEEP_RES		 3
#define DISINT_RES		 4
#define SHOCK_RES		 5
#define POISON_RES		 6
#define ACID_RES		 7
#define STONE_RES		 8
/* note: for the first eight properties, MR_xxx == (1 << (xxx_RES - 1)) */
#define ADORNED			 9
#define REGENERATION		10
#define SEARCHING		11
#define SEE_INVIS		12
#define INVIS			13
#define TELEPORT		14
#define TELEPORT_CONTROL	15
#define POLYMORPH		16
#define POLYMORPH_CONTROL	17
#define LEVITATION		18
#define STEALTH			19
#define AGGRAVATE_MONSTER	20
#define CONFLICT		21
#define PROTECTION		22
#define PROT_FROM_SHAPE_CHANGERS 23
#define WARNING			24
#define TELEPAT			25
#define FAST			26
#define STUNNED			27
#define CONFUSION		28
#define SICK			29
#define BLINDED			30
#define SLEEPING		31
#define WOUNDED_LEGS		32
#define STONED			33
#define STRANGLED		34
#define HALLUC			35
#define HALLUC_RES		36
#define FUMBLING		37
#define JUMPING			38
#define WWALKING		39
#define HUNGER			40
#define GLIB			41
#define REFLECTING		42
#define LIFESAVED		43
#define ANTIMAGIC		44
#define DISPLACED		45
#define CLAIRVOYANT		46
#define VOMITING		47
#define ENERGY_REGENERATION	48
#define MAGICAL_BREATHING	49
#define HALF_SPDAM		50
#define HALF_PHDAM		51
#define SICK_RES		52
#define DRAIN_RES		53
#define WARN_UNDEAD		54
#define INVULNERABLE		55
#define FREE_ACTION		56
#define SWIMMING		57
#define SLIMED			58
#define FIXED_ABIL		59
#define FLYING			60
#define UNCHANGING		61
#define PASSES_WALLS		62
#define SLOW_DIGESTION		63
#define INFRAVISION		64
#define WARN_OF_MON		65
#define DETECT_MONSTERS		66
#define LAST_PROP		(DETECT_MONSTERS)

/*** Where the properties come from ***/
/* Definitions were moved here from obj.h and you.h */
struct prop {
	/*** Properties conveyed by objects ***/
	long extrinsic;
	/* Armor */
#	define W_ARM	    0x00000001L /* Body armor */
#	define W_ARMC	    0x00000002L /* Cloak */
#	define W_ARMH	    0x00000004L /* Helmet/hat */
#	define W_ARMS	    0x00000008L /* Shield */
#	define W_ARMG	    0x00000010L /* Gloves/gauntlets */
#	define W_ARMF	    0x00000020L /* Footwear */
#ifdef TOURIST
#	define W_ARMU	    0x00000040L /* Undershirt */
#	define W_ARMOR	     (W_ARM | W_ARMC | W_ARMH | W_ARMS | W_ARMG | W_ARMF | W_ARMU)
#else
#	define W_ARMOR	     (W_ARM | W_ARMC | W_ARMH | W_ARMS | W_ARMG | W_ARMF)
#endif
	/* Weapons and artifacts */
#	define W_WEP	    0x00000100L /* Wielded weapon */
#	define W_QUIVER     0x00000200L /* Quiver for (f)iring ammo */
#	define W_SWAPWEP    0x00000400L /* Secondary weapon */
#	define W_ART	    0x00001000L /* Carrying artifact (not really worn) */
#	define W_ARTI	    0x00002000L /* Invoked artifact  (not really worn) */
	/* Amulets, rings, tools, and other items */
#	define W_AMUL	    0x00010000L /* Amulet */
#	define W_RINGL	    0x00020000L /* Left ring */
#	define W_RINGR	    0x00040000L /* Right ring */
#	define W_RING	    (W_RINGL | W_RINGR)
#	define W_TOOL	    0x00080000L /* Eyewear */
#ifdef STEED
#	define W_SADDLE     0x00100000L	/* KMH -- For riding monsters */
#endif
#	define W_BALL	    0x00200000L /* Punishment ball */
#	define W_CHAIN	    0x00400000L /* Punishment chain */

	/*** Property is blocked by an object ***/
	long blocked;					/* Same assignments as extrinsic */

	/*** Timeouts, permanent properties, and other flags ***/
	long intrinsic;
	/* Timed properties */
#	define TIMEOUT	    0x00ffffffL /* Up to 16 million turns */
	/* Permanent properties */
#	define FROMEXPER    0x01000000L /* Gain/lose with experience, for role */
#	define FROMRACE     0x02000000L /* Gain/lose with experience, for race */
#	define FROMOUTSIDE  0x04000000L /* By corpses, prayer, thrones, etc. */
#	define INTRINSIC    (FROMOUTSIDE|FROMRACE|FROMEXPER)
	/* Control flags */
#	define I_SPECIAL    0x10000000L /* Property is controllable */
};

/*** Definitions for backwards compatibility ***/
#define LEFT_RING	W_RINGL
#define RIGHT_RING	W_RINGR
#define LEFT_SIDE	LEFT_RING
#define RIGHT_SIDE	RIGHT_RING
#define BOTH_SIDES	(LEFT_SIDE | RIGHT_SIDE)
#define WORN_ARMOR	W_ARM
#define WORN_CLOAK	W_ARMC
#define WORN_HELMET	W_ARMH
#define WORN_SHIELD	W_ARMS
#define WORN_GLOVES	W_ARMG
#define WORN_BOOTS	W_ARMF
#define WORN_AMUL	W_AMUL
#define WORN_BLINDF	W_TOOL
#ifdef TOURIST
#define WORN_SHIRT	W_ARMU
#endif

#endif /* PROP_H */

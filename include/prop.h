/* NetHack 3.6	prop.h	$NHDT-Date: 1437877163 2015/07/26 02:19:23 $  $NHDT-Branch: master $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PROP_H
#define PROP_H

/*** What the properties are ***
 *
 * note:  propertynames[] array in timeout.c has string values for these.
 *        Property #0 is not used.
 */
/* Resistances to troubles */
enum prop_types {
    FIRE_RES = 1,
    COLD_RES,
    SLEEP_RES,
    DISINT_RES,
    SHOCK_RES,
    POISON_RES,
    ACID_RES,
    STONE_RES,
    /* note: for the first eight properties, MR_xxx == (1 << (xxx_RES - 1)) */
    DRAIN_RES,
    SICK_RES,
    INVULNERABLE,
    ANTIMAGIC,
    /* Troubles */
    STUNNED,
    CONFUSION,
    BLINDED,
    DEAF,
    SICK,
    STONED,
    STRANGLED,
    VOMITING,
    GLIB,
    SLIMED,
    HALLUC,
    HALLUC_RES,
    FUMBLING,
    WOUNDED_LEGS,
    SLEEPY,
    HUNGER,
    /* Vision and senses */
    SEE_INVIS,
    TELEPAT,
    WARNING,
    WARN_OF_MON,
    WARN_UNDEAD,
    SEARCHING,
    CLAIRVOYANT,
    INFRAVISION,
    DETECT_MONSTERS,
    /* Appearance and behavior */
    ADORNED,
    INVIS,
    DISPLACED,
    STEALTH,
    AGGRAVATE_MONSTER,
    CONFLICT,
    /* Transportation */
    JUMPING,
    TELEPORT,
    TELEPORT_CONTROL,
    LEVITATION,
    FLYING,
    WWALKING,
    SWIMMING,
    MAGICAL_BREATHING,
    PASSES_WALLS,
    /* Physical attributes */
    SLOW_DIGESTION,
    HALF_SPDAM,
    HALF_PHDAM,
    REGENERATION,
    ENERGY_REGENERATION,
    PROTECTION,
    PROT_FROM_SHAPE_CHANGERS,
    POLYMORPH,
    POLYMORPH_CONTROL,
    UNCHANGING,
    FAST,
    REFLECTING,
    FREE_ACTION,
    FIXED_ABIL,
    LIFESAVED
};
#define LAST_PROP (LIFESAVED)

/*** Where the properties come from ***/
/* Definitions were moved here from obj.h and you.h */
struct prop {
    /*** Properties conveyed by objects ***/
    long extrinsic;
/* Armor */
#define W_ARM 0x00000001L  /* Body armor */
#define W_ARMC 0x00000002L /* Cloak */
#define W_ARMH 0x00000004L /* Helmet/hat */
#define W_ARMS 0x00000008L /* Shield */
#define W_ARMG 0x00000010L /* Gloves/gauntlets */
#define W_ARMF 0x00000020L /* Footwear */
#define W_ARMU 0x00000040L /* Undershirt */
#define W_ARMOR (W_ARM | W_ARMC | W_ARMH | W_ARMS | W_ARMG | W_ARMF | W_ARMU)
/* Weapons and artifacts */
#define W_WEP 0x00000100L     /* Wielded weapon */
#define W_QUIVER 0x00000200L  /* Quiver for (f)iring ammo */
#define W_SWAPWEP 0x00000400L /* Secondary weapon */
#define W_WEAPON (W_WEP | W_SWAPWEP | W_QUIVER)
#define W_ART 0x00001000L     /* Carrying artifact (not really worn) */
#define W_ARTI 0x00002000L    /* Invoked artifact  (not really worn) */
/* Amulets, rings, tools, and other items */
#define W_AMUL 0x00010000L    /* Amulet */
#define W_RINGL 0x00020000L   /* Left ring */
#define W_RINGR 0x00040000L   /* Right ring */
#define W_RING (W_RINGL | W_RINGR)
#define W_TOOL 0x00080000L   /* Eyewear */
#define W_ACCESSORY (W_RING | W_AMUL | W_TOOL)
    /* historical note: originally in slash'em, 'worn' saddle stayed in
       hero's inventory; in nethack, it's kept in the steed's inventory */
#define W_SADDLE 0x00100000L /* KMH -- For riding monsters */
#define W_BALL 0x00200000L   /* Punishment ball */
#define W_CHAIN 0x00400000L  /* Punishment chain */

    /*** Property is blocked by an object ***/
    long blocked; /* Same assignments as extrinsic */

    /*** Timeouts, permanent properties, and other flags ***/
    long intrinsic;
/* Timed properties */
#define TIMEOUT 0x00ffffffL     /* Up to 16 million turns */
                                /* Permanent properties */
#define FROMEXPER 0x01000000L   /* Gain/lose with experience, for role */
#define FROMRACE 0x02000000L    /* Gain/lose with experience, for race */
#define FROMOUTSIDE 0x04000000L /* By corpses, prayer, thrones, etc. */
#define INTRINSIC (FROMOUTSIDE | FROMRACE | FROMEXPER)
/* Control flags */
#define FROMFORM 0x10000000L  /* Polyd; conferred by monster form */
#define I_SPECIAL 0x20000000L /* Property is controllable */
};

/*** Definitions for backwards compatibility ***/
#define LEFT_RING W_RINGL
#define RIGHT_RING W_RINGR
#define LEFT_SIDE LEFT_RING
#define RIGHT_SIDE RIGHT_RING
#define BOTH_SIDES (LEFT_SIDE | RIGHT_SIDE)
#define WORN_ARMOR W_ARM
#define WORN_CLOAK W_ARMC
#define WORN_HELMET W_ARMH
#define WORN_SHIELD W_ARMS
#define WORN_GLOVES W_ARMG
#define WORN_BOOTS W_ARMF
#define WORN_AMUL W_AMUL
#define WORN_BLINDF W_TOOL
#define WORN_SHIRT W_ARMU

#endif /* PROP_H */

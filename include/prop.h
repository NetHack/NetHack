/* NetHack 3.6	prop.h	$NHDT-Date: 1437877163 2015/07/26 02:19:23 $  $NHDT-Branch: master $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PROP_H
#define PROP_H

/*** What the properties are ***/
/* Resistances to troubles */
#define FIRE_RES 1
#define COLD_RES 2
#define SLEEP_RES 3
#define DISINT_RES 4
#define SHOCK_RES 5
#define POISON_RES 6
#define ACID_RES 7
#define STONE_RES 8
/* note: for the first eight properties, MR_xxx == (1 << (xxx_RES - 1)) */
#define DRAIN_RES 9
#define SICK_RES 10
#define INVULNERABLE 11
#define ANTIMAGIC 12
/* Troubles */
#define STUNNED 13
#define CONFUSION 14
#define BLINDED 15
#define DEAF 16
#define SICK 17
#define STONED 18
#define STRANGLED 19
#define VOMITING 20
#define GLIB 21
#define SLIMED 22
#define HALLUC 23
#define HALLUC_RES 24
#define FUMBLING 25
#define WOUNDED_LEGS 26
#define SLEEPY 27
#define HUNGER 28
/* Vision and senses */
#define SEE_INVIS 29
#define TELEPAT 30
#define WARNING 31
#define WARN_OF_MON 32
#define WARN_UNDEAD 33
#define SEARCHING 34
#define CLAIRVOYANT 35
#define INFRAVISION 36
#define DETECT_MONSTERS 37
/* Appearance and behavior */
#define ADORNED 38
#define INVIS 39
#define DISPLACED 40
#define STEALTH 41
#define AGGRAVATE_MONSTER 42
#define CONFLICT 43
/* Transportation */
#define JUMPING 44
#define TELEPORT 45
#define TELEPORT_CONTROL 46
#define LEVITATION 47
#define FLYING 48
#define WWALKING 49
#define SWIMMING 50
#define MAGICAL_BREATHING 51
#define PASSES_WALLS 52
/* Physical attributes */
#define SLOW_DIGESTION 53
#define HALF_SPDAM 54
#define HALF_PHDAM 55
#define REGENERATION 56
#define ENERGY_REGENERATION 57
#define PROTECTION 58
#define PROT_FROM_SHAPE_CHANGERS 59
#define POLYMORPH 60
#define POLYMORPH_CONTROL 61
#define UNCHANGING 62
#define FAST 63
#define REFLECTING 64
#define FREE_ACTION 65
#define FIXED_ABIL 66
#define LIFESAVED 67
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

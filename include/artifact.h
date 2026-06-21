/* NetHack 5.0	artifact.h	$NHDT-Date: 1781973076 2026/06/20 16:31:16 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ARTIFACT_H
#define ARTIFACT_H

#include "permonst.h"
#include "prop.h"

/* clang-format off */

#define SPFX_NONE   0x00000000L /* no special effects, just a bonus */
#define SPFX_NOGEN  0x00000001L /* item is special, bequeathed by gods */
#define SPFX_RESTR  0x00000002L /* item is restricted - can't be named */
#define SPFX_INTEL  0x00000004L /* item is self-willed - intelligent */
#define SPFX_SPEAK  0x00000008L /* item can speak (not implemented) */
#define SPFX_SEEK   0x00000010L /* item helps you search for things */
#define SPFX_WARN   0x00000020L /* item warns you of danger */
#define SPFX_ATTK   0x00000040L /* item has a special attack (attk) */
#define SPFX_DEFN   0x00000080L /* item has a special defence (defn) */
#define SPFX_DRLI   0x00000100L /* drains a level from monsters */
#define SPFX_SEARCH 0x00000200L /* helps searching */
#define SPFX_BEHEAD 0x00000400L /* beheads monsters */
#define SPFX_HALRES 0x00000800L /* blocks hallucinations */
#define SPFX_ESP    0x00001000L /* ESP (like amulet of ESP) */
#define SPFX_STLTH  0x00002000L /* Stealth */
#define SPFX_REGEN  0x00004000L /* Regeneration */
#define SPFX_EREGEN 0x00008000L /* Energy Regeneration */
#define SPFX_HSPDAM 0x00010000L /* 1/2 spell damage (on player) in combat */
#define SPFX_HPHDAM 0x00020000L /* 1/2 physical damage (on player) in combat */
#define SPFX_TCTRL  0x00040000L /* Teleportation Control */
#define SPFX_LUCK   0x00080000L /* Increase Luck (like Luckstone) */
#define SPFX_DMONS  0x00100000L /* attack bonus on one monster type */
#define SPFX_DCLAS  0x00200000L /* attack bonus on monsters w/ symbol mtype */
#define SPFX_DFLAG1 0x00400000L /* attack bonus on monsters w/ mflags1 flag */
#define SPFX_DFLAG2 0x00800000L /* attack bonus on monsters w/ mflags2 flag */
#define SPFX_DALIGN 0x01000000L /* attack bonus on non-aligned monsters  */
#define SPFX_DBONUS 0x01F00000L /* attack bonus mask */
#define SPFX_XRAY   0x02000000L /* gives X-RAY vision to player */
#define SPFX_REFLECT 0x04000000L /* Reflection */
#define SPFX_PROTECT 0x08000000L /* Protection */

struct artifact {
    short otyp;
    const char *name;
    unsigned long spfx;  /* special effect from wielding/wearing */
    unsigned long cspfx; /* special effect just from carrying obj */
    unsigned long mtype; /* monster type, symbol, or flag */
    struct attack attk, defn, cary;
    uchar inv_prop;     /* property obtained by invoking artifact */
    aligntyp alignment; /* alignment of bequeathing gods */
    short role;         /* character role associated with */
    short race;         /* character race associated with */
    schar gen_spe;      /* bias to spe when gifted or randomly generated */
    uchar gift_value;   /* minimum sacrifice value to be gifted this */
    long cost;          /* price when sold to hero (default 100 x base cost) */
    char acolor;        /* color to use if artifact 'glows' */
};

/* invoked properties with special powers */
enum invoke_prop_types {
    TAMING = (LAST_PROP + 1),
    HEALING,
    ENERGY_BOOST,
    UNTRAP,
    CHARGE_OBJ,
    LEV_TELE,
    CREATE_PORTAL,
    ENLIGHTENING,
    CREATE_AMMO,
    BANISH,
    FLING_POISON,
    FIRESTORM,
    SNOWSTORM,
    BLINDING_RAY
};

/* artifact tracking; gift and wish imply found; it also gets set for items
   seen on the floor, in containers, and wielded or dropped by monsters */
struct arti_info {
    Bitfield(exists, 1); /* 1 if corresponding artifact has been created */
    Bitfield(found, 1);  /* 1 if artifact is known by hero to exist */
    Bitfield(gift, 1);   /* 1 iff artifact was created as a prayer reward */
    Bitfield(wish, 1);   /* 1 iff artifact was created via wish */
    Bitfield(named, 1);  /* 1 iff artifact was made by naming an item */
    Bitfield(viadip, 1); /* 1 iff dipped long sword became Excalibur */
    Bitfield(lvldef, 1); /* 1 iff created by special level definition */
    Bitfield(bones, 1);  /* 1 iff came from bones file */
    Bitfield(rndm, 1);   /* 1 iff randomly generated */
};

/* clang-format on */
#endif /* ARTIFACT_H */

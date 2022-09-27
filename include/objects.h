/* NetHack 3.7	objects.h	$NHDT-Date: 1596498192 2020/08/03 23:43:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.66 $ */
/* Copyright (c) Mike Threepoint, 1989.                           */
/* NetHack may be freely redistributed.  See license for details. */

/*
   The data in this file is processed multiple times by its inclusion
   in several places in the code. The results depend on the definition
   of the following:
     OBJECTS_ENUM        to construct object onames enum entries (decl.h).
     OBJECTS_DESCR_INIT to construct obj_descr[] array entries (objects.c).
     OBJECTS_INIT       to construct objects[] array entries (objects.c).
*/

#ifndef NoDes
#define NoDes (char *) 0 /* less visual distraction for 'no description' */
#endif

#ifndef lint
#define HARDGEM(n) (n >= 8)
#else
#define HARDGEM(n) (0)
#endif

/*
 * Note...
 *  OBJECTS() currently has 15 parameters; it more become needed, some
 *  will need to be combined the way BITS() is used, because compilers
 *  are allowed to impose a limit of 15.
 */

#if defined(OBJECTS_DESCR_INIT)
#define OBJ(name,desc)  name, desc
#define OBJECT(obj,bits,prp,sym,prob,dly,wt, \
               cost,sdam,ldam,oc1,oc2,nut,color,sn)  { obj }

#elif defined(OBJECTS_INIT)
#define COLOR_FIELD(X) X,
/* notes: 'sub' was once a bitfield but got changed to separate schar when
   it was overloaded to hold negative weapon skill indices; the first zero
   is padding for oc_prediscovered which has variable init at run-time;
   the second zero is oc_spare1 for padding between oc_tough and oc_dir */
#define BITS(nmkn,mrg,uskn,ctnr,mgc,chrg,uniq,nwsh,big,tuf,dir,sub,mtrl) \
    nmkn,mrg,uskn,0,mgc,chrg,uniq,nwsh,big,tuf,0,dir,mtrl,sub /*cpp fodder*/
#define OBJECT(obj,bits,prp,sym,prob,dly,wt,        \
               cost,sdam,ldam,oc1,oc2,nut,color,sn) \
  { 0, 0, (char *) 0, bits, prp, sym, dly, COLOR_FIELD(color) prob, wt, \
    cost, sdam, ldam, oc1, oc2, nut }

#elif defined(OBJECTS_ENUM)
#define OBJ(name,desc)
#define OBJECT(obj,bits,prp,sym,prob,dly,wt,        \
               cost,sdam,ldam,oc1,oc2,nut,color,sn) \
    sn

#elif defined(DUMP_ENUMS)
#define OBJ(name,desc)
#define OBJECT(obj,bits,prp,sym,prob,dly,wt,        \
               cost,sdam,ldam,oc1,oc2,nut,color,sn) \
  { sn, #sn }

#else
#error Unproductive inclusion of objects.h
#endif  /* OBJECTS_DESCR_INIT || OBJECTS_INIT || OBJECTS_ENUM */

/* dummy object[0] -- description [2nd arg] *must* be NULL */
OBJECT(OBJ("strange object", NoDes),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0),
       0, ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, STRANGE_OBJECT),

/* weapons ... */
#define WEAPON(name,desc,kn,mg,bi,prob,wt,                          \
               cost,sdam,ldam,hitbon,typ,sub,metal,color,sn)        \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, mg, 1, 0, 0, 1, 0, 0, bi, 0, typ, sub, metal),  \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color,sn)
#define PROJECTILE(name,desc,kn,prob,wt,                            \
                   cost,sdam,ldam,hitbon,metal,sub,color,sn)        \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, 1, 1, 0, 0, 1, 0, 0, 0, 0, PIERCE, sub, metal), \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color, sn)
#define BOW(name,desc,kn,prob,wt,cost,hitbon,metal,sub,color,sn)    \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, sub, metal),      \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, 2, 2, hitbon, 0, wt, color, sn)

/* Note: for weapons that don't do an even die of damage (ex. 2-7 or 3-18)
   the extra damage is added on in weapon.c, not here! */

/* weapon strike mode overloads the oc_dir field */
#define P PIERCE
#define S SLASH
#define B WHACK

/* missiles; materiel reflects the arrowhead, not the shaft */
PROJECTILE("arrow", NoDes,
           1, 55, 1, 2, 6, 6, 0,        IRON, -P_BOW, HI_METAL,
                                                        ARROW),
PROJECTILE("elven arrow", "runed arrow",
           0, 20, 1, 2, 7, 6, 0,        WOOD, -P_BOW, HI_WOOD,
                                                        ELVEN_ARROW),
PROJECTILE("orcish arrow", "crude arrow",
           0, 20, 1, 2, 5, 6, 0,        IRON, -P_BOW, CLR_BLACK,
                                                        ORCISH_ARROW),
PROJECTILE("silver arrow", NoDes,
           1, 12, 1, 5, 6, 6, 0,        SILVER, -P_BOW, HI_SILVER,
                                                        SILVER_ARROW),
PROJECTILE("ya", "bamboo arrow",
           0, 15, 1, 4, 7, 7, 1,        METAL, -P_BOW, HI_METAL, YA),
PROJECTILE("crossbow bolt", NoDes,
           1, 55, 1, 2, 4, 6, 0,        IRON, -P_CROSSBOW, HI_METAL,
                                                        CROSSBOW_BOLT),

/* missiles that don't use a launcher */
WEAPON("dart", NoDes,
       1, 1, 0, 60,   1,   2,  3,  2, 0, P,   -P_DART, IRON, HI_METAL,
                                                        DART),
WEAPON("shuriken", "throwing star",
       0, 1, 0, 35,   1,   5,  8,  6, 2, P,   -P_SHURIKEN, IRON, HI_METAL,
                                                        SHURIKEN),
WEAPON("boomerang", NoDes,
       1, 1, 0, 15,   5,  20,  9,  9, 0, 0,   -P_BOOMERANG, WOOD, HI_WOOD,
                                                        BOOMERANG),

/* spears [note: javelin used to have a separate skill from spears,
   because the latter are primarily stabbing weapons rather than
   throwing ones; but for playability, they've been merged together
   under spear skill and spears can now be thrown like javelins] */
WEAPON("spear", NoDes,
       1, 1, 0, 50,  30,   3,  6,  8, 0, P,   P_SPEAR, IRON, HI_METAL,
                                                        SPEAR),
WEAPON("elven spear", "runed spear",
       0, 1, 0, 10,  30,   3,  7,  8, 0, P,   P_SPEAR, WOOD, HI_WOOD,
                                                        ELVEN_SPEAR),
WEAPON("orcish spear", "crude spear",
       0, 1, 0, 13,  30,   3,  5,  8, 0, P,   P_SPEAR, IRON, CLR_BLACK,
                                                        ORCISH_SPEAR),
WEAPON("dwarvish spear", "stout spear",
       0, 1, 0, 12,  35,   3,  8,  8, 0, P,   P_SPEAR, IRON, HI_METAL,
                                                        DWARVISH_SPEAR),
WEAPON("silver spear", NoDes,
       1, 1, 0,  2,  36,  40,  6,  8, 0, P,   P_SPEAR, SILVER, HI_SILVER,
                                                        SILVER_SPEAR),
WEAPON("javelin", "throwing spear",
       0, 1, 0, 10,  20,   3,  6,  6, 0, P,   P_SPEAR, IRON, HI_METAL,
                                                        JAVELIN),

/* spearish; doesn't stack, not intended to be thrown */
WEAPON("trident", NoDes,
       1, 0, 0,  8,  25,   5,  6,  4, 0, P,   P_TRIDENT, IRON, HI_METAL,
                                                        TRIDENT),
        /* +1 small, +2d4 large */

/* blades; all stack */
WEAPON("dagger", NoDes,
       1, 1, 0, 30,  10,   4,  4,  3, 2, P,   P_DAGGER, IRON, HI_METAL,
                                                        DAGGER),
WEAPON("elven dagger", "runed dagger",
       0, 1, 0, 10,  10,   4,  5,  3, 2, P,   P_DAGGER, WOOD, HI_WOOD,
                                                        ELVEN_DAGGER),
WEAPON("orcish dagger", "crude dagger",
       0, 1, 0, 12,  10,   4,  3,  3, 2, P,   P_DAGGER, IRON, CLR_BLACK,
                                                        ORCISH_DAGGER),
WEAPON("silver dagger", NoDes,
       1, 1, 0,  3,  12,  40,  4,  3, 2, P,   P_DAGGER, SILVER, HI_SILVER,
                                                        SILVER_DAGGER),
WEAPON("athame", NoDes,
       1, 1, 0,  0,  10,   4,  4,  3, 2, S,   P_DAGGER, IRON, HI_METAL,
                                                        ATHAME),
WEAPON("scalpel", NoDes,
       1, 1, 0,  0,   5,   6,  3,  3, 2, S,   P_KNIFE, METAL, HI_METAL,
                                                        SCALPEL),
WEAPON("knife", NoDes,
       1, 1, 0, 20,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL,
                                                        KNIFE),
WEAPON("stiletto", NoDes,
       1, 1, 0,  5,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL,
                                                        STILETTO),
/* 3.6: worm teeth and crysknives now stack;
   when a stack of teeth is enchanted at once, they fuse into one crysknife;
   when a stack of crysknives drops, the whole stack reverts to teeth */
/* 3.7: change crysknife from MINERAL to BONE and worm tooth from 0 to BONE */
WEAPON("worm tooth", NoDes,
       1, 1, 0,  0,  20,   2,  2,  2, 0, 0,   P_KNIFE, BONE, CLR_WHITE,
                                                        WORM_TOOTH),
WEAPON("crysknife", NoDes,
       1, 1, 0,  0,  20, 100, 10, 10, 3, P,   P_KNIFE, BONE, CLR_WHITE,
                                                        CRYSKNIFE),

/* axes */
WEAPON("axe", NoDes,
       1, 0, 0, 40,  60,   8,  6,  4, 0, S,   P_AXE, IRON, HI_METAL,
                                                        AXE),
WEAPON("battle-axe", "double-headed axe",       /* "double-bitted"? */
       0, 0, 1, 10, 120,  40,  8,  6, 0, S,   P_AXE, IRON, HI_METAL,
                                                        BATTLE_AXE),

/* swords */
WEAPON("short sword", NoDes,
       1, 0, 0,  8,  30,  10,  6,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL,
                                                        SHORT_SWORD),
WEAPON("elven short sword", "runed short sword",
       0, 0, 0,  2,  30,  10,  8,  8, 0, P,   P_SHORT_SWORD, WOOD, HI_WOOD,
                                                        ELVEN_SHORT_SWORD),
WEAPON("orcish short sword", "crude short sword",
       0, 0, 0,  3,  30,  10,  5,  8, 0, P,   P_SHORT_SWORD, IRON, CLR_BLACK,
                                                        ORCISH_SHORT_SWORD),
WEAPON("dwarvish short sword", "broad short sword",
       0, 0, 0,  2,  30,  10,  7,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL,
                                                        DWARVISH_SHORT_SWORD),
WEAPON("scimitar", "curved sword",
       0, 0, 0, 15,  40,  15,  8,  8, 0, S,   P_SABER, IRON, HI_METAL,
                                                        SCIMITAR),
WEAPON("silver saber", NoDes,
       1, 0, 0,  6,  40,  75,  8,  8, 0, S,   P_SABER, SILVER, HI_SILVER,
                                                        SILVER_SABER),
WEAPON("broadsword", NoDes,
       1, 0, 0,  8,  70,  10,  4,  6, 0, S,   P_BROAD_SWORD, IRON, HI_METAL,
                                                        BROADSWORD),
        /* +d4 small, +1 large */
WEAPON("elven broadsword", "runed broadsword",
       0, 0, 0,  4,  70,  10,  6,  6, 0, S,   P_BROAD_SWORD, WOOD, HI_WOOD,
                                                        ELVEN_BROADSWORD),
        /* +d4 small, +1 large */
WEAPON("long sword", NoDes,
       1, 0, 0, 50,  40,  15,  8, 12, 0, S,   P_LONG_SWORD, IRON, HI_METAL,
                                                        LONG_SWORD),
WEAPON("two-handed sword", NoDes,
       1, 0, 1, 22, 150,  50, 12,  6, 0, S,   P_TWO_HANDED_SWORD,
                                                            IRON, HI_METAL,
                                                        TWO_HANDED_SWORD),
        /* +2d6 large */
WEAPON("katana", "samurai sword",
       0, 0, 0,  4,  40,  80, 10, 12, 1, S,   P_LONG_SWORD, IRON, HI_METAL,
                                                        KATANA),
/* special swords set up for artifacts */
WEAPON("tsurugi", "long samurai sword",
       0, 0, 1,  0,  60, 500, 16,  8, 2, S,   P_TWO_HANDED_SWORD,
                                                            METAL, HI_METAL,
                                                        TSURUGI),
        /* +2d6 large */
WEAPON("runesword", "runed broadsword",
       0, 0, 0,  0,  40, 300,  4,  6, 0, S,   P_BROAD_SWORD, IRON, CLR_BLACK,
                                                        RUNESWORD),
        /* +d4 small, +1 large; Stormbringer: +5d2 +d8 from level drain */

/* polearms */
/* spear-type */
WEAPON("partisan", "vulgar polearm",
       0, 0, 1,  5,  80,  10,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL,
                                                        PARTISAN),
        /* +1 large */
WEAPON("ranseur", "hilted polearm",
       0, 0, 1,  5,  50,   6,  4,  4, 0, P,   P_POLEARMS, IRON, HI_METAL,
                                                        RANSEUR),
        /* +d4 both */
WEAPON("spetum", "forked polearm",
       0, 0, 1,  5,  50,   5,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL,
                                                        SPETUM),
        /* +1 small, +d6 large */
WEAPON("glaive", "single-edged polearm",
       0, 0, 1,  8,  75,   6,  6, 10, 0, S,   P_POLEARMS, IRON, HI_METAL,
                                                        GLAIVE),
/* axe-type */
WEAPON("halberd", "angled poleaxe",
       0, 0, 1,  8, 150,  10, 10,  6, 0, P|S, P_POLEARMS, IRON, HI_METAL,
                                                        HALBERD),
        /* +1d6 large */
WEAPON("bardiche", "long poleaxe",
       0, 0, 1,  4, 120,   7,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL,
                                                        BARDICHE),
        /* +1d4 small, +2d4 large */
WEAPON("voulge", "pole cleaver",
       0, 0, 1,  4, 125,   5,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL,
                                                        VOULGE),
        /* +d4 both */
/* curved/hooked */
WEAPON("fauchard", "pole sickle",
       0, 0, 1,  6,  60,   5,  6,  8, 0, P|S, P_POLEARMS, IRON, HI_METAL,
                                                        FAUCHARD),
WEAPON("guisarme", "pruning hook",
       0, 0, 1,  6,  80,   5,  4,  8, 0, S,   P_POLEARMS, IRON, HI_METAL,
                                                        GUISARME),
        /* +1d4 small */
WEAPON("bill-guisarme", "hooked polearm",
       0, 0, 1,  4, 120,   7,  4, 10, 0, P|S, P_POLEARMS, IRON, HI_METAL,
                                                        BILL_GUISARME),
        /* +1d4 small */
/* other */
WEAPON("lucern hammer", "pronged polearm",
       0, 0, 1,  5, 150,   7,  4,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL,
                                                        LUCERN_HAMMER),
        /* +1d4 small */
WEAPON("bec de corbin", "beaked polearm",
       0, 0, 1,  4, 100,   8,  8,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL,
                                                        BEC_DE_CORBIN),

/* formerly grouped with the polearms but don't use polearms skill;
   lance isn't even two-handed */
WEAPON("dwarvish mattock", "broad pick",
       0, 0, 1, 13, 120,  50, 12,  8, -1, B,  P_PICK_AXE, IRON, HI_METAL,
                                                        DWARVISH_MATTOCK),
WEAPON("lance", NoDes,
       1, 0, 0,  4, 180,  10,  6,  8, 0, P,   P_LANCE, IRON, HI_METAL,
                                                        LANCE),
        /* +2d10 when jousting with lance as primary weapon,
           +2d2 when jousting with it as secondary when dual wielding */

/* bludgeons */
WEAPON("mace", NoDes,
       1, 0, 0, 40,  30,   5,  6,  6, 0, B,   P_MACE, IRON, HI_METAL,
                                                        MACE),
        /* +1 small */
WEAPON("morning star", NoDes,
       1, 0, 0, 12, 120,  10,  4,  6, 0, B,   P_MORNING_STAR, IRON, HI_METAL,
                                                        MORNING_STAR),
        /* +d4 small, +1 large */
WEAPON("war hammer", NoDes,
       1, 0, 0, 15,  50,   5,  4,  4, 0, B,   P_HAMMER, IRON, HI_METAL,
                                                        WAR_HAMMER),
        /* +1 small */
WEAPON("club", NoDes,
       1, 0, 0, 12,  30,   3,  6,  3, 0, B,   P_CLUB, WOOD, HI_WOOD,
                                                        CLUB),
WEAPON("rubber hose", NoDes,
       1, 0, 0,  0,  20,   3,  4,  3, 0, B,   P_WHIP, PLASTIC, CLR_BROWN,
                                                        RUBBER_HOSE),
WEAPON("quarterstaff", "staff",
       0, 0, 1, 11,  40,   5,  6,  6, 0, B,   P_QUARTERSTAFF, WOOD, HI_WOOD,
                                                        QUARTERSTAFF),
/* two-piece */
WEAPON("aklys", "thonged club",
       0, 0, 0,  8,  15,   4,  6,  3, 0, B,   P_CLUB, IRON, HI_METAL,
                                                        AKLYS),
WEAPON("flail", NoDes,
       1, 0, 0, 40,  15,   4,  6,  4, 0, B,   P_FLAIL, IRON, HI_METAL,
                                                        FLAIL),
        /* +1 small, +1d4 large */

/* misc */
WEAPON("bullwhip", NoDes,
       1, 0, 0,  2,  20,   4,  2,  1, 0, 0,   P_WHIP, LEATHER, CLR_BROWN,
                                                        BULLWHIP),

/* bows */
BOW("bow", NoDes,               1, 24, 30, 60, 0, WOOD, P_BOW, HI_WOOD,
                                                        BOW),
BOW("elven bow", "runed bow",  0, 12, 30, 60, 0, WOOD, P_BOW, HI_WOOD,
                                                        ELVEN_BOW),
BOW("orcish bow", "crude bow", 0, 12, 30, 60, 0, WOOD, P_BOW, CLR_BLACK,
                                                        ORCISH_BOW),
BOW("yumi", "long bow",        0,  0, 30, 60, 0, WOOD, P_BOW, HI_WOOD,
                                                        YUMI),
BOW("sling", NoDes,             1, 40,  3, 20, 0, LEATHER, P_SLING, HI_LEATHER,
                                                        SLING),
BOW("crossbow", NoDes,          1, 45, 50, 40, 0, WOOD, P_CROSSBOW, HI_WOOD,
                                                        CROSSBOW),

#undef P
#undef S
#undef B

#undef WEAPON
#undef PROJECTILE
#undef BOW

/* armor ... */
        /* IRON denotes ferrous metals, including steel.
         * Only IRON weapons and armor can rust.
         * Only COPPER (including brass) corrodes.
         * Some creatures are vulnerable to SILVER.
         */
#define ARMOR(name,desc,kn,mgc,blk,power,prob,delay,wt,  \
              cost,ac,can,sub,metal,c,sn)                   \
    OBJECT(OBJ(name, desc),                                         \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, blk, 0, 0, sub, metal),  \
           power, ARMOR_CLASS, prob, delay, wt,                     \
           cost, 0, 0, 10 - ac, can, wt, c, sn)
#define HELM(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c,sn)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_HELM, metal, c, sn)
#define CLOAK(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c,sn)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_CLOAK, metal, c,sn)
#define SHIELD(name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,metal,c,sn) \
    ARMOR(name, desc, kn, mgc, blk, power, prob, delay, wt, \
          cost, ac, can, ARM_SHIELD, metal, c,sn)
#define GLOVES(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c,sn)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_GLOVES, metal, c,sn)
#define BOOTS(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c,sn)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_BOOTS, metal, c,sn)

/* helmets */
HELM("elven leather helm", "leather hat",
     0, 0,           0,  6, 1,  3,  8,  9, 0, LEATHER, HI_LEATHER,
                                                        ELVEN_LEATHER_HELM),
HELM("orcish helm", "iron skull cap",
     0, 0,           0,  6, 1, 30, 10,  9, 0, IRON, CLR_BLACK,
                                                        ORCISH_HELM),
HELM("dwarvish iron helm", "hard hat",
     0, 0,           0,  6, 1, 40, 20,  8, 0, IRON, HI_METAL,
                                                        DWARVISH_IRON_HELM),
HELM("fedora", NoDes,
     1, 0,           0,  0, 0,  3,  1, 10, 0, CLOTH, CLR_BROWN,
                                                        FEDORA),
HELM("cornuthaum", "conical hat",
     0, 1, CLAIRVOYANT,  3, 1,  4, 80, 10, 1, CLOTH, CLR_BLUE,
                                                        CORNUTHAUM),
        /* name coined by devteam; confers clairvoyance for wizards,
           blocks clairvoyance if worn by role other than wizard */
HELM("dunce cap", "conical hat",
     0, 1,           0,  3, 1,  4,  1, 10, 0, CLOTH, CLR_BLUE,
                                                        DUNCE_CAP),
HELM("dented pot", NoDes,
     1, 0,           0,  2, 0, 10,  8,  9, 0, IRON, CLR_BLACK,
                                                        DENTED_POT),
/* with shuffled appearances... */
HELM("helmet", "plumed helmet",
     0, 0,           0, 10, 1, 30, 10,  9, 0, IRON, HI_METAL,
                                                        HELMET),
HELM("helm of brilliance", "etched helmet",
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, CLR_GREEN,
                                                        HELM_OF_BRILLIANCE),
HELM("helm of opposite alignment", "crested helmet",
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, HI_METAL,
                                                 HELM_OF_OPPOSITE_ALIGNMENT),
HELM("helm of telepathy", "visored helmet",
     0, 1,     TELEPAT,  2, 1, 50, 50,  9, 0, IRON, HI_METAL,
                                                 HELM_OF_TELEPATHY),

/* suits of armor */
/*
 * There is code in polyself.c that assumes (1) and (2).
 * There is code in obj.h, objnam.c, mon.c, read.c that assumes (2).
 *      (1) The dragon scale mails and the dragon scales are together.
 *      (2) That the order of the dragon scale mail and dragon scales
 *          is the same as order of dragons defined in monst.c.
 */
#define DRGN_ARMR(name,mgc,power,cost,ac,color,snam)  \
    ARMOR(name, NoDes, 1, mgc, 1, power, 0, 5, 40,  \
          cost, ac, 0, ARM_SUIT, DRAGON_HIDE, color,snam)
/* 3.4.1: dragon scale mail reclassified as "magic" since magic is
   needed to create them */
DRGN_ARMR("gray dragon scale mail",    1, ANTIMAGIC,  1200, 1, CLR_GRAY,
                                                    GRAY_DRAGON_SCALE_MAIL),
    /* gold DSM is a light source; there's no property for that */
DRGN_ARMR("gold dragon scale mail",    1, 0,           900, 1, HI_GOLD,
                                                    GOLD_DRAGON_SCALE_MAIL),
DRGN_ARMR("silver dragon scale mail",  1, REFLECTING, 1200, 1, DRAGON_SILVER,
                                                    SILVER_DRAGON_SCALE_MAIL),
#if 0 /* DEFERRED */
DRGN_ARMR("shimmering dragon scale mail", 1, DISPLACED, 1200, 1, CLR_CYAN,
                                                SHIMMERING_DRAGON_SCALE_MAIL),
#endif
DRGN_ARMR("red dragon scale mail",     1, FIRE_RES,    900, 1, CLR_RED,
                                                    RED_DRAGON_SCALE_MAIL),
DRGN_ARMR("white dragon scale mail",   1, COLD_RES,    900, 1, CLR_WHITE,
                                                    WHITE_DRAGON_SCALE_MAIL),
DRGN_ARMR("orange dragon scale mail",  1, SLEEP_RES,   900, 1, CLR_ORANGE,
                                                    ORANGE_DRAGON_SCALE_MAIL),
DRGN_ARMR("black dragon scale mail",   1, DISINT_RES, 1200, 1, CLR_BLACK,
                                                    BLACK_DRAGON_SCALE_MAIL),
DRGN_ARMR("blue dragon scale mail",    1, SHOCK_RES,   900, 1, CLR_BLUE,
                                                    BLUE_DRAGON_SCALE_MAIL),
DRGN_ARMR("green dragon scale mail",   1, POISON_RES,  900, 1, CLR_GREEN,
                                                    GREEN_DRAGON_SCALE_MAIL),
DRGN_ARMR("yellow dragon scale mail",  1, ACID_RES,    900, 1, CLR_YELLOW,
                                                    YELLOW_DRAGON_SCALE_MAIL),
/* For now, only dragons leave these. */
/* 3.4.1: dragon scales left classified as "non-magic"; they confer magical
   properties but are produced "naturally"; affects use as polypile fodder */
DRGN_ARMR("gray dragon scales",        0, ANTIMAGIC,   700, 7, CLR_GRAY,
                                                        GRAY_DRAGON_SCALES),
DRGN_ARMR("gold dragon scales",        0, 0,           500, 7, HI_GOLD,
                                                        GOLD_DRAGON_SCALES),
DRGN_ARMR("silver dragon scales",      0, REFLECTING,  700, 7, DRAGON_SILVER,
                                                        SILVER_DRAGON_SCALES),
#if 0 /* DEFERRED */
DRGN_ARMR("shimmering dragon scales",  0, DISPLACED,   700, 7, CLR_CYAN,
                                                    SHIMMERING_DRAGON_SCALES),
#endif
DRGN_ARMR("red dragon scales",         0, FIRE_RES,    500, 7, CLR_RED,
                                                        RED_DRAGON_SCALES),
DRGN_ARMR("white dragon scales",       0, COLD_RES,    500, 7, CLR_WHITE,
                                                        WHITE_DRAGON_SCALES),
DRGN_ARMR("orange dragon scales",      0, SLEEP_RES,   500, 7, CLR_ORANGE,
                                                        ORANGE_DRAGON_SCALES),
DRGN_ARMR("black dragon scales",       0, DISINT_RES,  700, 7, CLR_BLACK,
                                                        BLACK_DRAGON_SCALES),
DRGN_ARMR("blue dragon scales",        0, SHOCK_RES,   500, 7, CLR_BLUE,
                                                        BLUE_DRAGON_SCALES),
DRGN_ARMR("green dragon scales",       0, POISON_RES,  500, 7, CLR_GREEN,
                                                        GREEN_DRAGON_SCALES),
DRGN_ARMR("yellow dragon scales",      0, ACID_RES,    500, 7, CLR_YELLOW,
                                                        YELLOW_DRAGON_SCALES),
#undef DRGN_ARMR
/* other suits */
ARMOR("plate mail", NoDes,
      1, 0, 1,  0, 44, 5, 450, 600,  3, 2,  ARM_SUIT, IRON, HI_METAL,
                                                        PLATE_MAIL),
ARMOR("crystal plate mail", NoDes,
      1, 0, 1,  0, 10, 5, 450, 820,  3, 2,  ARM_SUIT, GLASS, CLR_WHITE,
                                                        CRYSTAL_PLATE_MAIL),
ARMOR("bronze plate mail", NoDes,
      1, 0, 1,  0, 25, 5, 450, 400,  4, 1,  ARM_SUIT, COPPER, HI_COPPER,
                                                        BRONZE_PLATE_MAIL),
ARMOR("splint mail", NoDes,
      1, 0, 1,  0, 62, 5, 400,  80,  4, 1,  ARM_SUIT, IRON, HI_METAL,
                                                        SPLINT_MAIL),
ARMOR("banded mail", NoDes,
      1, 0, 1,  0, 72, 5, 350,  90,  4, 1,  ARM_SUIT, IRON, HI_METAL,
                                                        BANDED_MAIL),
ARMOR("dwarvish mithril-coat", NoDes,
      1, 0, 0,  0, 10, 1, 150, 240,  4, 2,  ARM_SUIT, MITHRIL, HI_SILVER,
                                                        DWARVISH_MITHRIL_COAT),
ARMOR("elven mithril-coat", NoDes,
      1, 0, 0,  0, 15, 1, 150, 240,  5, 2,  ARM_SUIT, MITHRIL, HI_SILVER,
                                                        ELVEN_MITHRIL_COAT),
ARMOR("chain mail", NoDes,
      1, 0, 0,  0, 72, 5, 300,  75,  5, 1,  ARM_SUIT, IRON, HI_METAL,
                                                        CHAIN_MAIL),
ARMOR("orcish chain mail", "crude chain mail",
      0, 0, 0,  0, 20, 5, 300,  75,  6, 1,  ARM_SUIT, IRON, CLR_BLACK,
                                                        ORCISH_CHAIN_MAIL),
ARMOR("scale mail", NoDes,
      1, 0, 0,  0, 72, 5, 250,  45,  6, 1,  ARM_SUIT, IRON, HI_METAL,
                                                        SCALE_MAIL),
ARMOR("studded leather armor", NoDes,
      1, 0, 0,  0, 72, 3, 200,  15,  7, 1,  ARM_SUIT, LEATHER, HI_LEATHER,
                                                        STUDDED_LEATHER_ARMOR),
ARMOR("ring mail", NoDes,
      1, 0, 0,  0, 72, 5, 250, 100,  7, 1,  ARM_SUIT, IRON, HI_METAL,
                                                        RING_MAIL),
ARMOR("orcish ring mail", "crude ring mail",
      0, 0, 0,  0, 20, 5, 250,  80,  8, 1,  ARM_SUIT, IRON, CLR_BLACK,
                                                        ORCISH_RING_MAIL),
ARMOR("leather armor", NoDes,
      1, 0, 0,  0, 82, 3, 150,   5,  8, 1,  ARM_SUIT, LEATHER, HI_LEATHER,
                                                        LEATHER_ARMOR),
ARMOR("leather jacket", NoDes,
      1, 0, 0,  0, 12, 0,  30,  10,  9, 0,  ARM_SUIT, LEATHER, CLR_BLACK,
                                                        LEATHER_JACKET),

/* shirts */
ARMOR("Hawaiian shirt", NoDes,
      1, 0, 0,  0,  8, 0,   5,   3, 10, 0,  ARM_SHIRT, CLOTH, CLR_MAGENTA,
                                                        HAWAIIAN_SHIRT),
ARMOR("T-shirt", NoDes,
      1, 0, 0,  0,  2, 0,   5,   2, 10, 0,  ARM_SHIRT, CLOTH, CLR_WHITE,
                                                        T_SHIRT),

/* cloaks */
CLOAK("mummy wrapping", NoDes,
      1, 0,          0,  0, 0,  3,  2, 10, 1,  CLOTH, CLR_GRAY, MUMMY_WRAPPING),
        /* worn mummy wrapping blocks invisibility */
CLOAK("elven cloak", "faded pall",
      0, 1,    STEALTH,  8, 0, 10, 60,  9, 1,  CLOTH, CLR_BLACK, ELVEN_CLOAK),
CLOAK("orcish cloak", "coarse mantelet",
      0, 0,          0,  8, 0, 10, 40, 10, 1,  CLOTH, CLR_BLACK, ORCISH_CLOAK),
CLOAK("dwarvish cloak", "hooded cloak",
      0, 0,          0,  8, 0, 10, 50, 10, 1,  CLOTH, HI_CLOTH, DWARVISH_CLOAK),
CLOAK("oilskin cloak", "slippery cloak",
      0, 0,          0,  8, 0, 10, 50,  9, 2,  CLOTH, HI_CLOTH, OILSKIN_CLOAK),
CLOAK("robe", NoDes,
      1, 1,          0,  3, 0, 15, 50,  8, 2,  CLOTH, CLR_RED, ROBE),
        /* robe was adopted from slash'em, where it's worn as a suit
           rather than as a cloak and there are several variations */
CLOAK("alchemy smock", "apron",
      0, 1, POISON_RES,  9, 0, 10, 50,  9, 1,  CLOTH, CLR_WHITE, ALCHEMY_SMOCK),
CLOAK("leather cloak", NoDes,
      1, 0,          0,  8, 0, 15, 40,  9, 1,  LEATHER, CLR_BROWN, LEATHER_CLOAK),
/* with shuffled appearances... */
CLOAK("cloak of protection", "tattered cape",
      0, 1, PROTECTION,  9, 0, 10, 50,  7, 3,  CLOTH, HI_CLOTH, CLOAK_OF_PROTECTION),
        /* cloak of protection is now the only item conferring MC 3 */
CLOAK("cloak of invisibility", "opera cloak",
      0, 1,      INVIS, 10, 0, 10, 60,  9, 1,  CLOTH, CLR_BRIGHT_MAGENTA,
                                                        CLOAK_OF_INVISIBILITY),
CLOAK("cloak of magic resistance", "ornamental cope",
      0, 1,  ANTIMAGIC,  2, 0, 10, 60,  9, 1,  CLOTH, CLR_WHITE,
                                                        CLOAK_OF_MAGIC_RESISTANCE),
        /*  'cope' is not a spelling mistake... leave it be */
CLOAK("cloak of displacement", "piece of cloth",
      0, 1,  DISPLACED, 10, 0, 10, 50,  9, 1,  CLOTH, HI_CLOTH,
                                                        CLOAK_OF_DISPLACEMENT),

/* shields */
SHIELD("small shield", NoDes,
       1, 0, 0,          0, 6, 0,  30,  3, 9, 0,  WOOD, HI_WOOD,
                                                        SMALL_SHIELD),
SHIELD("elven shield", "blue and green shield",
       0, 0, 0,          0, 2, 0,  40,  7, 8, 0,  WOOD, CLR_GREEN,
                                                        ELVEN_SHIELD),
SHIELD("Uruk-hai shield", "white-handed shield",
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, HI_METAL,
                                                        URUK_HAI_SHIELD),
SHIELD("orcish shield", "red-eyed shield",
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, CLR_RED,
                                                        ORCISH_SHIELD),
SHIELD("large shield", NoDes,
       1, 0, 1,          0, 7, 0, 100, 10, 8, 0,  IRON, HI_METAL,
                                                        LARGE_SHIELD),
SHIELD("dwarvish roundshield", "large round shield",
       0, 0, 0,          0, 4, 0, 100, 10, 8, 0,  IRON, HI_METAL,
                                                        DWARVISH_ROUNDSHIELD),
SHIELD("shield of reflection", "polished silver shield",
       0, 1, 0, REFLECTING, 3, 0,  50, 50, 8, 0,  SILVER, HI_SILVER,
                                                        SHIELD_OF_REFLECTION),

/* gloves */
/* These have their color but not material shuffled, so the IRON must
 * stay CLR_BROWN (== HI_LEATHER) even though it's normally either
 * HI_METAL or CLR_BLACK.  All have shuffled descriptions.
 */
GLOVES("leather gloves", "old gloves",
       0, 0,        0, 16, 1, 10,  8, 9, 0,  LEATHER, HI_LEATHER,
                                                        LEATHER_GLOVES),
GLOVES("gauntlets of fumbling", "padded gloves",
       0, 1, FUMBLING,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER,
                                                    GAUNTLETS_OF_FUMBLING),
GLOVES("gauntlets of power", "riding gloves",
       0, 1,        0,  8, 1, 30, 50, 9, 0,  IRON, CLR_BROWN,
                                                    GAUNTLETS_OF_POWER),
GLOVES("gauntlets of dexterity", "fencing gloves",
       0, 1,        0,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER,
                                                    GAUNTLETS_OF_DEXTERITY),

/* boots */
BOOTS("low boots", "walking shoes",
      0, 0,          0, 25, 2, 10,  8, 9, 0, LEATHER, HI_LEATHER, LOW_BOOTS),
BOOTS("iron shoes", "hard shoes",
      0, 0,          0,  7, 2, 50, 16, 8, 0, IRON, HI_METAL, IRON_SHOES),
BOOTS("high boots", "jackboots",
      0, 0,          0, 15, 2, 20, 12, 8, 0, LEATHER, HI_LEATHER, HIGH_BOOTS),
/* with shuffled appearances... */
BOOTS("speed boots", "combat boots",
      0, 1,       FAST, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER, SPEED_BOOTS),
BOOTS("water walking boots", "jungle boots",
      0, 1,   WWALKING, 12, 2, 15, 50, 9, 0, LEATHER, HI_LEATHER,
                                                        WATER_WALKING_BOOTS),
BOOTS("jumping boots", "hiking boots",
      0, 1,    JUMPING, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER,
                                                        JUMPING_BOOTS),
BOOTS("elven boots", "mud boots",
      0, 1,    STEALTH, 12, 2, 15,  8, 9, 0, LEATHER, HI_LEATHER,
                                                        ELVEN_BOOTS),
BOOTS("kicking boots", "buckled boots",
      0, 1,          0, 12, 2, 50,  8, 9, 0, IRON, CLR_BROWN,
                                                        KICKING_BOOTS),
        /* CLR_BROWN for same reason as gauntlets of power */
BOOTS("fumble boots", "riding boots",
      0, 1,   FUMBLING, 12, 2, 20, 30, 9, 0, LEATHER, HI_LEATHER,
                                                        FUMBLE_BOOTS),
BOOTS("levitation boots", "snow boots",
      0, 1, LEVITATION, 12, 2, 15, 30, 9, 0, LEATHER, HI_LEATHER,
                                                        LEVITATION_BOOTS),
#undef HELM
#undef CLOAK
#undef SHIELD
#undef GLOVES
#undef BOOTS
#undef ARMOR

/* rings ... */
#define RING(name,stone,power,cost,mgc,spec,mohs,metal,color,sn) \
    OBJECT(OBJ(name, stone),                                          \
           BITS(0, 0, spec, 0, mgc, spec, 0, 0, 0,                    \
                HARDGEM(mohs), 0, P_NONE, metal),                     \
           power, RING_CLASS, 1, 0, 3, cost, 0, 0, 0, 0, 15, color,sn)
RING("adornment", "wooden",
     ADORNED,                  100, 1, 1, 2, WOOD, HI_WOOD, RIN_ADORNMENT),
RING("gain strength", "granite",
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL,
                                                            RIN_GAIN_STRENGTH),
RING("gain constitution", "opal",
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL,
                                                        RIN_GAIN_CONSTITUTION),
RING("increase accuracy", "clay",
     0,                        150, 1, 1, 4, MINERAL, CLR_RED,
                                                        RIN_INCREASE_ACCURACY),
RING("increase damage", "coral",
     0,                        150, 1, 1, 4, MINERAL, CLR_ORANGE,
                                                        RIN_INCREASE_DAMAGE),
RING("protection", "black onyx",
     PROTECTION,               100, 1, 1, 7, MINERAL, CLR_BLACK,
                                                        RIN_PROTECTION),
        /* 'PROTECTION' intrinsic enhances MC from worn armor by +1,
           regardless of ring's enchantment; wearing a second ring of
           protection (or even one ring of protection combined with
           cloak of protection) doesn't give a second MC boost */
RING("regeneration", "moonstone",
     REGENERATION,             200, 1, 0,  6, MINERAL, HI_MINERAL,
                                                        RIN_REGENERATION),
RING("searching", "tiger eye",
     SEARCHING,                200, 1, 0,  6, GEMSTONE, CLR_BROWN,
                                                        RIN_SEARCHING  ),
RING("stealth", "jade",
     STEALTH,                  100, 1, 0,  6, GEMSTONE, CLR_GREEN,
                                                        RIN_STEALTH),
RING("sustain ability", "bronze",
     FIXED_ABIL,               100, 1, 0,  4, COPPER, HI_COPPER,
                                                        RIN_SUSTAIN_ABILITY),
RING("levitation", "agate",
     LEVITATION,               200, 1, 0,  7, GEMSTONE, CLR_RED,
                                                        RIN_LEVITATION),
RING("hunger", "topaz",
     HUNGER,                   100, 1, 0,  8, GEMSTONE, CLR_CYAN,
                                                        RIN_HUNGER),
RING("aggravate monster", "sapphire",
     AGGRAVATE_MONSTER,        150, 1, 0,  9, GEMSTONE, CLR_BLUE,
                                                        RIN_AGGRAVATE_MONSTER),
RING("conflict", "ruby",
     CONFLICT,                 300, 1, 0,  9, GEMSTONE, CLR_RED,
                                                        RIN_CONFLICT),
RING("warning", "diamond",
     WARNING,                  100, 1, 0, 10, GEMSTONE, CLR_WHITE,
                                                        RIN_WARNING),
RING("poison resistance", "pearl",
     POISON_RES,               150, 1, 0,  4, BONE, CLR_WHITE,
                                                        RIN_POISON_RESISTANCE),
RING("fire resistance", "iron",
     FIRE_RES,                 200, 1, 0,  5, IRON, HI_METAL,
                                                        RIN_FIRE_RESISTANCE),
RING("cold resistance", "brass",
     COLD_RES,                 150, 1, 0,  4, COPPER, HI_COPPER,
                                                        RIN_COLD_RESISTANCE),
RING("shock resistance", "copper",
     SHOCK_RES,                150, 1, 0,  3, COPPER, HI_COPPER,
                                                        RIN_SHOCK_RESISTANCE),
RING("free action", "twisted",
     FREE_ACTION,              200, 1, 0,  6, IRON, HI_METAL,
                                                        RIN_FREE_ACTION),
RING("slow digestion", "steel",
     SLOW_DIGESTION,           200, 1, 0,  8, IRON, HI_METAL,
                                                        RIN_SLOW_DIGESTION),
RING("teleportation", "silver",
     TELEPORT,                 200, 1, 0,  3, SILVER, HI_SILVER,
                                                        RIN_TELEPORTATION),
RING("teleport control", "gold",
     TELEPORT_CONTROL,         300, 1, 0,  3, GOLD, HI_GOLD,
                                                        RIN_TELEPORT_CONTROL),
RING("polymorph", "ivory",
     POLYMORPH,                300, 1, 0,  4, BONE, CLR_WHITE,
                                                        RIN_POLYMORPH),
RING("polymorph control", "emerald",
     POLYMORPH_CONTROL,        300, 1, 0,  8, GEMSTONE, CLR_BRIGHT_GREEN,
                                                        RIN_POLYMORPH_CONTROL),
RING("invisibility", "wire",
     INVIS,                    150, 1, 0,  5, IRON, HI_METAL,
                                                        RIN_INVISIBILITY),
RING("see invisible", "engagement",
     SEE_INVIS,                150, 1, 0,  5, IRON, HI_METAL,
                                                        RIN_SEE_INVISIBLE),
RING("protection from shape changers", "shiny",
     PROT_FROM_SHAPE_CHANGERS, 100, 1, 0,  5, IRON, CLR_BRIGHT_CYAN,
                                               RIN_PROTECTION_FROM_SHAPE_CHAN),
#undef RING

/* amulets ... - THE Amulet comes last because it is special */
#define AMULET(name,desc,power,prob,sn) \
    OBJECT(OBJ(name, desc),                                            \
           BITS(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, P_NONE, IRON),        \
           power, AMULET_CLASS, prob, 0, 20, 150, 0, 0, 0, 0, 20, HI_METAL, sn)
AMULET("amulet of ESP",                "circular", TELEPAT, 120,
                                                        AMULET_OF_ESP),
AMULET("amulet of life saving",       "spherical", LIFESAVED, 75,
                                                        AMULET_OF_LIFE_SAVING),
AMULET("amulet of strangulation",          "oval", STRANGLED, 115,
                                                        AMULET_OF_STRANGULATION),
AMULET("amulet of restful sleep",    "triangular", SLEEPY, 115,
                                                        AMULET_OF_RESTFUL_SLEEP),
AMULET("amulet versus poison",        "pyramidal", POISON_RES, 115,
                                                        AMULET_VERSUS_POISON),
AMULET("amulet of change",               "square", 0, 115,
                                                        AMULET_OF_CHANGE),
AMULET("amulet of unchanging",          "concave", UNCHANGING, 60,
                                                        AMULET_OF_UNCHANGING),
AMULET("amulet of reflection",        "hexagonal", REFLECTING, 75,
                                                        AMULET_OF_REFLECTION),
AMULET("amulet of magical breathing", "octagonal", MAGICAL_BREATHING, 75,
                                                  AMULET_OF_MAGICAL_BREATHING),
        /* +2 AC and +2 MC; +2 takes naked hero past 'warded' to 'guarded' */
AMULET("amulet of guarding",         "perforated", PROTECTION, 75,
                                                        AMULET_OF_GUARDING),
        /* cubical: some descriptions are already three dimensional and
           parallelogrammatical (real word!) would be way over the top */
AMULET("amulet of flying",              "cubical", FLYING, 60,
                                                        AMULET_OF_FLYING),
/* fixed descriptions; description duplication is deliberate;
 * fake one must come before real one because selection for
 * description shuffling stops when a non-magic amulet is encountered
 */
OBJECT(OBJ("cheap plastic imitation of the Amulet of Yendor",
           "Amulet of Yendor"),
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, PLASTIC),
       0, AMULET_CLASS, 0, 0, 20, 0, 0, 0, 0, 0, 1, HI_METAL,
                                                FAKE_AMULET_OF_YENDOR),
OBJECT(OBJ("Amulet of Yendor", /* note: description == name */
           "Amulet of Yendor"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, MITHRIL),
       0, AMULET_CLASS, 0, 0, 20, 30000, 0, 0, 0, 0, 20, HI_METAL,
                                                AMULET_OF_YENDOR),
#undef AMULET

/* tools ... */
/* tools with weapon characteristics come last */
#define TOOL(name,desc,kn,mrg,mgc,chg,prob,wt,cost,mat,color,sn) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, mrg, chg, 0, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat), \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color, sn)
#define CONTAINER(name,desc,kn,mgc,chg,prob,wt,cost,mat,color,sn) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 0, chg, 1, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat),   \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color, sn)
#define WEPTOOL(name,desc,kn,mgc,bi,prob,wt,cost,sdam,ldam,hitbon,sub,mat,clr,sn)\
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, bi, 0, hitbon, sub, mat),    \
           0, TOOL_CLASS, prob, 0, wt, cost, sdam, ldam, hitbon, 0, wt, clr, sn)
/* containers */
CONTAINER("large box",       NoDes, 1, 0, 0, 40, 350,   8, WOOD, HI_WOOD,
                                                                LARGE_BOX),
CONTAINER("chest",           NoDes, 1, 0, 0, 35, 600,  16, WOOD, HI_WOOD,
                                                                CHEST),
CONTAINER("ice box",         NoDes, 1, 0, 0,  5, 900,  42, PLASTIC, CLR_WHITE,
                                                                ICE_BOX),
CONTAINER("sack",           "bag", 0, 0, 0, 35,  15,   2, CLOTH, HI_CLOTH,
                                                                SACK),
CONTAINER("oilskin sack",   "bag", 0, 0, 0,  5,  15, 100, CLOTH, HI_CLOTH,
                                                                OILSKIN_SACK),
CONTAINER("bag of holding", "bag", 0, 1, 0, 20,  15, 100, CLOTH, HI_CLOTH,
                                                               BAG_OF_HOLDING),
CONTAINER("bag of tricks",  "bag", 0, 1, 1, 20,  15, 100, CLOTH, HI_CLOTH,
                                                                BAG_OF_TRICKS),
#undef CONTAINER

/* lock opening tools */
TOOL("skeleton key",       "key", 0, 0, 0, 0, 80,  3, 10, IRON, HI_METAL,
                                                                SKELETON_KEY),
TOOL("lock pick",           NoDes, 1, 0, 0, 0, 60,  4, 20, IRON, HI_METAL,
                                                                LOCK_PICK),
TOOL("credit card",         NoDes, 1, 0, 0, 0, 15,  1, 10, PLASTIC, CLR_WHITE,
                                                                CREDIT_CARD),
/* light sources */
TOOL("tallow candle",   "candle", 0, 1, 0, 0, 20,  2, 10, WAX, CLR_WHITE,
                                                                TALLOW_CANDLE),
TOOL("wax candle",      "candle", 0, 1, 0, 0,  5,  2, 20, WAX, CLR_WHITE,
                                                                WAX_CANDLE),
TOOL("brass lantern",       NoDes, 1, 0, 0, 0, 30, 30, 12, COPPER, CLR_YELLOW,
                                                                BRASS_LANTERN),
TOOL("oil lamp",          "lamp", 0, 0, 0, 0, 45, 20, 10, COPPER, CLR_YELLOW,
                                                                OIL_LAMP),
TOOL("magic lamp",        "lamp", 0, 0, 1, 0, 15, 20, 50, COPPER, CLR_YELLOW,
                                                                MAGIC_LAMP),
/* other tools */
TOOL("expensive camera",    NoDes, 1, 0, 0, 1, 15, 12,200, PLASTIC, CLR_BLACK,
                                                              EXPENSIVE_CAMERA),
TOOL("mirror",   "looking glass", 0, 0, 0, 0, 45, 13, 10, GLASS, HI_SILVER,
                                                                MIRROR),
TOOL("crystal ball", "glass orb", 0, 0, 1, 1, 15,150, 60, GLASS, HI_GLASS,
                                                                CRYSTAL_BALL),
TOOL("lenses",              NoDes, 1, 0, 0, 0,  5,  3, 80, GLASS, HI_GLASS,
                                                                LENSES),
TOOL("blindfold",           NoDes, 1, 0, 0, 0, 50,  2, 20, CLOTH, CLR_BLACK,
                                                                BLINDFOLD),
TOOL("towel",               NoDes, 1, 0, 0, 0, 50,  5, 50, CLOTH, CLR_MAGENTA,
                                                                TOWEL),
TOOL("saddle",              NoDes, 1, 0, 0, 0,  5,200,150, LEATHER, HI_LEATHER,
                                                                SADDLE),
TOOL("leash",               NoDes, 1, 0, 0, 0, 65, 12, 20, LEATHER, HI_LEATHER,
                                                                LEASH),
TOOL("stethoscope",         NoDes, 1, 0, 0, 0, 25,  4, 75, IRON, HI_METAL,
                                                                STETHOSCOPE),
TOOL("tinning kit",         NoDes, 1, 0, 0, 1, 15,100, 30, IRON, HI_METAL,
                                                                TINNING_KIT),
TOOL("tin opener",          NoDes, 1, 0, 0, 0, 35,  4, 30, IRON, HI_METAL,
                                                                TIN_OPENER),
TOOL("can of grease",       NoDes, 1, 0, 0, 1, 15, 15, 20, IRON, HI_METAL,
                                                                CAN_OF_GREASE),
TOOL("figurine",            NoDes, 1, 0, 1, 0, 25, 50, 80, MINERAL, HI_MINERAL,
                                                                FIGURINE),
        /* monster type specified by obj->corpsenm */
TOOL("magic marker",        NoDes, 1, 0, 1, 1, 15,  2, 50, PLASTIC, CLR_RED,
                                                                MAGIC_MARKER),
/* traps */
TOOL("land mine",           NoDes, 1, 0, 0, 0, 0, 300,180, IRON, CLR_RED,
                                                                LAND_MINE),
TOOL("beartrap",            NoDes, 1, 0, 0, 0, 0, 200, 60, IRON, HI_METAL,
                                                                BEARTRAP),
/* instruments;
   "If tin whistles are made out of tin, what do they make foghorns out of?" */
TOOL("tin whistle",    "whistle", 0, 0, 0, 0,100, 3, 10, METAL, HI_METAL,
                                                                TIN_WHISTLE),
TOOL("magic whistle",  "whistle", 0, 0, 1, 0, 30, 3, 10, METAL, HI_METAL,
                                                                MAGIC_WHISTLE),
TOOL("wooden flute",     "flute", 0, 0, 0, 0,  4, 5, 12, WOOD, HI_WOOD,
                                                                WOODEN_FLUTE),
TOOL("magic flute",      "flute", 0, 0, 1, 1,  2, 5, 36, WOOD, HI_WOOD,
                                                                MAGIC_FLUTE),
TOOL("tooled horn",       "horn", 0, 0, 0, 0,  5, 18, 15, BONE, CLR_WHITE,
                                                                TOOLED_HORN),
TOOL("frost horn",        "horn", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE,
                                                                FROST_HORN),
TOOL("fire horn",         "horn", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE,
                                                                FIRE_HORN),
TOOL("horn of plenty",    "horn", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE,
                                                            HORN_OF_PLENTY),
        /* horn, but not an instrument */
TOOL("wooden harp",       "harp", 0, 0, 0, 0,  4, 30, 50, WOOD, HI_WOOD,
                                                                WOODEN_HARP),
TOOL("magic harp",        "harp", 0, 0, 1, 1,  2, 30, 50, WOOD, HI_WOOD,
                                                                MAGIC_HARP),
TOOL("bell",                NoDes, 1, 0, 0, 0,  2, 30, 50, COPPER, HI_COPPER,
                                                                BELL),
TOOL("bugle",               NoDes, 1, 0, 0, 0,  4, 10, 15, COPPER, HI_COPPER,
                                                                BUGLE),
TOOL("leather drum",      "drum", 0, 0, 0, 0,  4, 25, 25, LEATHER, HI_LEATHER,
                                                                LEATHER_DRUM),
TOOL("drum of earthquake","drum", 0, 0, 1, 1,  2, 25, 25, LEATHER, HI_LEATHER,
                                                           DRUM_OF_EARTHQUAKE),
/* tools useful as weapons */
WEPTOOL("pick-axe", NoDes,
        1, 0, 0, 20, 100,  50,  6,  3, WHACK,  P_PICK_AXE, IRON, HI_METAL,
                                                                PICK_AXE),
WEPTOOL("grappling hook", NoDes,
        1, 0, 0,  5,  30,  50,  2,  6, WHACK,  P_FLAIL,    IRON, HI_METAL,
                                                             GRAPPLING_HOOK),
WEPTOOL("unicorn horn", NoDes,
        1, 1, 1,  0,  20, 100, 12, 12, PIERCE, P_UNICORN_HORN,
                                                           BONE, CLR_WHITE,
                                                                UNICORN_HORN),
        /* 3.4.1: unicorn horn left classified as "magic" */
/* two unique tools;
 * not artifacts, despite the comment which used to be here
 */
OBJECT(OBJ("Candelabrum of Invocation", "candelabrum"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, GOLD),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 200, HI_GOLD,
                                                    CANDELABRUM_OF_INVOCATION),
OBJECT(OBJ("Bell of Opening", "silver bell"),
       BITS(0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, P_NONE, SILVER),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 50, HI_SILVER,
                                                    BELL_OF_OPENING),
#undef TOOL
#undef WEPTOOL

/* Comestibles ... */
#define FOOD(name, prob, delay, wt, unk, tin, nutrition, color, sn)     \
    OBJECT(OBJ(name, NoDes),                                       \
           BITS(1, 1, unk, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, tin), 0,     \
           FOOD_CLASS, prob, delay, wt, nutrition / 20 + 5, 0, 0, 0, 0, \
           nutrition, color, sn)
/* All types of food (except tins & corpses) must have a delay of at least 1.
 * Delay on corpses is computed and is weight dependant.
 * Domestic pets prefer tripe rations above all others.
 * Fortune cookies can be read, using them up without ingesting them.
 * Carrots improve your vision.
 * +0 tins contain monster meat.
 * +1 tins (of spinach) make you stronger (like Popeye).
 * Meatballs/sticks/rings are only created from objects via stone to flesh.
 */
/* meat */
FOOD("tripe ration",        140,  2, 10, 0, FLESH, 200, CLR_BROWN,
                                                        TRIPE_RATION),
FOOD("corpse",                0,  1,  0, 0, FLESH,   0, CLR_BROWN,
                                                        CORPSE),
FOOD("egg",                  85,  1,  1, 1, FLESH,  80, CLR_WHITE,
                                                        EGG),
FOOD("meatball",              0,  1,  1, 0, FLESH,   5, CLR_BROWN,
                                                        MEATBALL),
FOOD("meat stick",            0,  1,  1, 0, FLESH,   5, CLR_BROWN,
                                                        MEAT_STICK),
/* formerly "huge chunk of meat" */
FOOD("enormous meatball",     0, 20,400, 0, FLESH,2000, CLR_BROWN,
                                                        ENORMOUS_MEATBALL),
/* special case because it's not mergable */
OBJECT(OBJ("meat ring", NoDes),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FLESH),
       0, FOOD_CLASS, 0, 1, 5, 1, 0, 0, 0, 0, 5, CLR_BROWN, MEAT_RING),
/* pudding 'corpses' will turn into these and combine;
   must be in same order as the pudding monsters */
FOOD("glob of gray ooze",     0,  2, 20, 0, FLESH,  20, CLR_GRAY,
                                                        GLOB_OF_GRAY_OOZE),
FOOD("glob of brown pudding", 0,  2, 20, 0, FLESH,  20, CLR_BROWN,
                                                        GLOB_OF_BROWN_PUDDING),
FOOD("glob of green slime",   0,  2, 20, 0, FLESH,  20, CLR_GREEN,
                                                        GLOB_OF_GREEN_SLIME),
FOOD("glob of black pudding", 0,  2, 20, 0, FLESH,  20, CLR_BLACK,
                                                        GLOB_OF_BLACK_PUDDING),

/* fruits & veggies */
FOOD("kelp frond",            0,  1,  1, 0, VEGGY,  30, CLR_GREEN, KELP_FROND),
FOOD("eucalyptus leaf",       3,  1,  1, 0, VEGGY,   1, CLR_GREEN,
                                                             EUCALYPTUS_LEAF),
FOOD("apple",                15,  1,  2, 0, VEGGY,  50, CLR_RED, APPLE),
FOOD("orange",               10,  1,  2, 0, VEGGY,  80, CLR_ORANGE, ORANGE),
FOOD("pear",                 10,  1,  2, 0, VEGGY,  50, CLR_BRIGHT_GREEN,
                                                                    PEAR),
FOOD("melon",                10,  1,  5, 0, VEGGY, 100, CLR_BRIGHT_GREEN,
                                                                    MELON),
FOOD("banana",               10,  1,  2, 0, VEGGY,  80, CLR_YELLOW, BANANA),
FOOD("carrot",               15,  1,  2, 0, VEGGY,  50, CLR_ORANGE, CARROT),
FOOD("sprig of wolfsbane",    7,  1,  1, 0, VEGGY,  40, CLR_GREEN,
                                                          SPRIG_OF_WOLFSBANE),
FOOD("clove of garlic",       7,  1,  1, 0, VEGGY,  40, CLR_WHITE, CLOVE_OF_GARLIC),
/* name of slime mold is changed based on player's OPTION=fruit:something
   and bones data might have differently named ones from prior games */
FOOD("slime mold",           75,  1,  5, 0, VEGGY, 250, HI_ORGANIC, SLIME_MOLD),

/* people food */
FOOD("lump of royal jelly",   0,  1,  2, 0, VEGGY, 200, CLR_YELLOW,
                                                        LUMP_OF_ROYAL_JELLY),
FOOD("cream pie",            25,  1, 10, 0, VEGGY, 100, CLR_WHITE, CREAM_PIE),
FOOD("candy bar",            13,  1,  2, 0, VEGGY, 100, CLR_BRIGHT_BLUE, CANDY_BAR),
FOOD("fortune cookie",       55,  1,  1, 0, VEGGY,  40, CLR_YELLOW,
                                                              FORTUNE_COOKIE),
FOOD("pancake",              25,  2,  2, 0, VEGGY, 200, CLR_YELLOW, PANCAKE),
FOOD("lembas wafer",         20,  2,  5, 0, VEGGY, 800, CLR_WHITE,
                                                                LEMBAS_WAFER),
FOOD("cram ration",          20,  3, 15, 0, VEGGY, 600, HI_ORGANIC,
                                                                CRAM_RATION),
FOOD("food ration",         380,  5, 20, 0, VEGGY, 800, HI_ORGANIC,
                                                                FOOD_RATION),
FOOD("K-ration",              0,  1, 10, 0, VEGGY, 400, HI_ORGANIC, K_RATION),
FOOD("C-ration",              0,  1, 10, 0, VEGGY, 300, HI_ORGANIC, C_RATION),
/* tins have type specified by obj->spe (+1 for spinach, other implies
   flesh; negative specifies preparation method {homemade,boiled,&c})
   and by obj->corpsenm (type of monster flesh) */
FOOD("tin",                  75,  0, 10, 1, METAL,   0, HI_METAL, TIN),
#undef FOOD

/* potions ... */
#define POTION(name,desc,mgc,power,prob,cost,color,sn) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, GLASS),      \
           power, POTION_CLASS, prob, 0, 20, cost, 0, 0, 0, 0, 10, color, sn)
POTION("gain ability",           "ruby",  1, 0, 42, 300, CLR_RED,
                                                        POT_GAIN_ABILITY),
POTION("restore ability",        "pink",  1, 0, 40, 100, CLR_BRIGHT_MAGENTA,
                                                        POT_RESTORE_ABILITY),
POTION("confusion",            "orange",  1, CONFUSION, 42, 100, CLR_ORANGE,
                                                        POT_CONFUSION),
POTION("blindness",            "yellow",  1, BLINDED, 40, 150, CLR_YELLOW,
                                                        POT_BLINDNESS),
POTION("paralysis",           "emerald",  1, 0, 42, 300, CLR_BRIGHT_GREEN,
                                                        POT_PARALYSIS),
POTION("speed",            "dark green",  1, FAST, 42, 200, CLR_GREEN,
                                                        POT_SPEED),
POTION("levitation",             "cyan",  1, LEVITATION, 42, 200, CLR_CYAN,
                                                        POT_LEVITATION),
POTION("hallucination",      "sky blue",  1, HALLUC, 40, 100, CLR_CYAN,
                                                        POT_HALLUCINATION),
POTION("invisibility", "brilliant blue",  1, INVIS, 40, 150, CLR_BRIGHT_BLUE,
                                                        POT_INVISIBILITY),
POTION("see invisible",       "magenta",  1, SEE_INVIS, 42, 50, CLR_MAGENTA,
                                                        POT_SEE_INVISIBLE),
POTION("healing",          "purple-red",  1, 0, 57, 100, CLR_MAGENTA,
                                                        POT_HEALING),
POTION("extra healing",          "puce",  1, 0, 47, 100, CLR_RED,
                                                        POT_EXTRA_HEALING),
POTION("gain level",            "milky",  1, 0, 20, 300, CLR_WHITE,
                                                        POT_GAIN_LEVEL),
POTION("enlightenment",        "swirly",  1, 0, 20, 200, CLR_BROWN,
                                                        POT_ENLIGHTENMENT),
POTION("monster detection",    "bubbly",  1, 0, 40, 150, CLR_WHITE,
                                                        POT_MONSTER_DETECTION),
POTION("object detection",      "smoky",  1, 0, 42, 150, CLR_GRAY,
                                                        POT_OBJECT_DETECTION),
POTION("gain energy",          "cloudy",  1, 0, 42, 150, CLR_WHITE,
                                                        POT_GAIN_ENERGY),
POTION("sleeping",       "effervescent",  1, 0, 42, 100, CLR_GRAY,
                                                        POT_SLEEPING),
POTION("full healing",          "black",  1, 0, 10, 200, CLR_BLACK,
                                                        POT_FULL_HEALING),
POTION("polymorph",            "golden",  1, 0, 10, 200, CLR_YELLOW,
                                                        POT_POLYMORPH),
POTION("booze",                 "brown",  0, 0, 42,  50, CLR_BROWN,
                                                        POT_BOOZE),
POTION("sickness",              "fizzy",  0, 0, 42,  50, CLR_CYAN,
                                                        POT_SICKNESS),
POTION("fruit juice",            "dark",  0, 0, 42,  50, CLR_BLACK,
                                                        POT_FRUIT_JUICE),
POTION("acid",                  "white",  0, 0, 10, 250, CLR_WHITE,
                                                        POT_ACID),
POTION("oil",                   "murky",  0, 0, 30, 250, CLR_BROWN,
                                                        POT_OIL),
/* fixed description
 */
POTION("water",                 "clear",  0, 0, 92, 100, CLR_CYAN,
                                                        POT_WATER),
#undef POTION

/* scrolls ... */
#define SCROLL(name,text,mgc,prob,cost,sn) \
    OBJECT(OBJ(name, text),                                           \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, PAPER),    \
           0, SCROLL_CLASS, prob, 0, 5, cost, 0, 0, 0, 0, 6, \
           HI_PAPER, sn)
SCROLL("enchant armor",              "ZELGO MER",  1,  63,  80,
                                                        SCR_ENCHANT_ARMOR),
SCROLL("destroy armor",         "JUYED AWK YACC",  1,  45, 100,
                                                        SCR_DESTROY_ARMOR),
SCROLL("confuse monster",                 "NR 9",  1,  53, 100,
                                                        SCR_CONFUSE_MONSTER),
SCROLL("scare monster",   "XIXAXA XOXAXA XUXAXA",  1,  35, 100,
                                                        SCR_SCARE_MONSTER),
SCROLL("remove curse",             "PRATYAVAYAH",  1,  65,  80,
                                                        SCR_REMOVE_CURSE),
SCROLL("enchant weapon",         "DAIYEN FOOELS",  1,  80,  60,
                                                        SCR_ENCHANT_WEAPON),
SCROLL("create monster",       "LEP GEX VEN ZEA",  1,  45, 200,
                                                        SCR_CREATE_MONSTER),
SCROLL("taming",                   "PRIRUTSENIE",  1,  15, 200,
                                                        SCR_TAMING),
SCROLL("genocide",                  "ELBIB YLOH",  1,  15, 300,
                                                        SCR_GENOCIDE),
SCROLL("light",                 "VERR YED HORRE",  1,  90,  50,
                                                        SCR_LIGHT),
SCROLL("teleportation",        "VENZAR BORGAVVE",  1,  55, 100,
                                                        SCR_TELEPORTATION),
SCROLL("gold detection",                 "THARR",  1,  33, 100,
                                                        SCR_GOLD_DETECTION),
SCROLL("food detection",               "YUM YUM",  1,  25, 100,
                                                        SCR_FOOD_DETECTION),
SCROLL("identify",                  "KERNOD WEL",  1, 180,  20,
                                                        SCR_IDENTIFY),
SCROLL("magic mapping",              "ELAM EBOW",  1,  45, 100,
                                                        SCR_MAGIC_MAPPING),
SCROLL("amnesia",                   "DUAM XNAHT",  1,  35, 200,
                                                        SCR_AMNESIA),
SCROLL("fire",                  "ANDOVA BEGARIN",  1,  30, 100,
                                                        SCR_FIRE),
SCROLL("earth",                          "KIRJE",  1,  18, 200,
                                                        SCR_EARTH),
SCROLL("punishment",            "VE FORBRYDERNE",  1,  15, 300,
                                                        SCR_PUNISHMENT),
SCROLL("charging",                "HACKEM MUCHE",  1,  15, 300,
                                                        SCR_CHARGING),
SCROLL("stinking cloud",             "VELOX NEB",  1,  15, 300,
                                                        SCR_STINKING_CLOUD),
    /* Extra descriptions, shuffled into use at start of new game.
     * Code in win/share/tilemap.c depends on SCR_STINKING_CLOUD preceding
     * these and on how many of them there are.  If a real scroll gets added
     * after stinking cloud or the number of extra descriptions changes,
     * tilemap.c must be modified to match.
     */
SCROLL(NoDes,      "FOOBIE BLETCH",  1,   0, 100, SC01),
SCROLL(NoDes,              "TEMOV",  1,   0, 100, SC02),
SCROLL(NoDes,         "GARVEN DEH",  1,   0, 100, SC03),
SCROLL(NoDes,            "READ ME",  1,   0, 100, SC04),
SCROLL(NoDes,      "ETAOIN SHRDLU",  1,   0, 100, SC05),
SCROLL(NoDes,        "LOREM IPSUM",  1,   0, 100, SC06),
SCROLL(NoDes,              "FNORD",  1,   0, 100, SC07), /* Illuminati */
SCROLL(NoDes,            "KO BATE",  1,   0, 100, SC08), /* Kurd Lasswitz */
SCROLL(NoDes,      "ABRA KA DABRA",  1,   0, 100, SC09), /* traditional incantation */
SCROLL(NoDes,       "ASHPD SODALG",  1,   0, 100, SC10), /* Portal */
SCROLL(NoDes,            "ZLORFIK",  1,   0, 100, SC11), /* Zak McKracken */
SCROLL(NoDes,      "GNIK SISI VLE",  1,   0, 100, SC12), /* Zak McKracken */
SCROLL(NoDes,    "HAPAX LEGOMENON",  1,   0, 100, SC13),
SCROLL(NoDes,  "EIRIS SAZUN IDISI",  1,   0, 100, SC14), /* Merseburg Incantations */
SCROLL(NoDes,    "PHOL ENDE WODAN",  1,   0, 100, SC15), /* Merseburg Incantations */
SCROLL(NoDes,              "GHOTI",  1,   0, 100, SC16), /* pronounced as 'fish',
                                                          George Bernard Shaw */
SCROLL(NoDes, "MAPIRO MAHAMA DIROMAT", 1, 0, 100, SC17), /* Wizardry */
SCROLL(NoDes,  "VAS CORP BET MANI",  1,   0, 100, SC18), /* Ultima */
SCROLL(NoDes,            "XOR OTA",  1,   0, 100, SC19), /* Aarne Haapakoski */
SCROLL(NoDes, "STRC PRST SKRZ KRK",  1,   0, 100, SC20), /* Czech and Slovak
                                                             tongue-twister */
    /* These must come last because they have special fixed descriptions.
     */
#ifdef MAIL_STRUCTURES
SCROLL("mail",          "stamped",  0,   0,   0, SCR_MAIL),
#endif
SCROLL("blank paper", "unlabeled",  0,  28,  60, SCR_BLANK_PAPER),
#undef SCROLL

/* spellbooks ... */
    /* Expanding beyond 52 spells would require changes in spellcasting
     * or imposition of a limit on number of spells hero can know because
     * they are currently assigned successive letters, a-zA-Z, when learned.
     * [The existing spell sorting capability could conceivably be extended
     * to enable moving spells from beyond Z to within it, bumping others
     * out in the process, allowing more than 52 spells be known but keeping
     * only 52 be castable at any given time.]
     */
#define SPELL(name,desc,sub,prob,delay,level,mgc,dir,color,sn)  \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 0, 0, 0, mgc, 0, 0, 0, 0, 0, dir, sub, PAPER),       \
           0, SPBOOK_CLASS, prob, delay, 50, level * 100,               \
           0, 0, 0, level, 20, color, sn)
/* Spellbook description normally refers to book covers (primarily color).
   Parchment and vellum would never be used for such, but rather than
   eliminate those, finagle their definitions to refer to the pages
   rather than the cover.  They are made from animal skin (typically of
   a goat or sheep) and books using them for pages generally need heavy
   covers with straps or clamps to tightly close the book in order to
   keep the pages flat.  (However, a wooden cover might itself be covered
   by a sheet of parchment, making this become less of an exception.  Also,
   changing the internal composition from paper to leather makes eating a
   parchment or vellum spellbook break vegetarian conduct, as it should.) */
#define PAPER LEATHER /* override enum for use in SPELL() expansion */
SPELL("dig",             "parchment",
      P_MATTER_SPELL,      20,  6, 5, 1, RAY, HI_LEATHER, SPE_DIG),
/* magic missile ... finger of death must be in this order; see buzz() */
SPELL("magic missile",   "vellum",
      P_ATTACK_SPELL,      45,  2, 2, 1, RAY, HI_LEATHER, SPE_MAGIC_MISSILE),
#undef PAPER /* revert to normal material */
SPELL("fireball",        "ragged",
      P_ATTACK_SPELL,      20,  4, 4, 1, RAY, HI_PAPER, SPE_FIREBALL),
SPELL("cone of cold",    "dog eared",
      P_ATTACK_SPELL,      10,  7, 4, 1, RAY, HI_PAPER, SPE_CONE_OF_COLD),
SPELL("sleep",           "mottled",
      P_ENCHANTMENT_SPELL, 30,  1, 3, 1, RAY, HI_PAPER, SPE_SLEEP),
SPELL("finger of death", "stained",
      P_ATTACK_SPELL,       5, 10, 7, 1, RAY, HI_PAPER, SPE_FINGER_OF_DEATH),
SPELL("light",           "cloth",
      P_DIVINATION_SPELL,  45,  1, 1, 1, NODIR, HI_CLOTH, SPE_LIGHT),
SPELL("detect monsters", "leathery",
      P_DIVINATION_SPELL,  43,  1, 1, 1, NODIR, HI_LEATHER,
                                                        SPE_DETECT_MONSTERS),
SPELL("healing",         "white",
      P_HEALING_SPELL,     40,  2, 1, 1, IMMEDIATE, CLR_WHITE,
                                                        SPE_HEALING),
SPELL("knock",           "pink",
      P_MATTER_SPELL,      35,  1, 1, 1, IMMEDIATE, CLR_BRIGHT_MAGENTA,
                                                        SPE_KNOCK),
SPELL("force bolt",      "red",
      P_ATTACK_SPELL,      35,  2, 1, 1, IMMEDIATE, CLR_RED,
                                                        SPE_FORCE_BOLT),
SPELL("confuse monster", "orange",
      P_ENCHANTMENT_SPELL, 49,  2, 1, 1, IMMEDIATE, CLR_ORANGE,
                                                        SPE_CONFUSE_MONSTER),
SPELL("cure blindness",  "yellow",
      P_HEALING_SPELL,     25,  2, 2, 1, IMMEDIATE, CLR_YELLOW,
                                                        SPE_CURE_BLINDNESS),
SPELL("drain life",      "velvet",
      P_ATTACK_SPELL,      10,  2, 2, 1, IMMEDIATE, CLR_MAGENTA,
                                                        SPE_DRAIN_LIFE),
SPELL("slow monster",    "light green",
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_BRIGHT_GREEN,
                                                        SPE_SLOW_MONSTER),
SPELL("wizard lock",     "dark green",
      P_MATTER_SPELL,      30,  3, 2, 1, IMMEDIATE, CLR_GREEN,
                                                        SPE_WIZARD_LOCK),
SPELL("create monster",  "turquoise",
      P_CLERIC_SPELL,      35,  3, 2, 1, NODIR, CLR_BRIGHT_CYAN,
                                                        SPE_CREATE_MONSTER),
SPELL("detect food",     "cyan",
      P_DIVINATION_SPELL,  30,  3, 2, 1, NODIR, CLR_CYAN,
                                                        SPE_DETECT_FOOD),
SPELL("cause fear",      "light blue",
      P_ENCHANTMENT_SPELL, 25,  3, 3, 1, NODIR, CLR_BRIGHT_BLUE,
                                                        SPE_CAUSE_FEAR),
SPELL("clairvoyance",    "dark blue",
      P_DIVINATION_SPELL,  15,  3, 3, 1, NODIR, CLR_BLUE,
                                                        SPE_CLAIRVOYANCE),
SPELL("cure sickness",   "indigo",
      P_HEALING_SPELL,     32,  3, 3, 1, NODIR, CLR_BLUE,
                                                        SPE_CURE_SICKNESS),
SPELL("charm monster",   "magenta",
      P_ENCHANTMENT_SPELL, 20,  3, 5, 1, IMMEDIATE, CLR_MAGENTA,
                                                        SPE_CHARM_MONSTER),
SPELL("haste self",      "purple",
      P_ESCAPE_SPELL,      33,  4, 3, 1, NODIR, CLR_MAGENTA,
                                                        SPE_HASTE_SELF),
SPELL("detect unseen",   "violet",
      P_DIVINATION_SPELL,  20,  4, 3, 1, NODIR, CLR_MAGENTA,
                                                        SPE_DETECT_UNSEEN),
SPELL("levitation",      "tan",
      P_ESCAPE_SPELL,      20,  4, 4, 1, NODIR, CLR_BROWN,
                                                        SPE_LEVITATION),
SPELL("extra healing",   "plaid",
      P_HEALING_SPELL,     27,  5, 3, 1, IMMEDIATE, CLR_GREEN,
                                                        SPE_EXTRA_HEALING),
SPELL("restore ability", "light brown",
      P_HEALING_SPELL,     25,  5, 4, 1, NODIR, CLR_BROWN,
                                                        SPE_RESTORE_ABILITY),
SPELL("invisibility",    "dark brown",
      P_ESCAPE_SPELL,      25,  5, 4, 1, NODIR, CLR_BROWN,
                                                        SPE_INVISIBILITY),
SPELL("detect treasure", "gray",
      P_DIVINATION_SPELL,  20,  5, 4, 1, NODIR, CLR_GRAY,
                                                        SPE_DETECT_TREASURE),
SPELL("remove curse",    "wrinkled",
      P_CLERIC_SPELL,      25,  5, 3, 1, NODIR, HI_PAPER,
                                                        SPE_REMOVE_CURSE),
SPELL("magic mapping",   "dusty",
      P_DIVINATION_SPELL,  18,  7, 5, 1, NODIR, HI_PAPER,
                                                        SPE_MAGIC_MAPPING),
SPELL("identify",        "bronze",
      P_DIVINATION_SPELL,  20,  6, 3, 1, NODIR, HI_COPPER,
                                                        SPE_IDENTIFY),
SPELL("turn undead",     "copper",
      P_CLERIC_SPELL,      16,  8, 6, 1, IMMEDIATE, HI_COPPER,
                                                        SPE_TURN_UNDEAD),
SPELL("polymorph",       "silver",
      P_MATTER_SPELL,      10,  8, 6, 1, IMMEDIATE, HI_SILVER,
                                                        SPE_POLYMORPH),
SPELL("teleport away",   "gold",
      P_ESCAPE_SPELL,      15,  6, 6, 1, IMMEDIATE, HI_GOLD,
                                                        SPE_TELEPORT_AWAY),
SPELL("create familiar", "glittering",
      P_CLERIC_SPELL,      10,  7, 6, 1, NODIR, CLR_WHITE,
                                                        SPE_CREATE_FAMILIAR),
SPELL("cancellation",    "shining",
      P_MATTER_SPELL,      15,  8, 7, 1, IMMEDIATE, CLR_WHITE,
                                                        SPE_CANCELLATION),
SPELL("protection",      "dull",
      P_CLERIC_SPELL,      18,  3, 1, 1, NODIR, HI_PAPER,
                                                        SPE_PROTECTION),
SPELL("jumping",         "thin",
      P_ESCAPE_SPELL,      20,  3, 1, 1, IMMEDIATE, HI_PAPER,
                                                        SPE_JUMPING),
SPELL("stone to flesh",  "thick",
      P_HEALING_SPELL,     15,  1, 3, 1, IMMEDIATE, HI_PAPER,
                                                        SPE_STONE_TO_FLESH),
#if 0 /* DEFERRED */
/* from slash'em, create a tame critter which explodes when attacking,
   damaging adjacent creatures--friend or foe--and dying in the process */
SPELL("flame sphere",    "canvas",
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN,
                                                        SPE_FLAME_SPHERE),
SPELL("freeze sphere",   "hardcover",
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN,
                                                        SPE_FREEZE_SPHERE),
#endif
/* books with fixed descriptions
 */
SPELL("blank paper", "plain", P_NONE, 18, 0, 0, 0, 0, HI_PAPER, SPE_BLANK_PAPER),
/* tribute book for 3.6 */
OBJECT(OBJ("novel", "paperback"),
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, PAPER),
       0, SPBOOK_CLASS, 1, 0, 0, 20, 0, 0, 0, 1, 20, CLR_BRIGHT_BLUE,
                                                        SPE_NOVEL),
/* a special, one of a kind, spellbook */
OBJECT(OBJ("Book of the Dead", "papyrus"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, PAPER),
       0, SPBOOK_CLASS, 0, 0, 20, 10000, 0, 0, 0, 7, 20, HI_PAPER,
                                                        SPE_BOOK_OF_THE_DEAD),
#undef SPELL

/* wands ... */
#define WAND(name,typ,prob,cost,mgc,dir,metal,color,sn) \
    OBJECT(OBJ(name, typ),                                              \
           BITS(0, 0, 1, 0, mgc, 1, 0, 0, 0, 0, dir, P_NONE, metal),    \
           0, WAND_CLASS, prob, 0, 7, cost, 0, 0, 0, 0, 30, color, sn)
WAND("light",           "glass", 95, 100, 1, NODIR, GLASS, HI_GLASS,
                                                            WAN_LIGHT),
WAND("secret door detection",
                        "balsa", 50, 150, 1, NODIR, WOOD, HI_WOOD,
                                                    WAN_SECRET_DOOR_DETECTION),
WAND("enlightenment", "crystal", 15, 150, 1, NODIR, GLASS, HI_GLASS,
                                                    WAN_ENLIGHTENMENT),
WAND("create monster",  "maple", 45, 200, 1, NODIR, WOOD, HI_WOOD,
                                                    WAN_CREATE_MONSTER),
WAND("wishing",          "pine",  5, 500, 1, NODIR, WOOD, HI_WOOD,
                                                    WAN_WISHING),
WAND("nothing",           "oak", 25, 100, 0, IMMEDIATE, WOOD, HI_WOOD,
                                                    WAN_NOTHING),
WAND("striking",        "ebony", 75, 150, 1, IMMEDIATE, WOOD, HI_WOOD,
                                                    WAN_STRIKING),
WAND("make invisible", "marble", 45, 150, 1, IMMEDIATE, MINERAL, HI_MINERAL,
                                                    WAN_MAKE_INVISIBLE),
WAND("slow monster",      "tin", 50, 150, 1, IMMEDIATE, METAL, HI_METAL,
                                                    WAN_SLOW_MONSTER),
WAND("speed monster",   "brass", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER,
                                                    WAN_SPEED_MONSTER),
WAND("undead turning", "copper", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER,
                                                    WAN_UNDEAD_TURNING),
WAND("polymorph",      "silver", 45, 200, 1, IMMEDIATE, SILVER, HI_SILVER,
                                                    WAN_POLYMORPH),
WAND("cancellation", "platinum", 45, 200, 1, IMMEDIATE, PLATINUM, CLR_WHITE,
                                                    WAN_CANCELLATION),
WAND("teleportation", "iridium", 45, 200, 1, IMMEDIATE, METAL,
                                     CLR_BRIGHT_CYAN, WAN_TELEPORTATION),
WAND("opening",          "zinc", 25, 150, 1, IMMEDIATE, METAL, HI_METAL,
                                                    WAN_OPENING),
WAND("locking",      "aluminum", 25, 150, 1, IMMEDIATE, METAL, HI_METAL,
                                                    WAN_LOCKING),
WAND("probing",       "uranium", 30, 150, 1, IMMEDIATE, METAL, HI_METAL,
                                                    WAN_PROBING),
WAND("digging",          "iron", 55, 150, 1, RAY, IRON, HI_METAL,
                                                    WAN_DIGGING),
/* magic missile ... lightning must be in this order; see buzz() */
WAND("magic missile",   "steel", 50, 150, 1, RAY, IRON, HI_METAL,
                                                    WAN_MAGIC_MISSILE),
WAND("fire",        "hexagonal", 40, 175, 1, RAY, IRON, HI_METAL,
                                                    WAN_FIRE),
WAND("cold",            "short", 40, 175, 1, RAY, IRON, HI_METAL,
                                                    WAN_COLD),
WAND("sleep",           "runed", 50, 175, 1, RAY, IRON, HI_METAL,
                                                    WAN_SLEEP),
WAND("death",            "long",  5, 500, 1, RAY, IRON, HI_METAL,
                                                    WAN_DEATH),
WAND("lightning",      "curved", 40, 175, 1, RAY, IRON, HI_METAL,
                                                    WAN_LIGHTNING),
/* extra descriptions, shuffled into use at start of new game */
WAND(NoDes,             "forked",  0, 150, 1, 0, WOOD, HI_WOOD, WAN1),
WAND(NoDes,             "spiked",  0, 150, 1, 0, IRON, HI_METAL, WAN2),
WAND(NoDes,            "jeweled",  0, 150, 1, 0, IRON, HI_MINERAL, WAN3),
#undef WAND

/* coins ... - so far, gold is all there is */
#define COIN(name,prob,metal,worth,sn) \
    OBJECT(OBJ(name, NoDes),                                         \
           BITS(1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, metal),    \
           0, COIN_CLASS, prob, 0, 1, worth, 0, 0, 0, 0, 0, HI_GOLD, sn)
COIN("gold piece", 1000, GOLD, 1, GOLD_PIECE),
#undef COIN

/* gems ... - includes stones and rocks but not boulders */
#define GEM(name,desc,prob,wt,gval,nutr,mohs,glass,color,sn) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0,                              \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, 1, gval, 3, 3, 0, 0, nutr, color, sn)
#define ROCK(name,desc,kn,prob,wt,gval,sdam,ldam,mgc,nutr,mohs,glass,color,sn) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 1, 0, 0, mgc, 0, 0, 0, 0,                           \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, wt, gval, sdam, ldam, 0, 0, nutr, color, sn)
GEM("dilithium crystal", "white",  2, 1, 4500, 15,  5, GEMSTONE, CLR_WHITE,
                                                        DILITHIUM_CRYSTAL),
GEM("diamond",           "white",  3, 1, 4000, 15, 10, GEMSTONE, CLR_WHITE,
                                                        DIAMOND),
GEM("ruby",                "red",  4, 1, 3500, 15,  9, GEMSTONE, CLR_RED,
                                                        RUBY),
GEM("jacinth",          "orange",  3, 1, 3250, 15,  9, GEMSTONE, CLR_ORANGE,
                                                        JACINTH),
GEM("sapphire",           "blue",  4, 1, 3000, 15,  9, GEMSTONE, CLR_BLUE,
                                                        SAPPHIRE),
GEM("black opal",        "black",  3, 1, 2500, 15,  8, GEMSTONE, CLR_BLACK,
                                                        BLACK_OPAL),
GEM("emerald",           "green",  5, 1, 2500, 15,  8, GEMSTONE, CLR_GREEN,
                                                        EMERALD),
GEM("turquoise",         "green",  6, 1, 2000, 15,  6, GEMSTONE, CLR_GREEN,
                                                        TURQUOISE),
GEM("citrine",          "yellow",  4, 1, 1500, 15,  6, GEMSTONE, CLR_YELLOW,
                                                        CITRINE),
GEM("aquamarine",        "green",  6, 1, 1500, 15,  8, GEMSTONE, CLR_GREEN,
                                                        AQUAMARINE),
GEM("amber",   "yellowish brown",  8, 1, 1000, 15,  2, GEMSTONE, CLR_BROWN,
                                                        AMBER),
GEM("topaz",   "yellowish brown", 10, 1,  900, 15,  8, GEMSTONE, CLR_BROWN,
                                                        TOPAZ),
GEM("jet",               "black",  6, 1,  850, 15,  7, GEMSTONE, CLR_BLACK,
                                                        JET),
GEM("opal",              "white", 12, 1,  800, 15,  6, GEMSTONE, CLR_WHITE,
                                                        OPAL),
GEM("chrysoberyl",      "yellow",  8, 1,  700, 15,  5, GEMSTONE, CLR_YELLOW,
                                                        CHRYSOBERYL),
GEM("garnet",              "red", 12, 1,  700, 15,  7, GEMSTONE, CLR_RED,
                                                        GARNET),
GEM("amethyst",         "violet", 14, 1,  600, 15,  7, GEMSTONE, CLR_MAGENTA,
                                                        AMETHYST),
GEM("jasper",              "red", 15, 1,  500, 15,  7, GEMSTONE, CLR_RED,
                                                        JASPER),
GEM("fluorite",         "violet", 15, 1,  400, 15,  4, GEMSTONE, CLR_MAGENTA,
                                                        FLUORITE),
GEM("obsidian",          "black",  9, 1,  200, 15,  6, GEMSTONE, CLR_BLACK,
                                                        OBSIDIAN),
GEM("agate",            "orange", 12, 1,  200, 15,  6, GEMSTONE, CLR_ORANGE,
                                                        AGATE),
GEM("jade",              "green", 10, 1,  300, 15,  6, GEMSTONE, CLR_GREEN,
                                                        JADE),
GEM("worthless piece of white glass", "white",
    77, 1, 0, 6, 5, GLASS, CLR_WHITE, WORTHLESS_WHITE_GLASS),
GEM("worthless piece of blue glass", "blue",
    77, 1, 0, 6, 5, GLASS, CLR_BLUE, WORTHLESS_BLUE_GLASS),
GEM("worthless piece of red glass", "red",
    77, 1, 0, 6, 5, GLASS, CLR_RED, WORTHLESS_RED_GLASS),
GEM("worthless piece of yellowish brown glass", "yellowish brown",
    77, 1, 0, 6, 5, GLASS, CLR_BROWN, WORTHLESS_YELLOWISH_BROWN_GLASS),
GEM("worthless piece of orange glass", "orange",
    76, 1, 0, 6, 5, GLASS, CLR_ORANGE, WORTHLESS_ORANGE_GLASS),
GEM("worthless piece of yellow glass", "yellow",
    77, 1, 0, 6, 5, GLASS, CLR_YELLOW, WORTHLESS_YELLOW_GLASS),
GEM("worthless piece of black glass", "black",
    76, 1, 0, 6, 5, GLASS, CLR_BLACK, WORTHLESS_BLACK_GLASS),
GEM("worthless piece of green glass", "green",
    77, 1, 0, 6, 5, GLASS, CLR_GREEN, WORTHLESS_GREEN_GLASS),
GEM("worthless piece of violet glass", "violet",
    77, 1, 0, 6, 5, GLASS, CLR_MAGENTA, WORTHLESS_VIOLET_GLASS),

/* Placement note: there is a wishable subrange for
 * "gray stones" in the o_ranges[] array in objnam.c
 * that is currently everything between luckstones and flint
 * (inclusive).
 */
ROCK("luckstone", "gray",  0,  10,  10, 60, 3, 3, 1, 10, 7, MINERAL, CLR_GRAY,
                                                                    LUCKSTONE),
ROCK("loadstone", "gray",  0,  10, 500,  1, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY,
                                                                    LOADSTONE),
ROCK("touchstone", "gray", 0,   8,  10, 45, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY,
                                                                    TOUCHSTONE),
ROCK("flint", "gray",      0,  10,  10,  1, 6, 6, 0, 10, 7, MINERAL, CLR_GRAY,
                                                                    FLINT),
ROCK("rock", NoDes,         1, 100,  10,  0, 3, 3, 0, 10, 7, MINERAL, CLR_GRAY,
                                                                    ROCK),
#undef GEM
#undef ROCK

/* miscellaneous ... */
/* Note: boulders and rocks are not normally created at random; the
 * probabilities only come into effect when you try to polymorph them.
 * Boulders weigh more than MAX_CARR_CAP; statues use corpsenm to take
 * on a specific type and may act as containers (both affect weight).
 */
OBJECT(OBJ("boulder", NoDes),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 100, 0, 6000, 0, 20, 20, 0, 0, 2000, HI_MINERAL, BOULDER),
OBJECT(OBJ("statue", NoDes),
       BITS(1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 900, 0, 2500, 0, 20, 20, 0, 0, 2500, CLR_WHITE, STATUE),

OBJECT(OBJ("heavy iron ball", NoDes),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       BALL_CLASS, 1000, 0, 480, 10, 25, 25, 0, 0, 200, HI_METAL,
                                                            HEAVY_IRON_BALL),
        /* +d4 when "very heavy" */
OBJECT(OBJ("iron chain", NoDes),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       CHAIN_CLASS, 1000, 0, 120, 0, 4, 4, 0, 0, 200, HI_METAL, IRON_CHAIN),
        /* +1 both l & s */

/* Venom is normally a transitory missile (spit by various creatures)
 * but can be wished for in wizard mode so could occur in bones data.
 */
OBJECT(OBJ("splash of blinding venom", "splash of venom"),
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 0, 0, 0, 0, 0, HI_ORGANIC, BLINDING_VENOM),
OBJECT(OBJ("splash of acid venom", "splash of venom"),
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 6, 6, 0, 0, 0, HI_ORGANIC, ACID_VENOM),
        /* +d6 small or large */

#if defined(OBJECTS_DESCR_INIT) || defined(OBJECTS_INIT)
/* fencepost, the deadly Array Terminator -- name [1st arg] *must* be NULL */
OBJECT(OBJ(NoDes, NoDes),
       BITS(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0), 0,
       ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
#undef BITS
#undef COLOR_FIELD
#endif

#undef OBJ
#undef OBJECT
#undef HARDGEM
#undef NoDes

/*objects.c*/

/* NetHack 3.7	permonst.h	$NHDT-Date: 1725653014 2024/09/06 20:03:34 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.26 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PERMONST_H
#define PERMONST_H

enum monnums {
#define MONS_ENUM
#include "monsters.h"
#undef MONS_ENUM
        NUMMONS,
        NON_PM = -1,              /* "not a monster" */
        LOW_PM = NON_PM + 1,      /* first monster in mons */
        LEAVESTATUE = NON_PM - 1, /* leave statue instead of corpse;
                                   * there are two lower values assigned
                                   * in end.c so that (x == LEAVESTATUE)
                                   * will test FALSE in bones.c:
                                   *  (NON_PM - 2) for no corpse
                                   *  (NON_PM - 3) for no corpse, no grave */
        HIGH_PM = NUMMONS - 1,
        SPECIAL_PM = PM_LONG_WORM_TAIL  /* [normal] < ~ < [special] */
                /* mons[SPECIAL_PM] through mons[NUMMONS-1], inclusive, are
                   never generated randomly and cannot be polymorphed into */
};

/*     This structure covers all attack forms.
 *     aatyp is the gross attack type (eg. claw, bite, breath, ...)
 *     adtyp is the damage type (eg. physical, fire, cold, spell, ...)
 *     damn is the number of hit dice of damage from the attack.
 *     damd is the number of sides on each die.
 *
 *     Some attacks can do no points of damage.  Additionally, some can
 *     have special effects *and* do damage as well.  If damn and damd
 *     are set, they may have a special meaning.  For example, if set
 *     for a blinding attack, they determine the amount of time blinded.
 */

struct attack {
    uchar aatyp;
    uchar adtyp, damn, damd;
};

/*     Max # of attacks for any given monster.
 */

#define NATTK 6

/*     Weight of human body, elf, dragon
 */
#define WT_HUMAN 1450U
#define WT_ELF 800U
#define WT_DRAGON 4500U

#ifndef ALIGN_H
#include "align.h"
#endif
#include "monattk.h"
#include "monflag.h"

struct permonst {
    const char *pmnames[NUM_MGENDERS];
    const enum monnums pmidx;   /* mons array index aka PM_ identifier */
    char mlet;                  /* symbol */
    schar mlevel,               /* base monster level */
        mmove,                  /* move speed */
        ac,                     /* (base) armor class */
        mr;                     /* (base) magic resistance */
    aligntyp maligntyp;         /* basic monster alignment */
    unsigned short geno;        /* creation/geno mask value */
    struct attack mattk[NATTK]; /* attacks matrix */
    unsigned short cwt,         /* weight of corpse */
        cnutrit;                /* its nutritional value */
    uchar msound;               /* noise it makes (6 bits) */
    uchar msize;                /* physical size (3 bits) */
    uchar mresists;             /* resistances */
    uchar mconveys;             /* conveyed by eating */
    unsigned long mflags1,      /* boolean bitflags */
        mflags2;                /* more boolean bitflags */
    unsigned short mflags3;     /* yet more boolean bitflags */
    uchar difficulty;           /* toughness (formerly from  makedefs -m) */
    uchar mcolor;               /* color to use */
};

extern NEARDATA struct permonst mons[NUMMONS + 1]; /* the master list of
                                                    * monster types */

#define VERY_SLOW 3
#define SLOW_SPEED 9
#define NORMAL_SPEED 12 /* movement rates */
#define FAST_SPEED 15
#define VERY_FAST 24

#endif /* PERMONST_H */

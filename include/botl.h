/* NetHack 3.6  botl.h  $NHDT-Date: 1433105378 2015/05/31 20:49:38 $  $NHDT-Branch: status_hilite $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) Michael Allison, 2003                            */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef BOTL_H
#define BOTL_H

/* MAXCO must hold longest uncompressed status line, and must be larger
 * than COLNO
 *
 * longest practical second status line at the moment is
 *	Astral Plane $:12345 HP:700(700) Pw:111(111) AC:-127 Xp:30/123456789
 *	T:123456 Satiated Conf FoodPois Ill Blind Stun Hallu Overloaded
 * -- or somewhat over 130 characters
 */
#if COLNO <= 140
#define MAXCO 160
#else
#define MAXCO (COLNO + 20)
#endif

#ifdef STATUS_VIA_WINDOWPORT
#if 0
/* clang-format off */
#define BL_FLUSH        -1
#define BL_TITLE        0
#define BL_STR          1
#define BL_DX           2
#define BL_CO           3
#define BL_IN           4
#define BL_WI           5
#define BL_CH           6
#define BL_ALIGN        7
#define BL_SCORE        8
#define BL_CAP          9
#define BL_GOLD         10
#define BL_ENE          11
#define BL_ENEMAX       12
#define BL_XP           13
#define BL_AC           14
#define BL_HD           15
#define BL_TIME         16
#define BL_HUNGER       17
#define BL_HP           18
#define BL_HPMAX        19
#define BL_LEVELDESC    20
#define BL_EXP          21
#define BL_CONDITION    22
/* clang-format on */

#else
enum statusfields { BL_FLUSH = -1, BL_TITLE = 0, BL_STR, BL_DX, BL_CO, BL_IN,
BL_WI, BL_CH, BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX, BL_LEVELDESC,
BL_EXP, BL_CONDITION };
#define MAXBLSTATS      BL_CONDITION+1

#define BEFORE  0
#define NOW     1
#endif

/* Boolean condition bits for the condition mask */

/* clang-format off */
#define BL_MASK_BLIND           0x00000001L
#define BL_MASK_CONF            0x00000002L
#define BL_MASK_FOODPOIS        0x00000004L
#define BL_MASK_ILL             0x00000008L
#define BL_MASK_HALLU           0x00000010L
#define BL_MASK_STUNNED         0x00000020L
#define BL_MASK_SLIMED          0x00000040L
/* clang-format on */

#define REASSESS_ONLY TRUE

#ifdef STATUS_HILITES
/* hilite status field behavior - coloridx values */
#define BL_HILITE_NONE -1    /* no hilite of this field */
#define BL_HILITE_INVERSE -2 /* inverse hilite */
#define BL_HILITE_BOLD -3    /* bold hilite */
                             /* or any CLR_ index (0 - 15) */
#define BL_TH_NONE 0
#define BL_TH_VAL_PERCENTAGE 100 /* threshold is percentage */
#define BL_TH_VAL_ABSOLUTE 101   /* threshold is particular value */
#define BL_TH_UPDOWN 102         /* threshold is up or down change */
#define BL_TH_CONDITION 103      /* threshold is bitmask of conditions */
#endif

extern const char *status_fieldnames[]; /* in botl.c */
#endif

#endif /* BOTL_H */

/* NetHack 3.7  botl.h  $NHDT-Date: 1596498528 2020/08/03 23:48:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.34 $ */
/* Copyright (c) Michael Allison, 2003                            */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef BOTL_H
#define BOTL_H

/* MAXCO must hold longest uncompressed status line, and must be larger
 * than COLNO
 *
 * longest practical second status line at the moment is
Astral Plane \GXXXXNNNN:123456 HP:1234(1234) Pw:1234(1234) AC:-127
 Xp:30/123456789 T:123456  Stone Slime Strngl FoodPois TermIll
 Satiated Overloaded Blind Deaf Stun Conf Hallu Lev Ride
 * -- or about 185 characters.  '$' gets encoded even when it
 * could be used as-is.  The first five status conditions are fatal
 * so it's rare to have more than one at a time.
 *
 * When the full line is wider than the map, the basic status line
 * formatting will move less important fields to the end, so if/when
 * truncation is necessary, it will chop off the least significant
 * information.
 */
#if COLNO <= 160
#define MAXCO 200
#else
#define MAXCO (COLNO + 40)
#endif

struct condmap {
    const char *id;
    unsigned long bitmask;
};

enum statusfields {
    BL_CHARACTERISTICS = -3, /* alias for BL_STR..BL_CH */
    BL_RESET = -2,           /* Force everything to redisplay */
    BL_FLUSH = -1,           /* Finished cycling through bot fields */
    BL_TITLE = 0,
    BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,  /* 1..6 */
    BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX, /* 7..12 */
    BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, /* 13..18 */
    BL_HPMAX, BL_LEVELDESC, BL_EXP, BL_CONDITION, /* 19..22 */
    MAXBLSTATS /* [23] */
};

enum relationships { NO_LTEQGT = -1,
                     EQ_VALUE, LT_VALUE, LE_VALUE,
                     GE_VALUE, GT_VALUE, TXT_VALUE };

enum blconditions {
    bl_bareh,
    bl_blind,
    bl_busy,
    bl_conf,
    bl_deaf,
    bl_elf_iron,
    bl_fly,
    bl_foodpois,
    bl_glowhands,
    bl_grab,
    bl_hallu,
    bl_held,
    bl_icy,
    bl_inlava,
    bl_lev,
    bl_parlyz,
    bl_ride,
    bl_sleeping,
    bl_slime,
    bl_slippery,
    bl_stone,
    bl_strngl,
    bl_stun,
    bl_submerged,
    bl_termill,
    bl_tethered,
    bl_trapped,
    bl_unconsc,
    bl_woundedl,
    bl_holding,

    CONDITION_COUNT
};

/* Boolean condition bits for the condition mask */

/* clang-format off */
#define BL_MASK_BAREH        0x00000001L
#define BL_MASK_BLIND        0x00000002L
#define BL_MASK_BUSY         0x00000004L
#define BL_MASK_CONF         0x00000008L
#define BL_MASK_DEAF         0x00000010L
#define BL_MASK_ELF_IRON     0x00000020L
#define BL_MASK_FLY          0x00000040L
#define BL_MASK_FOODPOIS     0x00000080L
#define BL_MASK_GLOWHANDS    0x00000100L
#define BL_MASK_GRAB         0x00000200L
#define BL_MASK_HALLU        0x00000400L
#define BL_MASK_HELD         0x00000800L
#define BL_MASK_ICY          0x00001000L
#define BL_MASK_INLAVA       0x00002000L
#define BL_MASK_LEV          0x00004000L
#define BL_MASK_PARLYZ       0x00008000L
#define BL_MASK_RIDE         0x00010000L
#define BL_MASK_SLEEPING     0x00020000L
#define BL_MASK_SLIME        0x00040000L
#define BL_MASK_SLIPPERY     0x00080000L
#define BL_MASK_STONE        0x00100000L
#define BL_MASK_STRNGL       0x00200000L
#define BL_MASK_STUN         0x00400000L
#define BL_MASK_SUBMERGED    0x00800000L
#define BL_MASK_TERMILL      0x01000000L
#define BL_MASK_TETHERED     0x02000000L
#define BL_MASK_TRAPPED      0x04000000L
#define BL_MASK_UNCONSC      0x08000000L
#define BL_MASK_WOUNDEDL     0x10000000L
#define BL_MASK_HOLDING      0x20000000L
#define BL_MASK_BITS            30 /* number of mask bits that can be set */
/* clang-format on */

struct conditions_t {
    int ranking;
    long mask;
    enum blconditions c;
    const char *text[3];
};
extern const struct conditions_t conditions[CONDITION_COUNT];

struct condtests_t {
    enum blconditions c;
    const char *useroption;
    enum optchoice opt;
    boolean enabled;
    boolean choice;
    boolean test;
};

extern struct condtests_t condtests[CONDITION_COUNT];
extern int cond_idx[CONDITION_COUNT];

#define BEFORE  0
#define NOW     1

/*
 * Possible additional conditions:
 *  major:
 *      grab   - grabbed by eel so about to be drowned ("wrapd"? damage type
 *               is AD_WRAP but message is "<mon> swings itself around you")
 *      digst  - swallowed and being digested
 *      lava   - trapped sinking into lava
 *  in_between: (potentially severe but don't necessarily lead to death;
 *               explains to player why he isn't getting to take any turns)
 *      unconc - unconscious
 *      parlyz - (multi < 0 && (!strncmp(multi_reason, "paralyzed", 9)
 *                              || !strncmp(multi_reason, "frozen", 6)))
 *      asleep - (multi < 0 && !strncmp(multi_reason, "sleeping", 8))
 *      busy   - other multi < 0
 *  minor:
 *      held   - grabbed by non-eel or by eel but not susceptible to drowning
 *      englf  - engulfed or swallowed but not being digested (usually
 *               obvious but the blank symbol set makes that uncertain)
 *      vomit  - vomiting (causes confusion and stun late in countdown)
 *      trap   - trapped in pit, bear trap, web, or floor (solidified lava)
 *      teth   - tethered to buried iron ball
 *      chain  - punished
 *      slip   - slippery fingers
 *      ice    - standing on ice (movement becomes uncertain)
 *     [underwater - movement uncertain, vision truncated, equipment at risk]
 *  other:
 *     [hold      - poly'd into grabber and holding adjacent monster]
 *      Stormbringer - wielded weapon poses risks
 *      Cleaver   - wielded weapon risks unintended consequences
 *      barehand  - not wielding any weapon nor wearing gloves
 *      no-weapon - not wielding any weapon
 *      bow/xbow/sling - wielding a missile launcher of specified type
 *      pole      - wielding a polearm
 *      pick      - wielding a pickaxe
 *      junk      - wielding non-weapon, non-weptool
 *      naked     - no armor
 *      no-gloves - self-explanatory
 *      no-cloak  - ditto
 *     [no-{other armor slots?} - probably much too verbose]
 *  conduct?
 *      [maybe if third status line is added]
 *
 *  Can't add all of these and probably don't want to.  But maybe we
 *  can add some of them and it's not as many as first appears:
 *  lava/trap/teth are mutually exclusive;
 *  digst/grab/englf/held/hold are also mutually exclusive;
 *  Stormbringer/Cleaver/barehand/no-weapon/bow&c/pole/pick/junk too;
 *  naked/no-{any armor slot} likewise.
 */

#define VIA_WINDOWPORT() \
    ((windowprocs.wincap2 & (WC2_HILITE_STATUS | WC2_FLUSH_STATUS)) != 0)

#define REASSESS_ONLY TRUE

/* #ifdef STATUS_HILITES */
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
#define BL_TH_TEXTMATCH 104      /* threshold text value to match against */
#define BL_TH_ALWAYS_HILITE 105  /* highlight regardless of value */
#define BL_TH_CRITICALHP 106     /* highlight critically low HP */

#define HL_ATTCLR_DIM     CLR_MAX + 0
#define HL_ATTCLR_BLINK   CLR_MAX + 1
#define HL_ATTCLR_ULINE   CLR_MAX + 2
#define HL_ATTCLR_INVERSE CLR_MAX + 3
#define HL_ATTCLR_BOLD    CLR_MAX + 4
#define BL_ATTCLR_MAX     CLR_MAX + 5

enum hlattribs { HL_UNDEF   = 0x00,
                 HL_NONE    = 0x01,
                 HL_BOLD    = 0x02,
                 HL_INVERSE = 0x04,
                 HL_ULINE   = 0x08,
                 HL_BLINK   = 0x10,
                 HL_DIM     = 0x20 };

#define MAXVALWIDTH 80 /* actually less, but was using 80 to allocate title
                       * and leveldesc then using QBUFSZ everywhere else   */
#ifdef STATUS_HILITES
struct hilite_s {
    enum statusfields fld;
    boolean set;
    unsigned anytype;
    anything value;
    int behavior;
    char textmatch[MAXVALWIDTH];
    enum relationships rel;
    int coloridx;
    struct hilite_s *next;
};
#endif

struct istat_s {
    const char *fldname;
    const char *fldfmt;
    long time;  /* moves when this field hilite times out */
    boolean chg; /* need to recalc time? */
    boolean percent_matters;
    short percent_value;
    unsigned anytype;
    anything a;
    char *val;
    int valwidth;
    enum statusfields idxmax;
    enum statusfields fld;
#ifdef STATUS_HILITES
    struct hilite_s *hilite_rule; /* the entry, if any, in 'thresholds'
                                   * list that currently applies        */
    struct hilite_s *thresholds;
#endif
};

extern const char *status_fieldnames[]; /* in botl.c */

#endif /* BOTL_H */

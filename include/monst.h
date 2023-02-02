/* NetHack 3.7	monst.h	$NHDT-Date: 1596498550 2020/08/03 23:49:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.42 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONST_H
#define MONST_H

/* start with incomplete types in case these aren't defined yet;
   basic pointers to them don't need to know their details */
struct monst;
struct obj;

#ifndef MEXTRA_H
#include "mextra.h"
#endif

/* The weapon_check flag is used two ways:
 * 1) When calling mon_wield_item, is 2-6 depending on what is desired.
 * 2) Between calls to mon_wield_item, is 0 or 1 depending on whether or not
 *    the weapon is known by the monster to be cursed (so it shouldn't bother
 *    trying for another weapon).
 * I originally planned to also use 0 if the monster already had its best
 * weapon, to avoid the overhead of a call to mon_wield_item, but it turns out
 * that there are enough situations which might make a monster change its
 * weapon that this is impractical.  --KAA
 */
enum wpn_chk_flags {
    NO_WEAPON_WANTED    = 0,
    NEED_WEAPON         = 1,
    NEED_RANGED_WEAPON  = 2,
    NEED_HTH_WEAPON     = 3,
    NEED_PICK_AXE       = 4,
    NEED_AXE            = 5,
    NEED_PICK_OR_AXE    = 6
};

/* The following flags are used for the second argument to display_minventory
 * in invent.c:
 *
 * PICK_NONE, PICK_ONE, PICK_ANY (wintype.h)  0, 1, 2
 * MINV_NOLET  If set, don't display inventory letters on monster's inventory.
 * MINV_ALL    If set, display all items in monster's inventory, otherwise
 *             just display wielded weapons and worn items.
 */
#define MINV_PICKMASK 0x03 /* 1|2 */
#define MINV_NOLET    0x04
#define MINV_ALL      0x08

/* monster appearance types */
enum m_ap_types {
    M_AP_NOTHING   = 0, /* mappearance unused--monster appears as itself */
    M_AP_FURNITURE = 1, /* stairs, a door, an altar, etc. */
    M_AP_OBJECT    = 2, /* an object */
    M_AP_MONSTER   = 3  /* a monster; mostly used for cloned Wizard */
};

#define MON_FLOOR        0x00
#define MON_OFFMAP       0x01
#define MON_DETACH       0x02
#define MON_MIGRATING    0x04
#define MON_LIMBO        0x08
#define MON_BUBBLEMOVE   0x10
#define MON_ENDGAME_FREE 0x20
#define MON_ENDGAME_MIGR 0x40
#define MON_OBLITERATE   0x80
#define MSTATE_MASK      0xFF

#define M_AP_TYPMASK  0x7
#define M_AP_F_DKNOWN 0x8
#define U_AP_TYPE (gy.youmonst.m_ap_type & M_AP_TYPMASK)
#define U_AP_FLAG (gy.youmonst.m_ap_type & ~M_AP_TYPMASK)
#define M_AP_TYPE(m) ((m)->m_ap_type & M_AP_TYPMASK)
#define M_AP_FLAG(m) ((m)->m_ap_type & ~M_AP_TYPMASK)

enum m_seen_resistance {
    M_SEEN_NOTHING = 0x0000,
    M_SEEN_MAGR    = 0x0001, /* Antimagic, AD_MAGM */
    M_SEEN_FIRE    = 0x0002, /* Fire_resistance, AD_FIRE */
    M_SEEN_COLD    = 0x0004, /* Cold_resistance, AD_COLD */
    M_SEEN_SLEEP   = 0x0008, /* Sleep_resistance, AD_SLEE */
    M_SEEN_DISINT  = 0x0010, /* Disint_resistance, AD_DISN */
    M_SEEN_ELEC    = 0x0020, /* Shock_resistance, AD_ELEC */
    M_SEEN_POISON  = 0x0040, /* AD_DRST */
    M_SEEN_ACID    = 0x0080, /* Acid_resistance, AD_ACID */
    M_SEEN_REFL    = 0x0100, /* reflection, no corresponding AD_foo */
};

#define m_seenres(mon, mask) ((mon)->seen_resistance & (mask))
#define m_setseenres(mon, mask) ((mon)->seen_resistance |= (mask))
#define monstseesu_ad(adtyp) monstseesu(cvt_adtyp_to_mseenres(adtyp))

struct monst {
    struct monst *nmon;
    struct permonst *data;
    unsigned m_id;
    short mnum;           /* permanent monster index number */
    short cham;           /* if shapeshifter, orig mons[] idx goes here */
    short movement;       /* movement points (derived from permonst definition
                             and added effects */
    uchar m_lev;          /* adjusted difficulty level of monster */
    aligntyp malign;      /* alignment of this monster, relative to the
                             player (positive = good to kill) */
    coordxy mx, my;
    coordxy mux, muy;       /* where the monster thinks you are */
#define MTSZ 4
    /* mtrack[0..2] is used to keep extra data when migrating the monster */
    coord mtrack[MTSZ];   /* monster track */
    int mhp, mhpmax;
    unsigned mappearance; /* for undetected mimics and the wiz */
    uchar m_ap_type;      /* what mappearance is describing, m_ap_types */

    schar mtame;                /* level of tameness, implies peaceful */
    unsigned short mintrinsics; /* low 8 correspond to mresists */
    unsigned short mextrinsics; /* low 8 correspond to mresists */
    unsigned long seen_resistance; /* M_SEEN_x; saw you resist an effect */
    int mspec_used;             /* monster's special ability attack timeout */

    Bitfield(female, 1);      /* is female */
    Bitfield(minvis, 1);      /* currently invisible */
    Bitfield(invis_blkd, 1);  /* invisibility blocked */
    Bitfield(perminvis, 1);   /* intrinsic minvis value */
    Bitfield(mcan, 1);        /* has been cancelled */
    Bitfield(mburied, 1);     /* has been buried */
    Bitfield(mundetected, 1); /* not seen in present hiding place;
                               * implies one of M1_CONCEAL or M1_HIDE,
                               * but not mimic (that is, snake, spider,
                               * trapper, piercer, eel) */
    Bitfield(mcansee, 1);   /* cansee 1, temp.blinded 0, blind 0 */

    Bitfield(mspeed, 2);    /* current speed */
    Bitfield(permspeed, 2); /* intrinsic mspeed value */
    Bitfield(mrevived, 1);  /* has been revived from the dead */
    Bitfield(mcloned, 1);   /* has been cloned from another */
    Bitfield(mavenge, 1);   /* did something to deserve retaliation */
    Bitfield(mflee, 1);     /* fleeing */

    Bitfield(mfleetim, 7);  /* timeout for mflee */
    Bitfield(msleeping, 1); /* asleep until woken */

    Bitfield(mblinded, 7);  /* cansee 0, temp.blinded n, blind 0 */
    Bitfield(mstun, 1);     /* stunned (off balance) */

    Bitfield(mfrozen, 7);
    Bitfield(mcanmove, 1);  /* paralysis, similar to mblinded */

    Bitfield(mconf, 1);     /* confused */
    Bitfield(mpeaceful, 1); /* does not attack unprovoked */
    Bitfield(mtrapped, 1);  /* trapped in a pit, web or bear trap */
    Bitfield(mleashed, 1);  /* monster is on a leash */
    Bitfield(isshk, 1);     /* is shopkeeper */
    Bitfield(isminion, 1);  /* is a minion */
    Bitfield(isgd, 1);      /* is guard */
    Bitfield(ispriest, 1);  /* is an aligned priest or high priest */

    Bitfield(iswiz, 1);     /* is the Wizard of Yendor */
    Bitfield(wormno, 5);    /* at most 31 worms on any level */
    Bitfield(mtemplit, 1);  /* temporarily seen; only valid during bhit() */
    /* 1 free bit */

#define MAX_NUM_WORMS 32    /* should be 2^(wormno bitfield size) */

    unsigned long mstrategy; /* for monsters with mflag3: current strategy */
#ifdef NHSTDC
#define STRAT_APPEARMSG 0x80000000UL
#else
#define STRAT_APPEARMSG 0x80000000L
#endif
#define STRAT_ARRIVE    0x40000000L /* just arrived on current level */
#define STRAT_WAITFORU  0x20000000L
#define STRAT_CLOSE     0x10000000L
#define STRAT_WAITMASK  (STRAT_CLOSE | STRAT_WAITFORU)
#define STRAT_HEAL      0x08000000L
#define STRAT_GROUND    0x04000000L
#define STRAT_MONSTR    0x02000000L
#define STRAT_PLAYER    0x01000000L
#define STRAT_NONE      0x00000000L
#define STRAT_STRATMASK 0x0f000000L
#define STRAT_XMASK     0x00ff0000L
#define STRAT_YMASK     0x0000ff00L
#define STRAT_GOAL      0x000000ffL
#define STRAT_GOALX(s) ((coordxy) ((s & STRAT_XMASK) >> 16))
#define STRAT_GOALY(s) ((coordxy) ((s & STRAT_YMASK) >> 8))

    long mtrapseen;        /* bitmap of traps we've been trapped in */
    long mlstmv;           /* for catching up with lost time */
    long mstate;           /* debugging info on monsters stored here */
    long migflags;         /* migrating flags */
    long mspare1;
    struct obj *minvent;   /* mon's inventory */
    struct obj *mw;        /* mon's weapon */
    long misc_worn_check;  /* mon's wornmask */
    xint16 weapon_check;   /* flag for whether to try switching weapons */

    int meating;           /* monster is eating timeout */
    struct mextra *mextra; /* point to mextra struct */
};

#define newmonst() (struct monst *) alloc(sizeof (struct monst))

/* these are in mspeed */
#define MSLOW 1 /* slowed monster */
#define MFAST 2 /* speeded monster */

#define MON_WEP(mon) ((mon)->mw)
#define MON_NOWEP(mon) ((mon)->mw = (struct obj *) 0)

#define DEADMONSTER(mon) ((mon)->mhp < 1)
#define is_starting_pet(mon) ((mon)->m_id == gc.context.startingpet_mid)
#define is_vampshifter(mon)                                      \
    ((mon)->cham == PM_VAMPIRE || (mon)->cham == PM_VAMPIRE_LEADER \
     || (mon)->cham == PM_VLAD_THE_IMPALER)
#define vampshifted(mon) (is_vampshifter((mon)) && !is_vampire((mon)->data))

/* monsters which cannot be displaced: priests, shopkeepers, vault guards,
   Oracle, quest leader */
#define mundisplaceable(mon) ((mon)->ispriest                    \
                              || (mon)->isshk                    \
                              || (mon)->isgd                     \
                              || (mon)->data == &mons[PM_ORACLE] \
                              || (mon)->m_id == gq.quest_status.leader_m_id)

/* mimic appearances that block vision/light */
#define is_lightblocker_mappear(mon)                       \
    (is_obj_mappear(mon, BOULDER)                          \
     || (M_AP_TYPE(mon) == M_AP_FURNITURE                    \
         && ((mon)->mappearance == S_hcdoor                \
             || (mon)->mappearance == S_vcdoor             \
             || (mon)->mappearance < S_ndoor /* = walls */ \
             || (mon)->mappearance == S_tree)))
#define is_door_mappear(mon) (M_AP_TYPE(mon) == M_AP_FURNITURE   \
                              && ((mon)->mappearance == S_hcdoor \
                                  || (mon)->mappearance == S_vcdoor))
#define is_obj_mappear(mon,otyp) (M_AP_TYPE(mon) == M_AP_OBJECT \
                                  && (mon)->mappearance == (otyp))

/* is mon m (presumably just killed) a troll and obj o Trollsbane? */
#define troll_baned(m,o) \
    ((m)->data->mlet == S_TROLL && (o) && (o)->oartifact == ART_TROLLSBANE)

#define engulfing_u(mon) (u.uswallow && (u.ustuck == (mon)))
#define helpless(mon) ((mon)->msleeping || !(mon)->mcanmove)

/* Get the maximum difficulty monsters that can currently be generated,
   given the current level difficulty and the hero's level. */
#define monmax_difficulty(levdif) (((levdif) + u.ulevel) / 2)
#define monmin_difficulty(levdif) ((levdif) / 6)
#define monmax_difficulty_lev() (monmax_difficulty(level_difficulty()))

/* Macros for whether a type of monster is too strong for a specific level. */
#define montoostrong(monindx, lev) (mons[monindx].difficulty > lev)
#define montooweak(monindx, lev) (mons[monindx].difficulty < lev)

#ifdef PMNAME_MACROS
#define Mgender(mon) ((mon)->female ? FEMALE : MALE)
#endif

#endif /* MONST_H */

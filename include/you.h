/* NetHack 3.6	you.h	$NHDT-Date: 1581322658 2020/02/10 08:17:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.42 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef YOU_H
#define YOU_H

#include "attrib.h"
#include "monst.h"
#ifndef PROP_H
#include "prop.h" /* (needed here for util/makedefs.c) */
#endif
#include "skills.h"

/*** Substructures ***/

struct RoleName {
    const char *m; /* name when character is male */
    const char *f; /* when female; null if same as male */
};

struct RoleAdvance {
    /* "fix" is the fixed amount, "rnd" is the random amount */
    xchar infix, inrnd; /* at character initialization */
    xchar lofix, lornd; /* gained per level <  g.urole.xlev */
    xchar hifix, hirnd; /* gained per level >= g.urole.xlev */
};

struct u_have {
    Bitfield(amulet, 1);   /* carrying Amulet   */
    Bitfield(bell, 1);     /* carrying Bell     */
    Bitfield(book, 1);     /* carrying Book     */
    Bitfield(menorah, 1);  /* carrying Candelabrum */
    Bitfield(questart, 1); /* carrying the Quest Artifact */
    Bitfield(unused, 3);
};

struct u_event {
    Bitfield(minor_oracle, 1); /* received at least 1 cheap oracle */
    Bitfield(major_oracle, 1); /*  "  expensive oracle */
    Bitfield(read_tribute, 1); /* read a passage from a novel */
    Bitfield(qcalled, 1);      /* called by Quest leader to do task */
    Bitfield(qexpelled, 1);    /* expelled from the Quest dungeon */
    Bitfield(qcompleted, 1);   /* successfully completed Quest task */
    Bitfield(uheard_tune, 2);  /* 1=know about, 2=heard passtune */

    Bitfield(uopened_dbridge, 1);   /* opened the drawbridge */
    Bitfield(invoked, 1);           /* invoked Gate to the Sanctum level */
    Bitfield(gehennom_entered, 1);  /* entered Gehennom via Valley */
    Bitfield(uhand_of_elbereth, 2); /* became Hand of Elbereth */
    Bitfield(udemigod, 1);          /* killed the wiz */
    Bitfield(uvibrated, 1);         /* stepped on "vibrating square" */
    Bitfield(ascended, 1);          /* has offered the Amulet */
};

/*
 * Achievements:  milestones reached during the current game.
 * Numerical order of these matters because they've been encoded in
 * a bitmask in xlogfile.  Reordering would break decoding that.
 * Aside from that, the number isn't significant--they're recorded
 * and eventually disclosed in the order achieved.
 *
 * Since xlogfile could be post-processed by unknown tools, we should
 * limit these to 31 total (it's possible that 32-bit signed longs are
 * the best such tools can offer).  Eventually that is likely to need
 * to change, probably by giving xlogfile an achieve2 field rather
 * than by assuming that 64-bit longs are viable or by squeezing in a
 * 32nd entry by switching to unsigned long.
 */
enum achivements {
    ACH_BELL =  1, /* acquired Bell of Opening */
    ACH_HELL =  2, /* entered Gehennom */
    ACH_CNDL =  3, /* acquired Candelabrum of Invocation */
    ACH_BOOK =  4, /* acquired Book of the Dead */
    ACH_INVK =  5, /* performed invocation to gain access to Sanctum */
    ACH_AMUL =  6, /* acquired The Amulet */
    ACH_ENDG =  7, /* entered end game */
    ACH_ASTR =  8, /* entered Astral Plane */
    ACH_UWIN =  9, /* ascended */
    ACH_MINE_PRIZE = 10, /* acquired Mines' End luckstone */
    ACH_SOKO_PRIZE = 11, /* acquired Sokoban bag or amulet */
    ACH_MEDU = 12, /* killed Medusa */
    ACH_BLND = 13, /* hero was always blond, no, blind */
    ACH_NUDE = 14, /* hero never wore armor */
    /* 1 through 14 were present in 3.6.x; the rest are newer; first,
       some easier ones so less skilled players can have achievements */
    ACH_MINE = 15, /* entered Gnomish Mines */
    ACH_TOWN = 16, /* reached Mine Town */
    ACH_SHOP = 17, /* entered a shop */
    ACH_TMPL = 18, /* entered a temple */
    ACH_ORCL = 19, /* consulted the Oracle */
    ACH_NOVL = 20, /* read at least one passage from a Discworld novel */
    ACH_SOKO = 21, /* entered Sokoban */
    ACH_BGRM = 22, /* entered Bigroom (not guaranteed to be in every dgn) */
    /* 23..31, 9 available potential achievements; #32 currently off-limits */
    N_ACH = 32     /* allocate room for 31 plus a slot for 0 terminator */
};
    /*
     * Other potential achievements to track (this comment briefly resided
     * in encodeachieve(topten.c) and has been revised since moving here:
     *  [reached experience level N for a few interesting values of N
     *   or perhaps "became a <rank title>" for each new rank reached]
     *  got quest summons,
     *  entered quest branch,
     *  chatted with leader,
     *  entered second or lower quest level (implies leader gave the Ok),
     *  entered last quest level,
     *  defeated nemesis (not same as acquiring Bell or artifact),
     *  completed quest (formally, by bringing artifact to leader),
     *  entered rogue level,
     *  entered Fort Ludios level/branch (not guaranteed to be achieveable),
     *  entered Medusa level,
     *  entered castle level,
     *  opened castle drawbridge,
     *  obtained castle wand (handle similarly to mines and sokoban prizes),
     *  passed Valley level (entered-Gehennom already covers Valley itself),
     *  [assorted demon lairs?],
     *  entered Vlad's tower branch,
     *  defeated Vlad (not same as acquiring Candelabrum),
     *  entered Wizard's tower area within relevant level,
     *  defeated Wizard,
     *  found vibrating square,
     *  entered sanctum level (maybe not; too close to performed-invocation),
     *  [defeated Famine, defeated Pestilence, defeated Death]
     */

struct u_realtime {
    long   realtime;     /* accumulated playing time in seconds */
    time_t start_timing; /* time game was started or restored or 'realtime'
                            was last updated (savegamestate for checkpoint) */
    time_t finish_time;  /* end of 'realtime' interval: time of save or
                            end of game; used for topten/logfile/xlogfile */
};

/* KMH, conduct --
 * These are voluntary challenges.  Each field denotes the number of
 * times a challenge has been violated.
 */
struct u_conduct {     /* number of times... */
    long unvegetarian; /* eaten any animal */
    long unvegan;      /* ... or any animal byproduct */
    long food;         /* ... or any comestible */
    long gnostic;      /* used prayer, priest, or altar */
    long weaphit;      /* hit a monster with a weapon */
    long killer;       /* killed a monster yourself */
    long literate;     /* read something (other than BotD) */
    long polypiles;    /* polymorphed an object */
    long polyselfs;    /* transformed yourself */
    long wishes;       /* used a wish */
    long wisharti;     /* wished for an artifact */
    /* genocides already listed at end of game */
};

struct u_roleplay {
    boolean blind;  /* permanently blind */
    boolean nudist; /* has not worn any armor, ever */
    long numbones;  /* # of bones files loaded  */
};

/*** Unified structure containing role information ***/
struct Role {
    /*** Strings that name various things ***/
    struct RoleName name;    /* the role's name (from u_init.c) */
    struct RoleName rank[9]; /* names for experience levels (from botl.c) */
    const char *lgod, *ngod, *cgod; /* god names (from pray.c) */
    const char *filecode;           /* abbreviation for use in file names */
    const char *homebase; /* quest leader's location (from questpgr.c) */
    const char *intermed; /* quest intermediate goal (from questpgr.c) */

    /*** Indices of important monsters and objects ***/
    short malenum, /* index (PM_) as a male (botl.c) */
        femalenum, /* ...or as a female (NON_PM == same) */
        petnum,    /* PM_ of preferred pet (NON_PM == random) */
        ldrnum,    /* PM_ of quest leader (questpgr.c) */
        guardnum,  /* PM_ of quest guardians (questpgr.c) */
        neminum,   /* PM_ of quest nemesis (questpgr.c) */
        enemy1num, /* specific quest enemies (NON_PM == random) */
        enemy2num;
    char enemy1sym, /* quest enemies by class (S_) */
        enemy2sym;
    short questarti; /* index (ART_) of quest artifact (questpgr.c) */

    /*** Bitmasks ***/
    short allow;                  /* bit mask of allowed variations */
#define ROLE_RACEMASK  0x0ff8     /* allowable races */
#define ROLE_GENDMASK  0xf000     /* allowable genders */
#define ROLE_MALE      0x1000
#define ROLE_FEMALE    0x2000
#define ROLE_NEUTER    0x4000
#define ROLE_ALIGNMASK AM_MASK    /* allowable alignments */
#define ROLE_LAWFUL    AM_LAWFUL
#define ROLE_NEUTRAL   AM_NEUTRAL
#define ROLE_CHAOTIC   AM_CHAOTIC

    /*** Attributes (from attrib.c and exper.c) ***/
    xchar attrbase[A_MAX];    /* lowest initial attributes */
    xchar attrdist[A_MAX];    /* distribution of initial attributes */
    struct RoleAdvance hpadv; /* hit point advancement */
    struct RoleAdvance enadv; /* energy advancement */
    xchar xlev;               /* cutoff experience level */
    xchar initrecord;         /* initial alignment record */

    /*** Spell statistics (from spell.c) ***/
    int spelbase; /* base spellcasting penalty */
    int spelheal; /* penalty (-bonus) for healing spells */
    int spelshld; /* penalty for wearing any shield */
    int spelarmr; /* penalty for wearing metal armour */
    int spelstat; /* which stat (A_) is used */
    int spelspec; /* spell (SPE_) the class excels at */
    int spelsbon; /* penalty (-bonus) for that spell */

    /*** Properties in variable-length arrays ***/
    /* intrinsics (see attrib.c) */
    /* initial inventory (see u_init.c) */
    /* skills (see u_init.c) */

    /*** Don't forget to add... ***/
    /* quest leader, guardians, nemesis (monst.c) */
    /* quest artifact (artilist.h) */
    /* quest dungeon definition (dat/Xyz.dat) */
    /* quest text (dat/quest.txt) */
    /* dictionary entries (dat/data.bas) */
};

extern const struct Role roles[]; /* table of available roles */
#define Role_if(X) (g.urole.malenum == (X))
#define Role_switch (g.urole.malenum)

/* used during initialization for race, gender, and alignment
   as well as for character class */
#define ROLE_NONE (-1)
#define ROLE_RANDOM (-2)

/*** Unified structure specifying race information ***/

struct Race {
    /*** Strings that name various things ***/
    const char *noun;           /* noun ("human", "elf") */
    const char *adj;            /* adjective ("human", "elven") */
    const char *coll;           /* collective ("humanity", "elvenkind") */
    const char *filecode;       /* code for filenames */
    struct RoleName individual; /* individual as a noun ("man", "elf") */

    /*** Indices of important monsters and objects ***/
    short malenum, /* PM_ as a male monster */
        femalenum, /* ...or as a female (NON_PM == same) */
        mummynum,  /* PM_ as a mummy */
        zombienum; /* PM_ as a zombie */

    /*** Bitmasks ***/
    short allow;    /* bit mask of allowed variations */
    short selfmask, /* your own race's bit mask */
        lovemask,   /* bit mask of always peaceful */
        hatemask;   /* bit mask of always hostile */

    /*** Attributes ***/
    xchar attrmin[A_MAX];     /* minimum allowable attribute */
    xchar attrmax[A_MAX];     /* maximum allowable attribute */
    struct RoleAdvance hpadv; /* hit point advancement */
    struct RoleAdvance enadv; /* energy advancement */
#if 0 /* DEFERRED */
    int   nv_range;           /* night vision range */
    int   xray_range;         /* X-ray vision range */
#endif

    /*** Properties in variable-length arrays ***/
    /* intrinsics (see attrib.c) */

    /*** Don't forget to add... ***/
    /* quest leader, guardians, nemesis (monst.c) */
    /* quest dungeon definition (dat/Xyz.dat) */
    /* quest text (dat/quest.txt) */
    /* dictionary entries (dat/data.bas) */
};

extern const struct Race races[]; /* Table of available races */
#define Race_if(X) (g.urace.malenum == (X))
#define Race_switch (g.urace.malenum)

/*** Unified structure specifying gender information ***/
struct Gender {
    const char *adj;      /* male/female/neuter */
    const char *he;       /* he/she/it */
    const char *him;      /* him/her/it */
    const char *his;      /* his/her/its */
    const char *filecode; /* file code */
    short allow;          /* equivalent ROLE_ mask */
};
#define ROLE_GENDERS 2    /* number of permitted player genders
                             increment to 3 if you allow neuter roles */

extern const struct Gender genders[]; /* table of available genders */
/* pronouns for the hero */
#define uhe()      (genders[flags.female ? 1 : 0].he)
#define uhim()     (genders[flags.female ? 1 : 0].him)
#define uhis()     (genders[flags.female ? 1 : 0].his)
/* pronoun_gender() flag masks */
#define PRONOUN_NORMAL 0 /* none of the below */
#define PRONOUN_NO_IT  1
#define PRONOUN_HALLU  2
/* corresponding pronouns for monsters; yields "it" when mtmp can't be seen */
#define mhe(mtmp)  (genders[pronoun_gender(mtmp, PRONOUN_HALLU)].he)
#define mhim(mtmp) (genders[pronoun_gender(mtmp, PRONOUN_HALLU)].him)
#define mhis(mtmp) (genders[pronoun_gender(mtmp, PRONOUN_HALLU)].his)
/* override "it" if reason is lack of visibility rather than neuter species */
#define noit_mhe(mtmp) \
    (genders[pronoun_gender(mtmp, (PRONOUN_NO_IT | PRONOUN_HALLU))].he)
#define noit_mhim(mtmp) \
    (genders[pronoun_gender(mtmp, (PRONOUN_NO_IT | PRONOUN_HALLU))].him)
#define noit_mhis(mtmp) \
    (genders[pronoun_gender(mtmp, (PRONOUN_NO_IT | PRONOUN_HALLU))].his)

/*** Unified structure specifying alignment information ***/
struct Align {
    const char *noun;     /* law/balance/chaos */
    const char *adj;      /* lawful/neutral/chaotic */
    const char *filecode; /* file code */
    short allow;          /* equivalent ROLE_ mask */
    aligntyp value;       /* equivalent A_ value */
};
#define ROLE_ALIGNS 3     /* number of permitted player alignments */

extern const struct Align aligns[]; /* table of available alignments */

enum utraptypes {
    TT_BEARTRAP   = 0,
    TT_PIT        = 1,
    TT_WEB        = 2,
    TT_LAVA       = 3,
    TT_INFLOOR    = 4,
    TT_BURIEDBALL = 5
};

/*** Information about the player ***/
struct you {
    xchar ux, uy;       /* current map coordinates */
    schar dx, dy, dz;   /* direction of move (or zap or ... ) */
    schar di;           /* direction of FF */
    xchar tx, ty;       /* destination of travel */
    xchar ux0, uy0;     /* initial position FF */
    d_level uz, uz0;    /* your level on this and the previous turn */
    d_level utolev;     /* level monster teleported you to, or uz */
    uchar utotype;      /* bitmask of goto_level() flags for utolev */
    boolean umoved;     /* changed map location (post-move) */
    int last_str_turn;  /* 0: none, 1: half turn, 2: full turn
                           +: turn right, -: turn left */
    int ulevel;         /* 1 to MAXULEV */
    int ulevelmax;
    unsigned utrap;     /* trap timeout */
    unsigned utraptype; /* defined if utrap nonzero. one of utraptypes */
    char urooms[5];         /* rooms (roomno + 3) occupied now */
    char urooms0[5];        /* ditto, for previous position */
    char uentered[5];       /* rooms (roomno + 3) entered this turn */
    char ushops[5];         /* shop rooms (roomno + 3) occupied now */
    char ushops0[5];        /* ditto, for previous position */
    char ushops_entered[5]; /* ditto, shops entered this turn */
    char ushops_left[5];    /* ditto, shops exited this turn */

    int uhunger;  /* refd only in eat.c and shk.c */
    unsigned uhs; /* hunger state - see eat.c */

    struct prop uprops[LAST_PROP + 1];

    unsigned umconf;
    Bitfield(usick_type, 2);
#define SICK_VOMITABLE 0x01
#define SICK_NONVOMITABLE 0x02
#define SICK_ALL 0x03

    /* These ranges can never be more than MAX_RANGE (vision.h). */
    int nv_range;   /* current night vision range */
    int xray_range; /* current xray vision range */

/*
 * These variables are valid globally only when punished and blind.
 */
#define BC_BALL 0x01  /* bit mask for ball  in 'bc_felt' below */
#define BC_CHAIN 0x02 /* bit mask for chain in 'bc_felt' below */
    int bglyph;       /* glyph under the ball */
    int cglyph;       /* glyph under the chain */
    int bc_order;     /* ball & chain order [see bc_order() in ball.c] */
    int bc_felt;      /* mask for ball/chain being felt */

    int umonster;               /* hero's "real" monster num */
    int umonnum;                /* current monster number */

    int mh, mhmax, mtimedone;   /* for polymorph-self */
    struct attribs macurr,      /* for monster attribs */
                   mamax;       /* for monster attribs */
    int ulycn;                  /* lycanthrope type */

    unsigned ucreamed;
    unsigned uswldtim;          /* time you have been swallowed */

    Bitfield(uswallow, 1);      /* true if swallowed */
    Bitfield(uinwater, 1);      /* if you're currently in water (only
                                   underwater possible currently) */
    Bitfield(uundetected, 1);   /* if you're a hiding monster/piercer */
    Bitfield(mfemale, 1);       /* saved human value of flags.female */
    Bitfield(uinvulnerable, 1); /* you're invulnerable (praying) */
    Bitfield(uburied, 1);       /* you're buried */
    Bitfield(uedibility, 1);    /* blessed food detect; sense unsafe food */
    /* 1 free bit! */

    unsigned udg_cnt;           /* how long you have been demigod */
    struct u_event uevent;      /* certain events have happened */
    struct u_have uhave;        /* you're carrying special objects */
    struct u_conduct uconduct;  /* KMH, conduct */
    struct u_roleplay uroleplay;
    struct attribs acurr,       /* your current attributes (eg. str)*/
                    aexe,       /* for gain/loss via "exercise" */
                    abon,       /* your bonus attributes (eg. str) */
                    amax,       /* your max attributes (eg. str) */
                   atemp,       /* used for temporary loss/gain */
                   atime;       /* used for loss/gain countdown */
    align ualign;               /* character alignment */
#define CONVERT    2
#define A_ORIGINAL 1
#define A_CURRENT  0
    aligntyp ualignbase[CONVERT]; /* for ualign conversion record */
    schar uluck, moreluck;        /* luck and luck bonus */
#define Luck (u.uluck + u.moreluck)
#define LUCKADD    3  /* value of u.moreluck when carrying luck stone;
                         + when blessed or uncursed, - when cursed */
#define LUCKMAX   10  /* maximum value of u.ulUck */
#define LUCKMIN (-10) /* minimum value of u.uluck */
    schar uhitinc;
    schar udaminc;
    schar uac;
    uchar uspellprot;        /* protection by SPE_PROTECTION */
    uchar usptime;           /* #moves until uspellprot-- */
    uchar uspmtime;          /* #moves between uspellprot-- */
    int uhp, uhpmax;         /* hit points, aka health */
    int uen, uenmax;         /* magical energy - M. Stephenson */
    xchar uhpinc[MAXULEV],   /* increases to uhpmax for each level gain */
          ueninc[MAXULEV];   /* increases to uenmax for each level gain */
    int ugangr;              /* if the gods are angry at you */
    int ugifts;              /* number of artifacts bestowed */
    int ublessed, ublesscnt; /* blessing/duration from #pray */
    long umoney0;
    long uspare1;
    long uexp, urexp;
    long ucleansed;          /* to record moves when player was cleansed */
    long usleep;             /* sleeping; monstermove you last started */
    int uinvault;
    struct monst *ustuck;    /* engulfer or grabber, maybe grabbee if Upolyd */
    struct monst *usteed;    /* mount when riding */
    long ugallop;            /* turns steed will run after being kicked */
    int urideturns;          /* time spent riding, for skill advancement */
    int umortality;          /* how many times you died */
    int ugrave_arise;    /* you die and become something aside from a ghost */
    int weapon_slots;        /* unused skill slots */
    int skills_advanced;     /* # of advances made so far */
    xchar skill_record[P_SKILL_LIMIT]; /* skill advancements */
    struct skills weapon_skills[P_NUM_SKILLS];
    boolean twoweap;         /* KMH -- Using two-weapon combat */
    short mcham;             /* vampire mndx if shapeshifted to bat/cloud */
    xchar uachieved[N_ACH];  /* list of achievements in the order attained */
}; /* end of `struct you' */

#define Upolyd (u.umonnum != u.umonster)

#endif /* YOU_H */

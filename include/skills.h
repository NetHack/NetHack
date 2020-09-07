/* NetHack 3.7	skills.h	$NHDT-Date: 1596498559 2020/08/03 23:49:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/*-Copyright (c) Pasi Kallinen, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SKILLS_H
#define SKILLS_H

/* Much of this code was taken from you.h.  It is now
 * in a separate file so it can be included in objects.c.
 */

enum p_skills {
    /* Code to denote that no skill is applicable */
    P_NONE = 0,

/* Weapon Skills -- Stephen White
 * Order matters and are used in macros.
 * Positive values denote hand-to-hand weapons or launchers.
 * Negative values denote ammunition or missiles.
 * Update weapon.c if you amend any skills.
 * Also used for oc_subtyp.
 */
    P_DAGGER             =  1,
    P_KNIFE              =  2,
    P_AXE                =  3,
    P_PICK_AXE           =  4,
    P_SHORT_SWORD        =  5,
    P_BROAD_SWORD        =  6,
    P_LONG_SWORD         =  7,
    P_TWO_HANDED_SWORD   =  8,
    P_SCIMITAR           =  9,
    P_SABER              = 10,
    P_CLUB               = 11, /* Heavy-shafted bludgeon */
    P_MACE               = 12,
    P_MORNING_STAR       = 13, /* Spiked bludgeon */
    P_FLAIL              = 14, /* Two pieces hinged or chained together */
    P_HAMMER             = 15, /* Heavy head on the end */
    P_QUARTERSTAFF       = 16, /* Long-shafted bludgeon */
    P_POLEARMS           = 17, /* attack two or three steps away */
    P_SPEAR              = 18, /* includes javelin */
    P_TRIDENT            = 19,
    P_LANCE              = 20,
    P_BOW                = 21, /* launchers */
    P_SLING              = 22,
    P_CROSSBOW           = 23,
    P_DART               = 24, /* hand-thrown missiles */
    P_SHURIKEN           = 25,
    P_BOOMERANG          = 26,
    P_WHIP               = 27, /* flexible, one-handed */
    P_UNICORN_HORN       = 28, /* last weapon, two-handed */

    /* Spell Skills added by Larry Stewart-Zerba */
    P_ATTACK_SPELL       = 29,
    P_HEALING_SPELL      = 30,
    P_DIVINATION_SPELL   = 31,
    P_ENCHANTMENT_SPELL  = 32,
    P_CLERIC_SPELL       = 33,
    P_ESCAPE_SPELL       = 34,
    P_MATTER_SPELL       = 35,

    /* Other types of combat */
    P_BARE_HANDED_COMBAT = 36, /* actually weaponless; gloves are ok */
    P_TWO_WEAPON_COMBAT  = 37, /* pair of weapons, one in each hand */
    P_RIDING             = 38, /* How well you control your steed */

    P_NUM_SKILLS         = 39
};

#define P_MARTIAL_ARTS P_BARE_HANDED_COMBAT /* Role distinguishes */

#define P_FIRST_WEAPON P_DAGGER
#define P_LAST_WEAPON P_UNICORN_HORN

#define P_FIRST_SPELL P_ATTACK_SPELL
#define P_LAST_SPELL P_MATTER_SPELL

#define P_LAST_H_TO_H P_RIDING
#define P_FIRST_H_TO_H P_BARE_HANDED_COMBAT

/* These roles qualify for a martial arts bonus */
#define martial_bonus() (Role_if(PM_SAMURAI) || Role_if(PM_MONK))

/*
 * These are the standard weapon skill levels.  It is important that
 * the lowest "valid" skill be be 1.  The code calculates the
 * previous amount to practice by calling  practice_needed_to_advance()
 * with the current skill-1.  To work out for the UNSKILLED case,
 * a value of 0 needed.
 */
enum skill_levels {
    P_ISRESTRICTED = 0, /* unskilled and can't be advanced */
    P_UNSKILLED    = 1, /* unskilled so far but can be advanced */
    /* Skill levels Basic/Advanced/Expert had long been used by
       Heroes of Might and Magic (tm) and its sequels... */
    P_BASIC        = 2,
    P_SKILLED      = 3,
    P_EXPERT       = 4,
    /* when the skill system was adopted into nethack, levels beyond expert
       were unnamed and just used numbers.  Devteam coined them Master and
       Grand Master.  Sometime after that, Heroes of Might and Magic IV (tm)
       was released and had two more levels which use these same names. */
    P_MASTER       = 5, /* Unarmed combat/martial arts only */
    P_GRAND_MASTER = 6  /* ditto */
};

#define practice_needed_to_advance(level) ((level) * (level) *20)

/* The hero's skill in various weapons. */
struct skills {
    xchar skill;
    xchar max_skill;
    unsigned short advance;
};

#define P_SKILL(type) (u.weapon_skills[type].skill)
#define P_MAX_SKILL(type) (u.weapon_skills[type].max_skill)
#define P_ADVANCE(type) (u.weapon_skills[type].advance)
#define P_RESTRICTED(type) (u.weapon_skills[type].skill == P_ISRESTRICTED)

#define P_SKILL_LIMIT 60 /* Max number of skill advancements */

/* Initial skill matrix structure; used in u_init.c and weapon.c */
struct def_skill {
    xchar skill;
    xchar skmax;
};

#endif /* SKILLS_H */

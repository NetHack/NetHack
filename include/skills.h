/* NetHack 3.6	skills.h	$NHDT-Date: 1432512778 2015/05/25 00:12:58 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
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
    P_DAGGER,
    P_KNIFE,
    P_AXE,
    P_PICK_AXE,
    P_SHORT_SWORD,
    P_BROAD_SWORD,
    P_LONG_SWORD,
    P_TWO_HANDED_SWORD,
    P_SCIMITAR,
    P_SABER,
    P_CLUB, /* Heavy-shafted bludgeon */
    P_MACE,
    P_MORNING_STAR, /* Spiked bludgeon */
    P_FLAIL,        /* Two pieces hinged or chained together */
    P_HAMMER,       /* Heavy head on the end */
    P_QUARTERSTAFF, /* Long-shafted bludgeon */
    P_POLEARMS,
    P_SPEAR, /* includes javelin */
    P_TRIDENT,
    P_LANCE,
    P_BOW,
    P_SLING,
    P_CROSSBOW,
    P_DART,
    P_SHURIKEN,
    P_BOOMERANG,
    P_WHIP,
    P_UNICORN_HORN, /* last weapon */

    /* Spell Skills added by Larry Stewart-Zerba */
    P_ATTACK_SPELL,
    P_HEALING_SPELL,
    P_DIVINATION_SPELL,
    P_ENCHANTMENT_SPELL,
    P_CLERIC_SPELL,
    P_ESCAPE_SPELL,
    P_MATTER_SPELL,

    /* Other types of combat */
    P_BARE_HANDED_COMBAT, /* actually weaponless; gloves are ok */
    P_TWO_WEAPON_COMBAT,
    P_RIDING,             /* How well you control your steed */

    P_NUM_SKILLS
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
    P_ISRESTRICTED = 0,
    P_UNSKILLED,
    P_BASIC,
    P_SKILLED,
    P_EXPERT,
    P_MASTER,       /* Unarmed combat/martial arts only */
    P_GRAND_MASTER  /* Unarmed combat/martial arts only */
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

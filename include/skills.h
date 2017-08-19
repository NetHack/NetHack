/* NetHack 3.6	skills.h	$NHDT-Date: 1432512778 2015/05/25 00:12:58 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SKILLS_H
#define SKILLS_H

/* Much of this code was taken from you.h.  It is now
 * in a separate file so it can be included in objects.c.
 */

/* Code to denote that no skill is applicable */
#define P_NONE 0

/* Weapon Skills -- Stephen White
 * Order matters and are used in macros.
 * Positive values denote hand-to-hand weapons or launchers.
 * Negative values denote ammunition or missiles.
 * Update weapon.c if you amend any skills.
 * Also used for oc_subtyp.
 */
#define P_DAGGER 1
#define P_KNIFE (P_DAGGER+1)
#define P_AXE (P_KNIFE+1)
#define P_PICK_AXE (P_AXE+1)
#define P_SHORT_SWORD (P_PICK_AXE+1)
#define P_BROAD_SWORD (P_SHORT_SWORD+1)
#define P_LONG_SWORD (P_BROAD_SWORD+1)
#define P_TWO_HANDED_SWORD (P_LONG_SWORD+1)
#define P_SCIMITAR (P_TWO_HANDED_SWORD+1)
#define P_SABER (P_SCIMITAR+1)
#define P_CLUB (P_SABER+1) /* Heavy-shafted bludgeon */
#define P_MACE (P_CLUB+1)
#define P_MORNING_STAR (P_MACE+1) /* Spiked bludgeon */
#define P_FLAIL (P_MORNING_STAR+1)        /* Two pieces hinged or chained together */
#define P_HAMMER (P_FLAIL+1)       /* Heavy head on the end */
#define P_QUARTERSTAFF (P_HAMMER+1) /* Long-shafted bludgeon */
#define P_POLEARMS (P_QUARTERSTAFF+1)
#define P_SPEAR (P_POLEARMS+1) /* includes javelin */
#define P_TRIDENT (P_SPEAR+1)
#define P_LANCE (P_TRIDENT+1)
#define P_BOW (P_LANCE+1)
#define P_SLING (P_BOW+1)
#define P_CROSSBOW (P_SLING+1)
#define P_DART (P_CROSSBOW+1)
#define P_SHURIKEN (P_DART+1)
#define P_BOOMERANG (P_SHURIKEN+1)
#define P_WHIP (P_BOOMERANG+1)
#define P_UNICORN_HORN (P_WHIP+1) /* last weapon */
#define P_FIRST_WEAPON P_DAGGER
#define P_LAST_WEAPON P_UNICORN_HORN

/* Spell Skills added by Larry Stewart-Zerba */
#define P_ATTACK_SPELL (P_LAST_WEAPON+1)
#define P_HEALING_SPELL (P_ATTACK_SPELL+1)
#define P_DIVINATION_SPELL (P_HEALING_SPELL+1)
#define P_ENCHANTMENT_SPELL (P_DIVINATION_SPELL+1)
#define P_CLERIC_SPELL (P_ENCHANTMENT_SPELL+1)
#define P_ESCAPE_SPELL (P_CLERIC_SPELL+1)
#define P_MATTER_SPELL (P_ESCAPE_SPELL+1)
#define P_FIRST_SPELL P_ATTACK_SPELL
#define P_LAST_SPELL P_MATTER_SPELL

/* Other types of combat */
#define P_BARE_HANDED_COMBAT (P_LAST_SPELL+1) /* actually weaponless; gloves are ok */
#define P_MARTIAL_ARTS P_BARE_HANDED_COMBAT /* Role distinguishes */
#define P_TWO_WEAPON_COMBAT (P_BARE_HANDED_COMBAT+1)              /* Finally implemented */
#define P_RIDING (P_TWO_WEAPON_COMBAT+1) /* How well you control your steed */
#define P_LAST_H_TO_H P_RIDING
#define P_FIRST_H_TO_H P_BARE_HANDED_COMBAT

#define P_NUM_SKILLS (P_LAST_H_TO_H + 1)

/* These roles qualify for a martial arts bonus */
#define martial_bonus() (Role_if(PM_SAMURAI) || Role_if(PM_MONK))

/*
 * These are the standard weapon skill levels.  It is important that
 * the lowest "valid" skill be be 1.  The code calculates the
 * previous amount to practice by calling  practice_needed_to_advance()
 * with the current skill-1.  To work out for the UNSKILLED case,
 * a value of 0 needed.
 */
#define P_ISRESTRICTED 0
#define P_UNSKILLED 1
#define P_BASIC 2
#define P_SKILLED 3
#define P_EXPERT 4
#define P_MASTER 5       /* Unarmed combat/martial arts only */
#define P_GRAND_MASTER 6 /* Unarmed combat/martial arts only */

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

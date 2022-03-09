/* NetHack 3.7	spell.h	$NHDT-Date: 1646838388 2022/03/09 15:06:28 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
/* Copyright 1986, M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SPELL_H
#define SPELL_H

#define NO_SPELL 0
#define UNKNOWN_SPELL (-1)

/* spellbook re-use control; used when reading and when polymorphing */
#define MAX_SPELL_STUDY 3

struct spell {
    short sp_id;  /* spell id (== object.otyp) */
    xchar sp_lev; /* power level */
    int sp_know;  /* knowlege of spell */
};

enum spellknowledge {
    spe_Forgotten  = -1, /* known but no longer castable */
    spe_Unknown    =  0, /* not yet known */
    spe_Fresh      =  1, /* castable if various casting criteria are met */
    spe_GoingStale =  2  /* still castable but nearly forgotten */
};

/* levels of memory destruction with a scroll of amnesia */
#define ALL_MAP 0x1
#define ALL_SPELLS 0x2

#define decrnknow(spell) g.spl_book[spell].sp_know--
#define spellid(spell) g.spl_book[spell].sp_id
#define spellknow(spell) g.spl_book[spell].sp_know

/* how much Pw a spell of level lvl costs to cast? */
#define SPELL_LEV_PW(lvl) ((lvl) * 5)

#endif /* SPELL_H */

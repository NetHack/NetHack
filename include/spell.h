/* NetHack 3.6	spell.h	$NHDT-Date: 1432512780 2015/05/25 00:13:00 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright 1986, M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SPELL_H
#define SPELL_H

#define NO_SPELL 0

/* spellbook re-use control; used when reading and when polymorphing */
#define MAX_SPELL_STUDY 3

struct spell {
    short sp_id;  /* spell id (== object.otyp) */
    xchar sp_lev; /* power level */
    int sp_know;  /* knowlege of spell */
};

/* levels of memory destruction with a scroll of amnesia */
#define ALL_MAP 0x1
#define ALL_SPELLS 0x2

#define decrnknow(spell) spl_book[spell].sp_know--
#define spellid(spell) spl_book[spell].sp_id
#define spellknow(spell) spl_book[spell].sp_know

#endif /* SPELL_H */

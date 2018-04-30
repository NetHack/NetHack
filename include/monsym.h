/* NetHack 3.6  monsym.h        $NHDT-Date: 1524689515 2018/04/25 20:51:55 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.11 $ */
/*      Copyright (c) 2016 by Pasi Kallinen              */
/* NetHack may be freely redistributed.  See license for details. */
/*      Monster symbols and creation information rev 1.0          */

#ifndef MONSYM_H
#define MONSYM_H

/*
 * Monster classes.  Below, are the corresponding default characters for
 * them.  Monster class 0 is not used or defined so we can use it as a
 * NULL character.
 */
enum mon_class_types {
    S_ANT = 1,
    S_BLOB,
    S_COCKATRICE,
    S_DOG,
    S_EYE,
    S_FELINE,
    S_GREMLIN,
    S_HUMANOID,
    S_IMP,
    S_JELLY,
    S_KOBOLD,
    S_LEPRECHAUN,
    S_MIMIC,
    S_NYMPH,
    S_ORC,
    S_PIERCER,
    S_QUADRUPED,
    S_RODENT,
    S_SPIDER,
    S_TRAPPER,
    S_UNICORN,
    S_VORTEX,
    S_WORM,
    S_XAN,
    S_LIGHT,
    S_ZRUTY,
    S_ANGEL,
    S_BAT,
    S_CENTAUR,
    S_DRAGON,
    S_ELEMENTAL,
    S_FUNGUS,
    S_GNOME,
    S_GIANT,
    S_invisible,    /* non-class present in def_monsyms[] */
    S_JABBERWOCK,
    S_KOP,
    S_LICH,
    S_MUMMY,
    S_NAGA,
    S_OGRE,
    S_PUDDING,
    S_QUANTMECH,
    S_RUSTMONST,
    S_SNAKE,
    S_TROLL,
    S_UMBER,
    S_VAMPIRE,
    S_WRAITH,
    S_XORN,
    S_YETI,
    S_ZOMBIE,
    S_HUMAN,
    S_GHOST,
    S_GOLEM,
    S_DEMON,
    S_EEL,
    S_LIZARD,

    S_WORM_TAIL,
    S_MIMIC_DEF,

    MAXMCLASSES /* number of monster classes */
};

/*
 * Default characters for monsters.  These correspond to the monster classes
 * above.
 */
/* clang-format off */
#define DEF_ANT         'a'
#define DEF_BLOB        'b'
#define DEF_COCKATRICE  'c'
#define DEF_DOG         'd'
#define DEF_EYE         'e'
#define DEF_FELINE      'f'
#define DEF_GREMLIN     'g'
#define DEF_HUMANOID    'h'
#define DEF_IMP         'i'
#define DEF_JELLY       'j'
#define DEF_KOBOLD      'k'
#define DEF_LEPRECHAUN  'l'
#define DEF_MIMIC       'm'
#define DEF_NYMPH       'n'
#define DEF_ORC         'o'
#define DEF_PIERCER     'p'
#define DEF_QUADRUPED   'q'
#define DEF_RODENT      'r'
#define DEF_SPIDER      's'
#define DEF_TRAPPER     't'
#define DEF_UNICORN     'u'
#define DEF_VORTEX      'v'
#define DEF_WORM        'w'
#define DEF_XAN         'x'
#define DEF_LIGHT       'y'
#define DEF_ZRUTY       'z'
#define DEF_ANGEL       'A'
#define DEF_BAT         'B'
#define DEF_CENTAUR     'C'
#define DEF_DRAGON      'D'
#define DEF_ELEMENTAL   'E'
#define DEF_FUNGUS      'F'
#define DEF_GNOME       'G'
#define DEF_GIANT       'H'
#define DEF_JABBERWOCK  'J'
#define DEF_KOP         'K'
#define DEF_LICH        'L'
#define DEF_MUMMY       'M'
#define DEF_NAGA        'N'
#define DEF_OGRE        'O'
#define DEF_PUDDING     'P'
#define DEF_QUANTMECH   'Q'
#define DEF_RUSTMONST   'R'
#define DEF_SNAKE       'S'
#define DEF_TROLL       'T'
#define DEF_UMBER       'U'
#define DEF_VAMPIRE     'V'
#define DEF_WRAITH      'W'
#define DEF_XORN        'X'
#define DEF_YETI        'Y'
#define DEF_ZOMBIE      'Z'
#define DEF_HUMAN       '@'
#define DEF_GHOST       ' '
#define DEF_GOLEM       '\''
#define DEF_DEMON       '&'
#define DEF_EEL         ';'
#define DEF_LIZARD      ':'

#define DEF_INVISIBLE   'I'
#define DEF_WORM_TAIL   '~'
#define DEF_MIMIC_DEF   ']'
/* clang-format on */

#endif /* MONSYM_H */

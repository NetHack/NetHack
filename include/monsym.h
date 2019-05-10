/* NetHack 3.6  monsym.h        $NHDT-Date: 1547428769 2019/01/14 01:19:29 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.12 $ */
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
    S_ANT        =  1, /* a */
    S_BLOB       =  2, /* b */
    S_COCKATRICE =  3, /* c */
    S_DOG        =  4, /* d */
    S_EYE        =  5, /* e */
    S_FELINE     =  6, /* f: cats */
    S_GREMLIN    =  7, /* g */
    S_HUMANOID   =  8, /* h: small humanoids: hobbit, dwarf */
    S_IMP        =  9, /* i: minor demons */
    S_JELLY      = 10, /* j */
    S_KOBOLD     = 11, /* k */
    S_LEPRECHAUN = 12, /* l */
    S_MIMIC      = 13, /* m */
    S_NYMPH      = 14, /* n */
    S_ORC        = 15, /* o */
    S_PIERCER    = 16, /* p */
    S_QUADRUPED  = 17, /* q: excludes horses */
    S_RODENT     = 18, /* r */
    S_SPIDER     = 19, /* s */
    S_TRAPPER    = 20, /* t */
    S_UNICORN    = 21, /* u: includes horses */
    S_VORTEX     = 22, /* v */
    S_WORM       = 23, /* w */
    S_XAN        = 24, /* x */
    S_LIGHT      = 25, /* y: yellow light, black light */
    S_ZRUTY      = 26, /* z */
    S_ANGEL      = 27, /* A */
    S_BAT        = 28, /* B */
    S_CENTAUR    = 29, /* C */
    S_DRAGON     = 30, /* D */
    S_ELEMENTAL  = 31, /* E: includes invisible stalker */
    S_FUNGUS     = 32, /* F */
    S_GNOME      = 33, /* G */
    S_GIANT      = 34, /* H: large humanoid: giant, ettin, minotaur */
    S_invisible  = 35, /* I: non-class present in def_monsyms[] */
    S_JABBERWOCK = 36, /* J */
    S_KOP        = 37, /* K */
    S_LICH       = 38, /* L */
    S_MUMMY      = 39, /* M */
    S_NAGA       = 40, /* N */
    S_OGRE       = 41, /* O */
    S_PUDDING    = 42, /* P */
    S_QUANTMECH  = 43, /* Q */
    S_RUSTMONST  = 44, /* R */
    S_SNAKE      = 45, /* S */
    S_TROLL      = 46, /* T */
    S_UMBER      = 47, /* U: umber hulk */
    S_VAMPIRE    = 48, /* V */
    S_WRAITH     = 49, /* W */
    S_XORN       = 50, /* X */
    S_YETI       = 51, /* Y: includes owlbear, monkey */
    S_ZOMBIE     = 52, /* Z */
    S_HUMAN      = 53, /* @ */
    S_GHOST      = 54, /* <space> */
    S_GOLEM      = 55, /* ' */
    S_DEMON      = 56, /* & */
    S_EEL        = 57, /* ; (fish) */
    S_LIZARD     = 58, /* : (reptiles) */

    S_WORM_TAIL  = 59, /* ~ */
    S_MIMIC_DEF  = 60, /* ] */

    MAXMCLASSES  = 61  /* number of monster classes */
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

/* NetHack 3.7	sym.h */
/*      Copyright (c) 2016 by Pasi Kallinen              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SYM_H
#define SYM_H

/*
 * Default characters for monsters.
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

enum mon_class_types {
#define MONSYMS_ENUM
#include "defsym.h"
#undef MONSYMS_ENUM

    MAXMCLASSES  /* number of monster classes */
};

#ifndef MAKEDEFS_C

/* Default characters for object classes */

#define ILLOBJ_SYM  ']' /* also used for mimics */
#define WEAPON_SYM  ')'
#define ARMOR_SYM   '['
#define RING_SYM    '='
#define AMULET_SYM  '"'
#define TOOL_SYM    '('
#define FOOD_SYM    '%'
#define POTION_SYM  '!'
#define SCROLL_SYM  '?'
#define SPBOOK_SYM  '+'
#define WAND_SYM    '/'
#define GOLD_SYM    '$'
#define GEM_SYM     '*'
#define ROCK_SYM    '`'
#define BALL_SYM    '0'
#define CHAIN_SYM   '_'
#define VENOM_SYM   '.'

/* Default characters for dungeon surroundings and furniture */
enum screen_symbols {
#define PCHAR_ENUM
#include "defsym.h"
#undef PCHAR_ENUM
    MAXPCHARS
};

struct symdef {
    uchar sym;
    const char *explanation;
#ifdef TEXTCOLOR
    uchar color;
#endif
};

enum symparse_range {
    SYM_CONTROL = 1, /* start/finish markers */
    SYM_PCHAR = 2,   /* index into showsyms  */
    SYM_OC = 3,      /* index into oc_syms   */
    SYM_MON = 4,     /* index into monsyms   */
    SYM_OTH = 5      /* misc                 */
};

struct symparse {
    unsigned range;
    int idx;
    const char *name;
};

/* linked list of symsets and their characteristics */
struct symsetentry {
    struct symsetentry *next; /* next in list                         */
    char *name;               /* ptr to symset name                   */
    char *desc;               /* ptr to description                   */
    int idx;                  /* an index value                       */
    int handling;             /* known handlers value                 */
    Bitfield(nocolor, 1);     /* don't use color if set               */
    Bitfield(primary, 1);     /* restricted for use as primary set    */
    Bitfield(rogue, 1);       /* restricted for use as rogue lev set  */
    Bitfield(explicitly, 1);  /* explicit symset set                  */
                              /* 4 free bits */
};

/*
 *
 */

#define MAXDCHARS (S_water - S_stone + 1) /* mapped dungeon characters */
#define MAXTCHARS (S_vibrating_square - S_arrow_trap + 1) /* trap chars */
#define MAXECHARS (S_explode9 - S_vbeam + 1) /* mapped effects characters */
#define MAXEXPCHARS 9 /* number of explosion characters */

#define DARKROOMSYM (Is_rogue_level(&u.uz) ? S_stone : S_darkroom)

#define is_cmap_trap(i) ((i) >= S_arrow_trap && (i) <= S_polymorph_trap)
#define is_cmap_drawbridge(i) ((i) >= S_vodbridge && (i) <= S_hcdbridge)
#define is_cmap_door(i) ((i) >= S_vodoor && (i) <= S_hcdoor)
#define is_cmap_wall(i) ((i) >= S_stone && (i) <= S_trwall)
#define is_cmap_room(i) ((i) >= S_room && (i) <= S_darkroom)
#define is_cmap_corr(i) ((i) >= S_corr && (i) <= S_litcorr)
#define is_cmap_furniture(i) ((i) >= S_upstair && (i) <= S_fountain)
#define is_cmap_water(i) ((i) == S_pool || (i) == S_water)
#define is_cmap_lava(i) ((i) == S_lava)
#define is_cmap_stairs(i) ((i) == S_upstair || (i) == S_dnstair || \
                           (i) == S_upladder || (i) == S_dnladder)

/* misc symbol definitions */
enum misc_symbols {
        SYM_NOTHING = 0,
        SYM_UNEXPLORED = 1,
        SYM_BOULDER = 2,
        SYM_INVISIBLE = 3,
        SYM_PET_OVERRIDE = 4,
        SYM_HERO_OVERRIDE = 5,
        MAXOTHER
};

/*
 * Graphics sets for display symbols
 */
#define DEFAULT_GRAPHICS 0 /* regular characters: '-', '+', &c */
enum graphics_sets {
        PRIMARY = 0,          /* primary graphics set        */
        ROGUESET = 1,         /* rogue graphics set          */
        NUM_GRAPHICS
};

/*
 * special symbol set handling types ( for invoking callbacks, etc.)
 * Must match the order of the known_handlers strings
 * in drawing.c
 */

enum symset_handling_types {
        H_UNK  = 0,
        H_IBM  = 1,
        H_DEC  = 2,
        H_CURS = 3,
        H_MAC  = 4 /* obsolete; needed so that the listing of available
                     * symsets by 'O' can skip it for !MAC_GRAPHICS_ENV */
};

extern const struct symdef defsyms[MAXPCHARS]; /* defaults */
#define WARNCOUNT 6 /* number of different warning levels */
extern const struct symdef def_warnsyms[WARNCOUNT];
#define SYMHANDLING(ht) (g.symset[g.currentgraphics].handling == (ht))

#endif /* !MAKEDEFS_C */
#endif /* SYM_H */


/* NetHack 3.6	drawing.c	$NHDT-Date: 1588778111 2020/05/06 15:15:11 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.76 $ */
/* Copyright (c) NetHack Development Team 1992.                   */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "color.h"
#include "rm.h"
#include "objclass.h"
#include "monsym.h"
#include "tcap.h"

/* Relevant header information in rm.h, objclass.h, and monsym.h. */

#ifdef C
#undef C
#endif

#ifdef TEXTCOLOR
#define C(n) n
#else
#define C(n)
#endif

/* Default object class symbols.  See objclass.h.
 * {symbol, name, explain}
 *     name:    used in object_detect().
 *     explain: used in do_look().
 */
const struct class_sym def_oc_syms[MAXOCLASSES] = {
    { '\0', "", "" }, /* placeholder for the "random class" */
    { ILLOBJ_SYM, "illegal objects", "strange object" },
    { WEAPON_SYM, "weapons", "weapon" },
    { ARMOR_SYM, "armor", "suit or piece of armor" },
    { RING_SYM, "rings", "ring" },
    { AMULET_SYM, "amulets", "amulet" },
    { TOOL_SYM, "tools", "useful item (pick-axe, key, lamp...)" },
    { FOOD_SYM, "food", "piece of food" },
    { POTION_SYM, "potions", "potion" },
    { SCROLL_SYM, "scrolls", "scroll" },
    { SPBOOK_SYM, "spellbooks", "spellbook" },
    { WAND_SYM, "wands", "wand" },
    { GOLD_SYM, "coins", "pile of coins" },
    { GEM_SYM, "rocks", "gem or rock" },
    { ROCK_SYM, "large stones", "boulder or statue" },
    { BALL_SYM, "iron balls", "iron ball" },
    { CHAIN_SYM, "chains", "iron chain" },
    { VENOM_SYM, "venoms", "splash of venom" }
};

/* Default monster class symbols.  See monsym.h. */
const struct class_sym def_monsyms[MAXMCLASSES] = {
    { '\0', "", "" },
    { DEF_ANT, "", "ant or other insect" },
    { DEF_BLOB, "", "blob" },
    { DEF_COCKATRICE, "", "cockatrice" },
    { DEF_DOG, "", "dog or other canine" },
    { DEF_EYE, "", "eye or sphere" },
    { DEF_FELINE, "", "cat or other feline" },
    { DEF_GREMLIN, "", "gremlin" },
    { DEF_HUMANOID, "", "humanoid" },
    { DEF_IMP, "", "imp or minor demon" },
    { DEF_JELLY, "", "jelly" },
    { DEF_KOBOLD, "", "kobold" },
    { DEF_LEPRECHAUN, "", "leprechaun" },
    { DEF_MIMIC, "", "mimic" },
    { DEF_NYMPH, "", "nymph" },
    { DEF_ORC, "", "orc" },
    { DEF_PIERCER, "", "piercer" },
    { DEF_QUADRUPED, "", "quadruped" },
    { DEF_RODENT, "", "rodent" },
    { DEF_SPIDER, "", "arachnid or centipede" },
    { DEF_TRAPPER, "", "trapper or lurker above" },
    { DEF_UNICORN, "", "unicorn or horse" },
    { DEF_VORTEX, "", "vortex" },
    { DEF_WORM, "", "worm" },
    { DEF_XAN, "", "xan or other mythical/fantastic insect" },
    { DEF_LIGHT, "", "light" },
    { DEF_ZRUTY, "", "zruty" },
    { DEF_ANGEL, "", "angelic being" },
    { DEF_BAT, "", "bat or bird" },
    { DEF_CENTAUR, "", "centaur" },
    { DEF_DRAGON, "", "dragon" },
    { DEF_ELEMENTAL, "", "elemental" },
    { DEF_FUNGUS, "", "fungus or mold" },
    { DEF_GNOME, "", "gnome" },
    { DEF_GIANT, "", "giant humanoid" },
    { '\0', "", "invisible monster" },
    { DEF_JABBERWOCK, "", "jabberwock" },
    { DEF_KOP, "", "Keystone Kop" },
    { DEF_LICH, "", "lich" },
    { DEF_MUMMY, "", "mummy" },
    { DEF_NAGA, "", "naga" },
    { DEF_OGRE, "", "ogre" },
    { DEF_PUDDING, "", "pudding or ooze" },
    { DEF_QUANTMECH, "", "quantum mechanic" },
    { DEF_RUSTMONST, "", "rust monster or disenchanter" },
    { DEF_SNAKE, "", "snake" },
    { DEF_TROLL, "", "troll" },
    { DEF_UMBER, "", "umber hulk" },
    { DEF_VAMPIRE, "", "vampire" },
    { DEF_WRAITH, "", "wraith" },
    { DEF_XORN, "", "xorn" },
    { DEF_YETI, "", "apelike creature" },
    { DEF_ZOMBIE, "", "zombie" },
    { DEF_HUMAN, "", "human or elf" },
    { DEF_GHOST, "", "ghost" },
    { DEF_GOLEM, "", "golem" },
    { DEF_DEMON, "", "major demon" },
    { DEF_EEL, "", "sea monster" },
    { DEF_LIZARD, "", "lizard" },
    { DEF_WORM_TAIL, "", "long worm tail" },
    { DEF_MIMIC_DEF, "", "mimic" },
};

const struct symdef def_warnsyms[WARNCOUNT] = {
    /* white warning  */
    { '0', "unknown creature causing you worry",    C(CLR_WHITE) },
    /* pink warning   */
    { '1', "unknown creature causing you concern",  C(CLR_RED) },
    /* red warning    */
    { '2', "unknown creature causing you anxiety",  C(CLR_RED) },
    /* ruby warning   */
    { '3', "unknown creature causing you disquiet", C(CLR_RED) },
    /* purple warning */
    { '4', "unknown creature causing you alarm",    C(CLR_MAGENTA) },
    /* black warning  */
    { '5', "unknown creature causing you dread",    C(CLR_BRIGHT_MAGENTA) },
};

/*
 *  Default screen symbols with explanations and colors.
 */
const struct symdef defsyms[MAXPCHARS] = {
/* 0*/ { ' ', "stone", C(NO_COLOR) },                /* stone */
       { '|', "wall", C(CLR_GRAY) },                 /* vwall */
       { '-', "wall", C(CLR_GRAY) },                 /* hwall */
       { '-', "wall", C(CLR_GRAY) },                 /* tlcorn */
       { '-', "wall", C(CLR_GRAY) },                 /* trcorn */
       { '-', "wall", C(CLR_GRAY) },                 /* blcorn */
       { '-', "wall", C(CLR_GRAY) },                 /* brcorn */
       { '-', "wall", C(CLR_GRAY) },                 /* crwall */
       { '-', "wall", C(CLR_GRAY) },                 /* tuwall */
       { '-', "wall", C(CLR_GRAY) },                 /* tdwall */
/*10*/ { '|', "wall", C(CLR_GRAY) },                 /* tlwall */
       { '|', "wall", C(CLR_GRAY) },                 /* trwall */
       { '.', "doorway", C(CLR_GRAY) },              /* ndoor */
       { '-', "open door", C(CLR_BROWN) },           /* vodoor */
       { '|', "open door", C(CLR_BROWN) },           /* hodoor */
       { '+', "closed door", C(CLR_BROWN) },         /* vcdoor */
       { '+', "closed door", C(CLR_BROWN) },         /* hcdoor */
       { '#', "iron bars", C(HI_METAL) },            /* bars */
       { '#', "tree", C(CLR_GREEN) },                /* tree */
       { '.', "floor of a room", C(CLR_GRAY) },      /* room */
/*20*/ { '.', "dark part of a room", C(CLR_BLACK) }, /* dark room */
       { '#', "corridor", C(CLR_GRAY) },             /* dark corr */
       { '#', "lit corridor", C(CLR_GRAY) },   /* lit corr (see mapglyph.c) */
       { '<', "staircase up", C(CLR_GRAY) },         /* upstair */
       { '>', "staircase down", C(CLR_GRAY) },       /* dnstair */
       { '<', "ladder up", C(CLR_BROWN) },           /* upladder */
       { '>', "ladder down", C(CLR_BROWN) },         /* dnladder */
       { '_', "altar", C(CLR_GRAY) },                /* altar */
       { '|', "grave", C(CLR_WHITE) },               /* grave */
       { '\\', "opulent throne", C(HI_GOLD) },       /* throne */
/*30*/ { '#', "sink", C(CLR_GRAY) },                 /* sink */
       { '{', "fountain", C(CLR_BRIGHT_BLUE) },      /* fountain */
       { '}', "water", C(CLR_BLUE) },                /* pool */
       { '.', "ice", C(CLR_CYAN) },                  /* ice */
       { '}', "molten lava", C(CLR_RED) },           /* lava */
       { '.', "lowered drawbridge", C(CLR_BROWN) },  /* vodbridge */
       { '.', "lowered drawbridge", C(CLR_BROWN) },  /* hodbridge */
       { '#', "raised drawbridge", C(CLR_BROWN) },   /* vcdbridge */
       { '#', "raised drawbridge", C(CLR_BROWN) },   /* hcdbridge */
       { ' ', "air", C(CLR_CYAN) },                  /* open air */
/*40*/ { '#', "cloud", C(CLR_GRAY) },                /* [part of] a cloud */
       { '}', "water", C(CLR_BLUE) },                /* under water */
       { '^', "arrow trap", C(HI_METAL) },           /* trap */
       { '^', "dart trap", C(HI_METAL) },            /* trap */
       { '^', "falling rock trap", C(CLR_GRAY) },    /* trap */
       { '^', "squeaky board", C(CLR_BROWN) },       /* trap */
       { '^', "bear trap", C(HI_METAL) },            /* trap */
       { '^', "land mine", C(CLR_RED) },             /* trap */
       { '^', "rolling boulder trap", C(CLR_GRAY) }, /* trap */
       { '^', "sleeping gas trap", C(HI_ZAP) },      /* trap */
/*50*/ { '^', "rust trap", C(CLR_BLUE) },            /* trap */
       { '^', "fire trap", C(CLR_ORANGE) },          /* trap */
       { '^', "pit", C(CLR_BLACK) },                 /* trap */
       { '^', "spiked pit", C(CLR_BLACK) },          /* trap */
       { '^', "hole", C(CLR_BROWN) },                /* trap */
       { '^', "trap door", C(CLR_BROWN) },           /* trap */
       { '^', "teleportation trap", C(CLR_MAGENTA) },  /* trap */
       { '^', "level teleporter", C(CLR_MAGENTA) },    /* trap */
       { '^', "magic portal", C(CLR_BRIGHT_MAGENTA) }, /* trap */
       { '"', "web", C(CLR_GRAY) },                    /* web */
/*60*/ { '^', "statue trap", C(CLR_GRAY) },            /* trap */
       { '^', "magic trap", C(HI_ZAP) },               /* trap */
       { '^', "anti-magic field", C(HI_ZAP) },         /* trap */
       { '^', "polymorph trap", C(CLR_BRIGHT_GREEN) }, /* trap */
       { '~', "vibrating square", C(CLR_MAGENTA) },    /* "trap" */
       /* zap colors are changed by mapglyph() to match type of beam */
       { '|', "", C(CLR_GRAY) },                /* vbeam */
       { '-', "", C(CLR_GRAY) },                /* hbeam */
       { '\\', "", C(CLR_GRAY) },               /* lslant */
       { '/', "", C(CLR_GRAY) },                /* rslant */
       { '*', "", C(CLR_WHITE) },               /* dig beam */
       { '!', "", C(CLR_WHITE) },               /* camera flash beam */
       { ')', "", C(HI_WOOD) },                 /* boomerang open left */
/*70*/ { '(', "", C(HI_WOOD) },                 /* boomerang open right */
       { '0', "", C(HI_ZAP) },                  /* 4 magic shield symbols */
       { '#', "", C(HI_ZAP) },
       { '@', "", C(HI_ZAP) },
       { '*', "", C(HI_ZAP) },
       { '#', "poison cloud", C(CLR_BRIGHT_GREEN) },   /* part of a cloud */
       { '?', "valid position", C(CLR_BRIGHT_GREEN) }, /*  target position */
       /* swallow colors are changed by mapglyph() to match engulfing monst */
       { '/', "", C(CLR_GREEN) },         /* swallow top left      */
       { '-', "", C(CLR_GREEN) },         /* swallow top center    */
       { '\\', "", C(CLR_GREEN) },        /* swallow top right     */
/*80*/ { '|', "", C(CLR_GREEN) },         /* swallow middle left   */
       { '|', "", C(CLR_GREEN) },         /* swallow middle right  */
       { '\\', "", C(CLR_GREEN) },        /* swallow bottom left   */
       { '-', "", C(CLR_GREEN) },         /* swallow bottom center */
       { '/', "", C(CLR_GREEN) },         /* swallow bottom right  */
       /* explosion colors are changed by mapglyph() to match type of expl. */
       { '/', "", C(CLR_ORANGE) },        /* explosion top left     */
       { '-', "", C(CLR_ORANGE) },        /* explosion top center   */
       { '\\', "", C(CLR_ORANGE) },       /* explosion top right    */
       { '|', "", C(CLR_ORANGE) },        /* explosion middle left  */
       { ' ', "", C(CLR_ORANGE) },        /* explosion middle center*/
/*90*/ { '|', "", C(CLR_ORANGE) },        /* explosion middle right */
       { '\\', "", C(CLR_ORANGE) },       /* explosion bottom left  */
       { '-', "", C(CLR_ORANGE) },        /* explosion bottom center*/
       { '/', "", C(CLR_ORANGE) },        /* explosion bottom right */
};

/* default rogue level symbols */
const uchar def_r_oc_syms[MAXOCLASSES] = {
/* 0*/ '\0', ILLOBJ_SYM, WEAPON_SYM, ']', /* armor */
       RING_SYM,
/* 5*/ ',',                     /* amulet */
       TOOL_SYM, ':',           /* food */
       POTION_SYM, SCROLL_SYM,
/*10*/ SPBOOK_SYM, WAND_SYM,
       GEM_SYM,                /* gold -- yes it's the same as gems */
       GEM_SYM, ROCK_SYM,
/*15*/ BALL_SYM, CHAIN_SYM, VENOM_SYM
};

#undef C

/*
 * Convert the given character to an object class.  If the character is not
 * recognized, then MAXOCLASSES is returned.  Used in detect.c, invent.c,
 * objnam.c, options.c, pickup.c, sp_lev.c, lev_main.c, and tilemap.c.
 */
int
def_char_to_objclass(ch)
char ch;
{
    int i;

    for (i = 1; i < MAXOCLASSES; i++)
        if (ch == def_oc_syms[i].sym)
            break;
    return i;
}

/*
 * Convert a character into a monster class.  This returns the _first_
 * match made.  If there are are no matches, return MAXMCLASSES.
 * Used in detect.c, options.c, read.c, sp_lev.c, and lev_main.c
 */
int
def_char_to_monclass(ch)
char ch;
{
    int i;

    for (i = 1; i < MAXMCLASSES; i++)
        if (ch == def_monsyms[i].sym)
            break;
    return i;
}

/* does 'ch' represent a furniture character?  returns index into defsyms[] */
int
def_char_is_furniture(ch)
char ch;
{
    /* note: these refer to defsyms[] order which is much different from
       levl[][].typ order but both keep furniture in a contiguous block */
    static const char first_furniture[] = "stair", /* "staircase up" */
                      last_furniture[] = "fountain";
    int i;
    boolean furniture = FALSE;

    for (i = 0; i < SIZE(defsyms); ++i) {
        if (!furniture) {
            if (!strncmp(defsyms[i].explanation, first_furniture, 5))
                furniture = TRUE;
        }
        if (furniture) {
            if (defsyms[i].sym == (uchar) ch)
                return i;
            if (!strcmp(defsyms[i].explanation, last_furniture))
                break; /* reached last furniture */
        }
    }
    return -1;
}

/*drawing.c*/

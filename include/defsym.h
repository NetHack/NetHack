/* NetHack 3.7	defsym.h */
/*      Copyright (c) 2016 by Pasi Kallinen              */
/* NetHack may be freely redistributed.  See license for details. */


/*
    This header is included in multiple places to produce
    different code depending on its use. Its purpose is to
    ensure that there is only one definitive source for
    pchar, objclass and mon symbols.

    The morphing macro expansions are used in these places:
  - in include/sym.h for enums of some S_ symbol values
    (define PCHAR_ENUM, MONSYMS_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of some *_CLASS values
    (define OBJCLASS_ENUM prior to #include defsym.h)
  - in src/symbols.c for parsing S_ entries in config files
    (define PCHAR_PARSE, MONSYMS_PARSE, OBJCLASS_PARSE prior
    to #include defsym.h)
  - in src/drawing.c for initializing some data structures/arrays
    (define PCHAR_DRAWING, MONSYMS_DRAWING, OBJCLASS_DRAWING prior
    to #include defsym.h)
  - in win/share/tilemap.c for processing a tile file
    (define PCHAR_TILES prior to #include defsym.h).
*/

#ifdef CLR
#undef CLR
#endif

#ifdef TEXTCOLOR
#define CLR(n) n
#else
#define CLR(n)
#endif

#if defined(PCHAR_ENUM) || defined(PCHAR_PARSE) || defined(PCHAR_DRAWING) || defined(PCHAR_TILES)

/*
   PCHAR(idx, ch, sym, desc, clr)
       idx:     index used in enum
       ch:      character symbol
       sym:     symbol name for parsing purposes
       desc:    description
       clr:     color

   PCHAR2(idx, ch, sym, tilenm, desc, clr)
       idx:     index used in enum
       ch:      character symbol
       sym:     symbol name for parsing purposes
       desc:    description
       clr:     color
*/

#if defined(PCHAR_ENUM)
/* sym.h */
#define PCHAR(idx, ch, sym, desc, clr) \
        sym = idx,
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) \
        sym = idx,

#elif defined(PCHAR_PARSE)
/* symbols.c */
#define PCHAR(idx, ch, sym, desc, clr) \
    { SYM_PCHAR, sym, #sym },
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) \
    { SYM_PCHAR, sym, #sym },

#elif defined(PCHAR_DRAWING)
/* drawing.c */
#define PCHAR(idx, ch, sym, desc, clr) \
    { ch, desc, clr },
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) \
    { ch, desc, clr },

#elif defined(PCHAR_TILES)
/* win/share/tilemap.c */
#define PCHAR(idx, ch, sym, desc, clr) \
    { sym, desc, desc },
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) \
    { sym, tilenm, desc },
#endif

    PCHAR2( 0, ' ',  S_stone, "dark part of a room", "stone",  CLR(NO_COLOR))
    PCHAR2( 1, '|',  S_vwall, "vertical wall", "wall",  CLR(CLR_GRAY))
    PCHAR2( 2, '-',  S_hwall, "horizontal wall", "wall",  CLR(CLR_GRAY))
    PCHAR2( 3, '-',  S_tlcorn, "top left corner wall", "wall",  CLR(CLR_GRAY))
    PCHAR2( 4, '-',  S_trcorn, "top right corner wall", "wall",  CLR(CLR_GRAY))
    PCHAR2( 5, '-',  S_blcorn, "bottom left corner wall", "wall", CLR(CLR_GRAY))
    PCHAR2( 6, '-',  S_brcorn, "bottom right corner wall", "wall", CLR(CLR_GRAY))
    PCHAR2( 7, '-',  S_crwall, "cross wall", "wall", CLR(CLR_GRAY))
    PCHAR2( 8, '-',  S_tuwall, "tuwall", "wall", CLR(CLR_GRAY))
    PCHAR2( 9, '-',  S_tdwall, "tdwall", "wall", CLR(CLR_GRAY))
    PCHAR2(10, '|',  S_tlwall, "tlwall", "wall", CLR(CLR_GRAY))
    PCHAR2(11, '|',  S_trwall, "trwall", "wall", CLR(CLR_GRAY))
    PCHAR2(12, '.',  S_ndoor, "no door", "doorway", CLR(CLR_GRAY))
    PCHAR2(13, '-',  S_vodoor, "vertical open door", "open door", CLR(CLR_BROWN))
    PCHAR2(14, '|',  S_hodoor, "horizontal open door", "open door", CLR(CLR_BROWN))
    PCHAR( 15, '+',  S_vcdoor, "vertical closed door", CLR(CLR_BROWN))
    PCHAR( 16, '+',  S_hcdoor, "horizontal closed door", CLR(CLR_BROWN))
    PCHAR( 17, '#',  S_bars, "iron bars", CLR(HI_METAL))
    PCHAR( 18, '#',  S_tree, "tree", CLR(CLR_GREEN))
    PCHAR( 19, '.',  S_room, "floor of a room", CLR(CLR_GRAY))
    PCHAR( 20, '.',  S_darkroom, "dark part of a room", CLR(CLR_BLACK))
    PCHAR2(21, '#',  S_corr, "dark corridor", "corridor", CLR(CLR_GRAY))
    PCHAR( 22, '#',  S_litcorr, "lit corridor", CLR(CLR_GRAY))
    PCHAR2(23, '<',  S_upstair, "up stairs", "staircase up", CLR(CLR_GRAY))
    PCHAR2(24, '>',  S_dnstair, "down stairs", "staircase down", CLR(CLR_GRAY))
    PCHAR2(25, '<',  S_upladder, "up ladder", "ladder up", CLR(CLR_BROWN))
    PCHAR2(26, '>',  S_dnladder, "down ladder", "ladder down", CLR(CLR_BROWN))
    PCHAR( 27, '<',  S_brupstair, "branch staircase up", CLR(CLR_YELLOW))
    PCHAR( 28, '>',  S_brdnstair, "branch staircase down", CLR(CLR_YELLOW))
    PCHAR( 29, '<',  S_brupladder, "branch ladder up", CLR(CLR_YELLOW))
    PCHAR( 30, '>',  S_brdnladder, "branch ladder down", CLR(CLR_YELLOW))
    PCHAR( 31, '_',  S_altar, "altar", CLR(CLR_GRAY))
    PCHAR( 32, '|',  S_grave, "grave", CLR(CLR_WHITE))
    PCHAR2(33, '\\', S_throne, "throne", "opulent throne", CLR(HI_GOLD))
    PCHAR( 34, '#',  S_sink, "sink", CLR(CLR_GRAY))
    PCHAR( 35, '{',  S_fountain, "fountain", CLR(CLR_BRIGHT_BLUE))
    PCHAR2(36, '}',  S_pool, "pool", "water", CLR(CLR_BLUE))
    PCHAR( 37, '.',  S_ice, "ice", CLR(CLR_CYAN))
    PCHAR( 38, '}',  S_lava, "molten lava", CLR(CLR_RED))
    PCHAR2(39, '.',  S_vodbridge, "vertical open drawbridge",
                                  "lowered drawbridge", CLR(CLR_BROWN))
    PCHAR2(40, '.',  S_hodbridge, "horizontal open drawbridge",
                                  "lowered drawbridge", CLR(CLR_BROWN))
    PCHAR2(41, '#',  S_vcdbridge, "vertical closed drawbridge",
                                  "raised drawbridge", CLR(CLR_BROWN))
    PCHAR2(42, '#',  S_hcdbridge, "horizontal closed drawbridge",
                                  "raised drawbridge", CLR(CLR_BROWN))
    PCHAR( 43, ' ',  S_air, "air", CLR(CLR_CYAN))
    PCHAR( 44, '#',  S_cloud, "cloud", CLR(CLR_GRAY))
    PCHAR( 45, '}',  S_water, "water", CLR(CLR_BLUE))
    /* end dungeon characters, begin traps */
    PCHAR( 46, '^',  S_arrow_trap, "arrow trap", CLR(HI_METAL))
    PCHAR( 47, '^',  S_dart_trap, "dart trap", CLR(HI_METAL))
    PCHAR( 48, '^',  S_falling_rock_trap, "falling rock trap", CLR(CLR_GRAY))
    PCHAR( 49, '^',  S_squeaky_board, "squeaky board", CLR(CLR_BROWN))
    PCHAR( 50, '^',  S_bear_trap, "bear trap", CLR(HI_METAL))
    PCHAR( 51, '^',  S_land_mine, "land mine", CLR(CLR_RED))
    PCHAR( 52, '^',  S_rolling_boulder_trap, "rolling boulder trap", CLR(CLR_GRAY))
    PCHAR( 53, '^',  S_sleeping_gas_trap, "sleeping gas trap", CLR(HI_ZAP))
    PCHAR( 54, '^',  S_rust_trap, "rust trap", CLR(CLR_BLUE))
    PCHAR( 55, '^',  S_fire_trap, "fire trap", CLR(CLR_ORANGE))
    PCHAR( 56, '^',  S_pit, "pit", CLR(CLR_BLACK))
    PCHAR( 57, '^',  S_spiked_pit, "spiked pit", CLR(CLR_BLACK))
    PCHAR( 58, '^',  S_hole, "hole", CLR(CLR_BROWN))
    PCHAR( 59, '^',  S_trap_door, "trap door", CLR(CLR_BROWN))
    PCHAR( 60, '^',  S_teleportation_trap, "teleportation trap", CLR(CLR_MAGENTA))
    PCHAR( 61, '^',  S_level_teleporter, "level teleporter", CLR(CLR_MAGENTA))
    PCHAR( 62, '^',  S_magic_portal, "magic portal", CLR(CLR_BRIGHT_MAGENTA))
    PCHAR( 63, '"',  S_web, "web", CLR(CLR_GRAY))
    PCHAR( 64, '^',  S_statue_trap, "statue trap", CLR(CLR_GRAY))
    PCHAR( 65, '^',  S_magic_trap, "magic trap", CLR(HI_ZAP))
    PCHAR2(66, '^',  S_anti_magic_trap, "anti magic trap", "anti-magic field", CLR(HI_ZAP))
    PCHAR( 67, '^',  S_polymorph_trap, "polymorph trap", CLR(CLR_BRIGHT_GREEN))
    PCHAR( 68, '~',  S_vibrating_square, "vibrating square", CLR(CLR_MAGENTA))
    /* end traps, begin special effects */
    /* zap colors are changed by map_glyphinfo() to match type of beam */
    PCHAR2(69, '|',  S_vbeam, "vertical beam", "", CLR(CLR_GRAY))
    PCHAR2(70, '-',  S_hbeam, "horizontal beam", "", CLR(CLR_GRAY))
    PCHAR2(71, '\\', S_lslant, "left slant beam", "", CLR(CLR_GRAY))
    PCHAR2(72, '/',  S_rslant, "right slant beam", "", CLR(CLR_GRAY))
    PCHAR2(73, '*',  S_digbeam, "dig beam", "", CLR(CLR_WHITE))
    PCHAR2(74, '!',  S_flashbeam, "flash beam", "", CLR(CLR_WHITE))
    PCHAR2(75, ')',  S_boomleft, "boom left", "", CLR(HI_WOOD))
    PCHAR2(76, '(',  S_boomright, "boom right", "", CLR(HI_WOOD))
    /* 4 magic shield symbols */
    PCHAR2(77, '0',  S_ss1, "shield1", "", CLR(HI_ZAP))
    PCHAR2(78, '#',  S_ss2, "shield2", "", CLR(HI_ZAP))
    PCHAR2(79, '@',  S_ss3, "shield3", "", CLR(HI_ZAP))
    PCHAR2(80, '*',  S_ss4, "shield4", "", CLR(HI_ZAP))
    PCHAR( 81, '#',  S_poisoncloud, "poison cloud", CLR(CLR_BRIGHT_GREEN))
    PCHAR( 82, '?',  S_goodpos, "valid position", CLR(CLR_BRIGHT_GREEN))
    /* The 8 swallow symbols.  Do NOT separate.  To change order or add, */
    /* see the function swallow_to_glyph() in display.c.                 */
    /* swallow colors are changed by map_glyphinfo() to match engulfing monst */
    PCHAR2(83, '/',  S_sw_tl, "swallow top left", "", CLR(CLR_GREEN))      /* 1        */
    PCHAR2(84, '-',  S_sw_tc, "swallow top center", "", CLR(CLR_GREEN))    /* 2 Order: */
    PCHAR2(85, '\\', S_sw_tr, "swallow top right", "", CLR(CLR_GREEN))     /* 3        */
    PCHAR2(86, '|',  S_sw_ml, "swallow middle left", "", CLR(CLR_GREEN))   /* 4  1 2 3 */
    PCHAR2(87, '|',  S_sw_mr, "swallow middle right", "", CLR(CLR_GREEN))  /* 6  4 5 6 */
    PCHAR2(88, '\\', S_sw_bl, "swallow bottom left", "", CLR(CLR_GREEN))   /* 7  7 8 9 */
    PCHAR2(89, '-',  S_sw_bc, "swallow bottom center", "", CLR(CLR_GREEN)) /* 8        */
    PCHAR2(90, '/',  S_sw_br, "swallow bottom right", "", CLR(CLR_GREEN))  /* 9        */
    /* explosion colors are changed by map_glyphinfo() to match type of expl. */
    PCHAR2(91, '/',  S_explode1, "explosion top left", "", CLR(CLR_ORANGE))      /*     */
    PCHAR2(92, '-',  S_explode2, "explosion top center", "", CLR(CLR_ORANGE))    /*     */
    PCHAR2(93, '\\', S_explode3, "explosion top right", "", CLR(CLR_ORANGE))     /*Ex.  */
    PCHAR2(94, '|',  S_explode4, "explosion middle left", "", CLR(CLR_ORANGE))   /*     */
    PCHAR2(95, ' ',  S_explode5, "explosion middle center", "", CLR(CLR_ORANGE)) /* /-\ */
    PCHAR2(96, '|',  S_explode6, "explosion middle right", "", CLR(CLR_ORANGE))  /* |@| */
    PCHAR2(97, '\\', S_explode7, "explosion bottom left", "", CLR(CLR_ORANGE))   /* \-/ */
    PCHAR2(98, '-',  S_explode8, "explosion bottom center", "", CLR(CLR_ORANGE)) /*     */
    PCHAR2(99, '/',  S_explode9, "explosion bottom right", "", CLR(CLR_ORANGE))  /*     */
#undef PCHAR
#undef PCHAR2
#endif /* PCHAR_ENUM || PCHAR_DEFSYMS || PCHAR_DRAWING || PCHAR_TILES */

#if defined(MONSYMS_ENUM) || defined(MONSYMS_PARSE) || defined (MONSYMS_DRAWING)

/*
    MONSYM(idx, ch, sym desc)
        idx:     index used in enum
        ch:      character symbol
        sym:     symbol name for parsing purposes
        desc:    description
*/

#if defined(MONSYMS_ENUM)
/* sym.h */
#define MONSYM(idx, ch, sym, desc) \
    sym = idx,

#elif defined(MONSYMS_PARSE)
/* symbols.c */
#define MONSYM(idx, ch, sym, desc) \
    { SYM_MON, sym + SYM_OFF_M, #sym },

#elif defined(MONSYMS_DRAWING)
/* drawing.c */
#define MONSYM(idx, ch, sym, desc) \
    { ch, "", desc },
#endif

    MONSYM( 1, DEF_ANT,  S_ANT, "ant or other insect")
    MONSYM( 2, DEF_BLOB, S_BLOB, "blob")
    MONSYM( 3, DEF_COCKATRICE, S_COCKATRICE, "cockatrice")
    MONSYM( 4, DEF_DOG, S_DOG, "dog or other canine")
    MONSYM( 5, DEF_EYE, S_EYE, "eye or sphere")
    MONSYM( 6, DEF_FELINE, S_FELINE, "cat or other feline")
    MONSYM( 7, DEF_GREMLIN, S_GREMLIN, "gremlin")
    /* small humanoids: hobbit, dwarf */
    MONSYM( 8, DEF_HUMANOID, S_HUMANOID, "humanoid")
    /* minor demons */
    MONSYM( 9, DEF_IMP, S_IMP, "imp or minor demon")
    MONSYM(10, DEF_JELLY, S_JELLY, "jelly")
    MONSYM(11, DEF_KOBOLD, S_KOBOLD, "kobold")
    MONSYM(12, DEF_LEPRECHAUN, S_LEPRECHAUN, "leprechaun")
    MONSYM(13, DEF_MIMIC, S_MIMIC, "mimic")    /* 'm' */
    MONSYM(14, DEF_NYMPH, S_NYMPH, "nymph")
    MONSYM(15, DEF_ORC, S_ORC, "orc")
    MONSYM(16, DEF_PIERCER, S_PIERCER, "piercer")
    /* quadruped excludes horses */
    MONSYM(17, DEF_QUADRUPED, S_QUADRUPED, "quadruped")
    MONSYM(18, DEF_RODENT, S_RODENT, "rodent")
    MONSYM(19, DEF_SPIDER, S_SPIDER, "arachnid or centipede")
    MONSYM(20, DEF_TRAPPER, S_TRAPPER, "trapper or lurker above")
    /* unicorn, horses */
    MONSYM(21, DEF_UNICORN, S_UNICORN, "unicorn or horse")
    MONSYM(22, DEF_VORTEX, S_VORTEX, "vortex")
    MONSYM(23, DEF_WORM, S_WORM, "worm")
    MONSYM(24, DEF_XAN, S_XAN, "xan or other mythical/fantastic insect")
    /* yellow light, black light */
    MONSYM(25, DEF_LIGHT, S_LIGHT, "light")
    MONSYM(26, DEF_ZRUTY, S_ZRUTY, "zruty")
    MONSYM(27, DEF_ANGEL, S_ANGEL, "angelic being")
    MONSYM(28, DEF_BAT, S_BAT, "bat or bird")
    MONSYM(29, DEF_CENTAUR, S_CENTAUR, "centaur")
    MONSYM(30, DEF_DRAGON, S_DRAGON, "dragon")
    /* elemental includes invisible stalker */
    MONSYM(31, DEF_ELEMENTAL, S_ELEMENTAL, "elemental")
    MONSYM(32, DEF_FUNGUS, S_FUNGUS, "fungus or mold")
    MONSYM(33, DEF_GNOME, S_GNOME, "gnome")
    /* large humanoid: giant, ettin, minotaur */
    MONSYM(34, DEF_GIANT, S_GIANT, "giant humanoid")
    MONSYM(35, DEF_INVISIBLE, S_invisible, "invisible monster")
    MONSYM(36, DEF_JABBERWOCK, S_JABBERWOCK, "jabberwock")
    MONSYM(37, DEF_KOP, S_KOP, "Keystone Kop")
    MONSYM(38, DEF_LICH, S_LICH, "lich")
    MONSYM(39, DEF_MUMMY, S_MUMMY, "mummy")
    MONSYM(40, DEF_NAGA, S_NAGA, "naga")
    MONSYM(41, DEF_OGRE, S_OGRE, "ogre")
    MONSYM(42, DEF_PUDDING, S_PUDDING, "pudding or ooze")
    MONSYM(43, DEF_QUANTMECH, S_QUANTMECH, "quantum mechanic")
    MONSYM(44, DEF_RUSTMONST, S_RUSTMONST, "rust monster or disenchanter")
    MONSYM(45, DEF_SNAKE, S_SNAKE, "snake")
    MONSYM(46, DEF_TROLL, S_TROLL, "troll")
    /* umber hulk */
    MONSYM(47, DEF_UMBER, S_UMBER, "umber hulk")
    MONSYM(48, DEF_VAMPIRE, S_VAMPIRE, "vampire")
    MONSYM(49, DEF_WRAITH, S_WRAITH, "wraith")
    MONSYM(50, DEF_XORN, S_XORN, "xorn")
    /* apelike creature includes owlbear, monkey */
    MONSYM(51, DEF_YETI, S_YETI, "apelike creature")
    MONSYM(52, DEF_ZOMBIE, S_ZOMBIE, "zombie")
    MONSYM(53, DEF_HUMAN, S_HUMAN, "human or elf")
    /* space symbol*/
    MONSYM(54, DEF_GHOST, S_GHOST, "ghost")
    MONSYM(55, DEF_GOLEM, S_GOLEM, "golem")
    MONSYM(56, DEF_DEMON, S_DEMON, "major demon")
    /* fish */
    MONSYM(57, DEF_EEL,  S_EEL, "sea monster")
    /* reptiles */
    MONSYM(58, DEF_LIZARD, S_LIZARD, "lizard")
    MONSYM(59, DEF_WORM_TAIL,  S_WORM_TAIL, "long worm tail")
    MONSYM(60, DEF_MIMIC_DEF,  S_MIMIC_DEF, "mimic")

#undef MONSYM
#endif /* MONSYMS_ENUM || MONSYMS_PARSE || MONSYMS_DRAWING */

#if defined(OBJCLASS_ENUM) || defined(OBJCLASS_PARSE) || defined (OBJCLASS_DRAWING)

/*
    OBJCLASS(idx, class, defsym, sym, name, explain)
        idx:     index used in enum
        class:   enum entry
        defsym:  symbol macro (defined in sym.h)
        sym:     symbol name for parsing purposes
        name:    used in object_detect()
        explain: used in do_look()
*/

#if defined(OBJCLASS_ENUM)
/* objclass.h */
#define OBJCLASS(idx, class, defsym, sym, name, explain) \
    class = idx,

#elif defined(OBJCLASS_PARSE)
/* symbols.c */
#define OBJCLASS(idx, class, defsym, sym, name, explain) \
    { SYM_OC, class + SYM_OFF_O, #sym },

#elif defined(OBJCLASS_DRAWING)
/* drawing.c */
#define OBJCLASS(idx, class, defsym, sym, name, explain) \
    { defsym, name, explain },
#endif

    OBJCLASS( 1, ILLOBJ_CLASS, ILLOBJ_SYM, S_strange_obj,
                "illegal objects", "strange object")
    OBJCLASS( 2, WEAPON_CLASS, WEAPON_SYM, S_weapon, "weapons", "weapon")
    OBJCLASS( 3, ARMOR_CLASS, ARMOR_SYM, S_armor,
                    "armor", "suit or piece of armor")
    OBJCLASS( 4, RING_CLASS, RING_SYM, S_ring, "rings", "ring")
    OBJCLASS( 5, AMULET_CLASS, AMULET_SYM, S_amulet, "amulets", "amulet")
    OBJCLASS( 6, TOOL_CLASS, TOOL_SYM, S_tool,
                    "tools", "useful item (pick-axe, key, lamp...)")
    OBJCLASS( 7, FOOD_CLASS, FOOD_SYM, S_food, "food", "piece of food")
    OBJCLASS( 8, POTION_CLASS, POTION_SYM, S_potion, "potions", "potion")
    OBJCLASS( 9, SCROLL_CLASS, SCROLL_SYM, S_scroll, "scrolls", "scroll")
    OBJCLASS(10, SPBOOK_CLASS, SPBOOK_SYM, S_book, "spellbooks", "spellbook")
    OBJCLASS(11, WAND_CLASS, WAND_SYM, S_wand, "wands", "wand")
    OBJCLASS(12, COIN_CLASS, GOLD_SYM, S_coin, "coins", "pile of coins")
    OBJCLASS(13, GEM_CLASS, GEM_SYM, S_gem, "rocks", "gem or rock")
    OBJCLASS(14, ROCK_CLASS, ROCK_SYM, S_rock,
                    "large stones", "boulder or statue")
    OBJCLASS(15, BALL_CLASS, BALL_SYM, S_ball, "iron balls", "iron ball")
    OBJCLASS(16, CHAIN_CLASS, CHAIN_SYM, S_chain, "chains", "iron chain")
    OBJCLASS(17, VENOM_CLASS, VENOM_SYM, S_venom, "venoms", "splash of venom")

#undef OBJCLASS
#endif /* OBJCLASS_ENUM || OBJCLASS_PARSE || OBJCLASS_DRAWING */

#undef CLR

/* end of defsym.h */

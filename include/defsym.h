/* NetHack 3.7	defsym.h */
/*      Copyright (c) 2016 by Pasi Kallinen              */
/* NetHack may be freely redistributed.  See license for details. */


/*
    This header is included in multiple places to produce
    different code depending on its use. Its purpose is to
    ensure that there is only one definitive source for
    pchar, objclass and mon symbols.

    The morphing macro expansions are used in these places:
  - in include/sym.h for enums of some S_* symbol values
    (define PCHAR_S_ENUM, MONSYMS_S_ENUM prior to #include defsym.h)
  - in include/sym.h for enums of some DEF_* symbol values
    (define MONSYMS_DEFCHAR_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of some default character values
    (define OBJCLASS_DEFCHAR_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of some *_CLASS values
    (define OBJCLASS_CLASS_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of S_* symbol values
    (define OBJCLASS_S_ENUM prior to #include defsym.h)
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

#if defined(PCHAR_S_ENUM) || defined(PCHAR_PARSE) \
    || defined(PCHAR_DRAWING) || defined(PCHAR_TILES)

/*
   PCHAR(idx, ch, sym, desc, clr)
       idx:     index used in enum
       ch:      character symbol
       sym:     symbol name for parsing purposes (also tile name)
       desc:    description
       clr:     color

   PCHAR2(idx, ch, sym, tilenm, desc, clr)
       idx:     index used in enum
       ch:      character symbol
       sym:     symbol name for parsing purposes
       tilenm:  if the name in the tile txt file differs from desc (below),
                the name in the tile txt file can be specified here.
       desc:    description
       clr:     color
*/

#if defined(PCHAR_S_ENUM)
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
    PCHAR2( 5, '-',  S_blcorn,
           "bottom left corner wall", "wall", CLR(CLR_GRAY))
    PCHAR2( 6, '-',  S_brcorn,
           "bottom right corner wall", "wall", CLR(CLR_GRAY))
    PCHAR2( 7, '-',  S_crwall, "cross wall", "wall", CLR(CLR_GRAY))
    PCHAR2( 8, '-',  S_tuwall, "tuwall", "wall", CLR(CLR_GRAY))
    PCHAR2( 9, '-',  S_tdwall, "tdwall", "wall", CLR(CLR_GRAY))
    PCHAR2(10, '|',  S_tlwall, "tlwall", "wall", CLR(CLR_GRAY))
    PCHAR2(11, '|',  S_trwall, "trwall", "wall", CLR(CLR_GRAY))
    /* start cmap A                                                      */
    PCHAR2(12, '.',  S_ndoor, "no door", "doorway", CLR(CLR_GRAY))
    PCHAR2(13, '-',  S_vodoor,
           "vertical open door", "open door", CLR(CLR_BROWN))
    PCHAR2(14, '|',  S_hodoor,
           "horizontal open door", "open door", CLR(CLR_BROWN))
    PCHAR2(15, '+',  S_vcdoor,
           "vertical closed door", "closed door", CLR(CLR_BROWN))
    PCHAR2(16, '+',  S_hcdoor,
           "horizontal closed door", "closed door", CLR(CLR_BROWN))
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
    /* end cmap A */
    PCHAR( 31, '_',  S_altar, "altar", CLR(CLR_GRAY))
    /* start cmap B */
    PCHAR( 32, '|',  S_grave, "grave", CLR(CLR_WHITE))
    PCHAR2(33, '\\', S_throne, "throne", "opulent throne", CLR(HI_GOLD))
    PCHAR( 34, '{',  S_sink, "sink", CLR(CLR_WHITE))
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
    /* end dungeon characters                                          */
    /*                                                                 */
    /* begin traps                                                     */
    /*                                                                 */
    PCHAR( 46, '^',  S_arrow_trap, "arrow trap", CLR(HI_METAL))
    PCHAR( 47, '^',  S_dart_trap, "dart trap", CLR(HI_METAL))
    PCHAR( 48, '^',  S_falling_rock_trap, "falling rock trap", CLR(CLR_GRAY))
    PCHAR( 49, '^',  S_squeaky_board, "squeaky board", CLR(CLR_BROWN))
    PCHAR( 50, '^',  S_bear_trap, "bear trap", CLR(HI_METAL))
    PCHAR( 51, '^',  S_land_mine, "land mine", CLR(CLR_RED))
    PCHAR( 52, '^',  S_rolling_boulder_trap, "rolling boulder trap",
                     CLR(CLR_GRAY))
    PCHAR( 53, '^',  S_sleeping_gas_trap, "sleeping gas trap", CLR(HI_ZAP))
    PCHAR( 54, '^',  S_rust_trap, "rust trap", CLR(CLR_BLUE))
    PCHAR( 55, '^',  S_fire_trap, "fire trap", CLR(CLR_ORANGE))
    PCHAR( 56, '^',  S_pit, "pit", CLR(CLR_BLACK))
    PCHAR( 57, '^',  S_spiked_pit, "spiked pit", CLR(CLR_BLACK))
    PCHAR( 58, '^',  S_hole, "hole", CLR(CLR_BROWN))
    PCHAR( 59, '^',  S_trap_door, "trap door", CLR(CLR_BROWN))
    PCHAR( 60, '^',  S_teleportation_trap, "teleportation trap",
                     CLR(CLR_MAGENTA))
    PCHAR( 61, '^',  S_level_teleporter, "level teleporter", CLR(CLR_MAGENTA))
    PCHAR( 62, '^',  S_magic_portal, "magic portal", CLR(CLR_BRIGHT_MAGENTA))
    PCHAR( 63, '"',  S_web, "web", CLR(CLR_GRAY))
    PCHAR( 64, '^',  S_statue_trap, "statue trap", CLR(CLR_GRAY))
    PCHAR( 65, '^',  S_magic_trap, "magic trap", CLR(HI_ZAP))
    PCHAR2(66, '^',  S_anti_magic_trap, "anti magic trap", "anti-magic field",
                     CLR(HI_ZAP))
    PCHAR( 67, '^',  S_polymorph_trap, "polymorph trap", CLR(CLR_BRIGHT_GREEN))
    PCHAR( 68, '~',  S_vibrating_square, "vibrating square", CLR(CLR_MAGENTA))
    PCHAR( 69, '^',  S_trapped_door, "trapped door", CLR(CLR_ORANGE))
    PCHAR( 70, '^',  S_trapped_chest, "trapped chest", CLR(CLR_ORANGE))
    /* end traps                                                       */
    /* end cmap B */
    /*                                                                   */
    /* begin special effects                                             */
    /*                                                                   */
    /* zap colors are changed by reset_glyphmap() to match type of beam */
    /*                                                                   */
    PCHAR2(71, '|',  S_vbeam, "vertical beam", "", CLR(CLR_GRAY))
    PCHAR2(72, '-',  S_hbeam, "horizontal beam", "", CLR(CLR_GRAY))
    PCHAR2(73, '\\', S_lslant, "left slant beam", "", CLR(CLR_GRAY))
    PCHAR2(74, '/',  S_rslant, "right slant beam", "", CLR(CLR_GRAY))
    /* start cmap C */
    PCHAR2(75, '*',  S_digbeam, "dig beam", "", CLR(CLR_WHITE))
    PCHAR2(76, '!',  S_flashbeam, "flash beam", "", CLR(CLR_WHITE))
    PCHAR2(77, ')',  S_boomleft, "boom left", "", CLR(HI_WOOD))
    PCHAR2(78, '(',  S_boomright, "boom right", "", CLR(HI_WOOD))
    /* 4 magic shield symbols                                          */
    PCHAR2(79, '0',  S_ss1, "shield1", "", CLR(HI_ZAP))
    PCHAR2(80, '#',  S_ss2, "shield2", "", CLR(HI_ZAP))
    PCHAR2(81, '@',  S_ss3, "shield3", "", CLR(HI_ZAP))
    PCHAR2(82, '*',  S_ss4, "shield4", "", CLR(HI_ZAP))
    PCHAR( 83, '#',  S_poisoncloud, "poison cloud", CLR(CLR_BRIGHT_GREEN))
    PCHAR( 84, '?',  S_goodpos, "valid position", CLR(CLR_BRIGHT_GREEN))
    /* end cmap C */
    /*                                                             */
    /* The 8 swallow symbols.  Do NOT separate.                    */
    /* To change order or add, see the function swallow_to_glyph() */
    /* in display.c. swallow colors are changed by                 */
    /* reset_glyphmap() to match the engulfing monst.              */
    /*                                                             */
    /*  Order:                                                     */
    /*                                                             */
    /*      1 2 3                                                  */
    /*      4 5 6                                                  */
    /*      7 8 9                                                  */
    /*                                                             */
    PCHAR2(85, '/',  S_sw_tl, "swallow top left", "", CLR(CLR_GREEN))    /*1*/
    PCHAR2(86, '-',  S_sw_tc, "swallow top center", "", CLR(CLR_GREEN))  /*2*/
    PCHAR2(87, '\\', S_sw_tr, "swallow top right", "", CLR(CLR_GREEN))   /*3*/
    PCHAR2(88, '|',  S_sw_ml, "swallow middle left", "", CLR(CLR_GREEN)) /*4*/
    PCHAR2(89, '|',  S_sw_mr, "swallow middle right", "", CLR(CLR_GREEN)) /*6*/
    PCHAR2(90, '\\', S_sw_bl, "swallow bottom left", "", CLR(CLR_GREEN))  /*7*/
    PCHAR2(91, '-',  S_sw_bc, "swallow bottom center", "", CLR(CLR_GREEN))/*8*/
    PCHAR2(92, '/',  S_sw_br, "swallow bottom right", "", CLR(CLR_GREEN)) /*9*/
    /*                                                             */
    /* explosion colors are changed by reset_glyphmap() to match   */
    /* the type of expl.                                           */
    /*                                                             */
    /*    Ex.                                                      */
    /*                                                             */
    /*      /-\                                                    */
    /*      |@|                                                    */
    /*      \-/                                                    */
    /*                                                             */
    PCHAR2(93, '/',  S_expl_tl, "explosion top left", "", CLR(CLR_ORANGE))
    PCHAR2(94, '-',  S_expl_tc, "explosion top center", "", CLR(CLR_ORANGE))
    PCHAR2(95, '\\', S_expl_tr, "explosion top right", "", CLR(CLR_ORANGE))
    PCHAR2(96, '|',  S_expl_ml, "explosion middle left", "", CLR(CLR_ORANGE))
    PCHAR2(97, ' ',  S_expl_mc, "explosion middle center", "", CLR(CLR_ORANGE))
    PCHAR2(98, '|',  S_expl_mr, "explosion middle right", "", CLR(CLR_ORANGE))
    PCHAR2(99, '\\', S_expl_bl, "explosion bottom left", "", CLR(CLR_ORANGE))
    PCHAR2(100, '-', S_expl_bc, "explosion bottom center", "", CLR(CLR_ORANGE))
    PCHAR2(101, '/', S_expl_br, "explosion bottom right", "", CLR(CLR_ORANGE))
#undef PCHAR
#undef PCHAR2
#endif /* PCHAR_S_ENUM || PCHAR_PARSE || PCHAR_DRAWING || PCHAR_TILES */

#if defined(MONSYMS_S_ENUM) || defined(MONSYMS_DEFCHAR_ENUM) \
        || defined(MONSYMS_PARSE) || defined(MONSYMS_DRAWING)

/*
    MONSYM(idx, ch, sym desc)
        idx:     index used in enum
        ch:      character symbol
        sym:     symbol name for parsing purposes
        desc:    description
*/

#if defined(MONSYMS_S_ENUM)
/* sym.h */
#define MONSYM(idx, ch, basename, sym, desc) \
    sym = idx,

#elif defined(MONSYMS_DEFCHAR_ENUM)
/* sym.h */
#define MONSYM(idx, ch, basename, sym,  desc) \
    DEF_##basename = ch,

#elif defined(MONSYMS_PARSE)
/* symbols.c */
#define MONSYM(idx, ch, basename, sym, desc) \
    { SYM_MON, sym + SYM_OFF_M, #sym },

#elif defined(MONSYMS_DRAWING)
/* drawing.c */
#define MONSYM(idx, ch, basename, sym, desc) \
    { DEF_##basename, "", desc },
#endif

    MONSYM( 1, 'a', ANT, S_ANT,   "ant or other insect")
    MONSYM( 2, 'b', BLOB, S_BLOB, "blob")
    MONSYM( 3, 'c', COCKATRICE, S_COCKATRICE, "cockatrice")
    MONSYM( 4, 'd', DOG, S_DOG, "dog or other canine")
    MONSYM( 5, 'e', EYE, S_EYE, "eye or sphere")
    MONSYM( 6, 'f', FELINE, S_FELINE, "cat or other feline")
    MONSYM( 7, 'g', GREMLIN, S_GREMLIN, "gremlin")
    /* small humanoids: hobbit, dwarf */
    MONSYM( 8, 'h', HUMANOID, S_HUMANOID, "humanoid")
    /* minor demons */
    MONSYM( 9, 'i', IMP, S_IMP, "imp or minor demon")
    MONSYM(10, 'j', JELLY, S_JELLY, "jelly")
    MONSYM(11, 'k', KOBOLD, S_KOBOLD, "kobold")
    MONSYM(12, 'l', LEPRECHAUN, S_LEPRECHAUN, "leprechaun")
    MONSYM(13, 'm', MIMIC, S_MIMIC, "mimic")
    MONSYM(14, 'n', NYMPH, S_NYMPH, "nymph")
    MONSYM(15, 'o', ORC, S_ORC, "orc")
    MONSYM(16, 'p', PIERCER, S_PIERCER, "piercer")
    /* quadruped excludes horses */
    MONSYM(17, 'q', QUADRUPED, S_QUADRUPED, "quadruped")
    MONSYM(18, 'r', RODENT, S_RODENT, "rodent")
    MONSYM(19, 's', SPIDER, S_SPIDER, "arachnid or centipede")
    MONSYM(20, 't', TRAPPER, S_TRAPPER, "trapper or lurker above")
    /* unicorn, horses */
    MONSYM(21, 'u', UNICORN, S_UNICORN, "unicorn or horse")
    MONSYM(22, 'v', VORTEX, S_VORTEX, "vortex")
    MONSYM(23, 'w', WORM, S_WORM, "worm")
    MONSYM(24, 'x', XAN, S_XAN, "xan or other mythical/fantastic insect")
    /* yellow light, black light */
    MONSYM(25, 'y', LIGHT, S_LIGHT, "light")
    MONSYM(26, 'z', ZRUTY, S_ZRUTY, "zruty")
    MONSYM(27, 'A', ANGEL, S_ANGEL, "angelic being")
    MONSYM(28, 'B', BAT, S_BAT, "bat or bird")
    MONSYM(29, 'C', CENTAUR, S_CENTAUR, "centaur")
    MONSYM(30, 'D', DRAGON, S_DRAGON, "dragon")
    /* elemental includes invisible stalker */
    MONSYM(31, 'E', ELEMENTAL, S_ELEMENTAL, "elemental")
    MONSYM(32, 'F', FUNGUS, S_FUNGUS, "fungus or mold")
    MONSYM(33, 'G', GNOME, S_GNOME, "gnome")
    /* large humanoid: giant, ettin, minotaur */
    MONSYM(34, 'H', GIANT, S_GIANT, "giant humanoid")
    MONSYM(35, 'I', INVISIBLE, S_invisible, "invisible monster")
    MONSYM(36, 'J', JABBERWOCK, S_JABBERWOCK, "jabberwock")
    MONSYM(37, 'K', KOP, S_KOP, "Keystone Kop")
    MONSYM(38, 'L', LICH, S_LICH, "lich")
    MONSYM(39, 'M', MUMMY, S_MUMMY, "mummy")
    MONSYM(40, 'N', NAGA, S_NAGA, "naga")
    MONSYM(41, 'O', OGRE, S_OGRE, "ogre")
    MONSYM(42, 'P', PUDDING, S_PUDDING, "pudding or ooze")
    MONSYM(43, 'Q', QUANTMECH, S_QUANTMECH, "quantum mechanic")
    MONSYM(44, 'R', RUSTMONST, S_RUSTMONST, "rust monster or disenchanter")
    MONSYM(45, 'S', SNAKE, S_SNAKE, "snake")
    MONSYM(46, 'T', TROLL, S_TROLL, "troll")
    /* umber hulk */
    MONSYM(47, 'U', UMBER, S_UMBER, "umber hulk")
    MONSYM(48, 'V', VAMPIRE, S_VAMPIRE, "vampire")
    MONSYM(49, 'W', WRAITH, S_WRAITH, "wraith")
    MONSYM(50, 'X', XORN, S_XORN, "xorn")
    /* apelike creature includes owlbear, monkey */
    MONSYM(51, 'Y', YETI, S_YETI, "apelike creature")
    MONSYM(52, 'Z', ZOMBIE, S_ZOMBIE, "zombie")
    MONSYM(53, '@', HUMAN, S_HUMAN, "human or elf")
    /* space symbol*/
    MONSYM(54, ' ', GHOST, S_GHOST, "ghost")
    MONSYM(55, '\'', GOLEM, S_GOLEM, "golem")
    MONSYM(56, '&', DEMON, S_DEMON, "major demon")
    /* fish */
    MONSYM(57, ';', EEL, S_EEL,  "sea monster")
    /* reptiles */
    MONSYM(58, ':', LIZARD, S_LIZARD, "lizard")
    MONSYM(59, '~', WORM_TAIL, S_WORM_TAIL, "long worm tail")
    MONSYM(60, ']', MIMIC_DEF, S_MIMIC_DEF, "mimic")

#undef MONSYM
#endif /* MONSYMS_S_ENUM || MONSYMS_DEFCHAR_ENUM || MONSYMS_PARSE */
       /* || MONSYMS_DRAWING */

#if defined(OBJCLASS_S_ENUM) || defined(OBJCLASS_DEFCHAR_ENUM) \
        || defined(OBJCLASS_CLASS_ENUM) || defined(OBJCLASS_PARSE) \
        || defined (OBJCLASS_DRAWING)

/*
    OBJCLASS(idx, ch, basename, sym, name, explain)
        idx:      index used in enum
        ch:       default character
        basename: unadorned base name of objclass, used
                  to construct enums through suffixes/prefixes
        sym:      symbol name for enum and parsing purposes
        name:     used in object_detect()
        explain:  used in do_look()

    OBJCLASS7(idx, ch, basename, sname, sym, name, explain)
        idx:      index used in enum
        ch:       default character
        basename: unadorned base name of objclass, used
                  to construct enums through suffixes/prefixes
        sname:    hardcoded *_SYM value for this entry (required
                  only because basename and GOLD_SYM differ
        sym:      symbol name for enum and parsing purposes
        name:     used in object_detect()
        explain:  used in do_look()
*/

#if defined(OBJCLASS_CLASS_ENUM)
/* objclass.h */
#define OBJCLASS(idx, ch, basename, sym, name, explain) \
    basename##_CLASS = idx,
#define OBJCLASS7(idx, ch, basename, sname, sym, name, explain) \
    basename##_CLASS = idx,

#elif defined(OBJCLASS_DEFCHAR_ENUM)
/* objclass.h */
#define OBJCLASS(idx, ch, basename, sym, name, explain) \
    basename##_SYM = ch,
#define OBJCLASS7(idx, ch, basename, sname, sym, name, explain) \
    sname = ch,

#elif defined(OBJCLASS_S_ENUM)
/* objclass.h */
#define OBJCLASS(idx, ch, basename, sym, name, explain) \
    sym = idx,
#define OBJCLASS7(idx, ch, basename, sname, sym, name, explain) \
    sym = idx,

#elif defined(OBJCLASS_PARSE)
/* symbols.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain) \
    { SYM_OC, sym + SYM_OFF_O, #sym },
#define OBJCLASS7(idx, ch, basename, sname, sym, name, explain) \
    { SYM_OC, sym + SYM_OFF_O, #sym },

#elif defined(OBJCLASS_DRAWING)
/* drawing.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain) \
    { basename##_SYM, name, explain },
#define OBJCLASS7(idx, ch, basename, sname, sym, name, explain) \
    { sname, name, explain },
#endif

    OBJCLASS( 1,  ']', ILLOBJ, S_strange_obj, "illegal objects",
                           "strange object")
    OBJCLASS( 2,  ')', WEAPON, S_weapon, "weapons", "weapon")
    OBJCLASS( 3,  '[', ARMOR, S_armor, "armor", "suit or piece of armor")
    OBJCLASS( 4,  '=', RING, S_ring, "rings", "ring")
    OBJCLASS( 5,  '"', AMULET, S_amulet, "amulets", "amulet")
    OBJCLASS( 6,  '(', TOOL, S_tool, "tools",
                           "useful item (pick-axe, key, lamp...)")
    OBJCLASS( 7,  '%', FOOD, S_food, "food", "piece of food")
    OBJCLASS( 8,  '!', POTION, S_potion, "potions", "potion")
    OBJCLASS( 9,  '?', SCROLL, S_scroll, "scrolls", "scroll")
    OBJCLASS(10,  '+', SPBOOK, S_book, "spellbooks", "spellbook")
    OBJCLASS(11,  '/', WAND, S_wand, "wands", "wand")
    OBJCLASS7(12, '$', COIN, GOLD_SYM, S_coin, "coins", "pile of coins")
    OBJCLASS(13,  '*', GEM, S_gem, "rocks", "gem or rock")
    OBJCLASS(14,  '`', ROCK, S_rock, "large stones", "boulder or statue")
    OBJCLASS(15,  '0', BALL, S_ball, "iron balls", "iron ball")
    OBJCLASS(16,  '_', CHAIN, S_chain, "chains", "iron chain")
    OBJCLASS(17,  '.', VENOM, S_venom, "venoms", "splash of venom")

#undef OBJCLASS
#undef OBJCLASS7
#endif /* OBJCLASS_S_ENUM || OBJCLASS_DEFCHAR_ENUM || OBJCLASS_CLASS_ENUM */
       /* || OBJCLASS_PARSE || OBJCLASS_DRAWING */

#undef CLR

#ifdef DEBUG
#if !defined(PCHAR_S_ENUM) && !defined(PCHAR_DRAWING) \
    && !defined(PCHAR_PARSE) && !defined(PCHAR_TILES) \
    && !defined(MONSYMS_S_ENUM) && !defined(MONSYMS_DEFCHAR_ENUM) \
    && !defined(MONSYMS_PARSE) && !defined(MONSYMS_DRAWING) \
    && !defined(OBJCLASS_S_ENUM) && !defined(OBJCLASS_DEFCHAR_ENUM) \
    && !defined(OBJCLASS_CLASS_ENUM) && !defined(OBJCLASS_PARSE) \
    && !defined (OBJCLASS_DRAWING)
#error Non-productive inclusion of defsym.h
#endif
#endif

/* end of defsym.h */

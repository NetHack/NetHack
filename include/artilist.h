/* NetHack 5.0  artilist.h      $NHDT-Date: 1710957374 2024/03/20 17:56:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.30 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#if defined(MAKEDEFS_C) || defined (MDLIB_C)
/* in makedefs.c, all we care about is the list of names */

#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, bn) nam

static const char *const artifact_names[] = {

#elif defined(ARTI_ENUM)
#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, bn)                                 \
    ART_##bn

#elif defined(DUMP_ARTI_ENUM)
#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, bn)                                 \
    { ART_##bn, "ART_" #bn }
#else
/* in artifact.c, set up the actual artifact list structure;
   color field is for an artifact when it glows, not for the item itself */

#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, bn)                                 \
    {                                                            \
        typ, nam, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac,   \
        gs, gv, cost, clr                                        \
    }

/* clang-format off */
#define     NO_ATTK     {0,0,0,0}               /* no attack */
#define     NO_DFNS     {0,0,0,0}               /* no defense */
#define     NO_CARY     {0,0,0,0}               /* no carry effects */
#define     DFNS(c)     {0,c,0,0}
#define     CARY(c)     {0,c,0,0}
#define     PHYS(a,b)   {0,AD_PHYS,a,b}         /* physical */
#define     DRLI(a,b)   {0,AD_DRLI,a,b}         /* life drain */
#define     COLD(a,b)   {0,AD_COLD,a,b}
#define     FIRE(a,b)   {0,AD_FIRE,a,b}
#define     ELEC(a,b)   {0,AD_ELEC,a,b}         /* electrical shock */
#define     STUN(a,b)   {0,AD_STUN,a,b}         /* magical attack */
#define     POIS(a,b)   {0,AD_DRST,a,b}         /* poison */
/* clang-format on */

static NEARDATA struct artifact artilist[] = {
#endif /* MAKEDEFS_C || MDLIB_C */

    /* Artifact cost rationale:
     * 1.  The more useful the artifact, the better its cost.
     * 2.  Quest artifacts are highly valued.
     * 3.  Chaotic artifacts are inflated due to scarcity (and balance).
     *
     * Artifact gen_spe rationale:
     * 1.  If the artifact is useful against most enemies, +0.
     * 2.  If the artifact is useful against only a few enemies, usually +2.
     *     This gives the artifact use to early-game characters who receive
     *     it as a gift or find it on the ground.
     * 3.  Role gift gen_spe is chosen to balance against the role's
     *     default starting weapon (it should be better, but need not be
     *     much better).
     * 4.  This can be modified as required for special cases.
     *     (In some cases, like Excalibur, the value is irrelevant.)
     * 5.  Nonweapon spe may have a special meaning, so gen_spe for
     *     nonweapons must always be 0.
     *
     * Artifact gift_value is chosen so that "endgame-quality" artifacts are
     * not gifted in the early game (so that characters don't grind on an
     * altar early-game until they have their endgame weapon, then use it to
     * carry them through the game).  Those artifacts have values ranging from
     * around 8 to 10, based on how good the artifact is.  Less powerful
     * artifacts have values in the 1 to 5 range.  Values in between are used
     * for conditionally good artifacts.  (Note that the value of a gift is
     * normally 1 higher than the difficulty of the monster.)
     */

    /*  dummy element #0, so that all interesting indices are non-zero */
    A("", STRANGE_OBJECT, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE,
      NON_PM, NON_PM,
      0, 0, 0L, NO_COLOR, NONARTIFACT),

    A("Excalibur", LONG_SWORD, (SPFX_NOGEN | SPFX_RESTR | SPFX_SEEK
                                | SPFX_DEFN | SPFX_INTEL | SPFX_SEARCH),
      0, 0, PHYS(5, 10), DRLI(0, 0), NO_CARY, 0, A_LAWFUL, PM_KNIGHT, NON_PM,
      0, 10, 4000L, NO_COLOR, EXCALIBUR),
    /*
     *      Stormbringer only has a 2 because it can drain a level,
     *      providing 8 more.
     */
    A("Stormbringer", RUNESWORD,
      (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN | SPFX_INTEL | SPFX_DRLI), 0, 0,
      DRLI(5, 2), DRLI(0, 0), NO_CARY, 0, A_CHAOTIC, NON_PM, NON_PM,
      0, 9, 8000L, NO_COLOR, STORMBRINGER),
    /*
     *      Mjollnir can be thrown when wielded if hero has 25 Strength
     *      (usually via gauntlets of power but possible with rings of
     *      gain strength).  If the thrower is a Valkyrie, Mjollnir will
     *      usually (99%) return and then usually (separate 99%) be caught
     *      and automatically be re-wielded.  When returning Mjollnir is
     *      not caught, there is a 50:50 chance of hitting hero for damage
     *      and its lightning shock might destroy some wands and/or rings.
     *
     *      Monsters don't throw Mjollnir regardless of strength (not even
     *      fake-player valkyries).
     */
    A("Mjollnir", WAR_HAMMER, /* Mjo:llnir */
      (SPFX_RESTR | SPFX_ATTK), 0, 0, ELEC(5, 24), NO_DFNS, NO_CARY, 0,
      A_NEUTRAL, PM_VALKYRIE, NON_PM,
      0, 8, 4000L, NO_COLOR, MJOLLNIR),

    A("Cleaver", BATTLE_AXE, SPFX_RESTR, 0, 0, PHYS(3, 6), NO_DFNS, NO_CARY,
      0, A_NEUTRAL, PM_BARBARIAN, NON_PM,
      0, 8, 1500L, NO_COLOR, CLEAVER),

    /*
     *      Grimtooth glows in warning when elves are present, but its
     *      damage bonus applies to all targets rather than just elves
     *      (handled as special case in spec_dbon()).
     */
    A("Grimtooth", ORCISH_DAGGER, (SPFX_RESTR | SPFX_WARN | SPFX_DFLAG2),
      0, M2_ELF, PHYS(2, 6), POIS(0,0),
      NO_CARY, FLING_POISON, A_CHAOTIC, NON_PM, PM_ORC,
      0, 5, 1200L, CLR_RED, GRIMTOOTH),
    /*
     *      Orcrist and Sting have same alignment as elves.
     *
     *      The combination of SPFX_WARN+SPFX_DFLAG2+M2_value will trigger
     *      EWarn_of_mon for all monsters that have the M2_value flag.
     *      Sting and Orcrist will warn of M2_ORC monsters.
     */
    A("Orcrist", ELVEN_BROADSWORD, (SPFX_WARN | SPFX_DFLAG2), 0, M2_ORC,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ELF,
      3, 4, 2000L, CLR_BRIGHT_BLUE, ORCRIST), /* actually light blue */

    A("Sting", ELVEN_DAGGER, (SPFX_WARN | SPFX_DFLAG2), 0, M2_ORC,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ELF,
      3, 1, 800L, CLR_BRIGHT_BLUE, STING),
    /*
     *      Magicbane is a bit different!  Its magic fanfare
     *      unbalances victims in addition to doing some damage.
     */
    A("Magicbane", ATHAME, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,
      STUN(3, 4), DFNS(AD_MAGM), NO_CARY, 0, A_NEUTRAL, PM_WIZARD, NON_PM,
      0, 7, 3500L, NO_COLOR, MAGICBANE),

    A("Frost Brand", LONG_SWORD, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,
      COLD(5, 0), COLD(0, 0), NO_CARY, SNOWSTORM, A_NONE, NON_PM, NON_PM,
      0, 9, 3000L, NO_COLOR, FROST_BRAND),

    A("Fire Brand", LONG_SWORD, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,
      FIRE(5, 0), FIRE(0, 0), NO_CARY, FIRESTORM, A_NONE, NON_PM, NON_PM,
      0, 5, 3000L, NO_COLOR, FIRE_BRAND),

    A("Dragonbane", BROADSWORD,
      (SPFX_RESTR | SPFX_DCLAS | SPFX_REFLECT), 0, S_DRAGON,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      2, 5, 500L, NO_COLOR, DRAGONBANE),

    A("Demonbane", SILVER_MACE, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_DEMON,
      PHYS(5, 0), NO_DFNS, NO_CARY, BANISH, A_LAWFUL, PM_CLERIC, NON_PM,
      1, 3, 2500L, NO_COLOR, DEMONBANE),

    A("Werebane", SILVER_SABER, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_WERE,
      PHYS(5, 0), DFNS(AD_WERE), NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      1, 4, 1500L, NO_COLOR, WEREBANE),

    A("Grayswandir", SILVER_SABER, (SPFX_RESTR | SPFX_HALRES), 0, 0,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_LAWFUL, NON_PM, NON_PM,
      0, 10, 8000L, NO_COLOR, GRAYSWANDIR),

    A("Giantslayer", LONG_SWORD, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_GIANT,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NEUTRAL, NON_PM, NON_PM,
      2, 4, 200L, NO_COLOR, GIANTSLAYER),

    A("Ogresmasher", WAR_HAMMER, (SPFX_RESTR | SPFX_DCLAS), 0, S_OGRE,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      2, 1, 200L, NO_COLOR, OGRESMASHER),

    A("Trollsbane", MORNING_STAR, (SPFX_RESTR | SPFX_DCLAS | SPFX_REGEN), 0,
      S_TROLL, PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      2, 1, 200L, NO_COLOR, TROLLSBANE),

    /*
     *      Two problems:  1) doesn't let trolls regenerate heads,
     *      2) doesn't give unusual message for 2-headed monsters (but
     *      allowing those at all causes more problems than worth the effort).
     */
    A("Vorpal Blade", LONG_SWORD, (SPFX_RESTR | SPFX_BEHEAD), 0, 0,
      PHYS(5, 1), NO_DFNS, NO_CARY, 0, A_NEUTRAL, NON_PM, NON_PM,
      1, 5, 4000L, NO_COLOR, VORPAL_BLADE),

    /*
     *      Ah, never shall I forget the cry,
     *              or the shriek that shrieked he,
     *      As I gnashed my teeth, and from my sheath
     *              I drew my Snickersnee!
     *                      --Ko-Ko, Lord high executioner of Titipu
     *                        (From Sir W.S. Gilbert's "The Mikado")
     */
    A("Snickersnee", KATANA, SPFX_RESTR, 0, 0, PHYS(0, 8), NO_DFNS, NO_CARY,
      0, A_LAWFUL, PM_SAMURAI, NON_PM,
      0, 8, 1200L, NO_COLOR, SNICKERSNEE),

    /* Sunsword emits light when wielded (handled in the core rather than
       via artifact fields), but that light has no particular color */
    A("Sunsword", LONG_SWORD, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_UNDEAD,
      PHYS(5, 0), DFNS(AD_BLND), NO_CARY, BLINDING_RAY, A_LAWFUL, NON_PM,
      NON_PM,
      0, 6, 1500L, NO_COLOR, SUNSWORD),

    /*
     *      The artifacts for the quest dungeon, all self-willed.
     *      gen_spe should be 0; gift_value irrelevant and set to 12.
     */

    A("The Orb of Detection", CRYSTAL_BALL,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), (SPFX_ESP | SPFX_HSPDAM), 0,
      NO_ATTK, NO_DFNS, CARY(AD_MAGM), INVIS, A_LAWFUL, PM_ARCHEOLOGIST,
      NON_PM,
      0, 12, 2500L, NO_COLOR, ORB_OF_DETECTION),

    A("The Heart of Ahriman", LUCKSTONE,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), SPFX_STLTH, 0,
      /* this stone does double damage if used as a projectile weapon */
      PHYS(5, 0), NO_DFNS, NO_CARY, LEVITATION, A_NEUTRAL, PM_BARBARIAN,
      NON_PM,
      0, 12, 2500L, NO_COLOR, HEART_OF_AHRIMAN),

    A("The Sceptre of Might", MACE,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DALIGN), 0, 0, PHYS(5, 0),
      DFNS(AD_MAGM), NO_CARY, CONFLICT, A_LAWFUL, PM_CAVE_DWELLER, NON_PM,
      0, 12, 2500L, NO_COLOR, SCEPTRE_OF_MIGHT),

#if 0   /* OBSOLETE -- from 3.1.0 to 3.2.x, this was quest artifact for the
         * Elf role; in 3.3.0 elf became a race available to several roles
         * and the Elf role along with its quest was eliminated; it's a bit
         * overpowered to be an ordinary artifact so leave it excluded */
    A("The Palantir of Westernesse", CRYSTAL_BALL,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL),
                                     (SPFX_ESP | SPFX_REGEN | SPFX_HSPDAM), 0,
      NO_ATTK, NO_DFNS, NO_CARY, TAMING, A_CHAOTIC, NON_PM, PM_ELF,
      0, 12, 8000L, NO_COLOR, PALANTIR_OF_WESTERNESSE),
#endif

    A("The Staff of Aesculapius", QUARTERSTAFF,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_ATTK | SPFX_INTEL | SPFX_DRLI
       | SPFX_REGEN),
      0, 0, DRLI(0, 0), DRLI(0, 0), NO_CARY, HEALING, A_NEUTRAL, PM_HEALER,
      NON_PM,
      0, 12, 5000L, NO_COLOR, STAFF_OF_AESCULAPIUS),

    A("The Magic Mirror of Merlin", MIRROR,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_SPEAK), SPFX_ESP, 0,
      NO_ATTK, NO_DFNS, CARY(AD_MAGM), 0, A_LAWFUL, PM_KNIGHT, NON_PM,
      0, 12, 1500L, NO_COLOR, MAGIC_MIRROR_OF_MERLIN),

    A("The Eyes of the Overworld", LENSES,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_XRAY), 0, 0, NO_ATTK,
      DFNS(AD_MAGM), NO_CARY, ENLIGHTENING, A_NEUTRAL, PM_MONK, NON_PM,
      0, 12, 2500L, NO_COLOR, EYES_OF_THE_OVERWORLD),
    
    /* 
    =================================
      APOSTOLIC WRITING KIT
    * Quest reward for Archeologist
    * Enables unlimited writing of approved scrolls and prophetic books.
    * Divine ink never runs dry (bypasses spe checks in write.c).
    * #invoke gives you a free sheet of paper (unlimited paper)
    =================================
    */
    A("Apostolic Writing Kit", MAGIC_MARKER,
      (SPFX_NOGEN|SPFX_RESTR), 0, 0,
      NO_ATTK, NO_DFNS, NO_CARY, QUILL_INVOKE, A_LAWFUL, PM_ARCHEOLOGIST, NON_PM,
      0, 5, 3000L, CLR_WHITE, APOSTOLIC_WRITING_KIT),

    /* 
    =================================
      RIDING BLANKET OF THE HEAVENLY HOST
    * Quest reward for Knight
    * 100HP Buff to Horse
    * Transforms a horse into a firey steed of the heavenly host
    * Enables jousting and pounding while mounted for the 
    * following weapon types: Long Sword, Mace, Hammer, Silver Saber
    * #invoke tames any horses (including unicorns) in a 5x5 area
    =================================
    */
    A("Riding Blanket of the Heavenly Host", SADDLE,
      (SPFX_NOGEN ), 0, 0,
      NO_ATTK, NO_DFNS, NO_CARY, TAMING, A_LAWFUL, PM_KNIGHT, NON_PM,
      0, 5, 3000L, CLR_WHITE, BLANKET_OF_THE_HEAVENLY_HOST),
    
    /* 
    =================================
      HANDKERCHIEF OF THE APOSTLE PAUL
    * Quest reward for Healer
    * Passive: 100% immunity to disease
    * #invoke heals to full, clears all ailments,
    * all enemies buffed back 5 squares, paralyzed for 5 rounds
    =================================
    */
    A("Handkerchief of the Apostle Paul", PIECE_OF_CLOTH,
      (SPFX_NOGEN ), 0, 0,
      NO_ATTK, NO_DFNS, CARY(AD_DISE), HANDKERCHIEF, A_LAWFUL, PM_KNIGHT, NON_PM,
      0, 5, 3000L, CLR_WHITE, HANDKERCHIEF_OF_THE_APOSTLE_PAUL),

    /* 
    =================================
      GOLDEN CENSER OF INCENSE
    * Quest reward for Priest
    * Passive: 1/3 Prayer Timeout
    * Passive: Permanent Detect Monsters
    * Apply: Permanent 11x11 Light Source Centered on Player
    * #invoke 5x5 Grid Around Player PSALM 91 engraved
    * All enemies in 5x5 grid teleported to random places in level
    =================================
    */
    A("Golden Censer of Incense", THURIBLE,
      (SPFX_NOGEN ), 0, 0,
      NO_ATTK, NO_DFNS, NO_CARY, CENSERAID, A_LAWFUL, PM_CLERIC, NON_PM,
      0, 5, 3000L, CLR_WHITE, GOLDEN_CENSER_OF_INCENSE),
    /* 
    =================================
      SCARLET CORD OF RAHAB
    * Quest reward for Adventurer
    * apply: Ability to scale walls, trees, secret doors
    * #invoke: toggle invisibility
    =================================
    */
    A("Scarlet Cord of Rahab", ROPE,
      (SPFX_NOGEN ), 0, 0,
      NO_ATTK, NO_DFNS, NO_CARY, INVIS, A_LAWFUL, PM_ROGUE, NON_PM,
      0, 5, 3000L, CLR_MAGENTA, SCARLET_CORD_OF_RAHAB),
    
    /* 
    =================================
      STAFF OF SIGNS AND WONDERS
    * Quest reward for Prophet
    * apply: converts a non-water tile to water, converts water to dry land.
    * staff begins with 8 charges, prayer will get charges back up to 6
    * #invoke: AoE Column of fire 5x5 grid centered around player 5d5 Fire Damage, 5d5 Lightening Damage
    * prayer time reduced by 1/3
    =================================
    */
    A("Staff of Signs and Wonders", ALMOND_STAVE,
      (SPFX_NOGEN ), 0, 0,
      NO_ATTK, NO_DFNS, NO_CARY, COLUMN_OF_FIRE, A_LAWFUL, PM_WIZARD, NON_PM,
      0, 5, 3000L, CLR_MAGENTA, STAFF_OF_SIGNS_AND_WONDERS),

    A("The Mitre of Holiness", HELM_OF_BRILLIANCE,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_DFLAG2 | SPFX_INTEL | SPFX_PROTECT), 0,
      M2_UNDEAD, NO_ATTK, NO_DFNS, CARY(AD_FIRE), ENERGY_BOOST, A_LAWFUL,
      PM_CLERIC, NON_PM,
      0, 12, 2000L, NO_COLOR, MITRE_OF_HOLINESS),

    A("The Longbow of Diana", BOW,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_REFLECT), SPFX_ESP, 0,
      PHYS(5, 0), NO_DFNS, NO_CARY, CREATE_AMMO, A_CHAOTIC, PM_RANGER, NON_PM,
      0, 12, 4000L, NO_COLOR, LONGBOW_OF_DIANA),

    /* MKoT has an additional carry property if the Key is not cursed (for
       rogues) or blessed (for non-rogues):  #untrap of doors and chests
       will always find any traps and disarming those will always succeed */
    A("The Master Key of Thievery", SKELETON_KEY,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_SPEAK),
      (SPFX_WARN | SPFX_TCTRL | SPFX_HPHDAM), 0, NO_ATTK, NO_DFNS, NO_CARY,
      UNTRAP, A_CHAOTIC, PM_ROGUE, NON_PM,
      0, 12, 3500L, NO_COLOR, MASTER_KEY_OF_THIEVERY),

    A("The Tsurugi of Muramasa", TSURUGI,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_BEHEAD | SPFX_LUCK
       | SPFX_PROTECT),
      0, 0, PHYS(0, 8), NO_DFNS, NO_CARY, 0, A_LAWFUL, PM_SAMURAI, NON_PM,
      0, 12, 4500L, NO_COLOR, TSURUGI_OF_MURAMASA),

    A("The Platinum Yendorian Express Card", CREDIT_CARD,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DEFN),
      (SPFX_ESP | SPFX_HSPDAM), 0, NO_ATTK, NO_DFNS, CARY(AD_MAGM),
      CHARGE_OBJ, A_NEUTRAL, PM_TOURIST, NON_PM,
      0, 12, 7000L, NO_COLOR, YENDORIAN_EXPRESS_CARD),

    A("The Orb of Fate", CRYSTAL_BALL,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_LUCK),
      (SPFX_WARN | SPFX_HSPDAM | SPFX_HPHDAM), 0, NO_ATTK, NO_DFNS, NO_CARY,
      LEV_TELE, A_NEUTRAL, PM_VALKYRIE, NON_PM,
      0, 12, 3500L, NO_COLOR, ORB_OF_FATE),

    A("The Eye of the Aethiopica", AMULET_OF_ESP,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), (SPFX_EREGEN | SPFX_HSPDAM), 0,
      NO_ATTK, DFNS(AD_MAGM), NO_CARY, CREATE_PORTAL, A_NEUTRAL, PM_WIZARD,
      NON_PM,
      0, 12, 4000L, NO_COLOR, EYE_OF_THE_AETHIOPICA),
    
     /*
    =================================
      GOSPEL OF PEACE
      Water Walking
      Haste
      Fire Resistance (Carried or Worn)
      Apply to tame
      +modifier to charisma
    =================================
    */
    A("Gospel of Peace", SANDALS,
      (SPFX_NOGEN), 0,
      0, NO_ATTK, DFNS(AD_FIRE), CARY(AD_FIRE), TAMING, A_LAWFUL,
      NON_PM, NON_PM,
      0, 10, 2000L, NO_COLOR, SHOES_GOSPEL_OF_PEACE),

     /*
    =================================
      BREASTPLATE OF RIGHTEOUSNESS
      BASE AC 0
      Immunity to Drain Life
      Magic Resistance
      1/2 Magic Damage
      Posion/Drain Strength Resistance
      MC3
      Apply to use energy
    =================================
    */
    A("Breastplate of Righteousness", PLATE_OF_RIGHT,
      (SPFX_NOGEN | SPFX_HSPDAM ), 0,
      0, NO_ATTK, DRLI(0, 0), CARY(AD_DRST), ENERGY_BOOST, A_LAWFUL,
      NON_PM, NON_PM,
      0, 10, 2000L, NO_COLOR, PLATE_OF_RIGHTEOUSNESS),
    
      /*
    =================================
      BELT OF TRUTH
      See Invisibility
      Better Searching
      Protection against Sleep
      Protection Against Hallucination
      Protection against Blindness
      +modifier to intelligence
      Enlightenment when used
    =================================
    */
    A("Belt of Truth", BELT,
      (SPFX_NOGEN | SPFX_SEARCH | SPFX_SEEK | SPFX_SLEEP_RES | SPFX_BLIND_RES | SPFX_HALRES ), 0,
      0, NO_ATTK, NO_DFNS, NO_CARY, ENLIGHTENING, A_LAWFUL,
      NON_PM, NON_PM,
      0, 10, 2000L, NO_COLOR, BELT_OF_TRUTH),
    
      /*
    =================================
      SWORD OF THE SPIRIT
      Reflection
      Stun Attack 4d5 (needs work)
      Increased base damage
      Better Searching
      Warning/Aware for boss monsters
    =================================
    */
    A("Sword of the Spirit", TWO_EDGED_SWORD,
      (SPFX_NOGEN | SPFX_REFLECT | SPFX_WARN ), 0,
      M2_NASTY, STUN(2, 4), NO_DFNS, NO_CARY, 0, A_LAWFUL,
      NON_PM, NON_PM,
      0, 10, 2000L, NO_COLOR, SWORD_OF_THE_SPIRIT),
    
        /*
    =================================
      HELMET OF SALVATION
      Underwater breathing
      1/2 physical damage
      protection against disintigration
      health regeneration
      +modifier to dexterity
    =================================
    */
    A("Cap of Salvation", DIVINE_COVERING,
      (SPFX_NOGEN | SPFX_HPHDAM | SPFX_REGEN), 0,
      0, NO_ATTK, NO_DFNS, CARY(AD_DISN), HEALING, A_LAWFUL,
      NON_PM, NON_PM,
      0, 10, 2000L, NO_COLOR, HELMET_OF_SALVATION),
    
    /*
    =================================
      SHIELD OF FAITH
      Free Action
      Protection against stoning
      Fire resistance
      Magic Missile resistance (needs work)
      +modifier to strength attribute
      Can heal player when used
    =================================
    */

    A("Shield of Faith", ANCIENT_SHIELD,
      (SPFX_NOGEN | SPFX_STONING ), 0,
      0, NO_ATTK, DFNS(AD_FIRE), CARY(AD_MAGM), HEALING, A_LAWFUL,
      NON_PM, NON_PM,
      0, 10, 2000L, NO_COLOR, SHIELD_OF_FAITH),

#if !defined(ARTI_ENUM) && !defined(DUMP_ARTI_ENUM)
    /*
     *  terminator; otyp must be zero
     */
    A(0, 0, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      0, 0, 0L, NO_COLOR, TERMINATOR)

}; /* artilist[] (or artifact_names[]) */
#endif

#undef A

#ifdef NO_ATTK
#undef NO_ATTK
#undef NO_DFNS
#undef DFNS
#undef PHYS
#undef DRLI
#undef COLD
#undef FIRE
#undef ELEC
#undef STUN
#endif

/*artilist.h*/

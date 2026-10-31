/* NetHack 5.0	role.c	$NHDT-Date: 1737607158 2025/01/22 20:39:18 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.107 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*** Table of all roles ***/
/* According to AD&D, HD for some classes (ex. Wizard) should be smaller
 * (4-sided for wizards).  But this is not AD&D, and using the AD&D
 * rule here produces an unplayable character.  Thus I have used a minimum
 * of an 10-sided hit die for everything.  Another AD&D change: wizards get
 * a minimum strength of 4 since without one you can't teleport or cast
 * spells. --KAA
 *
 * As the wizard has been updated (wizard patch 5 jun '96) their HD can be
 * brought closer into line with AD&D. This forces wizards to use magic more
 * and distance themselves from their attackers. --LSZ
 *
 * With the introduction of races, some hit points and energy
 * has been reallocated for each race.  The values assigned
 * to the roles has been reduced by the amount allocated to
 * humans.  --KMH
 *
 * God names use a leading underscore to flag goddesses.
 */

/* NUM_ROLES is defined in hack.h */

const struct Role roles[NUM_ROLES+1] = {
    { { "Royal Historian", 0 },
      { { "Scribe", 0 },                  /* copying the Word */
        { "Chronicler", 0 },              /* recording God's deeds */
        { "Keeper of Scrolls", 0 },       /* guarding sacred texts */
        { "Seeker of the Truth", 0 },    /* searching for Scripture/relics */
        { "Guardian of Artifacts", 0 },      /* protecting holy artifacts */
        { "Watcher on the Walls", 0 },    /* vigilant against false history */
        { "Interpreter of Prophetic Texts", 0 },    /* discerning prophecy and typology */
        { "Steward of Sacred History", 0 }, /* faithful preserver */
        { "Historian of the Kingdom", 0 } }  /* fully recognized servant of the true story */,
      "the Lord", "God", "the Almighty", /* Christian*/
      "His",
      "the College of Archeology",
      "the Tomb of the Toltec Kings",
      PM_ARCHEOLOGIST,
      NON_PM,
      PM_ZENODOTUS,
      PM_STUDENT,
      PM_MINION_OF_HUHETOTL,
      NON_PM,
      PM_HUMAN_MUMMY,
      S_SNAKE,
      S_MUMMY,
      ART_APOSTOLIC_WRITING_KIT,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 7, 10, 10, 7, 7, 7 },
      { 20, 20, 20, 10, 20, 10 },
      /* Init   Lower  Higher */
      { 11, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      14, /* Energy */
      10,
      5,
      0,
      2,
      10,
      A_INT,
      SPE_MAGIC_MAPPING,
      -4 },
    { { "Knight", 0 },
      { { "Gallant", 0 },
        { "Esquire", 0 },
        { "Bachelor", 0 },
        { "Sergeant", 0 },
        { "Knight", 0 },
        { "Banneret", 0 },
        { "Chevalier", "Chevaliere" },
        { "Seignieur", "Dame" },
        { "Paladin", 0 } },
      "the Lord", "God", "the Almighty", /* Christian*/
      "Kni",
      "Camelot Castle",
      "the Isle of Glass",
      PM_KNIGHT,
      PM_PONY,
      PM_KING_ARTHUR,
      PM_PAGE,
      PM_IXOTH,
      PM_QUASIT,
      PM_OCHRE_JELLY,
      S_IMP,
      S_JELLY,
      ART_BLANKET_OF_THE_HEAVENLY_HOST,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 13, 7, 14, 8, 10, 17 },
      { 30, 15, 15, 10, 20, 10 },
      /* Init   Lower  Higher */
      { 14, 0, 0, 8, 2, 0 }, /* Hit points */
      { 1, 4, 0, 1, 0, 2 },
      10, /* Energy */
      10,
      8,
      -2,
      0,
      9,
      A_WIS,
      SPE_TURN_UNDEAD,
      -4 },
    { { "Healer", 0 },
       { { "Compassionate One", 0 },         /* moved by mercy (Matt 9:36) */
        { "Servant of the Hurting", 0 },    /* washes feet, tends wounds */
        { "Bearer of Kindness", 0 },    /* like the Good Samaritan (Luke 10:34) */
        { "Merciful", 0 },        /* first acts of healing in Jesus’ name */
        { "Physician’s Apprentice", 0 },    /* learning from the Great Physician */
        { "Deliverer", 0 },                 /* casting out demons with authority */
        { "Physician", 0 },         /* proclaiming liberty to captives (Isa 61:1) */
        { "Faithful Physician", 0 },        /* mature in gifts of healing (1 Cor 12:9) */
        { "Healer in Christ", 0 } },         /* fully conformed to the image of the One who heals */
      "the Lord", "God", "the Almighty", /* Christian*/
      "Hea",
      "the Temple of Epidaurus",
      "the Temple of Coeus",
      PM_HEALER,
      NON_PM,
      PM_HIPPOCRATES,
      PM_ATTENDANT,
      PM_CYCLOPS,
      PM_GIANT_RAT,
      PM_SNAKE,
      S_RODENT,
      S_YETI,
      ART_HANDKERCHIEF_OF_THE_APOSTLE_PAUL,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 7, 7, 13, 7, 11, 16 },
      { 15, 20, 20, 15, 25, 5 },
      /* Init   Lower  Higher */
      { 11, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 4, 0, 1, 0, 2 },
      20, /* Energy */
      10,
      3,
      -3,
      2,
      10,
      A_WIS,
      SPE_CURE_SICKNESS,
      -4 },
    { { "Priest", 0 },
      { { "Seminarian", 0 },                 /* XL 1–2  – preparing for Holy Orders */
        { "Transitional Deacon", 0 },        /* XL 3–5  – ordained to the diaconate */
        { "Priest", 0 },                     /* XL 6–9  – first Mass, ordinary title after ordination */
        { "Pastor", 0 },                     /* XL 10–13 – parish priest in charge of souls */
        { "Monsignor", 0 },                  /* XL 14–17 – honorary title (Chaplain/Prelate/Protonotary) */
        { "Canon", 0 },                      /* XL 18–21 – member of a cathedral chapter */
        { "Bishop", 0 },                     /* XL 22–25 – successor of the Apostles */
        { "Archbishop", 0 },                 /* XL 26–29 – metropolitan or major see */
        { "Cardinal", 0 } },                 /* XL 30   – Prince of the Church, “Priest forever” */
      0, 0, 0, /* */
      "Pri",
      "the Great Temple",
      "the Temple of Nalzok",
      PM_CLERIC,
      NON_PM,
      PM_ARCH_PRIEST,
      PM_ACOLYTE,
      PM_NALZOK,
      PM_HUMAN_ZOMBIE,
      PM_WRAITH,
      S_ZOMBIE,
      S_WRAITH,
      ART_GOLDEN_CENSER_OF_INCENSE,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 7, 7, 10, 7, 7, 7 },
      { 15, 10, 30, 15, 20, 10 },
      /* Init   Lower  Higher */
      { 12, 0, 0, 8, 1, 0 }, /* Hit points */
      { 4, 3, 0, 2, 0, 2 },
      10, /* Energy */
      0,
      3,
      -2,
      2,
      10,
      A_WIS,
      SPE_REMOVE_CURSE,
      -4 },
    /* Note:  Rogue precedes Ranger so that use of `-R' on the command line
       retains its traditional meaning. */
    { { "Adventurer", 0 },
      { { "Prodigal", 0 },                  /* once lost, now returning (Luke 15:11–32) */
        { "Wanderer", 0 },                  /* seeking the narrow way */
        { "Pilgrim", 0 },                   /* on the road to the Celestial City */
        { "Wayfarer", 0 },                  /* pressing on through peril */
        { "Sojourner", 0 },                 /* stranger in this world (1 Pet 2:11) */
        { "Seeker of Truth", 0 },           /* knocking, asking, searching */
        { "Watchman", 0 },                  /* alert against the thief in the night */
        { "Runner", 0 },                    /* running the race set before them (Heb 12:1) */
        { "Faithful Adventurer", 0 } },      /* redeemed and victorious in Christ */
      "the Lord", "God", "the Almighty", /* Christian*/
      "Adv",
      "the Thieves' Guild Hall",
      "the Assassins' Guild Hall",
      PM_ROGUE,
      NON_PM,
      PM_MASTER_OF_THIEVES,
      PM_THUG,
      PM_MASTER_ASSASSIN,
      PM_LEPRECHAUN,
      PM_GUARDIAN_NAGA,
      S_NYMPH,
      S_NAGA,
      ART_SCARLET_CORD_OF_RAHAB,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 7, 7, 7, 10, 7, 6 },
      { 20, 10, 10, 30, 20, 10 },
      /* Init   Lower  Higher */
      { 10, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      11, /* Energy */
      10,
      8,
      0,
      1,
      9,
      A_INT,
      SPE_DETECT_TREASURE,
      -4 },
    { { "Champion", 0 },
      { { "Servant", 0 },                  /* humble beginning, willing to fight */
        { "Shield-Bearer", 0 },            /* carries the shield of faith */
        { "Sword-Bearer", 0 },             /* entrusted with the sword of the Spirit */
        { "Defender", 0 },                 /* stands in the gap for the oppressed */
        { "Protector of the Weak", 0 },      /* fully committed to God's battles */
        { "Captain of the Host", 0 },      /* leads in righteousness */
        { "Mighty in Valor", 0 },          /* like David, Gideon, or Joshua */
        { "Champion of the Faith", 0 },    /* the title earned at high experience */
        { "Anointed", 0 } },
      "the Lord", "God", "the Almighty", /* Christian*/
      "Chp",
      "the Shrine of Destiny",
      "the cave of Surtur",
      PM_VALKYRIE,
      NON_PM /*PM_WINTER_WOLF_CUB*/,
      PM_NORN,
      PM_WARRIOR,
      PM_LORD_SURTUR,
      PM_FIRE_ANT,
      PM_FIRE_GIANT,
      S_ANT,
      S_GIANT,
      ART_ORB_OF_FATE,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 10, 7, 7, 7, 10, 7 },
      { 30, 6, 7, 20, 30, 7 },
      /* Init   Lower  Higher */
      { 14, 0, 0, 8, 2, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      10, /* Energy */
      0,
      10,
      -2,
      0,
      9,
      A_WIS,
      SPE_CONE_OF_COLD,
      -4 },
    { { "Prophet", 0 },
      { { "Novice", 0 },                  /* XL 1–2  */
        { "Initiate", 0 },                 /* XL 3–5  */
        { "Meditant", 0 },                   /* XL 6–9  */
        { "Disciple", 0 },        /* XL 10–13 */
        { "Herald", 0 },                  /* XL 14–17 */
        { "Saint", 0 },                 /* XL 18–21 */
        { "Anointed", 0 },                   /* XL 22–25 */
        { "Faithful", 0 },              /* XL 26–29 */
        { "Disciple of the Word", 0 } },/* XL 30 – the ultimate title */
      "the Lord", "God", "the Almighty", /* Christian*/
      "Pht",
      "the Lonely Tower",
      "the Tower of Darkness",
      PM_WIZARD,
      PM_KITTEN,
      PM_NEFERET_THE_GREEN,
      PM_APPRENTICE,
      PM_DARK_ONE,
      PM_VAMPIRE_BAT,
      PM_XORN,
      S_BAT,
      S_WRAITH,
      ART_STAFF_OF_SIGNS_AND_WONDERS,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 7, 10, 7, 7, 7, 7 },
      { 10, 30, 10, 20, 20, 10 },
      /* Init   Lower  Higher */
      { 10, 0, 0, 8, 1, 0 }, /* Hit points */
      { 4, 3, 0, 2, 0, 3 },
      12, /* Energy */
      0,
      1,
      0,
      3,
      10,
      A_INT,
      SPE_MAGIC_MISSILE,
      -4 },
    /* Array terminator */
    UNDEFINED_ROLE,
};

/* Table of all races */

/* NUM_RACES is defined in hack.h */
const struct Race races[NUM_RACES + 1] = {
    {
        "human",
        "human",
        "humanity",
        "Hum",
        { "man", "woman" },
        PM_HUMAN,
        PM_HUMAN_MUMMY,
        PM_HUMAN_ZOMBIE,
        MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL | ROLE_NEUTRAL
            | ROLE_CHAOTIC,
        MH_HUMAN,
        0,
        MH_GNOME | MH_ORC,
        /*    Str     Int Wis Dex Con Cha */
        { 3, 3, 3, 3, 3, 3 },
        { STR18(100), 18, 18, 18, 18, 18 },
        /* Init   Lower  Higher */
        { 2, 0, 0, 2, 1, 0 }, /* Hit points */
        { 1, 0, 2, 0, 2, 0 }  /* Energy */
    },
    /* Array terminator */
    UNDEFINED_RACE,
};

/* Table of all genders */
const struct Gender genders[] = {
    { "male", "he", "him", "his", "Mal", ROLE_MALE },
    { "female", "she", "her", "her", "Fem", ROLE_FEMALE },
    { "neuter", "it", "it", "its", "Ntr", ROLE_NEUTER },
    /* used by pronoun_gender() when hallucinating */
    { "group", "they", "them", "their", "Grp", 0 },
};

/* Table of all alignments */
const struct Align aligns[] = {
    { "law", "lawful", "Law", ROLE_LAWFUL, A_LAWFUL },
    { "balance", "neutral", "Neu", ROLE_NEUTRAL, A_NEUTRAL },
    { "chaos", "chaotic", "Cha", ROLE_CHAOTIC, A_CHAOTIC },
    { "evil", "unaligned", "Una", 0, A_NONE }
};

staticfn int randrole_filtered(void);
staticfn char *promptsep(char *, int);
staticfn int role_gendercount(int);
staticfn int race_alignmentcount(int);

/* used by str2XXX() */
static char NEARDATA randomstr[] = "random";

boolean
validrole(int rolenum)
{
    return (boolean) (IndexOkT(rolenum, roles));
}

int
randrole(boolean for_display)
{
    int res = SIZE(roles) - 1;

    if (for_display)
        res = rn2_on_display_rng(res);
    else
        res = rn2(res);
    return res;
}

staticfn int
randrole_filtered(void)
{
    int i, n = 0, set[SIZE(roles)];

    /* this doesn't rule out impossible combinations but attempts to
       honor all the filter masks */
    for (i = 0; i < SIZE(roles) - 1; ++i) /* -1: avoid terminating element */
        if (ok_role(i, ROLE_NONE, ROLE_NONE, ROLE_NONE)
            && ok_race(i, ROLE_RANDOM, ROLE_NONE, ROLE_NONE)
            && ok_gend(i, ROLE_NONE, ROLE_RANDOM, ROLE_NONE)
            && ok_align(i, ROLE_NONE, ROLE_NONE, ROLE_RANDOM))
            set[n++] = i;
    return n ? set[rn2(n)] : randrole(FALSE);
}

int
str2role(const char *str)
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = Strlen(str);
    for (i = 0; roles[i].name.m; i++) {
        /* Does it match the male name? */
        if (!strncmpi(str, roles[i].name.m, len))
            return i;
        /* Or the female name? */
        if (roles[i].name.f && !strncmpi(str, roles[i].name.f, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, roles[i].filecode))
            return i;
    }

    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

boolean
validrace(int rolenum, int racenum)
{
    /* Assumes validrole */
    return (boolean) (IndexOkT(racenum, races)
                      && (roles[rolenum].allow & races[racenum].allow
                          & ROLE_RACEMASK));
}

int
randrace(int rolenum)
{
    int i, n = 0;

    /* Count the number of valid races */
    for (i = 0; races[i].noun; i++)
        if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK)
            n++;

    /* Pick a random race */
    /* Use a factor of 100 in case of bad random number generators */
    if (n)
        n = rn2(n * 100) / 100;
    for (i = 0; races[i].noun; i++)
        if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK) {
            if (n)
                n--;
            else
                return i;
        }

    /* This role has no permitted races? */
    return rn2(SIZE(races) - 1);
}

int
str2race(const char *str)
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = Strlen(str);
    for (i = 0; races[i].noun; i++) {
        /* Does it match the noun? */
        if (!strncmpi(str, races[i].noun, len))
            return i;
        /* check adjective too */
        if (races[i].adj && !strncmpi(str, races[i].adj, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, races[i].filecode))
            return i;
    }

    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

boolean
validgend(int rolenum, int racenum, int gendnum)
{
    /* Assumes validrole and validrace */
    return (boolean) (gendnum >= 0 && gendnum < ROLE_GENDERS
                      && (roles[rolenum].allow & races[racenum].allow
                          & genders[gendnum].allow & ROLE_GENDMASK));
}

int
randgend(int rolenum, int racenum)
{
    int i, n = 0;

    /* Count the number of valid genders */
    for (i = 0; i < ROLE_GENDERS; i++)
        if (roles[rolenum].allow & races[racenum].allow & genders[i].allow
            & ROLE_GENDMASK)
            n++;

    /* Pick a random gender */
    if (n)
        n = rn2(n);
    for (i = 0; i < ROLE_GENDERS; i++)
        if (roles[rolenum].allow & races[racenum].allow & genders[i].allow
            & ROLE_GENDMASK) {
            if (n)
                n--;
            else
                return i;
        }

    /* This role/race has no permitted genders? */
    return rn2(ROLE_GENDERS);
}

int
str2gend(const char *str)
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = Strlen(str);
    for (i = 0; i < ROLE_GENDERS; i++) {
        /* Does it match the adjective? */
        if (!strncmpi(str, genders[i].adj, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, genders[i].filecode))
            return i;
    }
    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

boolean
validalign(int rolenum, int racenum, int alignnum)
{
    /* Assumes validrole and validrace */
    return (boolean) (alignnum >= 0 && alignnum < ROLE_ALIGNS
                      && (roles[rolenum].allow & races[racenum].allow
                          & aligns[alignnum].allow & ROLE_ALIGNMASK));
}

int
randalign(int rolenum, int racenum)
{
    int i, n = 0;

    /* Count the number of valid alignments */
    for (i = 0; i < ROLE_ALIGNS; i++)
        if (roles[rolenum].allow & races[racenum].allow & aligns[i].allow
            & ROLE_ALIGNMASK)
            n++;

    /* Pick a random alignment */
    if (n)
        n = rn2(n);
    for (i = 0; i < ROLE_ALIGNS; i++)
        if (roles[rolenum].allow & races[racenum].allow & aligns[i].allow
            & ROLE_ALIGNMASK) {
            if (n)
                n--;
            else
                return i;
        }

    /* This role/race has no permitted alignments? */
    return rn2(ROLE_ALIGNS);
}

int
str2align(const char *str)
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = Strlen(str);
    for (i = 0; i < ROLE_ALIGNS; i++) {
        /* Does it match the adjective? */
        if (!strncmpi(str, aligns[i].adj, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, aligns[i].filecode))
            return i;
    }
    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

/* is rolenum compatible with any racenum/gendnum/alignnum constraints? */
boolean
ok_role(int rolenum, int racenum, int gendnum, int alignnum)
{
    int i;
    short allow;

    if (IndexOkT(rolenum, roles)) {
        if (gr.rfilter.roles[rolenum])
            return FALSE;
        allow = roles[rolenum].allow;
        if (IndexOkT(racenum, races)
            && !(allow & races[racenum].allow & ROLE_RACEMASK))
            return FALSE;
        if (gendnum >= 0 && gendnum < ROLE_GENDERS
            && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
            return FALSE;
        if (alignnum >= 0 && alignnum < ROLE_ALIGNS
            && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < SIZE(roles) - 1; i++) {
            if (gr.rfilter.roles[i])
                continue;
            allow = roles[i].allow;
            if (IndexOkT(racenum, races)
                && !(allow & races[racenum].allow & ROLE_RACEMASK))
                continue;
            if (gendnum >= 0 && gendnum < ROLE_GENDERS
                && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
                continue;
            if (alignnum >= 0 && alignnum < ROLE_ALIGNS
                && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* pick a random role subject to any racenum/gendnum/alignnum constraints */
/* If pickhow == PICK_RIGID a role is returned only if there is  */
/* a single possibility */
int
pick_role(int racenum, int gendnum, int alignnum, int pickhow)
{
    int i;
    int roles_ok = 0, set[SIZE(roles)];

    for (i = 0; i < SIZE(roles) - 1; i++) {
        if (ok_role(i, racenum, gendnum, alignnum)
            && ok_race(i, (racenum >= 0) ? racenum : ROLE_RANDOM,
                       gendnum, alignnum)
            && ok_gend(i, racenum,
                       (gendnum >= 0) ? gendnum : ROLE_RANDOM, alignnum)
            && ok_align(i, racenum,
                        gendnum, (alignnum >= 0) ? alignnum : ROLE_RANDOM))
            set[roles_ok++] = i;
    }
    if (roles_ok == 0 || (roles_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    return set[rn2(roles_ok)];
}

/* is racenum compatible with any rolenum/gendnum/alignnum constraints? */
boolean
ok_race(int rolenum, int racenum, int gendnum, int alignnum)
{
    int i;
    short allow;

    if (IndexOkT(racenum, races)) {
        if (gr.rfilter.mask & races[racenum].selfmask)
            return FALSE;
        allow = races[racenum].allow;
        if (IndexOkT(rolenum, roles)
            && !(allow & roles[rolenum].allow & ROLE_RACEMASK))
            return FALSE;
        if (gendnum >= 0 && gendnum < ROLE_GENDERS
            && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
            return FALSE;
        if (alignnum >= 0 && alignnum < ROLE_ALIGNS
            && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < SIZE(races) - 1; i++) {
            if (gr.rfilter.mask & races[i].selfmask)
                continue;
            allow = races[i].allow;
            if (IndexOkT(rolenum, roles)
                && !(allow & roles[rolenum].allow & ROLE_RACEMASK))
                continue;
            if (gendnum >= 0 && gendnum < ROLE_GENDERS
                && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
                continue;
            if (alignnum >= 0 && alignnum < ROLE_ALIGNS
                && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* Pick a random race subject to any rolenum/gendnum/alignnum constraints.
   If pickhow == PICK_RIGID a race is returned only if there is
   a single possibility. */
int
pick_race(int rolenum, int gendnum, int alignnum, int pickhow)
{
    int i;
    int races_ok = 0;

    for (i = 0; i < SIZE(races) - 1; i++) {
        if (ok_race(rolenum, i, gendnum, alignnum))
            races_ok++;
    }
    if (races_ok == 0 || (races_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    races_ok = rn2(races_ok);
    for (i = 0; i < SIZE(races) - 1; i++) {
        if (ok_race(rolenum, i, gendnum, alignnum)) {
            if (races_ok == 0)
                return i;
            else
                races_ok--;
        }
    }
    return ROLE_NONE;
}

/* is gendnum compatible with any rolenum/racenum/alignnum constraints? */
/* gender and alignment are not comparable (and also not constrainable) */
boolean
ok_gend(int rolenum, int racenum, int gendnum, int alignnum UNUSED)
{
    int i;
    short allow;

    if (gendnum >= 0 && gendnum < ROLE_GENDERS) {
        if (gr.rfilter.mask & genders[gendnum].allow)
            return FALSE;
        allow = genders[gendnum].allow;
        if (IndexOkT(rolenum, roles)
            && !(allow & roles[rolenum].allow & ROLE_GENDMASK))
            return FALSE;
        if (IndexOkT(racenum, races)
            && !(allow & races[racenum].allow & ROLE_GENDMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < ROLE_GENDERS; i++) {
            if (gr.rfilter.mask & genders[i].allow)
                continue;
            allow = genders[i].allow;
            if (IndexOkT(rolenum, roles)
                && !(allow & roles[rolenum].allow & ROLE_GENDMASK))
                continue;
            if (IndexOkT(racenum, races)
                && !(allow & races[racenum].allow & ROLE_GENDMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* pick a random gender subject to any rolenum/racenum/alignnum constraints */
/* gender and alignment are not comparable (and also not constrainable) */
/* If pickhow == PICK_RIGID a gender is returned only if there is  */
/* a single possibility */
int
pick_gend(int rolenum, int racenum, int alignnum, int pickhow)
{
    int i;
    int gends_ok = 0;

    for (i = 0; i < ROLE_GENDERS; i++) {
        if (ok_gend(rolenum, racenum, i, alignnum))
            gends_ok++;
    }
    if (gends_ok == 0 || (gends_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    gends_ok = rn2(gends_ok);
    for (i = 0; i < ROLE_GENDERS; i++) {
        if (ok_gend(rolenum, racenum, i, alignnum)) {
            if (gends_ok == 0)
                return i;
            else
                gends_ok--;
        }
    }
    return ROLE_NONE;
}

/* is alignnum compatible with any rolenum/racenum/gendnum constraints? */
/* alignment and gender are not comparable (and also not constrainable) */
boolean
ok_align(int rolenum, int racenum, int gendnum UNUSED, int alignnum)
{
    int i;
    short allow;

    if (alignnum >= 0 && alignnum < ROLE_ALIGNS) {
        if (gr.rfilter.mask & aligns[alignnum].allow)
            return FALSE;
        allow = aligns[alignnum].allow;
        if (IndexOkT(rolenum, roles)
            && !(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
            return FALSE;
        if (IndexOkT(racenum, races)
            && !(allow & races[racenum].allow & ROLE_ALIGNMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < ROLE_ALIGNS; i++) {
            if (gr.rfilter.mask & aligns[i].allow)
                continue;
            allow = aligns[i].allow;
            if (IndexOkT(rolenum, roles)
                && !(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
                continue;
            if (IndexOkT(racenum, races)
                && !(allow & races[racenum].allow & ROLE_ALIGNMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* Pick a random alignment subject to any rolenum/racenum/gendnum constraints;
   alignment and gender are not comparable (and also not constrainable).
   If pickhow == PICK_RIGID an alignment is returned only if there is
   a single possibility. */
int
pick_align(int rolenum, int racenum, int gendnum, int pickhow)
{
    int i;
    int aligns_ok = 0;

    for (i = 0; i < ROLE_ALIGNS; i++) {
        if (ok_align(rolenum, racenum, gendnum, i))
            aligns_ok++;
    }
    if (aligns_ok == 0 || (aligns_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    aligns_ok = rn2(aligns_ok);
    for (i = 0; i < ROLE_ALIGNS; i++) {
        if (ok_align(rolenum, racenum, gendnum, i)) {
            if (aligns_ok == 0)
                return i;
            else
                aligns_ok--;
        }
    }
    return ROLE_NONE;
}

void
rigid_role_checks(void)
{
    int tmp;

    /* Some roles are limited to a single race, alignment, or gender and
     * calling this routine prior to XXX_player_selection() will help
     * prevent an extraneous prompt that actually doesn't allow
     * you to choose anything further. Note the use of PICK_RIGID which
     * causes the pick_XX() routine to return a value only if there is one
     * single possible selection, otherwise it returns ROLE_NONE.
     *
     */
    if (flags.initrole == ROLE_RANDOM) {
        /* If the role was explicitly specified as ROLE_RANDOM
         * via -uXXXX-@ or OPTIONS=role:random then choose the role
         * in here to narrow down later choices.
         */
        flags.initrole = pick_role(flags.initrace, flags.initgend,
                                   flags.initalign, PICK_RANDOM);
        if (flags.initrole < 0)
            flags.initrole = randrole_filtered();
    }
    if (flags.initrace == ROLE_RANDOM
        && (tmp = pick_race(flags.initrole, flags.initgend,
                            flags.initalign, PICK_RANDOM)) != ROLE_NONE)
        flags.initrace = tmp;
    if (flags.initalign == ROLE_RANDOM
        && (tmp = pick_align(flags.initrole, flags.initrace,
                             flags.initgend, PICK_RANDOM)) != ROLE_NONE)
        flags.initalign = tmp;
    if (flags.initgend == ROLE_RANDOM
        && (tmp = pick_gend(flags.initrole, flags.initrace,
                            flags.initalign, PICK_RANDOM)) != ROLE_NONE)
        flags.initgend = tmp;

    if (flags.initrole != ROLE_NONE) {
        if (flags.initrace == ROLE_NONE)
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RIGID);
        if (flags.initalign == ROLE_NONE)
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RIGID);
        if (flags.initgend == ROLE_NONE)
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RIGID);
    }
}

boolean
setrolefilter(const char *bufp)
{
    int i;
    boolean reslt = TRUE;

    if ((i = str2role(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        gr.rfilter.roles[i] = TRUE;
    else if ((i = str2race(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        gr.rfilter.mask |= races[i].selfmask;
    else if ((i = str2gend(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        gr.rfilter.mask |= genders[i].allow;
    else if ((i = str2align(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        gr.rfilter.mask |= aligns[i].allow;
    else
        reslt = FALSE;
    return reslt;
}

boolean
gotrolefilter(void)
{
    int i;

    if (gr.rfilter.mask)
        return TRUE;
    for (i = 0; i < SIZE(roles) - 1; ++i)
        if (gr.rfilter.roles[i])
            return TRUE;
    return FALSE;
}

/* create a string like " !Bar !Kni" or " !chaotic" that can be
   put back into an RC file by #saveoptions */
char *
rolefilterstring(char *outbuf, int which)
{
    int i;

    outbuf[0] = outbuf[1] = '\0';
    switch (which) {
    case RS_ROLE:
        for (i = 0; i < SIZE(roles) - 1; ++i) {
            if (gr.rfilter.roles[i])
                Sprintf(eos(outbuf), " !%.3s", roles[i].name.m);
        }
        break;
    case RS_RACE:
        for (i = 0; i < SIZE(races) - 1; ++i) {
            if ((gr.rfilter.mask & races[i].selfmask) != 0)
                Sprintf(eos(outbuf), " !%s", races[i].noun);
        }
        break;
    case RS_GENDER:
        for (i = 0; i < SIZE(genders) - 1; ++i) {
            if ((gr.rfilter.mask & genders[i].allow) != 0)
                Sprintf(eos(outbuf), " !%s", genders[i].adj);
        }
        break;
    case RS_ALGNMNT:
        for (i = 0; i < SIZE(aligns) - 1; ++i) {
            if ((gr.rfilter.mask & aligns[i].allow) != 0)
                Sprintf(eos(outbuf), " !%s", aligns[i].adj);
        }
        break;
    default:
        impossible("rolefilterstring: bad role aspect (%d)", which);
        Strcpy(outbuf, " ?");
        break;
    }
    /* constructed with a leading space; drop it */
    return &outbuf[1];
}

void
clearrolefilter(int which)
{
    int i;

    switch (which) {
    case RS_filter:
        gr.rfilter.mask = 0; /* clear race, gender, and alignment filters */
        FALLTHROUGH;
        /*FALLTHRU*/
    case RS_ROLE:
        for (i = 0; i < SIZE(roles) - 1; ++i)
            gr.rfilter.roles[i] = FALSE;
        break;
    case RS_RACE:
        gr.rfilter.mask &= ~ROLE_RACEMASK;
        break;
    case RS_GENDER:
        gr.rfilter.mask &= ~ROLE_GENDMASK;
        break;
    case RS_ALGNMNT:
        gr.rfilter.mask &= ~ROLE_ALIGNMASK;
        break;
    }
}

staticfn char *
promptsep(char *buf, int num_post_attribs)
{
    const char *conjuct = "and ";

    if (num_post_attribs > 1 && gr.role_post_attribs < num_post_attribs
        && gr.role_post_attribs > 1)
        Strcat(buf, ",");
    Strcat(buf, " ");
    --gr.role_post_attribs;
    if (!gr.role_post_attribs && num_post_attribs > 1)
        Strcat(buf, conjuct);
    return buf;
}

staticfn int
role_gendercount(int rolenum)
{
    int gendcount = 0;

    if (validrole(rolenum)) {
        if (roles[rolenum].allow & ROLE_MALE)
            ++gendcount;
        if (roles[rolenum].allow & ROLE_FEMALE)
            ++gendcount;
        if (roles[rolenum].allow & ROLE_NEUTER)
            ++gendcount;
    }
    return gendcount;
}

staticfn int
race_alignmentcount(int racenum)
{
    int aligncount = 0;

    if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
        if (races[racenum].allow & ROLE_CHAOTIC)
            ++aligncount;
        if (races[racenum].allow & ROLE_LAWFUL)
            ++aligncount;
        if (races[racenum].allow & ROLE_NEUTRAL)
            ++aligncount;
    }
    return aligncount;
}

char *
root_plselection_prompt(
    char *suppliedbuf, int buflen,
    int rolenum, int racenum, int gendnum, int alignnum)
{
    int k, gendercount = 0, aligncount = 0;
    char buf[BUFSZ];
    static char err_ret[] = " character's";
    boolean donefirst = FALSE;

    if (!suppliedbuf || buflen < 1)
        return err_ret;

    /* initialize these static variables each time this is called */
    gr.role_post_attribs = 0;
    for (k = 0; k < NUM_BP; ++k)
        gr.role_pa[k] = 0;
    buf[0] = '\0';
    *suppliedbuf = '\0';

    /* How many alignments are allowed for the desired race? */
    if (racenum != ROLE_NONE && racenum != ROLE_RANDOM)
        aligncount = race_alignmentcount(racenum);

    if (alignnum != ROLE_NONE && alignnum != ROLE_RANDOM
        && ok_align(rolenum, racenum, gendnum, alignnum)) {
#if 0   /* 'if' and 'else' had duplicate code here; probably a copy+parse
         * oversight; if a problem with filtering of random role selection
         * crops up, this is probably the place to start looking */

        /* if race specified, and multiple choice of alignments for it */
        if ((racenum >= 0) && (aligncount > 1)) {
        } else {
        }
#endif  /* the four lines of code below were in both 'if' and 'else' above */
        if (donefirst)
            Strcat(buf, " ");
        Strcat(buf, aligns[alignnum].adj);
        donefirst = TRUE;
    } else {
        /* in case we got here by failing the ok_align() test */
        if (alignnum != ROLE_RANDOM)
            alignnum = ROLE_NONE;
        /* if alignment not specified, but race is specified
           and only one choice of alignment for that race then
           don't include it in the later list */
        if ((((racenum != ROLE_NONE && racenum != ROLE_RANDOM)
              && ok_race(rolenum, racenum, gendnum, alignnum))
             && (aligncount > 1))
            || (racenum == ROLE_NONE || racenum == ROLE_RANDOM)) {
            gr.role_pa[BP_ALIGN] = 1;
            gr.role_post_attribs++;
        }
    }
    /* <your lawful> */

    /* How many genders are allowed for the desired role? */
    if (validrole(rolenum))
        gendercount = role_gendercount(rolenum);

    if (gendnum != ROLE_NONE && gendnum != ROLE_RANDOM) {
        if (validrole(rolenum)) {
            /* if role specified, and multiple choice of genders for it,
               and name of role itself does not distinguish gender */
            if ((rolenum != ROLE_NONE) && (gendercount > 1)
                && !roles[rolenum].name.f) {
                if (donefirst)
                    Strcat(buf, " ");
                Strcat(buf, genders[gendnum].adj);
                donefirst = TRUE;
            }
        } else {
            if (donefirst)
                Strcat(buf, " ");
            Strcat(buf, genders[gendnum].adj);
            donefirst = TRUE;
        }
    } else {
        /* if gender not specified, but role is specified
                and only one choice of gender then
                don't include it in the later list */
        if ((validrole(rolenum) && (gendercount > 1))
            || !validrole(rolenum)) {
            gr.role_pa[BP_GEND] = 1;
            gr.role_post_attribs++;
        }
    }
    /* <your lawful female> */

    if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
        if (validrole(rolenum)
            && ok_race(rolenum, racenum, gendnum, alignnum)) {
            if (donefirst)
                Strcat(buf, " ");
            Strcat(buf, (rolenum == ROLE_NONE) ? races[racenum].noun
                                               : races[racenum].adj);
            donefirst = TRUE;
        } else if (!validrole(rolenum)) {
            if (donefirst)
                Strcat(buf, " ");
            Strcat(buf, races[racenum].noun);
            donefirst = TRUE;
        } else {
            gr.role_pa[BP_RACE] = 1;
            gr.role_post_attribs++;
        }
    } else {
        gr.role_pa[BP_RACE] = 1;
        gr.role_post_attribs++;
    }
    /* <your lawful female gnomish> || <your lawful female gnome> */

    if (validrole(rolenum)) {
        assert(IndexOkT(rolenum, roles));
        if (donefirst)
            Strcat(buf, " ");
        if (gendnum != ROLE_NONE) {
            if (gendnum == 1 && roles[rolenum].name.f)
                Strcat(buf, roles[rolenum].name.f);
            else
                Strcat(buf, roles[rolenum].name.m);
        } else {
            if (roles[rolenum].name.f) {
                Strcat(buf, roles[rolenum].name.m);
                Strcat(buf, "/");
                Strcat(buf, roles[rolenum].name.f);
            } else
                Strcat(buf, roles[rolenum].name.m);
        }
        donefirst = TRUE;
    } else if (rolenum == ROLE_NONE) {
        gr.role_pa[BP_ROLE] = 1;
        gr.role_post_attribs++;
    }

    if ((racenum == ROLE_NONE || racenum == ROLE_RANDOM)
        && !validrole(rolenum)) {
        if (donefirst)
            Strcat(buf, " ");
        Strcat(buf, "character");
        /*donefirst = TRUE;*/
    }
    /* <your lawful female gnomish cavewoman> || <your lawful female gnome>
     *    || <your lawful female character>
     */
    if (buflen > (int) (strlen(buf) + 1)) {
        Strcpy(suppliedbuf, buf);
        return suppliedbuf;
    } else
        return err_ret;
}

char *
build_plselection_prompt(
    char *buf, int buflen,
    int rolenum, int racenum, int gendnum, int alignnum)
{
    const char *defprompt = "Shall I pick a character for you? [ynaq] ";
    int num_post_attribs = 0;
    char tmpbuf[BUFSZ], *p;

    if (buflen < QBUFSZ)
        return (char *) defprompt;

    Strcpy(tmpbuf, "Shall I pick ");
    if (racenum != ROLE_NONE || validrole(rolenum))
        Strcat(tmpbuf, "your ");
    else
        Strcat(tmpbuf, "a ");
    /* <your> */

    (void) root_plselection_prompt(eos(tmpbuf), buflen - Strlen(tmpbuf),
                                   rolenum, racenum, gendnum, alignnum);
    /* "Shall I pick a character's role, race, gender, and alignment for you?"
       plus " [ynaq] (y)" is a little too long for a conventional 80 columns;
       also, "pick a character's <anything>" sounds a bit stilted */
    strsubst(tmpbuf, "pick a character", "pick character");
    Sprintf(buf, "%s", s_suffix(tmpbuf));
    /* don't bother splitting caveman/cavewoman or priest/priestess
       in order to apply possessive suffix to both halves, but do
       change "priest/priestess'" to "priest/priestess's" */
    if ((p = strstri(buf, "priest/priestess'")) != 0
        && p[sizeof "priest/priestess'" - sizeof ""] == '\0')
        strkitten(buf, 's');

    /* buf should now be:
     *    <your lawful female gnomish cavewoman's>
     * || <your lawful female gnome's>
     * || <your lawful female character's>
     *
     * Now append the post attributes to it
     */
    num_post_attribs = gr.role_post_attribs;
    if (!num_post_attribs) {
        /* some constraints might have been mutually exclusive, in which case
           some prompting that would have been omitted is needed after all */
        if (flags.initrole == ROLE_NONE && !gr.role_pa[BP_ROLE])
            gr.role_pa[BP_ROLE] = ++gr.role_post_attribs;
        if (flags.initrace == ROLE_NONE && !gr.role_pa[BP_RACE])
            gr.role_pa[BP_RACE] = ++gr.role_post_attribs;
        if (flags.initalign == ROLE_NONE && !gr.role_pa[BP_ALIGN])
            gr.role_pa[BP_ALIGN] = ++gr.role_post_attribs;
        if (flags.initgend == ROLE_NONE && !gr.role_pa[BP_GEND])
            gr.role_pa[BP_GEND] = ++gr.role_post_attribs;
        num_post_attribs = gr.role_post_attribs;
    }
    if (num_post_attribs) {
        if (gr.role_pa[BP_RACE]) {
            (void) promptsep(eos(buf), num_post_attribs);
            Strcat(buf, "race");
        }
        if (gr.role_pa[BP_ROLE]) {
            (void) promptsep(eos(buf), num_post_attribs);
            Strcat(buf, "role");
        }
        if (gr.role_pa[BP_GEND]) {
            (void) promptsep(eos(buf), num_post_attribs);
            Strcat(buf, "gender");
        }
        if (gr.role_pa[BP_ALIGN]) {
            (void) promptsep(eos(buf), num_post_attribs);
            Strcat(buf, "alignment");
        }
    }
    Strcat(buf, " for you? [ynaq] ");
    return buf;
}

#undef BP_ALIGN
#undef BP_GEND
#undef BP_RACE
#undef BP_ROLE
#undef NUM_BP

void
plnamesuffix(void)
{
    char *sptr, *eptr;
    int i;

    /* some generic user names will be ignored in favor of prompting */
    if (sysopt.genericusers) {
        if (*sysopt.genericusers == '*') {
            svp.plname[0] = '\0';
        } else {
            /* need to ignore appended '-role-race-gender-alignment';
               'plnamelen' is non-zero when dealing with plname[] value that
               contains a username with dash(es) in it and is usually 0 */
            i = ((eptr = strchr(svp.plname + gp.plnamelen, '-')) != 0)
                ? (int) (eptr - svp.plname)
                : (int) Strlen(svp.plname);
            /* look for plname[] in the 'genericusers' space-separated list */
            if (findword(sysopt.genericusers, svp.plname, i, FALSE))
                /* it's generic; remove it so that askname() will be called */
                svp.plname[0] = '\0';
        }
        if (!svp.plname[0])
            gp.plnamelen = 0;
    }

    do {
        if (!svp.plname[0]) {
            askname(); /* fill svp.plname[] if necessary, or set
                        * defer_plname */
            gp.plnamelen = 0; /* plname[] might have -role-race-&c attached */
        }

        /* Look for tokens delimited by '-' */
        sptr = svp.plname + gp.plnamelen;
        if ((eptr = strchr(sptr, '-')) != (char *) 0)
            *eptr++ = '\0';
        while (eptr) {
            /* Isolate the next token */
            sptr = eptr;
            if ((eptr = strchr(sptr, '-')) != (char *) 0)
                *eptr++ = '\0';

            /* Try to match it to something */
            if ((i = str2role(sptr)) != ROLE_NONE)
                flags.initrole = i;
            else if ((i = str2race(sptr)) != ROLE_NONE)
                flags.initrace = i;
            else if ((i = str2gend(sptr)) != ROLE_NONE)
                flags.initgend = i;
            else if ((i = str2align(sptr)) != ROLE_NONE)
                flags.initalign = i;
        }
    } while (!svp.plname[0] && !iflags.defer_plname);

    /* commas in the svp.plname confuse the record file, convert to spaces */
    (void) strNsubst(svp.plname, ",", " ", 0);
}

/* show current settings for name, role, race, gender, and alignment
   in the specified window */
void
role_selection_prolog(int which, winid where)
{
    static const char NEARDATA choosing[] = " choosing now",
                               not_yet[] = " not yet specified",
                               rand_choice[] = " random";
    char buf[BUFSZ];
    int r, c, gend, a, allowmask;

    r = flags.initrole;
    c = flags.initrace;
    gend = flags.initgend;
    a = flags.initalign;
    if (r >= 0) {
        assert(IndexOkT(r, roles));
        allowmask = roles[r].allow;
        if ((allowmask & ROLE_RACEMASK) == MH_HUMAN)
            c = 0; /* races[human] */
        else if (IndexOkT(c, races)
                 && !(allowmask & ROLE_RACEMASK & races[c].allow))
            c = ROLE_RANDOM;
        if ((allowmask & ROLE_GENDMASK) == ROLE_MALE)
            gend = 0; /* role forces male (hypothetical) */
        else if ((allowmask & ROLE_GENDMASK) == ROLE_FEMALE)
            gend = 1; /* role forces female (valkyrie) */
        if ((allowmask & ROLE_ALIGNMASK) == AM_LAWFUL)
            a = 0; /* aligns[lawful] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_NEUTRAL)
            a = 1; /* aligns[neutral] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_CHAOTIC)
            a = 2; /* aligns[chaotic] */
    }
    if (c >= 0) {
        assert(IndexOkT(c, races));
        allowmask = races[c].allow;
        if ((allowmask & ROLE_ALIGNMASK) == AM_LAWFUL)
            a = 0; /* aligns[lawful] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_NEUTRAL)
            a = 1; /* aligns[neutral] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_CHAOTIC)
            a = 2; /* aligns[chaotic] */
        /* [c never forces gender] */
    }
    /* [g and a don't constrain anything sufficiently
       to narrow something done to a single choice] */

    Sprintf(buf, "%12s ", "name:");
    Strcat(buf, (which == RS_NAME) ? choosing
                : !*svp.plname ? not_yet : svp.plname);
    putstr(where, 0, buf);
    Sprintf(buf, "%12s ", "role:");
    assert(which == RS_ROLE || r == ROLE_NONE || r == ROLE_RANDOM
           || IndexOkT(r, roles));
    Strcat(buf, (which == RS_ROLE) ? choosing
                : (r == ROLE_NONE) ? not_yet
                  : (r == ROLE_RANDOM) ? rand_choice
                    : roles[r].name.m);
    if (r >= 0 && roles[r].name.f) {
        /* distinct female name [caveman/cavewoman, priest/priestess] */
        if (gend == 1)
            /* female specified; replace male role name with female one */
            Sprintf(strchr(buf, ':'), ": %s", roles[r].name.f);
        else if (gend < 0)
            /* gender unspecified; append slash and female role name */
            Sprintf(eos(buf), "/%s", roles[r].name.f);
    }
    putstr(where, 0, buf);
    Sprintf(buf, "%12s ", "race:");
    assert(which == RS_RACE || c == ROLE_NONE || c == ROLE_RANDOM
           || IndexOkT(c, races));
    Strcat(buf, (which == RS_RACE) ? choosing
                : (c == ROLE_NONE) ? not_yet
                  : (c == ROLE_RANDOM) ? rand_choice
                    : races[c].noun);
    putstr(where, 0, buf);
    Sprintf(buf, "%12s ", "gender:");
    Strcat(buf, (which == RS_GENDER) ? choosing
                : (gend == ROLE_NONE) ? not_yet
                  : (gend == ROLE_RANDOM) ? rand_choice
                    : genders[gend].adj);
    putstr(where, 0, buf);
    Sprintf(buf, "%12s ", "alignment:");
    Strcat(buf, (which == RS_ALGNMNT) ? choosing
                : (a == ROLE_NONE) ? not_yet
                  : (a == ROLE_RANDOM) ? rand_choice
                    : aligns[a].adj);
    putstr(where, 0, buf);
}

/* add a "pick alignment first"-type entry to the specified menu */
void
role_menu_extra(int which, winid where, boolean preselect)
{
    static NEARDATA const char RS_menu_let[] = {
        '=',  /* name */
        '?',  /* role */
        '/',  /* race */
        '\"', /* gender */
        '[',  /* alignment */
    };
    anything any;
    char buf[BUFSZ];
    const char *what = 0, *constrainer = 0, *forcedvalue = 0;
    int f = 0, r, c, gend, a, i, allowmask;
    int clr = NO_COLOR;

    r = flags.initrole;
    c = flags.initrace;
    switch (which) {
    case RS_NAME:
        what = "name";
        break;
    case RS_ROLE:
        what = "role";
        f = r;
        for (i = 0; i < SIZE(roles) - 1; ++i)
            if (i != f && !gr.rfilter.roles[i])
                break;
        if (i == SIZE(roles) - 1) {
            constrainer = "filter";
            forcedvalue = "role";
        }
        break;
    case RS_RACE:
        what = "race";
        f = flags.initrace;
        c = ROLE_NONE; /* override player's setting */
        if (r >= 0) {
            allowmask = roles[r].allow & ROLE_RACEMASK;
            if (allowmask == MH_HUMAN)
                c = 0; /* races[human] */
            if (c >= 0) {
                constrainer = "role";
                forcedvalue = races[c].noun;
            } else if (f >= 0 && ((allowmask & ~gr.rfilter.mask)
                                  == races[f].selfmask)) {
                /* if there is only one race choice available due to user
                   options disallowing others, race menu entry is disabled */
                constrainer = "filter";
                forcedvalue = "race";
            }
        }
        break;
    case RS_GENDER:
        what = "gender";
        f = flags.initgend;
        gend = ROLE_NONE;
        if (r >= 0) {
            allowmask = roles[r].allow & ROLE_GENDMASK;
            if (allowmask == ROLE_MALE)
                gend = 0; /* genders[male] */
            else if (allowmask == ROLE_FEMALE)
                gend = 1; /* genders[female] */
            if (gend >= 0) {
                constrainer = "role";
                forcedvalue = genders[gend].adj;
            } else if (f >= 0 && ((allowmask & ~gr.rfilter.mask)
                                  == genders[f].allow)) {
                /* if there is only one gender choice available due to user
                   options disallowing other, gender menu entry is disabled */
                constrainer = "filter";
                forcedvalue = "gender";
            }
        }
        break;
    case RS_ALGNMNT:
        what = "alignment";
        f = flags.initalign;
        a = ROLE_NONE;
        if (r >= 0) {
            allowmask = roles[r].allow & ROLE_ALIGNMASK;
            if (allowmask == AM_LAWFUL)
                a = 0; /* aligns[lawful] */
            else if (allowmask == AM_NEUTRAL)
                a = 1; /* aligns[neutral] */
            else if (allowmask == AM_CHAOTIC)
                a = 2; /* aligns[chaotic] */
            if (a >= 0)
                constrainer = "role";
        }
        if (c >= 0 && !constrainer) {
            allowmask = races[c].allow & ROLE_ALIGNMASK;
            if (allowmask == AM_LAWFUL)
                a = 0; /* aligns[lawful] */
            else if (allowmask == AM_NEUTRAL)
                a = 1; /* aligns[neutral] */
            else if (allowmask == AM_CHAOTIC)
                a = 2; /* aligns[chaotic] */
            if (a >= 0)
                constrainer = "race";
        }
        if (f >= 0 && !constrainer
            && (ROLE_ALIGNMASK & ~gr.rfilter.mask) == aligns[f].allow) {
            /* if there is only one alignment choice available due to user
               options disallowing others, algn menu entry is disabled */
            constrainer = "filter";
            forcedvalue = "alignment";
        }
        if (a >= 0)
            forcedvalue = aligns[a].adj;
        break;
    }

    any = cg.zeroany; /* zero out all bits */
    if (constrainer) {
        any.a_int = 0;
        /* use four spaces of padding to fake a grayed out menu choice */
        Sprintf(buf, "%4s%s forces %s", "", constrainer, forcedvalue);
        add_menu_str(where, buf);
    } else if (what) {
        any.a_int = RS_menu_arg(which);
        Sprintf(buf, "Pick%s %s first", (f >= 0) ? " another" : "", what);
        add_menu(where, &nul_glyphinfo, &any, RS_menu_let[which], 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    } else if (which == RS_filter) {
        char setfiltering[40];

        any.a_int = RS_menu_arg(RS_filter);
        Sprintf(setfiltering, "%s role/race/&c filtering",
                gotrolefilter() ? "Reset" : "Set");
        add_menu(where, &nul_glyphinfo, &any, '~', 0, ATR_NONE,
                 clr, setfiltering, MENU_ITEMFLAGS_NONE);
    } else if (which == ROLE_RANDOM) {
        any.a_int = ROLE_RANDOM;
        add_menu(where, &nul_glyphinfo, &any, '*', 0,
                 ATR_NONE, clr, "Random",
                 preselect ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    } else if (which == ROLE_NONE) {
        any.a_int = ROLE_NONE;
        add_menu(where, &nul_glyphinfo, &any, 'q', 0,
                 ATR_NONE, clr, "Quit",
                 preselect ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    } else {
        impossible("role_menu_extra: bad arg (%d)", which);
    }
}

/*
 *      Special setup modifications here:
 *
 *      Unfortunately, this is going to have to be done
 *      on each newgame or restore, because you lose the permonst mods
 *      across a save/restore.  :-)
 *
 *      1 - The Rogue Leader is the Tourist Nemesis.
 *      2 - Priests start with a random alignment - convert the leader and
 *          guardians here.
 *      3 - Priests also get their set of deities from a randomly chosen role.
 *      4 - [obsolete] Elves can have one of two different leaders,
 *          but can't work it out here because it requires hacking the
 *          level file data (see sp_lev.c).
 *
 * This code also replaces quest_init().
 */
void
role_init(void)
{
    int alignmnt;
    struct permonst *pm;

    /* Strip the role letter out of the player name.
     * This is included for backwards compatibility.
     */
    plnamesuffix();

    /* Check for a valid role.  Try flags.initrole first. */
    if (!validrole(flags.initrole)) {
        /* Try the player letter second */
        if ((flags.initrole = str2role(svp.pl_character)) < 0)
            /* None specified; pick a random role */
            flags.initrole = randrole_filtered();
    }

    /* We now have a valid role index.  Copy the role name back. */
    /* This should become OBSOLETE */
    Strcpy(svp.pl_character, roles[flags.initrole].name.m);
    svp.pl_character[PL_CSIZ - 1] = '\0';

    /* Check for a valid race */
    if (!validrace(flags.initrole, flags.initrace))
        flags.initrace = randrace(flags.initrole);

    /* Check for a valid gender.  If new game, check both initgend
     * and female.  On restore, assume flags.female is correct. */
    if (flags.pantheon == -1) { /* new game */
        if (!validgend(flags.initrole, flags.initrace, flags.female))
            flags.female = !flags.female;
    }
    if (!validgend(flags.initrole, flags.initrace, flags.initgend))
        /* Note that there is no way to check for an unspecified gender. */
        flags.initgend = flags.female;

    /* Check for a valid alignment */
    if (!validalign(flags.initrole, flags.initrace, flags.initalign))
        /* Pick a random alignment */
        flags.initalign = randalign(flags.initrole, flags.initrace);
    alignmnt = aligns[flags.initalign].value;

    /* Initialize gu.urole and gu.urace */
    gu.urole = roles[flags.initrole];
    gu.urace = races[flags.initrace];

    /* Fix up the quest leader */
    if (gu.urole.ldrnum != NON_PM) {
        pm = &mons[gu.urole.ldrnum];
        pm->msound = MS_LEADER;
        pm->mflags2 |= (M2_PEACEFUL);
        pm->mflags3 |= M3_CLOSE;
        pm->maligntyp = alignmnt * 3;
        /* if gender is random, we choose it now instead of waiting
           until the leader monster is created */
        svq.quest_status.ldrgend =
            is_neuter(pm) ? 2 : is_female(pm) ? 1 : is_male(pm)
                                                        ? 0
                                                        : (rn2(100) < 50);
    }

    /* Fix up the quest guardians */
    if (gu.urole.guardnum != NON_PM) {
        pm = &mons[gu.urole.guardnum];
        pm->mflags2 |= (M2_PEACEFUL);
        pm->maligntyp = alignmnt * 3;
    }

    /* Fix up the quest nemesis */
    if (gu.urole.neminum != NON_PM) {
        pm = &mons[gu.urole.neminum];
        pm->msound = MS_NEMESIS;
        pm->mflags2 &= ~(M2_PEACEFUL);
        pm->mflags2 |= (M2_NASTY | M2_STALK | M2_HOSTILE);
        pm->mflags3 &= ~(M3_CLOSE);
        pm->mflags3 |= M3_WANTSARTI | M3_WAITFORU;
        /* if gender is random, we choose it now instead of waiting
           until the nemesis monster is created */
        svq.quest_status.nemgend = is_neuter(pm) ? 2 : is_female(pm) ? 1
                                   : is_male(pm) ? 0 : (rn2(100) < 50);
    }

    /* Fix up the god names */
    if (flags.pantheon == -1) {             /* new game */
        int trycnt = 0;
        flags.pantheon = flags.initrole;    /* use own gods */
        /* unless they're missing */
        while (!roles[flags.pantheon].lgod && ++trycnt < 100)
            flags.pantheon = randrole(FALSE);
        if (!roles[flags.pantheon].lgod) {
            int i;
            for (i = 0; i < SIZE(roles) - 1; i++)
                if (roles[i].lgod) {
                    flags.pantheon = i;
                    break;
                }
        }
    }
    if (!gu.urole.lgod) {
        gu.urole.lgod = roles[flags.pantheon].lgod;
        gu.urole.ngod = roles[flags.pantheon].ngod;
        gu.urole.cgod = roles[flags.pantheon].cgod;
    }
    /* 0 or 1; no gods are neuter, nor is gender randomized */
    svq.quest_status.godgend = !strcmpi(align_gtitle(alignmnt), "goddess");

    if (Role_if(PM_CLERIC))
        objects[SPE_LIGHT].oc_skill = P_CLERIC_SPELL;

#if 0
/*
 * Disable this fixup so that mons[] can be const.  The only
 * place where it actually matters for the hero is in set_uasmon()
 * and that can use mons[race] rather than mons[role] for this
 * particular property.  Despite the comment, it is checked--where
 * needed--via intrinsic 'Infravision' which set_uasmon() manages.
 */
    /* Fix up infravision */
    if (mons[gu.urace.mnum].mflags3 & M3_INFRAVISION) {
        /* although an infravision intrinsic is possible, infravision
         * is purely a property of the physical race.  This means that we
         * must put the infravision flag in the player's current race
         * (either that or have separate permonst entries for
         * elven/non-elven members of each class).  The side effect is that
         * all NPCs of that class will have (probably bogus) infravision,
         * but since infravision has no effect for NPCs anyway we can
         * ignore this.
         */
        mons[gu.urole.mnum].mflags3 |= M3_INFRAVISION;
    }
#endif /*0*/

    /* Artifacts are fixed in hack_artifacts() */

    /* Success! */
    return;
}

const char *
Hello(struct monst *mtmp)
{
    switch (Role_switch) {
    case PM_KNIGHT:
        return "Salutations"; /* Olde English */
    case PM_SAMURAI:
        return (mtmp && mtmp->data == &mons[PM_SHOPKEEPER])
                    ? "Irasshaimase"
                    : "Konnichi wa"; /* Japanese */
    case PM_TOURIST:
        return "Aloha"; /* Hawaiian */
    case PM_VALKYRIE:
        return
#ifdef MAIL_STRUCTURES
               (mtmp && mtmp->data == &mons[PM_MAIL_DAEMON]) ? "Hallo" :
#endif
               "Velkommen"; /* Norse */
    default:
        return "Hello";
    }
}

const char *
Goodbye(void)
{
    switch (Role_switch) {
    case PM_KNIGHT:
        return "Fare thee well"; /* Olde English */
    case PM_SAMURAI:
        return "Sayonara"; /* Japanese */
    case PM_TOURIST:
        return "Aloha"; /* Hawaiian */
    case PM_VALKYRIE:
        return "Farvel"; /* Norse */
    default:
        return "Goodbye";
    }
}

/* if pmindex is any player race (not necessarily the hero's),
   return a pointer to the races[] entry for it; if pmindex is for some
   other type of monster which isn't a player race, return Null */
const struct Race *
character_race(short pmindex)
{
    const struct Race *r;

    for (r = races; r->noun != NULL; ++r)
        if (r->mnum == pmindex)
            return r;
    return (const struct Race *) NULL;
}

/*--------------------------------------------------------------------------*/

/* potential interface routine */
void
genl_player_selection(void)
{
    if (genl_player_setup(0))
        return;

    /* player cancelled role/race/&c selection, so quit */
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS) || defined(SHIM_GRAPHICS)
/* ['#else' far below] */

staticfn boolean reset_role_filtering(void);
staticfn winid plsel_startmenu(int, int);
staticfn int maybe_skip_seps(int, int);
staticfn void setup_rolemenu(winid, boolean, int, int, int);
staticfn void setup_racemenu(winid, boolean, int, int, int);
staticfn void setup_gendmenu(winid, boolean, int, int, int);
staticfn void setup_algnmenu(winid, boolean, int, int, int);

/* try to reduce clutter in the code below... */
#define ROLE flags.initrole
#define RACE flags.initrace
#define GEND flags.initgend
#define ALGN flags.initalign

/* guts of tty's player_selection() */
int
genl_player_setup(int screenheight)
{
    char pbuf[QBUFSZ];
    anything any;
    int i, k, n, choice, nextpick;
    boolean getconfirmation, picksomething;
    winid win = WIN_ERR;
    menu_item *selected = 0;
    int clr = NO_COLOR;
    char pick4u = 'n';
    int result = 0; /* assume failure (player chooses to 'quit') */

    program_state.in_role_selection++; /* affects tty menu cleanup */
    /* Used to avoid "Is this ok?" if player has already specified all
     * four facets of role.
     * Note that rigid_role_checks might force any unspecified facets to
     * have a specific value, but that will still require confirmation;
     * player can specify the forced ones if avoiding that is demanded.
     */
    picksomething = (ROLE == ROLE_NONE || RACE == ROLE_NONE
                     || GEND == ROLE_NONE || ALGN == ROLE_NONE);
    /* Used for '-@';
     * choose randomly without asking for all unspecified facets.
     */
    if (flags.randomall && picksomething) {
        if (ROLE == ROLE_NONE)
            ROLE = ROLE_RANDOM;
        if (RACE == ROLE_NONE)
            RACE = ROLE_RANDOM;
        if (GEND == ROLE_NONE)
            GEND = ROLE_RANDOM;
        if (ALGN == ROLE_NONE)
            ALGN = ROLE_RANDOM;
    }

    /* prevent unnecessary prompting if role forces race (samurai) or gender
       (valkyrie) or alignment (rogue), or race forces alignment (orc), &c */
    rigid_role_checks();

    if (ROLE == ROLE_NONE || RACE == ROLE_NONE
        || GEND == ROLE_NONE || ALGN == ROLE_NONE) {
        char *prompt = build_plselection_prompt(pbuf, QBUFSZ,
                                                ROLE, RACE, GEND, ALGN);
        /* prompt[] contains "Shall I pick ... for you? [ynaq] "
           y - game picks role,&c then asks player to confirm;
           n - player manually chooses via menu selections;
           a - like 'y', but skips confirmation and starts game;
           q - quit
         */
#if 1
        trimspaces(prompt); /* 'prompt' is constructed with trailing space */
        /* accept any character and do validation ourselves so that we can
           shorten prompt; it will be "Shall I pick ... for you? [ynaq] "
           with final space appended by yn_function() [for tty at least] */
        do {
            pick4u = yn_function(prompt, (char *) 0, '\0', FALSE);
            pick4u = lowc(pick4u);
            if (pick4u == '\033' || pick4u == 'q') /* handle [q] */
                goto setup_done;
            if (pick4u == ' ' || pick4u == '\n' || pick4u == '\r')
                pick4u = 'y'; /* default */
            else if (pick4u == '@' || pick4u == '*')
                pick4u = 'a'; /* similar to '-@' on command line */
            /* TODO? handle response of '?' */
        } while (pick4u != 'y' && pick4u != 'n' && pick4u != 'a'); /* [yna] */

#else /* slightly simpler but more likely to end up being wrapped */

        char *p;
        /* strip choices off prompt string; yn_function() will show them */
        if ((p = strchr(prompt, '[')) != 0)
            *p = '\0';
        trimspaces(prompt); /* remove trailing space */
        /* prompt becomes "Shall I pick ... for you? [ynaq] (y) "
           with " [ynaq] (y) " appended by yn_function() which also changes
           user's <space> and <return> to 'y', <escape> to 'q' */
        pick4u = yn_function(prompt, ynaqchars, 'y', FALSE);
        if (pick4u != 'y' && pick4u != 'a' && pick4u != 'n')
            goto setup_done; /* bail */
#endif
    }

 makepicks:
    nextpick = RS_ROLE;
    do {
        if (nextpick == RS_ROLE) {
            nextpick = RS_RACE;
            /* Select a role, if necessary;
               we'll try to be compatible with pre-selected
               race/gender/alignment, but may not succeed. */
            if (ROLE < 0) {
                /* process the choice */
                if (pick4u == 'y' || pick4u == 'a' || ROLE == ROLE_RANDOM) {
                    /* pick a random role */
                    k = pick_role(RACE, GEND, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        pline("Incompatible role!");
                        k = randrole(FALSE);
                    }
                } else {
                    /* 'excess' is used to try to avoid tty pagination */
                    int excess = maybe_skip_seps(screenheight, RS_ROLE);

                    /* prompt for a role */
                    win = plsel_startmenu(screenheight, RS_ROLE);
                    /* populate the menu with role choices */
                    setup_rolemenu(win, TRUE, RACE, GEND, ALGN);
                    /* add miscellaneous menu entries */
                    role_menu_extra(ROLE_RANDOM, win, TRUE);
                    any = cg.zeroany; /* separator, not a choice */
                    if (excess < 1 || excess > 2)
                        add_menu_str(win, "");
                    role_menu_extra(RS_RACE, win, FALSE);
                    role_menu_extra(RS_GENDER, win, FALSE);
                    role_menu_extra(RS_ALGNMNT, win, FALSE);
                    role_menu_extra(RS_filter, win, FALSE);
                    role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                    Strcpy(pbuf, "Pick a role or profession");
                    end_menu(win, pbuf);
                    n = select_menu(win, PICK_ONE, &selected);
                    /*
                     * PICK_ONE with preselected choice behaves strangely:
                     *  n == -1 -- <escape>, so use quit choice;
                     *  n ==  0 -- explicitly chose preselected entry,
                     *             toggling it off, so use it;
                     *  n ==  1 -- implicitly chose preselected entry
                     *             with <space> or <return>;
                     *  n ==  2 -- explicitly chose a different entry, so
                     *             both it and preselected one are in list.
                     */
                    if (n > 0) {
                        choice = selected[0].item.a_int;
                        if (n > 1 && choice == ROLE_RANDOM)
                            choice = selected[1].item.a_int;
                    } else
                        choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                    if (selected)
                        free((genericptr_t) selected), selected = 0;
                    destroy_nhwindow(win), win = WIN_ERR;

                    if (choice == ROLE_NONE) {
                        goto setup_done; /* selected quit */
                    } else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                        ALGN = k = ROLE_NONE;
                        nextpick = RS_ALGNMNT;
                    } else if (choice == RS_menu_arg(RS_GENDER)) {
                        GEND = k = ROLE_NONE;
                        nextpick = RS_GENDER;
                    } else if (choice == RS_menu_arg(RS_RACE)) {
                        RACE = k = ROLE_NONE;
                        nextpick = RS_RACE;
                    } else if (choice == RS_menu_arg(RS_filter)) {
                        ROLE = k = ROLE_NONE;
                        (void) reset_role_filtering();
                        nextpick = RS_ROLE;
                    } else if (choice == ROLE_RANDOM) {
                        k = pick_role(RACE, GEND, ALGN, PICK_RANDOM);
                        if (k < 0)
                            k = randrole(FALSE);
                    } else {
                        k = choice - 1;
                    }
                }
                ROLE = k;
            } /* needed role */
        }     /* picking role */

        if (nextpick == RS_RACE) {
            nextpick = (ROLE < 0) ? RS_ROLE : RS_GENDER;
            /* Select a race, if necessary;
               force compatibility with role, try for compatibility
               with pre-selected gender/alignment. */
            if (RACE < 0 || !validrace(ROLE, RACE)) {
                /* no race yet, or pre-selected race not valid */
                if (pick4u == 'y' || pick4u == 'a' || RACE == ROLE_RANDOM) {
                    k = pick_race(ROLE, GEND, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        pline("Incompatible race!");
                        k = randrace(ROLE);
                    }
                } else { /* pick4u == 'n' */
                    /* Count the number of valid races */
                    n = 0; /* number valid */
                    k = 0; /* valid race */
                    for (i = 0; races[i].noun; i++)
                        if (ok_race(ROLE, i, GEND, ALGN)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; races[i].noun; i++)
                            if (validrace(ROLE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        win = plsel_startmenu(screenheight, RS_RACE);
                        any = cg.zeroany; /* zero out all bits */
                        /* populate the menu with role choices */
                        setup_racemenu(win, TRUE, ROLE, GEND, ALGN);
                        /* add miscellaneous menu entries */
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu_str(win, "");
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_GENDER, win, FALSE);
                        role_menu_extra(RS_ALGNMNT, win, FALSE);
                        role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick a race or species");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        } else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t) selected), selected = 0;
                        destroy_nhwindow(win), win = WIN_ERR;

                        if (choice == ROLE_NONE) {
                            goto setup_done; /* selected quit */
                        } else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                            ALGN = k = ROLE_NONE;
                            nextpick = RS_ALGNMNT;
                        } else if (choice == RS_menu_arg(RS_GENDER)) {
                            GEND = k = ROLE_NONE;
                            nextpick = RS_GENDER;
                        } else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        } else if (choice == RS_menu_arg(RS_filter)) {
                            RACE = k = ROLE_NONE;
                            if (reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_RACE;
                        } else if (choice == ROLE_RANDOM) {
                            k = pick_race(ROLE, GEND, ALGN, PICK_RANDOM);
                            if (k < 0)
                                k = randrace(ROLE);
                        } else {
                            k = choice - 1;
                        }
                    }
                }
                RACE = k;
            } /* needed race */
        }     /* picking race */

        if (nextpick == RS_GENDER) {
            nextpick = (ROLE < 0) ? RS_ROLE : (RACE < 0) ? RS_RACE
                       : RS_ALGNMNT;
            /* Select a gender, if necessary;
               force compatibility with role/race, try for compatibility
               with pre-selected alignment. */
            if (GEND < 0 || !validgend(ROLE, RACE, GEND)) {
                /* no gender yet, or pre-selected gender not valid */
                if (pick4u == 'y' || pick4u == 'a' || GEND == ROLE_RANDOM) {
                    k = pick_gend(ROLE, RACE, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        pline("Incompatible gender!");
                        k = randgend(ROLE, RACE);
                    }
                } else { /* pick4u == 'n' */
                    /* Count the number of valid genders */
                    n = 0; /* number valid */
                    k = 0; /* valid gender */
                    for (i = 0; i < ROLE_GENDERS; i++)
                        if (ok_gend(ROLE, RACE, i, ALGN)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; i < ROLE_GENDERS; i++)
                            if (validgend(ROLE, RACE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        win = plsel_startmenu(screenheight, RS_GENDER);
                        any = cg.zeroany; /* zero out all bits */
                        /* populate the menu with gender choices */
                        setup_gendmenu(win, TRUE, ROLE, RACE, ALGN);
                        /* add miscellaneous menu entries */
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu_str(win, "");
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_RACE, win, FALSE);
                        role_menu_extra(RS_ALGNMNT, win, FALSE);
                        role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick a gender or sex");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        } else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t) selected), selected = 0;
                        destroy_nhwindow(win), win = WIN_ERR;

                        if (choice == ROLE_NONE) {
                            goto setup_done; /* selected quit */
                        } else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                            ALGN = k = ROLE_NONE;
                            nextpick = RS_ALGNMNT;
                        } else if (choice == RS_menu_arg(RS_RACE)) {
                            RACE = k = ROLE_NONE;
                            nextpick = RS_RACE;
                        } else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        } else if (choice == RS_menu_arg(RS_filter)) {
                            GEND = k = ROLE_NONE;
                            if (reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_GENDER;
                        } else if (choice == ROLE_RANDOM) {
                            k = pick_gend(ROLE, RACE, ALGN, PICK_RANDOM);
                            if (k < 0)
                                k = randgend(ROLE, RACE);
                        } else {
                            k = choice - 1;
                        }
                    }
                }
                GEND = k;
            } /* needed gender */
        }     /* picking gender */

        if (nextpick == RS_ALGNMNT) {
            nextpick = (ROLE < 0) ? RS_ROLE
                       : (RACE < 0) ? RS_RACE
                         : RS_GENDER;
            /* Select an alignment, if necessary;
               force compatibility with role/race/gender. */
            if (ALGN < 0 || !validalign(ROLE, RACE, ALGN)) {
                /* no alignment yet, or pre-selected alignment not valid */
                if (pick4u == 'y' || pick4u == 'a' || ALGN == ROLE_RANDOM) {
                    k = pick_align(ROLE, RACE, GEND, PICK_RANDOM);
                    if (k < 0) {
                        pline("Incompatible alignment!");
                        k = randalign(ROLE, RACE);
                    }
                } else { /* pick4u == 'n' */
                    /* Count the number of valid alignments */
                    n = 0; /* number valid */
                    k = 0; /* valid alignment */
                    for (i = 0; i < ROLE_ALIGNS; i++)
                        if (ok_align(ROLE, RACE, GEND, i)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; i < ROLE_ALIGNS; i++)
                            if (validalign(ROLE, RACE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        win = plsel_startmenu(screenheight, RS_ALGNMNT);
                        any = cg.zeroany; /* zero out all bits */
                        setup_algnmenu(win, TRUE, ROLE, RACE, GEND);
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu_str(win, "");
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_RACE, win, FALSE);
                        role_menu_extra(RS_GENDER, win, FALSE);
                        role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick an alignment or creed");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        } else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t) selected), selected = 0;
                        destroy_nhwindow(win), win = WIN_ERR;

                        if (choice == ROLE_NONE) {
                            goto setup_done; /* selected quit */
                        } else if (choice == RS_menu_arg(RS_GENDER)) {
                            GEND = k = ROLE_NONE;
                            nextpick = RS_GENDER;
                        } else if (choice == RS_menu_arg(RS_RACE)) {
                            RACE = k = ROLE_NONE;
                            nextpick = RS_RACE;
                        } else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        } else if (choice == RS_menu_arg(RS_filter)) {
                            ALGN = k = ROLE_NONE;
                            if (reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_ALGNMNT;
                        } else if (choice == ROLE_RANDOM) {
                            k = pick_align(ROLE, RACE, GEND, PICK_RANDOM);
                            if (k < 0)
                                k = randalign(ROLE, RACE);
                        } else {
                            k = choice - 1;
                        }
                    }
                }
                ALGN = k;
            } /* needed alignment */
        }     /* picking alignment */

    } while (ROLE < 0 || RACE < 0 || GEND < 0 || ALGN < 0);

    /*
     *  Role, race, &c have now been determined;
     *  ask for confirmation and maybe go back to choose all over again.
     *
     *  Uses ynaq for familiarity, although 'a' is usually a
     *  superset of 'y' but here is an alternate form of 'n'.
     *  Menu layout:
     *   title:  Is this ok? [ynaq]
     *   blank:
     *    text:  $name, $alignment $gender $race $role
     *   blank:
     *    menu:  y + yes; play
     *           n - no; pick again
     *   maybe:  a - no; rename hero
     *           q - quit
     *           (end)
     */
    getconfirmation = (picksomething && pick4u != 'a' && !flags.randomall);
    while (getconfirmation) {
        win = plsel_startmenu(screenheight, RS_filter); /* filter: not ROLE */
        any = cg.zeroany; /* zero out all bits */
        /* [ynaq] menu choices */
        any.a_int = 1;
        add_menu(win, &nul_glyphinfo, &any, 'y', 0,
                 ATR_NONE, clr, "Yes; start game", MENU_ITEMFLAGS_SELECTED);
        any.a_int = 2;
        add_menu(win, &nul_glyphinfo, &any, 'n', 0,
                 ATR_NONE, clr, "No; choose role again", MENU_ITEMFLAGS_NONE);
        if (iflags.renameallowed) {
            any.a_int = 3;
            add_menu(win, &nul_glyphinfo, &any, 'a', 0, ATR_NONE,
                     clr, "Not yet; choose another name",
                     MENU_ITEMFLAGS_NONE);
        }
        any.a_int = -1;
        add_menu(win, &nul_glyphinfo, &any, 'q', 0,
                 ATR_NONE, clr, "Quit", MENU_ITEMFLAGS_NONE);
        Sprintf(pbuf, "Is this ok? [yn%sq]", iflags.renameallowed ? "a" : "");
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        /* [pick-one menus with a preselected entry behave oddly...] */
        choice = (n > 0) ? selected[n - 1].item.a_int : (n == 0) ? 1 : -1;
        if (selected)
            free((genericptr_t) selected), selected = 0;
        destroy_nhwindow(win);

        switch (choice) {
        default: /* 'q' or ESC */
            goto setup_done; /* quit */
            break;
        case 3: { /* 'a' */
            /*
             * TODO: what, if anything, should be done if the name is
             * changed to or from "wizard" after port-specific startup
             * code has set flags.debug based on the original name?
             */
            int saveROLE, saveRACE, saveGEND, saveALGN;

            iflags.renameinprogress = TRUE; /* affects main() in unixmain.c */
            /* plnamesuffix() can change any or all of ROLE, RACE,
               GEND, ALGN; we'll override that and honor only the name */
            saveROLE = ROLE, saveRACE = RACE,
            saveGEND = GEND, saveALGN = ALGN;
            svp.plname[0] = '\0';
            plnamesuffix(); /* calls askname() when svp.plname[] is empty */
            ROLE = saveROLE, RACE = saveRACE,
            GEND = saveGEND, ALGN = saveALGN;
            break; /* getconfirmation is still True */
        }
        case 2: /* 'n' */
            /* start fresh, but bypass "shall I pick everything for you?"
               step; any partial role selection via config file, command
               line, or name suffix is discarded this time */
            pick4u = 'n';
            ROLE = RACE = GEND = ALGN = ROLE_NONE;
            goto makepicks;
            break;
        case 1: /* 'y' or Space or Return/Enter */
            /* success; drop out through end of function */
            getconfirmation = FALSE;
            break;
        }
    } /* while 'getconfirmation' */
    /* Success! */
    result = 1;

 setup_done:
    program_state.in_role_selection--;
    return result;
}

staticfn boolean
reset_role_filtering(void)
{
    winid win;
    int i, n;
    char filterprompt[QBUFSZ];
    menu_item *selected = 0;

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);

    /* no extra blank line preceding this entry; end_menu supplies one */
    add_menu_str(win, "Unacceptable roles");
    setup_rolemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu_str(win, "");
    add_menu_str(win, "Unacceptable races");
    setup_racemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu_str(win, "");
    add_menu_str(win, "Unacceptable genders");
    setup_gendmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu_str(win, "");
    add_menu_str(win, "Unacceptable alignments");
    setup_algnmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    Sprintf(filterprompt, "Pick all that apply%s",
            gotrolefilter() ? " and/or unpick any that no longer apply" : "");
    end_menu(win, filterprompt);
    n = select_menu(win, PICK_ANY, &selected);

    if (n >= 0) { /* n==0: clear current filters and don't set new ones */
        clearrolefilter(RS_filter);
        for (i = 0; i < n; i++)
            setrolefilter(selected[i].item.a_string);

        ROLE = RACE = GEND = ALGN = ROLE_NONE;
    }
    if (selected)
        free((genericptr_t) selected), selected = 0;
    destroy_nhwindow(win);
    return (n > 0) ? TRUE : FALSE;
}

/* the change in format when this extended role selection was converted from
   tty-only to tty+curses+? made the role selection menu require two pages
   on a traditional 24-line tty; that wasn't fair to tty, so squeeze out
   some blank separator lines from the menu if that will make it fit on one */
staticfn int
maybe_skip_seps(int rows, int aspect)
{
    int i, n = 0;

    /* not much point to generalizing this to other aspects */
    if (aspect != RS_ROLE)
        return 0;
    /*
     * If there are one or two excess lines, setup_rolemenu() will omit
     * the separator between 'random' and 'pick race first'.  If there are
     * two, plsel_startmenu() will omit the one between role info so far
     * ("<role> <race> ...") and the set of role entries.
     */

    n += 4; /* title and ensuing separator, role info so far and separator */
    for (i = 0; roles[i].name.m; ++i)
        if (ok_role(i, RACE, GEND, ALGN) && ok_race(i, RACE, GEND, ALGN)
            && ok_gend(i, RACE, GEND, ALGN) && ok_align(i, RACE, GEND, ALGN))
            ++n;
    n += 2; /* 'random' and separator */
    n += 5; /* race 1st, gender 1st, alignment 1st, reset filter, quit */
    n += 1; /* footer/prompt */
    if (rows > 0 && n > rows)
        return n - rows;
    return 0;
}

/* start a menu; show role aspects specified so far as a header line */
staticfn winid
plsel_startmenu(int ttyrows, int aspect)
{
    char qbuf[QBUFSZ];
    winid win;
    const char *rolename;

    /* whatever aspect was just chosen might force others (Orc => chaotic,
       Samurai => Human+lawful, Valkyrie => female) */
    rigid_role_checks();

    rolename = (ROLE < 0) ? "<role>"
               : (GEND == 1 && roles[ROLE].name.f) ? roles[ROLE].name.f
                 : roles[ROLE].name.m;
    if (!svp.plname[0] || ROLE < 0 || RACE < 0 || GEND < 0 || ALGN < 0) {
        /* "<role> <race.noun> <gender> <alignment>" */
        Sprintf(qbuf, "%.20s %.20s %.20s %.20s",
                rolename,
                (RACE < 0) ? "<race>" : races[RACE].noun,
                (GEND < 0) ? "<gender>" : genders[GEND].adj,
                (ALGN < 0) ? "<alignment>" : aligns[ALGN].adj);
    } else {
        /* "<name> the <alignment> <gender> <race.adjective> <role>" */
        Sprintf(qbuf, "%.20s the %.20s %.20s %.20s %.20s",
                svp.plname,
                aligns[ALGN].adj,
                genders[GEND].adj,
                races[RACE].adj,
                rolename);
    }

    win = create_nhwindow(NHW_MENU);
    if (win == WIN_ERR)
        panic("could not create role selection window");
    start_menu(win, MENU_BEHAVE_STANDARD);

    add_menu_str(win, qbuf);
    if (maybe_skip_seps(ttyrows, aspect) != 2)
        add_menu_str(win, "");
    return win;
}

#undef ROLE
#undef RACE
#undef GEND
#undef ALGN

/*
 * setup_rolemenu() 
 *
 * Modified for RighteousHack to completely eliminate hotkey collisions
 * (especially important with both Priest and Prophet roles).
 *
 * Original NetHack 3.7 behavior: used the first letter of the male role name
 * → caused inevitable conflicts when two roles started with the same letter (P, H, etc.).
 *
 * New behavior (2025 patch):
 *   • First tries the lowercase first letter of the male role name
 *   • If already taken, automatically falls back to the second letter
 *   • If second letter is also taken, uses uppercase first letter
 *   • Tracks all used letters in a local array → guaranteed unique hotkeys
 *
 * Result: You can now safely use "Priest", "Prophet", "Pastor", "Presbyter", etc.
 * without ever worrying about menu conflicts — the function fixes it automatically.
 *
 * This is a permanent, future-proof solution for any number of added roles.
 */
staticfn void
setup_rolemenu(
    winid win,
    boolean filtering, /* True => exclude filtered roles; False => filter reset */
    int race, int gend, int algn)
{
    anything any;
    int i;
    boolean role_ok;
    char thisch, rolenamebuf[50];
    int clr = NO_COLOR;
    char used_letters[52] = {0};   /* a–z + A–Z */

    any = cg.zeroany;

    for (i = 0; roles[i].name.m; i++) {
        role_ok = (ok_role(i, race, gend, algn)
                   && ok_race(i, race, gend, algn)
                   && ok_gend(i, race, gend, algn)
                   && ok_align(i, race, gend, algn));
        if (filtering && !role_ok)
            continue;

        /* Build display name */
        Strcpy(rolenamebuf, roles[i].name.m);
        if (roles[i].name.f) {
            if (gend == 1) {
                Strcpy(rolenamebuf, roles[i].name.f);
            } else if (gend < 0) {
                Strcat(rolenamebuf, "/");
                Strcat(rolenamebuf, roles[i].name.f);
            }
        }

        /* === SMART HOTKEY LOGIC === */
        const char *p = roles[i].name.m;
        thisch = lowc(*p);

        /* If first letter already used, try second letter */
        if (used_letters[thisch - 'a']) {
            if (p[1]) {
                thisch = lowc(p[1]);
            } else {
                /* Very rare: name is 1 letter only – fall back to uppercase first */
                thisch = highc(thisch);
            }
        }

        /* If still conflict (e.g. "Prophet" and "Pastor" both want 'p' → 'r'), 
           force uppercase version */
        if (used_letters[thisch - 'a']) {
            thisch = highc(lowc(*p));
        }

        /* Final fallback: use uppercase if lowercase is taken */
        if (used_letters[thisch - 'a']) {
            thisch = highc(lowc(*p));
        }

        /* Mark this letter as used (both cases) */
        used_letters[lowc(thisch) - 'a'] = 1;

        /* Set menu item */
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = roles[i].name.m;

        add_menu(win, &nul_glyphinfo, &any, thisch, 0,
                 ATR_NONE, clr, an(rolenamebuf),
                 (!filtering && !role_ok) ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    }
}

staticfn void
setup_racemenu(
    winid win,
    boolean filtering,
    int role, int gend, int algn)
{
    anything any;
    boolean race_ok;
    int i;
    char this_ch;
    int clr = NO_COLOR;

    any = cg.zeroany;
    for (i = 0; races[i].noun; i++) {
        /* no ok_gend(); race isn't constrained by gender */
        race_ok = (ok_race(role, i, gend, algn)
                   && ok_role(role, i, gend, algn)
                   && ok_align(role, i, gend, algn));
        if (filtering && !race_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = races[i].noun;
        this_ch = *races[i].noun;
        /* filtering: picking race, so choose by first letter, with
           capital letter as unseen accelerator;
           !filtering: resetting filter rather than picking, choose by
           capital letter since lowercase role letters will be present */
        add_menu(win, &nul_glyphinfo, &any,
                 filtering ? this_ch : highc(this_ch),
                 filtering ? highc(this_ch) : 0,
                 ATR_NONE, clr, races[i].noun,
                 (!filtering && !race_ok)
                    ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    }
}

staticfn void
setup_gendmenu(
    winid win,
    boolean filtering,
    int role, int race, int algn)
{
    anything any;
    boolean gend_ok;
    int i;
    char this_ch;
    int clr = NO_COLOR;

    any = cg.zeroany;
    for (i = 0; i < ROLE_GENDERS; i++) {
        /* no ok_align(); gender isn't constrained by alignment */
        gend_ok = (ok_gend(role, race, i, algn)
                   && ok_role(role, race, i, algn)
                   && ok_race(role, race, i, algn));
        if (filtering && !gend_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = genders[i].adj;
        this_ch = *genders[i].adj;
        /* (see setup_racemenu for explanation of selector letters
           and setup_rolemenu for preselection) */
        add_menu(win, &nul_glyphinfo, &any,
                 filtering ? this_ch : highc(this_ch),
                 filtering ? highc(this_ch) : 0,
                 ATR_NONE, clr, genders[i].adj,
                 (!filtering && !gend_ok)
                    ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    }
}

staticfn void
setup_algnmenu(
    winid win,
    boolean filtering,
    int role, int race, int gend)
{
    anything any;
    boolean algn_ok;
    int i;
    char this_ch;
    int clr = NO_COLOR;

    any = cg.zeroany;
    for (i = 0; i < ROLE_ALIGNS; i++) {
        /* no ok_gend(); alignment isn't constrained by gender */
        algn_ok = (ok_align(role, race, gend, i)
                   && ok_role(role, race, gend, i)
                   && ok_race(role, race, gend, i));
        if (filtering && !algn_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = aligns[i].adj;
        this_ch = *aligns[i].adj;
        /* (see setup_racemenu for explanation of selector letters
           and setup_rolemenu for preselection) */
        add_menu(win, &nul_glyphinfo, &any,
                 filtering ? this_ch : highc(this_ch),
                 filtering ? highc(this_ch) : 0,
                 ATR_NONE, clr, aligns[i].adj,
                 (!filtering && !algn_ok)
                    ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    }
}

#else /* !TTY_GRAPHICS */

int
genl_player_setup(int screenheight UNUSED)
{
    return 0;
}

#endif /* ?TTY_GRAPHICS */

/* role.c */

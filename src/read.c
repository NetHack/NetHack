/* NetHack 3.7	read.c	$NHDT-Date: 1654931501 2022/06/11 07:11:41 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.257 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define Your_Own_Role(mndx)  ((mndx) == g.urole.mnum)
#define Your_Own_Race(mndx)  ((mndx) == g.urace.mnum)

static boolean learnscrolltyp(short);
static void cap_spe(struct obj *);
static char *erode_obj_text(struct obj *, char *);
static char *hawaiian_design(struct obj *, char *);
static int read_ok(struct obj *);
static void stripspe(struct obj *);
static void p_glow1(struct obj *);
static void p_glow2(struct obj *, const char *);
static void forget(int);
static int maybe_tame(struct monst *, struct obj *);
static boolean can_center_cloud(coordxy, coordxy);
static void display_stinking_cloud_positions(int);
static void seffect_enchant_armor(struct obj **);
static void seffect_destroy_armor(struct obj **);
static void seffect_confuse_monster(struct obj **);
static void seffect_scare_monster(struct obj **);
static void seffect_remove_curse(struct obj **);
static void seffect_create_monster(struct obj **);
static void seffect_enchant_weapon(struct obj **);
static void seffect_taming(struct obj **);
static void seffect_genocide(struct obj **);
static void seffect_light(struct obj **);
static void seffect_charging(struct obj **);
static void seffect_amnesia(struct obj **);
static void seffect_fire(struct obj **);
static void seffect_earth(struct obj **);
static void seffect_punishment(struct obj **);
static void seffect_stinking_cloud(struct obj **);
static void seffect_blank_paper(struct obj **);
static void seffect_teleportation(struct obj **);
static void seffect_gold_detection(struct obj **);
static void seffect_food_detection(struct obj **);
static void seffect_identify(struct obj **);
static void seffect_magic_mapping(struct obj **);
#ifdef MAIL_STRUCTURES
static void seffect_mail(struct obj **);
#endif /* MAIL_STRUCTURES */
static void set_lit(coordxy, coordxy, genericptr);
static void do_class_genocide(void);
static void do_stinking_cloud(struct obj *, boolean);
static boolean create_particular_parse(char *,
                                       struct _create_particular_data *);
static boolean create_particular_creation(struct _create_particular_data *);

static boolean
learnscrolltyp(short scrolltyp)
{
    if (!objects[scrolltyp].oc_name_known) {
        makeknown(scrolltyp);
        more_experienced(0, 10);
        return TRUE;
    } else
        return FALSE;
}

/* also called from teleport.c for scroll of teleportation */
void
learnscroll(struct obj* sobj)
{
    /* it's implied that sobj->dknown is set;
       we couldn't be reading this scroll otherwise */
    if (sobj->oclass != SPBOOK_CLASS)
        (void) learnscrolltyp(sobj->otyp);
}

/* max spe is +99, min is -99 */
static void
cap_spe(struct obj* obj)
{
    if (obj) {
        if (abs(obj->spe) > SPE_LIM)
            obj->spe = sgn(obj->spe) * SPE_LIM;
    }
}

static char *
erode_obj_text(struct obj* otmp, char* buf)
{
    int erosion = greatest_erosion(otmp);

    if (erosion)
        wipeout_text(buf, (int) (strlen(buf) * erosion / (2 * MAX_ERODE)),
                     otmp->o_id ^ (unsigned) ubirthday);
    return buf;
}

char *
tshirt_text(struct obj* tshirt, char* buf)
{
    static const char *const shirt_msgs[] = {
        /* Scott Bigham */
      "I explored the Dungeons of Doom and all I got was this lousy T-shirt!",
        "Is that Mjollnir in your pocket or are you just happy to see me?",
      "It's not the size of your sword, it's how #enhance'd you are with it.",
        "Madame Elvira's House O' Succubi Lifetime Customer",
        "Madame Elvira's House O' Succubi Employee of the Month",
        "Ludios Vault Guards Do It In Small, Dark Rooms",
        "Yendor Military Soldiers Do It In Large Groups",
        "I survived Yendor Military Boot Camp",
        "Ludios Accounting School Intra-Mural Lacrosse Team",
        "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
        "Hey, black dragon!  Disintegrate THIS!",
        "I'm With Stupid -->",
        "Don't blame me, I voted for Izchak!",
        "Don't Panic", /* HHGTTG */
        "Furinkan High School Athletic Dept.",                /* Ranma 1/2 */
        "Hel-LOOO, Nurse!",                                   /* Animaniacs */
        "=^.^=",
        "100% goblin hair - do not wash",
        "Aberzombie and Fitch",
        "cK -- Cockatrice touches the Kop",
        "Don't ask me, I only adventure here",
        "Down with pants!",
        "d, your dog or a killer?",
        "FREE PUG AND NEWT!",
        "Go team ant!",
        "Got newt?",
        "Hello, my darlings!", /* Charlie Drake */
        "Hey!  Nymphs!  Steal This T-Shirt!",
        "I <3 Dungeon of Doom",
        "I <3 Maud",
        /* note: there is a similarly worded apron (alchemy smock) slogan */
        "I am a Valkyrie.  If you see me running, try to keep up.",
        "I am not a pack rat - I am a collector",
        "I bounced off a rubber tree",         /* Monkey Island */
        "Plunder Island Brimstone Beach Club", /* Monkey Island */
        "If you can read this, I can hit you with my polearm",
        "I'm confused!",
        "I scored with the princess",
        "I want to live forever or die in the attempt.",
        "Lichen Park",
        "LOST IN THOUGHT - please send search party",
        "Meat is Mordor",
        "Minetown Better Business Bureau",
        "Minetown Watch",
        /* Discworld riff; unfortunately long */
 "Ms. Palm's House of Negotiable Affection--A Very Reputable House Of Disrepute",
        "Protection Racketeer",
        "Real men love Crom",
        "Somebody stole my Mojo!",
        "The Hellhound Gang",
        "The Werewolves",
        "They Might Be Storm Giants",
        "Weapons don't kill people, I kill people",
        "White Zombie",
        "You're killing me!",
        "Anhur State University - Home of the Fighting Fire Ants!",
        "FREE HUGS",
        "Serial Ascender",
        "Real men are valkyries",
        "Young Men's Cavedigging Association",
        "Occupy Fort Ludios",
        "I couldn't afford this T-shirt so I stole it!",
        "Mind flayers suck",
        "I'm not wearing any pants",
        "Down with the living!",
        "Pudding farmer",
        "Vegetarian",
        "Hello, I'm War!",
        "It is better to light a candle than to curse the darkness",
        "It is easier to curse the darkness than to light a candle",
        /* expanded "rock--paper--scissors" featured in TV show "Big Bang
           Theory" although they didn't create it (and an actual T-shirt
           with pentagonal diagram showing which choices defeat which) */
        "rock--paper--scissors--lizard--Spock!",
        /* "All men must die -- all men must serve" challange and response
           from book series _A_Song_of_Ice_and_Fire_ by George R.R. Martin,
           TV show "Game of Thrones" (probably an actual T-shirt too...) */
        "/Valar morghulis/ -- /Valar dohaeris/",
    };

    Strcpy(buf, shirt_msgs[tshirt->o_id % SIZE(shirt_msgs)]);
    return erode_obj_text(tshirt, buf);
}

char *
hawaiian_motif(struct obj *shirt, char *buf)
{
    static const char *const hawaiian_motifs[] = {
        /* birds */
        "flamingo",
        "parrot",
        "toucan",
        "bird of paradise", /* could be a bird or a flower */
        /* sea creatures */
        "sea turtle",
        "tropical fish",
        "jellyfish",
        "giant eel",
        "water nymph",
        /* plants */
        "plumeria",
        "orchid",
        "hibiscus flower",
        "palm tree",
        /* other */
        "hula dancer",
        "sailboat",
        "ukulele",
    };

    /* a tourist's starting shirt always has the same o_id; we need some
       additional randomness or else its design will never differ */
    unsigned motif = shirt->o_id ^ (unsigned) ubirthday;

    Strcpy(buf, hawaiian_motifs[motif % SIZE(hawaiian_motifs)]);
    return buf;
}

static char *
hawaiian_design(struct obj *shirt, char *buf)
{
    static const char *const hawaiian_bgs[] = {
        /* solid colors */
        "purple",
        "yellow",
        "red",
        "blue",
        "orange",
        "black",
        "green",
        /* adjectives */
        "abstract",
        "geometric",
        "patterned",
        "naturalistic",
    };

    /* This hash method is slightly different than the one in hawaiian_motif;
       using the same formula in both cases may lead to some shirt combos
       never appearing, if the sizes of the two lists have common factors. */
    unsigned bg = shirt->o_id ^ (unsigned) ~ubirthday;

    Sprintf(buf, "%s on %s background",
            makeplural(hawaiian_motif(shirt, buf)),
            an(hawaiian_bgs[bg % SIZE(hawaiian_bgs)]));
    return buf;
}

char *
apron_text(struct obj* apron, char* buf)
{
    static const char *const apron_msgs[] = {
        "Kiss the cook",
        "I'm making SCIENCE!",
        "Don't mess with the chef",
        "Don't make me poison you",
        "Gehennom's Kitchen",
        "Rat: The other white meat",
        "If you can't stand the heat, get out of Gehennom!",
        "If we weren't meant to eat animals, why are they made out of meat?",
        "If you don't like the food, I'll stab you",
        /* In the movie "The Sum of All Fears", a Russian worker in a weapons
           facility wears a T-shirt that a translator says reads, "I am a
           bomb technician, if you see me running ... try to catch up."
           In nethack, the quote is far more suitable to an alchemy smock
           (particularly since so many of these others are about cooking)
           than a T-shirt and is paraphrased to simplify/shorten it.
           [later... turns out that this is already a T-shirt message:
            "I am a Valkyrie.  If you see me running, try to keep up."
           so this one has been revised a little:  added alchemist prefix,
           changed "keep up" to original source's "catch up"] */
        "I am an alchemist; if you see me running, try to catch up...",
    };

    Strcpy(buf, apron_msgs[apron->o_id % SIZE(apron_msgs)]);
    return erode_obj_text(apron, buf);
}

static const char *const candy_wrappers[] = {
    "",                         /* (none -- should never happen) */
    "Apollo",                   /* Lost */
    "Moon Crunchy",             /* South Park */
    "Snacky Cake",    "Chocolate Nuggie", "The Small Bar",
    "Crispy Yum Yum", "Nilla Crunchie",   "Berry Bar",
    "Choco Nummer",   "Om-nom", /* Cat Macro */
    "Fruity Oaty",              /* Serenity */
    "Wonka Bar",                /* Charlie and the Chocolate Factory */
};

/* return the text of a candy bar's wrapper */
const char *
candy_wrapper_text(struct obj* obj)
{
    /* modulo operation is just bullet proofing; 'spe' is already in range */
    return candy_wrappers[obj->spe % SIZE(candy_wrappers)];
}

/* assign a wrapper to a candy bar stack */
void
assign_candy_wrapper(struct obj* obj)
{
    if (obj->otyp == CANDY_BAR) {
        /* skips candy_wrappers[0] */
        obj->spe = 1 + rn2(SIZE(candy_wrappers) - 1);
    }
    return;
}

/* getobj callback for object to read */
static int
read_ok(struct obj* obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS)
        return GETOBJ_SUGGEST;

    return GETOBJ_DOWNPLAY;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* the #read command; read a scroll or spell book or various other things */
int
doread(void)
{
    static const char find_any_braille[] = "feel any Braille writing.";
    register struct obj *scroll;
    boolean confused, nodisappear;
    int otyp;

    /*
     * Reading while blind is allowed in most cases, including the
     * Book of the Dead but not regular spellbooks.  For scrolls, the
     * description has to have been seen or magically learned (so only
     * when scroll->dknown is true):  hero recites the label while
     * holding the unfurled scroll.  We deliberately don't require
     * free hands because that would cripple scroll of remove curse,
     * but we ought to be requiring hands or at least limbs.  The
     * recitation could be sub-vocal; actual speech isn't required.
     *
     * Reading while confused is allowed and can produce alternate
     * outcome.
     *
     * Reading while stunned is currently allowed but probably should
     * be prevented....
     */

    g.known = FALSE;
    if (check_capacity((char *) 0))
        return ECMD_OK;

    scroll = getobj("read", read_ok, GETOBJ_PROMPT);
    if (!scroll)
        return ECMD_CANCEL;
    otyp = scroll->otyp;
    scroll->pickup_prev = 0; /* no longer 'just picked up' */

    /* outrumor has its own blindness check */
    if (otyp == FORTUNE_COOKIE) {
        if (Verbose(3, doread1))
            You("break up the cookie and throw away the pieces.");
        outrumor(bcsign(scroll), BY_COOKIE);
        if (!Blind)
            if (!u.uconduct.literate++)
                livelog_printf(LL_CONDUCT,
                               "became literate by reading a fortune cookie");
        useup(scroll);
        return ECMD_TIME;
    } else if (otyp == T_SHIRT || otyp == ALCHEMY_SMOCK
               || otyp == HAWAIIAN_SHIRT) {
        char buf[BUFSZ], *mesg;
        const char *endpunct;

        if (Blind) {
            You_cant(find_any_braille);
            return ECMD_OK;
        }
        /* can't read shirt worn under suit (under cloak is ok though) */
        if ((otyp == T_SHIRT || otyp == HAWAIIAN_SHIRT) && uarm
            && scroll == uarmu) {
            pline("%s shirt is obscured by %s%s.",
                  scroll->unpaid ? "That" : "Your", shk_your(buf, uarm),
                  suit_simple_name(uarm));
            return ECMD_OK;
        }
        if (otyp == HAWAIIAN_SHIRT) {
            pline("%s features %s.",
                  Verbose(3, doread2) ? "The design" : "It",
                  hawaiian_design(scroll, buf));
            return ECMD_TIME;
        }
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT, "became literate by reading %s",
                           (scroll->otyp == T_SHIRT) ? "a T-shirt"
                           : "an apron");

        /* populate 'buf[]' */
        mesg = (otyp == T_SHIRT) ? tshirt_text(scroll, buf)
                                 : apron_text(scroll, buf);
        endpunct = "";
        if (Verbose(3, doread3)) {
            int ln = (int) strlen(mesg);

            /* we will be displaying a sentence; need ending punctuation */
            if (ln > 0 && !index(".!?", mesg[ln - 1]))
                endpunct = ".";
            pline("It reads:");
        }
        pline("\"%s\"%s", mesg, endpunct);
        return ECMD_TIME;
    } else if ((otyp == DUNCE_CAP || otyp == CORNUTHAUM)
        /* note: "DUNCE" isn't directly connected to tourists but
           if everyone could read it, they would always be able to
           trivially distinguish between the two types of conical hat;
           limiting this to tourists is better than rejecting it */
               && Role_if(PM_TOURIST)) {
        /* another note: the misspelling, "wizzard", is correct;
           that's what is written on Rincewind's pointy hat from
           Pratchett's Discworld series, along with a lot of stars;
           rather than inked on or painted on, treat them as stitched
           or even separate pieces of fabric which have been attached
           (don't recall whether the books mention anything like that...) */
        const char *cap_text = (otyp == DUNCE_CAP) ? "DUNCE" : "WIZZARD";

        if (scroll->o_id % 3) {
            /* no need to vary this when blind; "on this ___" is important
               because it suggests that there might be something on others */
            You_cant("find anything to read on this %s.",
                     simpleonames(scroll));
            return ECMD_OK;
        }
        pline("%s on the %s.  It reads:  %s.",
              !Blind ? "There is writing" : "You feel lettering",
              simpleonames(scroll), cap_text);
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT, "became literate by reading %s",
                           (otyp == DUNCE_CAP) ? "a dunce cap"
                                               : "a cornuthaum");

        /* yet another note: despite the fact that player will recognize
           the object type, don't make it become a discovery for hero */
        trycall(scroll);
        return ECMD_TIME;
    } else if (otyp == CREDIT_CARD) {
        static const char *const card_msgs[] = {
            "Leprechaun Gold Tru$t - Shamrock Card",
            "Magic Memory Vault Charge Card",
            "Larn National Bank",                /* Larn */
            "First Bank of Omega",               /* Omega */
            "Bank of Zork - Frobozz Magic Card", /* Zork */
            "Ankh-Morpork Merchant's Guild Barter Card",
            "Ankh-Morpork Thieves' Guild Unlimited Transaction Card",
            "Ransmannsby Moneylenders Association",
            "Bank of Gehennom - 99% Interest Card",
            "Yendorian Express - Copper Card",
            "Yendorian Express - Silver Card",
            "Yendorian Express - Gold Card",
            "Yendorian Express - Mithril Card",
            "Yendorian Express - Platinum Card", /* must be last */
        };

        if (Blind) {
            You("feel the embossed numbers:");
        } else {
            if (Verbose(3, doread4))
                pline("It reads:");
            pline("\"%s\"",
                  scroll->oartifact
                      ? card_msgs[SIZE(card_msgs) - 1]
                      : card_msgs[scroll->o_id % (SIZE(card_msgs) - 1)]);
        }
        /* Make a credit card number */
        pline("\"%d0%d %ld%d1 0%d%d0\"%s",
              (((int) scroll->o_id % 89) + 10),
              ((int) scroll->o_id % 4),
              ((((long) scroll->o_id * 499L) % 899999L) + 100000L),
              ((int) scroll->o_id % 10),
              (!((int) scroll->o_id % 3)),
              (((int) scroll->o_id * 7) % 10),
              (Verbose(3, doread5) || Blind) ? "." : "");
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT,
                           "became literate by reading a credit card");

        return ECMD_TIME;
    } else if (otyp == CAN_OF_GREASE) {
        pline("This %s has no label.", singular(scroll, xname));
        return ECMD_OK;
    } else if (otyp == MAGIC_MARKER) {
        char buf[BUFSZ];
        const int red_mons[] = { PM_FIRE_ANT, PM_PYROLISK, PM_HELL_HOUND,
            PM_IMP, PM_LARGE_MIMIC, PM_LEOCROTTA, PM_SCORPION, PM_XAN,
            PM_GIANT_BAT, PM_WATER_MOCCASIN, PM_FLESH_GOLEM, PM_BARBED_DEVIL,
            PM_MARILITH, PM_PIRANHA };
        struct permonst *pm = &mons[red_mons[scroll->o_id % SIZE(red_mons)]];

        if (Blind) {
            You_cant(find_any_braille);
            return ECMD_OK;
        }
        if (Verbose(3, doread6))
            pline("It reads:");
        Sprintf(buf, "%s", pmname(pm, NEUTRAL));
        pline("\"Magic Marker(TM) %s Red Ink Marker Pen.  Water Soluble.\"",
              upwords(buf));
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT,
                           "became literate by reading a magic marker");

        return ECMD_TIME;
    } else if (scroll->oclass == COIN_CLASS) {
        if (Blind)
            You("feel the embossed words:");
        else if (Verbose(3, doread7))
            You("read:");
        pline("\"1 Zorkmid.  857 GUE.  In Frobs We Trust.\"");
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT,
                           "became literate by reading a coin's engravings");

        return ECMD_TIME;
    } else if (is_art(scroll, ART_ORB_OF_FATE)) {
        if (Blind)
            You("feel the engraved signature:");
        else
            pline("It is signed:");
        pline("\"Odin.\"");
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT,
                   "became literate by reading the divine signature of Odin");

        return ECMD_TIME;
    } else if (otyp == CANDY_BAR) {
        const char *wrapper = candy_wrapper_text(scroll);

        if (Blind) {
            You_cant(find_any_braille);
            return ECMD_OK;
        }
        if (!*wrapper) {
            pline("The candy bar's wrapper is blank.");
            return ECMD_OK;
        }
        pline("The wrapper reads: \"%s\".", wrapper);
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT,
                           "became literate by reading a candy bar wrapper");

        return ECMD_TIME;
    } else if (scroll->oclass != SCROLL_CLASS
               && scroll->oclass != SPBOOK_CLASS) {
        pline(silly_thing_to, "read");
        return ECMD_OK;
    } else if (Blind && otyp != SPE_BOOK_OF_THE_DEAD) {
        const char *what = 0;

        if (otyp == SPE_NOVEL)
            /* unseen novels are already distinguishable from unseen
               spellbooks so this isn't revealing any extra information */
            what = "words";
        else if (scroll->oclass == SPBOOK_CLASS)
            what = "mystic runes";
        else if (!scroll->dknown)
            what = "formula on the scroll";
        if (what) {
            pline("Being blind, you cannot read the %s.", what);
            return ECMD_OK;
        }
    }

    confused = (Confusion != 0);
#ifdef MAIL_STRUCTURES
    if (otyp == SCR_MAIL) {
        confused = FALSE; /* override */
        /* reading mail is a convenience for the player and takes
           place outside the game, so shouldn't affect gameplay;
           on the other hand, it starts by explicitly making the
           hero actively read something, which is pretty hard
           to simply ignore; as a compromise, if the player has
           maintained illiterate conduct so far, and this mail
           scroll didn't come from bones, ask for confirmation */
        if (!u.uconduct.literate) {
            if (!scroll->spe && yn(
             "Reading mail will violate \"illiterate\" conduct.  Read anyway?"
                                   ) != 'y')
                return ECMD_OK;
        }
    }
#endif

    /* Actions required to win the game aren't counted towards conduct */
    /* Novel conduct is handled in read_tribute so exclude it too */
    if (otyp != SPE_BOOK_OF_THE_DEAD && otyp != SPE_NOVEL
        && otyp != SPE_BLANK_PAPER && otyp != SCR_BLANK_PAPER)
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT, "became literate by reading %s",
                           (scroll->oclass == SPBOOK_CLASS) ? "a book"
                           : (scroll->oclass == SCROLL_CLASS) ? "a scroll"
                             : something);

    if (scroll->oclass == SPBOOK_CLASS) {
        return study_book(scroll) ? ECMD_TIME : ECMD_OK;
    }
    scroll->in_use = TRUE; /* scroll, not spellbook, now being read */
    if (otyp != SCR_BLANK_PAPER) {
        boolean silently = !can_chant(&g.youmonst);

        /* a few scroll feedback messages describe something happening
           to the scroll itself, so avoid "it disappears" for those */
        nodisappear = (otyp == SCR_FIRE
                       || (otyp == SCR_REMOVE_CURSE && scroll->cursed));
        if (Blind)
            pline(nodisappear
                      ? "You %s the formula on the scroll."
                      : "As you %s the formula on it, the scroll disappears.",
                  silently ? "cogitate" : "pronounce");
        else
            pline(nodisappear ? "You read the scroll."
                              : "As you read the scroll, it disappears.");
        if (confused) {
            if (Hallucination)
                pline("Being so trippy, you screw up...");
            else
                pline("Being confused, you %s the magic words...",
                      silently ? "misunderstand" : "mispronounce");
        }
    }
    if (!seffects(scroll)) {
        if (!objects[otyp].oc_name_known) {
            if (g.known)
                learnscroll(scroll);
            else
                trycall(scroll);
        }
        scroll->in_use = FALSE;
        if (otyp != SCR_BLANK_PAPER)
            useup(scroll);
    }
    return ECMD_TIME;
}

RESTORE_WARNING_FORMAT_NONLITERAL

static void
stripspe(register struct obj* obj)
{
    if (obj->blessed || obj->spe <= 0) {
        pline1(nothing_happens);
    } else {
        /* order matters: message, shop handling, actual transformation */
        pline("%s briefly.", Yobjnam2(obj, "vibrate"));
        costly_alteration(obj, COST_UNCHRG);
        obj->spe = 0;
        if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
            obj->age = 0;
    }
}

static void
p_glow1(register struct obj* otmp)
{
    pline("%s briefly.", Yobjnam2(otmp, Blind ? "vibrate" : "glow"));
}

static void
p_glow2(register struct obj* otmp, register const char* color)
{
    pline("%s%s%s for a moment.", Yobjnam2(otmp, Blind ? "vibrate" : "glow"),
          Blind ? "" : " ", Blind ? "" : hcolor(color));
}

/* getobj callback for object to charge */
int
charge_ok(struct obj* obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    if (obj->oclass == WAND_CLASS)
        return GETOBJ_SUGGEST;

    if (obj->oclass == RING_CLASS && objects[obj->otyp].oc_charged
        && obj->dknown && objects[obj->otyp].oc_name_known)
        return GETOBJ_SUGGEST;

    if (is_weptool(obj)) /* specific check before general tools */
        return GETOBJ_EXCLUDE;

    if (obj->oclass == TOOL_CLASS) {
        /* suggest tools that aren't oc_charged but can still be recharged */
        if (obj->otyp == BRASS_LANTERN
            || (obj->otyp == OIL_LAMP)
            /* only list magic lamps if they are not identified yet */
            || (obj->otyp == MAGIC_LAMP
                && !objects[MAGIC_LAMP].oc_name_known)) {
            return GETOBJ_SUGGEST;
        }
        return objects[obj->otyp].oc_charged ? GETOBJ_SUGGEST : GETOBJ_EXCLUDE;
    }
    /* why are weapons/armor considered charged anyway?
     * make them selectable even so for "feeling of loss" message */
    return GETOBJ_EXCLUDE_SELECTABLE;
}

/* recharge an object; curse_bless is -1 if the recharging implement
   was cursed, +1 if blessed, 0 otherwise. */
void
recharge(struct obj* obj, int curse_bless)
{
    register int n;
    boolean is_cursed, is_blessed;

    is_cursed = curse_bless < 0;
    is_blessed = curse_bless > 0;

    if (obj->oclass == WAND_CLASS) {
        int lim = (obj->otyp == WAN_WISHING)
                      ? 3
                      : (objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;

        /* undo any prior cancellation, even when is_cursed */
        if (obj->spe == -1)
            obj->spe = 0;

        /*
         * Recharging might cause wands to explode.
         *      v = number of previous recharges
         *            v = percentage chance to explode on this attempt
         *                    v = cumulative odds for exploding
         *      0 :   0       0
         *      1 :   0.29    0.29
         *      2 :   2.33    2.62
         *      3 :   7.87   10.28
         *      4 :  18.66   27.02
         *      5 :  36.44   53.62
         *      6 :  62.97   82.83
         *      7 : 100     100
         */
        n = (int) obj->recharged;
        if (n > 0 && (obj->otyp == WAN_WISHING
                      || (n * n * n > rn2(7 * 7 * 7)))) { /* recharge_limit */
            wand_explode(obj, rnd(lim));
            return;
        }
        /* didn't explode, so increment the recharge count */
        obj->recharged = (unsigned) (n + 1);

        /* now handle the actual recharging */
        if (is_cursed) {
            stripspe(obj);
        } else {
            n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
            if (!is_blessed)
                n = rnd(n);

            if (obj->spe < n)
                obj->spe = n;
            else
                obj->spe++;
            if (obj->otyp == WAN_WISHING && obj->spe > 3) {
                wand_explode(obj, 1);
                return;
            }
            if (obj->spe >= lim)
                p_glow2(obj, NH_BLUE);
            else
                p_glow1(obj);
#if 0 /*[shop price doesn't vary by charge count]*/
            /* update shop bill to reflect new higher price */
            if (obj->unpaid)
                alter_cost(obj, 0L);
#endif
        }

    } else if (obj->oclass == RING_CLASS && objects[obj->otyp].oc_charged) {
        /* charging does not affect ring's curse/bless status */
        int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
        boolean is_on = (obj == uleft || obj == uright);

        /* destruction depends on current state, not adjustment */
        if (obj->spe > rn2(7) || obj->spe <= -5) {
            pline("%s momentarily, then %s!", Yobjnam2(obj, "pulsate"),
                  otense(obj, "explode"));
            if (is_on)
                Ring_gone(obj);
            s = rnd(3 * abs(obj->spe)); /* amount of damage */
            useup(obj), obj = 0;
            losehp(Maybe_Half_Phys(s), "exploding ring", KILLED_BY_AN);
        } else {
            long mask = is_on ? (obj == uleft ? LEFT_RING : RIGHT_RING) : 0L;

            pline("%s spins %sclockwise for a moment.", Yname2(obj),
                  s < 0 ? "counter" : "");
            if (s < 0)
                costly_alteration(obj, COST_DECHNT);
            /* cause attributes and/or properties to be updated */
            if (is_on)
                Ring_off(obj);
            obj->spe += s; /* update the ring while it's off */
            if (is_on)
                setworn(obj, mask), Ring_on(obj);
            /* oartifact: if a touch-sensitive artifact ring is
               ever created the above will need to be revised  */
            /* update shop bill to reflect new higher price */
            if (s > 0 && obj->unpaid)
                alter_cost(obj, 0L);
        }

    } else if (obj->oclass == TOOL_CLASS) {
        int rechrg = (int) obj->recharged;

        if (objects[obj->otyp].oc_charged) {
            /* tools don't have a limit, but the counter used does */
            if (rechrg < 7) /* recharge_limit */
                obj->recharged++;
        }
        switch (obj->otyp) {
        case BELL_OF_OPENING:
            if (is_cursed)
                stripspe(obj);
            else if (is_blessed)
                obj->spe += rnd(3);
            else
                obj->spe += 1;
            if (obj->spe > 5)
                obj->spe = 5;
            break;
        case MAGIC_MARKER:
        case TINNING_KIT:
        case EXPENSIVE_CAMERA:
            if (is_cursed) {
                stripspe(obj);
            } else if (rechrg && obj->otyp == MAGIC_MARKER) {
                /* previously recharged */
                obj->recharged = 1; /* override increment done above */
                if (obj->spe < 3)
                    Your("marker seems permanently dried out.");
                else
                    pline1(nothing_happens);
            } else if (is_blessed) {
                n = rn1(16, 15); /* 15..30 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else if (obj->spe + n <= 75)
                    obj->spe = 75;
                else {
                    int chrg = (int) obj->spe;
                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(obj, NH_BLUE);
            } else {
                n = rn1(11, 10); /* 10..20 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else {
                    int chrg = (int) obj->spe;

                    if (chrg + n > SPE_LIM)
                        obj->spe = SPE_LIM;
                    else
                        obj->spe += n;
                }
                p_glow2(obj, NH_WHITE);
            }
            break;
        case OIL_LAMP:
        case BRASS_LANTERN:
            if (is_cursed) {
                stripspe(obj);
                if (obj->lamplit) {
                    if (!Blind)
                        pline("%s out!", Tobjnam(obj, "go"));
                    end_burn(obj, TRUE);
                }
            } else if (is_blessed) {
                obj->spe = 1;
                obj->age = 1500;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe = 1;
                obj->age += 750;
                if (obj->age > 1500)
                    obj->age = 1500;
                p_glow1(obj);
            }
            break;
        case CRYSTAL_BALL:
            if (obj->spe == -1) /* like wands, first uncancel */
                obj->spe = 0;

            if (is_cursed) {
                /* cursed scroll removes charges and curses ball */
                /*stripspe(obj); -- doesn't do quite what we want...*/
                if (!obj->cursed) {
                    p_glow2(obj, NH_BLACK);
                    curse(obj);
                } else {
                    pline("%s briefly.", Yobjnam2(obj, "vibrate"));
                }
                if (obj->spe > 0)
                    costly_alteration(obj, COST_UNCHRG);
                obj->spe = 0;
            } else if (is_blessed) {
                /* blessed scroll sets charges to max and blesses ball */
                obj->spe = 7;
                p_glow2(obj, !obj->blessed ? NH_LIGHT_BLUE : NH_BLUE);
                if (!obj->blessed)
                    bless(obj);
                /* [shop price stays the same regardless of charges or BUC] */
            } else {
                /* uncursed scroll increments charges and uncurses ball */
                if (obj->spe < 7 || obj->cursed) {
                    n = rnd(2);
                    obj->spe = min(obj->spe + n, 7);
                    if (!obj->cursed) {
                        p_glow1(obj);
                    } else {
                        p_glow2(obj, NH_AMBER);
                        uncurse(obj);
                    }
                } else {
                    /* charges at max and ball not being uncursed */
                    pline1(nothing_happens);
                }
            }
            break;
        case HORN_OF_PLENTY:
        case BAG_OF_TRICKS:
        case CAN_OF_GREASE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                if (obj->spe <= 10)
                    obj->spe += rn1(10, 6);
                else
                    obj->spe += rn1(5, 6);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe += rn1(5, 2);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow1(obj);
            }
            break;
        case MAGIC_FLUTE:
        case MAGIC_HARP:
        case FROST_HORN:
        case FIRE_HORN:
        case DRUM_OF_EARTHQUAKE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                obj->spe += d(2, 4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe += rnd(4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow1(obj);
            }
            break;
        default:
            goto not_chargable;
            /*NOTREACHED*/
            break;
        } /* switch */

    } else {
 not_chargable:
        You("have a feeling of loss.");
    }

    /* prevent enchantment from getting out of range */
    cap_spe(obj);
}

/*
 * Forget some things (e.g. after reading a scroll of amnesia).  When called,
 * the following are always forgotten:
 *      - felt ball & chain
 *      - skill training
 *
 * Other things are subject to flags:
 *      howmuch & ALL_SPELLS    = forget all spells
 */
static void
forget(int howmuch)
{
    if (Punished)
        u.bc_felt = 0; /* forget felt ball&chain */

    if (howmuch & ALL_SPELLS)
        losespells();

    /* Forget some skills. */
    drain_weapon_skill(rnd(howmuch ? 5 : 3));
}

/* monster is hit by scroll of taming's effect */
static int
maybe_tame(struct monst *mtmp, struct obj *sobj)
{
    int was_tame = mtmp->mtame;
    unsigned was_peaceful = mtmp->mpeaceful;

    if (sobj->cursed) {
        setmangry(mtmp, FALSE);
        if (was_peaceful && !mtmp->mpeaceful)
            return -1;
    } else {
        /* for a shopkeeper, tamedog() will call make_happy_shk() but
           not tame the target, so call it even if taming gets resisted */
        if (!resist(mtmp, sobj->oclass, 0, NOTELL) || mtmp->isshk)
            (void) tamedog(mtmp, (struct obj *) 0);
        if ((!was_peaceful && mtmp->mpeaceful) || (!was_tame && mtmp->mtame))
            return 1;
    }
    return 0;
}

/* Can a stinking cloud physically exist at a certain position?
 * NOT the same thing as can_center_cloud.
 */
boolean
valid_cloud_pos(coordxy x, coordxy y)
{
    if (!isok(x,y))
        return FALSE;
    return ACCESSIBLE(levl[x][y].typ) || is_pool(x, y) || is_lava(x, y);
}

/* Callback for getpos_sethilite, also used in determining whether a scroll
 * should have its regular effects, or not because it was out of range.
 */
static boolean
can_center_cloud(coordxy x, coordxy y)
{
    if (!valid_cloud_pos(x, y))
        return FALSE;
    return (cansee(x, y) && distu(x, y) < 32);
}

static void
display_stinking_cloud_positions(int state)
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        coordxy x, y, dx, dy;
        int dist = 6;

        for (dx = -dist; dx <= dist; dx++)
            for (dy = -dist; dy <= dist; dy++) {
                x = u.ux + dx;
                y = u.uy + dy;
                if (can_center_cloud(x,y))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

static void
seffect_enchant_armor(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    register schar s;
    boolean special_armor;
    boolean same_color;
    struct obj *otmp = some_armor(&g.youmonst);
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    boolean old_erodeproof, new_erodeproof;

    if (!otmp) {
        strange_feeling(sobj, !Blind
                        ? "Your skin glows then fades."
                        : "Your skin feels warm for a moment.");
        *sobjp = 0; /* useup() in strange_feeling() */
        exercise(A_CON, !scursed);
        exercise(A_STR, !scursed);
        return;
    }
    if (confused) {
        old_erodeproof = (otmp->oerodeproof != 0);
        new_erodeproof = !scursed;
        otmp->oerodeproof = 0; /* for messages */
        if (Blind) {
            otmp->rknown = FALSE;
            pline("%s warm for a moment.", Yobjnam2(otmp, "feel"));
        } else {
            otmp->rknown = TRUE;
            pline("%s covered by a %s %s %s!", Yobjnam2(otmp, "are"),
                  scursed ? "mottled" : "shimmering",
                  hcolor(scursed ? NH_BLACK : NH_GOLDEN),
                  scursed ? "glow"
                  : (is_shield(otmp) ? "layer" : "shield"));
        }
        if (new_erodeproof && (otmp->oeroded || otmp->oeroded2)) {
            otmp->oeroded = otmp->oeroded2 = 0;
            pline("%s as good as new!",
                  Yobjnam2(otmp, Blind ? "feel" : "look"));
        }
        if (old_erodeproof && !new_erodeproof) {
            /* restore old_erodeproof before shop charges */
            otmp->oerodeproof = 1;
            costly_alteration(otmp, COST_DEGRD);
        }
        otmp->oerodeproof = new_erodeproof ? 1 : 0;
        return;
    }
    /* elven armor vibrates warningly when enchanted beyond a limit */
    special_armor = is_elven_armor(otmp)
        || (Role_if(PM_WIZARD) && otmp->otyp == CORNUTHAUM);
    if (scursed)
        same_color = (otmp->otyp == BLACK_DRAGON_SCALE_MAIL
                      || otmp->otyp == BLACK_DRAGON_SCALES);
    else
        same_color = (otmp->otyp == SILVER_DRAGON_SCALE_MAIL
                      || otmp->otyp == SILVER_DRAGON_SCALES
                      || otmp->otyp == SHIELD_OF_REFLECTION);
    if (Blind)
        same_color = FALSE;

    /* KMH -- catch underflow */
    s = scursed ? -otmp->spe : otmp->spe;
    if (s > (special_armor ? 5 : 3) && rn2(s)) {
        otmp->in_use = TRUE;
        pline("%s violently %s%s%s for a while, then %s.", Yname2(otmp),
              otense(otmp, Blind ? "vibrate" : "glow"),
              (!Blind && !same_color) ? " " : "",
              (Blind || same_color) ? "" : hcolor(scursed ? NH_BLACK
                                                  : NH_SILVER),
              otense(otmp, "evaporate"));
        remove_worn_item(otmp, FALSE);
        useup(otmp);
        return;
    }
    s = scursed ? -1
        : (otmp->spe >= 9)
        ? (rn2(otmp->spe) == 0)
        : sblessed
        ? rnd(3 - otmp->spe / 3)
        : 1;
    if (s >= 0 && Is_dragon_scales(otmp)) {
        unsigned was_lit = otmp->lamplit;
        int old_light = artifact_light(otmp) ? arti_light_radius(otmp) : 0;

        /* dragon scales get turned into dragon scale mail */
        pline("%s merges and hardens!", Yname2(otmp));
        setworn((struct obj *) 0, W_ARM);
        /* assumes same order */
        otmp->otyp += GRAY_DRAGON_SCALE_MAIL - GRAY_DRAGON_SCALES;
        otmp->lamplit = 0; /* don't want bless() or uncurse() to adjust
                            * light radius because scales -> scale_mail will
                            * result in a second increase with own message */
        if (sblessed) {
            otmp->spe++;
            cap_spe(otmp);
            if (!otmp->blessed)
                bless(otmp);
        } else if (otmp->cursed)
            uncurse(otmp);
        otmp->known = 1;
        setworn(otmp, W_ARM);
        if (otmp->unpaid)
            alter_cost(otmp, 0L); /* shop bill */
        otmp->lamplit = was_lit;
        if (old_light)
            maybe_adjust_light(otmp, old_light);
        return;
    }
    pline("%s %s%s%s%s for a %s.", Yname2(otmp),
          (s == 0) ? "violently " : "",
          otense(otmp, Blind ? "vibrate" : "glow"),
          (!Blind && !same_color) ? " " : "",
          (Blind || same_color)
          ? "" : hcolor(scursed ? NH_BLACK : NH_SILVER),
          (s * s > 1) ? "while" : "moment");
    /* [this cost handling will need updating if shop pricing is
       ever changed to care about curse/bless status of armor] */
    if (s < 0)
        costly_alteration(otmp, COST_DECHNT);
    if (scursed && !otmp->cursed)
        curse(otmp);
    else if (sblessed && !otmp->blessed)
        bless(otmp);
    else if (!scursed && otmp->cursed)
        uncurse(otmp);
    if (s) {
        int oldspe = otmp->spe;
        /* despite being schar, it shouldn't be possible for spe to wrap
           here because it has been capped at 99 and s is quite small;
           however, might need to change s if it takes spe past 99 */
        otmp->spe += s;
        cap_spe(otmp); /* make sure that it doesn't exceed SPE_LIM */
        s = otmp->spe - oldspe; /* cap_spe() might have throttled 's' */
        if (s) /* skip if it got changed to 0 */
            adj_abon(otmp, s); /* adjust armor bonus for Dex or Int+Wis */
        g.known = otmp->known;
        /* update shop bill to reflect new higher price */
        if (s > 0 && otmp->unpaid)
            alter_cost(otmp, 0L);
    }

    if ((otmp->spe > (special_armor ? 5 : 3))
        && (special_armor || !rn2(7)))
        pline("%s %s.", Yobjnam2(otmp, "suddenly vibrate"),
              Blind ? "again" : "unexpectedly");
}

static void
seffect_destroy_armor(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    struct obj *otmp = some_armor(&g.youmonst);
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    boolean old_erodeproof, new_erodeproof;

    if (confused) {
        if (!otmp) {
            strange_feeling(sobj, "Your bones itch.");
            *sobjp = 0; /* useup() in strange_feeling() */
            exercise(A_STR, FALSE);
            exercise(A_CON, FALSE);
            return;
        }
        old_erodeproof = (otmp->oerodeproof != 0);
        new_erodeproof = scursed;
        otmp->oerodeproof = 0; /* for messages */
        p_glow2(otmp, NH_PURPLE);
        if (old_erodeproof && !new_erodeproof) {
            /* restore old_erodeproof before shop charges */
            otmp->oerodeproof = 1;
            costly_alteration(otmp, COST_DEGRD);
        }
        otmp->oerodeproof = new_erodeproof ? 1 : 0;
        return;
    }
    if (!scursed || !otmp || !otmp->cursed) {
        if (!destroy_arm(otmp)) {
            strange_feeling(sobj, "Your skin itches.");
            *sobjp = 0; /* useup() in strange_feeling() */
            exercise(A_STR, FALSE);
            exercise(A_CON, FALSE);
            return;
        } else
            g.known = TRUE;
    } else { /* armor and scroll both cursed */
        pline("%s.", Yobjnam2(otmp, "vibrate"));
        if (otmp->spe >= -6) {
            otmp->spe += -1;
            adj_abon(otmp, -1);
        }
        make_stunned((HStun & TIMEOUT) + (long) rn1(10, 10), TRUE);
    }
}

static void
seffect_confuse_monster(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed,
            scursed = sobj->cursed,
            confused = (Confusion != 0),
            altfeedback = (Blind || Invisible);

    if (g.youmonst.data->mlet != S_HUMAN || scursed) {
        if (!HConfusion)
            You_feel("confused.");
        make_confused(HConfusion + rnd(100), FALSE);
    } else if (confused) {
        if (!sblessed) {
            Your("%s begin to %s%s.", makeplural(body_part(HAND)),
                 altfeedback ? "tingle" : "glow ",
                 altfeedback ? "" : hcolor(NH_PURPLE));
            make_confused(HConfusion + rnd(100), FALSE);
        } else {
            pline("A %s%s surrounds your %s.",
                  altfeedback ? "" : hcolor(NH_RED),
                  altfeedback ? "faint buzz" : " glow", body_part(HEAD));
            make_confused(0L, TRUE);
        }
    } else {
        int incr = 0;

        if (!sblessed) {
            Your("%s%s %s%s.", makeplural(body_part(HAND)),
                 altfeedback ? "" : " begin to glow",
                 altfeedback ? (const char *) "tingle" : hcolor(NH_RED),
                 u.umconf ? " even more" : "");
            incr = rnd(2);
        } else {
            if (altfeedback)
                Your("%s tingle %s sharply.", makeplural(body_part(HAND)),
                     u.umconf ? "even more" : "very");
            else
                Your("%s glow %s brilliant %s.",
                     makeplural(body_part(HAND)),
                     u.umconf ? "an even more" : "a", hcolor(NH_RED));
            incr = rn1(8, 2);
        }
        /* after a while, repeated uses become less effective */
        if (u.umconf >= 40)
            incr = 1;
        u.umconf += (unsigned) incr;
    }
}

static void
seffect_scare_monster(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    int otyp = sobj->otyp;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    register int ct = 0;
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (cansee(mtmp->mx, mtmp->my)) {
            if (confused || scursed) {
                mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
                mtmp->mcanmove = 1;
            } else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
                monflee(mtmp, 0, FALSE, FALSE);
            if (!mtmp->mtame)
                ct++; /* pets don't laugh at you */
        }
    }
    if (otyp == SCR_SCARE_MONSTER || !ct)
        You_hear("%s %s.", (confused || scursed) ? "sad wailing"
                 : "maniacal laughter",
                 !ct ? "in the distance" : "close by");
}

static void
seffect_remove_curse(struct obj **sobjp)
{
    struct obj *sobj = *sobjp; /* scroll or fake spellbook */
    int otyp = sobj->otyp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    register struct obj *obj, *nxto;
    long wornmask;

    You_feel(!Hallucination
             ? (!confused ? "like someone is helping you."
                : "like you need some help.")
             : (!confused ? "in touch with the Universal Oneness."
                : "the power of the Force against you!"));

    if (scursed) {
        pline_The("scroll disintegrates.");
    } else {
        /* 3.7: this used to use a straight
               for (obj = invent; obj; obj = obj->nobj) {}
           traversal, but for the confused case, secondary weapon might
           become cursed and be dropped, moving it from the invent chain
           to the floor chain at hero's spot, so we have to remember the
           next object prior to processing the current one */
        for (obj = g.invent; obj; obj = nxto) {
            nxto = obj->nobj;
            /* gold isn't subject to cursing and blessing */
            if (obj->oclass == COIN_CLASS)
                continue;
            /* hide current scroll from itself so that perm_invent won't
               show known blessed scroll losing bknown when confused */
            if (obj == sobj && obj->quan == 1L)
                continue;
            wornmask = (obj->owornmask & ~(W_BALL | W_ART | W_ARTI));
            if (wornmask && !sblessed) {
                /* handle a couple of special cases; we don't
                   allow auxiliary weapon slots to be used to
                   artificially increase number of worn items */
                if (obj == uswapwep) {
                    if (!u.twoweap)
                        wornmask = 0L;
                } else if (obj == uquiver) {
                    if (obj->oclass == WEAPON_CLASS) {
                        /* mergeable weapon test covers ammo,
                           missiles, spears, daggers & knives */
                        if (!objects[obj->otyp].oc_merge)
                            wornmask = 0L;
                    } else if (obj->oclass == GEM_CLASS) {
                        /* possibly ought to check whether
                           alternate weapon is a sling... */
                        if (!uslinging())
                            wornmask = 0L;
                    } else {
                        /* weptools don't merge and aren't
                           reasonable quivered weapons */
                        wornmask = 0L;
                    }
                }
            }
            if (sblessed || wornmask || obj->otyp == LOADSTONE
                || (obj->otyp == LEASH && obj->leashmon)) {
                /* water price varies by curse/bless status */
                boolean shop_h2o = (obj->unpaid && obj->otyp == POT_WATER);

                if (confused) {
                    blessorcurse(obj, 2);
                    /* lose knowledge of this object's curse/bless
                       state (even if it didn't actually change) */
                    obj->bknown = 0;
                    /* blessorcurse() only affects uncursed items
                       so no need to worry about price of water
                       going down (hence no costly_alteration) */
                    if (shop_h2o && (obj->cursed || obj->blessed))
                        alter_cost(obj, 0L); /* price goes up */
                } else if (obj->cursed) {
                    if (shop_h2o)
                        costly_alteration(obj, COST_UNCURS);
                    uncurse(obj);
                    /* if the object was known to be cursed and is now
                       known not to be, make the scroll known; it's
                       trivial to identify anyway by comparing inventory
                       before and after */
                    if (obj->bknown && otyp == SCR_REMOVE_CURSE)
                        learnscrolltyp(SCR_REMOVE_CURSE);
                }
            }
        }
        /* if riding, treat steed's saddle as if part of hero's invent */
        if (u.usteed && (obj = which_armor(u.usteed, W_SADDLE)) != 0) {
            if (confused) {
                blessorcurse(obj, 2);
                obj->bknown = 0; /* skip set_bknown() */
            } else if (obj->cursed) {
                uncurse(obj);
                /* like rndcurse(sit.c), effect on regular inventory
                   doesn't show things glowing but saddle does */
                if (!Blind) {
                    pline("%s %s.", Yobjnam2(obj, "glow"),
                              hcolor("amber"));
                    obj->bknown = Hallucination ? 0 : 1;
                } else {
                    obj->bknown = 0; /* skip set_bknown() */
                }
            }
        }
    }
    if (Punished && !confused)
        unpunish();
    if (u.utrap && u.utraptype == TT_BURIEDBALL) {
        buried_ball_to_freedom();
        pline_The("clasp on your %s vanishes.", body_part(LEG));
    }
    update_inventory();
}

static void
seffect_create_monster(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);

    if (create_critters(1 + ((confused || scursed) ? 12 : 0)
                        + ((sblessed || rn2(73)) ? 0 : rnd(4)),
                        confused ? &mons[PM_ACID_BLOB]
                        : (struct permonst *) 0,
                        FALSE))
        g.known = TRUE;
    /* no need to flush monsters; we ask for identification only if the
     * monsters are not visible
     */
}

static void
seffect_enchant_weapon(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    boolean old_erodeproof, new_erodeproof;

    /* [What about twoweapon mode?  Proofing/repairing/enchanting both
       would be too powerful, but shouldn't we choose randomly between
       primary and secondary instead of always acting on primary?] */
    if (confused && uwep
        && erosion_matters(uwep) && uwep->oclass != ARMOR_CLASS) {
        old_erodeproof = (uwep->oerodeproof != 0);
        new_erodeproof = !scursed;
        uwep->oerodeproof = 0; /* for messages */
        if (Blind) {
            uwep->rknown = FALSE;
            Your("weapon feels warm for a moment.");
        } else {
            uwep->rknown = TRUE;
            pline("%s covered by a %s %s %s!", Yobjnam2(uwep, "are"),
                  scursed ? "mottled" : "shimmering",
                  hcolor(scursed ? NH_PURPLE : NH_GOLDEN),
                  scursed ? "glow" : "shield");
        }
        if (new_erodeproof && (uwep->oeroded || uwep->oeroded2)) {
            uwep->oeroded = uwep->oeroded2 = 0;
            pline("%s as good as new!",
                  Yobjnam2(uwep, Blind ? "feel" : "look"));
        }
        if (old_erodeproof && !new_erodeproof) {
            /* restore old_erodeproof before shop charges */
            uwep->oerodeproof = 1;
            costly_alteration(uwep, COST_DEGRD);
        }
        uwep->oerodeproof = new_erodeproof ? 1 : 0;
        return;
    }
    if (!chwepon(sobj, scursed ? -1
                 : !uwep ? 1
                 : (uwep->spe >= 9) ? !rn2(uwep->spe)
                 : sblessed ? rnd(3 - uwep->spe / 3)
                 : 1))
        *sobjp = 0; /* nothing enchanted: strange_feeling -> useup */
    if (uwep)
        cap_spe(uwep);
}

static void
seffect_taming(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean confused = (Confusion != 0);
    int candidates, res, results, vis_results;

    if (u.uswallow) {
        candidates = 1;
        results = vis_results = maybe_tame(u.ustuck, sobj);
    } else {
        int i, j, bd = confused ? 5 : 1;
        struct monst *mtmp;

        /* note: maybe_tame() can return either positive or
           negative values, but not both for the same scroll */
        candidates = results = vis_results = 0;
        for (i = -bd; i <= bd; i++)
            for (j = -bd; j <= bd; j++) {
                if (!isok(u.ux + i, u.uy + j))
                    continue;
                if ((mtmp = m_at(u.ux + i, u.uy + j)) != 0
                    || (!i && !j && (mtmp = u.usteed) != 0)) {
                    ++candidates;
                    res = maybe_tame(mtmp, sobj);
                    results += res;
                    if (canspotmon(mtmp))
                        vis_results += res;
                }
            }
    }
    if (!results) {
        pline("Nothing interesting %s.",
              !candidates ? "happens" : "seems to happen");
    } else {
        pline_The("neighborhood %s %sfriendlier.",
                  vis_results ? "is" : "seems",
                  (results < 0) ? "un" : "");
        if (vis_results > 0)
            g.known = TRUE;
    }
}

static void
seffect_genocide(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    int otyp = sobj->otyp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                             || objects[otyp].oc_name_known);

    if (!already_known)
        You("have found a scroll of genocide!");
    g.known = TRUE;
    if (sblessed)
        do_class_genocide();
    else
        do_genocide((!scursed) | (2 * !!Confusion));
}

static void
seffect_light(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);

    if (!confused) {
        if (!Blind)
            g.known = TRUE;
        litroom(!scursed, sobj);
        if (!scursed) {
            if (lightdamage(sobj, TRUE, 5))
                g.known = TRUE;
        }
    } else {
        int pm = scursed ? PM_BLACK_LIGHT : PM_YELLOW_LIGHT;

        if ((g.mvitals[pm].mvflags & G_GONE)) {
            pline("Tiny lights sparkle in the air momentarily.");
        } else {
            /* surround with cancelled tame lights which won't explode */
            struct monst *mon;
            boolean sawlights = FALSE;
            int i, numlights = rn1(2, 3) + (sblessed * 2);

            for (i = 0; i < numlights; ++i) {
                mon = makemon(&mons[pm], u.ux, u.uy,
                              MM_EDOG | NO_MINVENT | MM_NOMSG);
                initedog(mon);
                mon->msleeping = 0;
                mon->mcan = TRUE;
                if (canspotmon(mon))
                    sawlights = TRUE;
                newsym(mon->mx, mon->my);
            }
            if (sawlights) {
                pline("Lights appear all around you!");
                g.known = TRUE;
            }
        }
    }
}

static void
seffect_charging(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    int otyp = sobj->otyp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    boolean already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                             || objects[otyp].oc_name_known);
    struct obj *otmp;

    if (confused) {
        if (scursed) {
            You_feel("discharged.");
            u.uen = 0;
        } else {
            You_feel("charged up!");
            u.uen += d(sblessed ? 6 : 4, 4);
            if (u.uen > u.uenmax) /* if current energy is already at   */
                u.uenmax = u.uen; /* or near maximum, increase maximum */
            else
                u.uen = u.uenmax; /* otherwise restore current to max  */
        }
        g.context.botl = 1;
        return;
    }
    /* known = TRUE; -- handled inline here */
    if (!already_known) {
        pline("This is a charging scroll.");
        learnscroll(sobj);
    }
    /* use it up now to prevent it from showing in the
       getobj picklist because the "disappears" message
       was already delivered */
    useup(sobj);
    *sobjp = 0; /* it's gone */
    otmp = getobj("charge", charge_ok, GETOBJ_PROMPT | GETOBJ_ALLOWCNT);
    if (otmp)
        recharge(otmp, scursed ? -1 : sblessed ? 1 : 0);
}

static void
seffect_amnesia(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed;

    g.known = TRUE;
    forget((!sblessed ? ALL_SPELLS : 0));
    if (Hallucination) /* Ommmmmm! */
        Your("mind releases itself from mundane concerns.");
    else if (!strncmpi(g.plname, "Maud", 4))
        pline(
              "As your mind turns inward on itself, you forget everything else.");
    else if (rn2(2))
        pline("Who was that Maud person anyway?");
    else
        pline("Thinking of Maud you forget everything else.");
    exercise(A_WIS, FALSE);
}

static void
seffect_fire(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    int otyp = sobj->otyp;
    boolean sblessed = sobj->blessed;
    boolean confused = (Confusion != 0);
    boolean already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                             || objects[otyp].oc_name_known);
    coord cc;
    int dam, cval;

    cc.x = u.ux;
    cc.y = u.uy;
    cval = bcsign(sobj);
    dam = (2 * (rn1(3, 3) + 2 * cval) + 1) / 3;
    useup(sobj);
    *sobjp = 0; /* it's gone */
    if (!already_known)
        (void) learnscrolltyp(SCR_FIRE);
    if (confused) {
        if (Underwater) {
            pline("A little %s around you vaporizes.", hliquid("water"));
        }
        else if (Fire_resistance) {
            shieldeff(u.ux, u.uy);
            monstseesu(M_SEEN_FIRE);
            if (!Blind)
                pline("Oh, look, what a pretty fire in your %s.",
                      makeplural(body_part(HAND)));
            else
                You_feel("a pleasant warmth in your %s.",
                         makeplural(body_part(HAND)));
        } else {
            pline_The("scroll catches fire and you burn your %s.",
                      makeplural(body_part(HAND)));
            losehp(1, "scroll of fire", KILLED_BY_AN);
        }
        return;
    }
    if (Underwater) {
        pline_The("%s around you vaporizes violently!", hliquid("water"));
    } else {
        if (sblessed) {
            if (!already_known)
                pline("This is a scroll of fire!");
            dam *= 5;
            pline("Where do you want to center the explosion?");
            getpos_sethilite(display_stinking_cloud_positions,
                             can_center_cloud);
            (void) getpos(&cc, TRUE, "the desired position");
            if (!can_center_cloud(cc.x, cc.y)) {
                /* try to reach too far, get burned */
                cc.x = u.ux;
                cc.y = u.uy;
            }
        }
        if (u_at(cc.x, cc.y)) {
            pline_The("scroll erupts in a tower of flame!");
            iflags.last_msg = PLNMSG_TOWER_OF_FLAME; /* for explode() */
            burn_away_slime();
        }
    }
#define ZT_SPELL_O_FIRE 11 /* explained in splatter_burning_oil(explode.c) */
    explode(cc.x, cc.y, ZT_SPELL_O_FIRE, dam, SCROLL_CLASS, EXPL_FIERY);
#undef ZT_SPELL_O_FIRE
}

static void
seffect_earth(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);

    /* TODO: handle steeds */
    if (!Is_rogue_level(&u.uz) && has_ceiling(&u.uz)
        && (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
        coordxy x, y;
        int nboulders = 0;

        /* Identify the scroll */
        if (u.uswallow)
            You_hear("rumbling.");
        else
            pline_The("%s rumbles %s you!", ceiling(u.ux, u.uy),
                      sblessed ? "around" : "above");
        g.known = 1;
        sokoban_guilt();

        /* Loop through the surrounding squares */
        if (!scursed)
            for (x = u.ux - 1; x <= u.ux + 1; x++) {
                for (y = u.uy - 1; y <= u.uy + 1; y++) {
                    /* Is this a suitable spot? */
                    if (isok(x, y) && !closed_door(x, y)
                        && !IS_ROCK(levl[x][y].typ)
                        && !IS_AIR(levl[x][y].typ)
                        && (x != u.ux || y != u.uy)) {
                        nboulders +=
                            drop_boulder_on_monster(x, y, confused, TRUE);
                    }
                }
            }
        /* Attack the player */
        if (!sblessed) {
            drop_boulder_on_player(confused, !scursed, TRUE, FALSE);
        } else if (!nboulders)
            pline("But nothing else happens.");
    }
}

static void
seffect_punishment(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean sblessed = sobj->blessed;
    boolean confused = (Confusion != 0);

    g.known = TRUE;
    if (confused || sblessed) {
        You_feel("guilty.");
        return;
    }
    punish(sobj);
}

static void
seffect_stinking_cloud(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    int otyp = sobj->otyp;
    boolean already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                             || objects[otyp].oc_name_known);

    if (!already_known)
        You("have found a scroll of stinking cloud!");
    g.known = TRUE;
    do_stinking_cloud(sobj, already_known);
}

static void
seffect_blank_paper(struct obj **sobjp UNUSED)
{
    if (Blind)
        You("don't remember there being any magic words on this scroll.");
    else
        pline("This scroll seems to be blank.");
    g.known = TRUE;
}

static void
seffect_teleportation(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);

    if (confused || scursed) {
        level_tele();
        /* gives "materialize on different/same level!" message, must
           be a teleport scroll */
        g.known = TRUE;
    } else {
        scrolltele(sobj);
        /* this will call learnscroll() as appropriate, and has results
           which maybe shouldn't result in the scroll becoming known;
           either way, no need to set g.known here */
    }
}

static void
seffect_gold_detection(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);

    if ((confused || scursed) ? trap_detect(sobj) : gold_detect(sobj))
        *sobjp = 0; /* failure: strange_feeling() -> useup() */
}

static void
seffect_food_detection(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;

    if (food_detect(sobj))
        *sobjp = 0; /* nothing detected: strange_feeling -> useup */
}

static void
seffect_identify(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    int otyp = sobj->otyp;
    boolean is_scroll = (sobj->oclass == SCROLL_CLASS);
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    boolean already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                             || objects[otyp].oc_name_known);

    if (is_scroll) { /* scroll of identify */
        /* known = TRUE; -- handled inline here */
        /* use up the scroll first, before learnscrolltyp() -> makeknown()
           performs perm_invent update; also simplifies empty invent check */
        useup(sobj);
        *sobjp = 0; /* it's gone */
        /* scroll just identifies itself for any scroll read while confused
           or for cursed scroll read without knowing identify yet */
        if (confused || (scursed && !already_known))
            You("identify this as an identify scroll.");
        else if (!already_known)
            pline("This is an identify scroll.");
        if (!already_known)
            (void) learnscrolltyp(SCR_IDENTIFY);
        if (confused || (scursed && !already_known))
            return;
    }

    if (g.invent) {
        int cval = 1;
        if (sblessed || (!scursed && !rn2(5))) {
            cval = rn2(5);
            /* note: if cval==0, identify all items */
            if (cval == 1 && sblessed && Luck > 0)
                ++cval;
        }
        identify_pack(cval, !already_known);
    } else {
        /* spell cast with inventory empty or scroll read when it's
           the only item leaving empty inventory after being used up */
        pline("You're not carrying anything%s to be identified.",
              (is_scroll) ? " else" : "");
    }
}

static void
seffect_magic_mapping(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean is_scroll = (sobj->oclass == SCROLL_CLASS);
    boolean sblessed = sobj->blessed;
    boolean scursed = sobj->cursed;
    boolean confused = (Confusion != 0);
    int cval;

    if (is_scroll) {
        if (g.level.flags.nommap) {
            Your("mind is filled with crazy lines!");
            if (Hallucination)
                pline("Wow!  Modern art.");
            else
                Your("%s spins in bewilderment.", body_part(HEAD));
            make_confused(HConfusion + rnd(30), FALSE);
            return;
        }
        if (sblessed) {
            coordxy x, y;

            for (x = 1; x < COLNO; x++)
                for (y = 0; y < ROWNO; y++)
                    if (levl[x][y].typ == SDOOR)
                        cvt_sdoor_to_door(&levl[x][y]);
            /* do_mapping() already reveals secret passages */
        }
        g.known = TRUE;
    }

    if (g.level.flags.nommap) {
        Your("%s spins as %s blocks the spell!", body_part(HEAD),
             something);
        make_confused(HConfusion + rnd(30), FALSE);
        return;
    }
    pline("A map coalesces in your mind!");
    cval = (scursed && !confused);
    if (cval)
        HConfusion = 1; /* to screw up map */
    do_mapping();
    if (cval) {
        HConfusion = 0; /* restore */
        pline("Unfortunately, you can't grasp the details.");
    }
}

#ifdef MAIL_STRUCTURES
static void
seffect_mail(struct obj **sobjp)
{
    struct obj *sobj = *sobjp;
    boolean odd = (sobj->o_id % 2) == 1;

    g.known = TRUE;
    switch (sobj->spe) {
    case 2:
        /* "stamped scroll" created via magic marker--without a stamp */
        pline("This scroll is marked \"%s\".",
              odd ? "Postage Due" : "Return to Sender");
        break;
    case 1:
        /* scroll of mail obtained from bones file or from wishing;
           note to the puzzled: the game Larn actually sends you junk
           mail if you win! */
        pline("This seems to be %s.",
              odd ? "a chain letter threatening your luck"
              : "junk mail addressed to the finder of the Eye of Larn");
        break;
    default:
#ifdef MAIL
        readmail(sobj);
#else
        /* unreachable since with MAIL undefined, sobj->spe won't be 0;
           as a precaution, be prepared to give arbitrary feedback;
           caller has already reported that it disappears upon reading */
        pline("That was a scroll of mail?");
#endif
        break;
    }
}
#endif /* MAIL_STRUCTURES */

/* scroll effects; return 1 if we use up the scroll and possibly make it
   become discovered, 0 if caller should take care of those side-effects */
int
seffects(struct obj *sobj) /* sobj - scroll or fake spellbook for spell */
{
    int otyp = sobj->otyp;

    if (objects[otyp].oc_magic)
        exercise(A_WIS, TRUE);                       /* just for trying */

    switch (otyp) {
#ifdef MAIL_STRUCTURES
    case SCR_MAIL:
        seffect_mail(&sobj);
        break;
#endif
    case SCR_ENCHANT_ARMOR:
        seffect_enchant_armor(&sobj);
        break;
    case SCR_DESTROY_ARMOR:
        seffect_destroy_armor(&sobj);
        break;
    case SCR_CONFUSE_MONSTER:
    case SPE_CONFUSE_MONSTER:
        seffect_confuse_monster(&sobj);
        break;
    case SCR_SCARE_MONSTER:
    case SPE_CAUSE_FEAR:
        seffect_scare_monster(&sobj);
        break;
    case SCR_BLANK_PAPER:
        seffect_blank_paper(&sobj);
        break;
    case SCR_REMOVE_CURSE:
    case SPE_REMOVE_CURSE:
        seffect_remove_curse(&sobj);
        break;
    case SCR_CREATE_MONSTER:
    case SPE_CREATE_MONSTER:
        seffect_create_monster(&sobj);
        break;
    case SCR_ENCHANT_WEAPON:
        seffect_enchant_weapon(&sobj);
        break;
    case SCR_TAMING:
    case SPE_CHARM_MONSTER:
        seffect_taming(&sobj);
        break;
    case SCR_GENOCIDE:
        seffect_genocide(&sobj);
        break;
    case SCR_LIGHT:
        seffect_light(&sobj);
        break;
    case SCR_TELEPORTATION:
        seffect_teleportation(&sobj);
        break;
    case SCR_GOLD_DETECTION:
        seffect_gold_detection(&sobj);
        break;
    case SCR_FOOD_DETECTION:
    case SPE_DETECT_FOOD:
        seffect_food_detection(&sobj);
        break;
    case SCR_IDENTIFY:
    case SPE_IDENTIFY:
        seffect_identify(&sobj);
        break;
    case SCR_CHARGING:
        seffect_charging(&sobj);
        break;
    case SCR_MAGIC_MAPPING:
    case SPE_MAGIC_MAPPING:
        seffect_magic_mapping(&sobj);
        break;
    case SCR_AMNESIA:
        seffect_amnesia(&sobj);
        break;
    case SCR_FIRE:
        seffect_fire(&sobj);
        break;
    case SCR_EARTH:
        seffect_earth(&sobj);
        break;
    case SCR_PUNISHMENT:
        seffect_punishment(&sobj);
        break;
    case SCR_STINKING_CLOUD:
        seffect_stinking_cloud(&sobj);
        break;
    default:
        impossible("What weird effect is this? (%u)", otyp);
    }
    /* if sobj is gone, we've already called useup() above and the
       update_inventory() that it performs might have come too soon
       (before charging an item, for instance) */
    if (!sobj)
        update_inventory();
    return sobj ? 0 : 1;
}

void
drop_boulder_on_player(boolean confused, boolean helmet_protects, boolean byu, boolean skip_uswallow)
{
    int dmg;
    struct obj *otmp2;

    /* hit monster if swallowed */
    if (u.uswallow && !skip_uswallow) {
        drop_boulder_on_monster(u.ux, u.uy, confused, byu);
        return;
    }

    otmp2 = mksobj(confused ? ROCK : BOULDER, FALSE, FALSE);
    if (!otmp2)
        return;
    otmp2->quan = confused ? rn1(5, 2) : 1;
    otmp2->owt = weight(otmp2);
    if (!amorphous(g.youmonst.data) && !Passes_walls
        && !noncorporeal(g.youmonst.data) && !unsolid(g.youmonst.data)) {
        You("are hit by %s!", doname(otmp2));
        dmg = (int) (dmgval(otmp2, &g.youmonst) * otmp2->quan);
        if (uarmh && helmet_protects) {
            if (is_metallic(uarmh)) {
                pline("Fortunately, you are wearing a hard helmet.");
                if (dmg > 2)
                    dmg = 2;
            } else if (Verbose(3, drop_boulder_on_player)) {
                pline("%s does not protect you.", Yname2(uarmh));
            }
        }
    } else
        dmg = 0;
    wake_nearto(u.ux, u.uy, 4 * 4);
    /* Must be before the losehp(), for bones files */
    if (!flooreffects(otmp2, u.ux, u.uy, "fall")) {
        place_object(otmp2, u.ux, u.uy);
        stackobj(otmp2);
        newsym(u.ux, u.uy);
    }
    if (dmg)
        losehp(Maybe_Half_Phys(dmg), "scroll of earth", KILLED_BY_AN);
}

boolean
drop_boulder_on_monster(coordxy x, coordxy y, boolean confused, boolean byu)
{
    register struct obj *otmp2;
    register struct monst *mtmp;

    /* Make the object(s) */
    otmp2 = mksobj(confused ? ROCK : BOULDER, FALSE, FALSE);
    if (!otmp2)
        return FALSE; /* Shouldn't happen */
    otmp2->quan = confused ? rn1(5, 2) : 1;
    otmp2->owt = weight(otmp2);

    /* Find the monster here (won't be player) */
    mtmp = m_at(x, y);
    if (mtmp && !amorphous(mtmp->data) && !passes_walls(mtmp->data)
        && !noncorporeal(mtmp->data) && !unsolid(mtmp->data)) {
        struct obj *helmet = which_armor(mtmp, W_ARMH);
        long mdmg;

        if (cansee(mtmp->mx, mtmp->my)) {
            pline("%s is hit by %s!", Monnam(mtmp), doname(otmp2));
            if (mtmp->minvis && !canspotmon(mtmp))
                map_invisible(mtmp->mx, mtmp->my);
        } else if (engulfing_u(mtmp))
            You_hear("something hit %s %s over your %s!",
                     s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH),
                     body_part(HEAD));

        mdmg = dmgval(otmp2, mtmp) * otmp2->quan;
        if (helmet) {
            if (is_metallic(helmet)) {
                if (canspotmon(mtmp))
                    pline("Fortunately, %s is wearing a hard helmet.",
                          mon_nam(mtmp));
                else if (!Deaf)
                    You_hear("a clanging sound.");
                if (mdmg > 2)
                    mdmg = 2;
            } else {
                if (canspotmon(mtmp))
                    pline("%s's %s does not protect %s.", Monnam(mtmp),
                          xname(helmet), mhim(mtmp));
            }
        }
        mtmp->mhp -= mdmg;
        if (DEADMONSTER(mtmp)) {
            if (byu) {
                killed(mtmp);
            } else {
                pline("%s is killed.", Monnam(mtmp));
                mondied(mtmp);
            }
        } else {
            wakeup(mtmp, byu);
        }
        wake_nearto(x, y, 4 * 4);
    } else if (engulfing_u(mtmp)) {
        obfree(otmp2, (struct obj *) 0);
        /* fall through to player */
        drop_boulder_on_player(confused, TRUE, FALSE, TRUE);
        return 1;
    }
    /* Drop the rock/boulder to the floor */
    if (!flooreffects(otmp2, x, y, "fall")) {
        place_object(otmp2, x, y);
        stackobj(otmp2);
        newsym(x, y); /* map the rock */
    }
    return TRUE;
}

/* overcharging any wand or zapping/engraving cursed wand */
void
wand_explode(struct obj* obj, int chg /* recharging */)
{
    const char *expl = !chg ? "suddenly" : "vibrates violently and";
    int dmg, n, k;

    /* number of damage dice */
    if (!chg)
        chg = 2; /* zap/engrave adjustment */
    n = obj->spe + chg;
    if (n < 2)
        n = 2; /* arbitrary minimum */
    /* size of damage dice */
    switch (obj->otyp) {
    case WAN_WISHING:
        k = 12;
        break;
    case WAN_CANCELLATION:
    case WAN_DEATH:
    case WAN_POLYMORPH:
    case WAN_UNDEAD_TURNING:
        k = 10;
        break;
    case WAN_COLD:
    case WAN_FIRE:
    case WAN_LIGHTNING:
    case WAN_MAGIC_MISSILE:
        k = 8;
        break;
    case WAN_NOTHING:
        k = 4;
        break;
    default:
        k = 6;
        break;
    }
    /* inflict damage and destroy the wand */
    dmg = d(n, k);
    obj->in_use = TRUE; /* in case losehp() is fatal (or --More--^C) */
    pline("%s %s explodes!", Yname2(obj), expl);
    losehp(Maybe_Half_Phys(dmg), "exploding wand", KILLED_BY_AN);
    useup(obj);
    /* obscure side-effect */
    exercise(A_STR, FALSE);
}

/* used to collect gremlins being hit by light so that they can be processed
   after vision for the entire lit area has been brought up to date */
struct litmon {
    struct monst *mon;
    struct litmon *nxt;
};
static struct litmon *gremlins = 0;

/*
 * Low-level lit-field update routine.
 */
static void
set_lit(coordxy x, coordxy y, genericptr_t val)
{
    struct monst *mtmp;
    struct litmon *gremlin;

    if (val) {
        levl[x][y].lit = 1;
        if ((mtmp = m_at(x, y)) != 0 && mtmp->data == &mons[PM_GREMLIN]) {
            gremlin = (struct litmon *) alloc(sizeof *gremlin);
            gremlin->mon = mtmp;
            gremlin->nxt = gremlins;
            gremlins = gremlin;
        }
    } else {
        levl[x][y].lit = 0;
        snuff_light_source(x, y);
    }
}

void
litroom(
    boolean on,      /* True: make nearby area lit; False: cursed scroll */
    struct obj *obj) /* scroll, spellbook (for spell), or wand of light */
{
    struct obj *otmp;
    boolean blessed_effect = (obj && obj->oclass == SCROLL_CLASS
                              && obj->blessed);
    char is_lit = 0; /* value is irrelevant but assign something anyway; its
                      * address is used as a 'not null' flag for set_lit() */

    /* update object lights and produce message (provided you're not blind) */
    if (!on) {
        int still_lit = 0;

        /*
         * The magic douses lamps,&c too and might curse artifact lights.
         *
         * FIXME?
         *  Shouldn't this affect all lit objects in the area of effect
         *  rather than just those carried by the hero?
         */
        for (otmp = g.invent; otmp; otmp = otmp->nobj) {
            if (otmp->lamplit) {
                if (!artifact_light(otmp))
                    (void) snuff_lit(otmp);
                else
                    /* wielded Sunsword or worn gold dragon scales/mail;
                       maybe lower its BUC state if not already cursed */
                    impact_arti_light(otmp, TRUE, (boolean) !Blind);

                if (otmp->lamplit)
                    ++still_lit;
            }
        }
        /* scroll of light becomes discovered when not blind, so some
           message to justify that is needed */
        if (!Blind) {
            /* for the still_lit case, we don't know at this point whether
               anything currently visibly lit is going to go dark; if this
               message came after the darkening, we could count visibly
               lit squares before and after to know; we do know that being
               swallowed won't be affected--the interior is still lit */
            if (still_lit)
                pline_The("ambient light seems dimmer.");
            else if (u.uswallow)
                pline("It seems even darker in here than before.");
            else
                You("are surrounded by darkness!");
        }
    } else { /* on */
        if (blessed_effect) {
            /* might bless artifact lights; no effect on ordinary lights */
            for (otmp = g.invent; otmp; otmp = otmp->nobj) {
                if (otmp->lamplit && artifact_light(otmp))
                    /* wielded Sunsword or worn gold dragon scales/mail;
                       maybe raise its BUC state if not already blessed */
                    impact_arti_light(otmp, FALSE, (boolean) !Blind);
            }
        }
        if (u.uswallow) {
            if (Blind)
                ; /* no feedback */
            else if (digests(u.ustuck->data))
                pline("%s %s is lit.", s_suffix(Monnam(u.ustuck)),
                      mbodypart(u.ustuck, STOMACH));
            else if (is_whirly(u.ustuck->data))
                pline("%s shines briefly.", Monnam(u.ustuck));
            else
                pline("%s glistens.", Monnam(u.ustuck));
        } else if (!Blind)
            pline("A lit field surrounds you!");
    }

    /* No-op when swallowed or in water */
    if (u.uswallow || Underwater || Is_waterlevel(&u.uz))
        return;
    /*
     *  If we are darkening the room and the hero is punished but not
     *  blind, then we have to pick up and replace the ball and chain so
     *  that we don't remember them if they are out of sight.
     */
    if (Punished && !on && !Blind)
        move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

    if (Is_rogue_level(&u.uz)) {
        /* Can't use do_clear_area because MAX_RADIUS is too small */
        /* rogue lighting must light the entire room */
        int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET;
        int rx, ry;

        if (rnum >= 0) {
            for (rx = g.rooms[rnum].lx - 1; rx <= g.rooms[rnum].hx + 1; rx++)
                for (ry = g.rooms[rnum].ly - 1;
                     ry <= g.rooms[rnum].hy + 1; ry++)
                    set_lit(rx, ry,
                            (genericptr_t) (on ? &is_lit : (char *) 0));
            g.rooms[rnum].rlit = on;
        }
        /* hallways remain dark on the rogue level */
    } else
        do_clear_area(u.ux, u.uy, blessed_effect ? 9 : 5,
                      set_lit, (genericptr_t) (on ? &is_lit : (char *) 0));

    /*
     *  If we are not blind, then force a redraw on all positions in sight
     *  by temporarily blinding the hero.  The vision recalculation will
     *  correctly update all previously seen positions *and* correctly
     *  set the waslit bit [could be messed up from above].
     */
    if (!Blind) {
        vision_recalc(2);

        /* replace ball&chain */
        if (Punished && !on)
            move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
    }

    g.vision_full_recalc = 1; /* delayed vision recalculation */
    if (gremlins) {
        struct litmon *gremlin;

        /* can't delay vision recalc after all */
        vision_recalc(0);
        /* after vision has been updated, monsters who are affected
           when hit by light can now be hit by it */
        do {
            gremlin = gremlins;
            gremlins = gremlin->nxt;
            light_hits_gremlin(gremlin->mon, rnd(5));
            free((genericptr_t) gremlin);
        } while (gremlins);
    }
    return;
}

static void
do_class_genocide(void)
{
    int i, j, immunecnt, gonecnt, goodcnt, class, feel_dead = 0;
    int ll_done = 0;
    char buf[BUFSZ] = DUMMY;
    boolean gameover = FALSE; /* true iff killed self */

    for (j = 0;; j++) {
        if (j >= 5) {
            pline1(thats_enough_tries);
            return;
        }
        do {
            getlin("What class of monsters do you wish to genocide?", buf);
            (void) mungspaces(buf);
        } while (!*buf);
        /* choosing "none" preserves genocideless conduct */
        if (*buf == '\033' || !strcmpi(buf, "none")
            || !strcmpi(buf, "nothing")) {
            livelog_printf(LL_GENOCIDE,
                           "declined to perform class genocide");
            return;
        }

        class = name_to_monclass(buf, (int *) 0);
        if (class == 0 && (i = name_to_mon(buf, (int *) 0)) != NON_PM)
            class = mons[i].mlet;
        immunecnt = gonecnt = goodcnt = 0;
        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                if (!(mons[i].geno & G_GENO))
                    immunecnt++;
                else if (g.mvitals[i].mvflags & G_GENOD)
                    gonecnt++;
                else
                    goodcnt++;
            }
        }
        if (!goodcnt && class != mons[g.urole.mnum].mlet
            && class != mons[g.urace.mnum].mlet) {
            if (gonecnt)
                pline("All such monsters are already nonexistent.");
            else if (immunecnt || class == S_invisible)
                You("aren't permitted to genocide such monsters.");
            else if (wizard && buf[0] == '*') {
                register struct monst *mtmp, *mtmp2;

                gonecnt = 0;
                for (mtmp = fmon; mtmp; mtmp = mtmp2) {
                    mtmp2 = mtmp->nmon;
                    if (DEADMONSTER(mtmp))
                        continue;
                    mongone(mtmp);
                    gonecnt++;
                }
                pline("Eliminated %d monster%s.", gonecnt, plur(gonecnt));
                return;
            } else
                pline("That %s does not represent any monster.",
                      strlen(buf) == 1 ? "symbol" : "response");
            continue;
        }

        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                char nam[BUFSZ];

                Strcpy(nam, makeplural(mons[i].pmnames[NEUTRAL]));
                /* Although "genus" is Latin for race, the hero benefits
                 * from both race and role; thus genocide affects either.
                 */
                if (Your_Own_Role(i) || Your_Own_Race(i)
                    || ((mons[i].geno & G_GENO)
                        && !(g.mvitals[i].mvflags & G_GENOD))) {
                    /* This check must be first since player monsters might
                     * have G_GENOD or !G_GENO.
                     */
                    if (!ll_done++) {
                        if (!num_genocides())
                            livelog_printf(LL_CONDUCT | LL_GENOCIDE,
                                     "performed %s first genocide (class %c)",
                                           uhis(), def_monsyms[class].sym);
                        else
                            livelog_printf(LL_GENOCIDE, "genocided class %c",
                                           def_monsyms[class].sym);
                    }

                    g.mvitals[i].mvflags |= (G_GENOD | G_NOCORPSE);
                    kill_genocided_monsters();
                    update_inventory(); /* eggs & tins */
                    pline("Wiped out all %s.", nam);
                    if (Upolyd && vampshifted(&g.youmonst)
                        /* current shifted form or base vampire form */
                        && (i == u.umonnum || i == g.youmonst.cham))
                        polyself(POLY_REVERT); /* vampshifter back to vampire */
                    if (Upolyd && i == u.umonnum) {
                        u.mh = -1;
                        if (Unchanging) {
                            if (!feel_dead++)
                                urgent_pline("You die.");
                            /* finish genociding this class of
                               monsters before ultimately dying */
                            gameover = TRUE;
                        } else
                            rehumanize();
                    }
                    /* Self-genocide if it matches either your race
                       or role.  Assumption:  male and female forms
                       share same monster class. */
                    if (i == g.urole.mnum || i == g.urace.mnum) {
                        u.uhp = -1;
                        if (Upolyd) {
                            if (!feel_dead++)
                                You_feel("%s inside.", udeadinside());
                        } else {
                            if (!feel_dead++)
                                urgent_pline("You die.");
                            gameover = TRUE;
                        }
                    }
                } else if (g.mvitals[i].mvflags & G_GENOD) {
                    if (!gameover)
                        pline("%s are already nonexistent.", upstart(nam));
                } else if (!gameover) {
                    /* suppress feedback about quest beings except
                       for those applicable to our own role */
                    if ((mons[i].msound != MS_LEADER
                         || quest_info(MS_LEADER) == i)
                        && (mons[i].msound != MS_NEMESIS
                            || quest_info(MS_NEMESIS) == i)
                        && (mons[i].msound != MS_GUARDIAN
                            || quest_info(MS_GUARDIAN) == i)
                        /* non-leader/nemesis/guardian role-specific monster
                           */
                        && (i != PM_NINJA /* nuisance */
                            || Role_if(PM_SAMURAI))) {
                        boolean named, uniq;

                        named = type_is_pname(&mons[i]) ? TRUE : FALSE;
                        uniq = (mons[i].geno & G_UNIQ) ? TRUE : FALSE;
                        /* one special case */
                        if (i == PM_HIGH_CLERIC)
                            uniq = FALSE;

                        You("aren't permitted to genocide %s%s.",
                            (uniq && !named) ? "the " : "",
                            (uniq || named) ? mons[i].pmnames[NEUTRAL] : nam);
                    }
                }
            }
        }
        if (gameover || u.uhp == -1) {
            g.killer.format = KILLED_BY_AN;
            Strcpy(g.killer.name, "scroll of genocide");
            if (gameover)
                done(GENOCIDED);
        }
        return;
    }
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
void
do_genocide(int how)
/* how: */
/* 0 = no genocide; create monsters (cursed scroll) */
/* 1 = normal genocide */
/* 3 = forced genocide of player */
/* 5 (4 | 1) = normal genocide from throne */
{
    char buf[BUFSZ] = DUMMY;
    register int i, killplayer = 0;
    register int mndx;
    register struct permonst *ptr;
    const char *which;

    if (how & PLAYER) {
        mndx = u.umonster; /* non-polymorphed mon num */
        ptr = &mons[mndx];
        Strcpy(buf, pmname(ptr, Ugender));
        killplayer++;
    } else {
        for (i = 0;; i++) {
            if (i >= 5) {
                /* cursed effect => no free pass (unless rndmonst() fails) */
                if (!(how & REALLY) && (ptr = rndmonst()) != 0)
                    break;

                pline1(thats_enough_tries);
                return;
            }
            getlin("What monster do you want to genocide? [type the name]",
                   buf);
            (void) mungspaces(buf);
            /* choosing "none" preserves genocideless conduct */
            if (*buf == '\033' || !strcmpi(buf, "none")
                || !strcmpi(buf, "nothing")) {
                /* ... but no free pass if cursed */
                if (!(how & REALLY) && (ptr = rndmonst()) != 0)
                    break; /* remaining checks don't apply */

                livelog_printf(LL_GENOCIDE, "declined to perform genocide");
                return;
            }

            mndx = name_to_mon(buf, (int *) 0);
            if (mndx == NON_PM || (g.mvitals[mndx].mvflags & G_GENOD)) {
                pline("Such creatures %s exist in this world.",
                      (mndx == NON_PM) ? "do not" : "no longer");
                continue;
            }
            ptr = &mons[mndx];
            /* first revert if current shifted form or base vampire form */
            if (Upolyd && vampshifted(&g.youmonst)
                && (mndx == u.umonnum || mndx == g.youmonst.cham))
                polyself(POLY_REVERT); /* vampshifter (bat, &c) back to vampire */
            /* Although "genus" is Latin for race, the hero benefits
             * from both race and role; thus genocide affects either.
             */
            if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
                killplayer++;
                break;
            }
            if (is_human(ptr))
                adjalign(-sgn(u.ualign.type));
            if (is_demon(ptr))
                adjalign(sgn(u.ualign.type));

            if (!(ptr->geno & G_GENO)) {
                if (!Deaf) {
                    /* FIXME: unconditional "caverns" will be silly in some
                     * circumstances.  Who's speaking?  Divine pronouncements
                     * aren't supposed to be hampered by deafness....
                     */
                    if (Verbose(3, do_genocide))
                        pline("A thunderous voice booms through the caverns:");
                    verbalize("No, mortal!  That will not be done.");
                }
                continue;
            }
            /* KMH -- Unchanging prevents rehumanization */
            if (Unchanging && ptr == g.youmonst.data)
                killplayer++;
            break;
        }
        mndx = monsndx(ptr); /* needed for the 'no free pass' cases */
    }

    which = "all ";
    if (Hallucination) {
        if (Upolyd)
            Strcpy(buf, pmname(g.youmonst.data, flags.female ? FEMALE : MALE));
        else {
            Strcpy(buf, (flags.female && g.urole.name.f) ? g.urole.name.f
                                                       : g.urole.name.m);
            buf[0] = lowc(buf[0]);
        }
    } else {
        Strcpy(buf, ptr->pmnames[NEUTRAL]); /* make sure we have standard singular */
        if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_CLERIC])
            which = !type_is_pname(ptr) ? "the " : "";
    }
    if (how & REALLY) {
        if (!num_genocides())
            livelog_printf(LL_CONDUCT | LL_GENOCIDE,
                           "performed %s first genocide (%s)",
                           uhis(), makeplural(buf));
        else
            livelog_printf(LL_GENOCIDE, "genocided %s", makeplural(buf));

        /* setting no-corpse affects wishing and random tin generation */
        g.mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
        pline("Wiped out %s%s.", which,
              (*which != 'a') ? buf : makeplural(buf));

        if (killplayer) {
            u.uhp = -1;
            if (how & PLAYER) {
                g.killer.format = KILLED_BY;
                Strcpy(g.killer.name, "genocidal confusion");
            } else if (how & ONTHRONE) {
                /* player selected while on a throne */
                g.killer.format = KILLED_BY_AN;
                Strcpy(g.killer.name, "imperious order");
            } else { /* selected player deliberately, not confused */
                g.killer.format = KILLED_BY_AN;
                Strcpy(g.killer.name, "scroll of genocide");
            }

            /* Polymorphed characters will die as soon as they're rehumanized.
             */
            /* KMH -- Unchanging prevents rehumanization */
            if (Upolyd && ptr != g.youmonst.data) {
                delayed_killer(POLYMORPH, g.killer.format, g.killer.name);
                You_feel("%s inside.", udeadinside());
            } else
                done(GENOCIDED);
        } else if (ptr == g.youmonst.data) {
            rehumanize();
        }
        kill_genocided_monsters();
        update_inventory(); /* in case identified eggs were affected */
    } else {
        int cnt = 0, census = monster_census(FALSE);

        if (!(mons[mndx].geno & G_UNIQ)
            && !(g.mvitals[mndx].mvflags & (G_GENOD | G_EXTINCT)))
            for (i = rn1(3, 4); i > 0; i--) {
                if (!makemon(ptr, u.ux, u.uy, NO_MINVENT | MM_NOMSG))
                    break; /* couldn't make one */
                ++cnt;
                if (g.mvitals[mndx].mvflags & G_EXTINCT)
                    break; /* just made last one */
            }
        if (cnt) {
            /* accumulated 'cnt' doesn't take groups into account;
               assume bringing in new mon(s) didn't remove any old ones */
            cnt = monster_census(FALSE) - census;
            pline("Sent in %s%s.", (cnt > 1) ? "some " : "",
                  (cnt > 1) ? makeplural(buf) : an(buf));
        } else
            pline1(nothing_happens);
    }
}

void
punish(struct obj* sobj)
{
    struct obj *reuse_ball = (sobj && sobj->otyp == HEAVY_IRON_BALL)
                                ? sobj : (struct obj *) 0;

    /* KMH -- Punishment is still okay when you are riding */
    if (!reuse_ball)
        You("are being punished for your misbehavior!");
    if (Punished) {
        Your("iron ball gets heavier.");
        uball->owt += IRON_BALL_W_INCR * (1 + sobj->cursed);
        return;
    }
    if (amorphous(g.youmonst.data) || is_whirly(g.youmonst.data)
        || unsolid(g.youmonst.data)) {
        if (!reuse_ball) {
            pline("A ball and chain appears, then falls away.");
            dropy(mkobj(BALL_CLASS, TRUE));
        } else {
            dropy(reuse_ball);
        }
        return;
    }
    setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
    if (!reuse_ball)
        setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
    else
        setworn(reuse_ball, W_BALL);

    /*
     *  Place ball & chain if not swallowed.  If swallowed, the ball & chain
     *  variables will be set at the next call to placebc().
     */
    if (!u.uswallow) {
        placebc();
        if (Blind)
            set_bc(1);      /* set up ball and chain variables */
        newsym(u.ux, u.uy); /* see ball&chain if can't see self */
    }
}

/* remove the ball and chain */
void
unpunish(void)
{
    struct obj *savechain = uchain;

    /* chain goes away */
    setworn((struct obj *) 0, W_CHAIN); /* sets 'uchain' to Null */
    /* for floor, unhides monster hidden under chain, calls newsym() */
    delobj(savechain);

    /* the chain is gone but the no longer attached ball persists */
    setworn((struct obj *) 0, W_BALL); /* sets 'uball' to Null */
}

/* Prompt the player to create a stinking cloud and then create it if they give
 * a location. */
static void
do_stinking_cloud(struct obj *sobj, boolean mention_stinking)
{
    coord cc;

    pline("Where do you want to center the %scloud?",
          mention_stinking ? "stinking " : "");
    cc.x = u.ux;
    cc.y = u.uy;
    getpos_sethilite(display_stinking_cloud_positions, can_center_cloud);
    if (getpos(&cc, TRUE, "the desired position") < 0) {
        pline1(Never_mind);
        return;
    } else if (!can_center_cloud(cc.x, cc.y)) {
        if (Hallucination)
            pline("Ugh... someone cut the cheese.");
        else
            pline("%s a whiff of rotten eggs.",
                  sobj->oclass == SCROLL_CLASS ? "The scroll crumbles with"
                                               : "You smell");
        return;
    }
    (void) create_gas_cloud(cc.x, cc.y, 15 + 10 * bcsign(sobj),
                            8 + 4 * bcsign(sobj));
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to
 * revive one, the disoriented creature becomes a zombie
 */
boolean
cant_revive(int* mtype, boolean revival, struct obj* from_obj)
{
    /* SHOPKEEPERS can be revived now */
    if (*mtype == PM_GUARD || (*mtype == PM_SHOPKEEPER && !revival)
        || *mtype == PM_HIGH_CLERIC || *mtype == PM_ALIGNED_CLERIC
        || *mtype == PM_ANGEL) {
        *mtype = PM_HUMAN_ZOMBIE;
        return TRUE;
    } else if (*mtype == PM_LONG_WORM_TAIL) { /* for create_particular() */
        *mtype = PM_LONG_WORM;
        return TRUE;
    } else if (unique_corpstat(&mons[*mtype])
               && (!from_obj || !has_omonst(from_obj))) {
        /* unique corpses (from bones or wizard mode wish) or
           statues (bones or any wish) end up as shapechangers */
        *mtype = PM_DOPPELGANGER;
        return TRUE;
    }
    return FALSE;
}

static boolean
create_particular_parse(char* str, struct _create_particular_data* d)
{
    int gender_name_var = NEUTRAL;
    char *bufp = str;
    char *tmpp;

    d->quan = 1 + ((g.multi > 0) ? (int) g.multi : 0);
    d->monclass = MAXMCLASSES;
    d->which = g.urole.mnum; /* an arbitrary index into mons[] */
    d->fem = -1;     /* gender not specified */
    d->genderconf = -1;  /* no confusion on which gender to assign */
    d->randmonst = FALSE;
    d->maketame = d->makepeaceful = d->makehostile = FALSE;
    d->sleeping = d->saddled = d->invisible = d->hidden = FALSE;

    /* quantity */
    if (digit(*bufp)) {
        d->quan = atoi(bufp);
        while (digit(*bufp))
            bufp++;
        while (*bufp == ' ')
            bufp++;
    }
#define QUAN_LIMIT (ROWNO * (COLNO - 1))
    /* maximum possible quantity is one per cell: (0..ROWNO-1) x (1..COLNO-1)
       [21*79==1659 for default map size; could subtract 1 for hero's spot] */
    if (d->quan < 1 || d->quan > QUAN_LIMIT)
        d->quan = QUAN_LIMIT - monster_census(FALSE);
#undef QUAN_LIMIT
    /* gear -- extremely limited number of possibilities supported */
    if ((tmpp = strstri(bufp, "saddled ")) != 0) {
        d->saddled = TRUE;
        (void) memset(tmpp, ' ', sizeof "saddled " - 1);
    }
    /* state -- limited number of possibilitie supported */
    if ((tmpp = strstri(bufp, "sleeping ")) != 0) {
        d->sleeping = TRUE;
        (void) memset(tmpp, ' ', sizeof "sleeping " - 1);
    }
    if ((tmpp = strstri(bufp, "invisible ")) != 0) {
        d->invisible = TRUE;
        (void) memset(tmpp, ' ', sizeof "invisible " - 1);
    }
    if ((tmpp = strstri(bufp, "hidden ")) != 0) {
        d->hidden = TRUE;
        (void) memset(tmpp, ' ', sizeof "hidden " - 1);
    }
    /* check "female" before "male" to avoid false hit mid-word */
    if ((tmpp = strstri(bufp, "female ")) != 0) {
        d->fem = 1;
        (void) memset(tmpp, ' ', sizeof "female " - 1);
    }
    if ((tmpp = strstri(bufp, "male ")) != 0) {
        d->fem = 0;
        (void) memset(tmpp, ' ', sizeof "male " - 1);
    }
    bufp = mungspaces(bufp); /* after potential memset(' ') */
    /* allow the initial disposition to be specified */
    if (!strncmpi(bufp, "tame ", 5)) {
        bufp += 5;
        d->maketame = TRUE;
    } else if (!strncmpi(bufp, "peaceful ", 9)) {
        bufp += 9;
        d->makepeaceful = TRUE;
    } else if (!strncmpi(bufp, "hostile ", 8)) {
        bufp += 8;
        d->makehostile = TRUE;
    }
    /* decide whether a valid monster was chosen */
    if (wizard && (!strcmp(bufp, "*") || !strcmp(bufp, "random"))) {
        d->randmonst = TRUE;
        return TRUE;
    }
    d->which = name_to_mon(bufp, &gender_name_var);
    /*
     * With the introduction of male and female monster names
     * in 3.7, preserve that detail.
     *
     * If d->fem is already set to MALE or FEMALE at this juncture, it means
     * one of those terms was explicitly specified.
     */
    if (d->fem == MALE || d->fem == FEMALE) {     /* explicity expressed */
        if ((gender_name_var != NEUTRAL) && (d->fem != gender_name_var)) {
            /* apparent selection incompatibility */
            d->genderconf = gender_name_var;        /* resolve later */
        }
        /* otherwise keep the value of d->fem, as it's okay */
    } else {  /* no explicit gender term was specified */
        d->fem = gender_name_var;
    }
    if (d->which >= LOW_PM)
        return TRUE; /* got one */
    d->monclass = name_to_monclass(bufp, &d->which);

    if (d->which >= LOW_PM) {
        d->monclass = MAXMCLASSES; /* matters below */
        return TRUE;
    } else if (d->monclass == S_invisible) { /* not an actual monster class */
        d->which = PM_STALKER;
        d->monclass = MAXMCLASSES;
        return TRUE;
    } else if (d->monclass == S_WORM_TAIL) { /* empty monster class */
        d->which = PM_LONG_WORM;
        d->monclass = MAXMCLASSES;
        return TRUE;
    } else if (d->monclass > 0) {
        d->which = g.urole.mnum; /* reset from NON_PM */
        return TRUE;
    }
    return FALSE;
}

static boolean
create_particular_creation(struct _create_particular_data* d)
{
    struct permonst *whichpm = NULL;
    int i, mx, my, firstchoice = NON_PM;
    struct monst *mtmp;
    boolean madeany = FALSE;

    if (!d->randmonst) {
        firstchoice = d->which;
        if (cant_revive(&d->which, FALSE, (struct obj *) 0)
            && firstchoice != PM_LONG_WORM_TAIL) {
            /* wizard mode can override handling of special monsters */
            char buf[BUFSZ];

            Sprintf(buf, "Creating %s instead; force %s?",
                    mons[d->which].pmnames[NEUTRAL],
                    mons[firstchoice].pmnames[NEUTRAL]);
            if (yn(buf) == 'y')
                d->which = firstchoice;
        }
        whichpm = &mons[d->which];
    }
    for (i = 0; i < d->quan; i++) {
        mmflags_nht mmflags = NO_MM_FLAGS;

        if (d->monclass != MAXMCLASSES)
            whichpm = mkclass(d->monclass, 0);
        else if (d->randmonst)
            whichpm = rndmonst();
        if (d->genderconf == -1) {
            /* no confict exists between explicit gender term and
               the specified monster name */
            if (d->fem != -1 && (!whichpm || (!is_male(whichpm)
                                              && !is_female(whichpm))))
                mmflags |= (d->fem == FEMALE) ? MM_FEMALE
                               : (d->fem == MALE) ? MM_MALE : 0;
            /* no surprise; "<mon> appears." rather than "<mon> appears!" */
            mmflags |= MM_NOEXCLAM;
        } else {
            /* conundrum alert: an explicit gender term conflicts with an
               explicit gender-tied naming term (i.e. male cavewoman) */

            /* option not gone with: name overrides the explicit gender as
               commented out here */
            /*  d->fem = d->genderconf; */

            /* option chosen: let the explicit gender term (male or female)
               override the gender-tied naming term, so leave d->fem as-is */

            mmflags |= (d->fem == FEMALE) ? MM_FEMALE
                           : (d->fem == MALE) ? MM_MALE : 0;

            /* another option would be to consider it a faulty specification
               and reject the request completely and produce a random monster
               with a gender matching that specified instead (i.e. there is
               no such thing as a male cavewoman) */
            /* whichpm = rndmonst(); */
            /* mmflags |= (d->fem == FEMALE) ? MM_FEMALE : MM_MALE; */
        }
        mtmp = makemon(whichpm, u.ux, u.uy, mmflags);
        if (!mtmp) {
            /* quit trying if creation failed and is going to repeat */
            if (d->monclass == MAXMCLASSES && !d->randmonst)
                break;
            /* otherwise try again */
            continue;
        }
        mx = mtmp->mx, my = mtmp->my;
        if (d->maketame) {
            (void) tamedog(mtmp, (struct obj *) 0);
        } else if (d->makepeaceful || d->makehostile) {
            mtmp->mtame = 0; /* sanity precaution */
            mtmp->mpeaceful = d->makepeaceful ? 1 : 0;
            set_malign(mtmp);
        }
        if (d->saddled && can_saddle(mtmp) && !which_armor(mtmp, W_SADDLE)) {
            struct obj *otmp = mksobj(SADDLE, TRUE, FALSE);

            put_saddle_on_mon(otmp, mtmp);
        }
        if (d->invisible) {
            mon_set_minvis(mtmp);
            if (does_block(mx, my, &levl[mx][my]))
                block_point(mx, my);
            else
                unblock_point(mx, my);
        }
       if (d->hidden
           && ((is_hider(mtmp->data) && mtmp->data->mlet != S_MIMIC)
               || (hides_under(mtmp->data) && OBJ_AT(mx, my))
               || (mtmp->data->mlet == S_EEL && is_pool(mx, my))))
            mtmp->mundetected = 1;
        if (d->sleeping)
            mtmp->msleeping = 1;
        /* iff asking for 'hidden', show location of every created monster
           that can't be seen--whether that's due to successfully hiding
           or vision issues (line-of-sight, invisibility, blindness) */
        if (d->hidden && !canspotmon(mtmp)) {
            int count = couldsee(mx, my) ? 8 : 4;
            char saveviz = g.viz_array[my][mx];

            if (!flags.sparkle)
                count /= 2;
            g.viz_array[my][mx] |= (IN_SIGHT | COULD_SEE);
            flash_glyph_at(mx, my, mon_to_glyph(mtmp, newsym_rn2), count);
            g.viz_array[my][mx] = saveviz;
            newsym(mx, my);
        }
        madeany = TRUE;
        /* in case we got a doppelganger instead of what was asked
           for, make it start out looking like what was asked for */
        if (mtmp->cham != NON_PM && firstchoice != NON_PM
            && mtmp->cham != firstchoice)
            (void) newcham(mtmp, &mons[firstchoice], NO_NC_FLAGS);
    }
    return madeany;
}

/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic.  This behavior quirk is useful so don't "fix" it
 * (use 'm'--or "mimic"--to create a random mimic).
 *
 * Used in wizard mode only (for ^G command and for scroll or spell
 * of create monster).  Once upon a time, an earlier incarnation of
 * this code was also used for the scroll/spell in explore mode.
 */
boolean
create_particular(void)
{
#define CP_TRYLIM 5
    struct _create_particular_data d;
    char *bufp, buf[BUFSZ], prompt[QBUFSZ];
    int  tryct = CP_TRYLIM, altmsg = 0;

    buf[0] = '\0'; /* for EDIT_GETLIN */
    Strcpy(prompt, "Create what kind of monster?");
    do {
        getlin(prompt, buf);
        bufp = mungspaces(buf);
        if (*bufp == '\033')
            return FALSE;

        if (create_particular_parse(bufp, &d))
            break;

        /* no good; try again... */
        if (*bufp || altmsg || tryct < 2) {
            pline("I've never heard of such monsters.");
        } else {
            pline("Try again (type * for random, ESC to cancel).");
            ++altmsg;
        }
        /* when a second try is needed, expand the prompt */
        if (tryct == CP_TRYLIM)
            Strcat(prompt, " [type name or symbol]");
    } while (--tryct > 0);

    if (!tryct)
        pline1(thats_enough_tries);
    else
        return create_particular_creation(&d);

    return FALSE;
}

/*read.c*/

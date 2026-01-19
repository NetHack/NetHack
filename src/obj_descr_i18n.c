/* NetHack 3.7  obj_descr_i18n.c */
/* Copyright (c) HanNetHack Project, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file exists solely to mark item descriptions for translation
 * extraction by xgettext. The N_() macro marks strings for extraction
 * without actually translating them at compile time.
 *
 * DO NOT include this file in the build - it's only for xgettext.
 */

#include "i18n.h"

/* This function is never called - it just holds strings for xgettext */
static void
obj_descr_strings_for_extraction(void)
{
    /* === Potion descriptions === */
    (void) N_("ruby");
    (void) N_("pink");
    (void) N_("orange");
    (void) N_("yellow");
    (void) N_("emerald");
    (void) N_("dark green");
    (void) N_("cyan");
    (void) N_("sky blue");
    (void) N_("brilliant blue");
    (void) N_("magenta");
    (void) N_("purple-red");
    (void) N_("puce");
    (void) N_("milky");
    (void) N_("swirly");
    (void) N_("bubbly");
    (void) N_("smoky");
    (void) N_("cloudy");
    (void) N_("effervescent");
    (void) N_("black");
    (void) N_("golden");
    (void) N_("brown");
    (void) N_("fizzy");
    (void) N_("dark");
    (void) N_("white");
    (void) N_("murky");
    (void) N_("clear");

    /* === Ring descriptions === */
    (void) N_("wooden");
    (void) N_("granite");
    (void) N_("opal");
    (void) N_("clay");
    (void) N_("coral");
    (void) N_("black onyx");
    (void) N_("moonstone");
    (void) N_("tiger eye");
    (void) N_("jade");
    (void) N_("bronze");
    (void) N_("agate");
    (void) N_("topaz");
    (void) N_("sapphire");
    (void) N_("diamond");
    (void) N_("pearl");
    (void) N_("iron");
    (void) N_("brass");
    (void) N_("copper");
    (void) N_("twisted");
    (void) N_("steel");
    (void) N_("silver");
    (void) N_("gold");
    (void) N_("ivory");
    (void) N_("engagement");
    (void) N_("shiny");
    (void) N_("wire");

    /* === Wand descriptions === */
    (void) N_("glass");
    (void) N_("aluminum");
    (void) N_("uranium");
    (void) N_("long");
    (void) N_("short");
    (void) N_("curved");
    (void) N_("runed");
    (void) N_("oak");
    (void) N_("ebony");
    (void) N_("marble");
    (void) N_("tin");
    (void) N_("maple");
    (void) N_("pine");
    (void) N_("zinc");
    (void) N_("platinum");
    (void) N_("iridium");
    (void) N_("crystal");
    (void) N_("hexagonal");

    /* === Amulet descriptions === */
    (void) N_("circular");
    (void) N_("triangular");
    (void) N_("spherical");
    (void) N_("oval");
    (void) N_("octagonal");
    (void) N_("pyramidal");
    (void) N_("square");
    (void) N_("concave");
    (void) N_("cubical");
    (void) N_("perforated");

    /* === Common item name parts === */
    (void) N_("potion");
    (void) N_("scroll");
    (void) N_("wand");
    (void) N_("ring");
    (void) N_("amulet");
    (void) N_("spellbook");
    (void) N_("gem");
    (void) N_("stone");

    /* === Armor descriptions === */
    (void) N_("leather");
    (void) N_("studded leather");
    (void) N_("ring mail");
    (void) N_("scale mail");
    (void) N_("chain mail");
    (void) N_("plate mail");
    (void) N_("helmet");
    (void) N_("helm");
    (void) N_("shield");
    (void) N_("cloak");
    (void) N_("gloves");
    (void) N_("boots");

    /* === Weapon descriptions === */
    (void) N_("sword");
    (void) N_("dagger");
    (void) N_("knife");
    (void) N_("axe");
    (void) N_("bow");
    (void) N_("arrow");
    (void) N_("spear");
    (void) N_("lance");
    (void) N_("mace");
    (void) N_("flail");
    (void) N_("hammer");
    (void) N_("staff");
    (void) N_("whip");
    (void) N_("club");
    (void) N_("quarterstaff");
    (void) N_("crossbow");
    (void) N_("bolt");

    /* === Tool descriptions === */
    (void) N_("lamp");
    (void) N_("lantern");
    (void) N_("candle");
    (void) N_("mirror");
    (void) N_("key");
    (void) N_("lock pick");
    (void) N_("sack");
    (void) N_("bag");
    (void) N_("chest");
    (void) N_("box");
    (void) N_("horn");
    (void) N_("whistle");
    (void) N_("flute");
    (void) N_("harp");
    (void) N_("drum");
    (void) N_("bell");

    /* === Food descriptions === */
    (void) N_("food ration");
    (void) N_("apple");
    (void) N_("orange");
    (void) N_("pear");
    (void) N_("melon");
    (void) N_("banana");
    (void) N_("carrot");
    (void) N_("clove of garlic");
    (void) N_("cream pie");
    (void) N_("candy bar");
    (void) N_("fortune cookie");
    (void) N_("pancake");
    (void) N_("lembas wafer");
    (void) N_("cram ration");
    (void) N_("K-ration");
    (void) N_("C-ration");
    (void) N_("tin");
    (void) N_("egg");
    (void) N_("corpse");
    (void) N_("tripe ration");
    (void) N_("meatball");
    (void) N_("meat stick");
    (void) N_("huge chunk of meat");
    (void) N_("kelp frond");
    (void) N_("eucalyptus leaf");

    /* === Gem descriptions === */
    (void) N_("dilithium crystal");
    (void) N_("worthless piece of blue glass");
    (void) N_("worthless piece of red glass");
    (void) N_("worthless piece of yellow glass");
    (void) N_("worthless piece of green glass");
    (void) N_("worthless piece of white glass");
    (void) N_("worthless piece of orange glass");
    (void) N_("worthless piece of violet glass");
    (void) N_("flint");
    (void) N_("rock");
    (void) N_("loadstone");
    (void) N_("luckstone");
    (void) N_("touchstone");
    (void) N_("flint stone");

    /* === Color adjectives (for item names) === */
    (void) N_("red");
    (void) N_("blue");
    (void) N_("green");
    (void) N_("gray");
    (void) N_("grey");
    (void) N_("silver");
    (void) N_("bright");
    (void) N_("light");

    /* === Action verbs (for getobj messages) === */
    (void) N_("drink");
    (void) N_("eat");
    (void) N_("throw");
    (void) N_("wear");
    (void) N_("take off");
    (void) N_("put on");
    (void) N_("drop");
    (void) N_("rub");
    (void) N_("grease");
    (void) N_("use or apply");
    (void) N_("invoke");
    (void) N_("charge");
    (void) N_("name");
    (void) N_("call");
    (void) N_("remove");
    (void) N_("open");
    (void) N_("sacrifice");
    (void) N_("write with");
    (void) N_("identify");
    (void) N_("adjust");
    (void) N_("split");
    (void) N_("stash");
    (void) N_("tip");
    (void) N_("dip");
    (void) N_("read");
    (void) N_("zap");
    (void) N_("quiver");
    (void) N_("fire");
    (void) N_("offer");
    (void) N_("wield");
    (void) N_("quaff");

    /* === Item names === */
    (void) N_("gold piece");
    (void) N_("gold pieces");
    (void) N_("rock");
    (void) N_("rocks");
    (void) N_("arrow");
    (void) N_("arrows");
    (void) N_("corpse");
    (void) N_("gem");
    (void) N_("gems");

    /* === Pet/monster descriptions === */
    (void) N_("your kitten");
    (void) N_("your little dog");
    (void) N_("your pony");
    (void) N_("your cat");
    (void) N_("your dog");
    (void) N_("your horse");
    (void) N_("your large cat");
    (void) N_("your large dog");
    (void) N_("your warhorse");

    /* === Status line format strings (botl.c initblstats) === */
    (void) N_(" St:%s");
    (void) N_(" Dx:%s");
    (void) N_(" Co:%s");
    (void) N_(" In:%s");
    (void) N_(" Wi:%s");
    (void) N_(" Ch:%s");
    (void) N_(" Pw:%s");
    (void) N_(" Xp:%s");
    (void) N_(" AC:%s");
    (void) N_(" HP:%s");
    (void) N_(" HD:%s");
    (void) N_(" T:%s");
    (void) N_(" S:%s");

    /* === UI messages (invent.c) === */
    (void) N_("There %s %s here.");
    (void) N_("Things that are here:");
    (void) N_("Other things that are here:");
    (void) N_("Things that you feel here:");
    (void) N_("Other things that you feel here:");
    (void) N_("Things that are under the %s here:");
    (void) N_("Things that are buried %s:");

    /* === Common strings (decl.h macros) === */
    (void) N_("Something");
    (void) N_("something");

    /* === Item state descriptors (objnam.c) === */
    /* Erosion levels */
    (void) N_("very ");
    (void) N_("thoroughly ");
    (void) N_("rusty ");
    (void) N_("cracked ");
    (void) N_("burnt ");
    (void) N_("corroded ");
    (void) N_("rotted ");
    /* Erodeproof types */
    (void) N_("fixed ");
    (void) N_("rustproof ");
    (void) N_("corrodeproof ");
    (void) N_("fireproof ");
    (void) N_("tempered ");
    (void) N_("rotproof ");
    /* Item states */
    (void) N_("empty ");
    (void) N_("cursed ");
    (void) N_("blessed ");
    (void) N_("uncursed ");
    (void) N_("trapped ");
    (void) N_("broken ");
    (void) N_("locked ");
    (void) N_("unlocked ");
    (void) N_("greased ");
    (void) N_("poisoned ");
    (void) N_("pair of ");
    (void) N_("moist ");
    (void) N_("wet ");
    (void) N_("diluted ");
    (void) N_("holy ");
    (void) N_("unholy ");
    /* Item type names */
    (void) N_("potion");
    (void) N_(" potion");
    (void) N_(" of ");
    (void) N_("heavy iron ball");
    (void) N_("scroll");
    (void) N_(" scroll");
    (void) N_(" labeled ");
    (void) N_("wand");
    (void) N_("wand of %s");
    (void) N_("%s wand");
    (void) N_("book");
    (void) N_("novel");
    (void) N_("%s book");
    (void) N_("spellbook");
    (void) N_("spellbook of ");
    (void) N_("%s spellbook");
    (void) N_("ring");
    (void) N_("ring of %s");
    (void) N_("%s ring");
    (void) N_("amulet");
    (void) N_("pair of ");
    (void) N_("set of ");
    (void) N_("stone");
    (void) N_(" stone");
    (void) N_("gem");
    (void) N_("%s %s");

    /* === Monster names (pmnames) === */
    (void) N_("kitten");
    (void) N_("little dog");
    (void) N_("pony");
    (void) N_("cat");
    (void) N_("dog");
    (void) N_("horse");
    (void) N_("large cat");
    (void) N_("large dog");
    (void) N_("warhorse");
    (void) N_("newt");
    (void) N_("gecko");
    (void) N_("iguana");
    (void) N_("lizard");
    (void) N_("jackal");
    (void) N_("fox");
    (void) N_("coyote");
    (void) N_("wolf");
    (void) N_("warg");
    (void) N_("winter wolf");
    (void) N_("hell hound");
    (void) N_("goblin");
    (void) N_("hobgoblin");
    (void) N_("orc");
    (void) N_("hill orc");
    (void) N_("Mordor orc");
    (void) N_("Uruk-hai");
    (void) N_("orc shaman");
    (void) N_("orc-captain");
    (void) N_("elf");
    (void) N_("Woodland-elf");
    (void) N_("Green-elf");
    (void) N_("Grey-elf");
    (void) N_("elf-lord");
    (void) N_("Elvenking");
    (void) N_("dwarf");
    (void) N_("dwarf lord");
    (void) N_("dwarf king");
    (void) N_("gnome");
    (void) N_("gnome lord");
    (void) N_("gnome king");
    (void) N_("gnomish wizard");
    (void) N_("kobold");
    (void) N_("large kobold");
    (void) N_("kobold lord");
    (void) N_("kobold shaman");
    (void) N_("rat");
    (void) N_("sewer rat");
    (void) N_("giant rat");
    (void) N_("bat");
    (void) N_("giant bat");
    (void) N_("vampire bat");
    (void) N_("human");
    (void) N_("invisible ");
}

/*obj_descr_i18n.c*/

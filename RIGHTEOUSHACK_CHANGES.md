# RighteousHack customization inventory

Checklist of every customization on `righteoushack-0.1` relative to the
fork point with upstream NetHack, commit `94e5f7b861` (2025-11-28).
This file was written **before** merging tag `NetHack-5.0.0_Released`
(`16ff591153`) so the upgrade can be checked against it.

Spellings below are the owner's, including `elixer`, `exlier`,
`doubled-edged`, `trusty sandles`, `firey`, `wiggy gain ability`,
and `PSALM 91`. They are intentional content (or at least content the
owner shipped) and must not be "corrected" back to vanilla NetHack.

Status values:

- **preserved** — still present, same place, same behavior
- **ported** — moved onto a NetHack 5.0.0 API, filename, or pack list
- **needs owner decision** — kept as written, but unfinished or inconsistent
- **lost** — not used; none of the inventory items were dropped

The per-id results are in section 20. Nothing in this inventory was lost.
A short human-readable log of changes is in VERSION_CHANGES.txt.

---

## 1. Version and identity

| ID | Item | Where | Notes |
|----|------|-------|-------|
| ID-001 | Banner `RighteousHack, Copyright 2025` / `By jonk1984.` | `include/patchlevel.h` | Replaces the NetHack copyright banner lines A and B. |
| ID-002 | `WIZARD_NAME` is `"prophet"` | `include/config.h` | Ignored when SYSCF is on. |
| ID-003 | README rewritten as the RighteousHack 0.1 introduction | `README` | Still describes the base as NetHack 3.7.0 at inventory time. |
| ID-004 | `VERSION_MAJOR 3` / `VERSION_MINOR 7` / `EDITLEVEL 131` / `NH_STATUS_WIP` | `include/patchlevel.h` | Owner's 3.7 version. **5.0.0's version numbers replace this.** Old save and bones files are not compatible. |

## 2. Playable roles

`NUM_ROLES` is 7 (`include/hack.h`). `src/role.c` keeps seven roles.
Removed as playable roles: Barbarian, Caveman, Monk, Ranger, Samurai,
Tourist. Renamed in place (same `PM_` monster, new title and file code):

| ID | Vanilla role | RighteousHack name | File code | Quest artifact | Player monster |
|----|--------------|--------------------|-----------|----------------|----------------|
| ID-010 | Archeologist | Royal Historian | `His` | Apostolic Writing Kit | `PM_ARCHEOLOGIST` |
| ID-011 | Knight | Knight | `Kni` | Riding Blanket of the Heavenly Host | `PM_KNIGHT` |
| ID-012 | Healer | Healer | `Hea` | Handkerchief of the Apostle Paul | `PM_HEALER` |
| ID-013 | Priest | Priest | `Pri` | Golden Censer of Incense | `PM_CLERIC` |
| ID-014 | Rogue | Adventurer | `Adv` | Scarlet Cord of Rahab | `PM_ROGUE` |
| ID-015 | Valkyrie | Champion | `Chp` | still Orb of Fate | `PM_VALKYRIE` |
| ID-016 | Wizard | Prophet | `Pht` | Staff of Signs and Wonders | `PM_WIZARD` |

Rank titles, home level names, and quest level names are in `src/role.c`.
Deities for Historian, Knight, Healer, Adventurer, Champion, and Prophet
are `"the Lord"`, `"God"`, `"the Almighty"`. Priest leaves deities `0,0,0`
(filled from another role at startup).

Every playable role is `MH_HUMAN` plus `ROLE_LAWFUL` only. Neutral and
chaotic are not offered. Gender titles that used to differ (Priestess,
Cavewoman, and so on) are gone; the listed roles use one title.

| ID | Item | Where |
|----|------|-------|
| ID-017 | `NUM_RACES` is 1; only the human race remains in `races[]` | `include/hack.h`, `src/role.c` |
| ID-018 | Historian leader is Zenodotus, not Lord Carnarvon | `src/role.c`, `include/monsters.h` (`ZENODOTUS`) |
| ID-019 | Role file codes `His`, `Adv`, `Chp`, `Pht` must match `dat/quest.lua` keys and the `dat/*-*.lua` level names those quests load | `dat/quest.lua` |

`NameChanges.xlsx` sheet `Roles` is an earlier sketch (Archeologist→Historian,
Priest→Healer, Rogue→Adventurer, Valkyrie→Champion, Wizard→Disciple) and
does **not** match the code. The code wins.

## 3. New and renamed monsters

| ID | Item | Where |
|----|------|-------|
| ID-030 | `firey steed of the Heavenly Host` (`FIREY_STEED_OF_HEAVEN`), unicorn class, flying, peaceful | `include/monsters.h` |
| ID-031 | Unique human `Zenodotus` (`ZENODOTUS`), Historian quest leader | `include/monsters.h` |

No other monster names were changed. Vanilla monsters for removed roles
are still defined.

## 4. Object classes and player-facing class names

| ID | Vanilla | RighteousHack | Symbol enum | Where |
|----|---------|---------------|-------------|-------|
| ID-040 | potion / potions | elixer / elixers | `POTION` / `!` | `include/defsym.h`, `include/objects.h` |
| ID-041 | spellbook / spellbooks | prophetic book / prophetic books | `SPBOOK` / `+` | same |
| ID-042 | wand / wands | rod / rods | `WAND` / `/` | same |
| ID-043 | new class: anointing oil / bottle of anointing oil | `ANOINTING`, glyph `0xED`, `S_anointing_oil` | `include/defsym.h`, `include/objects.h` |

Internal class enums stay `POTION_CLASS`, `SPBOOK_CLASS`, `WAND_CLASS`.
The new class is `ANOINTING_CLASS`. `LAST_GENERIC` is `GENERIC_ANOINTING`.

`src/drawing.c` `def_r_oc_syms[]` was **not** given an anointing entry.
The extra slot is zero-filled. Pre-existing gap, not introduced by the upgrade.

Comments across the sources were edited to say prophetic book, elixer,
rod, or verse. That wording is part of the customization wherever a
player can see it. Internal macro `BZ_U_SPELL` / `BZ_M_SPELL` was renamed
`BZ_U_PBOOK` / `BZ_M_PBOOK` in `include/hack.h` and at the call sites in
`src/spell.c`, `src/zap.c`, `src/priest.c`, `src/timeout.c`, `src/mcastu.c`.
That rename is not player-facing. On 5.0 it must stay consistent with
every call site.

The spellbook table macro in `include/objects.h` was renamed from `SPELL`
to `PBOOK`. `#undef SPELL` was left behind, so `PBOOK` is not undefined.
Local to that header.

## 5. Objects added

All in `include/objects.h`. Artifact bases are in section 7.

| ID | Name | Enum | Kind |
|----|------|------|------|
| ID-050 | doubled-edged blade | `TWO_EDGED_SWORD` | weapon, long sword skill |
| ID-051 | almond-wood walking stick ("walking stick") | `ALMOND_STAVE` | quarterstaff skill |
| ID-052 | divine covering | `DIVINE_COVERING` | helm, magical breathing |
| ID-053 | gleaming armor | `PLATE_OF_RIGHT` | body armor, antimagic |
| ID-054 | humble sash | `BELT` | shirt slot, see invisible |
| ID-055 | ancient defense | `ANCIENT_SHIELD` | shield, free action |
| ID-056 | trusty sandals ("trusty sandles") | `SANDALS` | boots, water walking |
| ID-057 | thurible | `THURIBLE` | tool |
| ID-058 | torn piece of cloth | `PIECE_OF_CLOTH` | tool |
| ID-059 | rope | `ROPE` | weapon-tool, whip skill |
| ID-060 | magic marker renamed writing tablet | `MAGIC_MARKER` | same object, new name |

Wear and attribute behavior for sandals, divine covering, ancient shield,
and the belt is in `src/do_wear.c` (speed while worn for sandals; the
other three adjust attributes on wear).

## 6. Potions moved to anointing oils

These potion types are commented out of `POTION()` and recreated as
`ANOINTING()`. Code must use the `ANO_*` enums. `POT_*` for these names
does not exist.

| ID | Removed potion | Anointing oil | Enum |
|----|----------------|---------------|------|
| ID-070 | gain ability | wiggy gain ability | `ANO_GAIN_ABILITY` |
| ID-071 | restore ability | restore ability | `ANO_RESTORE_ABILITY` |
| ID-072 | speed | speed | `ANO_SPEED` |
| ID-073 | invisibility | invisibility | `ANO_INVISIBILITY` |
| ID-074 | see invisible | see invisible | `ANO_SEE_INVISIBLE` |
| ID-075 | healing | healing | `ANO_HEALING` |
| ID-076 | extra healing | extra healing | `ANO_EXTRA_HEALING` |
| ID-077 | gain level | gain level | `ANO_GAIN_LEVEL` |
| ID-078 | enlightenment | enlightenment | `ANO_ENLIGHTENMENT` |
| ID-079 | monster detection | monster detection | `ANO_MONSTER_DETECTION` |
| ID-080 | object detection | object detection | `ANO_OBJECT_DETECTION` |
| ID-081 | gain energy | gain energy | `ANO_GAIN_ENERGY` |
| ID-082 | full healing | full healing | `ANO_FULL_HEALING` |

Potions that remain potions include confusion, blindness, paralysis,
healing-adjacent ones that were **not** in the list above (booze, sickness,
acid, oil, fruit juice, water, polymorph, and the rest of the vanilla
potion list that was not commented out).

`ID-083` Command `M-a` / `anoint` calls `doanoint()` (`src/cmd.c`,
`src/potion.c`, `include/extern.h`). Quaffing an anointing oil is a
separate path (`doanointeffect`, `aeffects`) with scripture `pline`s
(Isaiah 11:2, Joel 2:25, 2 Samuel 22:34, Psalm 27:5, Psalm 119:18,
Isaiah 53:5, Exodus 15:26, 1 Samuel 2:8, John 16:13, Hebrews 4:13,
Psalm 103:5, Psalm 147:3).

`ID-084` Starting inventory in `src/u_init.c` uses `ANO_HEALING` /
`ANO_EXTRA_HEALING` for Healer, Monk, and Tourist, still tagged
`POTION_CLASS` in the `trobj` row. Healer `knows_object(ANO_FULL_HEALING)`.
Monk and Tourist are not playable; those rows still exist for the
monster-role tables.

`ID-085` `src/mkobj.c`, `src/objnam.c`, `src/muse.c`, `src/shknam.c`,
`src/zap.c`, and `src/sp_lev.c` know the anointing class and the
`elixer` word. `objnam.c` parses `elixer of`, `rod of`, `prophetic book of`,
`verse of`, and `anointing oil of`.

## 7. Scrolls renamed to verses

The object class is still `SCROLL`. Player-facing names and the label
(the old randomized scroll appearance string) are scripture references.
Messages in `src/read.c` call them verses. `src/objnam.c` accepts
`verse of`.

| ID | Enum | Name | Label |
|----|------|------|-------|
| ID-090 | `SCR_ENCHANT_ARMOR` | spiritual armor | PSALM 91:4 |
| ID-091 | `SCR_DESTROY_ARMOR` | no covering | JOB 26:6 |
| ID-092 | `SCR_CONFUSE_MONSTER` | confound foes | PSALM 35:4 |
| ID-093 | `SCR_SCARE_MONSTER` | righteous fear | PROVERBS 28:1 |
| ID-094 | `SCR_REMOVE_CURSE` | remove curse | GALATIANS 3:13 |
| ID-095 | `SCR_ENCHANT_WEAPON` | iron sharpens iron | PROVERBS 27:17 |
| ID-096 | `SCR_CREATE_MONSTER` | heavenly host | PSALM 68:17 |
| ID-097 | `SCR_TAMING` | subdue beast | GENESIS 1:28 |
| ID-098 | `SCR_GENOCIDE` | judgement to the wicked | DEUTERONOMY 7:2 |
| ID-099 | `SCR_LIGHT` | light | PSALM 119:105 |
| ID-100 | `SCR_TELEPORTATION` | dove wings | PSALM 55:6 |
| ID-101 | `SCR_GOLD_DETECTION` | render unto caesar | MATTHEW 22:21 |
| ID-102 | `SCR_FOOD_DETECTION` | reveal mana | EXODUS 16:4 |
| ID-103 | `SCR_IDENTIFY` | discernment | HEBREWS 4:12 |
| ID-104 | `SCR_MAGIC_MAPPING` | path revealed | ISAIAH 43:19 |
| ID-105 | `SCR_AMNESIA` | former things forgotten | ISAIAH 65:17 |
| ID-106 | `SCR_FIRE` | consuming fire | 1 KINGS 18:38 |
| ID-107 | `SCR_EARTH` | shaken earth | MATTHEW 27:51 |
| ID-108 | `SCR_PUNISHMENT` | correction | HEBREWS 12:6 |
| ID-109 | `SCR_CHARGING` | renewal | ISAIAH 40:31 |
| ID-110 | `SCR_STINKING_CLOUD` | fire and brimstone | GENESIS 19:24 |

Blank paper and mail stay scrolls. `NameChanges.xlsx` sheet `Verse`
matches these names (teleportation is "wings of the dove" on the sheet
and "dove wings" in code; gold detection is "render unto ceasar" on the
sheet and "render unto caesar" in code). The code wins.

## 8. Wands renamed to rods

Macro in `include/objects.h` is `ROD`, class remains `WAND_CLASS`.
Ray order for `buzz()` is unchanged: magic missile, fire, cold, sleep,
death, lightning.

| ID | Enum | Name | Appearance |
|----|------|------|------------|
| ID-120 | `WAN_LIGHT` | light | glass |
| ID-121 | `WAN_SECRET_DOOR_DETECTION` | reveal secret things | balsa |
| ID-122 | `WAN_ENLIGHTENMENT` | understanding | crystal |
| ID-123 | `WAN_CREATE_MONSTER` | heavenly host | maple |
| ID-124 | `WAN_WISHING` | ask and ye shall receive | pine |
| ID-125 | `WAN_NOTHING` | nothing | oak |
| ID-126 | `WAN_STRIKING` | rebuke | ebony |
| ID-127 | `WAN_MAKE_INVISIBLE` | divine concealment | marble |
| ID-128 | `WAN_SLOW_MONSTER` | staggering wine | tin |
| ID-129 | `WAN_SPEED_MONSTER` | quickening | brass |
| ID-130 | `WAN_UNDEAD_TURNING` | resist evil | copper |
| ID-131 | `WAN_POLYMORPH` | transformation | silver |
| ID-132 | `WAN_CANCELLATION` | cancellation | platinum |
| ID-133 | `WAN_TELEPORTATION` | dove wings | iridium |
| ID-134 | `WAN_OPENING` | door shall be open | zinc |
| ID-135 | `WAN_LOCKING` | seal entrance | aluminum |
| ID-136 | `WAN_PROBING` | revelation | uranium |
| ID-137 | `WAN_DIGGING` | move mountains | iron |
| ID-138 | `WAN_MAGIC_MISSILE` | hidden arrows | steel |
| ID-139 | `WAN_FIRE` | consuming fire | hexagonal |
| ID-140 | `WAN_COLD` | northern wind | short |
| ID-141 | `WAN_SLEEP` | tranquility | runed |
| ID-142 | `WAN_DEATH` | final judgement | long |
| ID-143 | `WAN_LIGHTNING` | lightning flash | curved |

A commented-out copy of the old wand names is left under the live list.
Shuffle appearances `forked` / `spiked` / `jeweled` remain.

`ID-144` Command `z` is described as "zap a rod" (`src/cmd.c`).

## 9. Prophetic books (spellbooks)

`PBOOK()` names. Enums are unchanged, so spell effects stay on the same
`SPE_*` values. `src/spell.c` calls them prophecies: the cast command
description is meditate, skill group names are spiritual warfare,
discernment, blessing, prayer, deliverance, and creation.

| ID | Enum | Name |
|----|------|------|
| ID-150 | `SPE_DIG` | move mountains |
| ID-151 | `SPE_MAGIC_MISSILE` | hidden arrows |
| ID-152 | `SPE_FIREBALL` | fiery tempest |
| ID-153 | `SPE_CONE_OF_COLD` | northern wind |
| ID-154 | `SPE_SLEEP` | tranquility |
| ID-155 | `SPE_FINGER_OF_DEATH` | finger of judgement |
| ID-156 | `SPE_LIGHT` | light |
| ID-157 | `SPE_DETECT_MONSTERS` | reveal foes |
| ID-158 | `SPE_HEALING` | healing |
| ID-159 | `SPE_KNOCK` | door shall be open |
| ID-160 | `SPE_FORCE_BOLT` | rebuke |
| ID-161 | `SPE_CONFUSE_MONSTER` | bewilder |
| ID-162 | `SPE_CURE_BLINDNESS` | cure blindness |
| ID-163 | `SPE_DRAIN_LIFE` | noisome pestilence |
| ID-164 | `SPE_SLOW_MONSTER` | staggering wine |
| ID-165 | `SPE_WIZARD_LOCK` | seal entrance |
| ID-166 | `SPE_CREATE_MONSTER` | heavenly host |
| ID-167 | `SPE_DETECT_FOOD` | reveal sustenance |
| ID-168 | `SPE_CAUSE_FEAR` | righteous fear |
| ID-169 | `SPE_CLAIRVOYANCE` | far sight |
| ID-170 | `SPE_CURE_SICKNESS` | cure sickness |
| ID-171 | `SPE_CHARM_MONSTER` | pacification |
| ID-172 | `SPE_HASTE_SELF` | quickening |
| ID-173 | `SPE_DETECT_UNSEEN` | piercing sight |
| ID-174 | `SPE_LEVITATION` | ascension |
| ID-175 | `SPE_EXTRA_HEALING` | abundant healing |
| ID-176 | `SPE_RESTORE_ABILITY` | divine ability |
| ID-177 | `SPE_INVISIBILITY` | divine concealment |
| ID-178 | `SPE_DETECT_TREASURE` | render unto Caesar |
| ID-179 | `SPE_REMOVE_CURSE` | remove curse |
| ID-180 | `SPE_MAGIC_MAPPING` | path revealed |
| ID-181 | `SPE_IDENTIFY` | discernment |
| ID-182 | `SPE_TURN_UNDEAD` | resist evil |
| ID-183 | `SPE_POLYMORPH` | transformation |
| ID-184 | `SPE_TELEPORT_AWAY` | banishment |
| ID-185 | `SPE_CREATE_FAMILIAR` | angelic help |
| ID-186 | `SPE_CANCELLATION` | cancellation |
| ID-187 | `SPE_PROTECTION` | protection |
| ID-188 | `SPE_JUMPING` | leaping like a deer |
| ID-189 | `SPE_STONE_TO_FLESH` | flesh renewed |
| ID-190 | `SPE_CHAIN_LIGHTNING` | lightning flash |

`SPE_FLAME_SPHERE`, `SPE_FREEZE_SPHERE`, `SPE_BLANK_PAPER`, `SPE_NOVEL`,
and `SPE_BOOK_OF_THE_DEAD` keep vanilla names. The Book of the Dead
comment calls it a prophetic book.

`ID-191` Command `Z` is `meditate` ("zap (meditate) on a prophecy")
instead of `cast` (`src/cmd.c`).

`ID-192` Wizard-mode command names that used to start with `wiz` start
with `pro` (`src/cmd.c`): `proborn`, `probury`, `procast`, `procustom`,
`prodetect`, `prodispmacros`, `profliplevel`, `progenesis`, `proidentify`,
`prointrinsic`, `prokill`, `prolevelport`, `proloaddes`, `proloadlua`,
`promakemap`, `promap`, `promondiff`, `prorumorcheck`, `proseenv`,
`prosmell`, `protelekinesis`, `prowhere`, `prowish`.

## 10. Artifacts

New property bits in `include/artifact.h`: `SPFX_STONING` (bit 28),
`SPFX_SLEEP_RES` (bit 30), `SPFX_BLIND_RES` (bit 31). New invoke ids:
`QUILL_INVOKE`, `HANDKERCHIEF`, `CENSERAID`, `CORD_HIDE`, `COLUMN_OF_FIRE`.
Helpers: `IS_STAFF_OF_WONDERS`, `IS_GOLDEN_CENSER`.

| ID | Artifact | Base | Role gate in `artilist.h` | Behavior |
|----|----------|------|---------------------------|----------|
| ID-200 | Apostolic Writing Kit | writing tablet (`MAGIC_MARKER`) | `PM_ARCHEOLOGIST` | Unlimited writing (`src/write.c`); invoke gives paper |
| ID-201 | Riding Blanket of the Heavenly Host | saddle | `PM_KNIGHT` | Apply transforms a horse into the firey steed; invoke tames nearby horses (`src/apply.c`, `src/artifact.c`) |
| ID-202 | Handkerchief of the Apostle Paul | torn piece of cloth | `PM_KNIGHT` in `artilist.h`, but the Healer quest reward in `src/role.c` | Carry grants disease resistance; invoke heals and pushes monsters (`mhurtle_giant` in `src/dothrow.c`) |
| ID-203 | Golden Censer of Incense | thurible | `PM_CLERIC` | Prayer timeout, light radius 5 with no burnout (`src/timeout.c`), invoke engraves and teleports |
| ID-204 | Scarlet Cord of Rahab | rope | `PM_ROGUE` | Apply climbs; invoke toggles invisibility. Climb/fall messages in `src/hack.c` |
| ID-205 | Staff of Signs and Wonders | almond-wood walking stick | `PM_WIZARD` | Apply changes water/floor; invoke is a 5x5 column of fire (`src/apply.c`, `src/artifact.c`). Prayer recharges it (`src/pray.c`) |
| ID-206 | Gospel of Peace | trusty sandals | not role-bound | Water walking, haste, fire resistance; invoke tames |
| ID-207 | Breastplate of Righteousness | gleaming armor | not role-bound | Half spell damage, drain resistance, magic resistance, poison carry |
| ID-208 | Belt of Truth | humble sash | not role-bound | Searching, see invisible, sleep/blind/hallucination resistance; invoke enlightens |
| ID-209 | Sword of the Spirit | doubled-edged blade | not role-bound | Reflection, stun attack, warning |
| ID-210 | Cap of Salvation | divine covering | not role-bound | Half physical damage, regeneration, disintegration carry, healing invoke |
| ID-211 | Shield of Faith | ancient defense | not role-bound | Stoning resistance, fire defense, magic missile carry, healing invoke |

`ID-212` needs owner decision, pre-existing: Handkerchief's `artilist.h`
role is `PM_KNIGHT` while `src/role.c` gives that artifact to the Healer.
Left unchanged.

`ID-213` Champion still receives `ART_ORB_OF_FATE`. No replacement
artifact was added for that role.

Vanilla artifacts are still in `include/artilist.h`.

## 11. Prayer, altars, and conduct

| ID | Item | Where |
|----|------|-------|
| ID-220 | Prayer rewritten: lawful-only altar check, scripture messages, "Be still, and know that I am God." (Psalm 46:10), Jeremiah 33:3, restoration and crowning gifts that can include the gospel-of-peace sandals | `src/pray.c` |
| ID-221 | Staff of Signs and Wonders extends prayer favor and can be recharged by prayer | `src/pray.c` |
| ID-222 | Offering a corpse is refused as human sacrifice, with Leviticus 18:21 and Psalm 19:13 | `src/pray.c` |
| ID-223 | `desecrate_altar()` declaration removed. `src/dig.c` and `src/music.c` print `placeholder for desecrate_altar` instead of calling it | `include/extern.h`, `src/dig.c`, `src/music.c` |
| ID-224 | Lawful anointing title is **"The Servant of the Lord"** in `src/pray.c` and `src/insight.c`. | those files |
| ID-225 | First-kill livelog text and a comment `RighteousHack - get rid of murderous messages` | `src/mon.c` |

## 12. Elbereth replaced by PSALM 91

The ward string compared by `sengr_at()` is `PSALM 91` instead of
`Elbereth`.

| ID | Where |
|----|-------|
| ID-230 | `src/engrave.c`, `src/mon.c`, `src/monmove.c`, `src/mhitu.c`, `src/teleport.c`, `src/uhitm.c` |
| ID-231 | `dat/engrave.txt` (the random engraving list) |
| ID-232 | `dat/oracles.txt` |
| ID-233 | `dat/rumors.fal`, `dat/rumors.tru` |
| ID-234 | `dat/Mon-loca.lua`, `dat/castle.lua`, `dat/soko1-1.lua`, `dat/soko1-2.lua` engravings |
| ID-235 | `dat/data.base` Bilbo line now says the song is to `PSALM 91` |

## 13. Quests and levels

| ID | Item | Where |
|----|------|-------|
| ID-240 | New Historian quest levels | `dat/His-strt.lua`, `His-loca.lua`, `His-fila.lua`, `His-filb.lua`, `His-goal.lua` |
| ID-241 | New Prophet quest levels | `dat/Pht-strt.lua`, `Pht-loca.lua`, `Pht-fila.lua`, `Pht-filb.lua`, `Pht-goal.lua` |
| ID-242 | Branch "Treasure Room of Alex" (`bonetag` `A`, mazelike, neutral) with level `his-treasure-1` | `dat/dungeon.lua`, `dat/his-treasure-1.lua` |
| ID-243 | Down stairs can be blocked until the narrow-gate level is available; messages cite Matthew 7:13–14 | `src/do.c` |
| ID-244 | `dat/quest.lua` keys: `Arc`→`His`, `Rog`→`Adv`, `Val`→`Chp`, `Wiz`→`Pht`. `Bar` text removed. `Cav`, `Mon`, `Ran`, `Sam`, `Tou` text kept even though those roles are not playable. `common` quest chat rewritten with scripture. | `dat/quest.lua` |
| ID-245 | Quest goal items swapped for the new artifacts | `dat/Arc-goal.lua` (writing tablet), `dat/Hea-goal.lua` (cloth; wand line is `rod of lightning flash`), `dat/Kni-goal.lua` (saddle), `dat/Pri-goal.lua` (thurible), `dat/Rog-goal.lua` (rope), `dat/Wiz-goal.lua` (almond-wood walking stick, spe 8) |
| ID-246 | Knight quest start map replaced | `dat/Kni-strt.lua` |
| ID-247 | Level object names updated for verses, rods, elixers, anointing oils, prophetic books | `dat/castle.lua`, `dat/medusa-3.lua`, `dat/minend-2.lua`, `dat/minetn-1.lua`, `dat/soko1-1.lua`, `dat/soko1-2.lua`, `dat/soko4-1.lua`, `dat/soko4-2.lua`, `dat/themerms.lua`, `dat/tower2.lua`, `dat/tut-1.lua`, `dat/Rog-loca.lua` |

Notable level strings that must survive:

- castle: `ask and ye shall receive`, `anointing oil of gain level`, `verse of righteous fear`, engraving `PSALM 91`
- mines end: `elixer of booze`, `anointing oil of object detection`
- minetown: `rod of rebuke`, `rod of hidden arrows`
- sokoban: `verse of shaken earth`, `verse of righteous fear`
- tower: `prophetic book of divine concealment`, `northern wind`, `angelic help`, `far sight`, `pacification`, `flesh renewed`, `transformation`
- tutorial: `prophetic book of light`, `anointing oil of object detection`

## 14. Other source behavior

| ID | Item | Where |
|----|------|-------|
| ID-260 | `mhurtle_giant()` for the handkerchief push | `src/dothrow.c`, `include/extern.h` |
| ID-261 | Rope use, staff-of-wonders apply, blanket apply | `src/apply.c` |
| ID-262 | Artifact invoke implementations | `src/artifact.c` |
| ID-263 | Ice walking for humble sandals / Gospel of Peace; falling without a rope; Hebrews 12:6 when a jump off a wall fails | `src/hack.c` |
| ID-264 | `Is_thurible()` | `include/obj.h` |
| ID-265 | `rndexp()` comment says elixer | `src/exper.c` |
| ID-266 | Tribute line: `hankerchief` corrected to `handkerchief` | `dat/tribute` |

Files whose only owner change is this terminology (spellbook / potion /
wand / scroll wording, or `Elbereth`→`PSALM 91`, or `BZ_*_SPELL`→
`BZ_*_PBOOK`) include: `src/attrib.c`, `src/decl.c`, `src/detect.c`,
`src/display.c`, `src/do_name.c`, `src/eat.c`, `src/end.c`, `src/explode.c`,
`src/fountain.c`, `src/glyphs.c`, `src/insight.c` (except the Elbereth
title, which was left), `src/mail.c`, `src/makemon.c`, `src/mcastu.c`,
`src/mklev.c`, `src/mkmaze.c`, `src/mthrowu.c`, `src/music.c` (plus the
desecrate placeholder), `src/o_init.c`, `src/options.c`, `src/pager.c`,
`src/pickup.c`, `src/priest.c`, `src/restore.c`, `src/save.c`, `src/shk.c`,
`src/shknam.c`, `src/sp_lev.c` (plus the anointing-oil name prefix),
`src/topten.c`, `src/trap.c`, `src/vision.c`, `src/allmain.c`,
`include/display.h`, `include/objclass.h`, `include/spell.h`,
`include/youprop.h`, `include/extern.h` (plus the new prototypes).

## 15. Tools, editor, and Windows build files the owner added

These are new files. They are not part of the NetHack build. They must
remain in the tree.

| ID | Path | What it is |
|----|------|------------|
| ID-270 | `tools/map_edit.html`, `tools/map_edit.css`, `tools/loadLUAMap.js`, `tools/menu_options.js`, `tools/output.js`, `tools/feature_def.js`, `tools/monster_def.js`, `tools/object_def.js`, `tools/terrain_def.js`, `tools/color_def.js` | Browser map editor |
| ID-271 | `tools/quest_edit.html` | Browser quest editor |
| ID-272 | `tools/parse_colors.py`, `tools/parse_monsters.py`, `tools/parse_monsters2.py`, `tools/parse_objects.py` | Parsers that regenerate the `*_def.js` data |
| ID-273 | `tools/bak.html` | Editor backup page |
| ID-274 | `.vscode/launch.json`, `.vscode/settings.json`, `.vscode/tasks.json` | VS Code debug/build |
| ID-275 | `build.bat` | Windows menu: full `nmake` package, quick rebuild, or launch `binary\nethack.exe -D -u prophet`. Calls VS 2022 `vcvarsall.bat` x64, then `src/Makefile` (and `Makefile.vstudio` if the binary is missing). |
| ID-276 | `GIT Instructions.txt`, `NameChanges.xlsx` | Owner notes and the name spreadsheet |
| ID-277 | `allObjects.js`, `color_def.js`, `monster_def.js`, `output.js` | Editor data copies at the repo root |

## 16. Submodules and Lua

| ID | Item | Notes |
|----|------|-------|
| ID-280 | Gitlinks `submodules/lua`, `submodules/pdcurses`, `submodules/pdcursesmod` removed | `.gitmodules` still lists them. `sys/unix/hints/linux.370` still sets `LUAHEADERS = submodules/lua`. The Linux tty build does **not** use the vendored tree. |
| ID-281 | Vendored Lua 5.4.6 | `sys/lib/lua-5.4.6/` and `sys/lib/lua-5.4.6.tar.gz` |

Upgrade rule: take the `NetHack-5.0.0_Released` gitlinks for those three
submodules. Leave the vendored Lua tree in place (clutter, section 18).
The owner's Linux hints only lost the executable bit; their text still
points at `submodules/lua`.

## 17. Mode-only noise

These paths differ from the fork point by file mode only (`100755`→`100644`),
with no content change. They are not gameplay customizations. Prefer
upstream's executable bit when 5.0 still ships the file.

`DEVEL/hooksdir/*`, `DEVEL/nhgitset.pl`, `dat/GENFILES`, `include/sndprocs.h`,
`outdated/sys/wince/cesetup.bat`, `sound/wav/uu2wav.sh`,
`sys/libnh/test/run.sh`, `sys/msdos/fetch-cross-compiler.sh`,
`sys/msdos/font.c`, `sys/msdos/font.h`, `sys/msdos/fonts/makefont.lua`,
`sys/msdos/setup.bat`, `sys/unix/Makefile.check`, `sys/unix/gitinfo.sh`,
`sys/unix/hints/include/compiler.370`, `gbdates-post.370`, `gbdates-pre.370`,
`sys/unix/hints/linux.370`, `macOS.370`, `macosx.sh`, `sys/unix/mkmkfile.sh`,
`sys/unix/nethack.sh`, `sys/unix/setup.sh`, `sys/vms/install.com`,
`sys/vms/nethack.com`, `sys/vms/spec_lev.com`, `sys/vms/vmsbuild.com`,
`sys/windows/nhsetup.bat`, `win/macosx/recover.pl`.

`sys/msdos/Makefile.GCC` was deleted by the owner. Upstream 5.0 still
has it and modified it.

Upstream renamed `sys/unix/hints/linux.370` to `sys/unix/hints/linux.500`
(and the other `*.370` hint fragments to `*.500`). The owner's mode-only
`linux.370` is not a content fork. The 5.0 Linux tty build uses
`linux.500`.

## 18. Clutter (leave in place)

Candidates for a later cleanup. Not deleted by this upgrade.

- `scratch.c` (repo root)
- `src/tile 2.c`, `src/tile 3.c`, `src/tile 4.c`, `src/tile 5.c` (copies of tile code, not in the build)
- `dat/quest_BAK.lua`
- `dat/Kni-strtBAK.lua`
- `tools/parse_objectsBAK.py`
- `tools/bak.html`
- `sys/lib/lua-5.4.6/` and `sys/lib/lua-5.4.6.tar.gz`
- duplicate editor data at repo root: `allObjects.js`, `color_def.js`, `monster_def.js`, `output.js`
- `NameChanges.xlsx` empty rows on the Prophetic Book sheet (the real rows are 2–42)

## 19. Save files

RighteousHack 0.1 saves are NetHack 3.7, `EDITLEVEL 131`. NetHack 5.0.0
is `VERSION_MAJOR 5`, `VERSION_MINOR 0`, `EDITLEVEL 0`, `NH_STATUS_RELEASED`,
and `struct objclass` grew shop-price fields (`oc_sell_minseen` and the
related min/max fields). 0.1 save files and bones will not load. That is
expected. There is no converter.

## 20. Upgrade status (after the 5.0.0 merge)

Linux tty build of this branch was made with:

```
sh sys/unix/setup.sh sys/unix/hints/linux.500
make fetch-lua
make -j4 WANT_WIN_TTY=1
make install
```

`make fetch-lua` downloaded lua-5.4.8 and the checksum in
`submodules/CHKSUMS` matched. The game binary is `src/nethack`. A
character start as a lawful human Royal Historian reached Dlvl:1 and
showed the Historian quest speech from `dat/quest.lua` (the "day of
salvation" / "Son of man" text). `dat/nhdat` contains `His-strt.lua`,
`Pht-strt.lua`, `Adv-goal.lua`, `Chp-strt.lua`, `his-treasure-1.lua`,
and `quest.lua`.

### Preserved

These are in the merged tree with the owner's wording and behavior.
Auto-merge kept them, or a conflict was resolved by keeping the owner's
text on top of 5.0 code that did not change the behavior.

ID-001, ID-002, ID-003 (README text; the base version in it now says
5.0.0), ID-010 through ID-019, ID-030, ID-031, ID-040 through ID-043,
ID-050 through ID-060, ID-071 through ID-083, ID-085's parsers in
`src/objnam.c`, `src/mkobj.c`, `src/muse.c`, `src/shknam.c`,
`src/sp_lev.c`, ID-090 through ID-110, ID-120 through ID-144, ID-150
through ID-190, ID-200 through ID-211, ID-220 through ID-225, ID-230
through ID-235, ID-240, ID-241, ID-243 through ID-247, ID-260 through
ID-266, ID-270 through ID-277, ID-281.

ID-224 originally kept the Elbereth crowning title. The 2026-11-01
cleanup changes the lawful event to an anointing: "The Servant of the Lord".

### Ported

| ID | What changed |
|----|----------------|
| ID-004 | Version is NetHack 5.0.0, `EDITLEVEL 0`, `NH_STATUS_RELEASED`. The RighteousHack banner from ID-001 is still the one players see. |
| ID-084 | Starting oils use the 5.0 `trobj` (`trquan_min`, `trquan_max`). Healer still starts with `ANO_HEALING` and `ANO_EXTRA_HEALING` and still `knows_object(ANO_FULL_HEALING)`. The rows still say `POTION_CLASS`, which is only a label when `trotyp` is set (`mksobj` uses the type). 5.0 removed the Monk's starting prophetic book; that row was not one of the owner's edits, so the 5.0 Monk table was kept and only the healing oil was swapped. Monk and Tourist are not playable. |
| ID-085 | `src/invent.c` item-action menu moved to `src/iactions.c` in 5.0. The owner's "elixer" comment was reapplied there. |
| ID-191, ID-192 | `Z` is still `meditate`. Wizard commands stay on the `pro` prefix. 5.0's new commands are `proobjprobs` and `proshownhuuid`. `anoint` is still Meta-a. Command flags from 5.0 (`CMD_M_PREFIX`, `GENERALCMD`) are kept. |
| ID-205 | Staff-of-wonders targeting uses 5.0 `getpos_sethilite(void (*)(boolean), boolean (*)(coordxy, coordxy))`. Same distance and visibility rules. |
| ID-242 | `his-treasure-1.lua` was not on the Unix `SPEC_LEVS` list or the Windows `LUA_FILES` list, so a DLB build never packed it. It is on both lists now. |
| ID-014, ID-015 | File codes are `Adv` and `Chp`, and the game builds quest level names from the file code (`src/dungeon.c`). The edited maps were still named `Rog-*.lua` and `Val-*.lua`. Copies are `dat/Adv-*.lua` and `dat/Chp-*.lua` (goal item and locate-level edits included). The `Rog-*` and `Val-*` files are still in the tree. Windows `LUA_FILES` lists the `His`, `Pht`, `Adv`, and `Chp` files. |
| ID-247 | `dat/tower2.lua` had `/**/` after two prophetic-book strings. That is not Lua, so the level would not load. Those two tokens were removed. The names are unchanged. |
| ID-280 | Gitlinks for `submodules/lua` (`6e22fedb`), `submodules/pdcurses` (`09cf16db`), and `submodules/pdcursesmod` (`5a255460`) are the 5.0.0 commits. The Linux hints still point at `submodules/lua`, but the default tty recipe is `make fetch-lua` (lua 5.4.8 tarball). The vendored `sys/lib/lua-5.4.6` tree is unused by that recipe and was left in place. |

`is_blanket_of_the_heavenly_host`, `rloc_pos_ok`, and `maybewakesteed`
are global and declared in `include/extern.h`. 5.0 made the last two
`static`, and both the Linux hints and `WIN32` define `NOSTATICFN`, so
a `static` definition is not visible to `src/apply.c` / `src/artifact.c`.
`#undef PBOOK` and `#undef ROD` were added next to the old `#undef SPELL`
and `#undef WAND` so those renamed macros do not leak out of
`include/objects.h`.

5.0's new wand is a rod named **stillness** (appearance redwood,
probability 45), inserted after "ask and ye shall receive", which is
where 5.0 put `WAN_STASIS`. The owner chose "stillness" (Psalm 46:10),
so the item is "rod of stillness" and the object enum is
`WAN_STILLNESS`. Charges, duration, and the no-teleport effect are the
5.0 behavior. 5.0's shield of drain resistance and shield of shock
resistance are kept under those names, and ancient defense is kept too.

### Food (2026-10-31, updated 2026-11-01)

Monster bodies are shown as raw meat. The noun comes from
`include/meatnoun.h` (one word per monster class): dragons are
steak, sea monsters (`S_EEL`) are fillet, fungi are cap, jellies
are jelly, puddings are pudding, and everything else is meat.
If the monster's name already ends with that noun, it is not
repeated (`raw blue jelly`, not `raw blue jelly jelly`). Fungus
is still a cap (`raw yellow mold cap`), because "mold" is not
the noun. Cooked meat is the same object with a cooked flag, so
poison, acid, petrification, and intrinsics stay the same.
Humanoids, humans, and elves leave no meat, with the exceptions
below. Trolls still leave meat, because they come back from it.
The ape class does too (monkey meat; ape, owlbear, yeti,
carnivorous ape, and sasquatch steak). Lycanthropes leave meat
in animal form and in human form, and eating either form causes
lycanthropy. Cyclops leaves raw Cyclops steak and cooked Cyclops
steak, with no "the" and no possessive. Goblins leave raw goblin
meat and cooked goblin meat, under the normal raw-meat rules.
Other giants, and the other orcs (hobgoblin, orc, hill orc,
Mordor orc, Uruk-hai, orc shaman, orc-captain), still leave nothing.

Essences are named "<monster>'s essence". They never spoil, cannot
be cooked, and can be blessed. That set is ghosts, shades, wraiths,
barrow wights, Nazgul, vampires, zombies, mummies, ghouls,
skeletons, nymphs, mind flayers, the quantum
mechanic, the genetic engineer, Medusa, the Wizard of Yendor,
Croesus, every `S_DEMON` monster, and every `M2_DEMON` monster.
A wraith's essence still grants a level. Eating a Rider's essence
is still fatal, and they still revive from it. Eating any demon
essence (`M2_DEMON`) is instantly fatal. The death reason is
"ate a demon's essence". Sandestin, djinni, and the mail daemon
are `S_DEMON` but not `M2_DEMON`, so their essences are not fatal.
The Riders are also not `M2_DEMON`. Their essences keep the old
meal effects: sandestin polymorphs you, and djinni and mail daemon
essences are poisonous. Imps, manes, lemures, quasits, and tengu
are not demons. They still leave meat, with the same effects as before.
Ghosts and other zero-nutrition essences give 50 nutrition.
Medusa's essence still petrifies, and killing her is still
recorded. The Wizard still comes back and still drops what he
carried; the essence is not what revives him. Croesus still drops
his gold. Charon would leave an essence, but this build does not
compile him. Liches, demiliches, master liches, and arch-liches
crumble into dust and leave nothing.
Vampire, zombie, and mummy essences are that undead, not an old
human or elf body, and undead turning brings them back from it.
Cannibalism is removed: no "You cannibal!" message, no Luck or
aggravate penalty, no cave dweller or orc exception, and pets do
not refuse food of their own race.

Jellies and lizard meat never rot, skip the 5% raw-poison roll,
and can be blessed while raw. They can still be cooked. Sea
fillets (jellyfish, piranha, shark, giant eel, electric eel,
kraken) also skip that roll, and they rot on the normal raw-meat
schedule. A raw fillet cannot be blessed. Cooked fillet rots
about ten times slower, and blessed cooked fillet never rots.
Jellyfish is a fillet, not a jelly. Other raw meat cannot be
blessed. Every role starts
with a flint and tinder box. `#cook` uses it and does not use it
up. Blessed cooked meat never spoils.
An adult dragon steak weighs 4500. Stock carrying capacity never
goes above 1000, and with an empty pack the heaviest single thing
that can be lifted is just under 3000, so the steak cannot be
picked up. Cook it and eat it where it lies. A baby dragon steak
weighs 1500 and can be carried. Nothing special was added to the
pickup code.
The workbook `docs/RighteousHack_Food.xlsx` lists every food and
every monster; regenerate it with `tools/gen_food_xlsx.py` when
those tables change. `EDITLEVEL` is 1, so saves from before the
first food change will not load.

### Cave of Gibeon (2026-11-01)

The Prophet's chamber has an up stair into a one-level branch, the
Cave of Gibeon. Sokoban still joins one level below the Prophet; this
stair is placed on the Prophet's level itself. A dark orthogonal tunnel
leads into a lit cave. The cave does not generate random monsters.
In the middle is the Altar at Gibeon, a lawful altar. Looking at it
says "There is the Altar at Gibeon here." King Solomon stands on the
platform and remains there. `#chat` with him says one of two
lines, chosen at random. One is: "When the Lord came to me in this
place, I could have asked for gold or the lives of my enemies.
Instead I asked for a heart to judge His people rightly." The other
is Mark 8:36: "For what shall it profit a man, if he shall gain the
whole world, and lose his own soul?" A chest on the platform holds one burnt offering. That food is not randomly
generated and cannot be wished for.

Praying while standing on the altar, with the burnt offering on it,
consumes the offering even during the normal prayer wait. The LORD
speaks the King James text of 1 Kings 3:5, with "to Solomon" changed
to "to thee": "In Gibeon the LORD appeared to thee in a dream by
night: and God said, Ask what I shall give thee." Asking for wisdom,
a discerning heart, a wise and discerning heart, wisdom and
knowledge, or knowledge (a leading "a" or "an" is ignored) is
answered with the King James text of 1 Kings 3:11-13, from "Because
thou hast asked this thing" through "all thy days." The gift is the
existing Belt of Truth at
+2 enchantment, a blessed anointing oil of gain level, and Wisdom 25.
Any other answer is an ordinary wish. The vision happens once. Later
prayers at the altar are ordinary prayers.

The Belt of Truth is the artifact that was already defined on the
humble sash. Its base item and properties are unchanged. The +2 is
the enchantment of this gift. While worn, that enchantment is +2
Intelligence. The human race table still lists a Wisdom ceiling of
18. This gift raises the ceiling for the hero who receives it, so
the 25 is not immediately pulled back down to 18.

Humans are the only race in the race table. No other race can be
selected.

### First-half occult cleanup (2026-11-01)

Player-visible "luck" is "favor". The mines prize object is named
"Favor Stone". The internal luck value is unchanged. The branch
once called The Gnomish Mines is "Salt Mines of Galilee", including
the #overview name, achievements, and the code that looks the
branch up by name.

Anointing replaces crowning for the lawful hero. The deity says
"I anoint thee...  The Servant of the Lord!" (two spaces after the
ellipsis, as in the old line). Enlightenment says "the Servant of
the Lord". The livelog says the hero was anointed. The old
Excalibur and prophetic-book gifts are not given. PLACEHOLDER:
the anointing gift itself is not designed yet. Resistances, a
blessed wielded weapon, and one extra weapon-skill slot still
happen. Neutral and chaotic crowning text is unused.

Crystal balls are not generated and cannot be wished for. The
Champion quest artifact Orb of Fate is still a crystal ball,
because class quests stay as they are. Fortune cookies are not
generated and cannot be wished for. Eating or reading a leftover
cookie prints one of four placeholder Prophet proverbs, exact
KJV: Proverbs 1:7, 3:5, 3:6, and 1:5 (1:5 keeps the KJV colon).
Jon should review that list.

Same-race sacrifice already refused the offering and did not
summon a demon or convert the altar. That refusal stays.
Random altars and temples follow God. The Gehennom filler no
longer places an unaligned shrine. A mimic that looks like an
altar follows God. The fixed Valley of the Dead shrine and
Moloch's Sanctum are unchanged. Class-quest altars are unchanged.

King Solomon's #chat, before the Gibeon vision, always speaks
the hint about asking for a heart to judge rightly, then
Mark 8:36. After the vision, either line may be spoken.

### Prophet mode (2026-11-01)

The mode started with -D is prophet mode. Help, the guidebook, the
options screen, and the messages that used to say wizard mode or
debug mode say prophet mode. `OPTIONS=playmode:prophet` selects it.
`playmode:debug` still selects it. The Wizard of Yendor and the
Wizard role are unchanged. The sysconf list is still named WIZARDS.

### Prophet, Capernaum, and PETRA (2026-11-01)

The Oracle is Isaiah, prophet of the Lord. That is his monster
name, and the consultation windows name him that way. Paying for a
minor consultation hears one of the placeholder proverbs. Paying
the full price for a major consultation hears "The treasure you
seek has always been yours." A short payment hears "That is not
enough." That short line is a stand-in; Jon did not write one.
The overview feature line is exactly "Prophet of the Lord."
Prophet mode tags that level [prophet]. The #levelport menu shows
prophet. The xlogfile achievement is consulted_the_prophet. The
big room's overview and achievements call it the Valley of Dry
Bones. The maps of those big rooms are unchanged.

Babylon's Palace is the old castle. The chest that held the rod of
wishing now holds a blessed +2 Shield of Faith. The drawbridge does not
open to the old tune, and the key is not applied at the bridge
itself. A Rock with a keyhole stands in the small courtyard
outside the moat, west of the closed bridge. Standing on it or
next to it and applying the Key to Babylon's Palace lowers the
drawbridge. PLACEHOLDER for Jon: "The key turns in the rock. The
drawbridge lowers." If it is already down: "The drawbridge is
already lowered." Palace doors still open with the key. A lock
pick does not open those doors. Interior chests can still be
picked. Prophet mode tags the level [babylon]. The #levelport
menu shows babylon. The level file is still castle.

Capernaum replaces every Minetown variant. Prophet mode tags that
level [capernaum], and the #levelport menu shows capernaum. The
xlogfile achievement is entered_capernaum. The old file name
minetn-1 used to trigger the orc-town gang; Capernaum does not.
The cats placed there are housecats; the id "cat" does not exist
and stopped the level from loading.
It is lit, with a food
shop, a general store, a tool shop, a lawful temple, a pool, a
fountain, and peaceful people, dogs, and cats. No random monsters
are generated there. Peter the Fisherman stands south of his door.
The first arrival opens a text window, the same kind as a quest
introduction: "As you approach Capernaum, a man calls out \"Hello!
You sir! My name is Peter the Fisherman, and I am in need of your
aid!\"" The first `#chat` is the quest: "I lost my fishing net
deep in the Salt Mines of Galilee, and I need you to retrieve it.
Please take this scroll for a hint of the trials you may find
along the way." He puts Peter's scroll in your pack. It cannot be
wished for. Reading it shows "The Master gave me a new name, and
on it He said He would build. Remember my name, and the floor will
hold you." It can be read any number of times and is not used up.
Later chats before the net is returned repeat that same hint.
Peter cannot be killed or hurt by a weapon, a missile, a spell, a
rod, a trap, or a pet. He does not turn hostile, and he cannot be
pushed off his post. A pet will not attack Peter, King Solomon,
Isaiah, the angel at Jacob's Ladder, or a Babel builder.

Mines' End is three PETRA layouts. The puzzle room, through the
safe row, is lit. The rest of the level is dark. The letters P, E,
T, R, A and the decoys are drawn on the map as those capitals.
Farlook and look-here call a tile "the letter P" (or whichever
letter it is). They are not engravings. The hint is an engraving
in the southwest corner of the safe row, so it shows the engraving
symbol: "Upon this rock I will build my church. Walk upon the rock
in the tongue of the Greeks." A wrong letter, including a decoy or
a diagonal step, becomes lava under you and deals 20 damage. If
that would kill, you are left at 1 hit point. A wrong step at 1 hit
point kills you. The path is stored on each letter, so a later
layout does not need a coordinate table. The first letter has to
be stepped on straight north from the safe row. There is one door
onto the letter floor, directly north of the A. The other two
doors are closets, and each holds a loadstone. Behind the real
door are Peter's net and a Favor Stone. The level does not
generate random monsters, does not allow teleport, and cannot be
dug. Those three limits stay, so the puzzle cannot be skipped.
The Philistine Champion is the only monster, on every layout. He
waits in a dark room far from the up stair and does not leave that
room. He can still fight anyone who comes in. The passage to that
room leaves the arrival corridor and winds under the puzzle,
around the outer edge, and back to a closed door. It does not
cross the letter floor or the safe row, so the scroll can be taken
before the puzzle. The passage is not visible from the stair. He
is hostile and male. Monster level 7, speed 12, magic resistance
30, size large. Hit points are a fixed 90, about three times an
ogre lord. Bare armor class is 3. He wears uncursed bronze plate
mail and a helmet, so his armor class in the fight is -4. One
weapon attack, listed as 2d6 with empty hands. He wields a +2
two-handed sword: 1d12+2 against a human-sized hero, plus
strength. No sorcery. Killing him always leaves a scholar's
scroll. The scroll is not generated anywhere else and cannot be
wished for. Reading it shows this text and does not use it up:
"In my research I have found the ancient Greek name for the rock
upon which the church was built. It is written with the Greek
letters Pi, Epsilon, Tau, Rho, Alpha, which in our letters is
PETRA."

Give Peter the net. He says "Thank you, friend... 'Follow me, and I
will make you fishers of men.' I have cast this net for many souls
since then." He keeps the net. He then gives the Key to Babylon's
Palace and the Key to the Gates of Hell (placeholder name) and says
Matthew 16:18. He steps aside and the door opens. The undiggable
closet holds a blessed +2 Breastplate of Righteousness. The net and
both keys cannot be wished for.

### Planned (not done)

Lawful, neutral, and chaotic will eventually be replaced by two
alignments, good and evil. That work is not done. Alignment itself
still works, and the attributes screen no longer describes the
player's own deity names as opposing each other.

### Needs owner decision

| ID | Why |
|----|-----|
| ID-070 | Anointing oil of gain ability is still named "wiggy gain ability". |
| ID-212 | Handkerchief of the Apostle Paul is the Healer quest artifact in `src/role.c`, and `include/artilist.h` still gates it to `PM_KNIGHT`. |
| ID-213 | Champion's quest artifact is still the Orb of Fate. |
| ID-223 | `desecrate_altar` is gone. `src/dig.c` and `src/music.c` print `placeholder for desecrate_altar`. |
| ID-224 | Resolved: anointing says "the Servant of the Lord". The gift is a placeholder. |
| (new) | Resolved: the owner named the 5.0 stasis rod "stillness" (`WAN_STILLNESS`, "rod of stillness", Psalm 46:10). |
| ID-084 | Healer's oil rows are tagged `POTION_CLASS` even though the objects are `ANOINTING_CLASS`. Creation uses `trotyp`, so the game start still works. |
| ID-017 | Only human is playable. `src/u_init.c` still has tables for the removed roles. |

### Not lost

No inventory item was dropped on the floor. Clutter from section 18 is
still in the tree, including `scratch.c`, `src/tile 2.c` through
`src/tile 5.c`, `dat/quest_BAK.lua`, `dat/Kni-strtBAK.lua`,
`tools/parse_objectsBAK.py`, `tools/bak.html`, and `sys/lib/lua-5.4.6`.

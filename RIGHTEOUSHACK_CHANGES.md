# RighteousHack customization inventory

Checklist of every customization on `righteoushack-0.1` relative to the
fork point with upstream NetHack, commit `94e5f7b861` (2025-11-28).
This file was written **before** merging tag `NetHack-5.0.0_Released`
(`16ff591153`) so the upgrade can be checked against it.

Spellings below are the owner's, including `elixer`, `exlier`,
`doubled-edged`, `trusty sandles`, `wiggy gain ability`,
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
| ID-030 | `fiery steed of the Heavenly Host` (`FIERY_STEED_OF_HEAVEN`), unicorn class, flying, peaceful | `include/monsters.h` |
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
| ID-201 | Riding Blanket of the Heavenly Host | saddle | `PM_KNIGHT` | Apply transforms a horse into the fiery steed; invoke tames nearby horses (`src/apply.c`, `src/artifact.c`) |
| ID-202 | Handkerchief of the Apostle Paul | torn piece of cloth | `PM_KNIGHT` in `artilist.h`, but the Healer quest reward in `src/role.c` | Carry grants disease resistance; invoke heals and pushes monsters (`mhurtle_giant` in `src/dothrow.c`) |
| ID-203 | Golden Censer of Incense | thurible | `PM_CLERIC` | Prayer timeout, light radius 5 with no burnout (`src/timeout.c`), invoke engraves and teleports |
| ID-204 | Scarlet Cord of Rahab | rope | `PM_ROGUE` | Apply climbs; invoke toggles invisibility. Climb/fall messages in `src/hack.c` |
| ID-205 | Staff of Signs and Wonders | almond-wood walking stick | `PM_WIZARD` | Apply changes water/floor; invoke is a 5x5 column of fire (`src/apply.c`, `src/artifact.c`). Prayer recharges it (`src/pray.c`) |
| ID-206 | Gospel of Peace | trusty sandals | not role-bound | Water walking, haste, fire resistance; invoke tames |
| ID-207 | Breastplate of Righteousness | gleaming armor | not role-bound | Half spell damage, drain resistance, magic resistance, poison carry |
| ID-208 | Belt of Truth | humble sash | not role-bound | Searching, see invisible, sleep/blind/hallucination resistance; invoke enlightens |
| ID-209 | Sword of the Spirit | doubled-edged blade | not role-bound | Reflection, stun attack, warning. When the stun lands and the hero can see it, one line: "The Sword of the Spirit blazes with holy light. <monster> is overwhelmed by divine truth!" or "seems" when the stun is 5 or less |
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
Intelligence. Wearing it, or any artifact with blindness resistance,
is recognized when a flash is resisted. The human race table still
lists a Wisdom ceiling of 18. This gift raises the ceiling for the
hero who receives it, so
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
the altar in the Sanctum of the Great Dragon are unchanged. Class-quest altars are unchanged.

King Solomon's #chat, before the Gibeon vision, always speaks
the hint about asking for a heart to judge rightly, then
Mark 8:36. After the vision, either line may be spoken.

### Prophet mode (2026-11-01)

The mode started with -D is prophet mode. Help, the guidebook, the
options screen, and the messages that name the mode say prophet mode. `OPTIONS=playmode:prophet` selects it.
`playmode:debug` still selects it. The Wizard of Yendor and the
Wizard role are unchanged. The sysconf list is still named WIZARDS.

### A word from the Lord (2026-11-01)

#chat with Isaiah asks, with no price:
"Wilt thou settle for a minor word from the Lord?" [ynq] (q)
Answering no asks:
"Then dost thou desire a major word from the Lord?" [yn] (n)
Both are free. The first of either is still the achievement
"received a word from the Lord." Each word's first-time
experience is the old amount: a few points for the minor word,
and the old major price divided by 10 (or by 25 if the minor
word came first). Hearing either again does not repeat that.
If he is angry: "Isaiah, prophet of the Lord is in no mood to
give a word from the Lord."

Saying no to both asks:
"Would you like to make a donation to God's church?" [yn] (n)
Yes asks "How much will you give?" More than you are carrying
gives all the gold you have. A gift of 0 takes nothing and has
no penalty. Isaiah quotes 2 Corinthians 9:7, King James, word
for word: "Every man according as he purposeth in his heart, so
let him give; not grudgingly, or of necessity: for God loveth a
cheerful giver."
PLACEHOLDER thank-yous, for Jon:
- 1 to 999 shekels: Isaiah quotes Proverbs 11:25, King James, word for word: "The liberal soul shall be made fat: and he that watereth shall be watered also himself." Then the placeholder "You feel the Lord's favor upon your gift." Favor +1. Any number of times.
- 1000 to 4999: Isaiah quotes Luke 6:38, King James, in two messages, then "Thy gift lifts thee a little." A quarter of this level's experience.
- 5000 to 9999: the same verse, then "Thy gift lifts thee further." Half a level.
- 10000 or more: the same verse, then "Thy gift lifts thee a full step." One level.
The verse, split at the sentence and otherwise unchanged: "Give, and it shall be given unto you; good measure, pressed down, and shaken together, and running over, shall men give into your bosom." Then: "For with the same measure that ye mete withal it shall be measured to you again."
Those three are capped at ten gifts in a game. After that, a gift
of 1000 or more still takes the gold and says "The Lord bless
thee. The church can receive no greater increase for now." Favor
+1, and no experience. That last line is a placeholder. A gift
never raises you more than one level, including at the level cap.

### Shekels (2026-11-01)

Prices, shop speech, the inventory gold line, and death and score
lines say "shekel" for one and "shekels" for more. The coins
themselves are still called gold pieces. A donation to God's
church is counted in shekels.
A wish for shekels or for zorkmids both yield gold pieces. The
game does not say zorkmid.

### Prophet, Capernaum, and PETRA (2026-11-01)

The Oracle is Isaiah, prophet of the Lord. That is his monster
name, and the text windows name him that way. The minor word
hears one of the placeholder proverbs. The major word hears
"The treasure you seek has always been yours." Neither costs gold.
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
next to it and applying the Key to Babylon's Palace toggles the
drawbridge. When it is up: "The key turns with a sound like
a stone rolled away. Far off, chains groan, and the drawbridge
of Babylon comes down." When it is down: "The key turns again.
Far off, chains groan, and the drawbridge of Babylon rises."
Whoever is standing on the span is handled the way closing
a drawbridge always handles them. Force bolt, striking, and
the passtune are unchanged. Palace doors still open with
the key. A lock
pick does not open those doors. Interior chests can still be
picked. Prophet mode tags the level [babylon]. The #levelport
menu shows babylon. The level file is still castle.

Capernaum replaces every Minetown variant. Prophet mode tags that
level [capernaum], and the #levelport menu shows capernaum. The
xlogfile achievement is entered_capernaum. The old file name
minetn-1 used to trigger the orc-town gang; Capernaum does not.
The cats placed there are housecats; the id "cat" does not exist
and stopped the level from loading.
Each game picks one of three approved town maps. The sea is on
the west, the south, or the east. The whole town is lit. There
is no random monster generation. The shops are a delicatessen, a
general store, a hardware store, Izchak's lighting store, a used
armor dealership, an antique weapons outlet, a second-hand
bookstore, a liquor emporium, and a jewelers. Two of the maps
also have a health food store, so those maps have 10 shops and
the south-sea map has 9. A candle shop in this town names its
shopkeeper Izchak. There is a lawful temple, two fountains, a
small pool, and peaceful people, dogs, and cats. Peter the
Fisherman stands against his closet door and has room to step
aside. Water that connects to the edge of the map is called the
Sea of Galilee. The town pool, and water on every other level,
keeps its ordinary name.
The first arrival opens a text window, the same kind as a quest
introduction. A male hero hears "As you approach Capernaum, a man
calls out \"Hello! You sir! My name is Peter the Fisherman, and
I am in need of your aid!\"" A female hero hears the same line
with "You ma'am!" in place of "You sir!" The first `#chat` is the quest: "I lost my fishing net
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
A pet will not step onto a Mines' End letter, whether that
letter is on the path or a decoy, so it cannot wander the
grid. If a pet is already standing on one, it steps off.
On Capernaum, a pet will not attack any monster: a shopkeeper,
townsfolk, the temple priest, a dog or a cat, Peter, or a
hostile. It can still move, follow, and eat. Off Capernaum,
pets keep their old behavior, apart from the plot people
named above.

Mines' End is three approved full-level maps, one chosen at
random. The overview calls that level
Mines' End and tags it [minesend]. The #levelport menu says
minesend. Achievements, the livelog, and arrival text do not say
PETRA. The scholar's scroll is the clue that does. Only the
puzzle room is lit. The rest of the level is dark. The letters
are a 7 by 4 grid drawn on the floor as those capitals.
Farlook and look-here call a tile "the letter P" (or whichever
letter it is). They are not engravings. The hint is an engraving
in the southwest corner of the safe row, so it shows the engraving
symbol: "Upon this rock I will build my church. Walk upon the rock
in the tongue of the Greeks." A false letter, such as a decoy,
becomes ordinary lava under you and deals 20 damage, or 10
if you have fire resistance. You stay on the square.
The lava does not sink you or kill you by itself, and the next
move can step off. Death happens only when the 20 takes you to
0 hit points or below, and the reason is "a false step." A
letter on this layout's true path, P through A, is never turned
to lava. Order and direction do not matter, and that stays true
after you leave the level or save, because the path step is not
replaced by the turn count when the level is loaded.
The path is stored on each letter, so a later
layout does not need a coordinate table. There is one door
onto the letter floor, directly north of the A. The other two
doors are closets, and each holds a loadstone. Behind the real
door are Peter's net and a Favor Stone. The level does not
generate random monsters, does not allow teleport, and cannot be
dug. Those three limits stay, so the puzzle cannot be skipped.
The Philistine Champion is the only monster, on every layout. He
waits in his own room, at the square marked for him, and does not
leave that room. He can still fight anyone who comes in. The only
door of that room is the one drawn on the map. The route from the
stair does not cross the letter floor. In his room, away from
the door, an unlocked untrapped chest holds 4 real gems (no
worthless glass; each is a single stone), 350 gold pieces, and
one random prophetic book. He
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
Matthew 16:18. He steps aside and the door opens. That is the
only way in. Kicking, lock picks, keys, credit cards, knock,
force, digging, phasing, and teleporting all fail, with "This
door will not yield." The closet floor and its walls cannot be
dug or phased through. The undiggable
closet holds a blessed +2 Breastplate of Righteousness. The net and
both keys cannot be wished for.

### Sodom and Tyre (2026-11-01)

Medusa's level is one map, Sodom and Tyre, and it is dark. Lot's
wife is a statue at the entrance. The engraving is Genesis 19:26,
exact KJV. Eating the statue licks it and does not consume it.
Breaking it fails. Each of the six houses holds one occupant. The level rolls
percent(50) separately for each house: man of Sodom, or woman
of Gomorrah. Nothing forces a mix. Farlook names them "man of
Sodom" and "woman of Gomorrah". She is female. Both use the
same numbers: base level 6, speed 12, armor class 4, magic
resistance 0, and one weapon attack of 2d4. They carry no armor.
Hit points are the usual dice for that level after the depth
adjustment, and the experience uses that same level. They call
with the three lines from the plan while peaceful, and they
meditate until you enter. The first step from outside a house
onto its floor bars that door and prints, once, naming the
nearest occupant: "The Man of Sodom looks at you with a
crooked smile... \"No one leaves this house, stranger.\" The
door bars itself." or the same sentence with "The Woman of
Gomorrah". Only that house's occupants turn hostile. They stop
meditating and attack, and the locked door keeps them inside.
The door will not yield to kicking, lock tools, knock, a wand
or spell of opening, digging, phasing, or teleporting in, the
same way Peter's closet will not. #force does not open doors.
When every occupant of that house is dead, the door opens. There
is no favor penalty. Killing one is not murder: no "You murderer!",
no loss of Favor, and no loss of telepathy. They are marked
always hostile for that check. The level still places them
peaceful, so before you enter they farlook as peaceful and
meditating. A reflected gaze that turns Medusa to
stone says she meets her own gaze in the reflecting item as
the hero knows it, and shrieks as her flesh turns to stone,
whether or not she is seen. With no worn or wielded item, the
line names no item. The kill says "You defeat Medusa!" The
experience is unchanged. Lightning the hero deals a monster,
from a rod, a prophecy, or a breath, is triple on this level.
Lightning that hits you, and lightning one monster deals another,
is unchanged. The flash is unchanged. Shock resistance still
negates lightning. The Sword of the Spirit deals double damage
here. Tyre's gold piles can be kept. Medusa's seven taunts each
play once, as a single message.
She still leaves Medusa's essence under the food rules. The level
does not generate random monsters and cannot be dug or teleported
across. The arrival line is the first taunt. The first real room on the
level is the open walk on the shore side of Tyre, map (38,2)
through (41,12). It is not a house, not the up stair, and not
Lot's wife. The random statues are placed in that room. The
level stays dark. The other Medusa maps are unused.

### Wilderness Trial fixes (2026-11-02)

Desert, Greed, Pride, and the Garden use real horizontal and
vertical walls. Fear already did.

Every engraving on the five Wilderness levels is a verse. Stepping
on one shows only that text, and it shows again every time you
step onto that same square. Waiting there does not repeat it. It does not say the writing was burned
or engraved, it does not ask for --More-- or Enter, and it does not
stop you if you are running. A blind hero reads the same line.
The verse cannot be scuffed, overwritten, cancelled, or zapped away.
Look-here and farlook quote it. The read command still reads a
scroll or a prophetic book.

Greed is the Cave of Wonders, 79 by 21. The up stair is at the west
end of the three-wide hall, with Matthew 16:26 beside it. Four side
rooms open off that hall. The pillared great room is packed with
flawless gems of every color and with gold. Most piles are 1,000 to
5,000 gold, and about one in five is 10,000 to 20,000. The narrow
gate is the door at the far east. Job 1:21 is on the square before
it. Past the gate is the restore square, Matthew 6:33, then the down
stair. Stepping on the gate while carrying anything, gold included,
sends you back to the entrance stair. The first bounce says
Matthew 19:22, "But when the young man heard that saying, he
went away sorrowful: for he had great possessions." Every bounce
after that says "Thou must leave all behind to pass this way."
Empty-handed, only objects you yourself dropped,
and not a flawless gem, are moved to the restore square. On entering
Greed the game records how much gold you are carrying, in hand and in
bags. On leaving, your gold is set back to exactly that amount. The
gold counter can show amounts into the tens of millions. The weight
of a coin stack is computed in a long and then clamped to a 32-bit
int, which is enough for the piles on this level. Food still turns
to ash. The hunger clock starts at 200 turns when you enter Greed.
It counts one step each time you actually move, including through
a bounce, and not while you wait. Every accepted prayer on Greed
adds 50 turns, after the ordinary prayer messages, and prints
"The Lord is pleased with you and has restored your strength."
A prayer that is still too soon does not add turns and does not
print that line. The prayer timeout is unchanged. Picking up a
gold pile, or a real gem, or worthless glass, costs 25 turns,
once per pile or stack, including several in one pickup and
including taking one out of a container. Rocks and gray stones
do not. After the pickup lines, one command prints one of
"Thy covetousness hath hastened thy hunger.", "Thy greed gnaweth
at thee, and thy hunger waxeth sore.", or "Woe unto thee, for thy
greed hath driven thee unto hunger." If that drops the clock to
0 or below, you starve at once. Whenever 30 or fewer turns remain, each move says how many
are left, from "30 turns until you starve." through "1 turn until
you starve." A prayer that lifts the clock above 30 stops the
countdown until it falls to 30 again. The next move at 0 is the
tombstone "Starved, clinging to worldly possessions."

The first time you arrive on a Wilderness floor, a full-screen
text window shows, the same kind the quest intro uses, and a
keypress dismisses it. It does not show again on a later visit.
The desert, the first floor, says "And he was led by the Spirit
into the wilderness, being tempted by the devil." A woman hears
"she" in place of "he". There is no reference. Greed then shows
Luke 12:15 under its verse, Pride shows Proverbs 16:18, and Fear
shows 2 Timothy 1:7. The garden shows its verse and no reference.
The desert is not one of those four floors, so only the entrance
line is shown there.

Pride is 76 by 19. The hall is five rows. The up stair is at the west
end. James 4:10 is engraved at the center. Four treasure rooms are
north of the hall and four are south, each with a door, a harmless
mirror in the hall outside the door, and a live mirror toward the
back. One lure is shuffled into each room. The cold lure is the rod
named Rod of Frozen Power, created as "rod of northern wind", so it
is the cold rod and not the prophetic book of the same name. At the
east end of the hall, just west of the secret door, is an altar of
your own alignment. Praying there works as a normal prayer, and the
first time it also locks the eight treasure doors so they will not
open, kick, unlock, or be dug, and creates the down stair beyond
the secret door. After the ordinary prayer lines it says one
line. If you are carrying a lure, even nested in a bag: "You drop
to your knees and repent of your pride. The treasures crumble
into dust, and a hidden way opens." With no lure in your pack:
"The treasures crumble into dust, and a hidden way opens."
A wish for any of those lure names is refused the way a wish for
a quest artifact is refused, including in prophet mode. The line
is "For a moment, you feel something in your hands, but it
disappears!" The wish is spent and nothing is given. That covers
the name by itself, as in "blessed Amulet of Immortality", and a
real item "named" with one of those names. The plain base item
can still be wished for. The +12 gray dragon scale mail is left
to the ordinary enchantment cap. Naming an item you already hold
is unchanged.
Until that prayer
the level has no down stair, and nothing else is allowed to add one.
Praying anywhere else on Pride does not change the doors or the stair.
The lures, including one inside a nested bag, are destroyed by that
prayer and again when you take the stairs down to Fear. The bag of
infinite holding is one of them. A reflection is a doppelganger with
60 hit points, armor eight points better than a doppelganger's, and
a firmer blow. It is created as a doppelganger and stays one: it
does not take a random shape when it steps out, and it does not
change shape later, including when protection from shape changers
wears off. Within five squares its level climbs from 0 to 4 and
stays there. The drain each of its turns is that level, so it
never takes more than 4 hit points.

On Fear, standing on a path tile or either stair stops every
monster affliction, not only damage. That includes stun, confusion,
blindness, paralysis, sleep, slowing, hallucination, deafness,
drain, poison, stoning, sliming, sickness, lycanthropy, teleport,
polymorph, theft, seduction, item destruction, grabs, engulfing,
knockback, and being moved off the tile. Melee, spells, gaze,
sounds, breath, spit, missiles, monster wands and potions,
explosions, and passive effects are all stopped at the start of
the attack. A purple worm does not swallow you, and if you were
already swallowed it spits you out. While you are on a path tile
or either stair, hostile monsters do not start an attack at all.
There is no swing, no miss, no spell, no breath, no spit, no wand
or potion thrown at you, no gaze, no theft, and no shriek to wake
the level. They still move. They walk toward you and crowd the
squares next to the path, and they do not flee or hang back to
shoot. The moment you step onto any other tile, they attack at
full strength, with no grace turn and no damage reduction. A
Psalm 91 you engrave on any other level still only scares
monsters, exactly as before. Attacking does not remove the Fear
protection. Monsters will not step onto a path tile or a stair.
A boulder will not land or be generated there. Each of the three
Fear maps used to place 8 monsters and now places 40, five each
of blue dragon, minotaur, ettin, storm giant, jabberwock, purple
worm, pit fiend, and balrog, on floor that is not a Psalm 91
tile and not a stair. All 40 fit. The master lich, arch-lich,
and titan were replaced so spellcasters do not cover the verse.

Picking up the Sword of the Spirit returns you to the main-dungeon
level that holds the Wilderness stair, standing on that stair.
There is no portal trap and no "no corresponding portal" error.
The branch stays sealed, and the stair will not let you back in.
Reading the finger of execution book always succeeds. Level,
intelligence, role, the book's level, and curse status do not
matter, and it does not ask whether to continue or punish a failed
reading. Other prophetic books are unchanged. Casting it is a flat
50 percent. The prophecy list shows 50. Armor, shield, skill, and
the old Pride cap do not change that. The ray kills every monster
in its path, including the undead, demons, and anything that
reflects or resists death or magic. It does not bounce back, and
it does not harm you. Pets and peaceful monsters are struck the
same way any other ray strikes them. Peter is spared, the same as
any other ray. Leaving Pride, by the stairs, by level teleport, by
death or life-saving, or by the prayer that opens the way, takes
the prophecy off the spell list entirely. The book is destroyed
with the other lures. The Rod of Frozen
Power does not use charges and does not wrest or fizzle. Zapping
it draws a cold explosion, the same explosion shapes, on every
square of its 3-square burst. Those glyphs are the blue explosion
(CLR_BLUE), not the white frost one. They stay up for the old two
display delays plus thirty more, about 1.6 seconds, then clear.
Other frost explosions stay white. The burst still
does 8d8 cold damage to monsters in that radius and does not hurt
you. The Amulet of Immortality is matched by that name. Dying
while wearing it on Pride prints the three vanity lines and does
not life-save, so the tombstone is reached. An ordinary amulet of
life saving, with no such name, still saves you on Pride and on
every other level. The garden fountains are unchanged.

### Wilderness Trial (2026-11-01)

The 2026-11-02 section above is the current Greed, Pride, Fear,
verse, and sword behavior. This section is the first build.

A branch, The Wilderness Trial, joins the main dungeon three
levels below the Prophet. It is chained to the level named prophet.
Five levels: desert, greed, pride, fear,
garden. The desert is two-way. From greed on, the way up is sealed
and teleport does not work. Taking the Sword of the Spirit in the
garden returns you to the stair you entered by and seals the branch.

Matthew 4:4 on the desert is the full KJV, including "But he
answered and said,". The plan's engraving omitted that opening.
Ephesians 6:17 in the garden keeps the KJV colon.

The desert does not make you hungry when you arrive. Four loaves
are scattered on the sand, not piled in one place. They are named
food rations. Eating one says the dust line and makes you hungrier.
That hunger is ordinary hunger, so it is still with you in greed.

Greed is a pillared great room, a central hall, and two side
treasure rooms, then the narrow gate. Matthew 16:26 is engraved
at the entrance and Job 1:21 at the gate. Anything in your pack,
gold included, sends you back to the start and cycles Matthew
6:19, Luke 18:22, and Job 1:21, full KJV. Empty-handed, only
objects you yourself dropped are moved into the next room, with
Matthew 6:33. The four lure gems never come with you: not through
the gate, and not onto the next level. They are an exquisite
flawless diamond, a flawless emerald, a flawless ruby, and a
flawless sapphire.
Food turns to ash. After 25 turns on the level, counting through
every reset, the hunger warning appears, including Matthew 6:21.
After 50 turns the tombstone is "Starved, clinging to worldly
possessions."

Pride has eight rooms. Each game puts one lure in each room.
A mirror outside the door is harmless. The mirror inside is
live: picking up the lure, stepping onto that mirror, or kicking
it says "Your reflection smiles, and steps out of the glass."
The stone is engraved with James 4:10. Praying there says
Proverbs 16:18, crumbles the lures, turns the treasure doors into
undiggable walls, and opens the wall between the hall and the
down stair. A reflection drains more life each turn it is near you.
The +12 sword and +12 dragon scales are real. The ring stops
sorcery on this level. The rod's zap is a cold blast of radius 3.
The book is the existing death spell, now named finger of
execution, and on this level it succeeds only 30% of the time.
The lamp refuses with its line. The bag weighs nothing. The
amulet, if you die wearing it, shows the vanity death and the
tombstone "Killed by their own vanity, believing they were immortal."
The line "The amulet seems to flare with light." and "You really
died, a fool killed by your own vanity." were not quoted in the
plan; the tombstone and Proverbs 16:18 were.

Fear uses the three approved path maps, one per game. Each path
tile is the next piece of Psalm 91, exact KJV, wrapping after the
last verse. Stepping on a tile shows that text. On the path or
either stair, monster damage does not land, and monsters cannot
step onto the path or the stairs. Attacking a monster, or leaving
the path, removes the protection until you step onto the path
again. The garden's entry line is the one in the plan. Trees stand in
the four corners. The last of Psalm 91, exact KJV, rings the
altar, but the altar square itself is not engraved, so stepping
there shows the sword instead of a verse. Praying on that
altar crowns you, the same crowning as anywhere else, and it
does so every time until you already are the Servant of the
Lord. The Sword of the
Spirit there is blessed and +2, and it stays +2 if it is created
again. No monsters are generated there, and none can step onto
the level. Drinking from a fountain says "The water is living.
It restores you." Hit points return to full and hunger is cured.
Dipping says "The living water does no harm." There is no snake,
no demon, and no curse.

### Sword of the Spirit hits (2026-11-03)

Every melee hit adds d(experience level, 2) on top of the blade's
own dice, its +2, and the holy light's 2d4. That 2d4 still happens
only when the to-hit roll is 8 or less. The paralyze chance is
unchanged. On Sodom and Tyre that whole hit total is doubled, so
the double covers this bonus. On the Sanctum the same total is
five fourths, rounded down, with no message, so the level dice
and the holy light are scaled the same way. When the holy light
does not print its own line, the ordinary hit message is shown.

### Long sword for every playable role (2026-11-03)

The Sword of the Spirit is a doubled-edged blade, trained as a
long sword. Royal Historian, Healer, Priest, and Prophet can
train it to Basic. It starts Unskilled. Knight stays Expert,
Adventurer stays Skilled, and Champion stays Expert.

### Sword of the Spirit proof (2026-11-03)

The garden sword is created blessed, +2, and erodeproof, with
oeroded and oeroded2 both 0. NetHack keeps one erodeproof flag.
erode_obj() consults that flag for burn, rust, rot, corrosion,
and cracking, after it asks whether the material can take that
kind of damage. This blade is iron, so it can rust and corrode,
and the flag stops both. Iron is not flammable or rottable, so
fire and rot do not start; the same flag would stop them if they
did. Identified, the adjective is "rustproof".

### Prophetic skills (2026-11-03)

The #enhance screen, and the same list in the dumplog, heads the
spell group "Prophetic Skills". The seven schools use the names
already shown for a prophecy's category: spiritual warfare,
healing, discernment, blessing, prayer, deliverance, and creation.
They stay lowercase, with no "spells" suffix.

### Babel and Jacob's Ladder (2026-11-01)

Sokoban still climbs from the entrance (soko4) to the prize
(soko1). That prize room is the end. It no longer fills with a
zoo, and the level does not generate random monsters. The two
mimics on the map are still placed. Six peaceful confused
humans wait there. A branch stair in that room climbs to
Jacob's Ladder, one lit garden, with no generation. Trees
ring the room. Six fountains are living water: the same
restore as the Wilderness garden (full HP and power, ailments
cleared). A seen magic portal returns to the Sokoban entry
level, on the stair where that branch is first entered. The
portal's destination is stored on the trap, so a save keeps it.
Six builders stand toward the corners of the prize room.
None stands on the branch stair. Two of the original eight
were removed.

### Jacob's garden (2026-11-04)

The room above Sokoban is a garden: trees, and six fountains of
living water. Drinking one does what the Wilderness garden
fountains do. A seen magic portal on that level goes to the
Sokoban entry level and puts the hero on the stair where that
branch is entered. The destination is kept on the portal trap.

Every builder says "We built to reach heaven, and forgot what
we had already lost." Then one of these:

- "The higher we climbed, the less we remembered why."
- "We called the work our own, and the work called us proud."
- "Man's knowledge grew great, and his heart grew small."
- "We were making a name, and losing the Name that made us."
- "Every new course of stone was another reason not to kneel."
- "We could measure the height. We could not measure ourselves."

None of them says "AI".

The six names are Akkadian, Sumerian, Hebrew, Canaanite,
Egyptian, and Elamite. No two share a name.

Three also speak before the English line. These are for
verification, not a certified translation:

- Hebrew builder: הבה נבנה לנו עיר ומגדל
  (meant as unpointed Genesis 11:4, "come, let us build ourselves a city and a tower")
- Akkadian builder: šarrum dannum
  (meant as "mighty king")
- Sumerian builder: e2 an-še3
  (meant as "house toward heaven")

Canaanite, Egyptian, and Elamite say only the English lines.
No ancient phrase was added for them.

The angel says "You seek what was taken from you, though you know it not."
The first time, a blessed +2 Gospel of Peace is given, and the
angel steps two squares toward the chest. The chest holds a
diamond named "PLACEHOLDER main-quest artifact". It does
nothing. The engraving is Genesis 28:12 KJV. The plan names
Jacob's dream and does not quote that verse.

### The soul in the Sanctum of the Great Dragon (2026-11-01)

The Amulet of Yendor is not given to the high priest. A chest on
the Sanctum altar holds one object, named from the player's name
as "Elisha's soul" (always with 's, even if the name already ends
in s). Until it is identified it looks like a chained relic called
by that name. Opening the chest shows four paragraphs on one
black screen, the same window a Wilderness floor intro uses.
The last paragraph is Romans 10:9 KJV, with no citation line.
After that screen is dismissed, the only question is "Dost thou
confess with thy mouth the Lord Jesus, and believe in thine
heart that God hath raised him from the dead?" The answer is
yes or no. There is no quit that asks again later. The first
open does not also take the soul out. A second open is ordinary
looting.

Stepping into the Sanctum temple, while the high priest can
speak and you are not deaf, that priest says "Welcome, we have
long awaited thee. Look thou within the chest, and receive thy
reward." The welcome leaves the priest peaceful. Attacking
still asks the ordinary question, then she turns hostile and the
smite is unchanged. Killing that priest, or any priest of the
Great Dragon, is not murder: no "You murderer!", and no Favor
loss. The

lead-in "The high priest intones:" still appears on its old
timer, and on the Sanctum it names the priest of the Great Dragon.
An endgame temple still does not name the altar's owner. Other
temples are unchanged.

The first arrival on the Sanctum while the Sword of the Spirit is
carried, wielded or not, shows one black screen: "As you enter
the Sanctum of the Great Dragon, the Sword of the Spirit glows
with power. For it is written: 'And take the helmet of salvation,
and the sword of the Spirit, which is the word of God.'" It is
once per game. Arriving without the sword shows nothing, and a
later arrival with the sword still shows it if it has not been
seen. The flag sits in the spare bits of the event word, so the
save format is unchanged.

Where the unaligned god was named Moloch in play, the name is
"the Great Dragon", capitalized when it starts a sentence. That
is the priest, the high altar, the #turn line
in Gehennom while the random god is that name, the historian's
amulet speech (without "the Wizard and"), the priest quest's
nemesis death, and the oracle paragraph. The encyclopedia, the
backup quest file, and comments are unchanged.

Offering the real Amulet of Yendor on the Sanctum high altar
prints "You feel ashamed." The Amulet is not used up, you live,
and the alignment record is not touched. It takes a turn. An
offering on the Astral Plane is unchanged.

After the yes path has frozen the Sanctum, a blow that would
have called down the smite on that high priest does not. Lucifer's
wrath is turned aside, and the LORD's fire consumes the priest.
A priestess is "she"; a priest is "he". You take none of that
bolt. Before the freeze, the smite is the stock lightning.

Both answers say the shadows line, then six hostile demons appear
beside you: horned devil, barbed devil, vrock, hezrou, marilith,
pit fiend. That roster is a placeholder for "the hosts".

Yes leaves them free to move for two turns. They do not attack,
cast, or throw. Then the light line, the whole Sanctum freezes
for the rest of the game, and a blessed +2 Cap of Salvation is
given. The artifact is not renamed. Frozen monsters never move or
fight. You can still kill them. The stairs up, and any other way
off the level, say the Road to Salvation is not yet open.

No lets them attack for three turns. Then God's spoken line is
shown exactly as written, not as Deuteronomy, and the tombstone
is "Killed by the hosts of darkness, having refused redemption."
During those turns, and during the two quiet turns, you cannot
leave the level.

The elemental planes still look for the Amulet if one exists.
None is placed. The soul is the quest object. Wishes for it are
refused.

### Entering the Sanctum of the Great Dragon (2026-11-01)

The Bell of Opening, the Candelabrum of Invocation, and the Book
of the Dead are not placed, cannot be wished for, and crumble if
one is used. Vlad does not carry the candelabrum. A quest nemesis
does not carry the bell. The wizard's tower does not hold the book.
Reading the book no longer performs the invocation. Class quest
artifacts are unchanged. Stock quest dialogue may still mention
the silver bell. That pager which sent you back for the bell is
no longer shown.

Completing a class quest places a blessed Lamp of God's Word in
inventory, once. Applying it lights it and does not snuff it on
the next apply, so it can be read again. The first time, it shows
Psalm 119:105 KJV without the acrostic heading. On the vibrating
square's level the flame bends toward a compass point, or burns
straight up on the square itself. Off that level it bends downward
or upward.

Praying on the vibrating square, even during a prayer timeout,
shows Ephesians 6:13 KJV, replaces a 9x9 square with floor and an
undiggable wall, and puts down stairs in the middle. Those stairs
are the Gates of Hell. They do not open until the Key to the Gates
of Hell is applied while standing on them. That shows Matthew
16:18 KJV, then Psalm 107:16 KJV. Going down before that says the
gates stay shut and names Peter's key. No Armor of God piece is
required. Opening the gates does not pretend the Wizard is already
dead. Prophet mode can levelport with "vibrating" or "gates".

The lines that are not in the plan, and the message order, are
listed in the pull request for Jon. The plan still marks that
order as pending.

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
| ID-223 | Resolved: a broken rod or the drum of earthquake no longer prints the placeholder. A lawful altar stands. An evil altar is razed. |
| ID-224 | Resolved: anointing says "the Servant of the Lord". The gift is a placeholder. |
| (new) | Resolved: the owner named the 5.0 stasis rod "stillness" (`WAN_STILLNESS`, "rod of stillness", Psalm 46:10). |
| ID-084 | Healer's oil rows are tagged `POTION_CLASS` even though the objects are `ANOINTING_CLASS`. Creation uses `trotyp`, so the game start still works. |
| ID-017 | Only human is playable. `src/u_init.c` still has tables for the removed roles. |

### Hinnom battle batch (2026-11-04)

The crash after razing Moloch's altar was the Orcus Town level file,
not Hinnom. `dat/orcus.lua` still asked for `"magic marker"`, which
is the writing tablet here, so half the time building that level
panicked with "Unknown object id". Orcus Town can be the level
under Hinnom, and the raze is what opens the way down. The file now
asks for `"writing tablet"`. While Moloch is alive, Hinnom also has
no way down: `hinnom_dest_clamp()` refuses any level change to a
deeper level (stairs, a fall, level teleport, prophet mode too) with
"You cannot go down until Moloch is defeated." (placeholder). The up
stairs and every route inside the level are unchanged.

Every #chat with Michael on Hinnom while the field stands now opens a
blackout text window with his briefing (placeholder text), addressed
"Hear me, <rank> of the Most High." The rank is the hero's current
rank title for the hero's gender, from `rank_of()`. His usual line
follows: the cloak on the first talk, the field line after that. The
arrival screen with "good sir" / "good lady" is unchanged. The
window code in `src/hinnom.c` is now one helper, `hinnom_screen()`,
shared with the Berachah arrival text.

Before the field falls, each of the four angels around Michael has
three lines of his own, one picked at random on each #chat. The angels
are told apart by creation order (`m_id`), which follows
`dat/hinnom.lua`: angel 1 is northwest of Michael, 2 northeast, 3
southwest, 4 southeast. They still hold their squares until the
assault (`hinnom_host_hold()`), and the Berachah victory lines are
separate and unchanged.

The first time the fortress's peace breaks while the field still
stands, a blackout screen says the disguise is broken. It is shown
from `hinnom_alarm_fortress()`, which sets the alarm bit, so it can
only show once. When the field falls the alarm is raised after the
bars are gone, so that screen is skipped and the battle screen is the
one shown. Two ways of being caught without the cloak now raise the
same alarm: standing next to a fortress inhabitant or gate guard
(`src/monmove.c`), and asking a gate guard to pass. Before, those
turned only that one monster hostile.

When the fifth lever drops the field, a blackout screen shows the
field breaking and the Host charging. It is shown from
`hinnom_field_down()`, which runs once, since the levers go dead with
the field. It replaces the two placeholder lines "The hum dies. The
barrier falls." and "Michael and his angels charge the fortress!".
The disguise-broken screen is not shown with it.

When the power field falls, Michael the Archangel and two angels are placed in the throne room and now make straight for Moloch (a Hinnom-only goal in m_move, marked by a new mstrategy bit). Four other angels take the foyer; extra angels are made if fewer than six are left. The foyer spots are inside the real room walls.

Power levers no longer trigger the paranoid "Really step onto that power lever?" question and are no longer skipped by the 1-in-5 "You escape" roll; walking onto one always asks "Throw the ... lever?". Flyers and levitators still pass over. All other traps are unchanged.

### Knight conduct (2026-11-04)

`check_caitiff()` in `src/uhitm.c` is the Knight penalty for striking
someone who is asleep, paralyzed, or fleeing without vengeance. It
prints "You caitiff!" and takes one alignment point. It does not
change Favor. A hostile monster no longer triggers it, whether that
monster is fleeing or helpless. A peaceful monster still does. The
same function is used for a kick, a polearm, and a grappling hook.

### Hinnom follow-ups (2026-11-04)

The map is placed one column to the right of the numbers in
`dat/hinnom.lua`, and the level does not flip. The gate, the
drawbridge span, the levers, and the back room use that shift,
so the drawbridge actually lowers when a cloaked hero is passed
or the key is used.

`#chat` with a priest or priestess of the Great Dragon, while the
High Priest of Moloch's cloak is worn and the fortress is not
alarmed, stays on the Hinnom lines and does not call the temple
anger in `priest_talk()`. A priest anywhere else is unchanged.
Killing a priest of the Great Dragon, a roving one as well as a
temple one, or a fortress guard or the doctor, does not print
"You murderer!" and does not take the murder Favor loss.

The cloak has a case in `Cloak_on()` and `Cloak_off()`, so wearing
it does not print "Unknown type of cloak". Every fortress
inhabitant, and the two gate guards outside the field, starts
peaceful and meditating. They stay on their square until they
are attacked, the alarm sounds, or the hero steps next to them
without the cloak. With the cloak on, the doctor gives the first
lever, an inside guard the second, and a priest or priestess the
third. The fourth clue is a burn engraving on the lab floor
against the north wall (map column 55, row 5). The fifth is the
same, against the south wall of the lever room (map column 58,
row 15). The engraving glyph shows once that square is seen, and
the text does not fade. Random floor objects inside the field
are removed after level fill. The lab supplies and the keys stay.
The steed's name is "fiery steed of the Heavenly Host"
(`FIERY_STEED_OF_HEAVEN`; the old spelling was only in the
display name and comments, so existing saves keep the same
monster number). Going down stairs on that steed flies down.
The saddle is the Knight's quest artifact. Changing levels used
to drop every quest artifact a monster carried, and dropping the
worn saddle threw the rider before the stair message. A worn
saddle stays on the steed; the Amulet and the invocation tools
are still dropped. A power lever is a floor trap: walking can
throw it, and flying or riding a flyer passes over it without
`immune_to_trap: bad ttype`.

### The Battle of Hinnom and the Valley of Berachah (2026-11-03)

EDITLEVEL is 10. New monsters: Michael the Archangel, Moloch, child, fortress
guard, doctor. New objects: High Priest of Moloch's cloak, Key to
Hinnom's Gate, Key to the Sanctum, Tears of the Children. New trap:
power lever. New event bits: hinnom_cloak, hinnom_alarm,
hinnom_razed, berachah, berachah_tears. Files: dat/hinnom.lua,
dat/berachah.lua, src/hinnom.c. The level is chained under baalz.
The overview name is The Battle of Hinnom, then The Valley of
Berachah after the swap.

#raze (M-z) is a five-turn occupation on an altar that is not
lawful and not a high altar. Success is +5 alignment, +1 Favor,
the raze line, and Deuteronomy 7:5. A lawful altar and a high
altar are refused and take no time. Prayer on an altar that is
not lawful is refused and does not start a prayer. A priest
created on such an altar starts hostile. A broken rod of digging
and the drum of earthquake leave a lawful altar and a high altar
standing, and raze an evil altar.

Peter's Key to the Gates of Hell is required at the Valley down
stair. Other ways into Gehennom before that stair are sent to the
Valley. The revealed stairs are the Gate of the Sanctum and open
with the Key to the Sanctum. Peter's key does not fit that gate.
The verses on that use are unchanged.

A shop square is a mimic, instead of an item, with probability
equal to the dungeon depth (`mkshobj_at` in `shknam.c`). Berachah
is deep, so that used to fill the Scribes of Ezra, the Library
of the Prophets, and the Sheep Gate Market. That roll is skipped
only when the level is Berachah. No other shop is changed, and
mimics outside shops are unchanged.

Hinnom's level file has `noflip`. Berachah's does too. The
loader's flip is also refused while that dungeon level is being
built or is current, so a rebuild, `nh.flip_level`, or
`#wizfliplevel` cannot mirror it. The up stair stays at the
bottom left and the fortress stays on the east. The arrival
line "To the north" matches that layout.

Hinnom does not grow random monsters inside the field while the
fortress is still closed. A wanderer that is already inside, or
that tries to walk in, is put back outside and does not attack
the disguised hero. Only the placed garrison counts as the
fortress, so striking a stalker does not raise the alarm. The
alarm is still the hero's own act: attacking that garrison,
standing next to them without the cloak, taking the cloak off,
forcing the gate, or dropping the field.

Hinnom: Michael the Archangel gives the cloak once. He and the
angels with him meditate in the portal room and do not leave
their squares until the force field drops, or until that one is
hurt, hostile, or under conflict. The displayed name is
"Michael the Archangel". Saves store his monster number, which
did not move, so an older save still has him. Wearing the cloak, the fortress
inside the field stays peaceful until the cloak comes off, you
attack, the field falls, or the gate key is used. The two gate
guards open the field gate and lower the drawbridge if the cloak
holds. Without it, only those two turn hostile. Five power levers
must be thrown in a random order. A wrong pull is 4d6 shock,
blocked by shock resistance, and resets the levers. The right
order drops the iron bars. Razing the altar of Moloch stops
random monsters on the level, unlocks the back room, and cuts
Moloch's current hit points to a quarter if he is on the level.
Leaving with Moloch dead and the altar razed replaces the level
with Berachah. Living children on the way out give +1 Favor and
+1 alignment each. A child who dies on Hinnom costs 5 alignment
and 2 Favor.

The six angels on Berachah each speak one of Jon's victory
lines, in the order they were placed. Michael still says
"Well done. Rest here a while." A seventh angel would take
the first line again, unless that angel is standing next to
one who already has it. The Valley's arrival text is a
full-screen window, the same kind the quest intro uses. The
swap runs while the hero is leaving, so the window waits
for the first step back into the Valley. `berachah_intro`
remembers that showing. The bit sits in padding that was
already part of `struct u_event`, so the save size is the
same.

Berachah has no random monsters. Three shops: the Scribes of Ezra
(Baruch), the Library of the Prophets (Nathan), the Sheep Gate
Market (Lydia). The first #chat with a child gives the Tears of
the Children, once. After that gift, each of the six children
has his own thank-you. Quaffing the Tears grants a level. They
do not break, dilute, dip, or curse. The two fountains, and the
Wilderness garden fountains, are living water.

On Hinnom, with the High Priest of Moloch's cloak worn and
the peace unbroken, #chat reaches every fortress creature.
The outside guards still say "Pass, Your Holiness." The
inside guard and the clerics still give their lever clues
and nothing else. The doctor gives his lever clue on every
chat, and then one of six lines of his own, so the clue is
never the one that failed to come up. The other nine (both
horned devils, both barbed devils, the bone devil, the ice
devil, the nalfeshnee, and both pit fiends) share eight
court lines. The first eight each have their own. The ninth
repeats the first, and that repeat is not next to the
creature who already has it. Moloch is not in that set. On
#chat he picks one of four lines of his own. The first time
an attack of his actually proceeds, he may say one of them
once. The swing,
the damage, and the raze are the same. Horned, barbed,
bone, and ice devils are silent in
the rest of the game; this chat is allowed before that
check. The six captive children, before the Valley, each
pick one of six lines about the doctor. After the Tears,
the Valley children still use their own thank-yous.

A male prisoner named "town elder" stands one square inside the
open gate (map 43,10). He is not a shopkeeper and not a priest.
A guide was not used: killing a guide also applies the
quest-guardian penalty. While the elder is peaceful he stays
on that square. #chat picks one of four lines; those lines are
still PLACEHOLDER. Killing him is murder, the same as any
other peaceful human.

Lines Jon has not written are marked PLACEHOLDER in the source.
The list is in the pull request.

### Not lost

No inventory item was dropped on the floor. Clutter from section 18 is
still in the tree, including `scratch.c`, `src/tile 2.c` through
`src/tile 5.c`, `dat/quest_BAK.lua`, `dat/Kni-strtBAK.lua`,
`tools/parse_objectsBAK.py`, `tools/bak.html`, and `sys/lib/lua-5.4.6`.

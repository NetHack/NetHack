#!/usr/bin/env python3
"""Regenerate docs/RighteousHack_Food.xlsx from the food and monster tables.

Run from the repository root:

    python3 tools/gen_food_xlsx.py

Reads include/meatnoun.h, include/objects.h, include/monsters.h, and
include/monflag.h. Update this script if those table formats change.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

try:
    from openpyxl import Workbook
    from openpyxl.styles import Alignment, Font, PatternFill
    from openpyxl.utils import get_column_letter
    from openpyxl.worksheet.table import Table, TableStyleInfo
except ImportError:
    sys.stderr.write("openpyxl is required: pip install openpyxl\n")
    sys.exit(1)

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "RighteousHack_Food.xlsx"

# Must stay in step with src/mon.c undead_to_corpse() and the golem/pudding
# cases in make_corpse() that do not fall through to a meat drop.
UNDEAD_BODY = {
    "kobold zombie": "kobold",
    "kobold mummy": "kobold",
    "dwarf zombie": "dwarf",
    "dwarf mummy": "dwarf",
    "gnome zombie": "gnome",
    "gnome mummy": "gnome",
    "orc zombie": "orc",
    "orc mummy": "orc",
    "elf zombie": "elf",
    "elf mummy": "elf",
    "vampire": "human",
    "vampire leader": "human",
    "human zombie": "human",
    "human mummy": "human",
    "giant zombie": "giant",
    "giant mummy": "giant",
    "ettin zombie": "ettin",
    "ettin mummy": "ettin",
}
# Always-compiled make_corpse() cases that drop something other than meat.
GOLEM_NO_MEAT = {
    "iron golem",
    "glass golem",
    "clay golem",
    "stone golem",
    "wood golem",
    "rope golem",
    "leather golem",
    "gold golem",
    "paper golem",
}
RIDERS = {"death", "pestilence", "famine"}
# 5% exemption is lizard, S_EEL, and S_JELLY. Only lizard and S_JELLY
# are stable (never rot, blessable raw). Fillets rot unless cooked.
EXEMPT_NOTE = (
    "Skip the 5% raw-poison roll: the lizard, every jelly, and every "
    "S_EEL fillet (jellyfish, piranha, shark, giant eel, electric eel, "
    "kraken). Lizard meat and jellies never rot and may be blessed raw. "
    "A fillet rots on the normal raw-meat schedule, cannot be blessed "
    "while raw, and once cooked rots about ten times slower. "
    "Blessed cooked fillet never rots."
)

ESSENCE_MLETS = {
    "S_GHOST",
    "S_WRAITH",
    "S_VAMPIRE",
    "S_ZOMBIE",
    "S_MUMMY",
    "S_NYMPH",
    "S_DEMON",
}
# Humanoids named by the owner. Other orc-class monsters are not here.
ESSENCE_NAMES = {
    "mind flayer",
    "master mind flayer",
    "goblin",
    "quantum mechanic",
    "genetic engineer",
    "medusa",
    "wizard of yendor",
    "croesus",
    "charon",
}

HEADER_FILL = PatternFill("solid", fgColor="1F4E79")
HEADER_FONT = Font(color="FFFFFF", bold=True)
WRAP = Alignment(wrap_text=True, vertical="top")


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_defines(text: str, prefixes: tuple[str, ...]) -> dict[str, int]:
    """#define NAME value, with values that may refer to earlier names."""
    table: dict[str, int] = {}
    for match in re.finditer(
        r"^#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.+)$", text, re.M
    ):
        name, raw = match.group(1), match.group(2).split("/*")[0].strip()
        if not name.startswith(prefixes):
            continue
        raw = raw.rstrip("L").strip()
        try:
            table[name] = int(raw, 0)
            continue
        except ValueError:
            pass
        if raw in table:
            table[name] = table[raw]
    return table


def eval_expr(expr: str, table: dict[str, int]) -> int:
    total = 0
    expr = re.sub(r"/\*.*?\*/", "", expr, flags=re.S)
    for part in expr.replace("\n", " ").split("|"):
        part = part.strip().strip("()")
        if not part:
            continue
        part = part.rstrip("L")
        if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", part):
            total |= int(part, 0)
        elif part in table:
            total |= table[part]
    return total


def parse_nouns(text: str) -> dict[str, str]:
    nouns = {}
    for sym, word in re.findall(r'MEAT_NOUN\(\s*(S_[A-Za-z0-9_]+)\s*,\s*"([^"]+)"\s*\)', text):
        nouns[sym] = word
    if "S_DRAGON" not in nouns or "S_EEL" not in nouns:
        sys.exit("meatnoun.h did not parse; expected S_DRAGON and S_EEL")
    return nouns


def parse_foods(text: str) -> list[dict]:
    rows = []
    for match in re.finditer(
        r'FOOD\(\s*"([^"]+)"\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*(-?\d+)',
        text,
    ):
        name, prob, delay, wt, _unk, material, nutrition = match.groups()
        rows.append(
            {
                "name": name,
                "prob": int(prob),
                "delay": int(delay),
                "weight": int(wt),
                "nutrition": int(nutrition),
                "material": material,
            }
        )
    ring = re.search(
        r'OBJ\("meat ring".*?,\s*(\d+)\s*,\s*CLR_',
        text,
        re.S,
    )
    # meat ring is an OBJECT, not FOOD. Pull the fields we know from the macro.
    if 'OBJ("meat ring"' in text:
        rows.append(
            {
                "name": "meat ring",
                "prob": 0,
                "delay": 1,
                "weight": 5,
                "nutrition": 5,
                "material": "FLESH",
            }
        )
    if not rows:
        sys.exit("no FOOD() rows parsed from objects.h")
    return rows


def split_top_args(block: str) -> list[str]:
    assert block.startswith("MON(")
    body = block[4:]
    if body.endswith(","):
        body = body[:-1]
    if body.endswith(")"):
        body = body[:-1]
    args: list[str] = []
    depth = 0
    cur: list[str] = []
    for ch in body:
        if ch == "(":
            depth += 1
            cur.append(ch)
        elif ch == ")":
            depth -= 1
            cur.append(ch)
        elif ch == "," and depth == 0:
            args.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if cur:
        args.append("".join(cur).strip())
    return args


def monster_blocks(text: str) -> list[str]:
    blocks = []
    idx = 0
    while True:
        j = text.find("MON(", idx)
        if j < 0:
            break
        line_start = text.rfind("\n", 0, j) + 1
        prefix = text[line_start:j]
        if "define" in prefix or prefix.strip().startswith("*"):
            idx = j + 4
            continue
        depth = 0
        k = j
        while k < len(text):
            if text[k] == "(":
                depth += 1
            elif text[k] == ")":
                depth -= 1
                if depth == 0:
                    k += 1
                    break
            k += 1
        blocks.append(text[j:k])
        idx = k
    return blocks


def strip_if0(text: str) -> str:
    """Drop `#if 0` regions so unused monsters stay out of the workbook."""
    out = []
    skip_stack: list[bool] = []
    for line in text.splitlines(keepends=True):
        stripped = line.lstrip()
        if stripped.startswith("#if"):
            parent = skip_stack[-1] if skip_stack else False
            this = parent or stripped.startswith("#if 0") or stripped.startswith("#if\t0")
            skip_stack.append(this)
            if this:
                continue
        elif stripped.startswith("#endif"):
            skipping = skip_stack.pop() if skip_stack else False
            if skipping:
                continue
        elif skip_stack and skip_stack[-1]:
            continue
        out.append(line)
    return "".join(out)


def parse_monsters(text: str, flags: dict[str, int]) -> list[dict]:
    text = strip_if0(text)
    mons = []
    for block in monster_blocks(text):
        # Comments can contain commas (erinys). Strip them before splitting.
        block = re.sub(r"/\*.*?\*/", " ", block, flags=re.S)
        args = split_top_args(block)
        if len(args) < 11:
            continue
        names = re.findall(r'"([^"]+)"', args[0])
        if not names:
            continue
        # NAMS(male, female, neutral) or NAM(name)
        if args[0].lstrip().startswith("NAMS(") and len(names) >= 3:
            male, female, neutral = names[0], names[1], names[2]
            display = neutral
        else:
            male = female = display = names[0]
        mlet_m = re.search(r"\b(S_[A-Za-z0-9_]+)\b", args[1])
        if not mlet_m:
            continue
        siz = re.search(
            r"SIZ\(\s*([^,]+)\s*,\s*(\d+)",
            args[5],
        )
        weight = siz.group(1).strip() if siz else ""
        nutrition = int(siz.group(2)) if siz else 0
        size_m = re.search(r"\b(MZ_[A-Z]+)\b", args[5])
        size = size_m.group(1) if size_m else ""
        attacks = " ".join(re.findall(r"\bAD_[A-Z0-9]+\b", args[4]))
        mons.append(
            {
                "name": display,
                "male": male,
                "female": female,
                "mlet": mlet_m.group(1),
                "geno": eval_expr(args[3], flags),
                "weight": weight,
                "nutrition": nutrition,
                "size": size,
                "resists": eval_expr(args[6], flags),
                "conveys": eval_expr(args[7], flags),
                "m1": eval_expr(args[8], flags),
                "m2": eval_expr(args[9], flags),
                "m3": eval_expr(args[10], flags),
                "attacks": attacks,
            }
        )
    if len(mons) < 100:
        sys.exit(f"only parsed {len(mons)} monsters; the MON() parser missed the table")
    return mons


def bit_names(value: int, table: dict[str, int], prefixes: tuple[str, ...]) -> list[str]:
    names = []
    for name, bit in table.items():
        if not name.startswith(prefixes) or bit == 0:
            continue
        if value & bit == bit and (bit & (bit - 1)) == 0:
            names.append(name)
    return names


CONVEY_LABEL = {
    "MR_FIRE": "fire resistance",
    "MR_COLD": "cold resistance",
    "MR_SLEEP": "sleep resistance",
    "MR_DISINT": "disintegration resistance",
    "MR_ELEC": "shock resistance",
    "MR_POISON": "poison resistance",
    "MR_ACID": "acid resistance",
    "MR_STONE": "stoning resistance",
    "MR2_SEE_INVIS": "see invisible",
    "MR2_LEVITATE": "levitation",
    "MR2_WATERWALK": "water walking",
    "MR2_MAGBREATH": "magical breathing",
    "MR2_DISPLACED": "displacement",
    "MR2_STRENGTH": "strength",
    "MR2_FUMBLING": "fumbling",
}


def is_humanoid_body(mon: dict, flags: dict[str, int]) -> bool:
    if mon["name"].lower() in RIDERS:
        return False
    return bool(
        (mon["m1"] & flags["M1_HUMANOID"])
        or (mon["m2"] & flags["M2_HUMAN"])
        or (mon["m2"] & flags["M2_ELF"])
    )


def leaves_essence(mon: dict, flags: dict[str, int]) -> bool:
    key = mon["name"].lower()
    if key in RIDERS or key in ESSENCE_NAMES:
        return True
    if mon["m2"] & flags["M2_DEMON"]:
        return True
    return mon["mlet"] in ESSENCE_MLETS


def is_stable_remains(mon: dict) -> bool:
    return mon["mlet"] == "S_JELLY" or mon["name"].lower() == "lizard"


def demon_flagged(mon: dict, flags: dict[str, int]) -> bool:
    return bool(mon["m2"] & flags["M2_DEMON"])


def classify(mon: dict, by_name: dict[str, dict], flags: dict[str, int]) -> tuple[str, str]:
    """Return (sheet, reason)."""
    key = mon["name"].lower()
    if leaves_essence(mon, flags):
        if demon_flagged(mon, flags):
            why = (
                "Demon (M2_DEMON). Leaves an essence. "
                "Eating it is instantly fatal."
            )
        elif key in RIDERS:
            why = (
                "Rider. S_DEMON but not M2_DEMON. Leaves an essence. "
                "Eating it is fatal, and the Rider comes back."
            )
        elif mon["mlet"] == "S_DEMON":
            why = (
                "S_DEMON but not M2_DEMON. Leaves an essence. "
                "Eating it is not fatal."
            )
        else:
            why = "Leaves an essence. Never spoils, cannot be cooked, can be blessed."
        return "essence", why
    if is_stable_remains(mon):
        if mon["mlet"] == "S_JELLY":
            why = "Jelly. Stable food: never rots, no 5% roll, blessable while raw."
        elif mon["name"].lower() == "lizard":
            why = "The lizard only. Stable while raw. Newt and the other lizards are not."
        else:
            why = "Sea fillet. Stable while raw: never rots, no 5% roll, blessable."
        return "stable", why
    if mon["mlet"] == "S_TROLL":
        return (
            "meat",
            "Troll. Flagged humanoid, but still leaves meat so it can revive.",
        )
    if mon["mlet"] == "S_YETI":
        return (
            "meat",
            "Ape class. Flagged humanoid, but leaves meat.",
        )
    if mon["m2"] & flags["M2_WERE"]:
        form = "human form" if mon["mlet"] == "S_HUMAN" else "animal form"
        return (
            "meat",
            f"Lycanthrope, {form}. Eating it causes lycanthropy.",
        )
    if key == "cyclops":
        return "meat", "Giant. Named raw Cyclops steak. No \"the\" and no possessive."
    if mon["mlet"] == "S_LICH":
        return "other", "Crumbles into dust. Leaves nothing."
    if mon["mlet"] == "S_EEL":
        return (
            "meat",
            "Sea fillet. Skips the 5% raw-poison roll. "
            "Rots like other raw meat and cannot be blessed while raw. "
            "Cooked rots about ten times slower; blessed cooked never rots.",
        )
    body = mon
    undead_note = ""
    if key in UNDEAD_BODY:
        body = by_name.get(UNDEAD_BODY[key], mon)
        undead_note = f" Would have left {body['name']} meat."
    if is_humanoid_body(body, flags) or is_humanoid_body(mon, flags):
        why = "Humanoid, human, or elf. Leaves no meat." + undead_note
        return "humanoid", why
    if key in GOLEM_NO_MEAT or mon["mlet"] == "S_GOLEM" and key in GOLEM_NO_MEAT:
        return "other", "Golem. Leaves parts or a statue, not meat."
    if mon["mlet"] == "S_PUDDING":
        return "other", "Pudding or ooze. Leaves a glob, not meat."
    if "AT_BOOM" in mon["attacks"] or key == "gas spore":
        return "other", "Explodes. No meat."
    if mon["geno"] & flags["G_NOCORPSE"]:
        return "other", "G_NOCORPSE. The monster never leaves meat."
    if mon["mlet"] == "S_GOLEM":
        return "other", "Golem that is not a special-case drop. Check make_corpse()."
    return "meat", "Leaves meat when killed (some small monsters only sometimes)."


def meat_noun_for(mon: dict, nouns: dict[str, str]) -> str:
    key = mon["name"].lower()
    if key == "monkey":
        return "meat"
    if key == "cyclops":
        return "steak"
    return nouns.get(mon["mlet"], "meat")


def ends_with_word(name: str, word: str) -> bool:
    low = name.lower()
    w = word.lower()
    return low == w or low.endswith(" " + w)


def possessive(name: str) -> str:
    if name.lower() == "it":
        return name + "s"
    if name.endswith("s") or name.endswith("S"):
        return name + "'"
    return name + "'s"


def formatted_meat(mon: dict, nouns: dict[str, str], flags: dict[str, int], cooked: bool) -> str:
    noun = meat_noun_for(mon, nouns)
    prep = "cooked" if cooked else "raw"
    if mon["name"].lower() == "cyclops":
        return f"{prep} Cyclops steak"
    name = mon["name"]
    pname = bool(mon["m2"] & flags.get("M2_PNAME", 0))
    uniq = bool(mon["geno"] & flags.get("G_UNIQ", 0)) and not pname
    if ends_with_word(name, noun):
        body = f"{prep} {name}"
    elif pname or uniq:
        body = f"{possessive(name)} {prep} {noun}"
    else:
        body = f"{prep} {name} {noun}"
    if uniq:
        body = "the " + body
    return body


def dangers(mon: dict, flags: dict[str, int]) -> str:
    bits = []
    if mon["m1"] & flags["M1_POIS"]:
        bits.append("poisonous")
    if mon["m1"] & flags["M1_ACID"]:
        bits.append("acidic")
    if mon["name"].lower() in {"cockatrice", "chickatrice", "medusa"}:
        bits.append("petrifies if eaten")
    if mon["m2"] & flags.get("M2_WERE", 0):
        bits.append("lycanthropy")
    if mon["name"].lower() == "green slime" or "AD_SLIM" in mon["attacks"]:
        bits.append("slime")
    if "AD_DISE" in mon["attacks"]:
        bits.append("disease")
    if "AD_WERE" in mon["attacks"] and "lycanthropy" not in bits:
        bits.append("lycanthropy")
    return ", ".join(bits)


def intrinsics(mon: dict, flags: dict[str, int]) -> str:
    labels = []
    for name in bit_names(mon["conveys"], flags, ("MR_", "MR2_")):
        labels.append(CONVEY_LABEL.get(name, name))
    if mon["m2"] & flags.get("M2_GIANT", 0):
        labels.append("strength (giant)")
    # Famous one-off meals. Same whether raw or cooked.
    special = {
        "nurse": "heals",
        "wraith": "level gain",
        "newt": "magical energy",
        "floating eye": "telepathy sometimes",
        "mind flayer": "intelligence",
        "master mind flayer": "intelligence",
        "lizard": "cures stoning",
    }
    extra = special.get(mon["name"].lower())
    if extra:
        labels.append(extra)
    return ", ".join(labels)


def essence_effects(mon: dict, flags: dict[str, int]) -> str:
    """What eating the essence does, matching cprefx/cpostfx and corpse intrinsics."""
    key = mon["name"].lower()
    if demon_flagged(mon, flags):
        return (
            "instantly fatal (ate a demon's essence). "
            "Poison and acid on the monster are not applied."
        )
    bits: list[str] = []
    special = {
        "quantum mechanic": "speed may toggle",
        "genetic engineer": "polymorph",
        "sandestin": "polymorph",
        "medusa": "petrifies if eaten",
    }
    if key in RIDERS:
        bits.append("instantly fatal; the Rider comes back")
    extra = special.get(key)
    if extra:
        bits.append(extra)
    gained = intrinsics(mon, flags)
    if gained:
        bits.append(gained)
    if mon["m1"] & flags["M1_POIS"]:
        bits.append("poisonous (the monster's own flag)")
    if mon["m1"] & flags["M1_ACID"]:
        bits.append("acidic")
    if not bits:
        bits.append("nutrition only; stock had no special meal effect")
    if key in {"djinni", "sandestin", "mail daemon", "charon"}:
        bits.append("stock left no corpse")
    return "; ".join(bits)


def stable_flag(name: str) -> str:
    if name == "corpse" or name.startswith("glob of ") or name == "egg":
        return "no"
    return "yes"


def style_sheet(ws, headers: list[str], nrows: int) -> None:
    for col, header in enumerate(headers, 1):
        cell = ws.cell(1, col, header)
        cell.fill = HEADER_FILL
        cell.font = HEADER_FONT
        cell.alignment = Alignment(wrap_text=True, vertical="center")
    ws.freeze_panes = "A2"
    ws.auto_filter.ref = f"A1:{get_column_letter(len(headers))}{max(nrows, 1)}"
    ws.row_dimensions[1].height = 30
    for row in ws.iter_rows(min_row=2, max_row=max(nrows, 2), max_col=len(headers)):
        for cell in row:
            cell.alignment = WRAP
    # Readable widths. Long text columns get more room.
    widths = {
        1: 28,
        2: 36,
        3: 38,
        4: 16,
        5: 14,
        6: 22,
        7: 22,
        8: 42,
        9: 36,
        10: 55,
    }
    for col in range(1, len(headers) + 1):
        letter = get_column_letter(col)
        ws.column_dimensions[letter].width = widths.get(col, 24)
    ws.page_setup.orientation = "landscape"
    ws.page_setup.fitToPage = True
    ws.page_setup.fitToWidth = 1
    ws.page_setup.fitToHeight = 0
    ws.sheet_properties.pageSetUpPr.fitToPage = True
    ws.oddHeader.left.text = ws.title
    ws.print_title_rows = "1:1"
    if nrows >= 2:
        tab = Table(
            displayName="T_" + re.sub(r"[^A-Za-z0-9]", "", ws.title)[:20],
            ref=f"A1:{get_column_letter(len(headers))}{nrows}",
        )
        tab.tableStyleInfo = TableStyleInfo(
            name="TableStyleMedium2", showRowStripes=True
        )
        ws.add_table(tab)


def main() -> None:
    flag_text = read(ROOT / "include" / "monflag.h")
    flags = parse_defines(flag_text, ("M1_", "M2_", "M3_", "G_", "MR_", "MR2_", "MZ_"))
    for needed in ("M1_HUMANOID", "M2_HUMAN", "M2_ELF", "G_NOCORPSE", "M1_POIS", "M1_ACID"):
        if needed not in flags:
            sys.exit(f"missing {needed} in monflag.h parse")
    nouns = parse_nouns(read(ROOT / "include" / "meatnoun.h"))
    foods = parse_foods(read(ROOT / "include" / "objects.h"))
    monsters = parse_monsters(read(ROOT / "include" / "monsters.h"), flags)
    by_name = {m["name"].lower(): m for m in monsters}

    wb = Workbook()

    ws = wb.active
    ws.title = "Stable foods"
    headers = [
        "Name",
        "Nutrition",
        "Weight",
        "Eat delay",
        "Stable",
        "Generation probability",
        "Material",
        "Notes",
    ]
    for col, header in enumerate(headers, 1):
        ws.cell(1, col, header)
    r = 2
    for food in foods:
        stable = stable_flag(food["name"])
        note = ""
        if food["name"] == "corpse":
            note = (
                "Internal object is still CORPSE. Players see raw or cooked meat. "
                "Weight and nutrition come from the monster, not this row."
            )
        elif food["name"] == "tin":
            note = "Nutrition depends on the contents. See the Tins and eggs sheet."
        elif food["name"] == "egg":
            note = "Can hatch. Not part of the raw/cooked meat rules."
        elif food["name"].startswith("glob of "):
            note = "Left by puddings and oozes instead of meat. Shrinks as it ages."
        elif stable == "yes":
            note = "Unchanged by the meat rework."
        ws.append(
            [
                food["name"],
                food["nutrition"],
                food["weight"],
                food["delay"],
                stable,
                food["prob"],
                food["material"],
                note,
            ]
        )
        r += 1
    style_sheet(ws, headers, r - 1)

    meat_ws = wb.create_sheet("Monster meat")
    meat_headers = [
        "Monster",
        "Also called",
        "Class",
        "Raw name",
        "Cooked name",
        "Nutrition",
        "Weight",
        "5% raw-poison exempt",
        "Blessable while raw",
        "Intrinsics and effects",
        "Dangers",
        "Drop",
        "Notes",
    ]
    for col, header in enumerate(meat_headers, 1):
        meat_ws.cell(1, col, header)
    human_ws = wb.create_sheet("No meat humanoids")
    human_headers = [
        "Monster",
        "Also called",
        "Class",
        "Why no meat",
        "M1_HUMANOID",
        "Human",
        "Elf",
    ]
    for col, header in enumerate(human_headers, 1):
        human_ws.cell(1, col, header)
    other_ws = wb.create_sheet("Other no meat")
    other_headers = ["Monster", "Class", "Why no meat"]
    for col, header in enumerate(other_headers, 1):
        other_ws.cell(1, col, header)
    ess_ws = wb.create_sheet("Essences")
    ess_headers = [
        "Monster",
        "Also called",
        "Class",
        "Name",
        "Nutrition when eaten",
        "Listed nutrition",
        "Weight of essence",
        "Spoils",
        "Can cook",
        "Can bless",
        "5% raw-poison",
        "Eating fatal",
        "Demon-flagged",
        "Eat effects",
        "Revival",
        "Notes",
    ]
    for col, header in enumerate(ess_headers, 1):
        ess_ws.cell(1, col, header)
    stable_ws = wb.create_sheet("Stable remains", 1)
    stable_headers = [
        "Monster",
        "Class",
        "Raw name",
        "Cooked name",
        "Nutrition",
        "Weight",
        "Rots",
        "5% raw-poison",
        "Blessable while raw",
        "Intrinsics and effects",
        "Dangers",
        "Notes",
    ]
    for col, header in enumerate(stable_headers, 1):
        stable_ws.cell(1, col, header)

    meat_n = human_n = other_n = ess_n = stable_n = 1
    for mon in monsters:
        sheet, reason = classify(mon, by_name, flags)
        aliases = ""
        if mon["male"] != mon["name"] or mon["female"] != mon["name"]:
            aliases = f"{mon['male']} / {mon['female']}"
        if sheet == "stable":
            raw = formatted_meat(mon, nouns, flags, False)
            cooked = formatted_meat(mon, nouns, flags, True)
            stable_ws.append(
                [
                    mon["name"],
                    mon["mlet"],
                    raw,
                    cooked,
                    mon["nutrition"],
                    mon["weight"],
                    "never",
                    "no",
                    "yes",
                    intrinsics(mon, flags),
                    dangers(mon, flags),
                    reason + " Can still be cooked. Cooking does not make it spoil.",
                ]
            )
            stable_n += 1
            continue
        if sheet == "meat":
            raw = formatted_meat(mon, nouns, flags, False)
            cooked = formatted_meat(mon, nouns, flags, True)
            poison_exempt = "yes" if mon["mlet"] == "S_EEL" else "no"
            bless_raw = "no"
            size_val = flags.get(mon["size"], 0)
            always = (
                size_val >= flags.get("MZ_LARGE", 3)
                or bool(mon["m2"] & flags["M2_WERE"])
            )
            drop = "always" if always else "sometimes"
            meat_ws.append(
                [
                    mon["name"],
                    aliases,
                    mon["mlet"],
                    raw,
                    cooked,
                    mon["nutrition"],
                    mon["weight"],
                    poison_exempt,
                    bless_raw,
                    intrinsics(mon, flags),
                    dangers(mon, flags),
                    drop,
                    reason,
                ]
            )
            meat_n += 1
        elif sheet == "essence":
            listed = mon["nutrition"]
            eaten = listed if listed else 50
            key = mon["name"].lower()
            flagged = demon_flagged(mon, flags)
            fatal = flagged or key in RIDERS
            effects = essence_effects(mon, flags)
            if key in RIDERS:
                revival = "yes, revives from this essence"
            elif mon["mlet"] in {"S_VAMPIRE", "S_ZOMBIE", "S_MUMMY"}:
                revival = "undead turning brings this monster back from the essence"
            elif key == "wizard of yendor":
                revival = "comes back on his own; not from this essence"
            elif mon["mlet"] == "S_LICH":
                revival = "no corpse revival in stock; turning can raise this essence"
            else:
                revival = "no automatic revival"
            ess_ws.append(
                [
                    mon["name"],
                    aliases,
                    mon["mlet"],
                    f"{possessive(mon['name'])} essence",
                    eaten,
                    listed,
                    20,
                    "never",
                    "no",
                    "yes",
                    "no",
                    "yes" if fatal else "no",
                    "yes" if flagged else "no",
                    effects,
                    revival,
                    reason + " Not counted as a living race. Cannibalism is gone.",
                ]
            )
            ess_n += 1
        elif sheet == "humanoid":
            human_ws.append(
                [
                    mon["name"],
                    aliases,
                    mon["mlet"],
                    reason,
                    "yes" if mon["m1"] & flags["M1_HUMANOID"] else "no",
                    "yes" if mon["m2"] & flags["M2_HUMAN"] else "no",
                    "yes" if mon["m2"] & flags["M2_ELF"] else "no",
                ]
            )
            human_n += 1
        else:
            other_ws.append([mon["name"], mon["mlet"], reason])
            other_n += 1

    style_sheet(meat_ws, meat_headers, meat_n)
    style_sheet(human_ws, human_headers, human_n)
    style_sheet(other_ws, other_headers, other_n)
    style_sheet(ess_ws, ess_headers, ess_n)
    style_sheet(stable_ws, stable_headers, stable_n)

    tin = wb.create_sheet("Tins and eggs")
    tin_headers = ["Item", "Player-facing name", "Notes"]
    for col, header in enumerate(tin_headers, 1):
        tin.cell(1, col, header)
    tin_rows = [
        (
            "tin",
            "tin of <monster> meat",
            "Meat monsters. The word corpse is not used. "
            "Example: tin of gray dragon meat.",
        ),
        (
            "tin",
            "tin of <monster>",
            "Vegetable monsters (fungi and the like) omit the word meat.",
        ),
        (
            "tin",
            "<preparation> tin of <monster> meat",
            "Known variety, such as deep fried or homemade, is placed around tin.",
        ),
        (
            "tin",
            "tin of spinach",
            "Spinach tin. Not a monster. Still works.",
        ),
        (
            "tin",
            "empty tin",
            "No contents.",
        ),
        (
            "egg",
            "egg",
            "Unidentified or generic egg. The word corpse is not used.",
        ),
        (
            "egg",
            "<monster> egg",
            "Once the egg type is known. Example: cockatrice egg. Can still hatch.",
        ),
        (
            "corpse object",
            "raw <monster> <noun> or cooked <monster> <noun>",
            "The object type is still CORPSE internally. "
            + EXEMPT_NOTE
            + " Cooked meat keeps the same intrinsics, poison, acid, and petrification as raw. "
            "The flint and tinder box is not used up.",
        ),
    ]
    for row in tin_rows:
        tin.append(list(row))
    style_sheet(tin, tin_headers, 1 + len(tin_rows))

    notes = wb.create_sheet("Notes")
    notes_headers = ["Topic", "Rule"]
    for col, header in enumerate(notes_headers, 1):
        notes.cell(1, col, header)
    note_rows = [
        ("Noun table", "include/meatnoun.h, one noun per monster class. Default is meat."),
        ("Dragon", "raw gray dragon steak, cooked gray dragon steak."),
        ("Sea monsters", EXEMPT_NOTE),
        ("Lizard", "The creature named lizard only, not newt, gecko, iguana, crocodile, or salamander. Lizard meat never rots."),
        ("Jellies", "Blue jelly, spotted jelly, and ochre jelly never rot, skip the 5% roll, and can be blessed raw. Jellyfish is a sea fillet, not a jelly. The noun is not repeated: raw blue jelly."),
        ("Humanoids", "M1_HUMANOID, or the human or elf race, leave no meat, except the named essences, the ape class, lycanthropes, Cyclops, and trolls."),
        ("Apes", "Monkey, ape, owlbear, yeti, carnivorous ape, and sasquatch leave meat. Monkey is meat. The large ones are steak."),
        ("Lycanthropes", "Wererat, werejackal, and werewolf leave meat in animal form and in human form. Eating either form causes lycanthropy. A wish for the shared name still produces the human form."),
        ("Cyclops", "Named raw Cyclops steak and cooked Cyclops steak. No \"the\" and no possessive. Other giants leave nothing."),
        ("Liches", "Lich, demilich, master lich, and arch-lich crumble into dust and leave nothing. Eating them grants nothing."),
        ("Orcs", "Only the goblin leaves an essence. Hobgoblin, orc, hill orc, Mordor orc, Uruk-hai, orc shaman, and orc-captain still leave nothing. Orc zombies and orc mummies already leave an essence."),
        ("Essences", "Named \"<monster>'s essence\" (wraith's essence, Death's essence). Wishes may also say \"essence of <monster>\". Never spoil, cannot be cooked, can be blessed. No 5% raw-poison roll."),
        ("Demons", "Every M2_DEMON leaves an essence, and eating it is instantly fatal. The death reason is \"ate a demon's essence\". S_DEMON monsters that are not M2_DEMON (sandestin, djinni, mail daemon) leave an essence and are not fatal. The Riders are S_DEMON, not M2_DEMON, and keep their own fatal meal, and they still come back. Imps, manes, lemures, quasits, and tengu are not demons."),
        ("Riders", "Death, Pestilence, and Famine leave essence. They still revive from it. Eating it is instantly fatal, as with the old Rider corpse."),
        ("Trolls", "Trolls still leave meat (raw troll steak), not essence. Revival uses that meat."),
        ("Medusa", "Leaves Medusa's essence. Eating it still petrifies. Killing her is still recorded. The level does not need her body."),
        ("Wizard of Yendor", "Leaves an essence. He still comes back, and still drops what he was carrying. The essence is not what brings him back."),
        ("Croesus", "Leaves an essence. His gold and other carried items still drop."),
        ("Charon", "Would leave an essence. This build does not compile Charon (CHARON is not defined)."),
        ("Quest", "Quest leaders who are ordinary humanoids still leave nothing. Demon nemeses leave a fatal essence. Cyclops leaves a steak."),
        ("Vampires, zombies, mummies", "They leave their own essence, not an old human/elf/orc body. Eating it uses that undead's own flags (poisonous if the undead is). It is not cannibalism and it is not tainted. Undead turning revives them from the essence. Ghoul and skeleton are included because they are zombie-class. Vlad leaves essence instead of crumbling."),
        ("Ghost nutrition", "Ghost, shade, wraith, barrow wight, and Nazgul are listed with 0 nutrition. Eating their essence gives 50, a small meal. A wraith's essence still grants a level."),
        ("Cannibalism", "Removed. No Luck penalty, no aggravate, no 'You cannibal!' message, no cave dweller or orc exception, and pets do not refuse same-race food."),
        ("Bones", "Your own body is still saved with a bones file, under your name. Sleeping-gas traps no longer place human, elf, dwarf, gnome, or orc meat."),
        ("Bless", "Raw meat cannot be blessed, except lizard meat, jellies, and essences. A raw fillet gets: You can't bless <food>! Cooked meat, including cooked fillets, can be blessed. Blessed cooked meat never rots and never causes tainted or fatal food poisoning."),
        ("Raw poison", "Eating raw meat rolls 1 in 20 for fatal food poisoning, in addition to the old tainted-meat rules. Same cure as other food poisoning."),
        ("Cook", "#cook needs a flint and tinder box. Every role starts with one. The box is not used up. Cooking takes a few turns. Cooked meat rots about ten times slower."),
        ("Tins and eggs", "Still work. Their names do not say corpse. See the Tins and eggs sheet."),
        ("Regenerate", "python3 tools/gen_food_xlsx.py"),
    ]
    for row in note_rows:
        notes.append(list(row))
    style_sheet(notes, notes_headers, 1 + len(note_rows))
    notes.column_dimensions["A"].width = 32
    notes.column_dimensions["B"].width = 110

    OUT.parent.mkdir(parents=True, exist_ok=True)
    wb.save(OUT)
    print(f"wrote {OUT}")
    print(
        f"foods {len(foods)} meat {meat_n - 1} essence {ess_n - 1} "
        f"stable {stable_n - 1} humanoid {human_n - 1} other {other_n - 1}"
    )
    demons = [m["name"] for m in monsters if demon_flagged(m, flags)]
    nonfatal = [
        m["name"]
        for m in monsters
        if m["mlet"] == "S_DEMON" and not demon_flagged(m, flags)
    ]
    print("M2_DEMON:", ", ".join(demons))
    print("S_DEMON not M2_DEMON:", ", ".join(nonfatal))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
NetHack Symbol Set Editor - Textual TUI Application
Edit and preview symbol sets for HanNetHack
"""

import os
import re
import json
import unicodedata
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional


def get_display_width(char: str) -> int:
    """Get display width of a character (CJK = 2, others = 1)"""
    if len(char) == 0:
        return 0
    # Use East Asian Width property
    ea_width = unicodedata.east_asian_width(char[0])
    # W (Wide), F (Fullwidth) = 2 columns
    # Na (Narrow), H (Halfwidth), N (Neutral), A (Ambiguous) = 1 column
    # Note: 'A' (Ambiguous) can be 2 in some terminals, but we use 1 for safety
    if ea_width in ('W', 'F'):
        return 2
    return 1


def get_string_width(s: str) -> int:
    """Get total display width of a string"""
    return sum(get_display_width(c) for c in s)


def pad_to_width(s: str, target_width: int) -> str:
    """Pad string to target display width"""
    current_width = get_string_width(s)
    if current_width >= target_width:
        return s
    return s + ' ' * (target_width - current_width)

from textual.app import App, ComposeResult
from textual.containers import Container, Horizontal, Vertical, ScrollableContainer
from textual.widgets import (
    Header, Footer, Static, Button, Select, Input,
    DataTable, Label, TabbedContent, TabPane
)
from textual.binding import Binding
from textual.reactive import reactive


# Symbol categories and their members
SYMBOL_CATEGORIES = {
    "dungeon": {
        "name": "던전 구조 (Dungeon)",
        "symbols": [
            ("S_vwall", "│", "수직 벽"),
            ("S_hwall", "─", "수평 벽"),
            ("S_tlcorn", "┌", "좌상 모서리"),
            ("S_trcorn", "┐", "우상 모서리"),
            ("S_blcorn", "└", "좌하 모서리"),
            ("S_brcorn", "┘", "우하 모서리"),
            ("S_crwall", "┼", "십자 벽"),
            ("S_tuwall", "┴", "T자 위"),
            ("S_tdwall", "┬", "T자 아래"),
            ("S_tlwall", "┤", "T자 왼쪽"),
            ("S_trwall", "├", "T자 오른쪽"),
            ("S_room", "·", "방 바닥"),
            ("S_darkroom", " ", "어두운 방"),
            ("S_corr", "░", "복도"),
            ("S_litcorr", "▒", "밝은 복도"),
            ("S_upstair", "<", "위층 계단"),
            ("S_dnstair", ">", "아래층 계단"),
            ("S_upladder", "<", "위층 사다리"),
            ("S_dnladder", ">", "아래층 사다리"),
        ]
    },
    "doors": {
        "name": "문/입구 (Doors)",
        "symbols": [
            ("S_vodoor", "|", "열린 문 (세로)"),
            ("S_hodoor", "-", "열린 문 (가로)"),
            ("S_vcdoor", "+", "닫힌 문 (세로)"),
            ("S_hcdoor", "+", "닫힌 문 (가로)"),
            ("S_ndoor", "·", "문 없음"),
            ("S_bars", "#", "철창"),
        ]
    },
    "features": {
        "name": "지형 (Features)",
        "symbols": [
            ("S_tree", "#", "나무"),
            ("S_altar", "_", "제단"),
            ("S_grave", "|", "무덤"),
            ("S_throne", "\\", "왕좌"),
            ("S_sink", "#", "싱크대"),
            ("S_fountain", "{", "분수"),
            ("S_pool", "}", "물웅덩이"),
            ("S_ice", ".", "얼음"),
            ("S_lava", "}", "용암"),
            ("S_water", "}", "물"),
            ("S_cloud", "#", "구름"),
            ("S_air", " ", "공기"),
        ]
    },
    "objects": {
        "name": "아이템 (Objects)",
        "symbols": [
            ("S_weapon", ")", "무기"),
            ("S_armor", "[", "갑옷"),
            ("S_ring", "=", "반지"),
            ("S_amulet", '"', "아뮬렛"),
            ("S_potion", "!", "물약"),
            ("S_scroll", "?", "두루마리"),
            ("S_book", "+", "책"),
            ("S_wand", "/", "지팡이"),
            ("S_coin", "$", "금화"),
            ("S_gem", "*", "보석"),
            ("S_rock", "`", "돌"),
            ("S_ball", "0", "쇠구슬"),
            ("S_chain", "_", "사슬"),
            ("S_food", "%", "음식"),
            ("S_tool", "(", "도구"),
        ]
    },
    "monsters": {
        "name": "몬스터 (Monsters)",
        "symbols": [
            ("S_human", "@", "인간"),
            ("S_ghost", " ", "유령"),
            ("S_demon", "&", "악마"),
            ("S_angel", "A", "천사"),
            ("S_dragon", "D", "용"),
            ("S_giant", "H", "거인"),
            ("S_snake", "S", "뱀"),
            ("S_rodent", "r", "설치류"),
            ("S_spider", "s", "거미"),
            ("S_dog", "d", "개"),
            ("S_feline", "f", "고양이"),
            ("S_horse", "u", "말"),
        ]
    },
    "traps": {
        "name": "함정 (Traps)",
        "symbols": [
            ("S_arrow_trap", "^", "화살 함정"),
            ("S_dart_trap", "^", "다트 함정"),
            ("S_falling_rock_trap", "^", "낙석 함정"),
            ("S_land_mine", "^", "지뢰"),
            ("S_fire_trap", "^", "불 함정"),
            ("S_pit", "^", "구덩이"),
            ("S_hole", "^", "구멍"),
            ("S_teleportation_trap", "^", "텔레포트 함정"),
            ("S_magic_portal", "^", "마법 포탈"),
            ("S_web", "^", "거미줄"),
        ]
    },
}


@dataclass
class SymbolSet:
    """Represents a symbol set configuration"""
    name: str
    description: str = ""
    handling: str = "UTF8"
    symbols: Dict[str, str] = field(default_factory=dict)

    def get_symbol(self, key: str, default: str = "?") -> str:
        return self.symbols.get(key, default)

    def set_symbol(self, key: str, value: str):
        self.symbols[key] = value


def parse_symbols_file(filepath: str) -> Dict[str, SymbolSet]:
    """Parse dat/symbols file and return dict of symbol sets"""
    symsets = {}
    current_set = None

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue

                if line.startswith('start:'):
                    name = line[6:].strip()
                    current_set = SymbolSet(name=name)
                    symsets[name] = current_set
                elif line.startswith('finish'):
                    current_set = None
                elif current_set and ':' in line:
                    # Parse symbol definition
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        key = parts[0].strip()
                        value_part = parts[1].strip()

                        # Handle Description, Handling, etc.
                        if key == "Description":
                            current_set.description = value_part
                        elif key == "Handling":
                            current_set.handling = value_part
                        elif key.startswith("S_") or key.startswith("G_"):
                            # Parse symbol value
                            symbol = parse_symbol_value(value_part)
                            if symbol:
                                current_set.symbols[key] = symbol
    except FileNotFoundError:
        pass

    return symsets


def parse_symbol_value(value: str) -> Optional[str]:
    """Parse a symbol value from the symbols file format"""
    # Remove comments
    if '#' in value:
        value = value.split('#')[0].strip()

    # Handle color suffix
    if '/' in value:
        value = value.split('/')[0].strip()

    # Handle Unicode format U+XXXX
    if value.startswith('U+'):
        try:
            codepoint = int(value[2:], 16)
            return chr(codepoint)
        except ValueError:
            return None

    # Handle hex format \xNN
    if value.startswith('\\x'):
        try:
            byte_val = int(value[2:4], 16)
            return chr(byte_val)
        except ValueError:
            return None

    # Handle decimal format \0NN
    if value.startswith('\\0'):
        try:
            dec_val = int(value[2:])
            return chr(dec_val)
        except ValueError:
            return None

    # Handle quoted character
    if value.startswith("'") and value.endswith("'"):
        return value[1:-1]

    # Plain character
    if len(value) == 1:
        return value

    return None


def format_symbol_value(char: str) -> str:
    """Format a character as a symbol file value"""
    codepoint = ord(char)
    if codepoint > 127:
        return f"U+{codepoint:04X}"
    elif char in "'\\#":
        return f"\\x{codepoint:02x}"
    else:
        return f"'{char}'"


def generate_symset_output(symset: SymbolSet) -> str:
    """Generate symbols file format output for a symbol set"""
    lines = [f"start: {symset.name}"]

    if symset.description:
        lines.append(f"\tDescription: {symset.description}")

    lines.append(f"\tHandling: {symset.handling}")

    # Sort symbols by category
    for cat_key, cat_info in SYMBOL_CATEGORIES.items():
        cat_symbols = [(s[0], s[2]) for s in cat_info["symbols"]]
        for sym_key, sym_desc in cat_symbols:
            if sym_key in symset.symbols:
                value = format_symbol_value(symset.symbols[sym_key])
                lines.append(f"\t{sym_key}: {value:<24} # {sym_desc}")

    lines.append("finish")
    return "\n".join(lines)


class DungeonPreview(Static):
    """Widget showing a preview of the dungeon with current symbols"""

    DEFAULT_CSS = """
    DungeonPreview {
        height: 12;
        border: solid green;
        padding: 1;
        background: $surface;
    }
    """

    def __init__(self, symset: Optional[SymbolSet] = None):
        super().__init__()
        self.symset = symset or SymbolSet(name="default")

    def get_sym(self, key: str, default: str) -> str:
        return self.symset.get_symbol(key, default)

    def render_preview(self) -> str:
        """Render preview with proper character width alignment using grid"""
        s = self.get_sym

        # Define the dungeon layout as a grid
        # Each row is a list of symbol keys with defaults
        layout = [
            # Row 0: top wall
            [('S_tlcorn', '┌'), ('S_hwall', '─'), ('S_hwall', '─'), ('S_hwall', '─'),
             ('S_hwall', '─'), ('S_hwall', '─'), ('S_tdwall', '┬'), ('S_hwall', '─'),
             ('S_hwall', '─'), ('S_hwall', '─'), ('S_trcorn', '┐')],
            # Row 1: room with food and altar
            [('S_vwall', '│'), ('S_room', '·'), ('S_room', '·'), ('S_room', '·'),
             ('S_food', '%'), ('S_room', '·'), ('S_vwall', '│'), ('S_room', '·'),
             ('S_altar', '_'), ('S_room', '·'), ('S_vwall', '│')],
            # Row 2: player, weapon, door
            [('S_vwall', '│'), ('S_room', '·'), ('S_human', '@'), ('S_room', '·'),
             ('S_weapon', ')'), ('S_room', '·'), ('S_vodoor', '|'), ('S_room', '·'),
             ('S_room', '·'), ('S_room', '·'), ('S_vwall', '│')],
            # Row 3: pet, gold, fountain
            [('S_vwall', '│'), ('S_room', '·'), ('S_dog', 'd'), ('S_room', '·'),
             ('S_coin', '$'), ('S_room', '·'), ('S_vwall', '│'), ('S_room', '·'),
             ('S_fountain', '{'), ('S_room', '·'), ('S_vwall', '│')],
            # Row 4: middle wall with door
            [('S_trwall', '├'), ('S_hwall', '─'), ('S_hwall', '─'), ('S_hodoor', '-'),
             ('S_hwall', '─'), ('S_hwall', '─'), ('S_tuwall', '┴'), ('S_hwall', '─'),
             ('S_hwall', '─'), ('S_hwall', '─'), ('S_brcorn', '┘')],
            # Row 5: corridor with stairs
            [('S_vwall', '│'), ('S_corr', '░'), ('S_corr', '░'), ('S_room', '·'),
             ('S_upstair', '<'), ('S_room', '·'), ('S_dnstair', '>')],
            # Row 6: corridor with dragon, potion, gem
            [('S_vwall', '│'), ('S_corr', '░'), ('S_dragon', 'D'), ('S_room', '·'),
             ('S_room', '·'), ('S_potion', '!'), ('S_room', '·'), ('S_gem', '*')],
            # Row 7: bottom wall
            [('S_blcorn', '└'), ('S_hwall', '─'), ('S_hwall', '─'), ('S_hwall', '─'),
             ('S_hwall', '─'), ('S_hwall', '─'), ('S_hwall', '─'), ('S_hwall', '─')],
        ]

        # Build each row with proper alignment
        lines = []
        for row in layout:
            line = "  "
            for key, default in row:
                char = s(key, default)
                width = get_display_width(char)
                line += char
                # Add padding only if char is 1-width (CJK chars are 2-width)
                if width == 1:
                    line += " "  # Pad narrow chars to make each cell 2 columns
            lines.append(line)

        return "\n".join(lines)

    def update_symset(self, symset: SymbolSet):
        self.symset = symset
        self.update(self.render_preview())

    def on_mount(self):
        self.update(self.render_preview())


class SymbolEditor(App):
    """Main Symbol Editor Application"""

    CSS = """
    Screen {
        layout: grid;
        grid-size: 2 2;
        grid-columns: 1fr 2fr;
        grid-rows: auto 1fr;
    }

    #preview-container {
        row-span: 1;
        column-span: 2;
        height: auto;
        padding: 1;
    }

    #sidebar {
        height: 100%;
        padding: 1;
        border: solid blue;
    }

    #main {
        height: 100%;
        padding: 1;
    }

    #category-select {
        width: 100%;
        margin-bottom: 1;
    }

    #preset-select {
        width: 100%;
        margin-bottom: 1;
    }

    .section-title {
        text-style: bold;
        margin-bottom: 1;
    }

    DataTable {
        height: 100%;
    }

    #button-bar {
        dock: bottom;
        height: 3;
        padding: 1;
    }

    Button {
        margin-right: 1;
    }
    """

    BINDINGS = [
        Binding("q", "quit", "종료"),
        Binding("s", "save", "저장"),
        Binding("r", "reset", "초기화"),
        Binding("tab", "next_category", "다음 카테고리"),
    ]

    current_category = reactive("dungeon")

    def __init__(self, symbols_path: str = None):
        super().__init__()

        # Find symbols file
        if symbols_path is None:
            # Try to find dat/symbols relative to this script
            script_dir = Path(__file__).parent
            symbols_path = script_dir.parent.parent / "dat" / "symbols"

        self.symbols_path = Path(symbols_path)
        self.symsets = parse_symbols_file(str(self.symbols_path))

        # Create default working set
        self.current_symset = SymbolSet(
            name="Custom",
            description="Custom symbol set",
            handling="UTF8"
        )

        # Load Korean preset if available
        if "Korean" in self.symsets:
            self.current_symset.symbols = dict(self.symsets["Korean"].symbols)

        self.preview = DungeonPreview(self.current_symset)

    def compose(self) -> ComposeResult:
        yield Header()

        with Container(id="preview-container"):
            yield Label("미리보기 (Preview)", classes="section-title")
            yield self.preview

        with Container(id="sidebar"):
            yield Label("프리셋 (Preset)", classes="section-title")
            preset_options = [(name, name) for name in ["Default", "Korean", "Emoji"] + list(self.symsets.keys())]
            yield Select(preset_options, id="preset-select", value="Korean")

            yield Label("카테고리 (Category)", classes="section-title")
            cat_options = [(info["name"], key) for key, info in SYMBOL_CATEGORIES.items()]
            yield Select(cat_options, id="category-select", value="dungeon")

        with Container(id="main"):
            yield Label("심볼 편집 (Symbol Edit)", classes="section-title")
            table = DataTable(id="symbol-table")
            table.add_columns("키", "기본값", "현재값", "설명")
            yield table

        with Horizontal(id="button-bar"):
            yield Button("저장 (S)", id="save-btn", variant="primary")
            yield Button("내보내기", id="export-btn", variant="default")
            yield Button("초기화 (R)", id="reset-btn", variant="warning")

        yield Footer()

    def on_mount(self):
        self.update_symbol_table()

    def update_symbol_table(self):
        """Update the symbol table for current category"""
        table = self.query_one("#symbol-table", DataTable)
        table.clear()

        cat_info = SYMBOL_CATEGORIES.get(self.current_category, {})
        symbols = cat_info.get("symbols", [])

        for key, default, desc in symbols:
            current = self.current_symset.get_symbol(key, default)
            table.add_row(key, default, current, desc, key=key)

    def on_select_changed(self, event: Select.Changed):
        if event.select.id == "category-select":
            self.current_category = event.value
            self.update_symbol_table()
        elif event.select.id == "preset-select":
            self.load_preset(event.value)

    def load_preset(self, preset_name: str):
        """Load a preset symbol set"""
        if preset_name == "Default":
            self.current_symset.symbols.clear()
        elif preset_name in self.symsets:
            self.current_symset.symbols = dict(self.symsets[preset_name].symbols)

        self.update_symbol_table()
        self.preview.update_symset(self.current_symset)

    def on_data_table_row_selected(self, event: DataTable.RowSelected):
        """Handle row selection - open edit dialog"""
        key = event.row_key.value
        if key:
            self.edit_symbol(key)

    def edit_symbol(self, key: str):
        """Open input for editing a symbol"""
        # Find symbol info
        for cat_info in SYMBOL_CATEGORIES.values():
            for sym_key, default, desc in cat_info["symbols"]:
                if sym_key == key:
                    current = self.current_symset.get_symbol(key, default)
                    # For now, just cycle through some options
                    # In a full implementation, we'd show an input dialog
                    self.notify(f"{key}: 현재 '{current}' - 편집 기능 구현 예정")
                    return

    def action_save(self):
        """Save current symbol set"""
        output = generate_symset_output(self.current_symset)

        # Save to a new file
        output_path = self.symbols_path.parent / "symbols.custom"
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(output)

        self.notify(f"저장됨: {output_path}")

    def action_reset(self):
        """Reset to default symbols"""
        self.current_symset.symbols.clear()
        self.update_symbol_table()
        self.preview.update_symset(self.current_symset)
        self.notify("초기화 완료")

    def action_next_category(self):
        """Cycle to next category"""
        cats = list(SYMBOL_CATEGORIES.keys())
        idx = cats.index(self.current_category)
        self.current_category = cats[(idx + 1) % len(cats)]

        select = self.query_one("#category-select", Select)
        select.value = self.current_category
        self.update_symbol_table()


def main():
    app = SymbolEditor()
    app.run()


if __name__ == "__main__":
    main()

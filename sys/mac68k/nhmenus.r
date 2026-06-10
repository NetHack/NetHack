/* nhmenus.r -- menu resources (MENU, STR#, MNU#) for the
 * Mac 68k port.  Transcribed from the historic NHrsrc.hqx by
 * tools/rsrc_to_rez.py; see macmenu.c for the MNU# format and
 * the STR# keystroke-dispatch convention ('\0xA5' = comment/
 * separator marker, key 0x1B + mark = hierarchical submenu).
 *
 * INVARIANT for hand edits: command menus dispatch through a
 * companion STR# whose entries correspond 1-for-1, BY POSITION,
 * to the MENU's items (separators included -- their STR# entry
 * is the '\0xA5'-prefixed placeholder).  Adding/removing/moving
 * a MENU item without the matching STR# edit silently shifts
 * every later item's command.  NOTE the pairing uses the RUNTIME
 * menu ID (forced by MNU# list position = firstMenuID + index),
 * not the MENU resource ID in this file.
 */

#include "Multiverse.r"

resource 'MENU' (128) {
    128,
    textMenuProc,
    0x7FFFFFFD,
    enabled,
    apple,
    {
        "About NetHack\0xC9", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (129) {
    129,
    textMenuProc,
    0x7FFFFEA7,
    enabled,
    "File",
    {
        "Redraw Map", noIcon, "R", "\0x11", plain,
        "Previous Msg", noIcon, "P", "\0x11", plain,
        "Reposition", noIcon, "N", noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Regular", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xC8", plain,
        "Enter Explore", noIcon, noKey, "X", plain,
        "-", noIcon, noKey, noMark, plain,
        "Save", noIcon, "S", "S", plain,
        "-", noIcon, noKey, noMark, plain,
        "Quit", noIcon, "Q", "Q", plain
    }
};

resource 'MENU' (130) {
    130,
    textMenuProc,
    0x7FFFFFFD,
    enabled,
    "Edit",
    {
        "Undo", noIcon, "Z", noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Cut", noIcon, "X", noMark, plain,
        "Copy", noIcon, "C", noMark, plain,
        "Paste", noIcon, "V", noMark, plain,
        "Clear", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (131) {
    131,
    textMenuProc,
    0x7FFFFEFF,
    enabled,
    "Kbd",
    {
        "Control Keys", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xC9", plain,
        "Punctuation", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCA", plain,
        "Brackets", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCB", plain,
        "a - m", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCC", plain,
        "n - z", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCD", plain,
        "A - M", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCE", plain,
        "N - Z", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCF", plain,
        "0 - 9", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xD0", plain,
        "-", noIcon, noKey, noMark, plain,
        "(escape)", noIcon, noKey, noMark, plain,
        "(space)", noIcon, noKey, noMark, plain,
        "(delete)", noIcon, noKey, noMark, plain,
        "(return)", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (132) {
    132,
    textMenuProc,
    0x7FFFFFED,
    enabled,
    "Help",
    {
        "Options", noIcon, noKey, "O", plain,
        "-", noIcon, noKey, noMark, plain,
        "Help", noIcon, noKey, "?", plain,
        "Describe Key", noIcon, noKey, "&", plain,
        "-", noIcon, noKey, noMark, plain,
        "Version", noIcon, noKey, "v", plain,
        "History", noIcon, noKey, "V", plain,
        "Version Features", noIcon, noKey, "#", plain
    }
};

resource 'MENU' (133) {
    133,
    textMenuProc,
    0x7FFFFEF7,
    enabled,
    "Info",
    {
        "Inventory All", noIcon, noKey, "i", plain,
        "Inventory Select", noIcon, noKey, "I", plain,
        "Adjust", noIcon, noKey, "#", plain,
        "-", noIcon, noKey, noMark, plain,
        "Look Down", noIcon, noKey, ":", plain,
        "Describe One", noIcon, noKey, ";", plain,
        "Describe Many", noIcon, noKey, "/", plain,
        "Describe Trap", noIcon, noKey, "^", plain,
        "-", noIcon, noKey, noMark, plain,
        "Call Monster", noIcon, noKey, "C", plain,
        "Name Object", noIcon, noKey, "#", plain,
        "Discoveries", noIcon, noKey, "\0x5C", plain
    }
};

resource 'MENU' (134) {
    134,
    textMenuProc,
    0x7FFFBBFD,
    enabled,
    "Equip",
    {
        "Current", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xD1", plain,
        "-", noIcon, noKey, noMark, plain,
        "Wield Weapon", noIcon, noKey, "w", plain,
        "Exchange Weapon", noIcon, noKey, "x", plain,
        "Select Quiver", noIcon, noKey, "Q", plain,
        "Fire Quiver", noIcon, noKey, "f", plain,
        "Throw", noIcon, noKey, "t", plain,
        "Apply", noIcon, noKey, "a", plain,
        "Enhance", noIcon, noKey, "#", plain,
        "Two Weapon Combat", noIcon, noKey, "#", plain,
        "-", noIcon, noKey, noMark, plain,
        "Wear Armor", noIcon, noKey, "W", plain,
        "Take Off", noIcon, noKey, "T", plain,
        "Ask Remove", noIcon, noKey, "A", plain,
        "-", noIcon, noKey, noMark, plain,
        "Put On", noIcon, noKey, "P", plain,
        "Remove", noIcon, noKey, "R", plain
    }
};

resource 'MENU' (135) {
    135,
    textMenuProc,
    0x7FFFFFBD,
    enabled,
    "Act",
    {
        "Wait", noIcon, noKey, ".", plain,
        "-", noIcon, noKey, noMark, plain,
        "Teleport", noIcon, "T", "\0x11", plain,
        "Jump", noIcon, noKey, "#", plain,
        "Monster", noIcon, noKey, "#", plain,
        "Wipe", noIcon, noKey, "#", plain,
        "-", noIcon, noKey, noMark, plain,
        "Apply", noIcon, noKey, "a", plain,
        "Pay", noIcon, noKey, "p", plain,
        "Kick", noIcon, "D", "\0x11", plain,
        "Chat", noIcon, noKey, "#", plain,
        "Offer", noIcon, noKey, "#", plain,
        "Pray", noIcon, noKey, "#", plain,
        "Ride", noIcon, noKey, "#", plain,
        "Turn", noIcon, noKey, "#", plain
    }
};

resource 'MENU' (136) {
    136,
    textMenuProc,
    0x7FFFFDEF,
    enabled,
    "Magic",
    {
        "Drop Item", noIcon, noKey, "d", plain,
        "Drop Select", noIcon, noKey, "D", plain,
        "Pickup", noIcon, noKey, ",", plain,
        "Toggle pickup", noIcon, noKey, "@", plain,
        "-", noIcon, noKey, noMark, plain,
        "Eat", noIcon, noKey, "e", plain,
        "Read", noIcon, noKey, "r", plain,
        "Quaff", noIcon, noKey, "q", plain,
        "Dip", noIcon, noKey, "#", plain,
        "-", noIcon, noKey, noMark, plain,
        "List Spells", noIcon, noKey, "+", plain,
        "Cast Spell", noIcon, noKey, "Z", plain,
        "Zap", noIcon, noKey, "z", plain,
        "Invoke", noIcon, noKey, "#", plain,
        "Rub", noIcon, noKey, "#", plain
    }
};

resource 'MENU' (137) {
    137,
    textMenuProc,
    0x7FFFDDDF,
    enabled,
    "Bits",
    {
        "Search", noIcon, noKey, "s", plain,
        "Close Door", noIcon, noKey, "c", plain,
        "Open Door", noIcon, noKey, "o", plain,
        "Apply", noIcon, noKey, "a", plain,
        "Kick", noIcon, "D", "\0x11", plain,
        "-", noIcon, noKey, noMark, plain,
        "Untrap", noIcon, noKey, "#", plain,
        "Force", noIcon, noKey, "#", plain,
        "Loot", noIcon, noKey, "#", plain,
        "-", noIcon, noKey, noMark, plain,
        "Engrave", noIcon, noKey, "E", plain,
        "Dip", noIcon, noKey, "#", plain,
        "Sit", noIcon, noKey, "#", plain,
        "-", noIcon, noKey, "#", 2,
        "Climp Up", noIcon, noKey, "<", plain,
        "Climb Down", noIcon, noKey, ">", plain
    }
};

resource 'MENU' (200) {
    200,
    textMenuProc,
    allEnabled,
    enabled,
    "wizard",
    {
        "Attributes", noIcon, noKey, "x", plain,
        "Detect Unseen", noIcon, noKey, "e", plain,
        "Floor Map", noIcon, noKey, "f", plain,
        "Generate Monster", noIcon, noKey, "g", plain,
        "Identify", noIcon, noKey, "i", plain,
        "Locations", noIcon, noKey, "o", plain,
        "Level Teleport", noIcon, noKey, "v", plain,
        "Wish", noIcon, noKey, "w", plain
    }
};

resource 'MENU' (201) {
    201,
    textMenuProc,
    0x7FFFFEFF,
    enabled,
    "control keys",
    {
        "b", noIcon, "1", noMark, plain,
        "j", noIcon, "2", noMark, plain,
        "n", noIcon, "3", noMark, plain,
        "h", noIcon, "4", noMark, plain,
        "l", noIcon, "6", noMark, plain,
        "y", noIcon, "7", noMark, plain,
        "k", noIcon, "8", noMark, plain,
        "u", noIcon, "9", noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "d", noIcon, "D", noMark, plain,
        "p", noIcon, "P", noMark, plain,
        "r", noIcon, "R", noMark, plain,
        "t", noIcon, "T", noMark, plain
    }
};

resource 'MENU' (202) {
    202,
    textMenuProc,
    allEnabled,
    enabled,
    "punctuation",
    {
        " .", noIcon, noKey, noMark, plain,
        " ,", noIcon, noKey, noMark, plain,
        " ;", noIcon, noKey, noMark, plain,
        " :", noIcon, noKey, noMark, plain,
        " !", noIcon, noKey, noMark, plain,
        " ?", noIcon, noKey, noMark, plain,
        " +", noIcon, noKey, noMark, plain,
        " -", noIcon, noKey, noMark, plain,
        " =", noIcon, noKey, noMark, plain,
        " #", noIcon, noKey, noMark, plain,
        " $", noIcon, noKey, noMark, plain,
        " @", noIcon, noKey, noMark, plain,
        " &", noIcon, noKey, noMark, plain,
        " *", noIcon, noKey, noMark, plain,
        " ~", noIcon, noKey, noMark, plain,
        " _", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (203) {
    203,
    textMenuProc,
    allEnabled,
    enabled,
    "brackets",
    {
        "[", noIcon, noKey, noMark, plain,
        "]", noIcon, noKey, noMark, plain,
        "(", noIcon, noKey, noMark, plain,
        ")", noIcon, noKey, noMark, plain,
        "{", noIcon, noKey, noMark, plain,
        "}", noIcon, noKey, noMark, plain,
        "<", noIcon, noKey, noMark, plain,
        ">", noIcon, noKey, noMark, plain,
        "^", noIcon, noKey, noMark, plain,
        "`", noIcon, noKey, noMark, plain,
        "'", noIcon, noKey, noMark, plain,
        "\"", noIcon, noKey, noMark, plain,
        "\0x5C", noIcon, noKey, noMark, plain,
        "/", noIcon, noKey, noMark, plain,
        "|", noIcon, noKey, noMark, plain,
        "%", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (204) {
    204,
    textMenuProc,
    allEnabled,
    enabled,
    "a - m",
    {
        "a", noIcon, noKey, noMark, plain,
        "b", noIcon, noKey, noMark, plain,
        "c", noIcon, noKey, noMark, plain,
        "d", noIcon, noKey, noMark, plain,
        "e", noIcon, noKey, noMark, plain,
        "f", noIcon, noKey, noMark, plain,
        "g", noIcon, noKey, noMark, plain,
        "h", noIcon, noKey, noMark, plain,
        "i", noIcon, noKey, noMark, plain,
        "j", noIcon, noKey, noMark, plain,
        "k", noIcon, noKey, noMark, plain,
        "l", noIcon, noKey, noMark, plain,
        "m", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (205) {
    205,
    textMenuProc,
    allEnabled,
    enabled,
    "n - z",
    {
        "n", noIcon, noKey, noMark, plain,
        "o", noIcon, noKey, noMark, plain,
        "p", noIcon, noKey, noMark, plain,
        "q", noIcon, noKey, noMark, plain,
        "r", noIcon, noKey, noMark, plain,
        "s", noIcon, noKey, noMark, plain,
        "t", noIcon, noKey, noMark, plain,
        "u", noIcon, noKey, noMark, plain,
        "v", noIcon, noKey, noMark, plain,
        "w", noIcon, noKey, noMark, plain,
        "x", noIcon, noKey, noMark, plain,
        "y", noIcon, noKey, noMark, plain,
        "z", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (206) {
    206,
    textMenuProc,
    allEnabled,
    enabled,
    "A - M",
    {
        "A", noIcon, noKey, noMark, plain,
        "B", noIcon, noKey, noMark, plain,
        "C", noIcon, noKey, noMark, plain,
        "D", noIcon, noKey, noMark, plain,
        "E", noIcon, noKey, noMark, plain,
        "F", noIcon, noKey, noMark, plain,
        "G", noIcon, noKey, noMark, plain,
        "H", noIcon, noKey, noMark, plain,
        "I", noIcon, noKey, noMark, plain,
        "J", noIcon, noKey, noMark, plain,
        "K", noIcon, noKey, noMark, plain,
        "L", noIcon, noKey, noMark, plain,
        "M", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (207) {
    207,
    textMenuProc,
    allEnabled,
    enabled,
    "N - Z",
    {
        "N", noIcon, noKey, noMark, plain,
        "O", noIcon, noKey, noMark, plain,
        "P", noIcon, noKey, noMark, plain,
        "Q", noIcon, noKey, noMark, plain,
        "R", noIcon, noKey, noMark, plain,
        "S", noIcon, noKey, noMark, plain,
        "T", noIcon, noKey, noMark, plain,
        "U", noIcon, noKey, noMark, plain,
        "V", noIcon, noKey, noMark, plain,
        "W", noIcon, noKey, noMark, plain,
        "X", noIcon, noKey, noMark, plain,
        "Y", noIcon, noKey, noMark, plain,
        "Z", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (208) {
    208,
    textMenuProc,
    allEnabled,
    enabled,
    "0 - 9",
    {
        "0", noIcon, noKey, noMark, plain,
        "1", noIcon, noKey, noMark, plain,
        "2", noIcon, noKey, noMark, plain,
        "3", noIcon, noKey, noMark, plain,
        "4", noIcon, noKey, noMark, plain,
        "5", noIcon, noKey, noMark, plain,
        "6", noIcon, noKey, noMark, plain,
        "7", noIcon, noKey, noMark, plain,
        "8", noIcon, noKey, noMark, plain,
        "9", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (209) {
    209,
    textMenuProc,
    allEnabled,
    enabled,
    "current",
    {
        "Weapon", noIcon, noKey, ")", plain,
        "Armor", noIcon, noKey, "[", plain,
        "Rings", noIcon, noKey, "=", plain,
        "Amulet", noIcon, noKey, "\"", plain,
        "Tools", noIcon, noKey, "(", plain,
        "Gold", noIcon, noKey, "$", plain,
        "Spells", noIcon, noKey, "+", plain
    }
};

data 'MNU#' (128, "menubar", locked, preload) {  /* short firstMenuID; short count; { short mresID; short 0; } [count] */
    $"0080 000A 0080 0000 0081 0000 0082 0000"
    $"0083 0000 0084 0000 0085 0000 0086 0000"
    $"0087 0000 0088 0000 0089 0000"
};

data 'MNU#' (129, "submenu", locked, preload) {  /* short firstMenuID; short count; { short mresID; short 0; } [count] */
    $"00C8 000A 00C8 0000 00C9 0000 00CA 0000"
    $"00CB 0000 00CC 0000 00CD 0000 00CE 0000"
    $"00CF 0000 00D0 0000 00D1 0000"
};

resource 'STR#' (128, "Misc. Strings", purgeable) {
    {
        "Mac NetHack Help\0xC9/?"
    }
};

resource 'STR#' (131, "Kbd") {
    {
        "\0xA5201 Control Keys",
        "\0xA5202 Punctuation",
        "\0xA5203 Brackets",
        "\0xA5204 a - m",
        "\0xA5205 n - z",
        "\0xA5206 A - M",
        "\0xA5207 N - Z",
        "\0xA5208 0 - 9",
        "\0xA5-",
        "\0x1B\0xA5escape",
        " \0xA5space",
        "\0x08\0xA5delete",
        "\0x0D\0xA5return"
    }
};

resource 'STR#' (132, "Help") {
    {
        "O",
        "\0xA5-",
        "?",
        "&",
        "\0xA5-",
        "v",
        "V",
        "#version"
    }
};

resource 'STR#' (133, "Info") {
    {
        "i",
        "I",
        "#adjust",
        "\0xA5-",
        ":",
        ";",
        "/",
        "^",
        "\0xA5-",
        "C",
        "#name",
        "\0x5C"
    }
};

resource 'STR#' (134, "Equip") {
    {
        "\0xA5209 Current",
        "\0xA5-",
        "w",
        "x",
        "Q",
        "f",
        "t",
        "a",
        "#enhance",
        "#twoweapon",
        "\0xA5-",
        "W",
        "T",
        "A",
        "\0xA5-",
        "P",
        "R"
    }
};

resource 'STR#' (135, "Act") {
    {
        ".",
        "\0xA5-",
        "\0x14\0xA5 ctl-t",
        "#jump",
        "#monster",
        "#wipe",
        "\0xA5-",
        "a",
        "p",
        "\0x04\0xA5ctl-d",
        "#chat",
        "#offer",
        "#pray",
        "#ride",
        "#turn"
    }
};

resource 'STR#' (136, "Magic") {
    {
        "d",
        "D",
        ",",
        "@",
        "\0xA5-",
        "e",
        "r",
        "q",
        "#dip",
        "\0xA5-",
        "+",
        "Z",
        "z",
        "#invoke",
        "#rub"
    }
};

resource 'STR#' (137, "Bits") {
    {
        "s",
        "c",
        "o",
        "a",
        "\0x04\0xA5ctl-d",
        "\0xA5-",
        "#untrap",
        "#force",
        "#loot",
        "\0xA5-",
        "E",
        "#dip",
        "#sit",
        "\0xA5-",
        "<",
        ">"
    }
};

resource 'STR#' (200, "wizard") {
    {
        "\0x18\0xA5ctl-x",
        "\0x05\0xA5ctl-e",
        "\0x06\0xA5ctl-f",
        "\0x07\0xA5ctl-g",
        "\0x09\0xA5ctl-i",
        "\0x0F\0xA5ctl-o",
        "\0x16\0xA5ctl-v",
        "\0x17\0xA5ctl-w"
    }
};

resource 'STR#' (201, "control keys") {
    {
        "\0x02\0xA5ctl-b",
        "\0x0A\0xA5ctl-j",
        "\0x0E\0xA5ctl-n",
        "\0x08\0xA5ctl-h",
        "\0x0C\0xA5ctl-l",
        "\0x19\0xA5ctl-y",
        "\0x0B\0xA5ctl-k",
        "\0x15\0xA5ctl-u",
        "\0xA5-",
        "\0x04\0xA5ctl-d",
        "\0x10\0xA5ctl-p",
        "\0x12\0xA5ctl-r",
        "\0x14\0xA5ctl-t"
    }
};

resource 'STR#' (202, "punctuation") {
    {
        ".",
        ",",
        ";",
        ":",
        "!",
        "?",
        "+",
        "-",
        "=",
        "#",
        "$",
        "@",
        "&",
        "*",
        "~",
        "_"
    }
};

resource 'STR#' (203, "brackets") {
    {
        "[",
        "]",
        "(",
        ")",
        "{",
        "}",
        "<",
        ">",
        "^",
        "`",
        "'",
        "\"",
        "\0x5C",
        "/",
        "|",
        "%"
    }
};

resource 'STR#' (204, "a - m") {
    {
        "a",
        "b",
        "c",
        "d",
        "e",
        "f",
        "g",
        "h",
        "i",
        "j",
        "k",
        "l",
        "m"
    }
};

resource 'STR#' (205, "n - z") {
    {
        "n",
        "o",
        "p",
        "q",
        "r",
        "s",
        "t",
        "u",
        "v",
        "w",
        "x",
        "y",
        "z"
    }
};

resource 'STR#' (206, "A - M") {
    {
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M"
    }
};

resource 'STR#' (207, "N - Z") {
    {
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z"
    }
};

resource 'STR#' (208, "0 - 9") {
    {
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9"
    }
};

resource 'STR#' (209, "current") {
    {
        ")",
        "[",
        "=",
        "\"",
        "(",
        "$",
        "+"
    }
};

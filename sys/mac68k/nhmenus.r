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
    0x7FFFFFFD,
    enabled,
    "File",
    {
        "Save", noIcon, "S", noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Preferences\0xC9", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Quit", noIcon, "Q", noMark, plain
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

resource 'MENU' (132) {
    132,
    textMenuProc,
    0x7FFFFFED,
    enabled,
    "Help",
    {
        "Options\0xC9", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Help", noIcon, noKey, noMark, plain,
        "Describe Key\0xC9", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Version", noIcon, noKey, noMark, plain,
        "History", noIcon, noKey, noMark, plain,
        "Version Features", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (133) {
    133,
    textMenuProc,
    0x7FFFEEF7,
    enabled,
    "Info",
    {
        "Inventory All", noIcon, noKey, noMark, plain,
        "Inventory Select", noIcon, noKey, noMark, plain,
        "Adjust\0xC9", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Look Down", noIcon, noKey, noMark, plain,
        "Describe One\0xC9", noIcon, noKey, noMark, plain,
        "Describe Many\0xC9", noIcon, noKey, noMark, plain,
        "Describe Trap\0xC9", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Call Monster\0xC9", noIcon, noKey, noMark, plain,
        "Name Object\0xC9", noIcon, noKey, noMark, plain,
        "Discoveries", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Overview", noIcon, noKey, noMark, plain,
        "Annotate\0xC9", noIcon, noKey, noMark, plain,
        "Show Terrain", noIcon, noKey, noMark, plain,
        "Chronicle", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (134) {
    134,
    textMenuProc,
    0x7FFFBBFD,
    enabled,
    "Equip",
    {
        "Current", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xC9", plain,
        "-", noIcon, noKey, noMark, plain,
        "Wield Weapon", noIcon, noKey, noMark, plain,
        "Exchange Weapon", noIcon, noKey, noMark, plain,
        "Select Quiver", noIcon, noKey, noMark, plain,
        "Fire Quiver", noIcon, noKey, noMark, plain,
        "Throw", noIcon, noKey, noMark, plain,
        "Apply", noIcon, noKey, noMark, plain,
        "Enhance", noIcon, noKey, noMark, plain,
        "Two Weapon Combat", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Wear Armor", noIcon, noKey, noMark, plain,
        "Take Off", noIcon, noKey, noMark, plain,
        "Ask Remove", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Put On", noIcon, noKey, noMark, plain,
        "Remove", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (135) {
    135,
    textMenuProc,
    0x7FBBB7BB,
    enabled,
    "Act",
    {
        "Wait", noIcon, noKey, noMark, plain,
        "Search", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Open Door", noIcon, noKey, noMark, plain,
        "Close Door", noIcon, noKey, noMark, plain,
        "Kick", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Force Lock", noIcon, noKey, noMark, plain,
        "Loot", noIcon, noKey, noMark, plain,
        "Tip Container", noIcon, noKey, noMark, plain,
        "Untrap", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Engrave", noIcon, noKey, noMark, plain,
        "Sit", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Climb Up", noIcon, noKey, noMark, plain,
        "Climb Down", noIcon, noKey, noMark, plain,
        "Travel", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Jump", noIcon, noKey, noMark, plain,
        "Monster Ability", noIcon, noKey, noMark, plain,
        "Wipe Face", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Pay Bill", noIcon, noKey, noMark, plain,
        "Chat", noIcon, noKey, noMark, plain,
        "Pray", noIcon, noKey, noMark, plain,
        "Offer Sacrifice", noIcon, noKey, noMark, plain,
        "Ride", noIcon, noKey, noMark, plain,
        "Turn Undead", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (136) {
    136,
    textMenuProc,
    0x7FFFFFF7,
    enabled,
    "Magic",
    {
        "Read", noIcon, noKey, noMark, plain,
        "Quaff", noIcon, noKey, noMark, plain,
        "Dip", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "List Spells", noIcon, noKey, noMark, plain,
        "Cast Spell", noIcon, noKey, noMark, plain,
        "Zap", noIcon, noKey, noMark, plain,
        "Invoke", noIcon, noKey, noMark, plain,
        "Rub", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (137) {
    137,
    textMenuProc,
    0x7FFFFFEF,
    enabled,
    "Item",
    {
        "Drop Item", noIcon, noKey, noMark, plain,
        "Drop Multiple\0xC9", noIcon, noKey, noMark, plain,
        "Pickup", noIcon, noKey, noMark, plain,
        "Toggle Autopickup", noIcon, noKey, noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Eat", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (138) {
    138,
    textMenuProc,
    0x7FFFFFEF,
    enabled,
    "Game",
    {
        "Redraw", noIcon, "R", noMark, plain,
        "Previous Message", noIcon, "P", noMark, plain,
        "Reposition Windows", noIcon, "N", noMark, plain,
        "Tile Mode", noIcon, "T", noMark, plain,
        "-", noIcon, noKey, noMark, plain,
        "Play Mode", noIcon, "\0x1B" /* hierarchicalMenu */, "\0xCA", plain
    }
};

resource 'MENU' (200) {
    200,
    textMenuProc,
    allEnabled,
    enabled,
    "wizard",
    {
        "Attributes", noIcon, noKey, noMark, plain,
        "Detect Unseen", noIcon, noKey, noMark, plain,
        "Floor Map", noIcon, noKey, noMark, plain,
        "Generate Monster", noIcon, noKey, noMark, plain,
        "Identify", noIcon, noKey, noMark, plain,
        "Locations", noIcon, noKey, noMark, plain,
        "Level Teleport", noIcon, noKey, noMark, plain,
        "Wish", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (209) {
    209,
    textMenuProc,
    allEnabled,
    enabled,
    "current",
    {
        "Weapon", noIcon, noKey, noMark, plain,
        "Armor", noIcon, noKey, noMark, plain,
        "Rings", noIcon, noKey, noMark, plain,
        "Amulet", noIcon, noKey, noMark, plain,
        "Tools", noIcon, noKey, noMark, plain,
        "Gold", noIcon, noKey, noMark, plain,
        "Spells", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (210) {
    210,
    textMenuProc,
    allEnabled,
    enabled,
    "Play Mode",
    {
        "Regular", noIcon, noKey, noMark, plain,
        "Explore", noIcon, noKey, noMark, plain,
        "Debug", noIcon, noKey, noMark, plain
    }
};

data 'MNU#' (128, "menubar", locked, preload) {  /* short firstMenuID; short count; { short mresID; short 0; } [count] */
    $"0080 000A 0080 0000 0081 0000 0082 0000"
    $"008A 0000 0085 0000 0086 0000 0087 0000"
    $"0089 0000 0088 0000 0084 0000"
};

data 'MNU#' (129, "submenu", locked, preload) {  /* short firstMenuID; short count; { short mresID; short 0; } [count] */
    $"00C8 0003 00C8 0000 00D1 0000 00D2 0000"
};

resource 'STR#' (128, "Misc. Strings", purgeable) {
    {
        "Mac NetHack Help\0xC9/?"
    }
};

resource 'STR#' (132, "Info") {
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
        "\0x5C",
        "\0xA5-",
        "\0x0F\0xA5ctl-o",
        "#annotate",
        "#terrain",
        "#chronicle"
    }
};

resource 'STR#' (133, "Equip") {
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

resource 'STR#' (134, "Act") {
    {
        ".",
        "s",
        "\0xA5-",
        "o",
        "c",
        "\0x04\0xA5ctl-d",
        "\0xA5-",
        "#force",
        "#loot",
        "#tip",
        "#untrap",
        "\0xA5-",
        "E",
        "#sit",
        "\0xA5-",
        "<",
        ">",
        "_",
        "\0xA5-",
        "#jump",
        "#monster",
        "#wipe",
        "\0xA5-",
        "p",
        "#chat",
        "#pray",
        "#offer",
        "#ride",
        "#turn"
    }
};

resource 'STR#' (135, "Item") {
    {
        "d",
        "D",
        ",",
        "@",
        "\0xA5-",
        "e"
    }
};

resource 'STR#' (136, "Magic") {
    {
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

/* NetHack 5.0 rebound 'v' to #chronicle and 'V' to #versionshort, so
   the version/history items use explicit extended names (rebinding-
   proof, same lesson as the Explore/'X' fix) */
resource 'STR#' (137, "Help") {
    {
        "O",
        "\0xA5-",
        "?",
        "&",
        "\0xA5-",
        "#versionshort",
        "#history",
        "#version"
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

resource 'STR#' (201, "current") {
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

/* Preferences dialog (macprefs.c).  Item numbers are load-bearing --
 * they must match the prefSave..prefSizeLast enum in macprefs.c: the
 * font popups (8-12) and size fields (13-17) are contiguous runs in
 * uiprefs_fonts order (map, status, message, menu, text). */
resource 'DLOG' (6100, "Preferences") {
    {24, 56, 320, 456},
    dBoxProc,
    visible,
    noGoAway,
    0x0,
    6100,
    "Preferences",
    centerMainScreen
};

resource 'DITL' (6100) {
    {
        {264, 310, 284, 384}, Button { enabled, "Save" },          /* 1 */
        {264, 228, 284, 292}, Button { enabled, "Cancel" },        /* 2 */
        {264, 16, 284, 140},  Button { enabled, "Forget Settings" }, /* 3 */
        {12, 16, 30, 190},    CheckBox { enabled, "Tiled map" },   /* 4 */
        {36, 16, 54, 190},    CheckBox { enabled, "Hit-point bar" }, /* 5 */
        {12, 210, 30, 384},   RadioButton { enabled, "2 status lines" }, /* 6 */
        {36, 210, 54, 384},   RadioButton { enabled, "3 status lines" }, /* 7 */
        {78, 100, 98, 300},   UserItem { enabled },                /* 8 map font */
        {106, 100, 126, 300}, UserItem { enabled },                /* 9 status */
        {134, 100, 154, 300}, UserItem { enabled },                /* 10 message */
        {162, 100, 182, 300}, UserItem { enabled },                /* 11 menu */
        {190, 100, 210, 300}, UserItem { enabled },                /* 12 text */
        {80, 316, 96, 356},   EditText { enabled, "" },            /* 13 map size */
        {108, 316, 124, 356}, EditText { enabled, "" },            /* 14 status */
        {136, 316, 152, 356}, EditText { enabled, "" },            /* 15 message */
        {164, 316, 180, 356}, EditText { enabled, "" },            /* 16 menu */
        {192, 316, 208, 356}, EditText { enabled, "" },            /* 17 text */
        {80, 16, 96, 96},     StaticText { disabled, "Map:" },     /* 18 */
        {108, 16, 124, 96},   StaticText { disabled, "Status:" },  /* 19 */
        {136, 16, 152, 96},   StaticText { disabled, "Message:" }, /* 20 */
        {164, 16, 180, 96},   StaticText { disabled, "Menu:" },    /* 21 */
        {192, 16, 208, 96},   StaticText { disabled, "Text:" },    /* 22 */
        {222, 16, 254, 384},  StaticText { disabled,
            "Changes take effect at the next launch." }            /* 23 */
    }
};

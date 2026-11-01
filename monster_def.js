const allMonsters = {
    "abbot": {
        id: "abbot",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "acid blob": {
        id: "acid blob",
        class1: "blob",
        class2: "",
        symbol: "b",
        color: "CLR_GREEN"
    },
    "acolyte": {
        id: "acolyte",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "air elemental": {
        id: "air elemental",
        class1: "elemental",
        class2: "",
        symbol: "E",
        color: "CLR_CYAN"
    },
    "aleax": {
        id: "Aleax",
        class1: "angelic being",
        class2: "",
        symbol: "A",
        color: "CLR_YELLOW"
    },
    "aligned cleric": {
        id: "aligned cleric",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_WHITE"
    },
    "amorous demon": {
        id: "amorous demon",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GRAY"
    },
    "angel": {
        id: "Angel",
        class1: "angelic being",
        class2: "",
        symbol: "A",
        color: "CLR_WHITE"
    },
    "ape": {
        id: "ape",
        class1: "apelike creature",
        class2: "",
        symbol: "Y",
        color: "CLR_BROWN"
    },
    "apprentice": {
        id: "apprentice",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "arch priest": {
        id: "Arch Priest",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_WHITE"
    },
    "arch-lich": {
        id: "arch-lich",
        class1: "lich",
        class2: "",
        symbol: "L",
        color: "HI_LORD"
    },
    "archeologist": {
        id: "archeologist",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "archon": {
        id: "Archon",
        class1: "angelic being",
        class2: "",
        symbol: "A",
        color: "HI_LORD"
    },
    "ashikaga takauji": {
        id: "Ashikaga Takauji",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "asmodeus": {
        id: "Asmodeus",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "attendant": {
        id: "attendant",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "baalzebub": {
        id: "Baalzebub",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "baby black dragon": {
        id: "baby black dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_BLACK"
    },
    "baby blue dragon": {
        id: "baby blue dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_BLUE"
    },
    "baby crocodile": {
        id: "baby crocodile",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_BROWN"
    },
    "baby gold dragon": {
        id: "baby gold dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "HI_GOLD"
    },
    "baby gray dragon": {
        id: "baby gray dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_GRAY"
    },
    "baby green dragon": {
        id: "baby green dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_GREEN"
    },
    "baby long worm": {
        id: "baby long worm",
        class1: "worm",
        class2: "",
        symbol: "w",
        color: "CLR_BROWN"
    },
    "baby orange dragon": {
        id: "baby orange dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_ORANGE"
    },
    "baby purple worm": {
        id: "baby purple worm",
        class1: "worm",
        class2: "",
        symbol: "w",
        color: "CLR_MAGENTA"
    },
    "baby red dragon": {
        id: "baby red dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_RED"
    },
    "baby shimmering dragon": {
        id: "baby shimmering dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_CYAN"
    },
    "baby silver dragon": {
        id: "baby silver dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "DRAGON_SILVER"
    },
    "baby white dragon": {
        id: "baby white dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_WHITE"
    },
    "baby yellow dragon": {
        id: "baby yellow dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_YELLOW"
    },
    "balrog": {
        id: "balrog",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_RED"
    },
    "baluchitherium": {
        id: "baluchitherium",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_GRAY"
    },
    "barbarian": {
        id: "barbarian",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "barbed devil": {
        id: "barbed devil",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_RED"
    },
    "barrow wight": {
        id: "barrow wight",
        class1: "wraith",
        class2: "",
        symbol: "W",
        color: "CLR_GRAY"
    },
    "bat": {
        id: "bat",
        class1: "bat",
        class2: "bird",
        symbol: "B",
        color: "CLR_BROWN"
    },
    "beholder": {
        id: "beholder",
        class1: "eye",
        class2: "sphere",
        symbol: "e",
        color: "CLR_BROWN"
    },
    "black dragon": {
        id: "black dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_BLACK"
    },
    "black light": {
        id: "black light",
        class1: "light",
        class2: "",
        symbol: "y",
        color: "CLR_BLACK"
    },
    "black naga": {
        id: "black naga",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "CLR_BLACK"
    },
    "black naga hatchling": {
        id: "black naga hatchling",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "CLR_BLACK"
    },
    "black pudding": {
        id: "black pudding",
        class1: "pudding",
        class2: "ooze",
        symbol: "P",
        color: "CLR_BLACK"
    },
    "black unicorn": {
        id: "black unicorn",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_BLACK"
    },
    "blue dragon": {
        id: "blue dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_BLUE"
    },
    "blue jelly": {
        id: "blue jelly",
        class1: "jelly",
        class2: "",
        symbol: "j",
        color: "CLR_BLUE"
    },
    "bone devil": {
        id: "bone devil",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GRAY"
    },
    "brown mold": {
        id: "brown mold",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_BROWN"
    },
    "brown pudding": {
        id: "brown pudding",
        class1: "pudding",
        class2: "ooze",
        symbol: "P",
        color: "CLR_BROWN"
    },
    "bugbear": {
        id: "bugbear",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_BROWN"
    },
    "captain": {
        id: "captain",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BLUE"
    },
    "carnivorous ape": {
        id: "carnivorous ape",
        class1: "apelike creature",
        class2: "",
        symbol: "Y",
        color: "CLR_BLACK"
    },
    "cave dweller": {
        id: "cave dweller",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "cave spider": {
        id: "cave spider",
        class1: "arachnid",
        class2: "centipede",
        symbol: "s",
        color: "CLR_GRAY"
    },
    "caveman": {
        id: "caveman",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "cavewoman": {
        id: "cavewoman",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "centipede": {
        id: "centipede",
        class1: "arachnid",
        class2: "centipede",
        symbol: "s",
        color: "CLR_YELLOW"
    },
    "cerberus": {
        id: "Cerberus",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_RED"
    },
    "chameleon": {
        id: "chameleon",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_BROWN"
    },
    "charon": {
        id: "Charon",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_WHITE"
    },
    "chickatrice": {
        id: "chickatrice",
        class1: "cockatrice",
        class2: "",
        symbol: "c",
        color: "CLR_BROWN"
    },
    "chieftain": {
        id: "chieftain",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "chromatic dragon": {
        id: "Chromatic Dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "HI_LORD"
    },
    "clay golem": {
        id: "clay golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "CLR_BROWN"
    },
    "cleric": {
        id: "cleric",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "cobra": {
        id: "cobra",
        class1: "snake",
        class2: "",
        symbol: "S",
        color: "CLR_BLUE"
    },
    "cockatrice": {
        id: "cockatrice",
        class1: "cockatrice",
        class2: "",
        symbol: "c",
        color: "CLR_YELLOW"
    },
    "couatl": {
        id: "couatl",
        class1: "angelic being",
        class2: "",
        symbol: "A",
        color: "CLR_GREEN"
    },
    "coyote": {
        id: "coyote",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_BROWN"
    },
    "crocodile": {
        id: "crocodile",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_BROWN"
    },
    "croesus": {
        id: "Croesus",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "cyclops": {
        id: "Cyclops",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_GRAY"
    },
    "dark one": {
        id: "Dark One",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BLACK"
    },
    "death": {
        id: "Death",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_OVERLORD"
    },
    "demilich": {
        id: "demilich",
        class1: "lich",
        class2: "",
        symbol: "L",
        color: "CLR_RED"
    },
    "demogorgon": {
        id: "Demogorgon",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "dingo": {
        id: "dingo",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_YELLOW"
    },
    "disenchanter": {
        id: "disenchanter",
        class1: "rust monster",
        class2: "disenchanter",
        symbol: "R",
        color: "CLR_BLUE"
    },
    "dispater": {
        id: "Dispater",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "displacer beast": {
        id: "displacer beast",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "CLR_BLUE"
    },
    "djinni": {
        id: "djinni",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_YELLOW"
    },
    "dog": {
        id: "dog",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "HI_DOMESTIC"
    },
    "doppelganger": {
        id: "doppelganger",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "dust vortex": {
        id: "dust vortex",
        class1: "vortex",
        class2: "",
        symbol: "v",
        color: "CLR_BROWN"
    },
    "dwarf": {
        id: "dwarf",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_RED"
    },
    "dwarf king": {
        id: "dwarf king",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "HI_LORD"
    },
    "dwarf lady": {
        id: "dwarf lady",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_BLUE"
    },
    "dwarf leader": {
        id: "dwarf leader",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_BLUE"
    },
    "dwarf lord": {
        id: "dwarf lord",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_BLUE"
    },
    "dwarf mummy": {
        id: "dwarf mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_RED"
    },
    "dwarf queen": {
        id: "dwarf queen",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "HI_LORD"
    },
    "dwarf ruler": {
        id: "dwarf ruler",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "HI_LORD"
    },
    "dwarf zombie": {
        id: "dwarf zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_RED"
    },
    "earendil": {
        id: "Earendil",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "earth elemental": {
        id: "earth elemental",
        class1: "elemental",
        class2: "",
        symbol: "E",
        color: "CLR_BROWN"
    },
    "electric eel": {
        id: "electric eel",
        class1: "sea monster",
        class2: "",
        symbol: ";",
        color: "CLR_BRIGHT_BLUE"
    },
    "elf": {
        id: "elf",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "elf mummy": {
        id: "elf mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_GREEN"
    },
    "elf zombie": {
        id: "elf zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_GREEN"
    },
    "elf-lady": {
        id: "elf-lady",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BRIGHT_BLUE"
    },
    "elf-lord": {
        id: "elf-lord",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BRIGHT_BLUE"
    },
    "elf-noble": {
        id: "elf-noble",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BRIGHT_BLUE"
    },
    "elven monarch": {
        id: "elven monarch",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "elvenking": {
        id: "Elvenking",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "elvenqueen": {
        id: "Elvenqueen",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "elwing": {
        id: "Elwing",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "energy vortex": {
        id: "energy vortex",
        class1: "vortex",
        class2: "",
        symbol: "v",
        color: "HI_ZAP"
    },
    "erinys": {
        id: "erinys",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_RED"
    },
    "ettin": {
        id: "ettin",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_BROWN"
    },
    "ettin mummy": {
        id: "ettin mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_BLUE"
    },
    "ettin zombie": {
        id: "ettin zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_BLUE"
    },
    "famine": {
        id: "Famine",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_OVERLORD"
    },
    "fire ant": {
        id: "fire ant",
        class1: "ant",
        class2: "other insect",
        symbol: "a",
        color: "CLR_RED"
    },
    "fire elemental": {
        id: "fire elemental",
        class1: "elemental",
        class2: "",
        symbol: "E",
        color: "CLR_YELLOW"
    },
    "fire giant": {
        id: "fire giant",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_YELLOW"
    },
    "fire vortex": {
        id: "fire vortex",
        class1: "vortex",
        class2: "",
        symbol: "v",
        color: "CLR_YELLOW"
    },
    "firey steed of the heavenly host": {
        id: "firey steed of the Heavenly Host",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_WHITE"
    },
    "flaming sphere": {
        id: "flaming sphere",
        class1: "eye",
        class2: "sphere",
        symbol: "e",
        color: "CLR_RED"
    },
    "flesh golem": {
        id: "flesh golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "CLR_RED"
    },
    "floating eye": {
        id: "floating eye",
        class1: "eye",
        class2: "sphere",
        symbol: "e",
        color: "CLR_BLUE"
    },
    "fog cloud": {
        id: "fog cloud",
        class1: "vortex",
        class2: "",
        symbol: "v",
        color: "CLR_GRAY"
    },
    "forest centaur": {
        id: "forest centaur",
        class1: "centaur",
        class2: "",
        symbol: "C",
        color: "CLR_GREEN"
    },
    "fox": {
        id: "fox",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_RED"
    },
    "freezing sphere": {
        id: "freezing sphere",
        class1: "eye",
        class2: "sphere",
        symbol: "e",
        color: "CLR_WHITE"
    },
    "frost giant": {
        id: "frost giant",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_WHITE"
    },
    "gargoyle": {
        id: "gargoyle",
        class1: "gremlin",
        class2: "",
        symbol: "g",
        color: "CLR_BROWN"
    },
    "garter snake": {
        id: "garter snake",
        class1: "snake",
        class2: "",
        symbol: "S",
        color: "CLR_GREEN"
    },
    "gas spore": {
        id: "gas spore",
        class1: "eye",
        class2: "sphere",
        symbol: "e",
        color: "CLR_GRAY"
    },
    "gecko": {
        id: "gecko",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_GREEN"
    },
    "gelatinous cube": {
        id: "gelatinous cube",
        class1: "blob",
        class2: "",
        symbol: "b",
        color: "CLR_CYAN"
    },
    "genetic engineer": {
        id: "genetic engineer",
        class1: "quantum mechanic",
        class2: "",
        symbol: "Q",
        color: "CLR_GREEN"
    },
    "geryon": {
        id: "Geryon",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "ghost": {
        id: "ghost",
        class1: "ghost",
        class2: "",
        symbol: " ",
        color: "CLR_GRAY"
    },
    "ghoul": {
        id: "ghoul",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_BLACK"
    },
    "giant": {
        id: "giant",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_RED"
    },
    "giant ant": {
        id: "giant ant",
        class1: "ant",
        class2: "other insect",
        symbol: "a",
        color: "CLR_BROWN"
    },
    "giant bat": {
        id: "giant bat",
        class1: "bat",
        class2: "bird",
        symbol: "B",
        color: "CLR_RED"
    },
    "giant beetle": {
        id: "giant beetle",
        class1: "ant",
        class2: "other insect",
        symbol: "a",
        color: "CLR_BLACK"
    },
    "giant eel": {
        id: "giant eel",
        class1: "sea monster",
        class2: "",
        symbol: ";",
        color: "CLR_CYAN"
    },
    "giant mimic": {
        id: "giant mimic",
        class1: "mimic",
        class2: "",
        symbol: "m",
        color: "HI_LORD"
    },
    "giant mummy": {
        id: "giant mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_CYAN"
    },
    "giant rat": {
        id: "giant rat",
        class1: "rodent",
        class2: "",
        symbol: "r",
        color: "CLR_BROWN"
    },
    "giant spider": {
        id: "giant spider",
        class1: "arachnid",
        class2: "centipede",
        symbol: "s",
        color: "CLR_MAGENTA"
    },
    "giant zombie": {
        id: "giant zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_CYAN"
    },
    "glass golem": {
        id: "glass golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "CLR_CYAN"
    },
    "glass piercer": {
        id: "glass piercer",
        class1: "piercer",
        class2: "",
        symbol: "p",
        color: "CLR_WHITE"
    },
    "gnome": {
        id: "gnome",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "CLR_BROWN"
    },
    "gnome king": {
        id: "gnome king",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "HI_LORD"
    },
    "gnome lady": {
        id: "gnome lady",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "CLR_BLUE"
    },
    "gnome leader": {
        id: "gnome leader",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "CLR_BLUE"
    },
    "gnome lord": {
        id: "gnome lord",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "CLR_BLUE"
    },
    "gnome mummy": {
        id: "gnome mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_RED"
    },
    "gnome queen": {
        id: "gnome queen",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "HI_LORD"
    },
    "gnome ruler": {
        id: "gnome ruler",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "HI_LORD"
    },
    "gnome zombie": {
        id: "gnome zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_BROWN"
    },
    "gnomish wizard": {
        id: "gnomish wizard",
        class1: "gnome",
        class2: "",
        symbol: "G",
        color: "HI_ZAP"
    },
    "goblin": {
        id: "goblin",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "CLR_GRAY"
    },
    "goblin king": {
        id: "Goblin King",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "HI_LORD"
    },
    "gold dragon": {
        id: "gold dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "HI_GOLD"
    },
    "gold golem": {
        id: "gold golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "HI_GOLD"
    },
    "golden naga": {
        id: "golden naga",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "HI_GOLD"
    },
    "golden naga hatchling": {
        id: "golden naga hatchling",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "HI_GOLD"
    },
    "grand master": {
        id: "Grand Master",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BLACK"
    },
    "gray dragon": {
        id: "gray dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_GRAY"
    },
    "gray ooze": {
        id: "gray ooze",
        class1: "pudding",
        class2: "ooze",
        symbol: "P",
        color: "CLR_GRAY"
    },
    "gray unicorn": {
        id: "gray unicorn",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_GRAY"
    },
    "green dragon": {
        id: "green dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_GREEN"
    },
    "green mold": {
        id: "green mold",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_GREEN"
    },
    "green slime": {
        id: "green slime",
        class1: "pudding",
        class2: "ooze",
        symbol: "P",
        color: "CLR_GREEN"
    },
    "green-elf": {
        id: "Green-elf",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BRIGHT_GREEN"
    },
    "gremlin": {
        id: "gremlin",
        class1: "gremlin",
        class2: "",
        symbol: "g",
        color: "CLR_GREEN"
    },
    "grey-elf": {
        id: "Grey-elf",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GRAY"
    },
    "grid bug": {
        id: "grid bug",
        class1: "xan",
        class2: "other mythical/fantastic insect",
        symbol: "x",
        color: "CLR_MAGENTA"
    },
    "guard": {
        id: "guard",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BLUE"
    },
    "guardian naga": {
        id: "guardian naga",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "CLR_GREEN"
    },
    "guardian naga hatchling": {
        id: "guardian naga hatchling",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "CLR_GREEN"
    },
    "guide": {
        id: "guide",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "healer": {
        id: "healer",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "hell hound": {
        id: "hell hound",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_RED"
    },
    "hell hound pup": {
        id: "hell hound pup",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_RED"
    },
    "hezrou": {
        id: "hezrou",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GREEN"
    },
    "high cleric": {
        id: "high cleric",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_WHITE"
    },
    "high priest": {
        id: "high priest",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_WHITE"
    },
    "high priestess": {
        id: "high priestess",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_WHITE"
    },
    "high-elf": {
        id: "High-elf",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "hill giant": {
        id: "hill giant",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_CYAN"
    },
    "hill orc": {
        id: "hill orc",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "CLR_YELLOW"
    },
    "hippocrates": {
        id: "Hippocrates",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "hobbit": {
        id: "hobbit",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_GREEN"
    },
    "hobgoblin": {
        id: "hobgoblin",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "CLR_BROWN"
    },
    "homunculus": {
        id: "homunculus",
        class1: "imp",
        class2: "minor demon",
        symbol: "i",
        color: "CLR_GREEN"
    },
    "horned devil": {
        id: "horned devil",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_BROWN"
    },
    "horse": {
        id: "horse",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_BROWN"
    },
    "housecat": {
        id: "housecat",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "HI_DOMESTIC"
    },
    "human": {
        id: "human",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "human mummy": {
        id: "human mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_GRAY"
    },
    "human zombie": {
        id: "human zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "HI_DOMESTIC"
    },
    "hunter": {
        id: "hunter",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "ice devil": {
        id: "ice devil",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_WHITE"
    },
    "ice troll": {
        id: "ice troll",
        class1: "troll",
        class2: "",
        symbol: "T",
        color: "CLR_WHITE"
    },
    "ice vortex": {
        id: "ice vortex",
        class1: "vortex",
        class2: "",
        symbol: "v",
        color: "CLR_CYAN"
    },
    "iguana": {
        id: "iguana",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_BROWN"
    },
    "imp": {
        id: "imp",
        class1: "imp",
        class2: "minor demon",
        symbol: "i",
        color: "CLR_RED"
    },
    "incubus": {
        id: "incubus",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GRAY"
    },
    "iron golem": {
        id: "iron golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "HI_METAL"
    },
    "iron piercer": {
        id: "iron piercer",
        class1: "piercer",
        class2: "",
        symbol: "p",
        color: "CLR_CYAN"
    },
    "ixoth": {
        id: "Ixoth",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_RED"
    },
    "jabberwock": {
        id: "jabberwock",
        class1: "jabberwock",
        class2: "",
        symbol: "J",
        color: "CLR_ORANGE"
    },
    "jackal": {
        id: "jackal",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_BROWN"
    },
    "jaguar": {
        id: "jaguar",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "CLR_BROWN"
    },
    "jellyfish": {
        id: "jellyfish",
        class1: "sea monster",
        class2: "",
        symbol: ";",
        color: "CLR_BLUE"
    },
    "juiblex": {
        id: "Juiblex",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_BRIGHT_GREEN"
    },
    "keystone kop": {
        id: "Keystone Kop",
        class1: "keystone kop",
        class2: "",
        symbol: "K",
        color: "CLR_BLUE"
    },
    "ki-rin": {
        id: "ki-rin",
        class1: "angelic being",
        class2: "",
        symbol: "A",
        color: "HI_GOLD"
    },
    "killer bee": {
        id: "killer bee",
        class1: "ant",
        class2: "other insect",
        symbol: "a",
        color: "CLR_YELLOW"
    },
    "king arthur": {
        id: "King Arthur",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "kitten": {
        id: "kitten",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "HI_DOMESTIC"
    },
    "knight": {
        id: "knight",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "kobold": {
        id: "kobold",
        class1: "kobold",
        class2: "",
        symbol: "k",
        color: "CLR_BROWN"
    },
    "kobold lady": {
        id: "kobold lady",
        class1: "kobold",
        class2: "",
        symbol: "k",
        color: "HI_LORD"
    },
    "kobold leader": {
        id: "kobold leader",
        class1: "kobold",
        class2: "",
        symbol: "k",
        color: "HI_LORD"
    },
    "kobold lord": {
        id: "kobold lord",
        class1: "kobold",
        class2: "",
        symbol: "k",
        color: "HI_LORD"
    },
    "kobold mummy": {
        id: "kobold mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_BROWN"
    },
    "kobold shaman": {
        id: "kobold shaman",
        class1: "kobold",
        class2: "",
        symbol: "k",
        color: "HI_ZAP"
    },
    "kobold zombie": {
        id: "kobold zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_BROWN"
    },
    "kop kaptain": {
        id: "Kop Kaptain",
        class1: "keystone kop",
        class2: "",
        symbol: "K",
        color: "HI_LORD"
    },
    "kop lieutenant": {
        id: "Kop Lieutenant",
        class1: "keystone kop",
        class2: "",
        symbol: "K",
        color: "CLR_CYAN"
    },
    "kop sergeant": {
        id: "Kop Sergeant",
        class1: "keystone kop",
        class2: "",
        symbol: "K",
        color: "CLR_BLUE"
    },
    "kraken": {
        id: "kraken",
        class1: "sea monster",
        class2: "",
        symbol: ";",
        color: "CLR_RED"
    },
    "large cat": {
        id: "large cat",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "HI_DOMESTIC"
    },
    "large dog": {
        id: "large dog",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "HI_DOMESTIC"
    },
    "large kobold": {
        id: "large kobold",
        class1: "kobold",
        class2: "",
        symbol: "k",
        color: "CLR_RED"
    },
    "large mimic": {
        id: "large mimic",
        class1: "mimic",
        class2: "",
        symbol: "m",
        color: "CLR_RED"
    },
    "leather golem": {
        id: "leather golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "HI_LEATHER"
    },
    "lemure": {
        id: "lemure",
        class1: "imp",
        class2: "minor demon",
        symbol: "i",
        color: "CLR_BROWN"
    },
    "leocrotta": {
        id: "leocrotta",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_RED"
    },
    "leprechaun": {
        id: "leprechaun",
        class1: "leprechaun",
        class2: "",
        symbol: "l",
        color: "CLR_GREEN"
    },
    "lich": {
        id: "lich",
        class1: "lich",
        class2: "",
        symbol: "L",
        color: "CLR_BROWN"
    },
    "lichen": {
        id: "lichen",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_BRIGHT_GREEN"
    },
    "lieutenant": {
        id: "lieutenant",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GREEN"
    },
    "little dog": {
        id: "little dog",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "HI_DOMESTIC"
    },
    "lizard": {
        id: "lizard",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_GREEN"
    },
    "long worm": {
        id: "long worm",
        class1: "worm",
        class2: "",
        symbol: "w",
        color: "CLR_BROWN"
    },
    "long worm tail": {
        id: "long worm tail",
        class1: "long worm tail",
        class2: "",
        symbol: "~",
        color: "CLR_BROWN"
    },
    "lord carnarvon": {
        id: "Lord Carnarvon",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "lord sato": {
        id: "Lord Sato",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "lord surtur": {
        id: "Lord Surtur",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "HI_LORD"
    },
    "lurker above": {
        id: "lurker above",
        class1: "trapper",
        class2: "lurker above",
        symbol: "t",
        color: "CLR_GRAY"
    },
    "lynx": {
        id: "lynx",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "CLR_CYAN"
    },
    "mail daemon": {
        id: "mail daemon",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_BRIGHT_BLUE"
    },
    "manes": {
        id: "manes",
        class1: "imp",
        class2: "minor demon",
        symbol: "i",
        color: "CLR_RED"
    },
    "marilith": {
        id: "marilith",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_RED"
    },
    "master assassin": {
        id: "Master Assassin",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "master kaen": {
        id: "Master Kaen",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "master lich": {
        id: "master lich",
        class1: "lich",
        class2: "",
        symbol: "L",
        color: "HI_LORD"
    },
    "master mind flayer": {
        id: "master mind flayer",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_BRIGHT_MAGENTA"
    },
    "master of thieves": {
        id: "Master of Thieves",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "mastodon": {
        id: "mastodon",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_BLACK"
    },
    "medusa": {
        id: "Medusa",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BRIGHT_GREEN"
    },
    "mind flayer": {
        id: "mind flayer",
        class1: "humanoid",
        class2: "",
        symbol: "h",
        color: "CLR_BRIGHT_MAGENTA"
    },
    "minion of huhetotl": {
        id: "Minion of Huhetotl",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_ORANGE"
    },
    "minotaur": {
        id: "minotaur",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_BROWN"
    },
    "monk": {
        id: "monk",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "monkey": {
        id: "monkey",
        class1: "apelike creature",
        class2: "",
        symbol: "Y",
        color: "CLR_GRAY"
    },
    "mordor orc": {
        id: "Mordor orc",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "CLR_BLUE"
    },
    "mountain centaur": {
        id: "mountain centaur",
        class1: "centaur",
        class2: "",
        symbol: "C",
        color: "CLR_CYAN"
    },
    "mountain nymph": {
        id: "mountain nymph",
        class1: "nymph",
        class2: "",
        symbol: "n",
        color: "CLR_BROWN"
    },
    "mumak": {
        id: "mumak",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_GRAY"
    },
    "nalfeshnee": {
        id: "nalfeshnee",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_RED"
    },
    "nalzok": {
        id: "Nalzok",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_ORANGE"
    },
    "nazgul": {
        id: "Nazgul",
        class1: "wraith",
        class2: "",
        symbol: "W",
        color: "HI_LORD"
    },
    "neanderthal": {
        id: "neanderthal",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "neferet the green": {
        id: "Neferet the Green",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GREEN"
    },
    "newt": {
        id: "newt",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_YELLOW"
    },
    "ninja": {
        id: "ninja",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "norn": {
        id: "Norn",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "nurse": {
        id: "nurse",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "ochre jelly": {
        id: "ochre jelly",
        class1: "jelly",
        class2: "",
        symbol: "j",
        color: "CLR_BROWN"
    },
    "ogre": {
        id: "ogre",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "CLR_BROWN"
    },
    "ogre king": {
        id: "ogre king",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "HI_LORD"
    },
    "ogre lady": {
        id: "ogre lady",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "CLR_RED"
    },
    "ogre leader": {
        id: "ogre leader",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "CLR_RED"
    },
    "ogre lord": {
        id: "ogre lord",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "CLR_RED"
    },
    "ogre queen": {
        id: "ogre queen",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "HI_LORD"
    },
    "ogre tyrant": {
        id: "ogre tyrant",
        class1: "ogre",
        class2: "",
        symbol: "O",
        color: "HI_LORD"
    },
    "olog-hai": {
        id: "Olog-hai",
        class1: "troll",
        class2: "",
        symbol: "T",
        color: "HI_LORD"
    },
    "prophet": {
        id: "Prophet",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_ZAP"
    },
    "orange dragon": {
        id: "orange dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_ORANGE"
    },
    "orc": {
        id: "orc",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "CLR_RED"
    },
    "orc mummy": {
        id: "orc mummy",
        class1: "mummy",
        class2: "",
        symbol: "M",
        color: "CLR_GRAY"
    },
    "orc shaman": {
        id: "orc shaman",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "HI_ZAP"
    },
    "orc zombie": {
        id: "orc zombie",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_GRAY"
    },
    "orc-captain": {
        id: "orc-captain",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "HI_LORD"
    },
    "orcus": {
        id: "Orcus",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "orion": {
        id: "Orion",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "owlbear": {
        id: "owlbear",
        class1: "apelike creature",
        class2: "",
        symbol: "Y",
        color: "CLR_BROWN"
    },
    "page": {
        id: "page",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "panther": {
        id: "panther",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "CLR_BLACK"
    },
    "paper golem": {
        id: "paper golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "HI_PAPER"
    },
    "pelias": {
        id: "Pelias",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "pestilence": {
        id: "Pestilence",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_OVERLORD"
    },
    "piranha": {
        id: "piranha",
        class1: "sea monster",
        class2: "",
        symbol: ";",
        color: "CLR_RED"
    },
    "pit fiend": {
        id: "pit fiend",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_RED"
    },
    "pit viper": {
        id: "pit viper",
        class1: "snake",
        class2: "",
        symbol: "S",
        color: "CLR_BLUE"
    },
    "plains centaur": {
        id: "plains centaur",
        class1: "centaur",
        class2: "",
        symbol: "C",
        color: "CLR_BROWN"
    },
    "pony": {
        id: "pony",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_BROWN"
    },
    "priest": {
        id: "priest",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "priestess": {
        id: "priestess",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "prisoner": {
        id: "prisoner",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "purple worm": {
        id: "purple worm",
        class1: "worm",
        class2: "",
        symbol: "w",
        color: "CLR_MAGENTA"
    },
    "pyrolisk": {
        id: "pyrolisk",
        class1: "cockatrice",
        class2: "",
        symbol: "c",
        color: "CLR_RED"
    },
    "python": {
        id: "python",
        class1: "snake",
        class2: "",
        symbol: "S",
        color: "CLR_MAGENTA"
    },
    "quantum mechanic": {
        id: "quantum mechanic",
        class1: "quantum mechanic",
        class2: "",
        symbol: "Q",
        color: "CLR_CYAN"
    },
    "quasit": {
        id: "quasit",
        class1: "imp",
        class2: "minor demon",
        symbol: "i",
        color: "CLR_BLUE"
    },
    "queen bee": {
        id: "queen bee",
        class1: "ant",
        class2: "other insect",
        symbol: "a",
        color: "HI_LORD"
    },
    "quivering blob": {
        id: "quivering blob",
        class1: "blob",
        class2: "",
        symbol: "b",
        color: "CLR_WHITE"
    },
    "rabid rat": {
        id: "rabid rat",
        class1: "rodent",
        class2: "",
        symbol: "r",
        color: "CLR_BROWN"
    },
    "ranger": {
        id: "ranger",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "raven": {
        id: "raven",
        class1: "bat",
        class2: "bird",
        symbol: "B",
        color: "CLR_BLACK"
    },
    "red dragon": {
        id: "red dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_RED"
    },
    "red mold": {
        id: "red mold",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_RED"
    },
    "red naga": {
        id: "red naga",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "CLR_RED"
    },
    "red naga hatchling": {
        id: "red naga hatchling",
        class1: "naga",
        class2: "",
        symbol: "N",
        color: "CLR_RED"
    },
    "rock mole": {
        id: "rock mole",
        class1: "rodent",
        class2: "",
        symbol: "r",
        color: "CLR_GRAY"
    },
    "rock piercer": {
        id: "rock piercer",
        class1: "piercer",
        class2: "",
        symbol: "p",
        color: "CLR_GRAY"
    },
    "rock troll": {
        id: "rock troll",
        class1: "troll",
        class2: "",
        symbol: "T",
        color: "CLR_CYAN"
    },
    "rogue": {
        id: "rogue",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "rope golem": {
        id: "rope golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "CLR_BROWN"
    },
    "roshi": {
        id: "roshi",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "rothe": {
        id: "rothe",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_BROWN"
    },
    "rust monster": {
        id: "rust monster",
        class1: "rust monster",
        class2: "disenchanter",
        symbol: "R",
        color: "CLR_BROWN"
    },
    "salamander": {
        id: "salamander",
        class1: "lizard",
        class2: "",
        symbol: ":",
        color: "CLR_ORANGE"
    },
    "samurai": {
        id: "samurai",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "sandestin": {
        id: "sandestin",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GRAY"
    },
    "sasquatch": {
        id: "sasquatch",
        class1: "apelike creature",
        class2: "",
        symbol: "Y",
        color: "CLR_GRAY"
    },
    "scorpion": {
        id: "scorpion",
        class1: "arachnid",
        class2: "centipede",
        symbol: "s",
        color: "CLR_RED"
    },
    "scorpius": {
        id: "Scorpius",
        class1: "arachnid",
        class2: "centipede",
        symbol: "s",
        color: "HI_LORD"
    },
    "sergeant": {
        id: "sergeant",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_RED"
    },
    "sewer rat": {
        id: "sewer rat",
        class1: "rodent",
        class2: "",
        symbol: "r",
        color: "CLR_BROWN"
    },
    "shade": {
        id: "shade",
        class1: "ghost",
        class2: "",
        symbol: " ",
        color: "CLR_BLACK"
    },
    "shaman karnov": {
        id: "Shaman Karnov",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "shark": {
        id: "shark",
        class1: "sea monster",
        class2: "",
        symbol: ";",
        color: "CLR_GRAY"
    },
    "shimmering dragon": {
        id: "shimmering dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_CYAN"
    },
    "shocking sphere": {
        id: "shocking sphere",
        class1: "eye",
        class2: "sphere",
        symbol: "e",
        color: "HI_ZAP"
    },
    "shopkeeper": {
        id: "shopkeeper",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "shrieker": {
        id: "shrieker",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_MAGENTA"
    },
    "silver dragon": {
        id: "silver dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "DRAGON_SILVER"
    },
    "skeleton": {
        id: "skeleton",
        class1: "zombie",
        class2: "",
        symbol: "Z",
        color: "CLR_WHITE"
    },
    "small mimic": {
        id: "small mimic",
        class1: "mimic",
        class2: "",
        symbol: "m",
        color: "CLR_BROWN"
    },
    "snake": {
        id: "snake",
        class1: "snake",
        class2: "",
        symbol: "S",
        color: "CLR_BROWN"
    },
    "soldier": {
        id: "soldier",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GRAY"
    },
    "soldier ant": {
        id: "soldier ant",
        class1: "ant",
        class2: "other insect",
        symbol: "a",
        color: "CLR_BLUE"
    },
    "spotted jelly": {
        id: "spotted jelly",
        class1: "jelly",
        class2: "",
        symbol: "j",
        color: "CLR_GREEN"
    },
    "stalker": {
        id: "stalker",
        class1: "elemental",
        class2: "",
        symbol: "E",
        color: "CLR_WHITE"
    },
    "steam vortex": {
        id: "steam vortex",
        class1: "vortex",
        class2: "",
        symbol: "v",
        color: "CLR_BLUE"
    },
    "stone giant": {
        id: "stone giant",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_GRAY"
    },
    "stone golem": {
        id: "stone golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "CLR_GRAY"
    },
    "storm giant": {
        id: "storm giant",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_BLUE"
    },
    "straw golem": {
        id: "straw golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "CLR_YELLOW"
    },
    "student": {
        id: "student",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "succubus": {
        id: "succubus",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GRAY"
    },
    "tengu": {
        id: "tengu",
        class1: "imp",
        class2: "minor demon",
        symbol: "i",
        color: "CLR_CYAN"
    },
    "thoth amon": {
        id: "Thoth Amon",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "thug": {
        id: "thug",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "tiger": {
        id: "tiger",
        class1: "cat",
        class2: "other feline",
        symbol: "f",
        color: "CLR_YELLOW"
    },
    "titan": {
        id: "titan",
        class1: "giant humanoid",
        class2: "",
        symbol: "H",
        color: "CLR_MAGENTA"
    },
    "titanothere": {
        id: "titanothere",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_GRAY"
    },
    "tourist": {
        id: "tourist",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "trapper": {
        id: "trapper",
        class1: "trapper",
        class2: "lurker above",
        symbol: "t",
        color: "CLR_GREEN"
    },
    "troll": {
        id: "troll",
        class1: "troll",
        class2: "",
        symbol: "T",
        color: "CLR_BROWN"
    },
    "twoflower": {
        id: "Twoflower",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "umber hulk": {
        id: "umber hulk",
        class1: "umber hulk",
        class2: "",
        symbol: "U",
        color: "CLR_BROWN"
    },
    "uruk-hai": {
        id: "Uruk-hai",
        class1: "orc",
        class2: "",
        symbol: "o",
        color: "CLR_BLACK"
    },
    "valkyrie": {
        id: "valkyrie",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "vampire": {
        id: "vampire",
        class1: "vampire",
        class2: "",
        symbol: "V",
        color: "CLR_RED"
    },
    "vampire bat": {
        id: "vampire bat",
        class1: "bat",
        class2: "bird",
        symbol: "B",
        color: "CLR_BLACK"
    },
    "vampire lady": {
        id: "vampire lady",
        class1: "vampire",
        class2: "",
        symbol: "V",
        color: "CLR_BLUE"
    },
    "vampire leader": {
        id: "vampire leader",
        class1: "vampire",
        class2: "",
        symbol: "V",
        color: "CLR_BLUE"
    },
    "vampire lord": {
        id: "vampire lord",
        class1: "vampire",
        class2: "",
        symbol: "V",
        color: "CLR_BLUE"
    },
    "vampire mage": {
        id: "vampire mage",
        class1: "vampire",
        class2: "",
        symbol: "V",
        color: "HI_ZAP"
    },
    "violet fungus": {
        id: "violet fungus",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_MAGENTA"
    },
    "vlad the impaler": {
        id: "Vlad the Impaler",
        class1: "vampire",
        class2: "",
        symbol: "V",
        color: "HI_LORD"
    },
    "vorpal jabberwock": {
        id: "vorpal jabberwock",
        class1: "jabberwock",
        class2: "",
        symbol: "J",
        color: "HI_LORD"
    },
    "vrock": {
        id: "vrock",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_GREEN"
    },
    "warg": {
        id: "warg",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_BLACK"
    },
    "warhorse": {
        id: "warhorse",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_BROWN"
    },
    "warrior": {
        id: "warrior",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "watch captain": {
        id: "watch captain",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GREEN"
    },
    "watchman": {
        id: "watchman",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GRAY"
    },
    "water demon": {
        id: "water demon",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "CLR_BLUE"
    },
    "water elemental": {
        id: "water elemental",
        class1: "elemental",
        class2: "",
        symbol: "E",
        color: "CLR_BLUE"
    },
    "water moccasin": {
        id: "water moccasin",
        class1: "snake",
        class2: "",
        symbol: "S",
        color: "CLR_RED"
    },
    "water nymph": {
        id: "water nymph",
        class1: "nymph",
        class2: "",
        symbol: "n",
        color: "CLR_BLUE"
    },
    "water troll": {
        id: "water troll",
        class1: "troll",
        class2: "",
        symbol: "T",
        color: "CLR_BLUE"
    },
    "werejackal": {
        id: "werejackal",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_RED"
    },
    "wererat": {
        id: "wererat",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_BROWN"
    },
    "werewolf": {
        id: "werewolf",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_ORANGE"
    },
    "white dragon": {
        id: "white dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_WHITE"
    },
    "white unicorn": {
        id: "white unicorn",
        class1: "unicorn",
        class2: "horse",
        symbol: "u",
        color: "CLR_WHITE"
    },
    "winged gargoyle": {
        id: "winged gargoyle",
        class1: "gremlin",
        class2: "",
        symbol: "g",
        color: "HI_LORD"
    },
    "winter wolf": {
        id: "winter wolf",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_CYAN"
    },
    "winter wolf cub": {
        id: "winter wolf cub",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_CYAN"
    },
    "wizard": {
        id: "wizard",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_DOMESTIC"
    },
    "wizard of yendor": {
        id: "Wizard of Yendor",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_OVERLORD"
    },
    "wolf": {
        id: "wolf",
        class1: "dog",
        class2: "other canine",
        symbol: "d",
        color: "CLR_GRAY"
    },
    "wood golem": {
        id: "wood golem",
        class1: "golem",
        class2: "",
        symbol: "'",
        color: "HI_WOOD"
    },
    "wood nymph": {
        id: "wood nymph",
        class1: "nymph",
        class2: "",
        symbol: "n",
        color: "CLR_GREEN"
    },
    "woodchuck": {
        id: "woodchuck",
        class1: "rodent",
        class2: "",
        symbol: "r",
        color: "CLR_BROWN"
    },
    "woodland-elf": {
        id: "Woodland-elf",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "CLR_GREEN"
    },
    "wraith": {
        id: "wraith",
        class1: "wraith",
        class2: "",
        symbol: "W",
        color: "CLR_BLACK"
    },
    "wumpus": {
        id: "wumpus",
        class1: "quadruped",
        class2: "",
        symbol: "q",
        color: "CLR_CYAN"
    },
    "xan": {
        id: "xan",
        class1: "xan",
        class2: "other mythical/fantastic insect",
        symbol: "x",
        color: "CLR_RED"
    },
    "xorn": {
        id: "xorn",
        class1: "xorn",
        class2: "",
        symbol: "X",
        color: "CLR_BROWN"
    },
    "yeenoghu": {
        id: "Yeenoghu",
        class1: "major demon",
        class2: "",
        symbol: "&",
        color: "HI_LORD"
    },
    "yellow dragon": {
        id: "yellow dragon",
        class1: "dragon",
        class2: "",
        symbol: "D",
        color: "CLR_YELLOW"
    },
    "yellow light": {
        id: "yellow light",
        class1: "light",
        class2: "",
        symbol: "y",
        color: "CLR_YELLOW"
    },
    "yellow mold": {
        id: "yellow mold",
        class1: "fungus",
        class2: "mold",
        symbol: "F",
        color: "CLR_YELLOW"
    },
    "yeti": {
        id: "yeti",
        class1: "apelike creature",
        class2: "",
        symbol: "Y",
        color: "CLR_WHITE"
    },
    "zenodotus": {
        id: "Zenodotus",
        class1: "human",
        class2: "elf",
        symbol: "@",
        color: "HI_LORD"
    },
    "zruty": {
        id: "zruty",
        class1: "zruty",
        class2: "",
        symbol: "z",
        color: "CLR_BROWN"
    },
};

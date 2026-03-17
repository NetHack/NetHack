# NetHack 3.7 Coding Documentation

This document provides a comprehensive overview of the NetHack codebase architecture, coding patterns, and development conventions.

## Table of Contents
1. [Project Overview](#project-overview)
2. [Directory Structure](#directory-structure)
3. [Core Architecture](#core-architecture)
4. [Key Data Structures](#key-data-structures)
5. [Global State Management](#global-state-management)
6. [Game Loop](#game-loop)
7. [Coding Conventions](#coding-conventions)
8. [Header Files](#header-files)
9. [Monster System](#monster-system)
10. [Object System](#object-system)
11. [Dungeon System](#dungeon-system)
12. [Windowing Interface](#windowing-interface)
13. [Save/Restore System](#saverestore-system)
14. [Lua Integration](#lua-integration)

---

## Project Overview

NetHack 3.7 is a terminal-based dungeon exploration game descended from Rogue and Hack. It is written primarily in C with some C++ for platform-specific windowing libraries.

**Key Characteristics:**
- Cross-platform support (Unix, Windows, macOS, VMS, etc.)
- Turn-based gameplay with real-time elements
- Procedural dungeon generation
- Extensive item and monster interactions
- Complex status and attribute system

---

## Directory Structure

```
NetHack/
├── src/           # C source files (150+ files)
├── include/       # Header files
├── dat/           # Data files and Lua level definitions
├── doc/           # Documentation
├── DEVEL/         # Developer resources
├── sys/           # System-specific code
├── win/           # Window system implementations
├── util/          # Utility programs
├── test/          # Test code
└── submodules/    # External dependencies
```

### Key Source Files (`src/`)

| File | Purpose |
|------|---------|
| `allmain.c` | Main game loop and initialization |
| `decl.c` | Global variable definitions |
| `hack.c` | Core movement and interaction logic |
| `cmd.c` | Command processing and input handling |
| `mon.c` | Monster management |
| `makemon.c` | Monster creation |
| `monmove.c` | Monster movement AI |
| `obj.c` | Object operations |
| `mkobj.c` | Object creation |
| `invent.c` | Inventory management |
| `dungeon.c` | Dungeon level management |
| `mklev.c` | Level generation |
| `sp_lev.c` | Special level handling |
| `nhlua.c` | Lua scripting integration |
| `save.c` / `restore.c` | Game save/restore |
| `end.c` | Game ending handling |

---

## Core Architecture

### Game Architecture Overview

NetHack uses a modular architecture with clear separation of concerns:

1. **Engine Core**: Game logic, turn management, state updates
2. **Entity Systems**: Monsters, objects, and player character
3. **World System**: Dungeon levels, map data, level transitions
4. **Interface Layer**: Windowing abstraction, user input, display
5. **Persistence**: Save/restore, bones files, configuration

### Component Interaction

```
┌─────────────────────────────────────────┐
│           Window Interface              │
│    (TTY, X11, Qt, Curses, etc.)         │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│           Command System                │
│    (cmd.c, parse, rhack)                │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│           Game Engine                   │
│  ┌──────────┐ ┌──────────┐ ┌─────────┐  │
│  │  Player  │ │ Monsters │ │ Objects │  │
│  │  (you)   │ │  (mon)   │ │  (obj)  │  │
│  └──────────┘ └──────────┘ └─────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌─────────┐  │
│  │ Dungeon  │ │   Traps  │ │  Shops  │  │
│  │  (lev)   │ │  (trap)  │ │  (shk)  │  │
│  └──────────┘ └──────────┘ └─────────┘  │
└─────────────────────────────────────────┘
```

---

## Key Data Structures

### Player Character (`struct you` in `include/you.h`)

The player character state is stored in the global `u` variable:

```c
struct you {
    coordxy ux, uy;          // Current position
    d_level uz, uz0;         // Current and previous dungeon level
    int ulevel;              // Experience level (1-30)
    int uhp, uhpmax;         // Hit points
    int uen, uenmax;         // Magical energy
    struct attribs acurr;    // Current attributes (str, dex, etc.)
    struct attribs amax;     // Maximum attributes
    unsigned umoney0;        // Starting gold (for score)
    struct u_event uevent;   // Event flags
    struct u_have uhave;     // Special items being carried
    // ... many more fields
} u;
```

Key macros:
- `Upolyd` - True if polymorphed
- `Luck` - Combined luck value

### Monster (`struct monst` in `include/monst.h`)

```c
struct monst {
    struct monst *nmon;      // Next monster in chain
    struct permonst *data;   // Monster type definition
    unsigned m_id;           // Unique monster ID
    short mnum;              // Monster type index
    coordxy mx, my;          // Position
    int mhp, mhpmax;         // Hit points
    struct obj *minvent;     // Inventory
    struct obj *mw;          // Wielded weapon
    Bitfield(female, 1);     // Gender
    Bitfield(minvis, 1);     // Invisible
    Bitfield(mcan, 1);       // Cancelled
    Bitfield(mpeaceful, 1);  // Peaceful
    Bitfield(mtame, 7);      // Tame level
    // ... many more fields
};
```

### Object (`struct obj` in `include/obj.h`)

```c
struct obj {
    struct obj *nobj;        // Next object in list
    short otyp;              // Object type index
    unsigned owt;            // Weight
    long quan;               // Quantity
    schar spe;               // Special enchantment/charges
    char oclass;             // Object class
    char invlet;             // Inventory letter
    coordxy ox, oy;          // Position
    xint8 where;             // Location type
    Bitfield(cursed, 1);     // Cursed status
    Bitfield(blessed, 1);    // Blessed status
    // ... many more fields
};
```

---

## Global State Management

NetHack 3.7 uses a structured approach to global state with the `instance_globals_*` system defined in `include/decl.h`.

### Global Variable Organization

Global variables are organized into alphabetically-named structures:

```c
// Non-saved globals (reinitialized each game)
extern struct instance_globals_a ga;  // afternmv, at_ladder, etc.
extern struct instance_globals_b gb;  // bhitpos, billobjs, etc.
extern struct instance_globals_c gc;  // command_queue, Cmd, etc.
// ... through gz

// Saved globals (persisted in save files)
extern struct instance_globals_saved_b svb;  // branches, bases
extern struct instance_globals_saved_c svc;  // context
extern struct instance_globals_saved_d svd;  // dungeons, disco
// ... through svy
```

### Access Pattern

```c
// Example usage:
gb.bhitpos.x = x;           // Set ball/hit position
gc.Cmd.dirchars = sdir;     // Set direction characters
svc.context.move = 0;       // Set game context
```

### Purpose

This organization:
- Groups related globals together
- Facilitates save/restore operations
- Supports "play again" feature without full restart
- Makes dependencies explicit

---

## Game Loop

The main game loop is implemented in `src/allmain.c`.

### Core Loop Structure

```c
void moveloop(boolean resuming)
{
    moveloop_preamble(resuming);
    
    for (;;) {
        moveloop_core();
    }
}
```

### Loop Phases

1. **Pre-turn processing**:
   - Handle signals
   - Process pending commands
   - Sanity checks

2. **Monster Movement**:
   ```c
   do {
       monscanmove = movemon();
       if (u.umovement >= NORMAL_SPEED)
           break;
   } while (monscanmove);
   ```

3. **Turn increment**:
   - Increment `svm.moves`
   - Update timeouts
   - Run region effects
   - Handle HP/Pw regeneration
   - Check random events

4. **Player Input**:
   - Process commands via `rhack()`
   - Handle movement with `domove()`
   - Update display

5. **Post-turn processing**:
   - Vision recalculation
   - Status line updates
   - Inventory updates

### Movement Points

```c
// Hero's movement is controlled by u.umovement
u.umovement += moveamt;      // Add movement points
u.umovement -= NORMAL_SPEED; // Deduct for taking a turn
```

---

## Coding Conventions

### Code Style (from `DEVEL/code_style.txt`)

**Indentation:**
- 4 spaces, no tabs
- Max 78 characters per line

**Function Definitions:**
```c
void
function_name(int arg1, int arg2)
{
    /* body */
}

// Or with parameter comments:
void
long_function_name(int first_arg,                /* main operation */
                   struct long_name *second_arg, /* control details */
                   int third_arg)                /* local conditions */
{
    /* body */
}
```

**Control Statements:**
```c
if (condition) {
    /* body */
} else if (condition) {
    do {
        /* body */
    } while (condition);
} else {
    /* body */
}
```

**Static Functions:**
```c
// Use staticfn instead of static for functions
staticfn void helper_function(void);
```

### Naming Conventions

| Prefix/Suffix | Meaning |
|---------------|---------|
| `u.` | Player character state |
| `gy.youmonst` | Player as monster struct |
| `m_` | Monster-related function |
| `o_` | Object-related function |
| `do_` | Command handler function |
| `PM_` | Permonst (monster type) constant |
| `ART_` | Artifact constant |
| `SP_` | Spell constant |

### Common Macros

```c
// From hack.h
#define TRUE 1
#define FALSE 0
#define TELL 1
#define NOTELL 0
#define ON 1
#define OFF 0

// Common operations
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define plur(x) (((x) == 1) ? "" : "s")

// Random numbers
#define rn2(x) (random() % (x))
#define rn1(x, y) (rn2(x) + (y))
#define rnd(x) (rn2(x) + 1)
```

---

## Header Files

### Main Headers

| Header | Purpose |
|--------|---------|
| `hack.h` | Primary game header, includes most others |
| `decl.h` | Global variable declarations |
| `you.h` | Player character structure |
| `monst.h` | Monster structure |
| `obj.h` | Object structure |
| `dungeon.h` | Dungeon/level structures |
| `extern.h` | Function declarations |
| `config.h` | Build configuration |

### Include Order
```c
#include "hack.h"    // Usually sufficient for most files
```

`hack.h` includes:
- `config.h` - Build configuration
- `dungeon.h` - Level structures
- `objclass.h` - Object classes
- `flag.h` - Game flags
- `you.h` - Player structure
- `monst.h` - Monster structure
- And many others...

---

## Monster System

### Monster Type Definition (`struct permonst`)

Monster types are defined in `src/monst.c` using the `MON()` macro:

```c
MON("giant ant", S_ANT,                           // name, symbol
    LVL(2, 18, 3, 0, 0), (G_GENO | G_SGROUP | 3), // level, geno
    A(ATTK(AT_BITE, AD_PHYS, 1, 4),               // attacks
      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
    SIZ(10, 10, MS_SILENT, MZ_TINY),              // size
    MR_NONE, MC_NONE,                             // resistances
    M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS,        // flags 1
    M2_HOSTILE | M2_EGGS,                         // flags 2
    M3_INFRAVISIBLE,                              // flags 3
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          // flags 4-15
    NO_RACE, 0, 0, 0, 0, 0, 0,                    // racial flags
    1, CLR_BROWN)                                 // difficulty, color
```

### Monster Movement (`src/monmove.c`)

Monster AI handles:
- Hostile pursuit
- Fleeing behavior
- Special attacks
- Item usage
- Spell casting

Key function:
```c
int dog_move(struct monst *mtmp, struct edog *edog, int after, int udist);
```

---

## Object System

### Object Classes

Objects are organized into classes:
```c
#define WEAPON_CLASS    1
#define ARMOR_CLASS     2
#define RING_CLASS      3
#define AMULET_CLASS    4
#define TOOL_CLASS      5
#define FOOD_CLASS      6
#define POTION_CLASS    7
#define SCROLL_CLASS    8
#define SPBOOK_CLASS    9 /* Spellbooks */
#define WAND_CLASS     10
#define COIN_CLASS     11
#define GEM_CLASS      12
#define ROCK_CLASS     13
#define BALL_CLASS     14
#define CHAIN_CLASS    15
#define VENOM_CLASS    16
```

### Object Definition

Objects defined in `src/objects.c` using `OBJECT()` macro:

```c
OBJECT(OBJ("long sword", None),
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_SWORD, IRON),
       0, WEAPON_CLASS, 50, 0, 15, 40, 8, 12, 0, 0, WHACK, 0, 0),
```

### Common Object Operations

```c
// Create object
struct obj *mkobj(int oclass, boolean artif);
struct obj *mksobj(int otyp, boolean init, boolean artif);

// Object properties
boolean is_wielded(struct obj *obj);
boolean is_worn(struct obj *obj);
int weight(struct obj *obj);

// Inventory operations
struct obj *addinv(struct obj *obj);
void freeinv(struct obj *obj);
struct obj *carrying(int type);
```

---

## Dungeon System

### Level Structure

Each dungeon level consists of:
- Map grid (`struct rm levl[COLNO][ROWNO]`)
- Monsters (`fmon` linked list)
- Objects (`fobj` linked list)
- Traps (`gf.ftrap` linked list)
- Engravings
- Rooms and corridors

### Special Levels

Special levels are defined in `dat/*.lua` files (Lua scripting):
- `castle.lua` - Castle level
- `medusa-1.lua` - Medusa level
- `tower1.lua` - Wizard's tower
- `soko*.lua` - Sokoban puzzles
- `quest.lua` - Quest levels

### Level Transitions

```c
// Go to next level
void next_level(boolean at_stairs);

// Go to previous level  
void prev_level(boolean at_stairs);

// Teleport to specific level
void goto_level(d_level *newlevel, boolean at_stairs, 
                boolean falling, boolean portal);
```

---

## Windowing Interface

### Window Abstraction

NetHack abstracts the display through the window port system:

```c
// Window types
#define NHW_MESSAGE 1
#define NHW_STATUS  2
#define NHW_MAP     3
#define NHW_MENU    4
#define NHW_TEXT    5
```

### Key Functions

```c
// Window operations
winid create_nhwindow(int type);
void destroy_nhwindow(winid window);
void display_nhwindow(winid window, boolean blocking);
void clear_nhwindow(winid window);

// Output
void putstr(winid window, int attr, const char *str);
void print_glyph(winid window, coordxy x, coordxy y, int glyph);

// Input
int nhgetch(void);
char *nh_getlin(const char *query);
int yn_function(const char *query, const char *resp, char def);
```

### Supported Interfaces

- **TTY** - Terminal/console (`win/tty/`)
- **X11** - X Window System (`win/X11/`)
- **Qt** - Qt GUI (`win/Qt/`)
- **Curses** - Curses library (`win/curses/`)

---

## Save/Restore System

### Save File Structure

Save files use a structured binary format with field-level saving:

```c
struct nh_file {
    int mode;              // READING, WRITING, FREEING
    int ftype;             // NHF_LEVELFILE, NHF_SAVEFILE, NHF_BONESFILE
    boolean structlevel;   // Traditional struct saves
    boolean fieldlevel;    // Field-by-field saves
    FILE *fpdef;           // File pointer
    // ...
};
```

### Save Operations

```c
// Save game
void savegamestate(void);
void savelev(int lev, xint8 levtyp);

// Restore game
void restore_saved_game(void);
void getlev(int fd, int lev, xint8 levtyp);

// Bones files (ghost levels)
void savebones(int how, time_t when, struct obj *corpse);
int getbones(void);
```

---

## Lua Integration

NetHack 3.7 uses Lua for level definitions and scripting:

### Lua State

```c
// Global Lua state
extern genericptr_t gl.luacore;  // lua_State *

// Core callbacks
enum nhcore_calls {
    NHCORE_START_NEW_GAME = 0,
    NHCORE_RESTORE_OLD_GAME,
    NHCORE_MOVELOOP_TURN,
    NHCORE_GAME_EXIT,
    NHCORE_GETPOS_TIP,
    NHCORE_ENTER_TUTORIAL,
    NHCORE_LEAVE_TUTORIAL,
    NUM_NHCORE_CALLS
};
```

### Level Files

Level definitions in `dat/*.lua`:
- Room placement
- Object generation
- Monster spawning
- Trap placement
- Special features

Example structure:
```lua
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "hardfloor")
des.map([[
    -----------
    |.........|
    |.........|
    -----------
]])
des.region({ region = {1,1, 9,3}, type = "ordinary" })
```

---

## Common Development Tasks

### Adding a New Monster

1. Add to `src/monst.c` using `MON()` macro
2. Define any special attacks in `include/monattk.h`
3. Add to appropriate dungeon generation in Lua files
4. Update racial flags if needed

### Adding a New Object

1. Add to `src/objects.c` using `OBJECT()` macro
2. Define object class in `include/objclass.h` if new type
3. Add any special handling code
4. Update discovery list if appropriate

### Adding a New Command

1. Define handler function in appropriate file
2. Add to `cmdlist[]` in `src/cmd.c`
3. Add to extended command list if needed
4. Implement help text

---

## Important Notes

### Memory Management

- Use `alloc()` / `free()` from `src/alloc.c`
- Object chains use `nobj`/`nexthere` pointers
- Monster chains use `nmon` pointer
- Always NULL pointers after freeing

### Random Numbers

- Use `rn2(n)` for 0 to n-1
- Use `rnd(n)` for 1 to n
- Use `rn1(x, y)` for y to y+x-1

### Coordinates

- `coordxy` type for map coordinates
- `isok(x, y)` to validate coordinates
- `u.ux, u.uy` for player position

### Boolean Values

```c
typedef signed char boolean;  // 0 = FALSE, 1 = TRUE
#define TRUE  1
#define FALSE 0
```

---

## References

- Main header: `include/hack.h`
- Global declarations: `include/decl.h`
- Coding style: `DEVEL/code_style.txt`
- Developer docs: `DEVEL/Developer.txt`

---

*Document generated for NetHack 3.7 development*
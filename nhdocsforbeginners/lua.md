# Lua in NetHack - Technical Documentation

## Overview

NetHack 3.7 uses **Lua 5.4** as an embedded scripting language for level generation and game configuration. This document explains how Lua integrates with NetHack.

---

## What Lua Files Are Used For

Lua files in the `dat/` directory serve multiple purposes:

### 1. Special Level Definitions
Files like `air.lua`, `castle.lua`, `medusa-1.lua`, `soko1-1.lua` define:
- Map layouts using `des.map()`
- Monster placement with `des.monster()`
- Object placement with `des.object()`
- Traps, portals, stairs with `des.trap()`, `des.portal()`
- Level flags (mazelevel, noteleport, hardfloor, etc.)

### 2. Quest Levels
Files like `Arc-goal.lua`, `Bar-strt.lua`, etc. define class-specific quest:
- Starting locations (`???-strt.lua`)
- Goal levels (`???-goal.lua`)
- Filament levels (`???-fila.lua`, `???-filb.lua`)
- Location levels (`???-loca.lua`)

### 3. Dungeon Structure
- `dungeon.lua` - Defines how the dungeon branches connect
- `quest.lua` - Quest text (replaced old `quest.txt`)

### 4. Helper Library
- `nhlib.lua` - Provides utility functions like `shuffle()`, `d()` (dice rolls), `percent()`
- `nhcore.lua` - Core game callbacks

### 5. Endgame Levels
- `air.lua`, `earth.lua`, `fire.lua`, `water.lua` - The four elemental planes
- `astral.lua` - The final Astral Plane

---

## How Lua Files Are "Compiled"

**Lua files in NetHack are NOT compiled to bytecode.** Instead, they are:

1. **Stored as source text** - The Lua scripts remain as human-readable text
2. **Packaged into an archive** - They are bundled using the **`dlb`** (Data Librarian) tool into a file called **`nhdat`** (NetHack Data Archive)

The process works like this:
```bash
# dlb reads a list of files (dlb.lst) and creates nhdat archive
dlb cvIf dlb.lst nhdat
```

The `dlb.lst` file includes all Lua files:
```
data
oracles
rumors
help
...
*.lua    # <-- All Lua files get included
```

### Output
The Lua files become part of the **`nhdat`** archive file, which contains:
- **Lua level scripts** (`.lua` files)
- **Game data** (`data`, `oracles`, `rumors`)
- **Help files** (`help`, `hh`, `cmdhelp`, etc.)
- **Text databases** (`engrave`, `epitaph`, `bogusmon`, `tribute`)

The `nhdat` file is a custom archive format (similar to `tar`) that NetHack reads at runtime.

---

## The Lua Integration Architecture

### Big Picture
NetHack **embeds** the Lua interpreter. This means the Lua engine runs *inside* the game, not as a separate program. NetHack calls Lua, and Lua calls back into NetHack.

### Two-Way Communication Flow

```
┌─────────────────┐
│   NetHack C     │  ← Main game engine
│   Code          │
└────────┬────────┘
         │ calls
         ▼
┌─────────────────┐
│  Lua Engine     │  ← Official Lua interpreter (from lua.org)
│  (embedded)     │
└────────┬────────┘
         │ executes
         ▼
┌─────────────────┐
│  Lua Scripts    │  ← Your level files (castle.lua, air.lua, etc.)
│  (in nhdat)     │
└────────┬────────┘
         │ calls back
         ▼
┌─────────────────┐
│  C Functions    │  ← nhlua.c registers these (nh.*, des.* functions)
│  (nhlua.c)      │    that modify the game world
└─────────────────┘
```

---

## Runtime Execution

### What Happens When You Enter a Special Level

Let's say you go down the stairs to the **Castle** level:

```
Player enters Castle → NetHack needs to create the level
         ↓
    nhl_loadlua(L, "castle.lua")  // Load Lua script from nhdat archive
         ↓
    luaL_loadbuffer(L, script)    // Parse the Lua code
         ↓
    lua_pcall(L, ...)             // EXECUTE the script
         ↓
    Lua script runs:
        des.map([[...]])          → Calls C function that draws the map
        des.monster("D")          → Calls C function that spawns a dragon
        des.object("chest")       → Calls C function that places a chest
```

### Real Example from `air.lua`:
```lua
des.level_init({ style = "solidfill", fg = " " });
des.map([[AAAAAAAAAA...]]);     -- C function draws 'A' tiles (air)
des.monster({ id = "air elemental", peaceful = 0 })  -- Spawns monster
```

When `des.monster()` runs, it actually calls a **C function** registered by NetHack that:
1. Parses the monster ID
2. Creates the monster struct
3. Places it on the map

---

## The Role of `nhlua.c`

### Is it a complete Lua interpreter?
**No.** `nhlua.c` is NOT the Lua interpreter itself. It is a **bridge/glue layer** written by the NetHack team that integrates the official Lua library into the game.

### Who wrote it?
- **Lua interpreter**: Written by the Lua team at PUC-Rio (Brazil), downloaded from lua.org (version 5.4.8)
- **`nhlua.c`**: Written by **Pasi Kallinen** (NetHack team) - see the copyright notice at the top of the file

### What `nhlua.c` actually does:

#### 1. Initializes Lua with Sandboxing (`nhl_init`, `nhlL_newstate`)
- Sets up memory limits and instruction counting (security)
- Controls which Lua libraries are available (base, string, table, math, etc.)

#### 2. Registers NetHack-specific functions in the `nh` table:
```c
static const struct luaL_Reg nhl_functions[] = {
    { "pline", nhl_pline },        // Print message
    { "rn2", nhl_rn2 },            // Random number
    { "getmap", nhl_getmap },      // Get map tile info
    { "menu", nhl_menu },          // Show menu
    { "stairways", nhl_stairways },// Get stair locations
    { "variable", nhl_variable },  // Game variables
    ...
};
```

#### 3. Integrates with NetHack's data system (`nhl_loadlua`)
- Loads Lua scripts from the `nhdat` archive using `dlb_fopen`
- Not from regular files - from the custom DLB archive

#### 4. Registers the `des.*` functions (via `l_register_des`)
- `des.map()`, `des.monster()`, `des.object()`, `des.trap()`, etc.
- These are defined in other files (`src/nhlsel.c`, `src/sp_lev.c`)

#### 5. Provides access to game state via the `u` table:
- `u.ux`, `u.uy` - player position
- `u.uhp`, `u.uen` - HP and energy
- `u.role`, `u.inventory`

#### 6. Implements sandboxing/security (`NHL_SANDBOX`)
- Removes dangerous Lua functions (`dofile`, `loadfile`, `os.execute`)
- Limits memory usage and instruction count
- Prevents infinite loops and excessive memory consumption

### Where the actual Lua comes from:
- Downloaded from: `http://www.lua.org/ftp/lua-5.4.8.tar.gz`
- Or git submodule: `https://github.com/lua/lua.git`
- Located in: `submodules/lua/` or `lib/lua-5.4.8/`
- Compiled as a static library (`lua54s.a` or `lua$(LUAVER)-static.lib`)

---

## Sandboxing Explained

When NetHack starts up, `nhl_init()` creates a Lua state with security restrictions:

| Feature | Why It Matters |
|---------|----------------|
| **Memory limit** (1MB) | Prevents Lua script from crashing the game with out-of-memory |
| **Instruction limit** | Prevents infinite loops in Lua scripts |
| **Removed functions** | `os.execute()`, `dofile()`, `loadfile()` are deleted - scripts can't run external programs or read arbitrary files |
| **DLB only** | Scripts can only be loaded from the `nhdat` archive, not from disk |

This is like a "walled garden" - Lua scripts can only do what NetHack explicitly allows.

---

## Summary

- **Lua files are data** (level blueprints), not the game engine itself
- They are **stored as text** in the `nhdat` archive, not compiled
- **NetHack calls Lua** when it needs to generate special levels
- **Lua calls C functions** (registered by `nhlua.c`) to actually build the level
- **`nhlua.c`** is a ~1700 line glue layer that creates a controlled, sandboxed Lua environment
- **Sandboxing** restricts what Lua can do for security and stability
- The actual Lua engine is the standard, unmodified official Lua interpreter from lua.org

---

## File Locations

| File | Purpose |
|------|---------|
| `dat/*.lua` | Level definition scripts |
| `dat/nhlib.lua` | Lua utility library |
| `dat/nhcore.lua` | Core game callbacks |
| `src/nhlua.c` | NetHack-Lua integration (glue code) |
| `src/nhlsel.c` | Selection and map functions |
| `src/nhlobj.c` | Object manipulation functions |
| `submodules/lua/` | Official Lua interpreter source |
| `dat/nhdat` | Archive containing all Lua scripts (generated at build time) |

---

## See Also

- `doc/dlb.txt` - Documentation for the Data Librarian tool
- `doc/makedefs.txt` - Build-time tool documentation
- `dat/nhlib.lua` - Example of NetHack-specific Lua functions
- `dat/air.lua` - Example level definition

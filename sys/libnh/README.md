# NetHack 3.6 WASM Library

A WebAssembly build of NetHack 3.6.7 that exposes all window system operations through a single JavaScript callback. Your JavaScript code receives rendering events (draw map, show menu, get input) and controls the game by returning values from the callback.

## Quick Start

```bash
# Prerequisites: Emscripten SDK (https://emscripten.org/docs/getting_started/downloads.html)

# From the repository root:
./build-wasm.sh

# Run the test:
node sys/libnh/test/test.mjs
```

Build outputs:
- `src/nethack.js` (~93KB) — ES6 module factory
- `src/nethack.wasm` (~4.8MB) — Game binary with embedded data files

## Minimal Example

```js
import createModule from "./nethack.js";

const Module = await createModule({ noInitialRun: true });

// 1. Install pointer helpers (required by the callback bridge)
globalThis.nethackGlobal = { helpers: {
    getPointerValue: (name, ptr, type) => {
        switch (type) {
            case "i": case "2": return Module.getValue(ptr, "i32");
            case "s": return Module.UTF8ToString(ptr);
            case "b": return Module.getValue(ptr, "i8") !== 0;
            case "c": return String.fromCharCode(Module.getValue(ptr, "i8"));
            case "0": return Module.getValue(ptr, "i8");
            case "1": return Module.getValue(ptr, "i16");
            case "p": return ptr;
            default:  return ptr;
        }
    },
    setPointerValue: (name, ptr, type, val) => {
        if (!ptr) return;
        switch (type) {
            case "i": case "2": Module.setValue(ptr, val | 0, "i32"); break;
            case "b": Module.setValue(ptr, val ? 1 : 0, "i8"); break;
            case "c": Module.setValue(ptr, typeof val === "string"
                          ? val.charCodeAt(0) : (val | 0), "i8"); break;
            case "0": Module.setValue(ptr, val | 0, "i8"); break;
            case "1": Module.setValue(ptr, val | 0, "i16"); break;
            case "v": break;
            default:  Module.setValue(ptr, val | 0, "i32"); break;
        }
    },
}};

// 2. Register your callback
globalThis.nethackCallback = async (name, ...args) => {
    switch (name) {
        case "shim_create_nhwindow": return 1;  // return a window ID
        case "shim_nhgetch":         return 32;  // return a keycode
        case "shim_yn_function":     return 121; // 'y'
        case "shim_putstr":
            console.log(args[2]); // args = [winid, attr, text]
            return;
        // ... handle other callbacks
        default: return 0;
    }
};

// 3. Set the callback name and start the game
const setCallback = Module.cwrap("shim_graphics_set_callback", null, ["string"]);
setCallback("nethackCallback");
Module._main(0, 0);
```

## Build

The build uses a three-phase process because NetHack 3.6 requires native utilities (`makedefs`, `lev_comp`, `dgn_comp`) to generate data files, but the game itself must be compiled to WASM.

```bash
./build-wasm.sh
```

The script handles all three phases:
1. **Phase 1** — Build native utilities with `cc`, using WASM hints for consistent `-D` flags
2. **Phase 2** — Copy generated data files into `wasm-data/` for embedding
3. **Phase 3** — Build the game with `emcc`, embedding data files into the WASM binary

See `nethack-3.6.7-wasm-backport.md` in the repository root for the full build system documentation.

## API

### Module Loading

The build produces an ES6 module factory. Load it and create an instance:

```js
import createModule from "./nethack.js";

const Module = await createModule({
    noInitialRun: true,  // required — we call _main() manually after setup
    print: (text) => {}, // optional stdout handler
    printErr: (text) => {}, // optional stderr handler
});
```

The `Module` object provides Emscripten runtime methods:

| Method | Purpose |
|--------|---------|
| `Module.cwrap(name, ret, args)` | Wrap a C function for calling from JS |
| `Module.getValue(ptr, type)` | Read a value from WASM memory |
| `Module.setValue(ptr, val, type)` | Write a value to WASM memory |
| `Module.UTF8ToString(ptr)` | Read a C string from WASM memory |
| `Module.stringToUTF8(str, ptr, len)` | Write a JS string to WASM memory |
| `Module.addFunction(fn, sig)` | Register a JS function as a C callback |
| `Module.FS` | Emscripten virtual filesystem |

### Exported C Functions

| Function | Description |
|----------|-------------|
| `_main(argc, argv)` | Start the game. Call with `(0, 0)` for defaults. |
| `shim_graphics_set_callback(name)` | Register a callback by name. Must be called before `_main`. |
| `_glyph_to_tile(glyph)` | Return the tile index for a glyph. See [Tile System](#tile-system). |
| `_malloc(size)` | Allocate WASM heap memory (for advanced use). |

### Pointer Helpers

Before calling `_main`, you must install two helper functions on `globalThis.nethackGlobal.helpers`. The C-to-JS bridge (`local_callback` in `winshim.c`) calls these to convert between WASM memory pointers and JavaScript values:

- **`getPointerValue(name, ptr, type)`** — Read a value from a WASM pointer. Called once per callback argument.
- **`setPointerValue(name, ptr, type, val)`** — Write a return value to a WASM pointer. Called once per callback return.

Type codes (shared with the format string system):

| Code | C Type | JS Read (`getValue`) | JS Write (`setValue`) |
|------|--------|---------------------|----------------------|
| `i` | `int` (4 bytes) | `"i32"` | `"i32"` |
| `s` | `char *` (string) | `UTF8ToString(ptr)` | N/A (strings are read-only) |
| `p` | `void *` (pointer) | raw pointer value | `"i32"` |
| `b` | `boolean` (1 byte) | `"i8"`, convert to bool | `"i8"`, 1 or 0 |
| `c` | `char` (1 byte) | `"i8"`, convert to char | `"i8"`, charCode |
| `0` | 1-byte int | `"i8"` | `"i8"` |
| `1` | 2-byte int | `"i16"` | `"i16"` |
| `2` | 4-byte int | `"i32"` | `"i32"` |
| `v` | `void` | N/A | no-op |

### The Callback

All game-to-UI communication goes through a single async callback:

```js
globalThis.yourCallbackName = async (name, ...args) => {
    // name: string — which window function is being called
    // args: mixed[] — arguments already decoded by getPointerValue
    // return: the value expected by the game (decoded by setPointerValue)
};
```

Register it before calling `_main`:

```js
const setCallback = Module.cwrap("shim_graphics_set_callback", null, ["string"]);
setCallback("yourCallbackName");
Module._main(0, 0);  // starts the game; callbacks begin firing via Asyncify
```

### Callback Reference

Each callback corresponds to a function in NetHack's `window_procs` interface. The format string column shows the return type (first character) followed by parameter types.

#### Lifecycle

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_init_nhwindows` | `vpp` | `(argcp, argv)` | void | Initialize the window system |
| `shim_player_selection` | `v` | `()` | void | Select character role/race/etc. Set `flags.initrole` et al. via exported globals, or leave unset for random |
| `shim_askname` | `v` | `()` | void | Prompt for player name. Write to exported `plname` global |
| `shim_exit_nhwindows` | `vs` | `(str)` | void | Shut down the window system |
| `shim_start_screen` | `v` | `()` | void | Called before first output (3.6 only) |
| `shim_end_screen` | `v` | `()` | void | Called after last output (3.6 only) |

#### Windows

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_create_nhwindow` | `ii` | `(type)` | `winid` | Create a window. Types: 1=MESSAGE, 2=STATUS, 3=MAP, 4=MENU, 5=TEXT |
| `shim_clear_nhwindow` | `vi` | `(winid)` | void | Clear window contents |
| `shim_display_nhwindow` | `vib` | `(winid, blocking)` | void | Display/refresh a window |
| `shim_destroy_nhwindow` | `vi` | `(winid)` | void | Destroy a window |
| `shim_curs` | `viii` | `(winid, x, y)` | void | Position the cursor |

#### Text Output

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_putstr` | `viis` | `(winid, attr, str)` | void | Write a string to a window |
| `shim_raw_print` | `vs` | `(str)` | void | Print a string (no window context) |
| `shim_raw_print_bold` | `vs` | `(str)` | void | Print a bold string |
| `shim_display_file` | `vsb` | `(name, complain)` | void | Display a text file (e.g., NEWS) |

#### Map

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_print_glyph` | `vi00ii` | `(winid, x, y, glyph, bkglyph)` | void | Draw a glyph on the map. `x`/`y` are 1-byte ints. See [Glyph System](#glyph-system) |
| `shim_cliparound` | `vii` | `(x, y)` | void | Center the viewport around a position |

#### Input

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_nhgetch` | `i` | `()` | `int` (keycode) | Get a single keypress |
| `shim_nh_poskey` | `ippp` | `(x_ptr, y_ptr, mod_ptr)` | `int` (keycode) | Get a keypress or mouse click. Write x/y/mod to pointers for mouse events |
| `shim_yn_function` | `css0` | `(query, resp, def)` | `char` (charcode) | Ask a yes/no/other question |
| `shim_getlin` | `vsp` | `(query, bufp)` | void | Get a line of text input. Write response to `bufp` |
| `shim_get_ext_cmd` | `iv` | `()` | `int` | Get an extended command index. Return -1 for none |

#### Menus

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_start_menu` | `vi` | `(winid)` | void | Begin building a menu |
| `shim_add_menu` | `viip00isb` | `(winid, glyph, identifier, ch, gch, attr, str, preselected)` | void | Add a menu item |
| `shim_end_menu` | `vis` | `(winid, prompt)` | void | Finish building a menu |
| `shim_select_menu` | `iiip` | `(winid, how, menu_list_ptr)` | `int` (count) | Display menu and get selection. `how`: 0=PICK_NONE, 1=PICK_ONE, 2=PICK_ANY |
| `shim_message_menu` | `ciis` | `(let, how, mesg)` | `char` | Display a message with menu-style selection |

#### Status

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_status_init` | `v` | `()` | void | Initialize the status display |
| `shim_status_update` | `vipiiip` | `(fldidx, ptr, chg, percent, color, colormasks)` | void | Update a status field. See [Status Fields](#status-fields) |

#### Miscellaneous

| Callback | Format | Args | Return | Description |
|----------|--------|------|--------|-------------|
| `shim_nhbell` | `v` | `()` | void | Ring the bell |
| `shim_delay_output` | `v` | `()` | void | Pause briefly for animation |
| `shim_mark_synch` | `v` | `()` | void | Synchronize display output |
| `shim_wait_synch` | `v` | `()` | void | Wait for display sync to complete |
| `shim_get_nh_event` | `v` | `()` | void | Poll for asynchronous events |
| `shim_number_pad` | `vi` | `(state)` | void | Notify of number pad mode change |
| `shim_doprev_message` | `iv` | `()` | `int` | Scroll back through message history |
| `shim_preference_update` | `vp` | `(pref)` | void | Notify of a preference change |
| `shim_getmsghistory` | `sb` | `(init)` | `string` | Get a message from history. Return `""` when done |
| `shim_putmsghistory` | `vsb` | `(msg, restoring)` | void | Store a message in history |
| `shim_outrip` | `viii` | `(winid, how, when)` | void | Display the tombstone (3.6 only) |
| `shim_update_positionbar` | `vs` | `(posbar)` | void | Update position indicator |
| `shim_suspend_nhwindows` | `vs` | `(str)` | void | Suspend the window system |
| `shim_resume_nhwindows` | `v` | `()` | void | Resume the window system |

### Glyph System

`shim_print_glyph` receives integer glyph IDs. Decode them using these offset ranges (for this build: NUMMONS=381, NUM_OBJECTS=453):

| Range | Category | Offset Constant | How to decode |
|-------|----------|----------------|---------------|
| 0–380 | Monster | `GLYPH_MON_OFF` (0) | `glyph - 0` = monster index into `mons[]` |
| 381–761 | Pet | `GLYPH_PET_OFF` (381) | `glyph - 381` = monster index (tame) |
| 762 | Invisible | `GLYPH_INVIS_OFF` | Single glyph for "remembered invisible monster" |
| 763–1143 | Detected | `GLYPH_DETECT_OFF` (763) | `glyph - 763` = monster index (detected) |
| 1144–1524 | Corpse | `GLYPH_BODY_OFF` (1144) | `glyph - 1144` = monster index (corpse) |
| 1525–1905 | Ridden | `GLYPH_RIDDEN_OFF` (1525) | `glyph - 1525` = monster index (ridden) |
| 1906–2358 | Object | `GLYPH_OBJ_OFF` (1906) | `glyph - 1906` = object index into `objects[]` |
| 2359–2454 | Cmap | `GLYPH_CMAP_OFF` (2359) | `glyph - 2359` = dungeon symbol (see `S_` constants in `rm.h`) |
| 2455+ | Explosions, zaps, swallows, warnings, statues | Higher offsets | See `display.h` for exact ranges |
| 5976 | No glyph | `NO_GLYPH` / `MAX_GLYPH` | Sentinel value meaning "no glyph" (used as `bkglyph`) |

Common cmap symbols (glyph = symbol index + 2359):

| Symbol | Index | ASCII | Description |
|--------|-------|-------|-------------|
| `S_stone` | 0 | ` ` | Unexplored/dark area |
| `S_vwall` | 1 | `\|` | Vertical wall |
| `S_hwall` | 2 | `-` | Horizontal wall |
| `S_ndoor` | 12 | `.` | Doorway (no door) |
| `S_room` | 19 | `.` | Room floor |
| `S_corr` | 21 | `#` | Corridor |
| `S_upstair` | 23 | `<` | Upstairs |
| `S_dnstair` | 24 | `>` | Downstairs |
| `S_fountain` | 31 | `{` | Fountain |

### Tile System

When rendering with graphical tiles instead of ASCII, you need to map each glyph to a tile index in your tileset. The WASM build exposes NetHack's internal `glyph2tile[]` array through a helper function.

#### Why not compute tile indices from offsets?

The glyph offset constants (`GLYPH_MON_OFF`, `GLYPH_OBJ_OFF`, etc.) identify *what* a glyph represents, but they do not map directly to tile indices. NetHack's tilesets include conditional extra tiles for variant monsters (Cerberus, vampire mage, etc.) that shift the tile numbering. For example, the first 28 monster glyphs map to these tile indices:

```
glyph  0 -> tile  0    glyph 14 -> tile 14
glyph  1 -> tile  1    glyph 15 -> tile 15
...                     ...
glyph 26 -> tile 26    glyph 27 -> tile 28  (tile 27 is a conditional monster)
```

The only reliable way to get the correct tile index is to use NetHack's precomputed `glyph2tile[]` lookup, which accounts for all conditional tiles.

#### tileIndexForGlyph(glyph)

After the game initializes (once `_main` is called and helpers are registered), use:

```js
const helpers = globalThis.nethackGlobal.helpers;
const tileIndex = helpers.tileIndexForGlyph(glyph);
```

This is equivalent to `glyph2tile[glyph]` in C -- a direct lookup into NetHack's authoritative glyph-to-tile mapping. Returns `-1` for out-of-range glyphs.

#### Example: rendering tiles in shim_print_glyph

```js
const TILE_WIDTH = 16;
const TILE_HEIGHT = 16;
const TILES_PER_ROW = 40; // depends on your tileset image layout

globalThis.nethackCallback = async (name, ...args) => {
    if (name === "shim_print_glyph") {
        const [winid, x, y, glyph, bkglyph] = args;
        const helpers = globalThis.nethackGlobal.helpers;

        const tileIndex = helpers.tileIndexForGlyph(glyph);
        const srcX = (tileIndex % TILES_PER_ROW) * TILE_WIDTH;
        const srcY = Math.floor(tileIndex / TILES_PER_ROW) * TILE_HEIGHT;

        // Draw from tileset image at (srcX, srcY) to map position (x, y)
        ctx.drawImage(tilesetImage, srcX, srcY, TILE_WIDTH, TILE_HEIGHT,
                      x * TILE_WIDTH, y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
        return;
    }
    // ... other callbacks
};
```

#### mapglyphHelper

The `mapglyphHelper` also returns a `tileIdx` field alongside the ASCII character and color:

```js
const helpers = globalThis.nethackGlobal.helpers;
const info = helpers.mapglyphHelper(glyph, x, y, 0);
// info = { glyph, ch, color, special, tileIdx, x, y, mgflags }
```

This is useful when you need both tile and ASCII information for the same glyph in a single call.

### Status Fields

`shim_status_update` receives a field index and a pointer. For most fields, the pointer is a `char *` to a pre-formatted string. Read it with `Module.UTF8ToString(ptr)`.

| Field Index | Name | Example Value |
|-------------|------|---------------|
| 0 | `BL_TITLE` | `"Web_user the Gallant"` |
| 1 | `BL_STR` | `"18/07"` |
| 2 | `BL_DX` | `"12"` |
| 3 | `BL_CO` | `"15"` |
| 4 | `BL_IN` | `"9"` |
| 5 | `BL_WI` | `"11"` |
| 6 | `BL_CH` | `"17"` |
| 7 | `BL_ALIGN` | `"Lawful"` |
| 9 | `BL_CAP` | `""` (empty when not encumbered) |
| 10 | `BL_GOLD` | `"\GXXXXNNNN:42"` (encoded — see note) |
| 11 | `BL_ENE` | `"5"` |
| 12 | `BL_ENEMAX` | `"5"` |
| 13 | `BL_XP` | `"1"` |
| 14 | `BL_AC` | `"3"` |
| 17 | `BL_HUNGER` | `""` (empty when not hungry) |
| 18 | `BL_HP` | `"16"` |
| 19 | `BL_HPMAX` | `"16"` |
| 20 | `BL_LEVELDESC` | `"Dlvl:1"` |
| 22 | `BL_CONDITION` | `""` (bitmask of conditions) |
| -2 | `BL_FLUSH` | N/A — signal to flush/redraw status |

**Gold encoding:** The `BL_GOLD` field uses `\GXXXXNNNN:amount` format where `XXXXXXXX` is a hex-encoded glyph for the gold symbol. Strip the `\G________:` prefix to get the numeric amount.

### Exported Globals

`js_globals_init()` (called automatically during `_main`) exports pointers to key game variables on `globalThis.nethackGlobal.globals`:

| Global | Type | Description |
|--------|------|-------------|
| `plname` | `string` | Player name (writable — set during `shim_askname`) |
| `WIN_MAP` | `int` | Map window ID |
| `WIN_MESSAGE` | `int` | Message window ID |
| `WIN_STATUS` | `int` | Status window ID |
| `WIN_INVEN` | `int` | Inventory window ID |
| `flags.initrole` | `int` | Role selection (-1=none, -2=random, 0+=specific role) |
| `flags.initrace` | `int` | Race selection |
| `flags.initgend` | `int` | Gender selection |
| `flags.initalign` | `int` | Alignment selection |

### Differences from 3.7

If you're porting a 3.7 client to 3.6:

| Aspect | 3.6 | 3.7 |
|--------|-----|-----|
| Player selection callback | `shim_player_selection` (void) | `shim_player_selection_or_tty` (returns boolean) |
| `shim_start_menu` | 1 arg (winid) | 2 args (winid, mbehavior) |
| `shim_print_glyph` | int glyph, int bkglyph | glyph_info*, glyph_info* |
| `shim_start_screen` / `shim_end_screen` | Present | Removed |
| `shim_get_nh_event` | Called periodically | Not present |
| `shim_display_file` | Called for NEWS | Not present |
| `shim_update_inventory` | No-op in WASM (reentrancy) | Has parameter |
| Tile lookup | `tileIndexForGlyph(glyph)` returns `int` | `mapGlyphInfoHelper(glyph, x, y, mgflags).tileidx` |
| Global variables | Direct (`plname`) | Prefixed (`g.plname`) |

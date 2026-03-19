# NetHack Windowing Interface Architecture

## Overview

NetHack uses a modular windowing system architecture that allows multiple display interfaces (TTY, X11, Qt, Curses, etc.) to be compiled and selected at runtime. The interface sits between the windowing system and the game engine.

```
┌─────────────────────────────────────────┐
│           Window Interface              │
│    (TTY, X11, Qt, Curses, Win32, etc.)  │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         Window Procedures               │
│      (struct window_procs)              │
│       src/windows.c (core)              │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│           Command System                │
│        (src/cmd.c - rhack)              │
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

## Core Interface Files

### 1. src/windows.c - Main Window Interface

**Location:** `src/windows.c`

This is the central interface file that manages all window systems. Key components:

#### Global Window Procedure Structure
```c
NEARDATA struct window_procs windowprocs;
```
This holds the currently active window system's function pointers.

#### Window Choices Array
```c
static struct win_choices {
    struct window_procs *procs;
    void (*ini_routine)(int);
    void *(*chain_routine)(int, int, void *, void *, void *);
} winchoices[] = {
#ifdef TTY_GRAPHICS
    { &tty_procs, win_tty_init },
#endif
#ifdef CURSES_GRAPHICS
    { &curses_procs, 0 },
#endif
#ifdef X11_GRAPHICS
    { &X11_procs, win_X11_init },
#endif
#ifdef QT_GRAPHICS
    { &Qt_procs, 0 },
#endif
    // ... more window systems
};
```

#### Key Functions
- **`choose_windows(const char *s)`** - Select and initialize a window system at runtime
- **`win_choices_find(const char *s)`** - Find a window system by name
- **Generic implementations** (`genl_*`) - Default implementations ports can use

---

### 2. include/winprocs.h - Window Procedures Definition

**Location:** `include/winprocs.h`

Defines the `struct window_procs` - a structure containing function pointers for all window operations:

#### Window Operations Function Pointers
| Category | Function Pointers |
|----------|------------------|
| **Initialization** | `win_init_nhwindows`, `win_exit_nhwindows`, `win_suspend_nhwindows`, `win_resume_nhwindows` |
| **Window Management** | `win_create_nhwindow`, `win_clear_nhwindow`, `win_display_nhwindow`, `win_destroy_nhwindow` |
| **Output** | `win_print_glyph`, `win_putstr`, `win_putmixed`, `win_raw_print`, `win_raw_print_bold` |
| **Input** | `win_nhgetch`, `win_nh_poskey`, `win_yn_function`, `win_getlin`, `win_get_ext_cmd` |
| **Menus** | `win_start_menu`, `win_add_menu`, `win_end_menu`, `win_select_menu`, `win_message_menu` |
| **Status** | `win_status_init`, `win_status_finish`, `win_status_enablefield`, `win_status_update` |
| **Misc** | `win_mark_synch`, `win_wait_synch`, `win_cliparound`, `win_delay_output` |

#### Window Capability Flags (wincap)
```c
WC_COLOR      - Supports color
WC_HILITE_PET - Can highlight pets
WC_ASCII_MAP  - Uses ASCII map
WC_TILED_MAP  - Uses tiled graphics
WC_MOUSE_SUPPORT - Supports mouse
// ... and more
```

---

### 3. win/shim/winshim.c - Shim/Wrapper Layer

**Location:** `win/shim/winshim.c`

A translation layer for embedding NetHack (e.g., WebAssembly). It:
- Provides callback-based interface
- Wraps all window calls
- Forwards to registered callback function
- Supports both libnethack.a and Emscripten/WASM interfaces

```c
struct window_procs shim_procs = {
    WPID(shim),
    WC_ASCII_MAP | WC_MOUSE_SUPPORT | WC_COLOR | ...,
    // ... function pointers to shim_* functions
};
```

---

### 4. src/cmd.c - Command System

**Location:** `src/cmd.c`

The command system that receives input from windowing layer and dispatches to game commands.

#### Key Functions
- **`rhack(int key)`** - Main command processing loop
- **`parse(void)`** - Parse user input and handle repeat counts
- **`readchar(void)`** - Get a character from the window system
- **`get_ext_cmd()`** - Get extended command input

#### Input Flow
```
User Input → nhgetch()/nh_poskey() → readchar() → parse() → rhack() → Command Execution
```

---

## Window System Implementations

Each window system is in its own subdirectory under `win/`:

### TTY (Text Terminal)
**Location:** `win/tty/`
- `wintty.c` - Main TTY implementation
- `topl.c` - Top line (message) handling
- `getline.c` - Line input
- `termcap.c` - Terminal capability handling

**Exports:** `tty_procs`, `win_tty_init()`

### Curses
**Location:** `win/curses/`
- `cursmain.c` - Main curses implementation
- `curswins.c` - Window management
- `cursmesg.c` - Message handling
- `cursdial.c` - Dialog boxes
- `cursstat.c` - Status line
- `cursinvt.c` - Inventory display

**Exports:** `curses_procs`

### X11
**Location:** `win/X11/`
- `winX.c` - Main X11 implementation
- `winmap.c` - Map display
- `winmenu.c` - Menu handling
- `winmesg.c` - Message window
- `winmisc.c` - Miscellaneous dialogs
- `winstat.c` - Status display

**Exports:** `X11_procs`, `win_X11_init()`

### Qt
**Location:** `win/Qt/`
- `qt_main.cpp` - Main Qt implementation
- `qt_win.cpp` - Window handling
- `qt_map.cpp` - Map display
- `qt_menu.cpp` - Menu dialogs
- `qt_msg.cpp` - Message handling
- `qt_stat.cpp` - Status bar
- `qt_inv.cpp` - Inventory display

**Exports:** `Qt_procs`

### Windows (Win32)
**Location:** `win/win32/`
- `mswproc.c` - Main Windows procedure
- `mhmain.c` - Main window
- `mhmap.c` - Map window
- `mhmenu.c` - Menu dialogs
- `mhmsgwnd.c` - Message window
- `mhstatus.c` - Status window

**Exports:** `mswin_procs`

### macOS
**Location:** `win/macosx/`
- Various AppleScript and native implementations

---

## Window Chain (WINCHAIN)

NetHack supports chaining window processors for debugging/profiling:

```c
#ifdef WINCHAIN
    { &chainin_procs, chainin_procs_init, chainin_procs_chain },
    { (struct window_procs *) &chainout_procs, chainout_procs_init, chainout_procs_chain },
    { (struct window_procs *) &trace_procs, trace_procs_init, trace_procs_chain },
#endif
```

Located in `win/chain/`:
- `wc_chainin.c` - Chain input processor
- `wc_chainout.c` - Chain output processor
- `wc_trace.c` - Trace/logging processor

---

## Data Flow

```
┌─────────────────────────────────────────────────────────┐
│  USER INPUT                                             │
│  (keyboard, mouse)                                      │
└──────────────────┬──────────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────────┐
│  WINDOW SYSTEM IMPLEMENTATION                           │
│  (tty/curses/X11/Qt/win32)                              │
│  • Receives raw input                                   │
│  • Calls windowprocs.* functions                        │
└──────────────────┬──────────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────────┐
│  src/windows.c INTERFACE                                │
│  • Routes calls through windowprocs                     │
│  • Provides generic implementations                     │
│  • Manages window switching                             │
└──────────────────┬──────────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────────┐
│  src/cmd.c COMMAND SYSTEM                               │
│  • rhack() - main command loop                          │
│  • parse() - input parsing                              │
│  • Command dispatch to game functions                   │
└──────────────────┬──────────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────────┐
│  GAME ENGINE                                            │
│  (player, monsters, objects, dungeon, etc.)             │
└─────────────────────────────────────────────────────────┘
```

---

## Runtime Window Selection

Window system selection at runtime:

```c
// In src/windows.c
void choose_windows(const char *s) {
    // Find window system in winchoices[] array
    // Copy its window_procs to windowprocs global
    // Call initialization routine
}
```

### Command Line / Config Selection
Users can select window system via:
- Command line: `nethack -windowtype:tty` or `nethack -windowtype:qt`
- Config file: `OPTIONS=windowtype:curses`

### Example: Initializing TTY
```c
#ifdef TTY_GRAPHICS
    if (!strcmpi(s, "tty")) {
        windowprocs = tty_procs;
        win_tty_init(WININIT);
        return;
    }
#endif
```

---

## Key Structures Summary

| Structure | Location | Purpose |
|-----------|----------|---------|
| `struct window_procs` | `include/winprocs.h` | Function pointers for window operations |
| `struct win_choices` | `src/windows.c` | Associates window name with procedures and init function |
| `windowprocs` (global) | `src/windows.c` | Currently active window system |
| `winchoices[]` | `src/windows.c` | Array of available window systems |

---

## Adding a New Window System

To add a new window system:

1. Create new directory under `win/` (e.g., `win/mynewgui/`)
2. Implement all functions in `struct window_procs`
3. Export `struct window_procs mynewgui_procs`
4. Add entry to `winchoices[]` in `src/windows.c`:
   ```c
   #ifdef MYNEWGUI_GRAPHICS
       { &mynewgui_procs, mynewgui_init },
   #endif
   ```
5. Add `#define MYNEWGUI_GRAPHICS` to your build configuration

---

## Additional Resources

- `doc/window.doc` - Window port documentation
- `include/wintype.h` - Window types and constants
- `include/decl.h` - Global declarations including window-related
- `win/share/` - Shared utilities for window systems (tile handling, etc.)

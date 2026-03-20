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

---

## How NetHack Uses Window Procedures (Code Examples)

This section shows actual code from the NetHack source that demonstrates how the window procedures are called.

### Output/Drawing Operations

#### print_glyph - Drawing Map Tiles

**File:** `src/display.c`
```c
/* Drawing the dungeon map - from show_glyph() in display.c */
print_glyph(WIN_MAP, x, y,
            Glyphinfo_at(x, y, glyph), &bkglyphinfo);
```

The `print_glyph()` function draws a single tile at position (x,y) on the map window.

#### putstr - Outputting Text

**File:** `src/pline.c` (main message output)
```c
/* Output a message to the message window */
putstr(WIN_MESSAGE, attr, line);
```

**File:** `src/cmd.c` (key bindings help display)
```c
/* Displaying help text in a window */
putstr(datawin, 0, "");
putstr(datawin, 0, "Directional keys:");
Sprintf(buf, "%-7s %s", key2txt(key, buf2), misc_keys[i].desc);
putstr(datawin, 0, buf);
```

#### Menu Operations

**File:** `src/windows.c` (wrapper functions)
```c
/* add_menu() - adds an item to a menu */
void
add_menu(winid window, const glyph_info *glyphinfo,
         const anything *identifier, char ch, char gch,
         int attr, int color, const char *str, unsigned int itemflags)
{
    /* Apply menu color filtering if enabled */
    if (iflags.use_menu_color) {
        if ((itemflags & MENU_ITEMFLAGS_SKIPMENUCOLORS) == 0)
            (void) get_menu_coloring(str, &color, &attr);
    }
    /* Call the actual window procedure */
    (*windowprocs.win_add_menu)(window, glyphinfo, identifier,
                                ch, gch, attr, color, str, itemflags);
}

/* select_menu() - displays menu and gets user selection */
int
select_menu(winid window, int how, menu_item **menu_list)
{
    int reslt;
    boolean old_bot_disabled = gb.bot_disabled;

    gb.bot_disabled = TRUE;  /* Disable status updates during menu */
    reslt = (*windowprocs.win_select_menu)(window, how, menu_list);
    gb.bot_disabled = old_bot_disabled;
    return reslt;
}
```

### Input Operations

#### yn_function - Yes/No Queries

**File:** `src/cmd.c`
```c
/* yn_function() - ask a yes/no question */
char
yn_function(const char *query, const char *resp, char def, boolean addcmdq)
{
    char res = '\033';  /* Default to ESC */
    /* ... command queue handling ... */
    
    /* Try menu-based response first, fall back to window procedure */
    if (!yn_function_menu(query, resp, def, &res)) {
        res = (*windowprocs.win_yn_function)(query, resp, def);
    }
    /* ... */
    return res;
}
```

**Usage Example:** `src/dokick.c`
```c
if (yn_function("Kick your steed?", ynchars, 'y', TRUE) == 'y') {
    You("kick %s.", mon_nam(u.usteed));
}
```

**Usage Example:** `src/eat.c`
```c
c = yn_function("Continue eating?", ynqchars, 'n', TRUE);
```

#### nhgetch/nh_poskey - Reading Keystrokes

**File:** `src/cmd.c`
```c
/* pgetchar() - gets a character, used for replaying commands */
char
pgetchar(void)
{
    int ch = '\0';

    if (iflags.debug_fuzzer)
        return randomkey();
    ch = nhgetch();  /* Calls windowprocs.win_nhgetch */
    return (char) ch;
}

/* readchar_core() - core input reading function */
staticfn char
readchar_core(coordxy *x, coordxy *y, int *mod)
{
    int sym;
    
    if (*readchar_queue)
        sym = *readchar_queue++;  /* From command queue */
    else if (gi.in_doagain)
        sym = pgetchar();          /* Replay */
    else
        sym = nh_poskey(x, y, mod); /* Get from window system */
    
    /* Handle mouse clicks and special keys */
    if (sym == 0) {
        /* Mouse click event - convert to command */
        click_to_cmd(*x, *y, *mod);
    }
    /* ... */
    return (char) sym;
}

/* readchar() - public interface to get a character */
char
readchar(void)
{
    char ch;
    coordxy x = u.ux, y = u.uy;
    int mod = 0;

    ch = readchar_core(&x, &y, &mod);
    return ch;
}
```

#### getlin - Line Input

**File:** `src/windows.c`
```c
/* getlin() - get a line of text from user */
void
getlin(const char *query, char *bufp)
{
    boolean old_bot_disabled = gb.bot_disabled;
    /* ... command queue handling for scripted input ... */
    
    program_state.in_getlin = 1;
    gb.bot_disabled = TRUE;
    (*windowprocs.win_getlin)(query, bufp);
    gb.bot_disabled = old_bot_disabled;
    program_state.in_getlin = 0;
}
```

**Usage Example:** `src/do_name.c` (naming monsters/items)
```c
getlin(prompt, outbuf);
if (!*outbuf || *outbuf == '\033')
    return;  /* Cancelled */
```

**Usage Example:** `src/engrave.c` (engraving text)
```c
getlin(de->qbuf, de->ebuf);
(void) mungspaces(de->ebuf);  /* Clean up whitespace */
```

**Usage Example:** `src/options.c` (autopickup patterns)
```c
getlin("What new autopickup exception pattern?", &apebuf[1]);
mungspaces(&apebuf[1]);
```

### Direct Windowprocs Calls

Some functions call `windowprocs` directly without wrappers:

**File:** `src/botl.c` (status line management)
```c
(*windowprocs.win_status_init)();
(*windowprocs.win_status_finish)();
```

**File:** `src/coloratt.c` (color management)
```c
(*windowprocs.win_change_color)(clridx, rgb, 0);
```

**File:** `src/invent.c` (inventory display updates)
```c
(*windowprocs.win_update_inventory)(0);  /* Full update */
(*windowprocs.win_update_inventory)(1);  /* Partial update */
```

### Complete Input Flow

**Main Game Loop** (`src/allmain.c`):
```c
/* Main turn loop */
for (;;) {
    cliparound(u.ux, u.uy);  /* Ensure player is visible */
    rhack(gc.cmd_key);       /* Get and execute player command */
    /* ... */
}
```

**Command Processing** (`src/cmd.c`):
```c
/* rhack() - main command dispatcher */
void
rhack(int key)
{
    boolean firsttime = (key == 0);
    
    if (firsttime) {
        key = parse();  /* Parse user input */
    }
    /* ... dispatch to command handlers based on key ... */
}

/* parse() - read and interpret user input */
staticfn int
parse(void)
{
    program_state.input_state = commandInp;
    flush_screen(1);  /* Flush display buffer */
    
    /* Get first character (possibly a repeat count prefix) */
    if (!gc.Cmd.num_pad || (foo = readchar()) == gc.Cmd.spkeys[NHKF_COUNT]) {
        foo = get_count((char *) 0, '\0', LARGEST_INT,
                        &gc.command_count, GC_NOFLAGS);
    }
    /* ... */
    return gc.cmd_key;
}
```

### Summary of Calling Conventions

| Operation | Wrapper Function | Direct Windowprocs Call | Common Usage |
|-----------|------------------|------------------------|--------------|
| Draw map tile | `print_glyph()` | `(*windowprocs.win_print_glyph)()` | Dungeon display |
| Output text | `putstr()` | `(*windowprocs.win_putstr)()` | Messages, menus |
| Yes/No prompt | `yn_function()` | `(*windowprocs.win_yn_function)()` | Confirmations |
| Get character | `readchar()` | `(*windowprocs.win_nh_poskey)()` | Command input |
| Get line | `getlin()` | `(*windowprocs.win_getlin)()` | Text entry |
| Add menu item | `add_menu()` | `(*windowprocs.win_add_menu)()` | Inventory, etc. |
| Show menu | `select_menu()` | `(*windowprocs.win_select_menu)()` | Selection |
| Update status | - | `(*windowprocs.win_status_update)()` | Status line |
| Clear window | `clear_nhwindow()` | `(*windowprocs.win_clear_nhwindow)()` | Screen clear |
| Display window | `display_nhwindow()` | `(*windowprocs.win_display_nhwindow)()` | Show dialog |

### Key Insight

Most game code uses **wrapper functions** defined in `src/windows.c` (like `getlin()`, `add_menu()`, `putstr()`) rather than calling `windowprocs` directly. These wrappers:

1. Add common functionality (command queue handling, status disable, color filtering)
2. Provide a stable API regardless of window system
3. Handle special cases (debug fuzzer, hangup recovery)

The `windowprocs` structure is the **polymorphic dispatch table** that routes these calls to the active window system (TTY, Curses, Qt, etc.).

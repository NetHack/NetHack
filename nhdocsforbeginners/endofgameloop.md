# End of Game Loop Analysis

## Overview

This document analyzes how player death is checked and how the game loop exits in NetHack's `moveloop_core()` function located in `src/allmain.c`.

---

## Player Death Checks in `moveloop_core()`

**Important: `moveloop_core()` does NOT directly check for player death.**

Death checks happen indirectly through the following mechanisms:

### 1. `regen_hp()` - HP Regeneration/Degeneration

Located within the once-per-turn processing section:

```c
if (Upolyd) {
    if (u.mh < 1) { /* shouldn't happen... */
        rehumanize();
    }
```

**What happens:**
- When the hero is polymorphed (`Upolyd` is true)
- If monster HP (`u.mh`) drops below 1
- `rehumanize()` is called to return to normal form
- If `Unchanging` property is set, `rehumanize()` calls `done(DIED)` to end the game

### 2. Overexertion from Encumbrance

```c
if (mvl_wtcap > MOD_ENCUMBER && u.umoved) {
    if (!(mvl_wtcap < EXT_ENCUMBER ? svm.moves % 30 : svm.moves % 10)) {
        overexert_hp();
    }
}
```

**What happens:**
- Moving while heavily encumbered causes HP loss
- `overexert_hp()` reduces HP periodically
- Can potentially reduce HP to 0, triggering death

### 3. Other Death Triggers (Outside `moveloop_core()`)

Death checks occur throughout the codebase in various files:

| File | Death Check | Function Called |
|------|-------------|-----------------|
| `src/hack.c` | `u.uhp < 1` | `done(DIED)` |
| `src/attrib.c` | `u.uhp < 1` | `done(DIED)` or `done(POISONING)` |
| `src/exper.c` | `u.uhp < 1` | `done(DIED)` |
| `src/mhitu.c` | `u.uhp < 1` | `done_in_by(mtmp, DIED)` |
| `src/eat.c` | Various conditions | `done(CHOKING)`, `done(POISONING)`, `done(STARVING)` |
| `src/trap.c` | Various conditions | `done(DROWNING)`, `done(BURNING)`, `done(DISSOLVED)`, `done(STONING)` |
| `src/polyself.c` | `u.mh < 1` with `Unchanging` | `done(DIED)` |
| `src/pray.c` | Various conditions | `done(DIED)`, `done(ESCAPED)`, `done(ASCENDED)` |
| `src/timeout.c` | Various conditions | `done(GENOCIDED)`, etc. |

---

## Game Loop Exit Mechanism

### The Infinite Loop Structure

```c
void
moveloop(boolean resuming)
{
    moveloop_preamble(resuming);

    if (!resuming)
        maybe_do_tutorial();

    for (;;) {
        moveloop_core();
    }
}
```

**Key Point:** `moveloop_core()` **always returns** to its caller. There is **no explicit exit** from this function itself.

### How the Game Actually Ends

The game loop exits through the **`done()` function** in `src/end.c`, which:

1. **Checks for life-saving:**
   - Amulet of Life Saving
   - Wizard/Explore mode "Die?" prompt

2. **Calls `really_done(how)`** if death is confirmed

3. **`really_done()` performs end-of-game tasks:**
   - Sets `program_state.gameover = 1`
   - Disables saving: `program_state.something_worth_saving = 0`
   - Creates bones file (if applicable)
   - Shows tombstone and final score
   - Calls `nh_terminate(EXIT_SUCCESS)` to exit the program

4. **`nh_terminate()` exits the program:**
   ```c
   program_state.in_moveloop = 0;
   nethack_exit(status);
   ```

### The ONE Explicit Exit in `moveloop_core()`

There is exactly one place where `moveloop_core()` can directly end the game:

```c
/* Never allow 'moves' to grow big enough to wrap */
if (svm.moves >= 1000000000L) {
    display_nhwindow(WIN_MESSAGE, TRUE);
    urgent_pline("The dungeon capitulates.");
    done(ESCAPED);
}
```

This is a safeguard to prevent the `moves` counter from overflowing after 1 billion turns.

---

## Death Functions Reference

### Primary Death Functions (`src/end.c`)

| Function | Description |
|----------|-------------|
| `done(how)` | Main death handler - checks life-saving, calls `really_done()` |
| `done_in_by(mtmp, how)` | Called when killed by a monster |
| `really_done(how)` | Performs end-of-game processing (NORETURN) |
| `savelife(how)` | Restores HP and saves the player (life-saving) |
| `panic(str)` | Called on fatal errors, saves game state |

### Death Reasons (from `src/hack.h`)

```c
#define DIED            0
#define CHOKING         1
#define POISONING       2
#define STARVING        3
#define DROWNING        4
#define BURNING         5
#define DISSOLVED       6
#define CRUSHING        7
#define STONING         8
#define TURNED_SLIME    9
#define GENOCIDED      10
#define PANICKED       11
#define TRICKED        12
#define QUIT           13
#define ESCAPED        14
#define ASCENDED       15
```

---

## Summary

1. **`moveloop_core()` does not directly check for player death**
2. **Death is detected indirectly** through HP checks in subroutines like `regen_hp()` and various damage-dealing functions throughout the codebase
3. **The game loop is infinite** (`for (;;)`) and only exits through `done()` → `really_done()` → `nh_terminate()`
4. **The only explicit game-ending check in `moveloop_core()`** is the 1 billion move limit safeguard
5. **`done()` is the central death handler** - all death paths eventually call this function

---

## Related Files

- `src/allmain.c` - Main game loop (`moveloop()`, `moveloop_core()`)
- `src/end.c` - Death handling (`done()`, `really_done()`)
- `src/hack.c` - Core game mechanics, HP damage
- `src/polyself.c` - Polymorph handling, `rehumanize()`
- `src/mhitu.c` - Monster attacks, `done_in_by()`
- `src/attrib.c` - Attribute changes, poison damage

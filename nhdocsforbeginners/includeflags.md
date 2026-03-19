# Include Flags (CFLAGS) in NetHack

## Understanding `-I` Paths in CFLAGS

When you see a Makefile with:
```makefile
CFLAGS ?= -I../include
```

The `../include` path is **NOT** relative to the individual C file being compiled. It's relative to the **working directory where the compiler is invoked** (typically where the Makefile is located or where `make` is run).

## Key Point

The `-I` paths are resolved relative to the **compiler's working directory at runtime**, not the source file location.

## Typical Scenario

If your project structure is:
```
NetHack/
├── include/          # header files
├── src/              # C source files
│   ├── Makefile
│   └── nhlua.c
```

And the Makefile in `src/` contains:
```makefile
CFLAGS ?= -I../include
```

When you run `make` from the `src/` directory:
- The compiler runs with `src/` as the working directory
- `-I../include` resolves to `NetHack/include/`
- This works correctly regardless of which `.c` file is being compiled

## Important Note

If you were to compile from a different directory (like the project root):
```bash
cd /path/to/NetHack && gcc -I../include src/nhlua.c  # WRONG - would look in parent of NetHack
```

You would need to adjust the path:
```bash
cd /path/to/NetHack && gcc -I./include src/nhlua.c   # CORRECT
```

## Summary

The `../include` path is relative to where `make`/`gcc` is executed from, not the location of individual source files. This is why standard project layouts typically have the Makefile in a consistent location relative to the include directory.

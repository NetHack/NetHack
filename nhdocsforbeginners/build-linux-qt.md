# Building NetHack for Linux with Qt
Details summarised from file: NetHack\sys\unix\NewInstall.unx

This document describes how to build NetHack for Linux with the Qt graphical interface.

## Makefile Location

The makefiles are located in **`sys/unix/`**:

| Template Makefile | Description |
|-------------------|-------------|
| `sys/unix/Makefile.top` | Top-level Makefile |
| `sys/unix/Makefile.src` | Source compilation Makefile |
| `sys/unix/Makefile.utl` | Utility programs Makefile |
| `sys/unix/Makefile.dat` | Data files Makefile |
| `sys/unix/Makefile.doc` | Documentation Makefile |

## Qt Hints File

The Linux Qt configuration is in:
- **`sys/unix/hints/linux.370`**

This file contains Qt-specific settings:
- `QTDIR=/usr` for Qt5
- `QTDIR=/usr/local/qt6` for Qt6 (adjust if your Qt is elsewhere)

## Build Steps

### 1. Run setup.sh to distribute the makefiles

```bash
cd sys/unix
sh setup.sh hints/linux
cd ../..
```

This creates the working Makefiles in the root, `src/`, `util/`, `dat/`, and `doc/` directories.

### 2. Build with Qt

**Run from the root NetHack directory** (where the top-level Makefile was created by setup.sh):

```bash
# Navigate to root directory (if not already there)
cd /path/to/NetHack

# For Qt5 (default)
make WANT_WIN_QT=1

# For Qt6
make WANT_WIN_QT6=1

# For Qt4 (older systems)
make WANT_WIN_QT4=1
```

To build with multiple window interfaces (e.g., Qt + TTY + Curses):
```bash
make WANT_WIN_QT=1 WANT_WIN_TTY=1 WANT_WIN_CURSES=1
```

### 3. Install

```bash
make install
```

## Summary - Which Folder to Run Commands In

| Step | Folder | Command |
|------|--------|---------|
| 1. setup.sh | `sys/unix/` | `sh setup.sh hints/linux` |
| 2. make | **Root NetHack folder** (e.g., `~/NetHack/`) | `make WANT_WIN_QT=1` |
| 3. install | Root NetHack folder | `make install` |

## Custom Qt Location

If Qt is installed in a non-standard location, edit `sys/unix/hints/linux.370` before running setup.sh:

```makefile
# For Qt6, change this line:
QTDIR=/usr/local/qt6

# Or for Qt5:
QTDIR=/usr
```

## Additional Options

| Option | Description |
|--------|-------------|
| `WANT_WIN_ALL=1` | Enable all window interfaces |
| `WANT_DEFAULT=Qt` | Set Qt as default interface |
| `QT6MANUAL=1` | Use manual Qt6 configuration instead of pkg-config |

## Troubleshooting

- If you get "QTDIR not defined" errors, ensure the Qt development packages are installed
- For Qt6 on Debian/Ubuntu: `sudo apt-get install qt6-base-dev qt6-multimedia-dev`
- For Qt5 on Debian/Ubuntu: `sudo apt-get install qtbase5-dev qtmultimedia5-dev`

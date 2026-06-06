# Building NetHack for Classic Mac OS (System 7, 68k)

Cross-compilation using [Retro68](https://github.com/autc04/Retro68) GCC
toolchain targeting Motorola 68k Macs in 32-bit addressing mode.

## Prerequisites

### Retro68 Toolchain

Install Retro68 to `/opt/retro68` (or set `RETRO68=` to your path).

One patch is required: apply `sys/mac68k/tools/retro68_elf2mac.patch` to
the Retro68 source tree and rebuild Elf2Mac.  It lets the linker accept
NetHack's function-pointer tables in `.data` (cross-section jump-table
references), which upstream Elf2Mac rejects with an assert.  Nothing
else in the toolchain needs patching: the stock `libretrocrt` runtime
works as-is in 32-bit addressing mode.

### Apple Universal Interfaces 3.4

Retro68's bundled "Multiversal" headers are incomplete. Install Apple's
original headers from the MPW-GM (Macintosh Programmer's Workshop Golden
Master) disk image:

    # Mount the MPW-GM image and copy CIncludes
    mkdir -p /opt/retro68/universal/CIncludes
    cp -r <mpw-gm>/Interfaces/CIncludes/* /opt/retro68/universal/CIncludes/

    # Wrap the Interface library (Pascal-calling-convention Toolbox glue)
    # in a proper ar archive so -lInterface resolves it
    /opt/retro68/bin/m68k-apple-macos-ar rcs \
       /opt/retro68/m68k-apple-macos/lib/libInterface.a \
       <mpw-gm>/Libraries/Libraries/Interface.o

### Host Tools

- `hfsutils` — `hformat`, `hmount`, `hcopy`, `hattrib`, `humount`, `hmkdir`
- Python 3 — for resource fork and disk image tools
- `qemu-system-m68k` (QEMU 8.0+) — optional, for testing

---

## Building

From a fresh checkout, generate the Makefiles and fetch Lua first:

    cd NetHack
    sys/unix/setup.sh sys/unix/hints/linux.500
    make fetch-lua

Then build:

    make CROSS_TO_MAC68K=1 all

This produces:
- `targets/mac68k/NetHack` — data fork (tiny text stub)
- `targets/mac68k/.rsrc/NetHack` — resource fork (CODE/DATA/RELA segments)
- `targets/mac68k/NetHack.gdb` — ELF with debug symbols

## Mounting the image in emulators

Classic Mac OS loads the driver that mounts a SCSI disk from the disk
itself; the packaged `NetHack.img` ships without one (BlueSCSI and
real disks don't need it).  For an image QEMU auto-mounts, point
`NHMAC_DRIVER_DONOR` at any formatted Mac disk image (your boot disk
works) and packaging clones its driver in.  The driver is Apple/LaCie
licensed code -- it stays on your machine and is never checked in.

## Packaging

    make -C src CROSS_TO_MAC68K=1 mac68kpkg

(Run from `src/`; the top-level Makefile's generated Lua paths break this
target when invoked from the repository root.)

This runs the full packaging pipeline:
1. Compile SIZE resource with Rez
2. Merge SIZE + NHrsrc UI resources into the resource fork
3. Emit the MacBinary directly from the same Rez call
4. Build HFS disk image with all data files, `save/` and `levels/`
   directories, and `nethack.cnf`
5. Wrap with Apple Partition Map for SCSI emulators

Output:
- `targets/mac68k/NetHack.img` — ready for QEMU or BlueSCSI
- `targets/mac68k/NetHack.sit` — StuffIt archive for distribution

(`targets/mac68k/NetHack.bin`, the MacBinary the image is built from,
also remains available for `hcopy -m` in-place updates of existing
disks.)

### Updating an existing disk (e.g. BlueSCSI)

    hmount /path/to/disk.hda
    hdel NetHack
    hcopy -m targets/mac68k/NetHack.bin :
    humount

**Never recreate a BlueSCSI disk image from scratch** if it has a working
SilverLining driver — use `hmount`/`hcopy`/`humount` to update files in place.

### Resource merging note

Resources are merged into the application fork with a single `Rez --copy`
pass.  Gotcha: on a non-Mac host, Rez locates a file's resource fork via
its sidecar conventions (`file` plus `.rsrc/file`); pass it a bare fork
image and it silently reads an EMPTY fork and emits an app with no CODE
resources, which crashes at launch.  Always hand `--copy` the data-fork
path of a file whose fork lives in the `.rsrc/` sidecar, as the pipeline
does.

---

## Testing with QEMU

    qemu-system-m68k -M q800 -m 128 \
        -bios "<path-to-quadra-800-rom>" \
        -drive file=pram.img,format=raw,if=mtd \
        -drive file=boot.hda,format=raw,media=disk \
        -drive file=NetHack.img,format=raw,media=disk \
        -g 800x600x8

- `pram.img` with `if=mtd` persists PRAM settings (32-bit mode, etc.)
- The boot disk must have System 7.x installed with 32-bit mode enabled
  (Memory control panel → 32-Bit Addressing: On)

---

## Tools (sys/mac68k/tools/)

| Script | Purpose |
|--------|---------|
| `make_scsi_image2.py` | Wrap an HFS image with Apple Partition Map for SCSI; `--driver-from <donor>` clones a SCSI driver so emulators auto-mount it |
| `decode_hqx.py` | Decode BinHex 4.0 (.hqx) files to data + resource forks; `--creator-fixup` renames the legacy signature/BNDL creator |
| `dump_rsrc.py` | Dump resource fork contents (types, IDs, sizes) |
| `make_info.py` | Write a macutils `.info` sidecar (name/type/creator + build-time dates) for the StuffIt staging |

## Historical files

`sys/mac68k/README`, `Install.mw`, and `NHrsrc.hqx`/`NHsound.hqx` date from the
original 1990s Macintosh port (Metrowerks/MPW).  They are retained for
reference; only this file describes the Retro68 cross-compile.

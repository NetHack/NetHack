# Building NetHack for Classic Mac OS (68k / PowerPC)

Cross-compiled with the [Retro68](https://github.com/autc04/Retro68) GCC
toolchain. Three targets:

- **68k** — System 7+ on 68020+ in 32-bit addressing mode
- **PowerPC** — System 7.x through Mac OS 9 (CFM/PEF, no Carbon)
- **Fat** — one app that runs native on both

## What you need

- Retro68 toolchain at `/opt/retro68` (or set `RETRO68=`). Verified with its
  default GCC 12 through GCC 16; the build pins `-std=gnu17` automatically.
- Apple Universal Interfaces 3.x in `/opt/retro68/universal` (Retro68's bundled
  Multiversal headers are incomplete). Install steps below.
- Host tools: `hfsutils` and `python3`; optionally `sit` for the StuffIt
  archive (see below) and `qemu-system-m68k` (8.0+) for testing.  Without
  `sit` the packaging step skips the `.sit` and still writes the disk image
  and MacBinary.

## Get the prerequisites

### Install Retro68 and the Universal Interfaces

Host packages (Debian/Ubuntu):

    sudo apt install build-essential cmake bison flex texinfo ruby hfsutils \
        libgmp-dev libmpfr-dev libmpc-dev libboost-all-dev

### Install sit (optional, for the StuffIt archive)

Build it from source and put it on `PATH`:

    git clone https://github.com/thecloudexpanse/sit.git
    cd sit && make
    cp sit ~/.local/bin/

Skip this if you only need `NetHack.img` or `NetHack.bin`; the packaging
targets note the missing tool and carry on.

Clone with submodules (GCC and binutils are submodules):

    git clone --recursive https://github.com/autc04/Retro68.git
    cd Retro68

Build the toolchain — binutils + GCC for 68k and PowerPC plus the host tools
(Rez, MakePEF, Elf2Mac, ...) — from a separate build dir into an empty, writable
`/opt/retro68`. This takes a while:

    sudo mkdir -p /opt/retro68 && sudo chown $USER /opt/retro68
    mkdir ../Retro68-build && cd ../Retro68-build
    ../Retro68/build-toolchain.bash --prefix=/opt/retro68 --no-carbon
    cd ../Retro68

> On a modern host (GCC 15+) the 68k GCC build can fail in libbacktrace with
> *"NM has changed"*; add `--disable-lto` to the 68k `gcc/configure` line in
> `build-toolchain.bash` (the PowerPC one already has it) and rerun.

Put the toolchain on `PATH` for the remaining steps:

    export PATH=/opt/retro68/bin:$PATH

Retro68's bundled "Multiversal" headers are incomplete, so install Apple's
Universal Interfaces 3.x over them. Download the **MPW 3.5 Golden Master** disk
image (MacBinary DiskCopy, ~25 MB, served as `mpw-gm.img__0.bin`) from
<http://macintoshgarden.org/apps/macintosh-programmers-workshop> into the current
(Retro68 source) directory, then — args to the second script are build-68k,
build-PPC, skip-Carbon:

    ./install-universal-interfaces.sh . mpw-gm.img__0.bin
    ./interfaces-and-libraries.sh /opt/retro68 ./InterfacesAndLibraries true true false

`/opt/retro68/universal/CIncludes` now holds the ~390 Apple headers.

## Configure (once)

    cd NetHack
    sys/unix/setup.sh sys/unix/hints/linux.500
    make fetch-lua

## Build and package

Run the `*pkg` targets from `src/` (`make -C src`) — the top-level Makefile's
generated Lua paths break them when invoked from the repository root.

### 68k

    make CROSS_TO_MAC68K=1 all
    make -C src CROSS_TO_MAC68K=1 mac68kpkg

`targets/mac68k/`: `NetHack.img` (self-mounting SCSI disk, embeds the port's
`.NHsd` driver), `NetHack.sit` (StuffIt), `NetHack.bin` (MacBinary).

### PowerPC

    make CROSS_TO_MACPPC=1 all
    make -C src CROSS_TO_MACPPC=1 macppcpkg

`targets/macppc/`: `NetHack.sit`, `NetHack.bin`.

### Fat (68k + PowerPC)

    make CROSS_TO_MAC68K=1 all
    make -C src CROSS_TO_MAC68K=1 mac68kpkg
    make CROSS_TO_MACPPC=1 all
    make -C src CROSS_TO_MACPPC=1 macfatpkg

`targets/macfat/`: `NetHack.sit`, `NetHack.bin` (+ `Recover.bin`).

## Test with QEMU (68k)

    qemu-system-m68k -M q800 -m 128 \
        -bios <quadra-800-rom> \
        -drive file=pram.img,format=raw,if=mtd \
        -drive file=boot.hda,format=raw,media=disk \
        -drive file=targets/mac68k/NetHack.img,format=raw,media=disk \
        -g 800x600x8

`pram.img` (`if=mtd`) persists PRAM. The boot disk needs System 7.x with 32-bit
addressing enabled (Memory control panel).

## Tools (`sys/mac68k/tools/`)

| Script | Purpose |
|--------|---------|
| `make_scsi_image2.py` | Wrap an HFS image with Apple Partition Map for SCSI; `--driver <bin>` embeds the auto-mount driver |
| `decode_hqx.py` | Decode BinHex 4.0 (`.hqx`) to data + resource forks |
| `dump_rsrc.py` | Dump resource fork contents (types, IDs, sizes) |
| `make_info.py` | Write a macutils `.info` sidecar for StuffIt staging |

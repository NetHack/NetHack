# Building NetHack for Classic Mac OS (PowerPC, CFM/PEF)

Native PowerPC classic build, parallel to the 68k build documented in
[`BUILD.md`](BUILD.md).  Runs on PowerPC Macs under System 7.x and later
(no Carbon, no Mac OS X).  Cross-compiled with the Retro68
`powerpc-apple-macos` GCC toolchain; the `sys/mac68k` UI sources are shared
verbatim with the 68k port.

## Prerequisites

- Retro68 at `/opt/retro68` (or set `RETRO68=`), including the
  `powerpc-apple-macos-*` tools, `MakePEF`, and `Rez`.
- Apple Universal Interfaces 3.x in `$RETRO68/universal/CIncludes`
  (same headers the 68k build uses; see `BUILD.md`).  If your Retro68 shipped
  the open-source Multiversal Interfaces instead, see
  [Toolchain compatibility](#toolchain-compatibility-retro68--gcc-version).
- Host tools: `sit` (for the StuffIt archive) and Python 3.

## Building

    cd NetHack
    sys/unix/setup.sh sys/unix/hints/linux.500
    make fetch-lua
    make CROSS_TO_MACPPC=1 all

Produces `targets/macppc/NetHack.xcoff`, the linked PowerPC executable.

## Packaging

    make -C src CROSS_TO_MACPPC=1 macppcpkg

Produces:
- `targets/macppc/NetHack.sit` — distribution archive
- `targets/macppc/NetHack.bin` — MacBinary app

## Fat (68k + PowerPC) binary

One application that runs native on both architectures:

    make CROSS_TO_MAC68K=1 mac68kpkg
    make CROSS_TO_MACPPC=1 macfatpkg

Produces `targets/macfat/NetHack.sit` (and `NetHack.bin` / `Recover.bin`),
alongside the per-arch `targets/mac68k/` and `targets/macppc/` packages.

## Toolchain compatibility (Retro68 / GCC version)

Verified with Retro68 using GCC 12 (the `/opt/retro68` default) and GCC 16.

The cross hint files pin `-std=gnu17`.  GCC 15 and later default to C23, whose
`_Generic`-based save dispatch in `include/savefile.h` has no plain `int *`
association and assumes `int == int32_t`.  On the classic-Mac targets `int32_t`
is `long int`, so a default-C23 build fails on integer save sites (e.g.
`save_dungeon` in `src/dungeon.c`: *`'_Generic' selector of type 'int *' is not
compatible with any association`*).  The `gnu17` pin keeps the working typed
save path on every GCC from 6 through 16; no action is needed on your part.

### Installing Apple Universal Interfaces on a multiversal Retro68

Newer Retro68 may ship the open-source **Multiversal Interfaces** rather than
Apple's Universal Interfaces.  The Multiversal headers are incomplete (no
Carbon, OpenTransport, or post-7.0 APIs) and this port expects the Universal
Interfaces.  Install them into an existing toolchain **without rebuilding the
compiler**:

1. Download the **MPW 3.5 Golden Master** disk image from the Macintosh Garden —
   <http://macintoshgarden.org/apps/macintosh-programmers-workshop> (served as
   `mpw-gm.img__0.bin`, ~25 MB).  It is a MacBinary DiskCopy image of the
   *Interfaces&Libraries* folder.  (Mirror, as `MPW-GM.img.bin`:
   <https://www.macintoshrepository.org/1360-macintosh-programmer-s-workshop-mpw-3-0-to-3-5>.)

2. Install `hfsutils` (provides `hmount`, `hls`, `hcopy`):

       sudo apt install hfsutils

3. Extract the headers and libraries — this uses Retro68's `ConvertDiskImage`,
   so put `$RETRO68/bin` on `PATH`:

       cp mpw-gm.img__0.bin /tmp/MPW-GM.img.bin
       PATH=$RETRO68/bin:$PATH \
         $RETRO68_SRC/install-universal-interfaces.sh /tmp MPW-GM.img.bin

   This writes `/tmp/InterfacesAndLibraries` (CIncludes, RIncludes, the 68k
   `Interface.o`, and the PPC `SharedLibraries`).

4. Install them into the toolchain.  This regenerates `$RETRO68/universal` with
   Retro68's own tools and relinks the per-target `include`/`lib` dirs from
   multiversal to universal (the trailing `true true false` = build 68k, build
   PPC, skip Carbon):

       PATH=$RETRO68/bin:$PATH \
         $RETRO68_SRC/interfaces-and-libraries.sh \
           $RETRO68 /tmp/InterfacesAndLibraries true true false

`$RETRO68` is the toolchain prefix (e.g. `/opt/retro68`); `$RETRO68_SRC` is the
Retro68 source checkout (the two helper scripts live there, not in `$RETRO68/bin`).
Afterwards `$RETRO68/universal/CIncludes` holds the ~390 Apple headers and the
build runs normally:

       make CROSS_TO_MACPPC=1 RETRO68=$RETRO68 all

## How it shares the 68k port

- `MAC68K` is auto-defined because `powerpc-apple-macos-gcc` predefines
  `macintosh` (see `include/config1.h`), so the core Mac code paths and
  `include/mac68kconf.h` apply unchanged.  `-DMACPPC` and the predefined
  `__POWERPC__` mark the few CPU-specific spots.
- `-DMAC68K_CROSS` is set for both classic cross-targets — it selects the
  external-`nhdat` `DLBLIB` data path over the legacy Mac-resource `DLBRSRC`,
  and is not a CPU marker.
- The Makefile blocks live in the cross hint files
  (`sys/unix/hints/include/cross-pre1.500`, `cross-pre2.500`,
  `cross-post.500`); `setup.sh` splices them into the generated `src/Makefile`.
  The PPC blocks are parallel duplicates of the 68k ones, kept separate so the
  68k target is untouched.
- Toolbox is linked via the CFM import stub `-lInterfaceLib`.

# Building NetHack for Classic Mac OS (PowerPC, CFM/PEF)

Native PowerPC classic build, parallel to the 68k build documented in
[`BUILD.md`](BUILD.md).  Runs on PowerPC Macs under System 7.x and later
(no Carbon, no Mac OS X).  Cross-compiled with the Retro68
`powerpc-apple-macos` GCC toolchain; the `sys/mac68k` UI sources are shared
verbatim with the 68k port.

## Prerequisites

- Retro68 at `/opt/retro68` (or set `RETRO68=`), including the
  `powerpc-apple-macos-*` tools, `MakePEF`, and `Rez`.
- Apple Universal Interfaces 3.4 in `/opt/retro68/universal/CIncludes`
  (same headers the 68k build uses; see `BUILD.md`).
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

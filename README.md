# NetHack 5.0 on m68k — work in progress

This is a feature branch of [NetHack 5.0](https://www.nethack.org) collecting
revival work for the classic m68k ports (Amiga, Atari ST/TT/Falcon (GEM) and 68k
Macintosh) against the current NetHack-5.0 codebase. The Amiga and Atari ports
were retired in 3.6 and have been catching up to the windowport, save-file, and
Lua level changes that landed since. 

This is where new commits land before being proposed for merge into
`NetHack-5.0`. If you're a player or porter with m68k hardware: I need your
feedback, you can find links to the latest work-in-progress builds below.

## Ports in this branch

### Amiga
**Stable** — part of the official NetHack 5.0 tree. This branch carries any
incremental fixes that should be folded back upstream alongside the Atari and
Mac work.

### Atari ST / TT / Falcon (GEM)
**Feature complete** — cross-compiled from Linux against
[E_GEM 2.2](https://github.com/ingpaschke/EGEM_220), runs under TOS 2.06+,
EmuTOS 1.3.x / 1.4, MagiC 6.2, FreeMiNT (XaAES). Tile mode, ASCII mode,
direct-color graphics at 16/24/32 bpp, resizable map window with tracking
status/message panes. See [`sys/atari/README.tos`](sys/atari/README.tos) and
[`sys/atari/README.Crosscompiling`](sys/atari/README.Crosscompiling).

### 68k Macintosh (Classic Mac OS)
**Feature complete** — the legacy Macintosh port revived against NetHack 5.0,
cross-compiled from Linux with [Retro68](https://github.com/autc04/Retro68).
Targets the classic Toolbox windowport on System 7 and should run on any
68020-or-later Macintosh with at least 8 MB of RAM -- ASCII on black & white
screens, tile mode in 256/16 colors on color-capable machines.  Resizable map
window with in-game tile/ASCII switching, MENUCOLOR, and a bundled
crash-recovery application.  See
[`sys/mac68k/ReadMe.txt`](sys/mac68k/ReadMe.txt) for players and
[`sys/mac68k/BUILD.md`](sys/mac68k/BUILD.md) for building.

## Try it now

Pre-built binaries for the Atari port can be found in my fork:

- **Atari (5.0 WIP)**: <https://github.com/ingpaschke/NetHack/releases/tag/v5.0-atari-wip>
- **68k Mac (5.0)**: <https://github.com/ingpaschke/NetHack/releases/tag/v5.0.0-mac68k>

Builds are tagged WIP and refreshed when fixes land.

## Building from source

See [`sys/atari/README.Crosscompiling`](sys/atari/README.Crosscompiling) for
the Atari ST port and [`sys/mac68k/BUILD.md`](sys/mac68k/BUILD.md) for the 68k
Macintosh port.

## Help wanted

If you have time and m68k hardware (or a configured emulator):

- **Atari testers on real hardware** — most useful on TT, Falcon, and
  Mega ST + accelerator/graphics card setups. Bug reports with reproducer steps and
  hardware/emulator details are gold.
- **MagiC and FreeMiNT users** I regularly test on MagiC on Linux and Magic 6.2
  on Macintosh myself. Feedback from running on FreeMint would be welcome.
- **Alternate VDI drivers** — NVDI, fVDI, and unusual direct-color screens are
  supported, but of course I couldn't test all combinations out there. 
- **Mac testers**

Open issues on [My NetHack fork](https://github.com/ingpaschke/NetHack/issues)
or send patches against this branch.

## Maintainer

Ingo Paschke ([@ingpaschke](https://github.com/ingpaschke),
<ipaschke@lpclabs.de>) — Atari, Amiga, and 68k Macintosh ports.

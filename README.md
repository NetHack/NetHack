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

### 68k Macintosh (Classic Mac OS) (planned, currently not in this repo)
I currently have a working version of the legacy 3.1.3 Macintosh build brought
up to 3.7. Targets the classic Toolbox windowport on System 7 / Mac OS 8 across
Macintosh SE/30 through Quadra. 

I will shift my focus to getting the Macintosh port ready once the Atari port is stable.
I aim for feature parity with the other 68k ports meaning tiles and support
for high resolutions and color. 

## Try it now

Pre-built binaries for the Atari port can be found in my fork:

- **Atari (5.0 WIP)**: <https://github.com/ingpaschke/NetHack/releases/tag/v5.0-atari-wip>
- **68k Mac (3.7 reference)**: <https://github.com/ingpaschke/NetHack/releases/tag/v3.7.0-mac68k>

Builds are tagged WIP and refreshed when fixes land.

## Building from source

See [`sys/atari/README.Crosscompiling`](sys/atari/README.Crosscompiling) for instructions on how to build the Atari ST port.

## Hardware / emulator support

| Target | Status |
|---|---|
| Atari TT, Falcon, Mega ST/STE with ≥8 MB RAM | supported, but I don't have real hardware to test on, feedback appreciated |
| Hatari (TOS 2.06 / 3.06 / 4.04, EmuTOS 1.3.x / 1.4) | regularly tested |
| ARAnyM 1.1+ (FreeMiNT / XaAES) | testers wanted |
| MagiC 6.2 under MagicMac / magic-on-linux | regularly tested |
| Plain ST/STE (4 MB) | won't fit — needs RAM expansion to ≥8 MB |
| 68k Macintosh (System 7 / 8 / 9, Quadra-class for tile mode goal) | basic port working, modernization in progress |

## Help wanted

If you have time and m68k hardware (or a configured emulator):

- **Atari testers on real hardware** — most useful on TT, Falcon, and
  Mega ST + accelerator/graphics card setups. Bug reports with reproducer steps and
  hardware/emulator details are gold.
- **MagiC and FreeMiNT users** I regularly test on MagiC on Linux and Magic 6.2
  on Macintosh myself. Feedback from running on FreeMint would be welcome.
- **Alternate VDI drivers** — NVDI, fVDI, and unusual direct-color screens are
  supported, but of course I couldn't test all combinations out there. 

Open issues on [My NetHack fork](https://github.com/ingpaschke/NetHack/issues)
or send patches against this branch.

## Maintainer

Ingo Paschke ([@ingpaschke](https://github.com/ingpaschke)) — Atari, Amiga,
and 68k Macintosh ports.

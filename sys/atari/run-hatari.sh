#!/bin/sh
# NetHack 5.0	run-hatari.sh	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
# Copyright (c) Ingo Paschke, 2026.
# NetHack may be freely redistributed.  See license for details.
#
# Run NetHack/Atari in Hatari from this directory.
#
# Defaults: STE with 68040 @ 32 MHz, EmuTOS 1.4 (512k), VGA monitor,
# VDI 1024x480x4 (16 colours), sound off (no keyclick), auto-launches
# nethack.prg.
#
# Place this script alongside nethack.prg (the package's extracted
# contents).  Auto-downloads EmuTOS 1.4 into $EMUTOS_CACHE if absent.
#
# Env overrides:
#   HATARI         hatari binary (default: PATH)
#   EMUTOS         ROM path (default: $EMUTOS_CACHE/etos512us.img)
#   EMUTOS_CACHE   download cache dir (default: ~/.cache/emutos)
#   MACHINE        st/ste/tt/falcon (default: ste)
#   CPUCLOCK       MHz, Hatari accepts 8/16/32 (default: 32)
#   CPULEVEL       0=000 1=010 2=020 3=030 4=040 (default: 4)
#   PLANES         VDI bit depth 1/2/4 (default: 4 = 16 colours)
#   VDI_W, VDI_H   VDI screen size (default: 1024x480)
#   SOUND          on|off|<freq> (default: off, kills the keyclick)
#   ZOOM           window scale 1.0-8.0 (default: auto, ~60% of screen)
#   FULL           set to 1 for fullscreen
#   NO_AUTO        set to 1 to skip auto-launching nethack.prg
set -eu

HD_DIR="$(cd "$(dirname "$0")" && pwd)"
[ -f "$HD_DIR/nethack.prg" ] || {
    echo "nethack.prg not found in $HD_DIR" >&2
    echo "Place this script next to nethack.prg (extracted NH500ST.ZIP)." >&2
    exit 1
}

HATARI="${HATARI:-hatari}"
command -v "$HATARI" >/dev/null 2>&1 || {
    echo "hatari not found in PATH (or set HATARI=/path/to/hatari)" >&2
    echo "  Debian/Ubuntu:  sudo apt install hatari" >&2
    echo "  macOS (brew):   brew install hatari" >&2
    exit 1
}

EMUTOS_CACHE="${EMUTOS_CACHE:-$HOME/.cache/emutos}"
EMUTOS="${EMUTOS:-$EMUTOS_CACHE/etos512us.img}"

if [ ! -f "$EMUTOS" ]; then
    echo ">> EmuTOS 1.4 not found at $EMUTOS, downloading..."
    mkdir -p "$EMUTOS_CACHE"
    tmpzip="$(mktemp --suffix=.zip)"
    trap 'rm -f "$tmpzip"' EXIT
    url="https://downloads.sourceforge.net/project/emutos/emutos/1.4/emutos-512k-1.4.zip"
    curl -fL --progress-bar -o "$tmpzip" "$url" || {
        echo "Download failed: $url" >&2
        exit 1
    }
    unzip -joq "$tmpzip" '*/etos512us.img' -d "$EMUTOS_CACHE" || {
        echo "Failed to extract etos512us.img from $tmpzip" >&2
        exit 1
    }
    [ -f "$EMUTOS" ] || {
        echo "etos512us.img missing after extract; check archive contents" >&2
        exit 1
    }
    echo ">> EmuTOS 1.4 installed at $EMUTOS"
fi

MACHINE="${MACHINE:-ste}"
PLANES="${PLANES:-4}"
VDI_W="${VDI_W:-1024}"
VDI_H="${VDI_H:-480}"
SOUND="${SOUND:-off}"
CPUCLOCK="${CPUCLOCK:-32}"
CPULEVEL="${CPULEVEL:-4}"

if [ -z "${ZOOM:-}" ]; then
    SCREEN_W="$(xrandr --current 2>/dev/null | awk '/\*/{print $1; exit}' \
                | cut -dx -f1)"
    if [ -n "$SCREEN_W" ]; then
        ZW=$(( SCREEN_W * 60 / 100 / VDI_W ))
        ZOOM="$ZW"
        [ "$ZOOM" -lt 1 ] && ZOOM=1
    else
        ZOOM=2
    fi
fi

FULL_FLAG=""
[ -n "${FULL:-}" ] && FULL_FLAG="--fullscreen"

AUTO_FLAG=""
[ -z "${NO_AUTO:-}" ] && AUTO_FLAG="--auto C:\\nethack.prg"

exec "$HATARI" \
    --tos "$EMUTOS" \
    --machine "$MACHINE" \
    --memsize 14 \
    --cpulevel "$CPULEVEL" --cpuclock "$CPUCLOCK" --cpu-exact off --fpu 68882 \
    --monitor vga \
    --vdi on --vdi-planes "$PLANES" --vdi-width "$VDI_W" --vdi-height "$VDI_H" \
    --sound "$SOUND" \
    --zoom "$ZOOM" \
    --harddrive "$HD_DIR" \
    $AUTO_FLAG \
    $FULL_FLAG \
    "$@"

#!/bin/sh
# install-toolchain.sh --- fetch & install the m68k-atari-mint cross-toolchain
#
# Default layout (canonical GNU cross-toolchain; everything under
# $PREFIX, gcc finds its sysroot via bindir/../<target>/sys-root):
#
#     $PREFIX/bin/m68k-atari-mint-gcc                       <- host driver
#     $PREFIX/m68k-atari-mint/bin/*                         <- binutils
#     $PREFIX/lib64/gcc/m68k-atari-mint/15/                 <- gcc internals
#     $PREFIX/m68k-atari-mint/sys-root/usr/include/stdio.h  <- mintlib
#     $PREFIX/m68k-atari-mint/sys-root/usr/lib/libc.a       <- mintlib
#     $PREFIX/m68k-atari-mint/sys-root/usr/lib/libm.a       <- fdlibm
#     $PREFIX/m68k-atari-mint/sys-root/usr/lib/libgem.a     <- gemlib
#     $PREFIX/m68k-atari-mint/sys-root/usr/lib/libe_gem.a   <- E_GEM
#
# After install, build NetHack with:
#     make CROSS_TO_ATARI=1 TOOLTOP=$PREFIX/bin package
#
# Tested with the version pins below.  Override with environment vars
# (GCC_VER, BINUTILS_VER, MINTLIB_VER, FDLIBM_VER, GEMLIB_VER) if you
# want a different release.  Versions current as of 2026-05-14:

: "${GCC_VER:=15.2.0-mint-20250810}"
: "${BINUTILS_VER:=2.45-mint-20250812}"
: "${MINTLIB_VER:=0.60.1-9d6-mint}"
: "${FDLIBM_VER:=5.3-46a-mint}"
: "${GEMLIB_VER:=0.44.0-44d-mint}"
: "${EGEM_VER:=2.2.1}"

set -eu

PREFIX=/opt/cross-mint
CACHE=
SUDO=

usage() {
    cat <<EOF
Usage: $0 [--prefix=DIR] [--cache=DIR] [--sudo]

  --prefix DIR    Install root.  Default: /opt/cross-mint
                  Host tools land at DIR/bin/m68k-atari-mint-*, target
                  libs at DIR/m68k-atari-mint/sys-root/usr/...  (Matches
                  the /opt/retro68 / /opt/amiga style of self-contained
                  cross-toolchain trees.)
  --cache DIR     Where to keep downloaded tarballs.  Default: \$PREFIX/.cache
  --sudo          Run mkdir/tar/cp as root (use when PREFIX is system-owned).
  --help          This message.

Toolchain version pins (set as environment variables to override):
  GCC_VER       = $GCC_VER
  BINUTILS_VER  = $BINUTILS_VER
  MINTLIB_VER   = $MINTLIB_VER
  FDLIBM_VER    = $FDLIBM_VER
  GEMLIB_VER    = $GEMLIB_VER
  EGEM_VER      = $EGEM_VER  (binary release from github.com/ingpaschke/EGEM_220)
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix=*) PREFIX="${1#--prefix=}" ;;
        --prefix)   PREFIX="${2:?--prefix needs a value}"; shift ;;
        --cache=*)  CACHE="${1#--cache=}" ;;
        --cache)    CACHE="${2:?--cache needs a value}"; shift ;;
        --sudo)     SUDO=sudo ;;
        --help|-h)  usage; exit 0 ;;
        *)          echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

[ -n "$CACHE" ] || CACHE="$PREFIX/.cache"

GCC_TAR=gcc-$GCC_VER-bin-linux64.tar.xz
BINUTILS_TAR=binutils-$BINUTILS_VER-bin-linux64.tar.xz
MINTLIB_TAR=mintlib-$MINTLIB_VER.tar.bz2
FDLIBM_TAR=fdlibm-$FDLIBM_VER.tar.bz2
GEMLIB_TAR=gemlib-$GEMLIB_VER.tar.bz2
EGEM_ZIP=lib_e_gem_$EGEM_VER.zip

GCC_URL=https://tho-otto.de/download/mint/$GCC_TAR
BINUTILS_URL=https://tho-otto.de/download/mint/$BINUTILS_TAR
MINTLIB_URL=https://tho-otto.de/snapshots/mintlib/$MINTLIB_TAR
FDLIBM_URL=https://tho-otto.de/snapshots/fdlibm/$FDLIBM_TAR
GEMLIB_URL=https://tho-otto.de/snapshots/gemlib/$GEMLIB_TAR
EGEM_URL=https://github.com/ingpaschke/EGEM_220/releases/download/v$EGEM_VER/$EGEM_ZIP

echo "Installing m68k-atari-mint cross-toolchain"
echo "  PREFIX   = $PREFIX"
echo "  CACHE    = $CACHE"
echo

# Need either curl or wget.
if   command -v curl >/dev/null 2>&1; then FETCH="curl -fL --progress-bar -o"
elif command -v wget >/dev/null 2>&1; then FETCH="wget -q --show-progress -O"
else echo "Need curl or wget on PATH." >&2; exit 1
fi
# Need unzip for the E_GEM release.
command -v unzip >/dev/null 2>&1 || { echo "Need unzip on PATH." >&2; exit 1; }

mkdir -p "$CACHE"

download() {
    name=$(basename "$1")
    if [ -s "$CACHE/$name" ]; then
        echo "  cached: $name"
    else
        echo "  fetch:  $name"
        $FETCH "$CACHE/$name" "$1"
    fi
}

download "$GCC_URL"
download "$BINUTILS_URL"
download "$MINTLIB_URL"
download "$FDLIBM_URL"
download "$GEMLIB_URL"
download "$EGEM_URL"

$SUDO mkdir -p "$PREFIX/m68k-atari-mint/sys-root"

echo
echo "Extracting host tools into $PREFIX/  (stripping leading usr/) ..."
$SUDO tar xJf "$CACHE/$GCC_TAR"      -C "$PREFIX" --strip-components=1
$SUDO tar xJf "$CACHE/$BINUTILS_TAR" -C "$PREFIX" --strip-components=1

echo "Extracting target libs into $PREFIX/m68k-atari-mint/sys-root/ ..."
$SUDO tar xjf "$CACHE/$MINTLIB_TAR" -C "$PREFIX/m68k-atari-mint/sys-root"
$SUDO tar xjf "$CACHE/$FDLIBM_TAR"  -C "$PREFIX/m68k-atari-mint/sys-root"
$SUDO tar xjf "$CACHE/$GEMLIB_TAR"  -C "$PREFIX/m68k-atari-mint/sys-root"

echo "Extracting E_GEM into $PREFIX/m68k-atari-mint/sys-root/usr/ ..."
$SUDO unzip -oq "$CACHE/$EGEM_ZIP" -d "$PREFIX/m68k-atari-mint/sys-root/usr"

echo
echo "Smoke test:"
"$PREFIX/bin/m68k-atari-mint-gcc" --version | head -1
"$PREFIX/bin/m68k-atari-mint-gcc" -print-sysroot
ls "$PREFIX/m68k-atari-mint/sys-root/usr/lib/libe_gem.a" \
   "$PREFIX/m68k-atari-mint/sys-root/usr/include/e_gem.h" >/dev/null && \
    echo "E_GEM:    OK"

echo
echo "Done.  Build NetHack with:"
echo "    make CROSS_TO_ATARI=1 TOOLTOP=$PREFIX/bin package"

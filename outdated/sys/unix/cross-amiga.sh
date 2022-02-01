#!/bin/sh
# outdated/sys/unix/cross-amiga.sh
set -x

cp outdated/sys/unix/hints/cross-amiga sys/unix/hints
cp outdated/sys/unix/hints/include/cross-amiga-post sys/unix/hints/include
cp outdated/sys/unix/hints/include/cross-amiga-pre sys/unix/hints/include

CROSSCOMPILER_REPO="https://github.com/bebbo/amiga-gcc"

if [ ! -d "/opt/amiga/bin" ]; then
 export myname="$USER"
 sudo mkdir -p /opt/amiga
 sudo chown $myname /opt/amiga
 mkdir -p lib
 cd lib
 git clone $CROSSCOMPILER_REPO
 cd amiga-gcc
 if [ "$(uname)" = "Darwin" ]; then
    #Mac (git, Xcode and Homebrew assumed to already be installed)
    platform=macOS
    brew install bash wget make lhasa gmp mpfr libmpc flex gettext gnu-sed \
        texinfo gcc@11 make autoconf
    CC=gcc-11 CXX=g++-11 gmake all SHELL=$(brew --prefix)/bin/bash
    special1=$(brew --prefix)/bin/bash
    special=SHELL=$special1
 elif [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
    #Linux (git gcc, g++ assumed to already be installed)
    platform=Linux
    sudo apt install make wget git lhasa libgmp-dev libmpfr-dev \
        libmpc-dev flex bison gettext texinfo ncurses-dev autoconf rsync
 else
    echo "No Amiga cross-compiler for you, sorry."
    exit 1
 fi
 make clean
 make clean-prefix
 make all
 cd ../..
fi

#!/bin/bash -x

if [ x$1 == "xlib" ]; then
    echo Doing lib...
    if [ -f Makefile ]; then
        make spotless
    fi
    cd sys/unix
    ./setup.sh hints/macOS.370
    cd ../..
    make WANT_LIBNH=1
fi

if [ x$1 == "xrunlib" ]; then
    LIBS="-Lsrc -lnh -Llib/lua -llua -lm"
    BADLIBS="-lncurses"
    rm nhlibtest
    gcc -o nhlibtest libtest.c $LIBS $BADLIBS
    ./nhlibtest
fi

if [ x$1 == "xwasm" ]; then
    echo Doing wasm...
    if [ -f Makefile ]; then
        make spotless
    fi
    cd sys/unix
    ./setup.sh hints/macOS.370
    cd ../..
    make CROSS_TO_WASM=1
fi

if [ x$1 == "xrunwasm" ]; then
    cd sys/lib/npm-package && npm run build && node test/test.js
fi

if [ x$1 == "xbin" ]; then
    echo Doing bin...
    make spotless
    cd sys/unix
    ./setup.sh hints/macOS.370
    cd ../..
    make
fi


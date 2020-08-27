#!/bin/bash -x

if [ x$1 == "xlib" ]; then
    echo Doing lib...
    make spotless
    cd sys/lib
    ./setup.sh hints/macOS.2020
    cd ../..
    make
fi

if [ x$1 == "xrunlib" ]; then
    LIBS="-Lsrc -lnethack -Llib/lua -llua -lm"
    BADLIBS="-lncurses"
    rm nhlibtest
    gcc -o nhlibtest libtest.c $LIBS $BADLIBS
    ./nhlibtest
fi

if [ x$1 == "xwasm" ]; then
    echo Doing wasm...
    make spotless
    cd sys/lib
    ./setup.sh hints/wasm
    cd ../..
    make
fi

if [ x$1 == "xrunwasm" ]; then
    cd sys/lib/npm-package && node test/test.js
fi

if [ x$1 == "xbin" ]; then
    echo Doing bin...
    make spotless
    cd sys/unix
    ./setup.sh hints/macOS.2020
    cd ../..
    make
fi


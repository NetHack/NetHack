#!/bin/sh

if [ ! -d lib ]; then
mkdir -p lib
fi

if [ $1 == "pdcursesmod" ]; then
 export CURLPDCSRC=https://github.com/Bill-Gray/PDCursesMod/archive/refs/tags/v4.4.0.zip
 export CURLPDCDST=pdcursesmod.zip

 if [ ! -f lib/pdcursesmod/curses.h ] ; then
	cd lib
	curl -L $CURLPDCSRC -o $CURLPDCDST
	/c/Windows/System32/tar -xvf $CURLPDCDST
	mkdir -p pdcursesmod
	/c/Windows/System32/tar -C pdcursesmod --strip-components=1 -xvf $CURLPDCDST
	cd ..
 fi
fi

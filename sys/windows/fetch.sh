#!/bin/sh

if [ ! -d lib ]; then
mkdir -p lib
fi

if [ $1 == "lua" ]; then
 if [ -z "$LUA_VERSION" ]; then
 export LUA_VERSION=5.4.6
 export LUASRC=../lib/lua
 fi

 export CURLLUASRC=http://www.lua.org/ftp/lua-5.4.6.tar.gz
 export CURLLUADST=lua-5.4.6.tar.gz

 if [ ! -f lib/lua.h ] ;then
	cd lib
	curl -L $CURLLUASRC -o $CURLLUADST
	/c/Windows/System32/tar -xvf $CURLLUADST
	cd ..
 fi
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

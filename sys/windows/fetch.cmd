@echo off
if not exist lib\* mkdir lib

if [%1] == [lua] (
   set LUA_VERSION=5.4.6
   set LUASRC=../lib/lua
   set CURLLUASRC=http://www.lua.org/ftp/lua-%LUA_VERSION%.tar.gz
   set CURLLUADST=lua-%LUA_VERSION.tar.gz
   if NOT exist lib/lua.h (
       cd lib
       curl -L %CURLLUASRC% -o %CURLLUADST%
       tar -xvf %CURLLUADST%
       cd ..
   )
   echo Lua placed in lib/lua-%LUA_VERSION%.tar.gz
)

if [%1] == [pdcursesmod] (
    set PDCVERSION=4.4.0
    set CURLPDCSRC=https://github.com/Bill-Gray/PDCursesMod/archive/refs/tags/v%PDCVERSION%.zip
    set CURLPDCDST=pdcursesmod.zip
    if NOT exist lib/pdcursesmod/curses.h (
        cd lib
        curl -L %CURLPDCSRC% -o %CURLPDCDST%
        tar -xvf %CURLPDCDST%
        mkdir pdcursesmod
        tar -C pdcursesmod --strip-components=1 -xvf %CURLPDCDST%
        cd ..
    )
)

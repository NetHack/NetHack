@echo on

if exist %cd%\..\sys\windows\Makefile.nmake set LIBDIR=..\lib
if exist %cd%\Makefile.nmake set LIBDIR=..\..\lib
if exist %cd%\sys\windows\Makefile.nmake set LIBDIR=lib

set LUA_VERSION=5.4.8
set LUASRC=%LIBDIR%/lua
set CURLLUASRC=http://www.lua.org/ftp/lua-%LUA_VERSION%.tar.gz
set CURLLUADST=lua-%LUA_VERSION%.tar.gz

set PDCVERSION=4.5.4
set CURLPDCSRC=https://github.com/Bill-Gray/PDCursesMod/archive/refs/tags/v%PDCVERSION%.zip
set CURLPDCDST=pdcursesmod.zip

:proceed

if not exist %LIBDIR%\* mkdir lib

if [%1] == [lua] (
   if NOT exist %LIBDIR%\lua.h (
       pushd %LIBDIR%
       curl -L %CURLLUASRC% -o %CURLLUADST%
       tar -xvf %CURLLUADST%
       popd
   )
   echo Lua placed in %LIBDIR%\lua-%LUA_VERSION%.tar.gz
)

if [%1] == [pdcursesmod] (
    if NOT exist %LIBDIR%\pdcursesmod\curses.h (
        pushd %LIBDIR%
        curl -L %CURLPDCSRC% -o %CURLPDCDST%
        mkdir pdcursesmod
        tar -C pdcursesmod --strip-components=1 -xvf %CURLPDCDST%
        popd
    )
    echo pdcursesmod placed in %LIBDIR%\pdcursesmod
)

:done

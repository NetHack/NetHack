:$NHDT-Header: NetHack 5.0 fetch.cmd 2026/09/25_21:05:38 NetHack-5.0:1.5 $
:Copyright (c) Michael Allison, 2024
:NetHack may be freely redistributed.  See license for details.

@echo on

if exist %cd%\..\sys\windows\Makefile.nmake set LIBDIR=..\lib
if exist %cd%\Makefile.nmake set LIBDIR=..\..\lib
if exist %cd%\sys\windows\Makefile.nmake set LIBDIR=lib

set PDCVERSION=4.5.4
set CURLPDCSRC=https://github.com/Bill-Gray/PDCursesMod/archive/refs/tags/v%PDCVERSION%.zip
set CURLPDCDST=pdcursesmod.zip

:proceed

if not exist %LIBDIR%\* mkdir lib

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

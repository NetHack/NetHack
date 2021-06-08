@REM  NetHack 3.7	nhsetup.bat	$NHDT-Date: 1596498315 2020/08/03 23:45:15 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.40 $ */
@REM  Copyright (c) NetHack PC Development Team 1993-2021
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.windows for details
@REM
@echo off
pushd %~dp0
set WIN32PATH=..\..\win\win32
set BINPATH=..\..\binary
set VCDir=

goto :main

:dirname
rem Get the dirname of the second argument and set the variable who's
rem name was specified in the first argument.
call set %~1=%%~dp2
call set %~1=%%%~1:~0,-1%%
goto :EOF

:main

echo Checking to see if source tree directories are set up properly...
if not exist ..\..\include\hack.h goto :err_dir
if not exist ..\..\src\hack.c goto :err_dir
if not exist ..\..\dat\wizard1.lua goto :err_dir
if not exist ..\..\util\makedefs.c goto :err_dir
if not exist ..\..\sys\windows\windsys.c goto :err_dir
if not exist ..\..\win\win32\mhmain.c goto :err_dir
echo Directories look ok.

:movemakes
echo Moving Makefiles into ..\..\src for those not using Visual Studio
REM Some file movemet for those that still want to use MAKE or NMAKE and a Makefile
:do_tty
if NOT exist %BINPATH%\*.* mkdir %BINPATH%
if NOT exist %BINPATH%\license copy ..\..\dat\license %BINPATH%\license >nul
echo Copying Microsoft Makefile - Makefile.msc to ..\..\src\Makefile...
if NOT exist ..\..\src\Makefile goto :domsc
copy ..\..\src\Makefile ..\..\src\Makefile-orig >nul
echo      Your existing
echo           ..\..\src\Makefile
echo      has been renamed to
echo           ..\..\src\Makefile-orig
:domsc
copy Makefile.msc ..\..\src\Makefile >nul
echo Microsoft Makefile copied ok.

echo Copying MinGW Makefile - Makefile.gcc to ..\..\src\Makefile.gcc...
if NOT exist ..\..\src\Makefile.gcc goto :dogcc
copy ..\..\src\Makefile.gcc ..\..\src\Makefile.gcc-orig >nul
echo      Your existing
echo           ..\..\src\Makefile.gcc
echo      has been renamed to
echo           ..\..\src\Makefile.gcc-orig
:dogcc
copy Makefile.gcc ..\..\src\Makefile.gcc >nul
echo MinGW Makefile copied ok.

echo Done copying files.
goto :done

:err_dir
echo Your directories are not set up properly, please re-read the
echo documentation and sys/windows/Install.windows.
goto :fini

:done
echo done!
echo.
echo Proceed with the next step documented in Install.windows 
echo.

:fini
:end
set _pause=N
for %%x in (%cmdcmdline%) do if /i "%%~x"=="/c" set _pause=Y
if "%_pause%"=="Y" pause
set _pause=
popd

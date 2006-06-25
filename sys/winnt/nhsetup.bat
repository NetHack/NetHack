@REM  SCCS Id: @(#)nhsetup.bat  3.5     $Date$
@REM  Copyright (c) NetHack PC Development Team 1993-2006
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.nt for details
@REM
@echo off

set _pause=

:nxtcheck
echo Checking to see if directories are set up properly
if not exist ..\..\include\hack.h goto :err_dir
if not exist ..\..\src\hack.c goto :err_dir
if not exist ..\..\dat\wizard.des goto :err_dir
if not exist ..\..\util\makedefs.c goto :err_dir
if not exist ..\..\sys\winnt\winnt.c goto :err_dir
echo Directories look ok.

:do_tty
if NOT exist ..\..\binary\*.* mkdir ..\..\binary
if NOT exist ..\..\binary\license copy ..\..\dat\license ..\..\binary\license >nul
echo Copying Microsoft Makefile - Makefile.msc to ..\..\src\Makefile.
if NOT exist ..\..\src\Makefile goto :domsc
copy ..\..\src\Makefile ..\..\src\Makefile-orig >nul
echo      Your existing
echo           ..\..\src\Makefile
echo      has been renamed to
echo           ..\..\src\Makefile-orig
:domsc
copy Makefile.msc ..\..\src\Makefile >nul
echo Microsoft Makefile copied ok.

echo Copying Borland Makefile - Makefile.bcc to ..\..\src\Makefile.bcc
if NOT exist ..\..\src\Makefile.bcc goto :dobor
copy ..\..\src\Makefile.bcc ..\..\src\Makefile.bcc-orig >nul
echo      Your existing 
echo           ..\..\src\Makefile.bcc 
echo      has been renamed to 
echo           ..\..\src\Makefile.bcc-orig
:dobor
copy Makefile.bcc ..\..\src\Makefile.bcc >nul
echo Borland Makefile copied ok.

echo Copying MinGW Makefile - Makefile.gcc to ..\..\src\Makefile.gcc
if NOT exist ..\..\src\Makefile.gcc goto :dogcc
copy ..\..\src\Makefile.gcc ..\..\src\Makefile.gcc-orig >nul
echo      Your existing
echo           ..\..\src\Makefile.gcc
echo      has been renamed to
echo           ..\..\src\Makefile.gcc-orig
:dogcc
copy Makefile.gcc ..\..\src\Makefile.gcc >nul
echo MinGW Makefile copied ok.

:do_win
if not exist ..\..\win\win32\nethack.dsw goto :err_win
if not exist ..\..\win\win32\nethack.sln goto :err_win

echo.
if exist ..\..\build\*.* goto projectcopy

echo Creating ..\..\build directory
mkdir ..\..\build

:projectcopy
echo Copying Visual C solution files to top level directory
@REM Visual Studio 6 workspace
echo Copying ..\..\win\win32\nethack.dsw  ..\..\nethack.dsw
copy ..\..\win\win32\nethack.dsw  ..\.. >nul

@REM Visual Studio 2005 Express solution file
if NOT exist ..\..\win\win32\nethack.sln goto skipsoln
echo Copying ..\..\win\win32\nethack.sln  ..\..\nethack.sln
copy ..\..\win\win32\nethack.sln  ..\.. >nul
:skipsoln

if NOT exist ..\..\binary\*.* echo Creating ..\..\binary directory
if NOT exist ..\..\binary\*.* mkdir ..\..\binary
if NOT exist ..\..\binary\license copy ..\..\dat\license ..\..\binary\license >nul

echo Copying Visual C project files to ..\..\build directory

copy ..\..\win\win32\dgnstuff.mak  ..\..\build >nul
copy ..\..\win\win32\levstuff.mak  ..\..\build >nul
copy ..\..\win\win32\tiles.mak     ..\..\build >nul

@REM Visual C++ 6 project files
copy ..\..\win\win32\dgncomp.dsp   ..\..\build >nul
copy ..\..\win\win32\dgnstuff.dsp  ..\..\build >nul
copy ..\..\win\win32\dlb_main.dsp  ..\..\build >nul
copy ..\..\win\win32\levcomp.dsp   ..\..\build >nul
copy ..\..\win\win32\levstuff.dsp  ..\..\build >nul
copy ..\..\win\win32\makedefs.dsp  ..\..\build >nul
copy ..\..\win\win32\recover.dsp   ..\..\build >nul
copy ..\..\win\win32\tile2bmp.dsp  ..\..\build >nul
copy ..\..\win\win32\tiles.dsp     ..\..\build >nul
copy ..\..\win\win32\tilemap.dsp   ..\..\build >nul
copy ..\..\win\win32\uudecode.dsp   ..\..\build >nul
copy ..\..\win\win32\nethackw.dsp   ..\..\build >nul

@REM Visual C++ 2005 Express project files
if NOT exist ..\..\win\win32\makedefs.vcproj goto skipVC2005
if NOT exist ..\..\win\win32\dgncomp.vcproj goto skipVC2005
if NOT exist ..\..\win\win32\dlb_main.vcproj goto skipVC2005
if NOT exist ..\..\win\win32\nethackw.vcproj goto skipVC2005
copy ..\..\win\win32\dgncomp.vcproj   ..\..\build >nul
copy ..\..\win\win32\dgnstuff.vcproj  ..\..\build >nul
copy ..\..\win\win32\dlb_main.vcproj  ..\..\build >nul
copy ..\..\win\win32\levcomp.vcproj   ..\..\build >nul
copy ..\..\win\win32\levstuff.vcproj  ..\..\build >nul
copy ..\..\win\win32\makedefs.vcproj  ..\..\build >nul
copy ..\..\win\win32\recover.vcproj   ..\..\build >nul
copy ..\..\win\win32\tile2bmp.vcproj  ..\..\build >nul
copy ..\..\win\win32\tiles.vcproj     ..\..\build >nul
copy ..\..\win\win32\tilemap.vcproj   ..\..\build >nul
copy ..\..\win\win32\uudecode.vcproj   ..\..\build >nul
copy ..\..\win\win32\nethackw.vcproj   ..\..\build >nul
:skipVC2005

goto :done

:err_win
echo Some of the files needed to build graphical NetHack
echo for Windows are not in the expected places.
echo Check "Install.nt" for a list of the steps required 
echo to build NetHack.
goto :fini

:err_data
echo A required file ..\..\dat\data.bas seems to be missing.
echo Check "Files." in the root directory for your NetHack distribution
echo and make sure that all required files exist.
goto :fini

:err_dir
echo Your directories are not set up properly, please re-read the
echo documentation and sys/winnt/Install.nt.
goto :fini

:done
echo done!
echo.
echo Proceed with the next step documented in Install.nt 
echo.

:fini
:end
set _pause=Y
if "%0"=="nhsetup" set _pause=N
if "%0"=="NHSETUP" set _pause=N
if "%_pause%"=="Y" pause
set _pause=

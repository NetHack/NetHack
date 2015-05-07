@REM  NetHack 3.6	nhsetup.bat	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
@REM  NetHack 3.6	nhsetup.bat	$Date: 2010/09/05 14:22:16 $  $Revision: 1.21 $ */
@REM  Copyright (c) NetHack PC Development Team 1993-2010
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
if not exist ..\..\win\win32\nethack.sln goto :err_win

echo.
if exist ..\..\build\*.* goto projectcopy

echo Creating ..\..\build directory
mkdir ..\..\build

:projectcopy

@REM Visual Studio Express solution file
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


@REM Visual C++ 2010 Express project files
:VC2010
if NOT exist ..\..\win\win32\makedefs.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\tile2bmp.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\tilemap.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\uudecode.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\NetHackW.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\dgncomp.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\dgnstuff.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\dlb_main.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\levcomp.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\levstuff.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\recover.vcxproj goto skipVC2010
if NOT exist ..\..\win\win32\tiles.vcxproj goto skipVC2010

copy ..\..\win\win32\makedefs.vcxproj ..\..\build >nul
copy ..\..\win\win32\tile2bmp.vcxproj ..\..\build >nul
copy ..\..\win\win32\tilemap.vcxproj ..\..\build >nul
copy ..\..\win\win32\uudecode.vcxproj ..\..\build >nul
copy ..\..\win\win32\NetHackW.vcxproj ..\..\build >nul
copy ..\..\win\win32\dgncomp.vcxproj ..\..\build >nul
copy ..\..\win\win32\dgnstuff.vcxproj ..\..\build >nul
copy ..\..\win\win32\dlb_main.vcxproj ..\..\build >nul
copy ..\..\win\win32\levcomp.vcxproj ..\..\build >nul
copy ..\..\win\win32\levstuff.vcxproj ..\..\build >nul
copy ..\..\win\win32\recover.vcxproj ..\..\build >nul
copy ..\..\win\win32\tiles.vcxproj ..\..\build >nul
:skipVC2010

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

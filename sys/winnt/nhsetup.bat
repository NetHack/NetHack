@REM  NetHack 3.6	nhsetup.bat	$NHDT-Date: 1432512794 2015/05/25 00:13:14 $  $NHDT-Branch: master $:$NHDT-Revision: 1.33 $ */
@REM  Copyright (c) NetHack PC Development Team 1993-2015
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.nt for details
@REM
@echo off
pushd %~dp0
set WIN32PATH=..\..\win\win32
set BUILDPATH=..\..\build
set BINPATH=..\..\binary
set VCDir=

:studiocheck
@REM Set fallbacks here for 32-bit VS2010
SET REGTREE=HKLM\Software\Microsoft\VCExpress\12.0\Setup\VC
SET MSVCVERSION=2010

@REM if we're in a 64-bit cmd prompt, gotta include Wow6432Node
echo Checking for 64-bit environment...
if "%ProgramFiles%" NEQ "%ProgramFiles(x86)%" SET REGTREE=HKLM\Software\Wow6432Node\Microsoft\VCExpress\12.0\Setup\VC

@REM i can see your house from here... or at least your VC++ folder
echo Checking version of VC++ installed...
echo Checking for VC2013 Express...
for /f "usebackq skip=2 tokens=1-2*" %%a IN (`reg query %REGTREE% /v ProductDir`) do @set VCDir="%%c"
if not defined VCDir goto :othereditions
if not exist %VCDir% goto :othereditions

set MSVCVERSION=2013
goto :fallback

:othereditions
@REM TODO: teach ourselves to detect full versions of Studio, which are under a different registry hive
echo VC2013 Express not found; dropping back.

:fallback
echo Using VS%MSVCVERSION%.
set SRCPATH=%WIN32PATH%\vs%MSVCVERSION%

:nxtcheck
echo Checking to see if directories are set up properly...
if not exist ..\..\include\hack.h goto :err_dir
if not exist ..\..\src\hack.c goto :err_dir
if not exist ..\..\dat\wizard.des goto :err_dir
if not exist ..\..\util\makedefs.c goto :err_dir
if not exist ..\..\sys\winnt\winnt.c goto :err_dir
echo Directories look ok.

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

:do_win
if not exist %SRCPATH%\nethack.sln goto :err_win

echo.
if exist %BUILDPATH%\*.* goto projectcopy

echo Creating %BUILDPATH% directory...
mkdir %BUILDPATH%

:projectcopy

@REM Visual Studio Express solution file
if NOT exist %SRCPATH%\nethack.sln goto skipsoln
echo Copying %SRCPATH%\nethack.sln to ..\..\nethack.sln...
copy %SRCPATH%\nethack.sln  ..\.. >nul
:skipsoln

if NOT exist %BINPATH%\*.* echo Creating %BINPATH% directory...
if NOT exist %BINPATH%\*.* mkdir %BINPATH%
if NOT exist %BINPATH%\license copy ..\..\dat\license %BINPATH%\license >nul

echo Copying Visual C project files to %BUILDPATH% directory...

copy %WIN32PATH%\dgnstuff.mak  %BUILDPATH% >nul
copy %WIN32PATH%\levstuff.mak  %BUILDPATH% >nul
copy %WIN32PATH%\tiles.mak     %BUILDPATH% >nul

@REM Visual C++ 201X Express project files
:vcexpress
if NOT exist %SRCPATH%\makedefs.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\tile2bmp.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\tilemap.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\uudecode.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\NetHackW.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\NetHack.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\dgncomp.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\dgnstuff.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\dlb_main.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\levcomp.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\levstuff.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\recover.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\tiles.vcxproj goto skipvcexpress
if NOT exist %SRCPATH%\nhdefkey.vcxproj goto skipvcexpress

copy %SRCPATH%\makedefs.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\tile2bmp.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\tilemap.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\uudecode.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\NetHackW.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\NetHack.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\dgncomp.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\dgnstuff.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\dlb_main.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\levcomp.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\levstuff.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\recover.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\tiles.vcxproj %BUILDPATH% >nul
copy %SRCPATH%\nhdefkey.vcxproj %BUILDPATH% >nul
echo LIBRARY nhdefkey >%BUILDPATH%\nhdefkey64.def
echo LIBRARY nhdefkey >%BUILDPATH%\nhdefkey.def
echo EXPORTS >>%BUILDPATH%\nhdefkey.def
echo ProcessKeystroke >>%BUILDPATH%\nhdefkey.def
echo NHkbhit >>%BUILDPATH%\nhdefkey.def
echo CheckInput >>%BUILDPATH%\nhdefkey.def 
echo SourceWhere >>%BUILDPATH%\nhdefkey.def
echo SourceAuthor >>%BUILDPATH%\nhdefkey.def
echo KeyHandlerName >>%BUILDPATH%\nhdefkey.def

echo Done copying files.
:skipvcexpress

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
set _pause=N
for %%x in (%cmdcmdline%) do if /i "%%~x"=="/c" set _pause=Y
if "%_pause%"=="Y" pause
set _pause=
popd

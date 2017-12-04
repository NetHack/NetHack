@REM  NetHack 3.6	nhsetup.bat	$NHDT-Date: 1432512794 2015/05/25 00:13:14 $  $NHDT-Branch: master $:$NHDT-Revision: 1.33 $ */
@REM  Copyright (c) NetHack PC Development Team 1993-2017
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.nt for details
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
if not exist ..\..\dat\wizard.des goto :err_dir
if not exist ..\..\util\makedefs.c goto :err_dir
if not exist ..\..\sys\winnt\winnt.c goto :err_dir
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

:vscheck2017
SET REGTREE=HKLM\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7
@REM i can see your house from here... or at least your VC++ folder
echo Checking version of VC++ installed...
echo Checking for VC2017 Community Edition...
for /f "usebackq skip=2 tokens=1-2*" %%a IN (`reg query %REGTREE% /v 15.0`) do @set VCDir="%%c"
if not defined VCDir goto :vscheck2015
if not exist %VCDir% goto :vscheck2015
set MSVCVERSION=2017
goto :fallback

:vscheck2015
rem cannot use the registry trick used for vc2017
rem 14 = 2015
SET VCVERS=14
rem Finally, let's determine the root folder for this VC installation.
set VCROOT=%%VS%VCVERS%0COMNTOOLS%%
if "%VCROOT:~-1%"=="\" set VCROOT=%VCROOT:~0,-1%
rem VCROOT=VSDir\Common7\Tools
call :dirname VCROOT "%VCROOT%"
rem VCROOT=VSDir\Common7
call :dirname VCROOT "%VCROOT%"
rem VCROOT=VSDir
set VCDir=%VCROOT%\VC
SET MSVCVERSION=2015

:fallback
echo Using VS%MSVCVERSION%.
set SRCPATH=%WIN32PATH%\vs%MSVCVERSION%
echo NetHack VS%MSVCVERSION% project files are in %SRCPATH%
goto :done

:err_win
echo Some of the files needed to build graphical NetHack
echo for Windows are not in the expected places.
echo Check "Install.nt" for a list of the steps required 
echo to build NetHack.
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

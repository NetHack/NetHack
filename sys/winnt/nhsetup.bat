@REM  SCCS Id: @(#)nhsetup.bat      2002/02/28
@REM  Copyright (c) NetHack PC Development Team 1993, 1996, 2002
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.nt for details
@REM
@echo off

set err_copy=
set opt=

echo Checking to see if directories are set up properly
if not exist ..\..\include\hack.h goto err_dir
if not exist ..\..\src\hack.c goto err_dir
if not exist ..\..\dat\wizard.des goto err_dir
if not exist ..\..\util\makedefs.c goto err_dir
if not exist ..\..\sys\winnt\winnt.c goto err_dir
echo Directories look ok.

if "%1"=="tty"  goto do_tty
if "%1"=="TTY"  goto do_tty
if "%1"=="win"  goto do_win
if "%1"=="WIN"  goto do_win
if "%1"=="gui"  goto do_win
if "%1"=="GUI"  goto do_win
goto err_set

:do_tty
set opt=NetHack for NT Console
echo "Copying Makefile.NT to ..\..\src\Makefile"
copy makefile.NT ..\..\src\Makefile >nul
echo Makefile copied ok.
echo done!
echo.
echo Proceed with the next step documented in Install.nt
echo.
goto done

:do_win
set opt=Graphical NetHack for Windows
if not exist ..\..\win\win32\nethack.dsw goto err_win

echo.
echo "Copying Visual C project files file to ..\..\build directory"

REM copy ..\..\win\win32\winnt.dsw ..\.. >nul
echo copy ..\..\win\win32\nethack.dsw  ..\..
copy ..\..\win\win32\nethack.dsw  ..\..

if NOT exist ..\..\build\*.* mkdir ..\..\build
copy ..\..\win\win32\dgncomp.dsp   ..\..\build >nul
copy ..\..\win\win32\dgnstuff.dsp  ..\..\build >nul
copy ..\..\win\win32\dgnstuff.mak  ..\..\build >nul
copy ..\..\win\win32\dlb_main.dsp  ..\..\build >nul
copy ..\..\win\win32\levcomp.dsp   ..\..\build >nul
copy ..\..\win\win32\levstuff.dsp  ..\..\build >nul
copy ..\..\win\win32\levstuff.mak  ..\..\build >nul
copy ..\..\win\win32\makedefs.dsp  ..\..\build >nul
copy ..\..\win\win32\recover.dsp   ..\..\build >nul
copy ..\..\win\win32\tile2bmp.dsp  ..\..\build >nul
copy ..\..\win\win32\tiles.dsp     ..\..\build >nul
copy ..\..\win\win32\tiles.mak     ..\..\build >nul
copy ..\..\win\win32\tilemap.dsp   ..\..\build >nul
copy ..\..\win\win32\uudecode.dsp   ..\..\build >nul
copy ..\..\win\win32\nethackw.dsp   ..\..\build >nul

goto done

:err_win
echo Some of the files needed to build graphical NetHack
echo for Windows are not in the expected places.
echo Check "Install.nt" for a list of the steps required 
echo to build NetHack.
goto done

:err_data
echo A required file ..\..\dat\data.bas seems to be missing.
echo Check "Files." in the root directory for your NetHack distribution
echo and make sure that all required files exist.
goto done

:err_dir
echo Your directories are not set up properly, please re-read the
echo documentation.
goto done

:err_set
echo.
echo Usage:
echo "%0 <TTY | win | CE >"
echo.
echo    Run this batch file specifying one of the following:
echo            tty, win, ce
echo.
echo    The tty argument is for preparing to build a console I/O TTY version
echo    of NetHack.
echo.
echo    The win argument is for preparing to build a graphical version
echo    of NetHack.
echo.
echo    The CE argument is for preparing to build a Windows CE version
echo    of NetHack.
echo.
goto end

:done
echo done!
echo.
echo Proceed with the next step documented in Install.nt
echo  for building %opt%.
echo.

:fini
:end


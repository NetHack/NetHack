@REM  SCCS Id: @(#)nhsetup.bat      2002/01/13
@REM  Copyright (c) NetHack PC Development Team 1993, 1996, 2002
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.nt for details
@REM
@echo off

set err_nouu=
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
if exist .\nethack.ico goto hasicon
if exist .\nhico.uu uudecode nhico.uu >nul
if exist .\nethack.ico goto hasicon
set err_nouu=Y
goto done
:hasicon
echo NetHack icon exists ok.
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
if not exist ..\..\win\win32\winnt.dsw goto err_win
echo "Copying Visual C project files to ..\..\winhacknt directory"
if NOT exist ..\..\winhacknt\*.* mkdir ..\..\winhacknt
REM copy ..\..\win\win32\winnt.dsw ..\.. >nul
copy ..\..\win\win32\nethack.dsw ..\.. >nul
copy ..\..\win\win32\dgncomp.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\dgnstuff.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\dgnstuff.mak ..\..\winhacknt >nul
copy ..\..\win\win32\dlb_main.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\levcomp.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\levstuff.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\levstuff.mak ..\..\winhacknt >nul
copy ..\..\win\win32\makedefs.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\mhaskyn.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhaskyn.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhcmd.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhcmd.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhdlg.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhdlg.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhfont.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhfont.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhinput.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhinput.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhmain.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhmain.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhmap.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhmap.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhmenu.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhmenu.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhmsg.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhmsgwnd.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhmsgwnd.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhrip.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhrip.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhstatus.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhstatus.h ..\..\winhacknt >nul
copy ..\..\win\win32\mhtext.c ..\..\winhacknt >nul
copy ..\..\win\win32\mhtext.h ..\..\winhacknt >nul
copy ..\..\win\win32\mswproc.c ..\..\winhacknt >nul
copy ..\..\win\win32\recover.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\resource.h ..\..\winhacknt >nul
copy ..\..\win\win32\tile2bmp.c ..\..\winhacknt >nul
copy ..\..\win\win32\tile2bmp.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\tilemap.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\winMS.h ..\..\winhacknt >nul
copy ..\..\win\win32\winhack.c ..\..\winhacknt >nul
copy ..\..\win\win32\winhack.dsp ..\..\winhacknt >nul
copy ..\..\win\win32\winhack.h ..\..\winhacknt >nul
copy ..\..\win\win32\winhack.rc ..\..\winhacknt >nul

if exist ..\..\winhacknt\mnsel.bmp goto hasmnsel2
if exist .\mnsel.bmp goto hasmnsel1
if exist ..\..\win\win32\mnsel.uu uudecode ..\..\win\win32\mnsel.uu >nul
if exist .\mnsel.bmp goto hasmnsel1
echo Error - No UUDECODE utility to decode ..\..\win\win32\mnsel.uu
goto hasmnsel2
:hasmnsel1
echo copy .\mnsel.bmp ..\..\winhacknt
copy .\mnsel.bmp ..\..\winhacknt
:hasmnsel2
if NOT exist ..\..\winhacknt\mnsel.bmp set err_nouu=Y

if exist ..\..\winhacknt\mnunsel.bmp goto hasmnuns2
if exist .\mnunsel.bmp goto hasmnuns1
if exist ..\..\win\win32\mnunsel.uu uudecode ..\..\win\win32\mnunsel.uu >nul
if exist .\mnunsel.bmp goto hasmnuns1
echo Error - No UUDECODE utility to decode ..\..\win\win32\mnunsel.uu
goto hasmnuns2
:hasmnuns1
echo copy .\mnsel.bmp ..\..\winhacknt
copy .\mnunsel.bmp ..\..\winhacknt
:hasmnuns2
if NOT exist ..\..\winhacknt\mnunsel.bmp set err_nouu=Y

if exist ..\..\winhacknt\nethack.ico goto hasicon2
if exist .\nethack.ico goto hasicon1
if exist .\nhico.uu uudecode nhico.uu >nul
if exist .\nethack.ico goto hasicon1
echo Error - No UUDECODE utility to decode nhico.uu
goto hasicon2
:hasicon1
echo copy .\nethack.ico ..\..\winhacknt
copy .\nethack.ico ..\..\winhacknt
:hasicon2
if NOT exist ..\..\winhacknt\nethack.ico set err_nouu=Y

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
echo "%0 <TTY | WINHACK | CE >"
echo.
echo    Run this batch file specifying one of the following:
echo            tty, winhack, ce
echo.
echo    The tty argument is for preparing to build a console I/O TTY version
echo    of NetHack.
echo.
echo    The winhack argument is for preparing to build a graphical version
echo    of NetHack.
echo.
echo    The CE argument is for preparing to build a Windows CE version
echo    of NetHack.
echo.
goto end

:done
echo done!
echo.
if "%err_nouu%"=="" echo Proceed with the next step documented in Install.nt
if "%err_nouu%"=="" echo  for building %opt%.
echo.
if "%err_nouu%"=="" goto fini
echo Apparently you have no UUDECODE utility in your path.  
echo You need a UUDECODE utility in order to turn several .uu files
echo into their decoded binary versions.
echo Check "Install.nt" for a list of prerequisites for building NetHack.

:fini
:end

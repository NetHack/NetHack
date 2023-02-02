@echo off
REM    NetHack 3.7 setup.bat   $NHDT-Date: 1596498274 2020/08/03 23:44:34 $ $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.18 $
REM    Copyright (c) NetHack PC Development Team 1990 - 2019
REM    NetHack may be freely redistributed.  See license for details.

echo.
echo   Copyright (c) NetHack PC Development Team 1990 - 2019
echo   NetHack may be freely redistributed.  See license for details.
echo.
REM setup batch file for msdos, see Install.dos for details.

if not %1.==. goto ok_parm
goto err_set

:ok_parm
echo Checking to see if directories are set up properly ...
if not exist ..\..\include\hack.h  goto err_dir
if not exist ..\..\src\hack.c      goto err_dir
if not exist ..\..\dat\wizard1.lua  goto err_dir
if not exist ..\..\util\makedefs.c goto err_dir
if not exist ..\..\win\tty\wintty.c goto err_dir
echo Directories OK.

if not exist ..\..\binary\* mkdir ..\..\binary
if NOT exist ..\..\binary\license copy ..\..\dat\license ..\..\binary\license >nul

if exist ..\..\dat\data.bas goto long1ok
if exist ..\..\dat\data.base goto long1a
if exist ..\..\dat\data~1.bas goto long1b
goto err_long
:long1a
echo Changing some long-named distribution file names:
echo "Renaming ..\..\dat\data.base -> ..\..\dat\data.bas"
ren ..\..\dat\data.base data.bas
goto long1ok
:long1b
echo Changing some long-named distribution file names:
echo "Renaming ..\..\dat\data~1.bas -> ..\..\dat\data.bas"
ren ..\..\dat\data~1.bas data.bas
:long1ok

if exist ..\..\include\patchlev.h goto long2ok
if exist ..\..\include\patchlevel.h goto long2a
if exist ..\..\include\patchl~1.h goto long2b
goto err_long
:long2a
echo "Renaming ..\..\include\patchlevel.h -> ..\..\include\patchlev.h"
ren ..\..\include\patchlevel.h patchlev.h
goto long2ok
:long2b
echo "Renaming ..\..\include\patchl~1.h -> ..\..\include\patchlev.h"
ren ..\..\include\patchl~1.h patchlev.h
:long2ok

REM Missing guidebook is not fatal to the build process
if exist ..\..\doc\guideboo.txt goto long3ok
if exist ..\..\doc\guidebook.txt goto long3a
if exist ..\..\doc\guideb~1.txt goto long3b
goto warn3long
:long3a
echo "Copying ..\..\doc\guidebook.txt -> ..\..\doc\guidebk.txt"
ren ..\..\doc\guidebook.txt guidebk.txt
goto long3ok
:long3b
echo "Copying ..\..\doc\guideb~1.txt -> ..\..\doc\guidebk.txt"
ren ..\..\doc\guideb~1.txt guidebk.txt
goto long3ok
:warn3long
echo "Warning - There is no NetHack Guidebook (..\..\doc\guidebk.txt)"
echo "          included in your distribution.  Build will proceed anyway."
:long3ok

if exist ..\..\sys\share\posixreg.c goto long4ok
if exist ..\..\sys\share\posixregex.c goto long4a
if exist ..\..\sys\share\posixr~1.c goto long4b
goto err_long
:long4a
echo "Renaming ..\..\sys\share\posixregex.c -> ..\..\sys\share\posixreg.c"
ren ..\..\sys\share\posixregex.c posixreg.c
goto long4ok
:long4b
echo "Renaming ..\..\sys\share\posixr~1.c -> ..\..\sys\share\posixreg.c"
ren ..\..\sys\share\posixr~1.c posixreg.c
:long4ok

if "%1"=="GCC"   goto ok_gcc
if "%1"=="gcc"   goto ok_gcc
if "%1"=="nmake" goto ok_msc
if "%1"=="NMAKE" goto ok_msc
if "%1"=="BC"   goto ok_bc
if "%1"=="bc"   goto ok_bc
if "%1"=="MSC"   goto ok_msc
if "%1"=="msc"   goto ok_msc
goto err_set

:ok_gcc
echo Symbolic links, msdos style
echo "Makefile.GCC -> ..\..\src\makefile"
copy makefile.GCC ..\..\src\makefile
goto done

:ok_msc
echo Copying Makefile for Microsoft C and Microsoft NMAKE.
echo "Makefile.MSC -> ..\..\src\makefile"
copy Makefile.MSC ..\..\src\makefile
echo Copying overlay schemas to ..\..\src
copy schema*.MSC ..\..\src\schema*.DEF
:ok_cl
goto done

:ok_bc
echo Copying Makefile for Borland C and Borland's MAKE.
echo "Makefile.BC -> ..\..\src\makefile"
copy Makefile.BC ..\..\src\makefile
echo Copying overlay schemas to ..\..\src
copy schema*.BC ..\..\src
goto done

:err_long
echo.
echo ** ERROR - New file system with "long file name support" problem. **
echo A couple of NetHack distribution files that are packaged with 
echo a long filename ( exceeds 8.3) appear to be missing from your 
echo distribution.
echo The following files need to exist under the names on the
echo right in order to build NetHack:
echo.
echo  ..\..\dat\data.base           needs to be copied to ..\..\dat\data.bas
echo  ..\..\include\patchlevel.h    needs to be copied to ..\..\include\patchlev.h
echo  ..\..\sys\share\posixregex.c  needs to be copied to ..\..\sys\share\posixreg.c
echo.
echo setup.bat was unable to perform the necessary changes because at least
echo one of the files doesn't exist under its short name, and the 
echo original (long) file name to copy it from was not found either.
echo.
goto end

:err_set
echo.
echo Usage:
echo "%0 <GCC | MSC | BC >"
echo.
echo    Run this batch file specifying on of the following:
echo            GCC, MSC, BC
echo.
echo    (depending on which compiler and/or make utility you are using).
echo.
echo    The GCC argument is for use with djgpp and the NDMAKE utility.
echo.
echo    The MSC argument is for use with Microsoft C and the NMAKE utility
echo    that ships with it (MSC 7.0 or greater only, including Visual C).
echo.
echo    The BC argument is for use with Borland C and Borland's MAKE utility
echo    that ships with it (Borland C++ 3.1 only).
echo.
goto end

:err_dir
echo/
echo Your directories are not set up properly, please re-read the
echo Install.dos and README documentation.
goto end

:done
echo Setup Done!
echo Please continue with next step from Install.dos.

:end

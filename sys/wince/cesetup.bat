@REM  SCCS Id: @(#)nhsetup.bat	$Date$
@REM  Copyright (c) Alex Kompel, 2002
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 nhsetup batch file, see Install.ce for details
@REM
@echo off
REM
REM  Make sure directories necessary for build exist
REM
if NOT exist ..\..\wince\*.* mkdir ..\..\wince
if NOT exist ..\..\wince\ceinc\*.* mkdir ..\..\wince\ceinc
if NOT exist ..\..\wince\ceinc\sys\*.* mkdir ..\..\wince\ceinc\sys
REM
REM  Get these files from the win\win32 port
REM
copy ..\..\win\win32\mnsel.uu    ..\..\wince\mnsel.uu
copy ..\..\win\win32\mnselcnt.uu ..\..\wince\mnselcnt.uu
copy ..\..\win\win32\mnunsel.uu  ..\..\wince\mnunsel.uu
copy ..\..\win\win32\petmark.uu  ..\..\wince\petmark.uu
copy ..\..\sys\winnt\nhico.uu    ..\..\wince\nhico.uu
REM
REM  Get these files from sys\wince
REM
copy bootstrp.mak ..\..\wince\bootstrp.mak
copy wince.vcw    ..\..\wince.vcw
copy keypad.uu    ..\..\wince\keypad.uu
copy assert.h     ..\..\wince\assert.h
copy assert.h     ..\..\wince\ceinc\assert.h
copy errno.h      ..\..\wince\ceinc\errno.h
copy fcntl.h      ..\..\wince\ceinc\fcntl.h
copy stat.h       ..\..\wince\ceinc\sys\stat.h
copy celib.c      ..\..\wince\celib.c
copy mhaskyn.c    ..\..\wince\mhaskyn.c
copy mhaskyn.h    ..\..\wince\mhaskyn.h
copy mhcmd.c      ..\..\wince\mhcmd.c
copy mhcmd.h      ..\..\wince\mhcmd.h
copy mhcolor.c    ..\..\wince\mhcolor.c
copy mhcolor.h    ..\..\wince\mhcolor.h
copy mhdlg.c      ..\..\wince\mhdlg.c
copy mhdlg.h      ..\..\wince\mhdlg.h
copy mhfont.c     ..\..\wince\mhfont.c
copy mhfont.h     ..\..\wince\mhfont.h
copy mhinput.c    ..\..\wince\mhinput.c
copy mhinput.h    ..\..\wince\mhinput.h
copy mhmain.c     ..\..\wince\mhmain.c
copy mhmain.h     ..\..\wince\mhmain.h
copy mhmap.c      ..\..\wince\mhmap.c
copy mhmap.h      ..\..\wince\mhmap.h
copy mhmenu.c     ..\..\wince\mhmenu.c
copy mhmenu.h     ..\..\wince\mhmenu.h
copy mhmsg.h      ..\..\wince\mhmsg.h
copy mhmsgwnd.c   ..\..\wince\mhmsgwnd.c
copy mhmsgwnd.h   ..\..\wince\mhmsgwnd.h
copy mhrip.c      ..\..\wince\mhrip.c
copy mhrip.h      ..\..\wince\mhrip.h
copy mhstatus.c   ..\..\wince\mhstatus.c
copy mhstatus.h   ..\..\wince\mhstatus.h
copy mhtext.c     ..\..\wince\mhtext.c
copy mhtext.h     ..\..\wince\mhtext.h
copy mswproc.c    ..\..\wince\mswproc.c
copy newres.h     ..\..\wince\newres.h
copy resource.h   ..\..\wince\resource.h
copy hpc.vcp      ..\..\wince\wince_hpc.vcp
copy palmpc.vcp   ..\..\wince\wince_palm_pc.vcp
copy pocketpc.vcp ..\..\wince\wince_pocket_pc.vcp
copy smartphn.vcp ..\..\wince\wince_smartphone.vcp
copy winhack.c    ..\..\wince\winhack.c
copy winhack.rc   ..\..\wince\winhack.rc
copy winhcksp.rc  ..\..\wince\winhack_sp.rc
copy winmain.c    ..\..\wince\winmain.c
copy winMS.h      ..\..\wince\winMS.h
copy cesound.c    ..\..\wince\cesound.c
echo.
echo Proceed with the following steps:
echo.
echo        cd ..\..\wince
echo        nmake /f bootstrp.mak
echo.
echo Then start Embedded Visual C and open 
echo the workspace wince.vcw (at the top of the NetHack tree)
echo to build.  See Install.ce for details.
echo.


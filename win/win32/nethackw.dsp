# Microsoft Developer Studio Project File - Name="NetHackW" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=NetHackW - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NetHackW.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NetHackW.mak" CFG="NetHackW - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NetHackW - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "NetHackW - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NetHackW - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Og /Oy /Ob1 /Gs /Gf /Gy /Oi- /Ot  /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /Og /Oy /Ob1 /Gs /Gf /Gy /Oi- /Ot  /I "..\win\win32" /I "..\include" /I "..\sys\winnt" /I "..\sys\share" /I "..\win\share" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "DLB" /D "MSWIN_GRAPHICS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib winmm.lib /nologo /subsystem:windows /map /debug /machine:I386 /MAPINFO:EXPORTS /MAPINFO:LINES
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
OutDir=.\Release
SOURCE="$(InputPath)"
PostBuild_Desc=Install exe
PostBuild_Cmds=copy $(OutDir)\NetHackW.exe ..\binary	\
copy ..\dat\nhdat ..\binary	\
copy ..\dat\license ..\binary	\
if exist tiles.bmp copy tiles.bmp ..\binary	\
if exist ..\doc\Guidebook.txt copy ..\doc\Guidebook.txt ..\binary\Guidebook.txt	\
if exist ..\doc\nethack.txt copy ..\doc\nethack.txt ..\binary\NetHack.txt	\
copy ..\sys\winnt\defaults.nh ..\binary\defaults.nh
# End Special Build Tool

!ELSEIF  "$(CFG)" == "NetHackW - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\win\win32" /I "..\include" /I "..\sys\winnt" /I "..\sys\share" /I "..\win\share" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "DLB" /D "MSWIN_GRAPHICS" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Debug
SOURCE="$(InputPath)"
PostBuild_Desc=Install exe
PostBuild_Cmds=if NOT exist ..\binary\*.* mkdir ..\binary	\
copy $(OutDir)\NetHackW.exe ..\binary	\
copy ..\dat\nhdat ..\binary	\
copy ..\dat\license ..\binary	\
if exist tiles.bmp copy tiles.bmp ..\binary	\
if exist ..\doc\Guidebook.txt copy ..\doc\Guidebook.txt ..\binary\Guidebook.txt	\
if exist ..\doc\nethack.txt copy ..\doc\nethack.txt ..\binary\NetHack.txt	\
copy ..\sys\winnt\defaults.nh ..\binary\defaults.nh
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "NetHackW - Win32 Release"
# Name "NetHackW - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\allmain.c
# End Source File
# Begin Source File

SOURCE=..\src\alloc.c
# End Source File
# Begin Source File

SOURCE=..\src\apply.c
# End Source File
# Begin Source File

SOURCE=..\src\artifact.c
# End Source File
# Begin Source File

SOURCE=..\src\attrib.c
# End Source File
# Begin Source File

SOURCE=..\src\ball.c
# End Source File
# Begin Source File

SOURCE=..\src\bones.c
# End Source File
# Begin Source File

SOURCE=..\src\botl.c
# End Source File
# Begin Source File

SOURCE=..\src\cmd.c
# End Source File
# Begin Source File

SOURCE=..\src\dbridge.c
# End Source File
# Begin Source File

SOURCE=..\src\decl.c
# End Source File
# Begin Source File

SOURCE=..\src\detect.c
# End Source File
# Begin Source File

SOURCE=..\src\dig.c
# End Source File
# Begin Source File

SOURCE=..\src\display.c
# End Source File
# Begin Source File

SOURCE=..\src\dlb.c
# End Source File
# Begin Source File

SOURCE=..\src\do.c
# End Source File
# Begin Source File

SOURCE=..\src\do_name.c
# End Source File
# Begin Source File

SOURCE=..\src\do_wear.c
# End Source File
# Begin Source File

SOURCE=..\src\dog.c
# End Source File
# Begin Source File

SOURCE=..\src\dogmove.c
# End Source File
# Begin Source File

SOURCE=..\src\dokick.c
# End Source File
# Begin Source File

SOURCE=..\src\dothrow.c
# End Source File
# Begin Source File

SOURCE=..\src\drawing.c
# End Source File
# Begin Source File

SOURCE=..\src\dungeon.c
# End Source File
# Begin Source File

SOURCE=..\src\eat.c
# End Source File
# Begin Source File

SOURCE=..\src\end.c
# End Source File
# Begin Source File

SOURCE=..\src\engrave.c
# End Source File
# Begin Source File

SOURCE=..\src\exper.c
# End Source File
# Begin Source File

SOURCE=..\src\explode.c
# End Source File
# Begin Source File

SOURCE=..\src\extralev.c
# End Source File
# Begin Source File

SOURCE=..\src\files.c
# End Source File
# Begin Source File

SOURCE=..\src\fountain.c
# End Source File
# Begin Source File

SOURCE=..\win\tty\getline.c
# End Source File
# Begin Source File

SOURCE=..\src\hack.c
# End Source File
# Begin Source File

SOURCE=..\src\hacklib.c
# End Source File
# Begin Source File

SOURCE=..\src\invent.c
# End Source File
# Begin Source File

SOURCE=..\src\light.c
# End Source File
# Begin Source File

SOURCE=..\src\lock.c
# End Source File
# Begin Source File

SOURCE=..\src\mail.c
# End Source File
# Begin Source File

SOURCE=..\src\makemon.c
# End Source File
# Begin Source File

SOURCE=..\src\mapglyph.c
# End Source File
# Begin Source File

SOURCE=..\src\mcastu.c
# End Source File
# Begin Source File

SOURCE=..\src\mhitm.c
# End Source File
# Begin Source File

SOURCE=..\src\mhitu.c
# End Source File
# Begin Source File

SOURCE=..\src\minion.c
# End Source File
# Begin Source File

SOURCE=..\src\mklev.c
# End Source File
# Begin Source File

SOURCE=..\src\mkmap.c
# End Source File
# Begin Source File

SOURCE=..\src\mkmaze.c
# End Source File
# Begin Source File

SOURCE=..\src\mkobj.c
# End Source File
# Begin Source File

SOURCE=..\src\mkroom.c
# End Source File
# Begin Source File

SOURCE=..\src\mon.c
# End Source File
# Begin Source File

SOURCE=..\src\mondata.c
# End Source File
# Begin Source File

SOURCE=..\src\monmove.c
# End Source File
# Begin Source File

SOURCE=..\src\monst.c
# End Source File
# Begin Source File

SOURCE=..\src\monstr.c
# End Source File
# Begin Source File

SOURCE=..\src\mplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\mthrowu.c
# End Source File
# Begin Source File

SOURCE=..\src\muse.c
# End Source File
# Begin Source File

SOURCE=..\src\music.c
# End Source File
# Begin Source File

SOURCE=..\sys\winnt\ntsound.c
# End Source File
# Begin Source File
 
SOURCE=..\src\o_init.c
# End Source File
# Begin Source File

SOURCE=..\src\objects.c
# End Source File
# Begin Source File

SOURCE=..\src\objnam.c
# End Source File
# Begin Source File

SOURCE=..\src\options.c
# End Source File
# Begin Source File

SOURCE=..\src\pager.c
# End Source File
# Begin Source File

SOURCE=..\sys\share\pcmain.c
# End Source File
# Begin Source File

SOURCE=..\sys\share\pcsys.c
# End Source File
# Begin Source File

SOURCE=..\sys\share\pcunix.c
# End Source File
# Begin Source File

SOURCE=..\src\pickup.c
# End Source File
# Begin Source File

SOURCE=..\src\pline.c
# End Source File
# Begin Source File

SOURCE=..\src\polyself.c
# End Source File
# Begin Source File

SOURCE=..\src\potion.c
# End Source File
# Begin Source File

SOURCE=..\src\pray.c
# End Source File
# Begin Source File

SOURCE=..\src\priest.c
# End Source File
# Begin Source File

SOURCE=..\src\quest.c
# End Source File
# Begin Source File

SOURCE=..\src\questpgr.c
# End Source File
# Begin Source File

SOURCE=..\sys\share\random.c
# End Source File
# Begin Source File

SOURCE=..\src\read.c
# End Source File
# Begin Source File

SOURCE=..\src\rect.c
# End Source File
# Begin Source File

SOURCE=..\src\region.c
# End Source File
# Begin Source File

SOURCE=..\src\restore.c
# End Source File
# Begin Source File

SOURCE=..\src\rip.c
# End Source File
# Begin Source File

SOURCE=..\src\rnd.c
# End Source File
# Begin Source File

SOURCE=..\src\role.c
# End Source File
# Begin Source File

SOURCE=..\src\rumors.c
# End Source File
# Begin Source File

SOURCE=..\src\save.c
# End Source File
# Begin Source File

SOURCE=..\src\shk.c
# End Source File
# Begin Source File

SOURCE=..\src\shknam.c
# End Source File
# Begin Source File

SOURCE=..\src\sit.c
# End Source File
# Begin Source File

SOURCE=..\src\sounds.c
# End Source File
# Begin Source File

SOURCE=..\src\sp_lev.c
# End Source File
# Begin Source File

SOURCE=..\src\spell.c
# End Source File
# Begin Source File

SOURCE=..\src\steal.c
# End Source File
# Begin Source File

SOURCE=..\src\steed.c
# End Source File
# Begin Source File

SOURCE=..\src\teleport.c
# End Source File
# Begin Source File

SOURCE=..\src\tile.c
# End Source File
# Begin Source File

SOURCE=..\src\timeout.c
# End Source File
# Begin Source File

SOURCE=..\src\topten.c
# End Source File
# Begin Source File

SOURCE=..\src\track.c
# End Source File
# Begin Source File

SOURCE=..\src\trap.c
# End Source File
# Begin Source File

SOURCE=..\src\u_init.c
# End Source File
# Begin Source File

SOURCE=..\src\uhitm.c
# End Source File
# Begin Source File

SOURCE=..\src\vault.c
# End Source File
# Begin Source File

SOURCE=..\src\version.c
# End Source File
# Begin Source File

SOURCE=..\src\vision.c
# End Source File
# Begin Source File

SOURCE=..\src\weapon.c
# End Source File
# Begin Source File

SOURCE=..\src\were.c
# End Source File
# Begin Source File

SOURCE=..\src\wield.c
# End Source File
# Begin Source File

SOURCE=..\src\windows.c
# End Source File
# Begin Source File

SOURCE=..\sys\winnt\winnt.c
# End Source File
# Begin Source File

SOURCE=..\win\tty\wintty.c
# End Source File
# Begin Source File

SOURCE=..\src\wizard.c
# End Source File
# Begin Source File

SOURCE=..\src\worm.c
# End Source File
# Begin Source File

SOURCE=..\src\worn.c
# End Source File
# Begin Source File

SOURCE=..\src\write.c
# End Source File
# Begin Source File

SOURCE=..\src\zap.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\align.h
# End Source File
# Begin Source File

SOURCE=..\include\amiconf.h
# End Source File
# Begin Source File

SOURCE=..\include\artifact.h
# End Source File
# Begin Source File

SOURCE=..\include\artilist.h
# End Source File
# Begin Source File

SOURCE=..\include\attrib.h
# End Source File
# Begin Source File

SOURCE=..\include\beconf.h
# End Source File
# Begin Source File

SOURCE=..\include\bitmfile.h
# End Source File
# Begin Source File

SOURCE=..\include\color.h
# End Source File
# Begin Source File

SOURCE=..\include\config.h
# End Source File
# Begin Source File

SOURCE=..\include\config1.h
# End Source File
# Begin Source File

SOURCE=..\include\coord.h
# End Source File
# Begin Source File

SOURCE=..\include\decl.h
# End Source File
# Begin Source File

SOURCE=..\include\def_os2.h
# End Source File
# Begin Source File

SOURCE=..\include\dgn_file.h
# End Source File
# Begin Source File

SOURCE=..\include\display.h
# End Source File
# Begin Source File

SOURCE=..\include\dlb.h
# End Source File
# Begin Source File

SOURCE=..\include\dungeon.h
# End Source File
# Begin Source File

SOURCE=..\include\edog.h
# End Source File
# Begin Source File

SOURCE=..\include\emin.h
# End Source File
# Begin Source File

SOURCE=..\include\engrave.h
# End Source File
# Begin Source File

SOURCE=..\include\epri.h
# End Source File
# Begin Source File

SOURCE=..\include\eshk.h
# End Source File
# Begin Source File

SOURCE=..\include\extern.h
# End Source File
# Begin Source File

SOURCE=..\include\flag.h
# End Source File
# Begin Source File

SOURCE=..\include\func_tab.h
# End Source File
# Begin Source File

SOURCE=..\include\gem_rsc.h
# End Source File
# Begin Source File

SOURCE=..\include\global.h
# End Source File
# Begin Source File

SOURCE=..\include\hack.h
# End Source File
# Begin Source File

SOURCE=..\include\lev.h
# End Source File
# Begin Source File

SOURCE=..\include\load_img.h
# End Source File
# Begin Source File

SOURCE=..\include\macconf.h
# End Source File
# Begin Source File

SOURCE=..\include\macpopup.h
# End Source File
# Begin Source File

SOURCE=..\include\mactty.h
# End Source File
# Begin Source File

SOURCE=..\include\macwin.h
# End Source File
# Begin Source File

SOURCE=..\include\mail.h
# End Source File
# Begin Source File

SOURCE=..\include\mfndpos.h
# End Source File
# Begin Source File

SOURCE=..\include\micro.h
# End Source File
# Begin Source File

SOURCE=..\include\mkroom.h
# End Source File
# Begin Source File

SOURCE=..\include\monattk.h
# End Source File
# Begin Source File

SOURCE=..\include\mondata.h
# End Source File
# Begin Source File

SOURCE=..\include\monflag.h
# End Source File
# Begin Source File

SOURCE=..\include\monst.h
# End Source File
# Begin Source File

SOURCE=..\include\monsym.h
# End Source File
# Begin Source File

SOURCE=..\include\mttypriv.h
# End Source File
# Begin Source File

SOURCE=..\include\nhlan.h
# End Source File
# Begin Source File

SOURCE=..\include\ntconf.h
# End Source File
# Begin Source File

SOURCE=..\include\obj.h
# End Source File
# Begin Source File

SOURCE=..\include\objclass.h
# End Source File
# Begin Source File

SOURCE=..\include\os2conf.h
# End Source File
# Begin Source File

SOURCE=..\include\patchlevel.h
# End Source File
# Begin Source File

SOURCE=..\include\pcconf.h
# End Source File
# Begin Source File

SOURCE=..\include\permonst.h
# End Source File
# Begin Source File

SOURCE=..\include\prop.h
# End Source File
# Begin Source File

SOURCE=..\include\qt_clust.h
# End Source File
# Begin Source File

SOURCE=..\include\qt_kde0.h
# End Source File
# Begin Source File

SOURCE=..\include\qt_win.h
# End Source File
# Begin Source File

SOURCE=..\include\qt_xpms.h
# End Source File
# Begin Source File

SOURCE=..\include\qtext.h
# End Source File
# Begin Source File

SOURCE=..\include\quest.h
# End Source File
# Begin Source File

SOURCE=..\include\rect.h
# End Source File
# Begin Source File

SOURCE=..\include\region.h
# End Source File
# Begin Source File

SOURCE=..\include\rm.h
# End Source File
# Begin Source File

SOURCE=..\include\skills.h
# End Source File
# Begin Source File

SOURCE=..\include\sp_lev.h
# End Source File
# Begin Source File

SOURCE=..\include\spell.h
# End Source File
# Begin Source File

SOURCE=..\include\system.h
# End Source File
# Begin Source File

SOURCE=..\include\tcap.h
# End Source File
# Begin Source File

SOURCE=..\include\tile2x11.h
# End Source File
# Begin Source File

SOURCE=..\include\timeout.h
# End Source File
# Begin Source File

SOURCE=..\include\tosconf.h
# End Source File
# Begin Source File

SOURCE=..\include\tradstdc.h
# End Source File
# Begin Source File

SOURCE=..\include\trampoli.h
# End Source File
# Begin Source File

SOURCE=..\include\trap.h
# End Source File
# Begin Source File

SOURCE=..\include\unixconf.h
# End Source File
# Begin Source File

SOURCE=..\include\vault.h
# End Source File
# Begin Source File

SOURCE=..\include\vision.h
# End Source File
# Begin Source File

SOURCE=..\include\vmsconf.h
# End Source File
# Begin Source File

SOURCE=..\include\winami.h
# End Source File
# Begin Source File

SOURCE=..\include\wingem.h
# End Source File
# Begin Source File

SOURCE=..\include\winGnome.h
# End Source File
# Begin Source File

SOURCE=..\include\winprocs.h
# End Source File
# Begin Source File

SOURCE=..\include\wintty.h
# End Source File
# Begin Source File

SOURCE=..\include\wintype.h
# End Source File
# Begin Source File

SOURCE=..\include\winX.h
# End Source File
# Begin Source File

SOURCE=..\include\xwindow.h
# End Source File
# Begin Source File

SOURCE=..\include\xwindowp.h
# End Source File
# Begin Source File

SOURCE=..\include\you.h
# End Source File
# Begin Source File

SOURCE=..\include\youprop.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\win\win32\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\win\win32\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=..\win\win32\mnsel.bmp
# End Source File
# Begin Source File

SOURCE=..\win\win32\mnunsel.bmp
# End Source File
# Begin Source File

SOURCE=..\win\win32\NETHACK.ICO
# End Source File
# Begin Source File

SOURCE=..\win\win32\small.ico
# End Source File
# Begin Source File

SOURCE=..\win\win32\tiles.bmp
# End Source File
# Begin Source File

SOURCE=..\win\win32\winhack.ico
# End Source File
# End Group
# Begin Group "wnd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\win\win32\mhaskyn.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhaskyn.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhdlg.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhdlg.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhfont.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhfont.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhinput.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhinput.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmain.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmain.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmap.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmap.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmenu.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmenu.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmsg.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmsgwnd.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhmsgwnd.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhsplash.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhrip.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhsplash.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhrip.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhstatus.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhstatus.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhtext.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\mhtext.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\mswproc.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\resource.h
# End Source File
# Begin Source File

SOURCE=..\win\win32\winhack.c
# End Source File
# Begin Source File

SOURCE=..\win\win32\winhack.rc
# End Source File
# Begin Source File

SOURCE=..\win\win32\winMS.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\win\win32\ReadMe.txt
# End Source File
# End Target
# End Project

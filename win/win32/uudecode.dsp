# Microsoft Developer Studio Project File - Name="uudecode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=uudecode - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "uudecode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "uudecode.mak" CFG="uudecode - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "uudecode - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "uudecode - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "uudecode - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib /nologo /subsystem:console /machine:I386 /out:"..\util\uudecode.exe"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=echo chdir ..\win\win32	chdir ..\win\win32	\
echo decoding icon (nhico.uu to NetHack.ico)	\
..\..\util\uudecode.exe ../../sys/winnt/nhico.uu	\
echo decoding mnsel (mnsel.uu to mnsel.bmp)	\
..\..\util\uudecode.exe mnsel.uu	\
echo decoding mnselcnt (mnselcnt.uu to mnselcnt.bmp)	\
..\..\util\uudecode.exe mnselcnt.uu	\
echo decoding mnunsel (mnunsel.uu to mnunsel.bmp)	\
..\..\util\uudecode.exe mnunsel.uu	\
echo decoding petmark (petmark.uu to petmark.bmp)	\
..\..\util\uudecode.exe petmark.uu	\
echo decoding splash (splash.uu to splash.bmp)	\
..\..\util\uudecode.exe splash.uu	\
echo decoding tombstone (rip.uu to rip.bmp)	\
..\..\util\uudecode.exe rip.uu	\
chdir ..\..\binary

# End Special Build Tool

!ELSEIF  "$(CFG)" == "uudecode - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ  /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib /nologo /subsystem:console /debug /machine:I386 /out:"..\util\uudecode.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=echo chdir ..\win\win32	chdir ..\win\win32	\
echo decoding icon (nhico.uu to NetHack.ico)	\
..\..\util\uudecode.exe ../../sys/winnt/nhico.uu	\
echo decoding mnsel (mnsel.uu to mnsel.bmp)	\
..\..\util\uudecode.exe mnsel.uu	\
echo decoding mnselcnt (mnselcnt.uu to mnselcnt.bmp)	\
..\..\util\uudecode.exe mnselcnt.uu	\
echo decoding mnunsel (mnunsel.uu to mnunsel.bmp)	\
..\..\util\uudecode.exe mnunsel.uu	\
echo decoding petmark (petmark.uu to petmark.bmp)	\
..\..\util\uudecode.exe petmark.uu	\
echo decoding splash (splash.uu to splash.bmp)	\
..\..\util\uudecode.exe splash.uu	\
echo decoding tombstone (rip.uu to rip.bmp)	\
..\..\util\uudecode.exe rip.uu	\
chdir ..\..\binary

# End Special Build Tool

!ENDIF 

# Begin Target

# Name "uudecode - Win32 Release"
# Name "uudecode - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\sys\share\uudecode.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

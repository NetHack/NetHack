# Microsoft Developer Studio Project File - Name="makedefs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=makedefs - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "makedefs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "makedefs.mak" CFG="makedefs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "makedefs - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "makedefs - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "makedefs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\util"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I "..\include" /I "..\sys\winnt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WIN32CON" /D "DLB" /D "MSWIN_GRAPHICS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Running makedefs
PostBuild_Cmds=echo chdir ..\util	chdir ..\util	chdir	\
echo makedefs.exe -v	makedefs.exe -v	\
echo makedefs.exe -o	makedefs.exe  -o	\
echo makedefs.exe -p	makedefs.exe -p	\
echo makedefs.exe -m	makedefs.exe -m	\
echo makedefs.exe -z	makedefs.exe -z	\
echo chdir ..\dat	chdir ..\dat	chdir	\
echo Generating NetHack database	echo ..\util\makedefs.exe -d	..\util\makedefs.exe -d	\
echo Generating rumors	echo ..\util\makedefs.exe -r	..\util\makedefs.exe -r	\
echo Generating quests	echo ..\util\makedefs.exe -q	..\util\makedefs.exe -q	\
echo Generating oracles	echo ..\util\makedefs.exe -h	..\util\makedefs.exe -h	\
echo Generating dungeon.pdf	echo ..\util\makedefs.exe -e	..\util\makedefs.exe -e	\
echo chdir ..\build	chdir ..\build	\
copy ..\win\share\tilemap.c ..\win\share\tiletxt.c
# End Special Build Tool

!ELSEIF  "$(CFG)" == "makedefs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\util"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "." /I "..\include" /I "..\sys\winnt" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "WIN32CON" /D "DLB" /D "MSWIN_GRAPHICS" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Running makedefs
PostBuild_Cmds=echo chdir ..\util	chdir ..\util	chdir	\
echo makedefs.exe -v	makedefs.exe -v	\
echo makedefs.exe -o	makedefs.exe  -o	\
echo makedefs.exe -p	makedefs.exe -p	\
echo makedefs.exe -m	makedefs.exe -m	\
echo makedefs.exe -z	makedefs.exe -z	\
echo chdir ..\dat	chdir ..\dat	chdir	\
echo Generating NetHack database	echo ..\util\makedefs.exe -d	..\util\makedefs.exe -d	\
echo Generating rumors	echo ..\util\makedefs.exe -r	..\util\makedefs.exe -r	\
echo Generating quests	echo ..\util\makedefs.exe -q	..\util\makedefs.exe -q	\
echo Generating oracles	echo ..\util\makedefs.exe -h	..\util\makedefs.exe -h	\
echo Generating dungeon.pdf	echo ..\util\makedefs.exe -e	..\util\makedefs.exe -e	\
echo chdir ..\build	chdir ..\build	\
copy ..\win\share\tilemap.c ..\win\share\tiletxt.c
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "makedefs - Win32 Release"
# Name "makedefs - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\util\makedefs.c
# End Source File
# Begin Source File

SOURCE=..\src\monst.c
# End Source File
# Begin Source File

SOURCE=..\src\objects.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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

SOURCE=..\include\global.h
# End Source File
# Begin Source File

SOURCE=..\include\monattk.h
# End Source File
# Begin Source File

SOURCE=..\include\monflag.h
# End Source File
# Begin Source File

SOURCE=..\include\monsym.h
# End Source File
# Begin Source File

SOURCE=..\include\nhlan.h
# End Source File
# Begin Source File

SOURCE=..\include\ntconf.h
# End Source File
# Begin Source File

SOURCE=..\include\objclass.h
# End Source File
# Begin Source File

SOURCE=..\include\patchlevel.h
# End Source File
# Begin Source File

SOURCE=..\include\qtext.h
# End Source File
# Begin Source File

SOURCE=..\include\tradstdc.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

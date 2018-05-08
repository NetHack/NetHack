@echo off

if "%VisualStudioVersion%"=="" (
	echo MSBuild environment not set ... attempting to setup build environment.
	call :setup_environment
)

if "%VisualStudioVersion%"=="" (
	echo Unable to setup build environment.  Exiting.
	goto :EOF
)

msbuild NetHack.sln /t:Clean;Build /p:Configuration=Debug;Platform=Win32
msbuild NetHack.sln /t:Clean;Build /p:Configuration=Debug;Platform=x64
msbuild NetHack.sln /t:Clean;Build /p:Configuration=Release;Platform=Win32
msbuild NetHack.sln /t:Clean;Build /p:Configuration=Release;Platform=x64

goto :EOF

:setup_environment


if "%VS140COMNTOOLS%"=="" (
	call :set_vs14comntools
)

if "%VS140COMNTOOLS%"=="" (
	echo Can not find Visual Studio 2015 Common Tools path.
	echo Set VS140COMNTOOLS appropriately.
	goto :EOF
)

call "%VS140COMNTOOLS%VsMSBuildCmd.bat"
cd %~dp0

goto :EOF

:set_vs14comntools

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\Tools" (
	set "VS140COMNTOOLS=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\Tools\"
)

goto :EOF

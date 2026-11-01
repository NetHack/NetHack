@echo off
setlocal

cd /d "%~dp0"

:: Load VS environment once
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul

echo.
echo ============================================================
echo   RighteousHack build menu
echo ============================================================
echo.
echo   "1) Full clean + package     (use only when you changed sys/winnt files,"
echo   "                            added new source files, or want to be 100% safe)"
echo   "2) Quick rebuild            (default - just recompile changed .c files)"
echo   "3) Just run the game        (no compile - fastest to testing)"
echo.
choice /c 123 /n /m "Choose [1-3]: "

REM ─────────────────────────────────────────────────────────────
if %errorlevel%==1 (
    echo.
    echo *** Full clean + package ***
    call :refresh_makefile
    if errorlevel 1 goto :done
    cd src
    nmake -f Makefile clean
    nmake -f Makefile package
    goto :done
)

if %errorlevel%==2 (
    echo.
    echo "*** Quick rebuild (only changed files) ***"
    call :refresh_makefile
    if errorlevel 1 goto :done
    cd src
    nmake -f Makefile
    goto :done
)

if %errorlevel%==3 (
    echo.
    echo "*** Launching the game (no build) ***"
    if exist binary\nethack.exe (
        echo Starting nethack.exe ...
        start "" "binary\nethack.exe" -D -u prophet
    ) else (
        echo No binary\nethack.exe found – doing a quick build first...
        call :refresh_makefile
        if errorlevel 1 goto :done
        cd src
        nmake -f Makefile
    )
    goto :done
)

:refresh_makefile
REM src\Makefile is not in git. nhsetup.bat copies Makefile.nmake there.
REM A copy left over from the 3.7 tree still lists win\share\safeproc.c,
REM which NetHack 5.0 removed, and nmake then stops with U1073.
echo Refreshing src\Makefile from sys\windows\Makefile.nmake
copy /Y "%~dp0sys\windows\Makefile.nmake" "%~dp0src\Makefile" >nul
if errorlevel 1 (
    echo Failed to copy sys\windows\Makefile.nmake to src\Makefile
    exit /b 1
)
exit /b 0

:done
echo.
if %errorlevel%==0 (
    echo ========== BUILD / LAUNCH SUCCEEDED ==========
) else (
    echo ========== BUILD FAILED ==========
)
echo.
echo Press any key to exit...
pause >nul
endlocal
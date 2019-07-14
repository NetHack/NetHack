echo off

SETLOCAL ENABLEEXTENSIONS
SETLOCAL ENABLEDELAYEDEXPANSION

set STEP_SIZE=5000
set FINAL_MOVE=500000
set START_MOVE=5000

set BIN_DIR=..\..\..\..\bin\Debug\Win32
set SAVED_GAME=%USERNAME%-wizard.NetHack-saved-game
set LOG_FILE=%BIN_DIR%\runtil.log
set FUZZER_LOG=%BIN_DIR%\fuzzer.log
set FUZZER_DIR=%BIN_DIR%\fuzzer
set SAVE_DIR=%FUZZER_DIR%\save
set BASELINE=%FUZZER_DIR%\fuzzer.log

if exist %FUZZER_DIR% rmdir /s /q %FUZZER_DIR%

mkdir %FUZZER_DIR%
mkdir %SAVE_DIR%

for /L %%i in (%START_MOVE%, %STEP_SIZE%, %FINAL_MOVE%) do (

    call runtill.bat %%i
	
	if ERRORLEVEL 1 (
	    echo FAILED getting running to %%i.
		exit /b 1
	)

)

echo SUCCESS.


